/*
 * hardware/amlogic/audio/utils/aml_buffer_provider.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 */

#define LOG_TAG "buffer_provider"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <aml_ringbuffer.h>

#define LPCM_BUFFER_SIZE       (2048*8*2)

#define GENERAL_BUFFER_SIZE    (LPCM_BUFFER_SIZE*96) //in byte

#define DDP_BUFFER_SIZE        16384 //in byte
#define DD_61937_BUFFER_SIZE   16384


struct ring_buffer general_buffer;
struct ring_buffer DDP_out_buffer;
struct ring_buffer DD_61937_buffer;

/*************************************************
Function: init_buffer_provider
Description: initialize the buffers
Input:
Output:
Return: 0 for success, otherwise fail
*************************************************/
int init_buffer_provider()
{
    memset(&general_buffer, 0, sizeof(struct ring_buffer));
    memset(&DDP_out_buffer, 0, sizeof(struct ring_buffer));
    memset(&DD_61937_buffer, 0, sizeof(struct ring_buffer));

    pthread_mutex_init(&general_buffer.lock, NULL);
    ring_buffer_init(&general_buffer, GENERAL_BUFFER_SIZE);

    pthread_mutex_init(&DDP_out_buffer.lock, NULL);
    ring_buffer_init(&DDP_out_buffer, DDP_BUFFER_SIZE);

    pthread_mutex_init(&DD_61937_buffer.lock, NULL);
    ring_buffer_init(&DD_61937_buffer, DD_61937_BUFFER_SIZE);

    return 0;
}

/*************************************************
Function: release_buffer_provider
Description: release the buffers
Input:
Output:
Return: 0 for success, otherwise fail
*************************************************/
int release_buffer_provider()
{
    ring_buffer_release(&general_buffer);
    pthread_mutex_destroy(&general_buffer.lock);

    ring_buffer_release(&DDP_out_buffer);
    pthread_mutex_destroy(&DDP_out_buffer.lock);

    ring_buffer_release(&DD_61937_buffer);
    pthread_mutex_destroy(&DD_61937_buffer.lock);

    return 0;
}

/*************************************************
Function: get_general_buffer
Description: get the general ring buffer
Input:
Output:
Return: general ring buffer
*************************************************/
struct ring_buffer *get_general_buffer()
{
    return &general_buffer;
}

/*************************************************
Function: get_DDP_buffer
Description: get the DDP ring buffer
Input:
Output:
Return: DDP ring buffer
*************************************************/
struct ring_buffer *get_DDP_buffer()
{
    return &DDP_out_buffer;
}

/*************************************************
Function: get_DD_61937_buffer
Description: get the DD ring buffer
Input:
Output:
Return: DD ring buffer
*************************************************/
struct ring_buffer *get_DD_61937_buffer()
{
    return &DD_61937_buffer;
}

#ifdef __cplusplus
}
#endif
