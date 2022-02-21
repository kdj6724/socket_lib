.SUFFIXES : .c .o 

SERVER = serversock
SERVER_LIB = lib$(SERVER).so
CLIENT = clientsock
CLIENT_LIB = lib$(CLIENT).so

CC = gcc
CFLAGS = -g 
CFLAGS += -fPIC
CFLAGS += -pthread

SERVER_SRCS = lib_server_socket.c lib_queue.c
SERVER_OBJS = $(SERVER_SRCS:%.c=%.o)

CLIENT_SRCS = lib_client_socket.c lib_queue.c
CLIENT_OBJS = $(CLIENT_SRCS:%.c=%.o)

all : $(SERVER_LIB) $(CLIENT_LIB)

$(SERVER_LIB): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $^

$(CLIENT_LIB): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $^

clean :
	rm -rf $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_LIB) $(CLIENT_LIB)
