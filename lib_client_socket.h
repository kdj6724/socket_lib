#ifndef __LIB_SERVER_SOCKET_H__
#define __LIB_SERVER_SOCKET_H__
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "lib_queue.h"

struct SocketLibinfo {
	int ipAddr;
	int port;
	int serverSocketFd;
	int clientSocketFd;
	struct sockaddr_in serverAddr;
	struct sockaddr_in clientAddr;
	pthread_t sendThread;
	pthread_t receiveThread;
	pthread_mutex_t lock;
	void (*forwardingFn)(unsigned char* readData, int readLen);
	struct QueueType sendQueue;
	struct QueueType receiveQueue;
};

int lib_server_socket_init(struct SocketLibinfo* socket);
int lib_server_socket_run(struct SocketLibinfo* socket);
void lib_server_socket_stop(struct SocketLibinfo* socket);
int lib_send_data(struct SocketLibinfo* socket, unsigned char* data, int len);
#endif	// __LIB_SERVER_SOCKET_H__
