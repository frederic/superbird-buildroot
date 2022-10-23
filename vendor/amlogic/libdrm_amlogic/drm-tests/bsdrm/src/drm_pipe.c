/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "bs_drm.h"

struct bs_drm_pipe_plumber {
	int fd;
	const uint32_t *connector_ranks;
	uint32_t crtc_mask;
	drmModeConnector **connector_ptr;
};

struct fd_connector {
	int fd;
	drmModeRes *res;
	drmModeConnector *connector;
};

struct pipe_internal {
	struct fd_connector *connector;
	size_t next_connector_index;
	uint32_t connector_rank;
	uint32_t encoder_id;
	int next_encoder_index;
	uint32_t crtc_id;
	int next_crtc_index;
};

struct pipe_ctx {
	size_t connector_count;
	struct fd_connector *connectors;

	uint32_t crtc_mask;
	const uint32_t *connector_ranks;

	uint32_t best_rank;
	size_t first_connector_index;
};

static bool pipe_piece_connector(void *c, void *p)
{
	struct pipe_ctx *ctx = c;
	struct pipe_internal *pipe = p;

	bool use_connector = false;
	size_t connector_index;
	for (connector_index = pipe->next_connector_index == 0 ? ctx->first_connector_index
							       : pipe->next_connector_index;
	     connector_index < ctx->connector_count; connector_index++) {
		struct fd_connector *fd_connector = &ctx->connectors[connector_index];
		drmModeConnector *connector = fd_connector->connector;
		if (connector == NULL)
			continue;

		use_connector =
		    connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0;
		if (use_connector && ctx->connector_ranks) {
			uint32_t rank =
			    bs_drm_connectors_rank(ctx->connector_ranks, connector->connector_type);
			if (rank >= ctx->best_rank)
				use_connector = false;
			pipe->connector_rank = rank;
		}

		if (use_connector) {
			pipe->connector = fd_connector;
			break;
		}
	}

	pipe->next_connector_index = connector_index + 1;
	return use_connector;
}

static bool pipe_piece_encoder(void *c, void *p)
{
	(void)c;
	struct pipe_internal *pipe = p;
	int fd = pipe->connector->fd;

	drmModeConnector *connector = pipe->connector->connector;
	if (connector == NULL)
		return false;

	int encoder_index = 0;
	for (encoder_index = pipe->next_encoder_index; encoder_index < connector->count_encoders;
	     encoder_index++) {
		drmModeEncoder *encoder = drmModeGetEncoder(fd, connector->encoders[encoder_index]);
		if (encoder == NULL)
			continue;

		drmModeFreeEncoder(encoder);

		pipe->encoder_id = connector->encoders[encoder_index];

		break;
	}

	pipe->next_encoder_index = encoder_index + 1;
	return encoder_index < connector->count_encoders;
}

static bool pipe_piece_crtc(void *c, void *p)
{
	struct pipe_ctx *ctx = c;
	struct pipe_internal *pipe = p;
	int fd = pipe->connector->fd;
	drmModeRes *res = pipe->connector->res;

	drmModeEncoder *encoder = drmModeGetEncoder(fd, pipe->encoder_id);
	if (encoder == NULL)
		return false;

	uint32_t possible_crtcs = encoder->possible_crtcs & ctx->crtc_mask;
	drmModeFreeEncoder(encoder);

	bool use_crtc = false;
	int crtc_index;
	for (crtc_index = pipe->next_crtc_index; crtc_index < res->count_crtcs; crtc_index++) {
		use_crtc = (possible_crtcs & (1 << crtc_index));
		if (use_crtc) {
			pipe->crtc_id = res->crtcs[crtc_index];
			break;
		}
	}

	pipe->next_crtc_index = crtc_index;
	return use_crtc;
}

static void bs_drm_pipe_plumber_init(struct bs_drm_pipe_plumber *self)
{
	assert(self);
	self->fd = -1;
	self->crtc_mask = 0xffffffff;
}

struct bs_drm_pipe_plumber *bs_drm_pipe_plumber_new()
{
	struct bs_drm_pipe_plumber *self = calloc(1, sizeof(struct bs_drm_pipe_plumber));
	assert(self);
	bs_drm_pipe_plumber_init(self);
	return self;
}

void bs_drm_pipe_plumber_destroy(struct bs_drm_pipe_plumber **self)
{
	assert(self);
	assert(*self);
	free(*self);
	*self = NULL;
}

void bs_drm_pipe_plumber_connector_ranks(struct bs_drm_pipe_plumber *self,
					 const uint32_t *connector_ranks)
{
	assert(self);
	self->connector_ranks = connector_ranks;
}

