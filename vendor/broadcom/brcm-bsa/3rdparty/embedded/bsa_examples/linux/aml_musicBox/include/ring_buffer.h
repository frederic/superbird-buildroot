#ifndef RING_BUFFER_H
#define RING_BUFFER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct {
	char *ring_buffer;
	char *buffer_start;
	char *buffer_end;
	unsigned int buffer_length;
	unsigned int avl_room;
	char *head;
	char *tail;
	pthread_mutex_t mutex;
} tRingBuffer;

void ring_buffer_clear(tRingBuffer *trb);
void ring_buffer_delinit(tRingBuffer *trb);

#endif
