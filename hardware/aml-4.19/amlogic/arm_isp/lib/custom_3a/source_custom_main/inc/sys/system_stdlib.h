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

#ifndef __SYSTEM_STDLIB_H__
#define __SYSTEM_STDLIB_H__

#include "acamera_types.h"


/**
 *   Copy block of memory
 *
 *   Copies the values of num bytes from
 *   the location pointed to by source directly to the memory block
 *   pointed to by destination.
 *
 *   @param   dst - pointer to the destination array where the content is to be copied
 *   @param   src - pointer to source of data to be copied
 *   @param   size - number of bytes to copy
 *
 *   @return  0 - success
 *           -1 - on error
 */

int32_t system_memcpy( void *dst, const void *src, uint32_t size );


/**
 *   Fill block of memory
 *
 *   Sets the first size bytes of the block of memory
 *   pointed by ptr to the specified value
 *
 *   @param   ptr - pointer to the block of memory to fill
 *   @param   value - valute to be set
 *   @param   size - number of bytes to be set to the value
 *
 *   @return  0 - success
 *           -1 - on error
 */

int32_t system_memset( void *ptr, uint8_t value, uint32_t size );

#endif // __SYSTEM_STDLIB_H__
