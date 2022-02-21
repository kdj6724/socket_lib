#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <sys/epoll.h>

#include "lib_server_socket.h"

static int create_tcp_socket(struct SocketLibinfo* info, int backlog) {
	int sd;

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
		perror("[socket-lib] socket error");
		return -1;
	}
	if (bind(sd, (struct sockaddr*)&info->addr, sizeof(struct sockaddr_in)) < 0) {
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
	struct SocketLibinfo* info = (struct SocketLibinfo*)data;
	unsigned char buffer[LIB_QUEUE_MAX_DATA_LENGTH];

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

	while(1) {
		memset(buffer, 0, LIB_QUEUE_MAX_DATA_LENGTH);
		readLen = lib_dequeue(&info->sendQueue, buffer);
		if (readLen <= 0) {
			usleep(500000);
		} else {
			pthread_mutex_lock(&info->lock);
			write(info->connectedFd, buffer, readLen);
			pthread_mutex_unlock(&info->lock);
		}
	}
}

#define LIB_EPOLL_SIZE 20
static void* lib_receive_thread(void *data) {
	struct SocketLibinfo* info = (struct SocketLibinfo*)data;
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
	ev.data.fd = info->socketFd;
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, info->socketFd, &ev)) {
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
			if (events[i].data.fd == info->socketFd) {
				int addrSize;
				addrSize = sizeof(info->addr);
				info->connectedFd = accept(info->socketFd,
						(struct sockaddr*)&info->addr,	
						(socklen_t*)&addrSize);
				ev.events = EPOLLIN;
				ev.data.fd = info->connectedFd;
				if (epoll_ctl(epollFd, EPOLL_CTL_ADD, info->connectedFd, &ev)) {
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
				lib_enqueue(&info->receiveQueue, buffer, readByte);
				if (readByte <= 0) {
					epoll_ctl(epollFd, EPOLL_CTL_DEL, events[i].data.fd, events);
					close(events[i].data.fd);
					events[i].data.fd = -1;
					printf("[socket-lib] client disconnected %d \n", readByte);
					continue;
				} else {
					if (info->forwardingFn)
						info->forwardingFn(buffer, readByte);
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

int lib_send_data(struct SocketLibinfo* info, unsigned char* data, int len) {
	int leftLen = len;
	pthread_mutex_lock(&info->lock);
	
	while (leftLen > 0) {
		int sendLen = 0;
		
		if (leftLen > LIB_QUEUE_MAX_DATA_LENGTH)
			sendLen = LIB_QUEUE_MAX_DATA_LENGTH;
		else 
			sendLen = leftLen;
		leftLen = leftLen - sendLen;
		lib_enqueue(&info->sendQueue, data, sendLen);
		data = data + sendLen;
	}

	pthread_mutex_unlock(&info->lock);
	return len - leftLen;
}

int lib_server_socket_init(struct SocketLibinfo* info, int ip, int port) {
	lib_queue_init(&info->sendQueue);
	lib_queue_init(&info->receiveQueue);
	pthread_mutex_init(&info->lock, NULL);

	bzero((char *)&info->addr, sizeof(info->addr));
	info->addr.sin_family = AF_INET;
	info->addr.sin_addr.s_addr = ip;
	info->addr.sin_port = htons(port);

	info->socketFd = create_tcp_socket(info, 5);
	return 0;
}

int lib_server_socket_run(struct SocketLibinfo* info) {
	int res = -1;
	int status;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	res = pthread_create(&info->sendThread, &attr, lib_send_thread, (void*)info);
	if (res < 0) {
		perror("[socket-lib] socket send thread error");
		return -1;
	}

	res = pthread_create(&info->receiveThread, &attr, lib_receive_thread, (void*)info);
	if (res < 0) {
		perror("[socket-lib] socket receive thread error");
		goto receive_err;
	}
	pthread_attr_destroy(&attr);

	return 0;

receive_err:
	pthread_attr_destroy(&attr);
	pthread_cancel(info->sendThread);
	pthread_join(info->sendThread, (void**)&status);

	return res;
}

void lib_server_socket_stop(struct SocketLibinfo* info) {
	int status;

	pthread_cancel(info->sendThread);
	pthread_join(info->sendThread, (void**)&status);
	pthread_cancel(info->receiveThread);
	pthread_join(info->receiveThread, (void**)&status);

	lib_queue_remove(&info->sendQueue);
	lib_queue_remove(&info->receiveQueue);
	close(info->connectedFd);
	close(info->socketFd);
}

