#ifndef __LIB_SERVER_SOCKET_H__
#define __LIB_SERVER_SOCKET_H__
#include <pthread.h>
#include <sys/socket.h>

struct NodeType {
        unsigned char* data;
        int len;
        struct NodeType* next;
};

struct QueueType {
        pthread_mutex_t mutex;
        struct NodeType* front;
        struct NodeType* rear;
};

struct SocketLibinfo {
	int port;
	int serverSocketFd;
	int clientSocketFd;
	struct sockaddr_in serverAddr;
	struct sockaddr_in clientAddr;
	pthread_t sendThread;
	pthread_t receiveThread;
	pthread_mutex_t lock;
	void (*forwardingFn)(unsigned char* readData, int readLen);
	struct QueueType* queue;
};
#endif	// __LIB_SERVER_SOCKET_H__
