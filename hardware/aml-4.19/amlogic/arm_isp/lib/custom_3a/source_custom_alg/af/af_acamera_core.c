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

#include "af_acamera_log.h"
#include "af_standard_api.h"
#include "af_acamera_core.h"

#define AF_ZONES_COUNT_MAX (33 * 33)

typedef struct _af_acamera_core_obj_ {
	af_status_t af_status;
	af_fast_search_param_t fs;
	af_track_position_param_t tp;

	uint32_t pos_min;
	uint32_t pos_inf;
	uint32_t pos_macro;
	uint32_t pos_max;
	uint32_t def_pos_min;
	uint32_t def_pos_inf;
	uint32_t def_pos_macro;
	uint32_t def_pos_max;
	uint32_t def_pos_min_up;
	uint32_t def_pos_inf_up;
	uint32_t def_pos_macro_up;
	uint32_t def_pos_max_up;
	uint32_t def_pos_min_down;
	uint32_t def_pos_inf_down;
	uint32_t def_pos_macro_down;
	uint32_t def_pos_max_down;

	uint32_t last_position;
	int32_t last_sharp;
	uint32_t frame_number_from_start;
	uint8_t skip_frame;
	uint8_t frame_num;
	uint8_t zone_weight[AF_ZONES_COUNT_MAX];
	uint64_t zone_process_statistic[AF_ZONES_COUNT_MAX];
	uint32_t zone_process_reliablility[AF_ZONES_COUNT_MAX];
} af_acamera_core_obj_t;

static af_acamera_core_obj_t af_core_objs[FIRMWARE_CONTEXT_NUMBER];

void *af_acamera_core_init(uint32_t ctx_id)
{
	af_acamera_core_obj_t *p_af_core_obj = NULL;

	if (ctx_id >= FIRMWARE_CONTEXT_NUMBER) {
		LOG( LOG_CRIT, "Invalid ctx_id: %d, greater than max: %d.", ctx_id, FIRMWARE_CONTEXT_NUMBER - 1 );
		return NULL;
	}

	p_af_core_obj = &af_core_objs[ctx_id];
	memset(p_af_core_obj, 0, sizeof( *p_af_core_obj ));

	p_af_core_obj->af_status = AF_STATUS_INVALID;
	p_af_core_obj->frame_num = 60;

	return p_af_core_obj;
}

int32_t af_acamera_core_deinit(void *af_ctx)
{
	return 0;
}

int32_t af_acamera_core_proc( void *af_ctx, af_stats_data_t *stats, af_input_data_t *input, af_output_data_t *output )
{
	if ( !af_ctx || !stats || !input || !input->acamera_input ||
				!output || !output->acamera_output ) {
		LOG( LOG_ERR, "Invalid parameter: %p-%p-%p-%p-%p-%p.",
				af_ctx, stats, input,
				input ? input->acamera_input : NULL,
				output, output ? output->acamera_output : NULL );
		return -1;
	}

	af_acamera_core_obj_t *p_af_core_obj = (af_acamera_core_obj_t *)af_ctx;
	af_acamera_output_t *p_af_acamera_output = (af_acamera_output_t *)output->acamera_output;
	af_acamera_input_t *p_af_acamera_input = (af_acamera_input_t *)input->acamera_input;

	if ( stats->zones_size > AF_ZONES_COUNT_MAX ) {
		LOG( LOG_ERR, "Not supported AF zones, current size: %d, max: %d.",
				stats->zones_size, AF_ZONES_COUNT_MAX );
		return -2;
	}

	p_af_acamera_output->af_lens_pos = p_af_core_obj->last_position;
	p_af_acamera_output->af_sharp_val = p_af_core_obj->last_sharp;

	return 0;
}

