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

#include "acamera_logger.h"

void *system_sw_alloc( uint32_t size )
{
    return NULL;
}

void system_sw_free( void *ptr )
{
    return;
}

int32_t init_sw_io( void )
{
    int32_t result = 0;
    //
    return result;
}

int32_t close_sw_io( void )
{
    int32_t result = 0;
    return result;
}

uint32_t system_sw_read_32( uintptr_t addr )
{
    return 0;
}

uint16_t system_sw_read_16( uintptr_t addr )
{
    return 0;
}

uint8_t system_sw_read_8( uintptr_t addr )
{
    return 0;
}


void system_sw_write_32( uintptr_t addr, uint32_t data )
{
}

void system_sw_write_16( uintptr_t addr, uint16_t data )
{
}

void system_sw_write_8( uintptr_t addr, uint8_t data )
{
}
