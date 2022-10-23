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

#ifndef _AE_CUSTOM_CORE_H_
#define _AE_CUSTOM_CORE_H_

#include "acamera_math.h"
#include "ae_standard_api.h"

typedef struct _ae_custom_input_ {
    uint32_t target_point;
    uint32_t coeff;

    uint8_t global_manual_exposure;
    uint32_t global_exposure;
    int32_t max_exposure_log2;

    const uint32_t *awb_zone_stats;
    uint32_t awb_zone_size;
} ae_custom_input_t;

typedef struct _ae_custom_output_ {
    int32_t exposure_log2;
    uint32_t exposure_ratio;
} ae_custom_output_t;

void *ae_custom_init( uint32_t ctx_id );
int32_t ae_custom_deinit( void *ae_ctx );
int32_t ae_custom_proc( void *ae_ctx, ae_stats_data_t *stats, ae_input_data_t *input, ae_output_data_t *output );
void ae_custom_expo( void *ae_ctx, int32_t corr );

#endif /* _AE_CUSTOM_CORE_H_ */