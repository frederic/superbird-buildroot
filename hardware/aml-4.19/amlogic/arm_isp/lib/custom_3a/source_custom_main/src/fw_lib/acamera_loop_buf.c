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

#include "acamera_loop_buf.h"

void acamera_loop_buffer_init( acamera_loop_buf_ptr_t p_buf, uint8_t *p_data_buf, int data_buf_size )
{
    p_buf->head = p_buf->tail = 0;
    p_buf->p_data_buf = p_data_buf;
    p_buf->data_buf_size = data_buf_size;
}

uint8_t acamera_loop_buffer_read_u8( acamera_loop_buf_ptr_t p_buf, int pos )
{
    pos += p_buf->tail;
    if ( pos >= p_buf->data_buf_size ) {
        pos -= p_buf->data_buf_size;
    }
    return p_buf->p_data_buf[pos];
}

int acamera_loop_buffer_write_u8( acamera_loop_buf_ptr_t p_buf, int pos, uint8_t sample )
{
    pos += p_buf->head;
    if ( pos >= p_buf->data_buf_size ) {
        pos -= p_buf->data_buf_size;
    }
    p_buf->p_data_buf[pos++] = sample;
    if ( pos >= p_buf->data_buf_size ) {
        pos -= p_buf->data_buf_size;
    }
    return pos;
}
