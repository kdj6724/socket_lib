#include <stdio.h>
#include <unistd.h>
#include "lib_server_socket.h"

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
        hex_dump(readData, readLen);
}

int main(void)
{
	char key = -1;	
	struct SocketLibinfo socket;
	int res = -1;

	socket.port = 7070;
	lib_server_socket_init(&socket);
	socket.forwardingFn = test_receive;
	res = lib_server_socket_run(&socket);
	if (res < 0)
		printf("lib_server_socket_run error\n");

	printf("q is quit\n");
	while(1){
		key = getchar();
		if (key == 'q')
			break;
	}
	lib_server_socket_stop(&socket);

	return 0;
}
