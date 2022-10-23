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

#include "acamera_firmware_config.h"
#include "acamera_fw.h"

uint32_t acamera_fsm_util_is_irq_event_ignored( fsm_irq_mask_t *p_mask, uint8_t irq_event )
{
    uint32_t ignore_irq = 1;
    uint32_t fsm_mask;
    uint32_t mask = 1 << irq_event;
    fsm_mask = p_mask->irq_mask & mask;
    if ( fsm_mask ) {
        p_mask->irq_mask &= ( ~fsm_mask ) | ( p_mask->repeat_irq_mask );
        ignore_irq = 0;
    }

    return ignore_irq;
}

uint32_t acamera_fsm_util_get_cur_frame_id( fsm_common_t *p_cmn )
{
    uint32_t cur_frame_id = 0;
    cur_frame_id = acamera_isp_isp_global_dbg_frame_cnt_ctx0_read( p_cmn->isp_base );
    return cur_frame_id;
}
