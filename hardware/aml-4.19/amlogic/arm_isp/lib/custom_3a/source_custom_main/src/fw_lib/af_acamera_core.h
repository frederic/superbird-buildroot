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

#ifndef _AF_ACAMERA_CORE_H_
#define _AF_ACAMERA_CORE_H_

#include "acamera_math.h"
#include "af_standard_api.h"
#include "af_acamera_common.h"
#include "acamera_firmware_config.h"


#define AF_SPOT_IGNORE_NUM 1
#define AF_CALIBRATION_BOUNDARIES 1

typedef enum _af_spot_status_t {
    AF_SPOT_STATUS_NOT_FINISHED,
    AF_SPOT_STATUS_FINISHED_VALID,
    AF_SPOT_STATUS_FINISHED_INVALID,
} af_spot_status_t;


typedef enum _af_status_t {
    AF_STATUS_INVALID,
    AF_STATUS_FAST_SEARCH,
    AF_STATUS_TRACKING,
    AF_STATUS_CONVERGED,
} af_status_t;

typedef struct _af_spot_param_t {
    int32_t max_value;
    int32_t min_value;
    uint32_t max_position;
    int32_t before_max_value;
    uint32_t before_max_position;
    int32_t after_max_value;
    uint32_t after_max_position;
    int32_t previous_value;
    uint32_t optimal_position;
    uint8_t status;
    uint32_t reliable;
    uint32_t dynamic_range;
} af_spot_param_t;

typedef struct _af_fast_search_param_t {
    uint32_t step;
    uint32_t step_number;
    uint32_t position;
    uint32_t prev_position;
    uint32_t spot_zone_step_x;
    uint32_t spot_zone_step_y;
    uint32_t spot_zone_size_x;
    uint32_t spot_zone_size_y;
    af_spot_param_t spots[AF_SPOT_COUNT_Y][AF_SPOT_COUNT_X];
    uint32_t finished_valid_spot_count;
} af_fast_search_param_t;


typedef struct _af_track_position_param_t {
    uint8_t frames_in_tracking;
    uint8_t scene_is_changed;
    int32_t values[10];
    int values_inx;
    uint32_t diff0_cnt;
} af_track_position_param_t;


typedef struct _af_info_ {
    uint8_t af_mode;
    uint8_t refocus_required;

    uint8_t zones_horiz;
    uint8_t zones_vert;

    uint32_t roi;
    uint32_t af_pos_manual;
} af_info_t;

typedef struct _af_misc_info_ {

    int16_t accel_angle;

    uint16_t lens_min_step;
} af_misc_info_t;


typedef struct _af_calibration_data_ {
    af_lms_param_t *af_param;

    uint16_t *af_zone_whgh_h;
    uint32_t af_zone_whgh_h_len;

    uint16_t *af_zone_whgh_v;
    uint32_t af_zone_whgh_v_len;
} af_calibration_data_t;


typedef struct _af_acamera_input_ {
    af_info_t af_info;
    af_misc_info_t misc_info;
    af_calibration_data_t cali_data;
} af_acamera_input_t;


typedef struct _af_custom_input_ {
    uint32_t *custom_cali_data;
    uint32_t custom_cali_data_size;
} af_custom_input_t;


typedef struct _af_acamera_output_ {
    uint16_t af_lens_pos;
    int32_t af_sharp_val;

} af_acamera_output_t;


void *af_acamera_core_init( uint32_t ctx_id );
int32_t af_acamera_core_deinit( void *af_ctx );
int32_t af_acamera_core_proc( void *af_ctx, af_stats_data_t *stats, af_input_data_t *input, af_output_data_t *output );


#endif