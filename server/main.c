#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lib_server_socket.h"

static void test_receive(unsigned char* readData, int readLen) {
	printf("RX: %s\n", readData);
}

int main(int argc, char* argv[])
{
	struct SocketLibinfo socket;
	int res = -1;
	char str[128];
	int opt;
	int port = 7070;
	int ip = INADDR_ANY;

	socket.ipAddr = INADDR_ANY;
	while((opt = getopt(argc, argv, "a:p:")) != -1) {
		switch(opt) {
			case 'a':
				socket.ipAddr = inet_addr(optarg);
				break;
			case 'p':
				socket.port = htons(atoi(optarg));
				break;
			case '?':
				break;
		}
	}

	lib_server_socket_init(&socket, ip, port);
	socket.forwardingFn = test_receive;
	res = lib_server_socket_run(&socket);
	if (res < 0)
		printf("lib_server_socket_run error\n");

	printf("q is quit\n");
	memset(str, 0, sizeof(str));
	while(1){
		fgets(str, sizeof(str), stdin);
		lib_send_data(&socket, str, strlen(str));
		if (str[0] == 'q' && strlen(str) == 2)
			break;
	}

	lib_server_socket_stop(&socket);

	return 0;
}
