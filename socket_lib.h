#include <pthread.h>
#include <sys/socket.h>

struct socketLibinfo {
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    pthread_t sendThread;
    pthread_t receiveThread;
    pthread_mutex_t lock;
    void (*forwardingFn)(unsigned char* readData, int readLen);
};