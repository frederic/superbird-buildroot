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

#ifndef _GAMMA_STANDARD_API_H_
#define _GAMMA_STANDARD_API_H_

#include <stdint.h>

typedef struct _gamma_stats_data_ {
	uint32_t *fullhist;
	uint32_t fullhist_size;
} gamma_stats_data_t;

typedef struct _gamma_input_data_ {
	void *custom_input;
	void *acamera_input;
} gamma_input_data_t;

typedef struct _gamma_output_data_ {
	void *custom_output;
	void *acamera_output;
} gamma_output_data_t;

typedef void *( *gamma_std_init_func )( uint32_t ctx_id );
typedef int32_t ( *gamma_std_proc_func )( void *gamma_ctx, gamma_stats_data_t *stats, gamma_input_data_t *input, gamma_output_data_t *output );
typedef int32_t ( *gamma_std_deinit_func )( void *gamma_ctx );

typedef struct _gamma_std_obj_ {
	void *gamma_ctx;
	gamma_std_init_func init;
	gamma_std_proc_func proc;
	gamma_std_deinit_func deinit;
} gamma_std_obj_t;

#endif
