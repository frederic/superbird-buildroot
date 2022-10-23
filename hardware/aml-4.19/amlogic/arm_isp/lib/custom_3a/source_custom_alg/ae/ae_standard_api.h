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

#ifndef _AE_STANDARD_API_H_
#define _AE_STANDARD_API_H_

#include <stdint.h>

#define FIRMWARE_CONTEXT_NUMBER 1
#define ISP_FULL_HISTOGRAM_SIZE 1024

typedef struct _ae_stats_data_ {
	uint32_t *fullhist;
	uint32_t fullhist_size;
	uint32_t fullhist_sum;
	uint16_t *zone_hist;
	uint32_t zone_hist_size;
} ae_stats_data_t;

typedef struct _ae_input_data_ {
	void *custom_input;
	void *acamera_input;
} ae_input_data_t;

typedef struct _ae_output_data_ {
	void *custom_output;
	void *acamera_output;
} ae_output_data_t;

typedef void *( *ae_std_init_func )( uint32_t ctx_id );
typedef int32_t ( *ae_std_proc_func )( void *ae_ctx, ae_stats_data_t *stats, ae_input_data_t *input, ae_output_data_t *output );
typedef int32_t ( *ae_std_deinit_func )( void *ae_ctx );

typedef struct _ae_std_obj_ {
	void *ae_ctx;
	ae_std_init_func init;
	ae_std_proc_func proc;
	ae_std_deinit_func deinit;
} ae_std_obj_t;

#endif
