#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "queue_lib.h"

struct NodeType* lib_node_alloc(int len) {
	struct NodeType* node;
	node = malloc(sizeof(struct NodeType));
	if (node == NULL) {
		printf("[queue lib] node alloc error %s(%d) \n", __func__, __LINE__);
		return NULL;	
	}
	memset(node, 0, sizeof(struct NodeType));

	node->data = malloc(len);
	if (node->data == NULL) {
		printf("[queue lib] node data alloc error %s(%d) \n", __func__, __LINE__);
		free(node);
		return NULL;	
	}
	memset(node->data, 0, len);
	return node;
}

static void lib_node_free(struct NodeType* node) {
	free(node->data);
	free(node);
	
}

static int lib_queue_isempty(struct QueueType* queue) {
	return (queue->front == NULL);
}

static int lib_queue_isfull(struct QueueType* queue) {
	struct NodeType* test;
	
	test = lib_node_alloc(LIB_QUEUE_MAX_DATA_LENGTH);
	if (test == NULL)
		return 1;

	lib_node_free(test);
	return 0;
}

void lib_queue_make_empty(struct QueueType* queue) {
	struct NodeType* tmp;
	pthread_mutex_lock(&queue->mutex);
	while(queue->front != NULL) {
		tmp = queue->front;	
		queue->front = queue->front->next;
		lib_node_free(tmp);
	}
	pthread_mutex_unlock(&queue->mutex);
	queue->rear = NULL;
}

int lib_enqueue(struct QueueType* queue, unsigned char* data, int len) {
	if (lib_queue_isfull(queue)) {
		printf("[queue lib] full \n");
		return -1;
	} {
		struct NodeType* tmp;
		tmp = lib_node_alloc(len);
		memcpy(tmp->data, data, len);
		tmp->len = len;
		tmp->next = NULL;
		pthread_mutex_lock(&queue->mutex);
		if (queue->rear == NULL)
			queue->front = tmp;
		else
			queue->rear->next = tmp;
		
		queue->rear = tmp;
		pthread_mutex_unlock(&queue->mutex);
	}
	return len;
}

int lib_dequeue(struct QueueType* queue, unsigned char* data) {
	int deQueueLen = 0;
	if (lib_queue_isempty(queue)) {
		//printf("[queue lib] empty \n");
		return 0;
	} else {
		struct NodeType* tmp;
		pthread_mutex_lock(&queue->mutex);
		tmp = queue->front;
		memcpy(data, queue->front->data, queue->front->len);			
		deQueueLen = queue->front->len;
		queue->front = queue->front->next;
		if (queue->front == NULL)
			queue->rear = NULL;

		lib_node_free(tmp);
		pthread_mutex_unlock(&queue->mutex);
	}
	return deQueueLen;
}

void lib_queue_init(struct QueueType* queue) {
	pthread_mutex_init(&queue->mutex, NULL);
	queue->front = NULL;
	queue->rear = NULL;
}

void lib_queue_remove(struct QueueType* queue) {
	lib_queue_make_empty(queue);
}

