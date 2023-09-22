#include <sys/msg.h>
#include <sys/threads.h>
#include <monitor/get_mdata_q.h>
#include <monitor/empty_full_mbuffer.h>
#include <phoenix/monitor.h>
#include <phoenix/errno.h>

#include "files.h"
#include "socket_connection.h"

#define MAINTHR_PRIOROTY 5
#define DQTHR_PRIOROTY   7

struct {
	unsigned port;

	m_data mdata_qcpy[RTQ_MAXSIZE];

	struct {
#define MBUFF(NAME, TYPE, SIZE) m_data NAME[SIZE];
		MBUFFERS()
#undef MBUFF
	} odq_mbuffer_cpy;

	char stack[0x1000] __attribute__((aligned(8)));
} monitorsrv_common;


static int fail(const char *str)
{
	printf("monitorsrv fail: %s\n", str);
	return EOK;
}

m_data *get_odq_mbuffer_cpy(unsigned ebuff)
{
	switch (ebuff) {
#define MBUFF(NAME, TYPE, SIZE) \
	case mbuff_##NAME: return &monitorsrv_common.odq_mbuffer_cpy.NAME;
		MBUFFERS()
#undef MBUFF
		default: return NULL;
	}
}

void monitorsrv_dq_thr()
{
	int rtq_size;
	int mbuff_size;
	m_data *mbuffer;

	for (;;) {
		if ((rtq_size = get_mdata_q(&monitorsrv_common.mdata_qcpy)))
			for (int i = 0; i < rtq_size; ++i)
				realtime_write(&monitorsrv_common.mdata_qcpy[i]);

		for (int ebuff = 0; ebuff < mbuff_end; ++ebuff) {
			mbuffer = get_odq_mbuffer_cpy(ebuff);
			if ((mbuff_size = empty_full_mbuffer(ebuff, mbuffer)) > 0)
				ondemand_write(mbuffer, ebuff, mbuff_size);
		}
	}
}

void monitorsrvthr()
{
	unsigned long rid;
	msg_t msg;

	priority(MAINTHR_PRIOROTY);

	for (;;) {
		if (!(msgRecv(monitorsrv_common.port, &msg, &rid) < 0)) {
			switch (msg.type) {
				case monReadOnDemandData:
					ondemand_read(&msg.i.raw);
					break;
				default: break;
			}
			msgRespond(monitorsrv_common.port, &msg, rid);
		}
	}
}

void main(int argc, char **argv)
{
	int err = EOK;
	printf("monitorsrv: starting server\n");

	// Create port and pass it to monitor kernel module
	if ((err = portCreate(&monitorsrv_common.port)) < 0) {
		fail("port create");
		return err;
	}
	printf("monitorsrv: port created: %u\n", monitorsrv_common.port);

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
