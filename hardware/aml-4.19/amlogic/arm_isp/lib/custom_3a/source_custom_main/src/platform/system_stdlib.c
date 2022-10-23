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

#include "system_stdlib.h"
#include <stdlib.h>
#include <string.h>


int32_t system_memcpy( void *dst, const void *src, uint32_t size )
{
    int32_t result = 0;
    memcpy( dst, src, size );
    return result;
}


int32_t system_memset( void *ptr, uint8_t value, uint32_t size )
{
    int32_t result = 0;
    memset( ptr, value, size );
    return result;
}

void *system_malloc( uint32_t size )
{
    void *result = malloc( size );
    return result;
}


void system_free( void *ptr )
{
    if ( ptr )
        free( ptr );
}