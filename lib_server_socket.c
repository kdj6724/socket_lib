#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <sys/epoll.h>

#include "lib_server_socket.h"

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
	if (bind(sd, (struct sockaddr*)&serverAddr, sizeof(struct sockaddr_in)) < 0) {
		perror("[socket-lib] socket bind error");
		return -1;
	}

	if (listen(sd, backlog) < 0) {
		perror("[socket-lib] socket listen error");
		return -1;
	}

	return sd;
}

static int create_udp_socket(int host, int port, int backlog) {
	return 0;
}

static void* lib_send_thread(void *data) {
	int readLen = 0;
	struct SocketLibinfo* socket = (struct SocketLibinfo*)data;
	unsigned char buffer[LIB_QUEUE_MAX_DATA_LENGTH];

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

	while(1) {
		memset(buffer, 0, LIB_QUEUE_MAX_DATA_LENGTH);
		readLen = lib_dequeue(&socket->sendQueue, buffer);
		if (readLen <= 0) {
			usleep(500000);
		} else {
			pthread_mutex_lock(&socket->lock);
			printf("[kdj6724] %s(%d) %d %c\n", __func__, __LINE__, readLen, buffer[0]);
			write(socket->connectedFd, buffer, readLen);
			pthread_mutex_unlock(&socket->lock);
		}
	}
}

#define LIB_EPOLL_SIZE 20
static void* lib_receive_thread(void *data) {
	struct SocketLibinfo* socket = (struct SocketLibinfo*)data;
	unsigned char buffer[LIB_QUEUE_MAX_DATA_LENGTH];
	int epollFd;
	struct epoll_event ev;
	struct epoll_event events[LIB_EPOLL_SIZE];
	int res = -1;
	int i = 0;

	socket->socketFd = create_tcp_socket(socket->ipAddr, socket->port, 5);

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

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

	while(1) {
		res = epoll_wait(epollFd, events, LIB_EPOLL_SIZE, 1000);
		if (res < 0) {
			perror("[socket-lib] epoll_wait error");
			goto out;
		}
		for (i=0; i<res; i++) {
			if (events[i].data.fd == socket->socketFd) {
				int addrSize;
				addrSize = sizeof(socket->clientAddr);
				socket->connectedFd = accept(socket->socketFd,
						(struct sockaddr*)&socket->clientAddr,	
						(socklen_t*)&addrSize);
				ev.events = EPOLLIN;
				ev.data.fd = socket->connectedFd;
				if (epoll_ctl(epollFd, EPOLL_CTL_ADD, socket->connectedFd, &ev)) {
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

int lib_server_socket_init(struct SocketLibinfo* socket) {
	lib_queue_init(&socket->sendQueue);
	lib_queue_init(&socket->receiveQueue);
	pthread_mutex_init(&socket->lock, NULL);

	return 0;
}

int lib_server_socket_run(struct SocketLibinfo* socket) {
	int res = -1;
	int status;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	res = pthread_create(&socket->sendThread, &attr, lib_send_thread, (void*)socket);
	if (res < 0) {
		perror("[socket-lib] socket send thread error");
		return -1;
	}

	res = pthread_create(&socket->receiveThread, &attr, lib_receive_thread, (void*)socket);
	if (res < 0) {
		perror("[socket-lib] socket receive thread error");
		goto receive_err;
	}
	pthread_attr_destroy(&attr);

	return 0;

receive_err:
	pthread_attr_destroy(&attr);
	pthread_cancel(socket->sendThread);
	pthread_join(socket->sendThread, (void**)&status);

	return res;
}

void lib_server_socket_stop(struct SocketLibinfo* socket) {
	int status;

	pthread_cancel(socket->sendThread);
	pthread_join(socket->sendThread, (void**)&status);
	pthread_cancel(socket->receiveThread);
	pthread_join(socket->receiveThread, (void**)&status);

	lib_queue_remove(&socket->sendQueue);
	lib_queue_remove(&socket->receiveQueue);
	close(socket->connectedFd);
	close(socket->socketFd);
}

