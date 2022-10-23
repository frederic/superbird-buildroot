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

#if !defined( __ACAMERA_EVENT_QUEUE_H__ )
#define __ACAMERA_EVENT_QUEUE_H__

#include "acamera.h"
#include "acamera_loop_buf.h"
#include "system_spinlock.h"

typedef struct acamera_event_queue *acamera_event_queue_ptr_t;
typedef const struct acamera_event_queue *acamera_event_queue_const_ptr_t;

typedef struct acamera_event_queue {
    sys_spinlock lock;
    acamera_loop_buf_t buf;
} acamera_event_queue_t;

static __inline void acamera_event_queue_init( acamera_event_queue_ptr_t p_queue, uint8_t *p_data_buf, int data_buf_size )
{
    acamera_loop_buffer_init( &p_queue->buf, p_data_buf, data_buf_size );
    system_spinlock_init( &p_queue->lock );
}
void acamera_event_queue_push( acamera_event_queue_ptr_t p_queue, int event );
int acamera_event_queue_pop( acamera_event_queue_ptr_t p_queue );
int32_t acamera_event_queue_empty( acamera_event_queue_ptr_t p_queue );


static __inline void acamera_event_queue_deinit( acamera_event_queue_ptr_t p_queue )
{
    system_spinlock_destroy( p_queue->lock );
}

#endif /* __ACAMERA_EVENT_QUEUE_H__ */
