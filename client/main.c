#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lib_client_socket.h"

static void hex_dump(unsigned char* buf, int len) {
        int i;

        for(i = 0; i < len; i++) {
                        if(i != 0 && (i%16) == 0)
                                        printf("\n%02x ", buf[i]);
                        else
                                        printf("%02x ", buf[i]);
        }
}

static void test_receive(unsigned char* readData, int readLen) {
	printf("RX: %s\n", readData);
}

int main(int argc, char* argv[])
{
	char key = -1;	
	struct SocketLibinfo socket;
	int res = -1;
	char str[128];
	int port = 7070;
	int ip = inet_addr("127.0.0.1");
	int opt;

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

	lib_client_socket_init(&socket, ip, port);
	socket.forwardingFn = test_receive;
	res = lib_client_socket_run(&socket);
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
	lib_client_socket_stop(&socket);

	return 0;
}
