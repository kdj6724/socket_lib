.SUFFIXES : .c .o 

SERVER_LIB = serversock
SERVER_LIB_PATH = ../
TARGET = server
CC = gcc
CFLAGS = -g 
CFLAGS += -fPIC
CFLAGS += -pthread
CFLAGS += -I../ 
LIBS = -L$(SERVER_LIB_PATH) -l$(SERVER_LIB)

SRCS = main.c
OBJS = $(SRCS:%.c=%.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)
	@if [ ! -f lib$(SERVER_LIB).so ]; then \
		ln -s $(SERVER_LIB_PATH)/lib$(SERVER_LIB).so ./;\
	fi

clean :
	rm -rf $(OBJS) $(TARGET) lib$(SERVER_LIB).so
