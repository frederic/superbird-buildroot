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


#ifndef __ISP_METADATA_H__
#define __ISP_METADATA_H__
/*
 * Apical(ARM) V4L2 test application 2016
 *
 * This is ARM internal development purpose SW tool running on JUNO.
 */

enum {
    CCM_R = 0,
    CCM_G,
    CCM_B,
    CCM_MAX
};

typedef struct _firmware_metadata_t {
    uint32_t    frame_id;

    int32_t     sensor_width;
    int32_t     sensor_height;
    int32_t     image_format;
    int32_t     sensor_bits;
    int32_t     rggb_start;
    int32_t     isp_mode;
    int32_t     fps;

    int32_t     int_time;
    int64_t     int_time_ms;
    int32_t     int_time_medium;
    int32_t     int_time_long;
    int32_t     again;
    int32_t     dgain;
    int32_t     isp_dgain;
    int32_t     exposure;
    int32_t     exposure_equiv;
    int32_t     gain_log2;

    int32_t     lens_pos;

    int32_t     anti_flicker;
    int32_t     gain_00;
    int32_t     gain_01;
    int32_t     gain_10;
    int32_t     gain_11;

    int32_t     black_level_00;
    int32_t     black_level_01;
    int32_t     black_level_10;
    int32_t     black_level_11;

    int32_t     lsc_table;
    int32_t     lsc_blend;
    int32_t     lsc_mesh_strength;

    int64_t     awb_rgain;
    int64_t     awb_bgain;
    int64_t     awb_cct;

    int32_t     sinter_strength;
    int32_t     sinter_strength1;
    int32_t     sinter_strength4;
    int32_t     sinter_thresh_1h;
    int32_t     sinter_thresh_4h;
    int32_t     sinter_sad;

    int32_t     temper_strength;

    int32_t     iridix_strength;

    int32_t     dp_threash1;
    int32_t     dp_slope1;

    int32_t     dp_threash2;
    int32_t     dp_slope2;

    int32_t     sharpening_directional;
    int32_t     sharpening_unidirectional;

    int32_t     demosaic_np_offset;

    int32_t     fr_sharpern_strength;
    int32_t     ds1_sharpen_strength;
    int32_t     ds2_sharpen_strength;

    int32_t     ccm[3][3];
} firmware_metadata_t;

void    metadata_dump(const firmware_metadata_t *md);
int32_t fill_meta_buf(char *metadata_buf, firmware_metadata_t *md);

#endif // __METADATA_API_H__