/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "vp9.h"

int vp9_superframe_split_filter(struct vp9_superframe_split *s)
{
    int i, j, ret, marker;
    bool is_superframe = false;

    if (!s->data)
        return -1;

    marker = s->data[s->data_size - 1];
    if ((marker & 0xe0) == 0xc0) {
        int length_size = 1 + ((marker >> 3) & 0x3);
        int   nb_frames = 1 + (marker & 0x7);
        int    idx_size = 2 + nb_frames * length_size;

        if (s->data_size >= idx_size &&
                s->data[s->data_size - idx_size] == marker) {
            int total_size = 0;
            int idx = s->data_size + 1 - idx_size;

            for (i = 0; i < nb_frames; i++) {
                int frame_size = 0;
                for (j = 0; j < length_size; j++)
                    frame_size |= s->data[idx++] << (j * 8);

                total_size += frame_size;
                if (frame_size < 0 ||
                        total_size > s->data_size - idx_size) {
                    printf("Invalid frame size in a sframe: %d\n",
                            frame_size);
                    ret = -1;
                    goto fail;
                }
                s->sizes[i] = frame_size;
            }

            s->nb_frames         = nb_frames;
            s->size              = total_size;
            s->next_frame        = 0;
            s->next_frame_offset = 0;
            is_superframe        = true;
        }
    }else {
        s->nb_frames = 1;
        s->sizes[0]  = s->data_size;
        s->size      = s->data_size;
    }

#ifdef DEBUG_FRAME
    printf("sframe: %d, frames: %d, IN: %x, OUT: %x\n",
            is_superframe, s->nb_frames,
            s->data_size, s->size);
#endif

    /* parse uncompressed header. */
    if (is_superframe)
        printf("superframe deteced\n");

#ifdef DEBUG_FRAME
    printf("in: %x, %d, out: %x, sizes %d,%d,%d,%d,%d,%d,%d,%d\n",
            s->data_size,
            s->nb_frames,
            s->size,
            s->sizes[0],
            s->sizes[1],
            s->sizes[2],
            s->sizes[3],
            s->sizes[4],
            s->sizes[5],
            s->sizes[6],
            s->sizes[7]);
#endif

    return 0;
fail:
    return ret;
}

void fill_vp9_header(uint8_t *buf, int len)
{
    if (!buf)
        return;

    /* tricky part:
     * ucode starts handling from 0001, so add additional 4
     * bytes to the total size to cover 'AMLV'
     */
    len += 4;
    buf[0] = (len >> 24) & 0xff;
    buf[1] = (len >> 16) & 0xff;
    buf[2] = (len >> 8) & 0xff;
    buf[3] = (len >> 0) & 0xff;
    buf[4] = ((len >> 24) & 0xff) ^ 0xff;
    buf[5] = ((len >> 16) & 0xff) ^ 0xff;
    buf[6] = ((len >> 8) & 0xff) ^ 0xff;
    buf[7] = ((len >> 0) & 0xff) ^ 0xff;
    buf[8] = 0;
    buf[9] = 0;
    buf[10] = 0;
    buf[11] = 1;
    buf[12] = 'A';
    buf[13] = 'M';
    buf[14] = 'L';
    buf[15] = 'V';
}
