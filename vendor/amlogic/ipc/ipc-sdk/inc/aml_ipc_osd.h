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

#include "aml_ipc_component.h"
#include "aml_ipc_ge2d.h"

#ifndef AML_IPC_OSD_H

#define AML_IPC_OSD_H

#ifdef __cplusplus
extern "C" {
#endif

struct AmlIPCOSDContextPriv;
struct AmlIPCOSDContext {
    AML_OBJ_EXTENDS(AmlIPCOSDContext, AmlObject, AmlObjectClass);
    struct AmlIPCOSDContextPriv *priv;
    char *font_file;
    int font_size;
    int color; // char [4] rgba
    int bgcolor;
    int line_width;
    struct AmlIPCVideoFrame *cur_surface;
    uint8_t *frame_buffer;
    const uint8_t *color_index; // char [4] r g b a index
    uint8_t color_surface[4];   // convert color to surface pixel format
    uint8_t bgcolor_surface[4];
    int pixel_bytes;
    int has_alpha; // pixel_bytes - 3
};
AML_OBJ_DECLARE_TYPEID(AmlIPCOSDContext, AmlObject, AmlObjectClass);

AmlStatus aml_ipc_osd_load_font(struct AmlIPCOSDContext *ctx, const char *fontfile, int size);
AmlStatus aml_ipc_osd_get_font_bbox(struct AmlIPCOSDContext *ctx, int *maxw, int *maxh);
// preload the text, and calculate the text width and height
AmlStatus aml_ipc_osd_preload_text(struct AmlIPCOSDContext *ctx, const char *text, int *w, int *h);
AmlStatus aml_ipc_osd_preload_png(struct AmlIPCOSDContext *ctx, const char *file, int *w, int *h);
AmlStatus aml_ipc_osd_preload_jpeg(struct AmlIPCOSDContext *ctx, const char *file, int *w, int *h);
// refcnt will add inside aml_ipc_osd_start_paint, and will release in aml_ipc_osd_done_paint
AmlStatus aml_ipc_osd_start_paint(struct AmlIPCOSDContext *ctx, struct AmlIPCVideoFrame *frame);
AmlStatus aml_ipc_osd_load_png(struct AmlIPCOSDContext *ctx, const char *file, int x, int y);
AmlStatus aml_ipc_osd_load_jpeg(struct AmlIPCOSDContext *ctx, const char *file, int x, int y);
AmlStatus aml_ipc_osd_draw_text(struct AmlIPCOSDContext *ctx, const char *text, int *x, int *y);
AmlStatus aml_ipc_osd_draw_line(struct AmlIPCOSDContext *ctx, int x1, int y1, int x2, int y2);
AmlStatus aml_ipc_osd_draw_rect(struct AmlIPCOSDContext *ctx, struct AmlRect *rc);
AmlStatus aml_ipc_osd_fill_rect(struct AmlIPCOSDContext *ctx, struct AmlRect *rc, int color);
// alpha blend src frame src_rc region over paint frame (aml_ipc_osd_start_paint)
AmlStatus aml_ipc_osd_alpha_blend(struct AmlIPCOSDContext *ctx, struct AmlIPCVideoFrame *src,
                                  struct AmlRect *src_rc, int x, int y);
AmlStatus aml_ipc_osd_done_paint(struct AmlIPCOSDContext *ctx);

struct AmlIPCOSDContext *aml_ipc_osd_context_new();

// very slow, do not use in critical case
void aml_ipc_osd_set_pixel(struct AmlIPCOSDContext *ctx, uint8_t *dst, int color);

AmlStatus aml_ipc_osd_set_color(struct AmlIPCOSDContext *ctx, int color);

AmlStatus aml_ipc_osd_set_bgcolor(struct AmlIPCOSDContext *ctx, int color);

AmlStatus aml_ipc_osd_set_width(struct AmlIPCOSDContext *ctx, int width);

AmlStatus aml_ipc_osd_clear_rect(struct AmlIPCOSDContext *ctx, struct AmlRect *rc);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_OSD_H */
