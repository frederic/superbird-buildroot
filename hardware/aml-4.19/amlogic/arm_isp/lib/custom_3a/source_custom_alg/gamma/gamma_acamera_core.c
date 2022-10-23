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

#include "gamma_acamera_log.h"
#include "gamma_standard_api.h"
#include "gamma_acamera_core.h"

typedef struct _gamma_acamera_core_obj_ {
    uint32_t cumu_hist[ISP_FULL_HISTOGRAM_SIZE];
    uint16_t gain_target;
    uint32_t gain_avg;

    uint32_t gamma_gain;
    uint32_t gamma_offset;
} gamma_acamera_core_obj_t;

static gamma_acamera_core_obj_t gamma_core_objs[FIRMWARE_CONTEXT_NUMBER];

void *gamma_acamera_core_init( uint32_t ctx_id )
{
    gamma_acamera_core_obj_t *p_gamma_core_obj = NULL;

    if ( ctx_id >= FIRMWARE_CONTEXT_NUMBER ) {
        LOG( LOG_CRIT, "Invalid ctx_id: %d, greater than max: %d.", ctx_id, FIRMWARE_CONTEXT_NUMBER - 1 );
        return NULL;
    }

    p_gamma_core_obj = &gamma_core_objs[ctx_id];
    memset( p_gamma_core_obj, 0, sizeof( *p_gamma_core_obj ) );

    p_gamma_core_obj->gain_target = 256;
    p_gamma_core_obj->gain_avg = 256;
    p_gamma_core_obj->gamma_gain = 256;
    p_gamma_core_obj->gamma_offset = 0;

    return p_gamma_core_obj;
}

int32_t gamma_acamera_core_deinit( void *gamma_ctx )
{
    return 0;
}

int32_t gamma_acamera_core_proc( void *gamma_ctx, gamma_stats_data_t *stats, gamma_input_data_t *input, gamma_output_data_t *output )
{
    if ( !gamma_ctx || !stats || !input || !input->acamera_input || !output || !output->acamera_output ) {
        LOG( LOG_ERR, "Invalid parameter: %p-%p-%p-%p-%p-%p.", gamma_ctx, stats, input, input ? input->acamera_input : NULL, output, output ? output->acamera_output : NULL );
        return -1;
    }

    if ( stats->fullhist_size != ISP_FULL_HISTOGRAM_SIZE ) {
        LOG( LOG_ERR, "Not supported gamma size, current size: %d, max: %d.", stats->fullhist_size, ISP_FULL_HISTOGRAM_SIZE );
        return -2;
    }

    gamma_acamera_core_obj_t *p_gamma_core_obj = (gamma_acamera_core_obj_t *)gamma_ctx;
    gamma_acamera_input_t *acamera_input = input->acamera_input;
    gamma_acamera_output_t *p_gamma_acamera_output = (gamma_acamera_output_t *)output->acamera_output;

    p_gamma_acamera_output->gamma_gain = p_gamma_core_obj->gamma_gain;
    p_gamma_acamera_output->gamma_offset = p_gamma_core_obj->gamma_offset;

    return 0;
}
