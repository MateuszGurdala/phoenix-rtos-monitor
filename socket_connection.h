#ifndef _SOCKET_CONNECTION_H
#define _SOCKET_CONNECTION_H

#define SERVER_IP   "10.0.2.2"
#define SERVER_PORT 3171

#define RECONNECT_T_S 1

extern int _sock_conn_init();

extern int realtime_write(void *data);

#endif