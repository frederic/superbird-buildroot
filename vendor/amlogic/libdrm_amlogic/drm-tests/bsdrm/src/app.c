/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "bs_drm.h"

struct bs_app_fb {
	struct gbm_bo *bo;
	uint32_t id;
};

struct bs_app {
	bool setup;

	int fd;
	struct gbm_device *gbm;

	uint32_t connector_id, crtc_id;
	drmModeModeInfo mode;

	uint32_t fb_format;
	size_t fb_count;
	struct bs_app_fb *fbs;
};

struct bs_app *bs_app_new()
{
	struct bs_app *self = calloc(1, sizeof(struct bs_app));
	assert(self);
	self->fd = -1;
	self->fb_format = GBM_FORMAT_XRGB8888;
	self->fb_count = 2;
	return self;
}

void bs_app_destroy(struct bs_app **app)
{
	assert(app);
	struct bs_app *self = *app;
	assert(self);

	if (self->setup) {
		if (self->fbs) {
			for (size_t fb_index = 0; fb_index < self->fb_count; fb_index++) {
				struct bs_app_fb *fb = &self->fbs[fb_index];
				gbm_bo_destroy(fb->bo);
				if (fb->id)
					drmModeRmFB(self->fd, fb->id);
			}
			free(self->fbs);
		}

		if (self->gbm) {
			gbm_device_destroy(self->gbm);
			self->gbm = NULL;
		}

		if (self->fd >= 0) {
			close(self->fd);
			self->fd = -1;
		}
	}

	free(self);
	*app = NULL;
}

int bs_app_fd(struct bs_app *self)
{
	assert(self);
	assert(self->setup);
	return self->fd;
}
size_t bs_app_fb_count(struct bs_app *self)
{
	assert(self);
	return self->fb_count;
}

void bs_app_set_fb_count(struct bs_app *self, size_t fb_count)
{
	assert(self);
	assert(!self->setup);
	assert(fb_count > 0);
	self->fb_count = fb_count;
}

struct gbm_bo *bs_app_fb_bo(struct bs_app *self, size_t index)
{
	assert(self);
	assert(self->fbs);
	assert(index < self->fb_count);
	return self->fbs[index].bo;
}

uint32_t bs_app_fb_id(struct bs_app *self, size_t index)
{
	assert(self);
	assert(self->fbs);
	assert(index < self->fb_count);
	return self->fbs[index].id;
}

bool bs_app_setup(struct bs_app *self)
{
	assert(self);
	assert(!self->setup);
	assert(self->fb_count > 0);

	self->fd = bs_drm_open_main_display();
	if (self->fd < 0) {
		bs_debug_error("failed to open card for display");
		return false;
	}

	self->gbm = gbm_create_device(self->fd);
	if (!self->gbm) {
		bs_debug_error("failed to create gbm");
		goto close_fd;
	}

	struct bs_drm_pipe pipe = { 0 };
	if (!bs_drm_pipe_make(self->fd, &pipe)) {
		bs_debug_error("failed to make pipe");
		goto destroy_device;
	}
	self->crtc_id = pipe.crtc_id;
	self->connector_id = pipe.connector_id;

	drmModeConnector *connector = drmModeGetConnector(self->fd, pipe.connector_id);
	if (!connector) {
		bs_debug_error("failed to get connector %u", pipe.connector_id);
		goto destroy_device;
	}

	self->mode = connector->modes[0];
	drmModeFreeConnector(connector);

	self->fbs = calloc(self->fb_count, sizeof(self->fbs[0]));
	assert(self->fbs);
	for (size_t fb_index = 0; fb_index < self->fb_count; fb_index++) {
		struct bs_app_fb *fb = &self->fbs[fb_index];
		fb->bo = gbm_bo_create(self->gbm, self->mode.hdisplay, self->mode.vdisplay,
				       self->fb_format, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
		if (fb->bo == NULL) {
			bs_debug_error("failed to allocate framebuffer %zu of %zu", fb_index + 1,
				       self->fb_count);
			goto destroy_fbs;
		}
		fb->id = bs_drm_fb_create_gbm(fb->bo);
		if (fb->id == 0) {
			bs_debug_error("failed to create framebuffer id %zu of %zu", fb_index + 1,
				       self->fb_count);
			goto destroy_fbs;
		}
	}

	self->setup = true;
	return true;

destroy_fbs:
	for (size_t fb_index = 0; fb_index < self->fb_count; fb_index++) {
		struct bs_app_fb *fb = &self->fbs[fb_index];
		if (fb->id != 0)
			drmModeRmFB(self->fd, fb->id);
		if (fb->bo)
			gbm_bo_destroy(fb->bo);
	}
	free(self->fbs);
	self->fbs = NULL;

destroy_device:
	gbm_device_destroy(self->gbm);
	self->gbm = NULL;

close_fd:
	close(self->fd);
	self->fd = -1;
	return false;
}

int bs_app_display_fb(struct bs_app *self, size_t index)
{
	assert(self);
	assert(self->fd >= 0);
	assert(self->fbs);
	assert(index < self->fb_count);
	return drmModeSetCrtc(self->fd, self->crtc_id, self->fbs[index].id, 0 /* x */, 0 /* y */,
			      &self->connector_id, 1 /* connector count */, &self->mode);
}
