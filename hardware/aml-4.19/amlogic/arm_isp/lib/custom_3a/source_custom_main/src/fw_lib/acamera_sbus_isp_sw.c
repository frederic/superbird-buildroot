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

#include "acamera_sbus_api.h"
#include "acamera_logger.h"
#include "system_sw_io.h"

static uint32_t isp_io_read_sample( acamera_sbus_t *p_bus, uintptr_t addr, uint8_t sample_size )
{
    if ( ( sample_size == 2 && ( addr & 1 ) ) || ( sample_size == 4 && ( addr & 3 ) ) ) {
        LOG( LOG_CRIT, "Missaligned access to IPS" );
        return 0xFFFFFFFF;
    }
    switch ( sample_size ) {
    case 1:
        return system_sw_read_8( addr );
    case 2:
        return system_sw_read_16( addr );
    }
    return system_sw_read_32( addr );
}


static void isp_io_write_sample( acamera_sbus_t *p_bus, uintptr_t addr, uint32_t sample, uint8_t sample_size )
{
    if ( ( sample_size == 2 && ( addr & 1 ) ) || ( sample_size == 4 && ( addr & 3 ) ) ) {
        LOG( LOG_CRIT, "Missaligned access to ISP" );
        return;
    }
    uintptr_t align_mask = (uintptr_t)~3;
    uintptr_t addr_aligned = addr & align_mask;
    switch ( sample_size ) {
    case 1:
        system_sw_write_32( addr_aligned, ( system_sw_read_32( addr_aligned ) & ~( 0xFF << 8 * ( addr & 3 ) ) ) | ( sample << ( 8 * ( addr & 3 ) ) ) );
        break;
    case 2:
        system_sw_write_32( addr_aligned, ( system_sw_read_32( addr_aligned ) & ~( 0xFFFF << 8 * ( addr & 2 ) ) ) | ( sample << ( 8 * ( addr & 2 ) ) ) );
        break;
    default:
        system_sw_write_32( addr, sample );
        break;
    }
}

void acamera_sbus_isp_sw_init( acamera_sbus_t *p_bus )
{
    p_bus->read_sample = isp_io_read_sample;
    p_bus->write_sample = isp_io_write_sample;
}
