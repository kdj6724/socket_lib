#ifndef __LIB_QUEUE_H__
#define __LIB_QUEUE_H__

#define	LIB_QUEUE_MAX_DATA_LENGTH	2048

struct NodeType {
	unsigned char* data;
	int len;
	struct NodeType* next;
};

struct QueueType {
	pthread_mutex_t mutex;
	struct NodeType* front;
	struct NodeType* rear;
};

void lib_queue_make_empty(struct QueueType* queue);
int lib_queue_en(struct QueueType* queue, unsigned char* data, int len);
int lib_queue_de(struct QueueType* queue, unsigned char* data);
void lib_queue_init(struct QueueType* queue);
void lib_queue_remove(struct QueueType* queue);

#endif	// __LIB_QUEUE_H__
