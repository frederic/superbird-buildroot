/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef __DRM_339_H__
#define __DRM_339_H__

#include <stdint.h>
#include <stdbool.h>

enum frame_format {
    FRAME_FMT_NV12,
    FRAME_FMT_NV21,
    FRAME_FMT_AFBC,
};

typedef struct drm_frame drm_frame;

typedef int (*drm_frame_destroy)(drm_frame*);
struct drm_frame {
    void* gem;
    void* pri_dec;
    drm_frame_destroy destroy;
};

typedef int (*displayed_cb_func)(void* handle);

int display_engine_start();
int display_engine_stop();
struct drm_frame* display_create_buffer(unsigned int width, unsigned int height,
        enum frame_format format, int planes_count);
int display_get_buffer_fds(struct drm_frame* drm_f, int *fd, int cnt);
int display_engine_show(struct drm_frame* frame);
int display_engine_register_cb(displayed_cb_func cb);
int display_wait_for_display();
#endif
