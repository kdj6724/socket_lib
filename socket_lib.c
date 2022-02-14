#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include "socket_lib.h"

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
}

int socket_run(struct socketLibinfo* socket) {
    int res = -1;
    int status;

    pthread_mutex_init(&socket->lock, NULL);
    socket->forwardingFn = NULL;

    res = pthread_create(&socket->sendThread, NULL, socket_send_thread, (void*)socket);
    if (res < 0) {
        perror("[socket-lib] socket send thread error");
        return -1;
    }

    res = pthread_create(&socket->receiveThread, NULL, socket_receive_thread, (void*)socket);
    if (res < 0) {
        perror("[socket-lib] socket receive thread error");
        goto receive_err;
    }

    return 0;
    
receive_err:
    pthread_join(socket->sendThread, (void**)&status);

    return res;
}

int socket_stop(struct socketLibinfo* socket) {

}