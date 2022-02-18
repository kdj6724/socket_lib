.SUFFIXES : .c .o 

CLIENT_LIB = clientsock
CLIENT_LIB_PATH = ../
TARGET = sockettest
CC = gcc
CFLAGS = -g 
CFLAGS += -fPIC
CFLAGS += -pthread
CFLAGS += -I../ 
LIBS = -L$(CLIENT_LIB_PATH) -l$(CLIENT_LIB)

SRCS = main.c
OBJS = $(SRCS:%.c=%.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)
	@if [ ! -f lib$(CLIENT_LIB).so ]; then \
		ln -s $(CLIENT_LIB_PATH)/lib$(CLIENT_LIB).so ./;\
	fi

clean :
	rm -rf $(OBJS) $(TARGET) lib$(CLIENT_LIB).so
