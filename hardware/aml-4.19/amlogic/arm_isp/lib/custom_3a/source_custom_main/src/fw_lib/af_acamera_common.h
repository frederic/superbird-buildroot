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

#ifndef _AF_ACAMERA_COMMON_H_
#define _AF_ACAMERA_COMMON_H_


typedef struct _af_lms_param_t {
    uint32_t pos_min_down;
    uint32_t pos_min;
    uint32_t pos_min_up;
    uint32_t pos_inf_down;
    uint32_t pos_inf;
    uint32_t pos_inf_up;
    uint32_t pos_macro_down;
    uint32_t pos_macro;
    uint32_t pos_macro_up;
    uint32_t pos_max_down;
    uint32_t pos_max;
    uint32_t pos_max_up;
    uint32_t fast_search_positions;
    uint32_t skip_frames_init;
    uint32_t skip_frames_move;
    uint32_t dynamic_range_th;
    uint32_t spot_tolerance;
    uint32_t exit_th;
    uint32_t caf_trigger_th;
    uint32_t caf_stable_th;
    uint32_t print_debug;
} af_lms_param_t;


#endif