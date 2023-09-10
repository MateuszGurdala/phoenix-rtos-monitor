#include <arpa/inet.h>
#include <sys/threads.h>
#include <phoenix/errno.h>
#include "socket_connection.h"
#include "mtype_str.h"

struct {
	struct sockaddr_in s_addr;
	int sock;
	int conn;
	int can_send;

	handle_t lock;

	char stack[0x1000] __attribute__((aligned(8)));
} sock_conn_common;

int _socksleep(int sec)
{
	int nsec = 0;
	return nsleep(&sec, &nsec);
}

int realtime_write(void *data)
{
	char *write_data = NULL;
	int data_size;
	int send;

	if (sock_conn_common.can_send) {
		data_size = mdata_to_str(&write_data, data);
		if ((send = sys_sendto(sock_conn_common.sock, write_data, data_size, 0, NULL, 0)) < 0) {
			// Lost connection, try to reconnect
			sock_conn_common.can_send = 0;
			free(write_data);
			mutexUnlock(sock_conn_common.lock);
			return send;
		}
		free(write_data);
	}

	return EOK;
}

void sockthr()
{
	// Other modules have to be loaded before the socket can be initialized
	do {
		_socksleep(RECONNECT_T_S);
		sock_conn_common.sock = sys_socket(AF_INET, SOCK_STREAM, 0);
	} while (sock_conn_common.sock < 0);

	// Main connect loop
	for (;;) {
		do {
			sock_conn_common.conn = sys_connect(sock_conn_common.sock, (struct sockaddr *)&sock_conn_common.s_addr, sizeof(struct sockaddr));
			// Socket has to be re-initiated after EISCONN/ENOTCONN return code
			if (sock_conn_common.conn < 0) {
				sys_shutdown(sock_conn_common.sock, SHUT_RDWR);
				sys_close(sock_conn_common.sock);
				sock_conn_common.sock = sys_socket(AF_INET, SOCK_STREAM, 0);
			}

			_socksleep(RECONNECT_T_S);
		} while (sock_conn_common.conn < 0);

		// Established connection, thread is halted
		sock_conn_common.can_send = 1;
		mutexLock(sock_conn_common.lock);

		// Lock is lifted, connection has to be reestablished
		sys_shutdown(sock_conn_common.sock, SHUT_RDWR);
		sys_close(sock_conn_common.sock);
		sock_conn_common.sock = sys_socket(AF_INET, SOCK_STREAM, 0);
	}
}

int _sock_conn_init()
{
	int err;
	// Initialize locks
	mutexCreate(&sock_conn_common.lock);
	mutexLock(sock_conn_common.lock);

	sock_conn_common.can_send = 0;

	sock_conn_common.s_addr.sin_family = AF_INET;
	sock_conn_common.s_addr.sin_port = htons(SERVER_PORT);
	if ((err = inet_pton(AF_INET, SERVER_IP, &sock_conn_common.s_addr.sin_addr)) != 1) {
		return err;
	}

	beginthread(sockthr, 4, sock_conn_common.stack, sizeof(sock_conn_common.stack), NULL);
	return EOK;
}