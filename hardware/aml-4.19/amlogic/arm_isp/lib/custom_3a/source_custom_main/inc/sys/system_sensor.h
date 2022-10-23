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

#ifndef __SYSTEM_SENSOR_H__
#define __SYSTEM_SENSOR_H__

#include "acamera_types.h"

/**
 *   Reset a sensor
 *
 *   This function must reset the current sensor
 *
 *   @param mask  - a parameter which will be sent from a sensor driver
 */
void system_reset_sensor( uint32_t mask );

#endif /* __SYSTEM_SENSOR_H__ */
