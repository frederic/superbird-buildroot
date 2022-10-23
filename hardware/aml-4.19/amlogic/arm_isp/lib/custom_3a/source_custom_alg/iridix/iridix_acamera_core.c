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

#include <string.h>

#include "iridix_acamera_log.h"
#include "iridix_acamera_core.h"

typedef struct _iridix_acamera_core_obj_ {
    uint16_t strength_target;

    uint16_t dark_enh;
    uint32_t dark_enh_avg;
    uint32_t iridix_global_DG_avg;

    uint32_t iridix_contrast;
    uint16_t iridix_global_DG;

    uint16_t mp_iridix_strength;
} iridix_acamera_core_obj_t;

static iridix_acamera_core_obj_t iridix_core_objs[FIRMWARE_CONTEXT_NUMBER];

void *iridix_acamera_core_init( uint32_t ctx_id )
{
    iridix_acamera_core_obj_t *p_iridix_core_obj = NULL;

    if ( ctx_id >= FIRMWARE_CONTEXT_NUMBER ) {
        LOG( LOG_CRIT, "Invalid ctx_id: %d, greater than max: %d.", ctx_id, FIRMWARE_CONTEXT_NUMBER - 1 );
        return NULL;
    }

    p_iridix_core_obj = &iridix_core_objs[ctx_id];
    memset( p_iridix_core_obj, 0, sizeof( *p_iridix_core_obj ) );

    p_iridix_core_obj->dark_enh_avg = IRIDIX_STRENGTH_TARGET * CALIBRATION_IRIDIX_AVG_COEF_INIT * 2;
    p_iridix_core_obj->iridix_global_DG_avg = IRIDIX_STRENGTH_TARGET * CALIBRATION_IRIDIX_AVG_COEF_INIT * 2;
    p_iridix_core_obj->iridix_contrast = 256;
    p_iridix_core_obj->iridix_global_DG = 256;
    p_iridix_core_obj->dark_enh = 15000;
    p_iridix_core_obj->strength_target = IRIDIX_STRENGTH_TARGET;

    return p_iridix_core_obj;
}

int32_t iridix_acamera_core_deinit( void *iridix_ctx )
{
    return 0;
}

int32_t iridix_acamera_core_proc( void *iridix_ctx, iridix_stats_data_t *stats, iridix_input_data_t *input, iridix_output_data_t *output )
{
    if ( !iridix_ctx || !stats || !input || !input->acamera_input || !output || !output->acamera_output ) {
        LOG( LOG_ERR, "Invalid NULL pointer, %p-%p-%p-%p-%p-%p.", iridix_ctx, stats, input, input ? input->acamera_input : NULL, output, output ? output->acamera_output : NULL );
        return -1;
    }

    iridix_acamera_core_obj_t *p_iridix_core_obj = (iridix_acamera_core_obj_t *)iridix_ctx;
    iridix_acamera_input_t *p_acamera_input = (iridix_acamera_input_t *)input->acamera_input;
    iridix_acamera_output_t *p_acamera_output = (iridix_acamera_output_t *)output->acamera_output;

    uint16_t tmp_strength_target;

    tmp_strength_target = p_acamera_input->misc_info.global_iridix_strength_target << 8;

    uint16_t smin = (uint16_t)p_acamera_input->misc_info.global_minimum_iridix_strength * 256;
    uint16_t smax = (uint16_t)p_acamera_input->misc_info.global_maximum_iridix_strength * 256;
    if ( tmp_strength_target < smin ) {
        tmp_strength_target = smin;
    }

    if ( tmp_strength_target > smax ) {
        tmp_strength_target = smax;
    }

    p_iridix_core_obj->strength_target = tmp_strength_target;

    p_acamera_output->strength_target = tmp_strength_target;
    p_acamera_output->dark_enh = p_iridix_core_obj->dark_enh;
    p_acamera_output->iridix_global_DG = p_iridix_core_obj->iridix_global_DG;
    p_acamera_output->iridix_contrast = p_iridix_core_obj->iridix_contrast;

    return 0;
}