void bs_drm_pipe_plumber_crtc_mask(struct bs_drm_pipe_plumber *self, uint32_t crtc_mask)
{
	assert(self);
	self->crtc_mask = crtc_mask;
}

void bs_drm_pipe_plumber_fd(struct bs_drm_pipe_plumber *self, int card_fd)
{
	assert(self);
	self->fd = card_fd;
}

void bs_drm_pipe_plumber_connector_ptr(struct bs_drm_pipe_plumber *self, drmModeConnector **ptr)
{
	assert(self);
	self->connector_ptr = ptr;
}

bool bs_drm_pipe_plumber_make(struct bs_drm_pipe_plumber *self, struct bs_drm_pipe *pipe)
{
	assert(self);
	assert(pipe);

	size_t connector_count = 0;
	size_t fd_count = 0;
	int fds[DRM_MAX_MINOR];
	drmModeRes *fd_res[DRM_MAX_MINOR];
	if (self->fd >= 0) {
		fds[0] = self->fd;
		fd_res[0] = drmModeGetResources(fds[0]);
		if (!fd_res[0])
			return false;
		connector_count += fd_res[0]->count_connectors;
		fd_count++;
	} else {
		for (int fd_index = 0; fd_index < DRM_MAX_MINOR; fd_index++) {
			char *file_path = NULL;
			int ret = asprintf(&file_path, "/dev/dri/card%d", fd_index);
			assert(ret != -1);
			assert(file_path);

			int fd = open(file_path, O_RDWR);
			free(file_path);
			if (fd < 0)
				continue;

			drmModeRes *res = drmModeGetResources(fd);
			if (!res) {
				close(fd);
				continue;
			}
			fds[fd_count] = fd;
			fd_res[fd_count] = res;
			connector_count += res->count_connectors;
			fd_count++;
		}
	}

	struct fd_connector *connectors = calloc(connector_count, sizeof(struct fd_connector));
	assert(connectors);
	size_t connector_index = 0;
	for (size_t fd_index = 0; fd_index < fd_count; fd_index++) {
		int fd = fds[fd_index];
		drmModeRes *res = fd_res[fd_index];
		for (int fd_conn_index = 0; fd_conn_index < res->count_connectors;
		     fd_conn_index++) {
			connectors[connector_index].fd = fd;
			connectors[connector_index].res = res;
			connectors[connector_index].connector =
			    drmModeGetConnector(fd, res->connectors[fd_conn_index]);
			connector_index++;
		}
	}

	struct pipe_ctx ctx = { connector_count,       connectors,   self->crtc_mask,
				self->connector_ranks, bs_rank_skip, 0 };
	struct pipe_internal pipe_internal;
	bs_make_pipe_piece pieces[] = { pipe_piece_connector, pipe_piece_encoder, pipe_piece_crtc };
	bool success = false;
	while (ctx.best_rank != 0) {
		struct pipe_internal current_pipe_internal;
		bool current_success =
		    bs_pipe_make(&ctx, pieces, sizeof(pieces) / sizeof(pieces[0]),
				 &current_pipe_internal, sizeof(struct pipe_internal));
		if (!current_success)
			break;

		pipe_internal = current_pipe_internal;
		success = true;
		if (ctx.connector_ranks == NULL)
			break;

		ctx.best_rank = current_pipe_internal.connector_rank;
		ctx.first_connector_index = current_pipe_internal.next_connector_index;
	}

	if (success) {
		struct fd_connector *fd_connector = pipe_internal.connector;
		pipe->fd = fd_connector->fd;
		pipe->connector_id = fd_connector->connector->connector_id;
		pipe->encoder_id = pipe_internal.encoder_id;
		pipe->crtc_id = pipe_internal.crtc_id;
		if (self->connector_ptr) {
			*self->connector_ptr = fd_connector->connector;
			fd_connector->connector = NULL;
		}
	}

	for (size_t connector_index = 0; connector_index < connector_count; connector_index++)
		drmModeFreeConnector(connectors[connector_index].connector);
	free(connectors);

	for (size_t fd_index = 0; fd_index < fd_count; fd_index++) {
		int fd = fds[fd_index];
		if (self->fd != fd && !(success && pipe->fd == fd))
			close(fd);
		drmModeFreeResources(fd_res[fd_index]);
	}

	return success;
}

bool bs_drm_pipe_make(int fd, struct bs_drm_pipe *pipe)
{
	assert(fd >= 0);
	assert(pipe);

	struct bs_drm_pipe_plumber plumber = { 0 };
	bs_drm_pipe_plumber_init(&plumber);
	bs_drm_pipe_plumber_fd(&plumber, fd);

	return bs_drm_pipe_plumber_make(&plumber, pipe);
}
