#include <sys/msg.h>
#include <sys/threads.h>
#include <monitor/monitorsrv_set_port.h>
#include <monitor/run_monitor_dqthr.h>
#include <monitor/get_mdata_q.h>
#include <monitor/get_mbuffer_q.h>
#include <phoenix/monitor.h>
#include <phoenix/errno.h>

#include "files.h"
#include "socket_connection.h"

#define MAINTHR_PRIOROTY 5
#define DQTHR_PRIOROTY   7

struct {
	unsigned port;
	handle_t lock;

	m_data *mdata_qcpy;
	m_buffer *mbuffer_qcpy;

	char stack[0x1000] __attribute__((aligned(8)));
} monitorsrv_common;


static int fail(const char *str)
{
	printf("monitorsrv fail: %s\n", str);
	return EOK;
}

void monitorsrv_dq_thr()
{
	int rtq_size, odq_size;
	m_buffer *mbuff_ptr;

	for (;;) {
		if ((rtq_size = get_mdata_q(monitorsrv_common.mdata_qcpy))) {
			for (int i = 0; i < rtq_size; ++i) {
				realtime_write(&monitorsrv_common.mdata_qcpy[i]);
			}
		}

		if ((odq_size = get_mbuffer_q(monitorsrv_common.mbuffer_qcpy))) {
			for (int i = 0; i < odq_size; ++i) {
				mbuff_ptr = &monitorsrv_common.mbuffer_qcpy[i];
				ondemand_write(mbuff_ptr->buffer, mbuff_ptr->id, mbuff_ptr->size);
			}
		}
	}
}

void monitorsrvthr()
{
	unsigned long rid;
	msg_t *msg;

	priority(MAINTHR_PRIOROTY);

	for (;;) {
		msg = malloc(sizeof(*msg));
		if (msgRecv(monitorsrv_common.port, msg, &rid) < 0) {
			continue;
		}

		switch (msg->type) {
			case monRealTimeData:
				realtime_write(msg->i.data);
				break;
			case monOnDemandData:
				if (msg->i.data != NULL) {
					ondemand_write(msg->i.data, msg->i.mbuff_data.id, msg->i.mbuff_data.size);
				}
				break;
			case monReadOnDemandData:
				ondemand_read(&msg->i.raw);
				break;
			default: break;
		}
		msgRespond(monitorsrv_common.port, msg, rid);

		free(msg);
	}
}

void main(int argc, char **argv)
{
	int err = EOK;
	printf("monitorsrv: starting server\n");
	mutexCreate(&monitorsrv_common.lock);

	monitorsrv_common.mdata_qcpy = malloc(sizeof(m_data) * RTQ_MAXSIZE);
	monitorsrv_common.mbuffer_qcpy = malloc(sizeof(m_buffer) * ODQ_MAXSIZE);

	// Create port and pass it to monitor kernel module
	if ((err = portCreate(&monitorsrv_common.port)) < 0) {
		fail("port create");
		return err;
	}
	printf("monitorsrv: port created: %u\n", monitorsrv_common.port);
	monitorsrv_set_port(monitorsrv_common.port);

	if ((err = _monitor_file_init()) < 0) {
		fail("monitor file init");
		return err;
	}

	if ((err = _sock_conn_init()) < 0) {
		fail("monitor socket init");
		return err;
	}

	// Run data queue thread as part of server
	beginthread(monitorsrv_dq_thr, DQTHR_PRIOROTY, monitorsrv_common.stack, sizeof(monitorsrv_common.stack), NULL);

	monitorsrvthr();
}
