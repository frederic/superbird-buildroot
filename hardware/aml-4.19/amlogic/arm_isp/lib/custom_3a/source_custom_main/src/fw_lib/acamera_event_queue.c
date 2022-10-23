//----------------------------------------------------------------------------
//   The confidential and proprietary information contained in this file may
//   only be used by a person authorised under and to the extent permitted
//   by a subsisting licensing agreement from ARM Limited or its affiliates.
//
//          (C) COPYRIGHT [2018] ARM Limited or its affiliates.
//              ALL RIGHTS RESERVED
//
//   This entire notice must be reproduced on all copies of this file
//   and copies of this file may only be made by a person if such person is
//   permitted to do so under the terms of a subsisting license agreement
//   from ARM Limited or its affiliates.
//----------------------------------------------------------------------------

#include "acamera_event_queue.h"
#include "acamera_logger.h"
#include "system_spinlock.h"

static void acamera_event_queue_reset( acamera_loop_buf_ptr_t p_buf )
{
    p_buf->head = p_buf->tail = 0;
}

void acamera_event_queue_push( acamera_event_queue_ptr_t p_queue, int event )
{
    int err = 0;
    acamera_loop_buf_ptr_t p_buf = &( p_queue->buf );
    unsigned long flags;

    flags = system_spinlock_lock( p_queue->lock );

    if ( ( p_buf->head < 0 ) ||
         ( p_buf->head >= p_buf->data_buf_size ) ||
         ( p_buf->tail < 0 ) ||
         ( p_buf->tail >= p_buf->data_buf_size ) ) {
        err = -1;
    }

    int pos = acamera_loop_buffer_write_u8( p_buf, 0, (uint8_t)event );
    if ( pos != p_buf->tail ) {
        p_buf->head = pos;
    } else {
        err = -2;
        acamera_event_queue_reset( p_buf );
    }

    system_spinlock_unlock( p_queue->lock, flags );

    if ( -1 == err ) {
        LOG( LOG_ERR, "Event Queue Corrupted\n" );
    } else if ( -2 == err ) {
        static uint32_t counter = 0;
        if ( !( counter++ & 0x3F ) )
            LOG( LOG_CRIT, "Event Queue overflow\n" );
    }
}

int acamera_event_queue_pop( acamera_event_queue_ptr_t p_queue )
{
    int rc = 0;
    unsigned long flags;
    acamera_loop_buf_ptr_t p_buf = &( p_queue->buf );

    flags = system_spinlock_lock( p_queue->lock );

    if ( p_buf->head == p_buf->tail ) {
        rc = -1;
    }

    if ( ( p_buf->head < 0 ) ||
         ( p_buf->head >= p_buf->data_buf_size ) ||
         ( p_buf->tail < 0 ) ||
         ( p_buf->tail >= p_buf->data_buf_size ) ) {
        rc = -2;
    }

    if ( !rc ) {
        int pos;
        rc = acamera_loop_buffer_read_u8( p_buf, 0 );

        pos = p_buf->tail + 1;
        if ( pos >= p_buf->data_buf_size ) {
            pos -= p_buf->data_buf_size;
        }

        p_buf->tail = pos;
    }

    system_spinlock_unlock( p_queue->lock, flags );

    if ( -2 == rc ) {
        LOG( LOG_ERR, "Event Queue Corrupted\n" );
    }

    return rc;
}


int32_t acamera_event_queue_empty( acamera_event_queue_ptr_t p_queue )
{
    int32_t result = 0;
    uint32_t flags = 0;
    acamera_loop_buf_ptr_t p_buf = &( p_queue->buf );
    flags = system_spinlock_lock( p_queue->lock );
    if ( p_buf->head == p_buf->tail ) {
        result = 1;
    }
    system_spinlock_unlock( p_queue->lock, flags );
    return result;
}
