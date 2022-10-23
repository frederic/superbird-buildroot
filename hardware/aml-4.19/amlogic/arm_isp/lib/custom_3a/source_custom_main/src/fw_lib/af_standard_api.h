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

#ifndef _AF_STANDARD_API_H_
#define _AF_STANDARD_API_H_


typedef struct _af_stats_data_ {
    uint32_t *zones_stats;
    uint32_t zones_size;
} af_stats_data_t;


typedef struct _af_input_data_ {
    void *custom_input;
    void *acamera_input;
} af_input_data_t;


typedef struct _af_output_data_ {
    void *custom_output;
    void *acamera_output;
} af_output_data_t;


typedef void *( *af_std_init_func )( uint32_t ctx_id );
typedef int32_t ( *af_std_proc_func )( void *af_ctx, af_stats_data_t *stats, af_input_data_t *input, af_output_data_t *output );
typedef int32_t ( *af_std_deinit_func )( void *af_ctx );


typedef struct _af_std_obj_ {
    void *af_ctx;
    af_std_init_func init;
    af_std_proc_func proc;
    af_std_deinit_func deinit;
} af_std_obj_t;


#endif