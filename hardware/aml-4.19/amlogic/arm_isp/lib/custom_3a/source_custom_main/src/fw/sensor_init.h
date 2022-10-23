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

#ifndef __SENSOR_INIT_H__
#define __SENSOR_INIT_H__

#include "acamera_sbus_api.h"


#if SENSOR_BINARY_SEQUENCE == 1
#define sensor_load_sequence acamera_sensor_load_binary_sequence
#else
#define sensor_load_sequence acamera_sensor_load_array_sequence
#endif

typedef struct acam_reg_t {
    uint32_t address;
    uint32_t value;
    uint32_t mask;
    uint8_t len;
} acam_reg_t;

void acamera_sensor_load_binary_sequence( acamera_sbus_ptr_t p_sbus, char size, const char *sequence, int group );
void acamera_sensor_load_array_sequence( acamera_sbus_ptr_t p_sbus, char size, const acam_reg_t **sequence, int group );

#endif /* __SENSOR_INIT_H__ */
