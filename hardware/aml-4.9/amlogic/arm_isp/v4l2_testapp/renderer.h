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

#ifndef _RENDERER_H_
#define _RENDERER_H_

/* renderer modes */
typedef enum render_mode {
    AFD_RENDER_MODE_CENTER,
    AFD_RENDER_MODE_LEFT_TOP,
    AFD_RENDER_MODE_MAX
} render_mode_t;

/* renderer image parameter */
typedef struct image_info {
    unsigned char *ptr;
    int width;
    int height;
    int bpp;
    uint32_t fmt;
} image_info_t;

/* renderer function */
int renderImage(unsigned char *dst, struct fb_var_screeninfo vinfo, struct fb_fix_screeninfo finfo,
    unsigned char *src, int width, int height, render_mode_t mode, int fb_fd, int fb_index);

#endif