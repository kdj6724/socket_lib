#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <sys/epoll.h>

#include "lib_client_socket.h"

static int create_tcp_socket(int host, int port, int backlog) {
	int sd;
	struct sockaddr_in serverAddr;

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
		perror("[socket-lib] socket error");
		return -1;
	}
	bzero((char *)&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(host);
	serverAddr.sin_port = htons(port);
	if (connect(sd, (struct sockaddr*)&serverAddr, sizeof(struct sockaddr_in)) < 0) {
		perror("[socket-lib] socket connect error");
		return -1;
	}

	return sd;
}

static int create_udp_socket(int host, int port, int backlog) {
	return 0;
}

int sendThreadRun_ = 1;
static void* lib_send_thread(void *data) {
	int readLen = 0;
	struct SocketLibinfo* socket = (struct SocketLibinfo*)data;
	unsigned char buffer[LIB_QUEUE_MAX_DATA_LENGTH];

	while(sendThreadRun_) {
		memset(buffer, 0, LIB_QUEUE_MAX_DATA_LENGTH);
		readLen = lib_dequeue(&socket->sendQueue, buffer);
		if (readLen <= 0) {
			usleep(500000);
		} else {
			pthread_mutex_lock(&socket->lock);
			write(socket->socketFd, buffer, readLen);
			pthread_mutex_unlock(&socket->lock);
		}
	}
}

int receiveThreadRun_ = 1;
#define LIB_EPOLL_SIZE 20
static void* lib_receive_thread(void *data) {
	struct SocketLibinfo* socket = (struct SocketLibinfo*)data;
	unsigned char buffer[LIB_QUEUE_MAX_DATA_LENGTH];
	int epollFd;
	struct epoll_event ev;
	struct epoll_event events[LIB_EPOLL_SIZE];
	int res = -1;
	int i = 0;

	epollFd = epoll_create(100);
	if (epollFd < 0) {
		perror("[socket-lib] epoll_create error ");
		return NULL;
	}
	ev.events = EPOLLIN;
	ev.data.fd = socket->socketFd;
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, socket->socketFd, &ev)) {
		perror("[socket-lib] epoll_ctl error");
		goto out;
	}

	while(receiveThreadRun_) {
		res = epoll_wait(epollFd, events, LIB_EPOLL_SIZE, 1000);
		if (res < 0) {
			perror("[socket-lib] epoll_wait error");
			goto out;
		}
		for (i=0; i<res; i++) {
			if (events[i].data.fd == socket->socketFd) {
				int addrSize;
				addrSize = sizeof(socket->clientAddr);
				ev.events = EPOLLIN;
				ev.data.fd = socket->socketFd;
				if (epoll_ctl(epollFd, EPOLL_CTL_ADD, socket->socketFd, &ev)) {
					perror("[socket-lib] client epoll_ctl error");
					goto out;
				}
			} else if (events[i].events & EPOLLIN) {
				int readByte = 0;
				if (events[i].data.fd < 0)
					continue;
				memset(buffer, 0, sizeof(buffer));
				readByte = recv(events[i].data.fd, buffer,
						LIB_QUEUE_MAX_DATA_LENGTH, 0);
				lib_enqueue(&socket->receiveQueue, buffer, readByte);
				if (readByte <= 0) {
					epoll_ctl(epollFd, EPOLL_CTL_DEL, events[i].data.fd, events);
					close(events[i].data.fd);
					events[i].data.fd = -1;
					printf("[socket-lib] client disconnected %d \n", readByte);
					continue;
				} else {
					if (socket->forwardingFn)
						socket->forwardingFn(buffer, readByte);
				}
			} else {
				perror("[socket-lib] epoll unexped event");
			}
		}
	}

out:
	close(epollFd);

	return NULL;
}

int lib_send_data(struct SocketLibinfo* socket, unsigned char* data, int len) {
	int leftLen = len;
	pthread_mutex_lock(&socket->lock);
	
	while (leftLen > 0) {
		int sendLen = 0;
		
		if (leftLen > LIB_QUEUE_MAX_DATA_LENGTH)
			sendLen = LIB_QUEUE_MAX_DATA_LENGTH;
		else 
			sendLen = leftLen;
		leftLen = leftLen - sendLen;
		lib_enqueue(&socket->sendQueue, data, sendLen);
		data = data + sendLen;
	}

	pthread_mutex_unlock(&socket->lock);
	return len - leftLen;
}

int lib_client_socket_init(struct SocketLibinfo* socket) {
	lib_queue_init(&socket->sendQueue);
	lib_queue_init(&socket->receiveQueue);
	pthread_mutex_init(&socket->lock, NULL);
	socket->socketFd = create_tcp_socket(INADDR_ANY, socket->port, 5);

	return 0;
}

int lib_client_socket_run(struct SocketLibinfo* socket) {
	int res = -1;
	int status;

	sendThreadRun_ = 1;
	res = pthread_create(&socket->sendThread, NULL, lib_send_thread, (void*)socket);
	if (res < 0) {
		perror("[socket-lib] socket send thread error");
		return -1;
	}

	receiveThreadRun_ = 1;
	res = pthread_create(&socket->receiveThread, NULL, lib_receive_thread, (void*)socket);
	if (res < 0) {
		perror("[socket-lib] socket receive thread error");
		goto receive_err;
	}

	return 0;

receive_err:
	pthread_join(socket->sendThread, (void**)&status);

	return res;
}

void lib_client_socket_stop(struct SocketLibinfo* socket) {
	int status;

	sendThreadRun_ = 0;
	pthread_join(socket->sendThread, (void**)&status);
	receiveThreadRun_ = 0;
	pthread_join(socket->receiveThread, (void**)&status);

	close(socket->socketFd);
	lib_queue_remove(&socket->sendQueue);
	lib_queue_remove(&socket->receiveQueue);
}

