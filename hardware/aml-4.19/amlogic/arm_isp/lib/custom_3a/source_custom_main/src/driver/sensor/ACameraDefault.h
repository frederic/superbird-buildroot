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

#ifndef __ACAMERADEFAULT_H__
#define __ACAMERADEFAULT_H__

//#include <stdint.h>
#include "acamera_sbus_api.h"

typedef struct _sensor_ACameraDefault_iface_t {
    acamera_sbus_t mipi_i2c_bus;
} sensor_ACameraDefault_iface_t;
typedef sensor_ACameraDefault_iface_t *sensor_ACameraDefault_iface_ptr_t;

void reset_sensor_interface( sensor_ACameraDefault_iface_ptr_t p_iface );
void load_sensor_interface( sensor_ACameraDefault_iface_ptr_t p_iface, uint8_t mode );
void mipi_auto_tune( sensor_ACameraDefault_iface_ptr_t p_iface, uint8_t mode, uint32_t refw, uint32_t refh );

#endif /* __ACAMERADEFAULT_H__ */
