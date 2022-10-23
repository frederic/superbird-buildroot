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


int32_t get_next_sensor_bus_address(void) ;
int32_t get_next_lens_bus_address(void) ;
void reset_sensor_bus_counter(void)  ;
void reset_lens_bus_counter(void) ;
