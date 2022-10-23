/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef GE2D_PORT_H_
#define GE2D_PORT_H_

#define ge2d_fail      -1
#define ge2d_success        0

#define OSD0        0
#define OSD1        1

#define SRC1_GB_ALPHA_ENABLE 0x80000000

#if defined (__cplusplus)
extern "C" {
#endif

typedef enum {
    GE2D_CANVAS_OSD0 = 0,
    GE2D_CANVAS_OSD1,
    GE2D_CANVAS_ALLOC,
    GE2D_CANVAS_TYPE_INVALID,
}ge2d_canvas_t;

typedef enum {
    LAYER_MODE_INVALID = 0,
    LAYER_MODE_NON = 1,
    LAYER_MODE_PREMULTIPLIED = 2,
    LAYER_MODE_COVERAGE = 3,
} layer_mode_t;

/* Blend modes, settable per layer */
typedef enum {
    BLEND_MODE_INVALID = 0,

    /* colorOut = colorSrc */
    BLEND_MODE_NONE = 1,

    /* colorOut = colorSrc + colorDst * (1 - alphaSrc) */
    BLEND_MODE_PREMULTIPLIED = 2,

    /* colorOut = colorSrc * alphaSrc + colorDst * (1 - alphaSrc) */
    BLEND_MODE_COVERAGE = 3,
} blend_mode_t;

/**
 * pixel format definitions
 */
typedef enum  {
    PIXEL_FORMAT_RGBA_8888          = 1,
    PIXEL_FORMAT_RGBX_8888          = 2,
    PIXEL_FORMAT_RGB_888            = 3,
    PIXEL_FORMAT_RGB_565            = 4,
    PIXEL_FORMAT_BGRA_8888          = 5,
    PIXEL_FORMAT_YV12               = 0x32315659, // YCrCb 4:2:0 Planar  YYYY......  U......V......
    PIXEL_FORMAT_Y8                 = 0x20203859, // YYYY
    PIXEL_FORMAT_YCbCr_422_SP       = 0x10, // NV16   YYYY.....UVUV....
    PIXEL_FORMAT_YCrCb_420_SP       = 0x11, // NV21   YYYY.....UV....
    PIXEL_FORMAT_YCbCr_422_I        = 0x14, // YUY2   Y0 U0 Y1 V0
    PIXEL_FORMAT_BGR_888
}pixel_format_t;

typedef enum {
    GE2D_ROTATION_0,
    GE2D_ROTATION_90,
    GE2D_ROTATION_180,
    GE2D_ROTATION_270,
} GE2D_ROTATION;

typedef enum {
    AML_GE2D_FILLRECTANGLE,
    AML_GE2D_BLEND,
    AML_GE2D_STRETCHBLIT,
    AML_GE2D_BLIT,
    AML_GE2D_NONE,
} GE2DOP;

typedef struct{
    int x;
    int y;
    int w;
    int h;
}rectangle_t;

typedef struct buffer_info {
    unsigned int memtype;
    char* vaddr;
    unsigned long offset;
    unsigned int canvas_w;
    unsigned int canvas_h;
    rectangle_t rect;
    int format;
    unsigned int rotation;
    int shared_fd;
    unsigned char plane_alpha;
    unsigned char layer_mode;
    unsigned char fill_color_en;
    unsigned int  def_color;
} buffer_info_t;

typedef struct aml_ge2d_info {
    unsigned int offset;
    unsigned int blend_mode;
    GE2DOP ge2d_op;
    buffer_info_t src_info[2];
    buffer_info_t dst_info;
    unsigned int color;
    unsigned int gl_alpha;
    unsigned int const_color;
	unsigned int dst_plane_cnt;
    unsigned int reserved;
} aml_ge2d_info_t;

int ge2d_open(void);
int ge2d_close(int fd);
int ge2d_process(int fd,aml_ge2d_info_t *pge2dinfo);

#if defined (__cplusplus)
}
#endif

#endif
