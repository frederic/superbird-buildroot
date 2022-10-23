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

#ifndef __system_sw_io_H__
#define __system_sw_io_H__

#include "acamera_types.h"

void *system_sw_alloc( uint32_t size );
void system_sw_free( void *ptr );
/**
 *   Read 32 bit word from isp memory
 *
 *   This function returns a 32 bits word from ISP memory with a given offset.
 *
 *   @param addr - the offset in ISP memory to read 32 bits word.
 *                 Correct values from 0 to ACAMERA_sw_MAX_ADDR
 *
 *   @return 32 bits memory value
 */
uint32_t system_sw_read_32( uintptr_t addr );


/**
 *   Read 16 bit word from isp memory
 *
 *   This function returns a 16 bits word from ISP memory with a given offset.
 *
 *   @param addr - the offset in ISP memory to read 16 bits word.
 *                 Correct values from 0 to ACAMERA_sw_MAX_ADDR
 *
 *   @return 16 bits memory value
 */
uint16_t system_sw_read_16( uintptr_t addr );


/**
 *   Read 8 bit word from isp memory
 *
 *   This function returns a 8 bits word from ISP memory with a given offset.
 *
 *   @param addr - the offset in ISP memory to read 8 bits word.
 *                 Correct values from 0 to ACAMERA_sw_MAX_ADDR
 *
 *   @return 8 bits memory value
 */
uint8_t system_sw_read_8( uintptr_t addr );


/**
 *   Write 32 bits word to isp memory
 *
 *   This function writes a 32 bits word to ISP memory with a given offset.
 *
 *   @param addr - the offset in ISP memory to write data.
 *                 Correct values from 0 to ACAMERA_sw_MAX_ADDR.
 *   @param data - data to be written
 */
void system_sw_write_32( uintptr_t addr, uint32_t data );


/**
 *   Write 16 bits word to isp memory
 *
 *   This function writes a 16 bits word to ISP memory with a given offset.
 *
 *   @param addr - the offset in ISP memory to write data.
 *                 Correct values from 0 to ACAMERA_sw_MAX_ADDR.
 *   @param data - data to be written
 */
void system_sw_write_16( uintptr_t addr, uint16_t data );


/**
 *   Write 8 bits word to isp memory
 *
 *   This function writes a 8 bits word to ISP memory with a given offset.
 *
 *   @param addr - the offset in ISP memory to write data.
 *                 Correct values from 0 to ACAMERA_sw_MAX_ADDR.
 *   @param data - data to be written
 */
void system_sw_write_8( uintptr_t addr, uint8_t data );


#endif /* __system_sw_io_H__ */
