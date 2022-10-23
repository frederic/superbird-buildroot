/*
 * Copyright (C) 2014-2019 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AML_IPC_GE2D_H

#define AML_IPC_GE2D_H

#include "aml_ipc_component.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GE2D has two input channel, and one output channel
 *   input channel 0 is a AmlIPCVideoFrame
 *   input channel 1 is AmlIPCOSDFrame or list of AmlIPCOSDFrame
 *
 * GE2D has following working mode:
 * 1. fill_rect to ch0
 *   ch0: the original buffer to be fill, AmlIPCVideoFrame
 *   ch1: list of AmlIPCOSDFrame, specify rectangle and color
 *   rectangles in ch1 is drawn on ch0 image, and output ch0 image
 * 2. bitblt to ch0
 *   ch0: the dst buffer, AmlIPCVideoFrame
 *   ch1: AmlIPCOSDFrame, specify the image, src_rect and dst_rect, the width and height in
 *   dst_rect are ignored
 *   ch1 image src_rect area is blit to ch0 image dst_rect area, then output ch0 image
 * 3. bitblt from ch0
 *   ch0: the source buffer, AmlIPCVideoFrame
 *   ch1: AmlIPCOSDFrame, specify the image, src_rect and dst_rect, the width and height in
 *   dst_rect are ignored
 *   ch0 image src_rect area is blit to ch1 image dst_rect area, then output ch1 image
 * 4. stretchblt to ch0
 *   ch0: the dst buffer, AmlIPCVideoFrame
 *   ch1: AmlIPCOSDFrame, specify the image, src_rect and dst_rect
 *   ch1 image src_rect area is blit to ch0 image dst_rect area, then output ch0 image
 * 5. stretchblt from ch0
 *   ch0: the source buffer, AmlIPCVideoFrame
 *   ch1: AmlIPCOSDFrame, specify the image, src_rect and dst_rect
 *   ch0 image src_rect area is blit to ch1 image dst_rect area, then output ch1 image
 * 6. alphablend to ch0
 *   ch0: blend dst image, AmlIPCVideoFrame
 *   ch1: blend src images
 *   output: alpha blend all ch1 images src_rect area to ch0 dst_rect area
 * 7. alphablend from ch0
 *   ch0: blend src image, AmlIPCVideoFrame
 *   ch1: blend dst images
 *   output: alpha blend ch0 image src_rect areas to ch1 dst_rect areas
 *
 */

enum AmlIPCGE2DChannel { AML_GE2D_CH0, AML_GE2D_CH1, AML_GE2D_OUTPUT, AML_GE2D_CH_SDK_MAX };
enum AmlIPCGE2DOP {
    AML_GE2D_OP_SDK_MIN,
    AML_GE2D_OP_FILLRECT,
    AML_GE2D_OP_BITBLT_TO,
    AML_GE2D_OP_BITBLT_FROM,
    AML_GE2D_OP_STRETCHBLT_TO,
    AML_GE2D_OP_STRETCHBLT_FROM,
    AML_GE2D_OP_ALPHABLEND_TO,
    AML_GE2D_OP_ALPHABLEND_FROM,
    AML_GE2D_OP_SDK_MAX
};

struct AmlIPCGE2DPriv;
struct AmlIPCGE2D {
    AML_OBJ_EXTENDS(AmlIPCGE2D, AmlIPCComponent, AmlIPCComponentClass);
    struct AmlIPCGE2DPriv *priv;
    struct AmlIPCFrame *buffer;
    enum AmlIPCGE2DOP op;
    int pass_through;
    int rotation; // degree, 0~360
    pthread_mutex_t buf_lock;
    int is_bt709tobt601j;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCGE2D, AmlIPCComponent, AmlIPCComponentClass);

struct AmlIPCOSDFrame {
    AML_OBJ_EXTENDS(AmlIPCOSDFrame, AmlIPCVideoFrame, AmlObjectClass);
    TAILQ_ENTRY(AmlIPCOSDFrame) node;
    struct AmlRect src_rect;
    struct AmlRect dst_rect;
    uint32_t color;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCOSDFrame, AmlIPCVideoFrame, AmlObjectClass);
#define AML_IPC_OSD_FRAME(obj) ((struct AmlIPCOSDFrame *)(obj))

struct AmlIPCOSDFrameList {
    AML_OBJ_EXTENDS(AmlIPCOSDFrameList, AmlIPCFrame, AmlObjectClass);
    TAILQ_HEAD(AmlIPCOSDFrameListHead, AmlIPCOSDFrame) head;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCOSDFrameList, AmlIPCFrame, AmlObjectClass);
#define AML_IPC_OSD_FRAME_LIST(obj) ((struct AmlIPCOSDFrameList *)(obj))

struct AmlIPCGE2D *aml_ipc_ge2d_new(enum AmlIPCGE2DOP op);
AmlStatus aml_ipc_ge2d_set_op(struct AmlIPCGE2D *ge2d, enum AmlIPCGE2DOP op);

AmlStatus aml_ipc_ge2d_set_passthrough(struct AmlIPCGE2D *ge2d, int passthrough);

AmlStatus aml_ipc_ge2d_set_bt709to601j(struct AmlIPCGE2D *ge2d, int bt709to601j);

/**
 * @brief set rotation of the dest image
 *
 * @param ge2d
 * @param angle, rotation angle, only support 0, 90, 180, 270 degree rotation
 *
 * @return
 */
