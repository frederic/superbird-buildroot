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
#include "acamera_logger.h"
#include "system_i2c.h"


void system_i2c_init( uint32_t bus )
{
}

void system_i2c_deinit( uint32_t bus )
{
}

uint8_t system_i2c_write( uint32_t bus, uint32_t address, uint8_t *data, uint32_t size )
{
    return I2C_OK;
}

uint8_t system_i2c_read( uint32_t bus, uint32_t address, uint8_t *data, uint32_t size )
{
    return I2C_OK;
}
