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

#if !defined( __FSM_UTILS_H__ )
#define __FSM_UTILS_H__

typedef struct _fsm_irq_mask_t_ {
    uint32_t irq_mask;
    uint32_t repeat_irq_mask;
} fsm_irq_mask_t;

#ifndef ARR_SIZE
#define ARR_SIZE( x ) ( sizeof( x ) / sizeof( x[0] ) )
#endif

uint32_t acamera_fsm_util_is_irq_event_ignored( fsm_irq_mask_t *p_mask, uint8_t irq_event );

uint32_t acamera_fsm_util_get_cur_frame_id( fsm_common_t *p_fsm );

#endif /* __FSM_UTILS_H__ */
