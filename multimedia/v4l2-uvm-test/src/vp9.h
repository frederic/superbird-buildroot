/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef _UMV_VP9_H_
#define _UMV_VP9_H_

#include <stdint.h>

struct vp9_superframe_split {
    /*in data*/
    uint8_t *data;
    uint32_t data_size;

    /*out data*/
    int nb_frames;
    int size;
    int next_frame;
    uint32_t next_frame_offset;
    int prefix_size;
    int sizes[8];
};

int vp9_superframe_split_filter(struct vp9_superframe_split *s);
void fill_vp9_header(uint8_t *buf, int len);
#endif
