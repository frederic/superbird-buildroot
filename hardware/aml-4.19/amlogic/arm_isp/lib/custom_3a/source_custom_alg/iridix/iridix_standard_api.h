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

#ifndef _IRIDIX_STANDARD_API_H_
#define _IRIDIX_STANDARD_API_H_

#include <stdint.h>

typedef struct _iridix_stats_data_ {
	const uint32_t *fullhist;
	uint32_t fullhist_size;
	uint32_t fullhist_sum;
} iridix_stats_data_t;

typedef struct _iridix_input_data_ {
	void *custom_input;
	void *acamera_input;
} iridix_input_data_t;

typedef struct _iridix_output_data_ {
	void *custom_output;
	void *acamera_output;
} iridix_output_data_t;

typedef void *( *iridix_std_init_func )( uint32_t ctx_id );
typedef int32_t ( *iridix_std_proc_func )( void *iridix_ctx, iridix_stats_data_t *stats, iridix_input_data_t *input, iridix_output_data_t *output );
typedef int32_t ( *iridix_std_deinit_func )( void *iridix_ctx );

typedef struct _iridix_std_obj_ {
	void *iridix_ctx;
	iridix_std_init_func init;
	iridix_std_proc_func proc;
	iridix_std_deinit_func deinit;
} iridix_std_obj_t;

#endif
