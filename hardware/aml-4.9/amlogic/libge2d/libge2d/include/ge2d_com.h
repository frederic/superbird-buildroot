/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef GE2D_COM_H_
#define GE2D_COM_H_

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

typedef struct{
    unsigned int color;
    rectangle_t src1_rect;
    rectangle_t src2_rect;
    rectangle_t dst_rect;
    int op;
}ge2d_op_para_t;



typedef struct{
    unsigned int color_blending_mode;
    unsigned int color_blending_src_factor;
    unsigned int color_blending_dst_factor;
    unsigned int alpha_blending_mode;
    unsigned int alpha_blending_src_factor;
    unsigned int alpha_blending_dst_factor;
}ge2d_blend_op;


#endif
