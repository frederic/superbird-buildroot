/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "bs_drm.h"

int main()
{
	drmModeConnector *connector;
	struct bs_drm_pipe pipe = { 0 };
	struct bs_drm_pipe_plumber *plumber = bs_drm_pipe_plumber_new();
	bs_drm_pipe_plumber_connector_ptr(plumber, &connector);
	if (!bs_drm_pipe_plumber_make(plumber, &pipe)) {
		bs_debug_error("failed to make pipe");
		return 1;
	}
	bs_drm_pipe_plumber_destroy(&plumber);

	int fd = pipe.fd;
	drmModeModeInfo *mode = &connector->modes[0];

	struct gbm_device *gbm = gbm_create_device(fd);
	if (!gbm) {
		bs_debug_error("failed to create gbm");
		return 1;
	}

	struct gbm_bo *bos[2];
	uint32_t ids[2];
	for (size_t fb_index = 0; fb_index < 2; fb_index++) {
		bos[fb_index] =
		    gbm_bo_create(gbm, mode->hdisplay, mode->vdisplay, GBM_FORMAT_XRGB8888,
				  GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
		if (bos[fb_index] == NULL) {
			bs_debug_error("failed to allocate frame buffer");
			return 1;
		}
		ids[fb_index] = bs_drm_fb_create_gbm(bos[fb_index]);
		if (ids[fb_index] == 0) {
			bs_debug_error("failed to create framebuffer id");
			return 1;
		}
	}

	struct bs_mapper *mapper = bs_mapper_dma_buf_new();
	if (mapper == NULL) {
		bs_debug_error("failed to create mapper object");
		return 1;
	}

	for (size_t frame_index = 0; frame_index < 10000; frame_index++) {
		size_t fb_index = frame_index % 2;
		struct gbm_bo *bo = bos[fb_index];
		size_t bo_size = gbm_bo_get_stride(bo) * mode->vdisplay;
		void *map_data;
		char *ptr = bs_mapper_map(mapper, bo, 0, &map_data);
		if (ptr == MAP_FAILED) {
			bs_debug_error("failed to mmap gbm buffer object");
			return 1;
		}

		for (size_t i = 0; i < bo_size / 4; i++) {
			ptr[i * 4 + 0] = (i + frame_index * 50) % 256;
			ptr[i * 4 + 1] = (i + frame_index * 50 + 85) % 256;
			ptr[i * 4 + 2] = (i + frame_index * 50 + 170) % 256;
			ptr[i * 4 + 3] = 0;
		}
		bs_mapper_unmap(mapper, bo, map_data);

		int ret = drmModeSetCrtc(fd, pipe.crtc_id, ids[fb_index], 0 /* x */, 0 /* y */,
					 &pipe.connector_id, 1 /* connector count */, mode);
		if (ret) {
			bs_debug_error("failed to set crtc: %d", ret);
			return 1;
		}
		usleep(16667);
	}

	bs_mapper_destroy(mapper);

	for (size_t fb_index = 0; fb_index < 2; fb_index++) {
		gbm_bo_destroy(bos[fb_index]);
		drmModeRmFB(fd, ids[fb_index]);
	}

	gbm_device_destroy(gbm);

	drmModeFreeConnector(connector);
	close(fd);

	return 0;
}
