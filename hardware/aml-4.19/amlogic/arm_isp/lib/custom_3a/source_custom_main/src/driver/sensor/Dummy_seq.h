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

#if !defined(__DUMMY_SENSOR_H__)
#define __DUMMY_SENSOR_H__


/*-----------------------------------------------------------------------------
Initialization sequence - do not edit
-----------------------------------------------------------------------------*/

#include "sensor_init.h"

static acam_reg_t init[] = {
    //wait command - address is 0xFFFF
    { 0xFFFF, 20 },
    //stop sequence - address is 0x0000
    { 0x0000, 0x0000, 0x0000, 0x0000 }
};

static const acam_reg_t *seq_table[] = {
    init
};

#define SENSOR_DUMMY_SEQUENCE_DEFAULT seq_table

#define SENSOR_DUMMY_SEQUENCE_DEFAULT_INIT    0
#endif /* __DUMMY_SENSOR_H__ */
