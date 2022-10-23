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

#ifndef _GAMMA_ACAMERA_CORE_H_
#define _GAMMA_ACAMERA_CORE_H_

#include <stdint.h>

#define FIRMWARE_CONTEXT_NUMBER 1
#define ISP_FULL_HISTOGRAM_SIZE 1024

typedef struct _gamma_calibration_data_ {
	uint32_t *auto_level_ctrl;
} gamma_calibration_data_t;

typedef struct _gamma_acamera_input_ {
	gamma_calibration_data_t cali_data;
} gamma_acamera_input_t;

typedef struct _gamma_custom_input_ {
	uint32_t *custom_cali_data;
	uint32_t custom_cali_data_size;
} gamma_custom_input_t;

typedef struct _gamma_acamera_output_ {
	uint32_t gamma_gain;
	uint32_t gamma_offset;
} gamma_acamera_output_t;

void *gamma_acamera_core_init( uint32_t ctx_id );
int32_t gamma_acamera_core_deinit( void *gamma_ctx );
int32_t gamma_acamera_core_proc( void *gamma_ctx, gamma_stats_data_t *stats, gamma_input_data_t *input, gamma_output_data_t *output );

#endif