AmlStatus aml_ipc_ge2d_set_rotation(struct AmlIPCGE2D *ge2d, int angle);

/**
 * @brief set static buffer for channel 1
 *   ge2d requires both ch0 and ch1 image to do the operation, ch1 image can be set to a static
 *   buffer, if static buffer is NULL, ge2d will request new buffers from upstream each time it
 *   receives an image from ch0, else ge2d will use the static buffer
 *
 * @param ge2d
 * @param buf
 *   ge2d will take ownership of the buffer, by setting the new buffer, ge2d will free the old
 *   buffer, so if set buf to NULL, ge2d will request new buffers from upstream
 *
 * @return
 */
AmlStatus aml_ipc_ge2d_lock_static_buffer(struct AmlIPCGE2D *ge2d);

AmlStatus aml_ipc_ge2d_unlock_static_buffer(struct AmlIPCGE2D *ge2d);

AmlStatus aml_ipc_ge2d_set_static_buffer(struct AmlIPCGE2D *ge2d, struct AmlIPCFrame *buf);

// buffer list manipulate
AmlStatus aml_ipc_ge2d_static_buffer_list_replace(struct AmlIPCGE2D *ge2d,
                                                  struct AmlIPCOSDFrame *oldOsd,
                                                  struct AmlIPCOSDFrame *newOsd);

AmlStatus aml_ipc_ge2d_static_buffer_list_append(struct AmlIPCGE2D *ge2d,
                                                 struct AmlIPCOSDFrame *osd);

AmlStatus aml_ipc_ge2d_static_buffer_list_remove(struct AmlIPCGE2D *ge2d,
                                                 struct AmlIPCOSDFrame *osd);

/**
 * @brief help function to get the image size in byte for ge2d image
 *
 * @param pixfmt
 * @param width
 * @param height
 * @param pitch
 *
 * @return
 */
int aml_ipc_ge2d_get_image_size(enum AmlPixelFormat pixfmt, int width, int height, int *pitch);

/**
 * @brief helper function to do GE2D operation
 * this function does not take ownershipt of ch0 and ch1, caller continue to use the pointer after
 * returns
 *
 * @param ge2d
 * @param ch0, see GE2D working mode above
 * @param ch1, see GE2D working mode above
 *
 * @return
 */
AmlStatus aml_ipc_ge2d_process(struct AmlIPCGE2D *ge2d, struct AmlIPCFrame *ch0,
                               struct AmlIPCFrame *ch1);
// below helper functions can be used standalone, ge2d does not have to be bound, and ge2d's
// op (AmlIPCGE2DOP) is ignored
//
// fill rect in dst image with specified color
AmlStatus aml_ipc_ge2d_fill_rect(struct AmlIPCGE2D *ge2d, struct AmlIPCVideoFrame *dst,
                                 struct AmlRect *rc, int color);
// bitblt src image to dst
AmlStatus aml_ipc_ge2d_bitblt(struct AmlIPCGE2D *ge2d, struct AmlIPCVideoFrame *src,
                              struct AmlIPCVideoFrame *dst, struct AmlRect *srcrc, int dx, int dy);
// stretchblt src image to dst
AmlStatus aml_ipc_ge2d_stretchblt(struct AmlIPCGE2D *ge2d, struct AmlIPCVideoFrame *src,
                                  struct AmlIPCVideoFrame *dst, struct AmlRect *srcrc,
                                  struct AmlRect *dstrc);
// blend src image over dst, and the result is written to out
AmlStatus aml_ipc_ge2d_blend(struct AmlIPCGE2D *ge2d, struct AmlIPCVideoFrame *src,
                             struct AmlIPCVideoFrame *dst, struct AmlIPCVideoFrame *out,
                             struct AmlRect *srcrc, int dx, int dy, int ox, int oy);

// functions to manage AmlIPCOSDFrame/AmlIPCOSDFrameList
//
struct AmlIPCOSDFrameList *aml_ipc_osd_frame_list_new();

AmlStatus aml_ipc_osd_frame_list_replace(struct AmlIPCOSDFrameList *list,
                                         struct AmlIPCOSDFrame *oldOsd,
                                         struct AmlIPCOSDFrame *newOsd);

AmlStatus aml_ipc_osd_frame_list_append(struct AmlIPCOSDFrameList *list,
                                        struct AmlIPCOSDFrame *osd);

AmlStatus aml_ipc_osd_frame_list_remove(struct AmlIPCOSDFrameList *list,
                                        struct AmlIPCOSDFrame *osd);

AmlStatus aml_ipc_osd_frame_set_src_rect(struct AmlIPCOSDFrame *osd, int x, int y, int w, int h);

AmlStatus aml_ipc_osd_frame_set_dst_rect(struct AmlIPCOSDFrame *osd, int x, int y, int w, int h);

AmlStatus aml_ipc_osd_frame_set_rect(struct AmlIPCOSDFrame *osd, struct AmlRect *src,
                                     struct AmlRect *dst);

AmlStatus aml_ipc_osd_frame_set_color(struct AmlIPCOSDFrame *osd, uint32_t gbra);

struct AmlIPCOSDFrame *aml_ipc_osd_frame_new_alloc(struct AmlIPCVideoFormat *fmt);

struct AmlIPCOSDFrame *aml_ipc_osd_frame_new_fill_rect(struct AmlRect *rc, uint32_t gbra);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_GE2D_H */
