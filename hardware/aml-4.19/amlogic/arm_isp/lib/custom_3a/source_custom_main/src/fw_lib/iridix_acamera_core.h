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

#ifndef _IRIDIX_ACAMERA_CORE_H_
#define _IRIDIX_ACAMERA_CORE_H_

#include "iridix_standard_api.h"

typedef struct _iridix_misc_info_ {

    uint8_t global_manual_iridix;
    uint8_t global_iridix_strength_target;
    uint8_t global_minimum_iridix_strength;
    uint8_t global_maximum_iridix_strength;

    int32_t cur_exposure_log2;
} iridix_misc_info_t;


typedef struct _iridix_calibration_data_ {

    uint8_t *cali_iridix_avg_coef;

    uint16_t *cali_iridix_min_max_str;

    uint32_t *cali_ae_ctrl;
    uint32_t *cali_iridix_ev_lim_no_str;
    uint32_t *cali_iridix_ev_lim_full_str;
    uint32_t *cali_iridix_strength_dk_enh_ctrl;
} iridix_calibration_data_t;


typedef struct _iridix_acamera_input_ {
    iridix_misc_info_t misc_info;
    iridix_calibration_data_t cali_data;
} iridix_acamera_input_t;


typedef struct _iridix_custom_input_ {
    uint32_t *custom_cali_data;
    uint32_t custom_cali_data_size;
} iridix_custom_input_t;


typedef struct _iridix_acamera_output_ {
    uint16_t strength_target;
    uint16_t dark_enh;
    uint16_t iridix_global_DG;
    uint32_t iridix_contrast;
} iridix_acamera_output_t;


void *iridix_acamera_core_init( uint32_t ctx_id );
int32_t iridix_acamera_core_deinit( void *iridix_ctx );
int32_t iridix_acamera_core_proc( void *iridix_ctx, iridix_stats_data_t *stats, iridix_input_data_t *input, iridix_output_data_t *output );
void iridix_acamera_core_frame_id( void *iridix_ctx, uint32_t frame_id );
void iridix_acamera_core_prev_param( void *iridix_ctx, isp_iridix_preset_t * param );

#endif
