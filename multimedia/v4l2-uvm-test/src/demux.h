/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef __DEMUX_H_
#define __DEMUX_H_

#include <stdint.h>

#define DMX_SECOND 1000000

enum vtype {
    VIDEO_TYPE_MPEG2,
    VIDEO_TYPE_H264,
    VIDEO_TYPE_H265,
    VIDEO_TYPE_VP9,
    VIDEO_TYPE_AV1,
    VIDEO_TYPE_MJPEG,
    VIDEO_TYPE_MAX
};

struct dmx_v_data {
    int width;
    int height;
    enum vtype type;
};

typedef int (*dmx_write)(const uint8_t *data, int size);
typedef int (*dmx_frame_done)(int64_t pts);
typedef int (*dmx_meta_done)(struct dmx_v_data *);
typedef int (*dmx_eos)(void);

struct dmx_cb {
    dmx_write write;
    dmx_frame_done frame_done;
    dmx_meta_done meta_done;
    dmx_eos eos;
};

int demux_init(const char *file, struct dmx_cb *cb);
int dmx_destroy();
#endif
