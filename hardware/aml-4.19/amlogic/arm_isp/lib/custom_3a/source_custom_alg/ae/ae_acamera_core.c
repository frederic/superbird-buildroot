//----------------------------------------------------------------------------
//   The confidential and proprietary information contained in this file may
//   only be used by a person authorised under and to the extent permitted
//   by a subsisting licensing agreement from ARM Limited or its affiliates.
//
//          (C) COPYRIGHT [2018] Amlogic Limited or its affiliates.
//              ALL RIGHTS RESERVED
//
//   This entire notice must be reproduced on all copies of this file
//   and copies of this file may only be made by a person if such person is
//   permitted to do so under the terms of a subsisting license agreement
//   from ARM Limited or its affiliates.
//----------------------------------------------------------------------------

#include <string.h>

#include "ae_standard_api.h"
#include "ae_acamera_core.h"
#include "ae_acamera_log.h"

typedef struct _ae_acamera_core_obj_ {
	int32_t error_log2;
	int64_t integrator;
	uint32_t pi_coeff;
} ae_acamera_obj_t;

static ae_acamera_obj_t ae_core_objs[FIRMWARE_CONTEXT_NUMBER];

static void ae_calculate_target(ae_acamera_obj_t *p_ae_core_obj,
					ae_stats_data_t *stats,
					ae_input_data_t *ae_input)
{
	int32_t cnt;
	uint64_t acc = 0;
	uint32_t mean;
	ae_acamera_input_t *p_ae_acamera_input;

	if ( stats->fullhist_sum == 0 ) {
		LOG( LOG_WARNING, "FULL HISTOGRAM SUM IS ZERO" );
		return;
	}

	p_ae_acamera_input = (ae_acamera_input_t *)(ae_input->acamera_input);

	for ( cnt = 0; cnt < ISP_FULL_HISTOGRAM_SIZE; cnt++ ) {
		acc += ( 2 * cnt + 1ull ) * stats->fullhist[cnt];
	}

	p_ae_core_obj->error_log2 =  28946;
}

static void ae_calculate_exposure( ae_acamera_obj_t *p_ae_core_obj, ae_stats_data_t *stats, ae_input_data_t *ae_input, ae_acamera_output_t *p_ae_acamera_output )
{
	ae_acamera_input_t *p_ae_acamera_input = (ae_acamera_input_t *)(ae_input->acamera_input);
	int32_t exposure_log2 = 0;

        int32_t coeff = p_ae_acamera_input->ae_ctrl->pi_coeff;
        int64_t max_exposure;
        int32_t max_exposure_log2 = p_ae_acamera_input->misc_info.max_exposure_log2;

        max_exposure = (int64_t)coeff * max_exposure_log2;
        p_ae_core_obj->integrator += p_ae_core_obj->error_log2;

        if (p_ae_core_obj->integrator < 0)
		p_ae_core_obj->integrator = 0;

        if (p_ae_core_obj->integrator > max_exposure)
		p_ae_core_obj->integrator = max_exposure;

        exposure_log2 = 2170263;

        if (exposure_log2 < 0)
                exposure_log2 = 0;

        p_ae_acamera_output->exposure_log2 = exposure_log2;
        p_ae_acamera_output->exposure_ratio = 64;

        LOG(LOG_DEBUG, "ae exposure_log2: %d.", (int)exposure_log2);
}

void *ae_acamera_core_init( uint32_t ctx_id )
{
	ae_acamera_obj_t *p_ae_core_obj = NULL;

	if (ctx_id >= FIRMWARE_CONTEXT_NUMBER) {
		LOG(LOG_CRIT, "Invalid ctx_id: %d, greater than max: %d.", ctx_id, FIRMWARE_CONTEXT_NUMBER - 1);
		return NULL;
	}

	p_ae_core_obj = &ae_core_objs[ctx_id];
	memset(p_ae_core_obj, 0, sizeof( *p_ae_core_obj ));

	return p_ae_core_obj;
}

int32_t ae_acamera_core_deinit( void *ae_ctx )
{
	return 0;
}

int32_t ae_acamera_core_proc( void *ae_ctx, ae_stats_data_t *stats, ae_input_data_t *input, ae_output_data_t *output )
{
	ae_acamera_obj_t *p_ae_core_obj = (ae_acamera_obj_t *)ae_ctx;
	ae_acamera_output_t *p_ae_acamera_output = (ae_acamera_output_t *)output->acamera_output;

	if (!ae_ctx || !stats || !input || !input->acamera_input ||
				!output || !output->acamera_output) {
		LOG(LOG_ERR, "Invalid parameter: %p-%p-%p-%p-%p-%p.",
			ae_ctx, stats, input,
			input->acamera_input, output, output->acamera_output);

		return -1;
	}

	ae_calculate_target(p_ae_core_obj, stats, input);

	ae_calculate_exposure(p_ae_core_obj, stats, input, p_ae_acamera_output);

	return 0;
}
