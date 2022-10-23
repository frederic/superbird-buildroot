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
#include "system_control.h"
#include "acamera_logger.h"
#include "system_interrupts.h"

extern int32_t pcie_isp_irq_close();
extern int32_t pcie_i2c_close();

void bsp_init( void )
{
    system_interrupts_init();
}

void bsp_destroy( void )
{
}
