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

#ifndef __ACAMERA_MEM_ACCESS_H__
#define __ACAMERA_MEM_ACCESS_H__

#define MEM_OFFSET_PTR( p_data, offset ) \
    ( ( (uint8_t *)( p_data ) ) + ( offset ) )

#define MEM_ALIGNED_READ( type, p_data ) \
    ( *(type *)( (uint8_t *)( p_data ) ) )

#define MEM_ALIGNED_WRITE( type, p_buf, data ) \
    ( *(type *)( (uint8_t *)( p_buf ) ) ) = ( data )

uint16_t acamera_mem_read_u16( void *p_data );
uint32_t acamera_mem_read_u32( void *p_data );
void acamera_mem_write_u16( void *p_buf, uint16_t data );
void acamera_mem_write_u32( void *p_buf, uint32_t data );

#endif /* __ACAMERA_MEM_ACCESS_H__ */
