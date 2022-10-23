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

#if !defined(__ISP_SENSOR_H__)
#define __ISP_SENSOR_H__


/*-----------------------------------------------------------------------------
Initialization sequence - do not edit
-----------------------------------------------------------------------------*/

#include "sensor_init.h"

static acam_reg_t linear[] = {
    { 0x18f98, 0x20000L, 0x70007,4 },
    { 0x18eac, 0x30L, 0x30,1 },
    { 0x1937c, 0x0L, 0xffffff,4 },
    { 0x18e8c, 0x0L, 0x3000000,4 },
    { 0x1aa3c, 0x40L, 0xffffff,4 },
    //stop sequence - address is 0x0000
    { 0x0000, 0x0000, 0x0000, 0x0000 }
};

static const acam_reg_t *seq_table[] = {
    linear
};

#define SENSOR_ISP_SEQUENCE_DEFAULT seq_table

#define SENSOR_ISP_SEQUENCE_DEFAULT_LINEAR    0
#endif /* __ISP_SENSOR_H__ */
