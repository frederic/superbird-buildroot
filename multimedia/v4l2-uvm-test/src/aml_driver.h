/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef AML_DRIVER_H__
#define AML_DRIVER_H__

#include <stdint.h>

#define V4L2_CONFIG_PARM_DECODE_CFGINFO (1 << 0)
#define V4L2_CONFIG_PARM_DECODE_PSINFO  (1 << 1)
#define V4L2_CONFIG_PARM_DECODE_HDRINFO (1 << 2)
#define V4L2_CONFIG_PARM_DECODE_CNTINFO (1 << 3)

struct aml_vdec_cfg_infos {
    uint32_t double_write_mode;
    uint32_t init_width;
    uint32_t init_height;
    uint32_t ref_buf_margin;
    uint32_t canvas_mem_mode;
    uint32_t canvas_mem_endian;
};

#define SEI_PicTiming         1
#define SEI_MasteringDisplayColorVolume 137
#define SEI_ContentLightLevel 144
struct vframe_content_light_level_s {
    uint32_t present_flag;
    uint32_t max_content;
    uint32_t max_pic_average;
}; /* content_light_level from SEI */

struct vframe_master_display_colour_s {
    uint32_t present_flag;
    uint32_t primaries[3][2];
    uint32_t white_point[2];
    uint32_t luminance[2];
    struct vframe_content_light_level_s
        content_light_level;
}; /* master_display_colour_info_volume from SEI */

struct aml_vdec_hdr_infos {
    /*
     * bit 29   : present_flag
     * bit 28-26: video_format "component", "PAL", "NTSC", "SECAM", "MAC", "unspecified"
     * bit 25   : range "limited", "full_range"
     * bit 24   : color_description_present_flag
     * bit 23-16: color_primaries "unknown", "bt709", "undef", "bt601",
     *            "bt470m", "bt470bg", "smpte170m", "smpte240m", "film", "bt2020"
     * bit 15-8 : transfer_characteristic unknown", "bt709", "undef", "bt601",
     *            "bt470m", "bt470bg", "smpte170m", "smpte240m",
     *            "linear", "log100", "log316", "iec61966-2-4",
     *            "bt1361e", "iec61966-2-1", "bt2020-10", "bt2020-12",
     *            "smpte-st-2084", "smpte-st-428"
     * bit 7-0  : matrix_coefficient "GBR", "bt709", "undef", "bt601",
     *            "fcc", "bt470bg", "smpte170m", "smpte240m",
     *            "YCgCo", "bt2020nc", "bt2020c"
     */
    uint32_t signal_type;
    struct vframe_master_display_colour_s color_parms;
};

struct aml_vdec_ps_infos {
    uint32_t visible_width;
    uint32_t visible_height;
    uint32_t coded_width;
    uint32_t coded_height;
    uint32_t profile;
    uint32_t mb_width;
    uint32_t mb_height;
    uint32_t dpb_size;
    uint32_t ref_frames;
    uint32_t reorder_frames;
};

struct aml_vdec_cnt_infos {
    uint32_t bit_rate;
    uint32_t frame_count;
    uint32_t error_frame_count;
    uint32_t drop_frame_count;
    uint32_t total_data;
};

struct aml_dec_params {
    /* one of V4L2_CONFIG_PARM_DECODE_xxx */
    uint32_t parms_status;
    struct aml_vdec_cfg_infos   cfg;
    struct aml_vdec_ps_infos    ps;
    struct aml_vdec_hdr_infos   hdr;
    struct aml_vdec_cnt_infos   cnt;
};

#endif
