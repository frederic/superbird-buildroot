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

#ifndef __ACAMERA_LOOP_BUF_H__
#define __ACAMERA_LOOP_BUF_H__

#include "acamera.h"

typedef struct _acamera_loop_buf_t acamera_loop_buf_t;
typedef struct _acamera_loop_buf_t *acamera_loop_buf_ptr_t;
typedef const struct _acamera_loop_buf_t *acamera_loop_buf_const_ptr_t;

struct _acamera_loop_buf_t {
    volatile int head;
    volatile int tail;
    uint8_t *p_data_buf;
    int data_buf_size;
};

void acamera_loop_buffer_init( acamera_loop_buf_ptr_t p_buf, uint8_t *p_data_buf, int data_buf_size );
uint8_t acamera_loop_buffer_read_u8( acamera_loop_buf_ptr_t p_buf, int pos );
int acamera_loop_buffer_write_u8( acamera_loop_buf_ptr_t p_buf, int pos, uint8_t sample );

#endif /* __ACAMERA_LOOP_BUF_H__ */
