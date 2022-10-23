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

#ifndef _AWB_STANDARD_API_H_
#define _AWB_STANDARD_API_H_

#include <stdint.h>

typedef struct _awb_zone_t {
	uint16_t rg;
	uint16_t bg;
	uint32_t sum;
} awb_zone_t;

typedef struct _awb_stats_data_ {
	awb_zone_t *awb_zones;
	uint32_t zones_size;
} awb_stats_data_t;

typedef struct _awb_input_data_ {
	void *custom_input;
	void *acamera_input;
} awb_input_data_t;

typedef struct _awb_output_data_ {
	void *custom_output;
	void *acamera_output;
} awb_output_data_t;

typedef void *( *awb_std_init_func )( uint32_t ctx_id );
typedef int32_t ( *awb_std_proc_func )( void *awb_ctx, awb_stats_data_t *stats, awb_input_data_t *input, awb_output_data_t *output );
typedef int32_t ( *awb_std_deinit_func )( void *awb_ctx );

typedef struct _awb_std_obj_ {
	void *awb_ctx;
	awb_std_init_func init;
	awb_std_proc_func proc;
	awb_std_deinit_func deinit;
} awb_std_obj_t;

#endif
