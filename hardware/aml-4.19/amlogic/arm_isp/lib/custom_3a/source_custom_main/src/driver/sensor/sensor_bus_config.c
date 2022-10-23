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
#include "acamera_firmware_config.h"
#include "acamera_logger.h"
#include "acamera_types.h"


uint32_t bus_addr[] = { 
0x0
} ;
static uint32_t sensor_counter = 0 ;
static uint32_t lens_counter = 0 ;


int32_t get_next_sensor_bus_address(void) {
    int32_t result = 0 ;
    if ( sensor_counter < FIRMWARE_CONTEXT_NUMBER ) { 
        result = bus_addr[ sensor_counter ] ;
        sensor_counter ++ ;
    } else {
        result = -1 ;
        LOG( LOG_ERR, "Attempt to initialize more sensor instances than was configured.") ; 
    }
    return result ; 
}


int32_t get_next_lens_bus_address(void) {
    int32_t result = 0 ;
    if ( lens_counter < FIRMWARE_CONTEXT_NUMBER ) { 
        result = bus_addr[ lens_counter ] ;
        lens_counter ++ ;
    } else {
        result = -1 ;
        LOG( LOG_ERR, "Attempt to initialize more sensor instances than was configured.") ; 
    }
    return result ; 
}


void reset_sensor_bus_counter(void) {
    sensor_counter = 0 ;
}


void reset_lens_bus_counter(void) {
    lens_counter = 0 ;
}

