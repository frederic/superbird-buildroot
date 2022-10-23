/*
 * Copyright (C) 2017 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _RINGBUFFER_H
#define _RINGBUFFER_H

#include <pthread.h>

typedef struct ring_buffer {
    pthread_mutex_t lock;
    unsigned char *start_addr;
    unsigned char *rd;
    unsigned char *wr;
    int size;
    int last_is_write;
} ring_buffer_t;

#define UNCOVER_WRITE    0
#define COVER_WRITE      1

size_t ring_buffer_write(struct ring_buffer *rbuffer, unsigned char* data, size_t bytes, int cover);
size_t ring_buffer_read(struct ring_buffer *rbuffer, unsigned char* buffer, size_t bytes);
int ring_buffer_init(struct ring_buffer *rbuffer, int buffer_size);
int ring_buffer_release(struct ring_buffer *rbuffer);
int ring_buffer_reset(struct ring_buffer *rbuffer);
int ring_buffer_reset_size(struct ring_buffer *rbuffer, int buffer_size);
int get_buffer_read_space(struct ring_buffer *rbuffer);
int get_buffer_write_space(struct ring_buffer *rbuffer);
void ring_buffer_dump(struct ring_buffer *rbuffer);

#endif
