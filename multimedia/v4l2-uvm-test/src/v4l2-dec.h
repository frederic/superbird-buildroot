/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef _V4L2_DEC_H_
#define _V4L2_DEC_H_

#include <stdint.h>
#include "demux.h"

enum vdec_dw_mode {
    VDEC_DW_AFBC_ONLY = 0,
    VDEC_DW_AFBC_1_1_DW = 1,
    VDEC_DW_AFBC_1_4_DW = 2,
    VDEC_DW_AFBC_x2_1_4_DW = 3,
    VDEC_DW_AFBC_1_2_DW = 4,
    VDEC_DW_NO_AFBC = 16,
};

typedef void (*decode_finish_fn)();

int v4l2_dec_init(enum vtype type, int secure, decode_finish_fn);
int v4l2_dec_destroy();

int v4l2_dec_write_es(const uint8_t *data, int size);
int v4l2_dec_frame_done();
int capture_buffer_recycle(void* handle);
int v4l2_dec_eos();
void dump_v4l2_decode_state();

#endif
