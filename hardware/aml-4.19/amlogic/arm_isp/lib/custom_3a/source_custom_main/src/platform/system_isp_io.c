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

#include "acamera_types.h"

int32_t pcie_init_isp_io()
{
    int32_t result = 0;
    // not needed for user firmware
    return result;
}

int32_t pcie_close_isp_io()
{
    int32_t result = 0;
    // not needed for user firmware
    return result;
}

uint32_t system_isp_read_32( uint32_t addr )
{
    uint32_t result = 0;
    // not needed for user firmware
    return result;
}

uint16_t system_isp_read_16( uint32_t addr )
{
    uint16_t result = 0;
    // not needed for user firmware
    return result;
}

uint8_t system_isp_read_8( uint32_t addr )
{
    uint8_t result = 0;
    // not needed for user firmware
    return result;
}


void system_isp_write_32( uint32_t addr, uint32_t data )
{
    // not needed for user firmware
}

void system_isp_write_16( uint32_t addr, uint16_t data )
{
    // not needed for user firmware
}

void system_isp_write_8( uint32_t addr, uint8_t data )
{
    // not needed for user firmware
}
