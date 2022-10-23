#include "ring_buffer.h"

int ring_buffer_init(tRingBuffer *trb, unsigned int size)
{
	if (trb == 0) {
		printf("invalid tRingBuffer\n");
		return -1;
	}

	trb->ring_buffer = malloc(size);

	if (trb->ring_buffer == 0) {
		printf("can not alloc mem\n");
		return -1;
	}
	memset(trb->ring_buffer, 0, size);
	trb->buffer_start = trb->ring_buffer;
	trb->buffer_end = trb->ring_buffer + size  - 1;
	trb->buffer_length = size;
	trb->avl_room = size;
	trb->head = trb->buffer_start;
	trb->tail = trb->buffer_start;
	pthread_mutex_init(&trb->mutex, NULL);
	printf("ring buffer created, size = %d\n", trb->buffer_length);
	return 0;
}
void ring_buffer_delinit(tRingBuffer *trb)
{
	printf("%s\n", __func__);

	if (trb == 0) {
		printf("invalid tRingBuffer\n");
		return -1;
	}

	pthread_mutex_lock(&trb->mutex);
	free(trb->ring_buffer);
	pthread_mutex_unlock(&trb->mutex);
	memset(trb, 0, sizeof(tRingBuffer));
}

int ring_buffer_pop(tRingBuffer *trb, char *data, unsigned int size)
{
	unsigned int head_used = trb->buffer_end - trb->head + 1;
	if (trb == 0 || trb->ring_buffer == 0) {
		printf("invalid tRingBuffer or ring_buffer\n");
		return 0;
	}

	pthread_mutex_lock(&trb->mutex);


	if (trb->avl_room == trb->buffer_length) {
		printf("Warinning, no available data\n");
		pthread_mutex_unlock(&trb->mutex);
		return 0;
	}


	if (trb->buffer_length - trb->avl_room < size) {
		printf("Warinning, not enough available data\n");
		size = trb->buffer_length - trb->avl_room;
	}


	if (head_used  < size) {
		unsigned int lev = size - head_used;
		memcpy(data, trb->head, head_used);
		memcpy(data + head_used, trb->buffer_start, lev);
		trb->head = trb->buffer_start + lev;
	} else {
		memcpy(data, trb->head, size);
		trb->head += size;
	}

	//printf("%d byte popoed, head = %d\n", size, trb->head - trb->buffer_start);
	trb->avl_room += size;
	pthread_mutex_unlock(&trb->mutex);

	return size;
}

int ring_buffer_push(tRingBuffer *trb, char *data, unsigned int size)
{
	unsigned int tail_free = trb->buffer_end - trb->tail + 1;

	if (trb == 0 || trb->ring_buffer == 0) {
		printf("invalid tRingBuffer or ring_buffer\n");
		return 0;
	}

	pthread_mutex_lock(&trb->mutex);
	if (trb->avl_room == 0) {
		printf("Warning, no available room\n");
		pthread_mutex_unlock(&trb->mutex);
		return 0;
	}


	if (trb->avl_room - 0 < size) {
		printf("Warinning, not enough room for data\n");
		size = trb->avl_room;
	}


	if (tail_free  < size) {
		unsigned int lev = size - tail_free;
		memcpy(trb->tail, data, tail_free);
		memcpy(trb->buffer_start, data + tail_free, lev);
		trb->tail = trb->buffer_start + lev;

	} else {
		memcpy(trb->tail, data, size);
		trb->tail += size;
	}


	//printf("%d byte pushed, tail = %d\n", size, trb->tail - trb->buffer_start);
	trb->avl_room -= size;
	pthread_mutex_unlock(&trb->mutex);

	return size;
}


void ring_buffer_clear(tRingBuffer *trb)
{
	printf("%s\n", __func__);

	if (trb == 0 || trb->ring_buffer == 0) {
		printf("invalid tRingBuffer or ring_buffer\n");
		return ;
	}

	pthread_mutex_lock(&trb->mutex);
	memset(trb->ring_buffer, 0, trb->buffer_length);
	trb->avl_room = trb->buffer_length;
	trb->head = trb->buffer_start;
	trb->tail = trb->buffer_start;
	pthread_mutex_unlock(&trb->mutex);
}


