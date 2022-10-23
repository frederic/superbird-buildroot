/*
 * Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file performs some sanity checks on the DRM atomic API. To run a test, please run the
 * following command:
 *
 * atomictest <testname>
 *
 * To get a list of possible tests, run:
 *
 * atomictest
 */

#include <getopt.h>

#include "bs_drm.h"

#define CHECK(cond)                                               \
	do {                                                      \
		if (!(cond)) {                                    \
			bs_debug_error("check %s failed", #cond); \
			return -1;                                \
		}                                                 \
	} while (0)

#define CHECK_RESULT(ret)                                             \
	do {                                                          \
		if ((ret) < 0) {                                      \
			bs_debug_error("failed with error: %d", ret); \
			return -1;                                    \
		}                                                     \
	} while (0)

#define CURSOR_SIZE 64

// TODO(gsingh) This is defined in upstream libdrm -- remove when CROS libdrm is updated.
#ifndef DRM_ROTATE_0
#define DRM_ROTATE_0 (1UL << 0)
#endif

#ifndef DRM_REFLECT_Y
#define DRM_REFLECT_Y (1UL << 5)
#endif

#define TEST_COMMIT_FAIL 1

static const uint32_t yuv_formats[] = {
	DRM_FORMAT_NV12,
	DRM_FORMAT_YVU420,
};

/*
 * The blob for the CTM propery is a drm_color_ctm.
 * drm_color_ctm contains a 3x3 u64 matrix. Every element is represented as
 * sign and U31.32. The sign is the MSB.
 */
// clang-format off
static int64_t identity_ctm[9] = {
	0x100000000, 0x0, 0x0,
	0x0, 0x100000000, 0x0,
	0x0, 0x0, 0x100000000
};
static int64_t red_shift_ctm[9] = {
	0x140000000, 0x0, 0x0,
	0x0, 0xC0000000, 0x0,
	0x0, 0x0, 0xC0000000
};
// clang-format on

static bool automatic = false;
static struct gbm_device *gbm = NULL;

static void page_flip_handler(int fd, unsigned int sequence, unsigned int tv_sec,
			      unsigned int tv_usec, void *user_data)
{
	// Nothing to do.
}

struct atomictest_property {
	uint32_t pid;
	uint32_t value;
};

struct atomictest_plane {
	drmModePlane drm_plane;
	struct gbm_bo *bo;

	uint32_t format_idx;

	/* Properties. */
	struct atomictest_property crtc_id;
	struct atomictest_property crtc_x;
	struct atomictest_property crtc_y;
	struct atomictest_property crtc_w;
	struct atomictest_property crtc_h;
	struct atomictest_property fb_id;
	struct atomictest_property src_x;
	struct atomictest_property src_y;
	struct atomictest_property src_w;
	struct atomictest_property src_h;
	struct atomictest_property type;
	struct atomictest_property rotation;
	struct atomictest_property ctm;
};

struct atomictest_connector {
	uint32_t connector_id;
	struct atomictest_property crtc_id;
	struct atomictest_property edid;
	struct atomictest_property dpms;
};

struct atomictest_crtc {
	uint32_t crtc_id;
	uint32_t width;
	uint32_t height;
	uint32_t *primary_idx;
	uint32_t *cursor_idx;
	uint32_t *overlay_idx;
	uint32_t num_primary;
	uint32_t num_cursor;
	uint32_t num_overlay;

	struct atomictest_plane *planes;
	struct atomictest_property mode_id;
	struct atomictest_property active;
	struct atomictest_property ctm;
	struct atomictest_property gamma_lut;
	struct atomictest_property gamma_lut_size;
};

struct atomictest_mode {
	uint32_t height;
	uint32_t width;
	uint32_t id;
};

struct atomictest_context {
	int fd;
	uint32_t num_crtcs;
	uint32_t num_connectors;
	uint32_t num_modes;

	struct atomictest_connector *connectors;
	struct atomictest_crtc *crtcs;
	struct atomictest_mode *modes;
	drmModeAtomicReqPtr pset;
	drmEventContext drm_event_ctx;

	struct bs_mapper *mapper;
};

typedef int (*test_function)(struct atomictest_context *ctx, struct atomictest_crtc *crtc);

struct atomictest_testcase {
	const char *name;
	test_function test_func;
};

// clang-format off
enum draw_format_type {
	DRAW_NONE = 0,
	DRAW_STRIPE = 1,
	DRAW_TRANSPARENT_HOLE = 2,
	DRAW_ELLIPSE = 3,
	DRAW_CURSOR = 4,
	DRAW_LINES = 5,
};
// clang-format on

static int32_t get_format_idx(struct atomictest_plane *plane, uint32_t format)
{
	for (int32_t i = 0; i < plane->drm_plane.count_formats; i++)
		if (plane->drm_plane.formats[i] == format)
			return i;
	return -1;
}

static void copy_drm_plane(drmModePlane *dest, drmModePlane *src)
{
	memcpy(dest, src, sizeof(drmModePlane));
	dest->formats = calloc(src->count_formats, sizeof(uint32_t));
	memcpy(dest->formats, src->formats, src->count_formats * sizeof(uint32_t));
}

static struct atomictest_plane *get_plane(struct atomictest_crtc *crtc, uint32_t idx, uint64_t type)
{
	uint32_t index;
	switch (type) {
		case DRM_PLANE_TYPE_OVERLAY:
			index = crtc->overlay_idx[idx];
			break;
		case DRM_PLANE_TYPE_PRIMARY:
			index = crtc->primary_idx[idx];
			break;
		case DRM_PLANE_TYPE_CURSOR:
			index = crtc->cursor_idx[idx];
			break;
		default:
			bs_debug_error("invalid plane type returned");
			return NULL;
	}

	return &crtc->planes[index];
}

static int draw_to_plane(struct bs_mapper *mapper, struct atomictest_plane *plane,
			 enum draw_format_type pattern)
{
	struct gbm_bo *bo = plane->bo;
	uint32_t format = gbm_bo_get_format(bo);
	const struct bs_draw_format *draw_format = bs_get_draw_format(format);

	if (draw_format && pattern) {
		switch (pattern) {
			case DRAW_STRIPE:
				CHECK(bs_draw_stripe(mapper, bo, draw_format));
				break;
			case DRAW_TRANSPARENT_HOLE:
				CHECK(bs_draw_transparent_hole(mapper, bo, draw_format));
				break;
			case DRAW_ELLIPSE:
				CHECK(bs_draw_ellipse(mapper, bo, draw_format, 0));
				break;
			case DRAW_CURSOR:
				CHECK(bs_draw_cursor(mapper, bo, draw_format));
				break;
			case DRAW_LINES:
				CHECK(bs_draw_lines(mapper, bo, draw_format));
				break;
			default:
				bs_debug_error("invalid draw type");
				return -1;
		}
	} else {
		// DRM_FORMAT_RGB565 --> red, DRM_FORMAT_BGR565 --> blue,
		// everything else --> something
		void *map_data;
		uint16_t value = 0xF800;
		void *addr = bs_mapper_map(mapper, bo, 0, &map_data);
		uint32_t num_shorts = gbm_bo_get_plane_size(bo, 0) / sizeof(uint16_t);
		uint16_t *pixel = (uint16_t *)addr;

		CHECK(addr);
		for (uint32_t i = 0; i < num_shorts; i++)
			pixel[i] = value;

		bs_mapper_unmap(mapper, bo, map_data);
	}

	return 0;
}

static int get_prop(int fd, drmModeObjectPropertiesPtr props, const char *name,
		    struct atomictest_property *bs_prop)
{
	/* Property ID should always be > 0. */
	bs_prop->pid = 0;
	drmModePropertyPtr prop;
	for (uint32_t i = 0; i < props->count_props; i++) {
		if (bs_prop->pid)
			break;

		prop = drmModeGetProperty(fd, props->props[i]);
		if (prop) {
			if (!strcmp(prop->name, name)) {
				bs_prop->pid = prop->prop_id;
				bs_prop->value = props->prop_values[i];
			}
			drmModeFreeProperty(prop);
		}
	}

	return (bs_prop->pid == 0) ? -1 : 0;
}

static int get_connector_props(int fd, struct atomictest_connector *connector,
			       drmModeObjectPropertiesPtr props)
{
	CHECK_RESULT(get_prop(fd, props, "CRTC_ID", &connector->crtc_id));
	CHECK_RESULT(get_prop(fd, props, "EDID", &connector->edid));
	CHECK_RESULT(get_prop(fd, props, "DPMS", &connector->dpms));
	return 0;
}

static int get_crtc_props(int fd, struct atomictest_crtc *crtc, drmModeObjectPropertiesPtr props)
{
	CHECK_RESULT(get_prop(fd, props, "MODE_ID", &crtc->mode_id));
	CHECK_RESULT(get_prop(fd, props, "ACTIVE", &crtc->active));

	/*
	 * The atomic API makes no guarantee a property is present in object. This test
	 * requires the above common properties since a plane is undefined without them.
	 * Other properties (i.e: ctm) are optional.
	 */
	get_prop(fd, props, "CTM", &crtc->ctm);
	get_prop(fd, props, "GAMMA_LUT", &crtc->gamma_lut);
	get_prop(fd, props, "GAMMA_LUT_SIZE", &crtc->gamma_lut_size);
	return 0;
}

static int get_plane_props(int fd, struct atomictest_plane *plane, drmModeObjectPropertiesPtr props)
{
	CHECK_RESULT(get_prop(fd, props, "CRTC_ID", &plane->crtc_id));
	CHECK_RESULT(get_prop(fd, props, "FB_ID", &plane->fb_id));
	CHECK_RESULT(get_prop(fd, props, "CRTC_X", &plane->crtc_x));
	CHECK_RESULT(get_prop(fd, props, "CRTC_Y", &plane->crtc_y));
	CHECK_RESULT(get_prop(fd, props, "CRTC_W", &plane->crtc_w));
	CHECK_RESULT(get_prop(fd, props, "CRTC_H", &plane->crtc_h));
	CHECK_RESULT(get_prop(fd, props, "SRC_X", &plane->src_x));
	CHECK_RESULT(get_prop(fd, props, "SRC_Y", &plane->src_y));
	CHECK_RESULT(get_prop(fd, props, "SRC_W", &plane->src_w));
	CHECK_RESULT(get_prop(fd, props, "SRC_H", &plane->src_h));
	CHECK_RESULT(get_prop(fd, props, "type", &plane->type));

	/*
	 * The atomic API makes no guarantee a property is present in object. This test
	 * requires the above common properties since a plane is undefined without them.
	 * Other properties (i.e: rotation and ctm) are optional.
	 */
	get_prop(fd, props, "rotation", &plane->rotation);
	get_prop(fd, props, "PLANE_CTM", &plane->ctm);
	return 0;
}

int set_connector_props(struct atomictest_connector *conn, drmModeAtomicReqPtr pset)
{
	uint32_t id = conn->connector_id;
	CHECK_RESULT(drmModeAtomicAddProperty(pset, id, conn->crtc_id.pid, conn->crtc_id.value));
	return 0;
}

int set_crtc_props(struct atomictest_crtc *crtc, drmModeAtomicReqPtr pset)
{
	uint32_t id = crtc->crtc_id;
	CHECK_RESULT(drmModeAtomicAddProperty(pset, id, crtc->mode_id.pid, crtc->mode_id.value));
	CHECK_RESULT(drmModeAtomicAddProperty(pset, id, crtc->active.pid, crtc->active.value));
	if (crtc->ctm.pid)
		CHECK_RESULT(drmModeAtomicAddProperty(pset, id, crtc->ctm.pid, crtc->ctm.value));
	if (crtc->gamma_lut.pid)
		CHECK_RESULT(
		    drmModeAtomicAddProperty(pset, id, crtc->gamma_lut.pid, crtc->gamma_lut.value));
	return 0;
}

int set_plane_props(struct atomictest_plane *plane, drmModeAtomicReqPtr pset)
{
	uint32_t id = plane->drm_plane.plane_id;
	CHECK_RESULT(drmModeAtomicAddProperty(pset, id, plane->crtc_id.pid, plane->crtc_id.value));
	CHECK_RESULT(drmModeAtomicAddProperty(pset, id, plane->fb_id.pid, plane->fb_id.value));
	CHECK_RESULT(drmModeAtomicAddProperty(pset, id, plane->crtc_x.pid, plane->crtc_x.value));
	CHECK_RESULT(drmModeAtomicAddProperty(pset, id, plane->crtc_y.pid, plane->crtc_y.value));
	CHECK_RESULT(drmModeAtomicAddProperty(pset, id, plane->crtc_w.pid, plane->crtc_w.value));
	CHECK_RESULT(drmModeAtomicAddProperty(pset, id, plane->crtc_h.pid, plane->crtc_h.value));
	CHECK_RESULT(drmModeAtomicAddProperty(pset, id, plane->src_x.pid, plane->src_x.value));
	CHECK_RESULT(drmModeAtomicAddProperty(pset, id, plane->src_y.pid, plane->src_y.value));
	CHECK_RESULT(drmModeAtomicAddProperty(pset, id, plane->src_w.pid, plane->src_w.value));
	CHECK_RESULT(drmModeAtomicAddProperty(pset, id, plane->src_h.pid, plane->src_h.value));
	if (plane->rotation.pid)
		CHECK_RESULT(
		    drmModeAtomicAddProperty(pset, id, plane->rotation.pid, plane->rotation.value));
	if (plane->ctm.pid)
		CHECK_RESULT(drmModeAtomicAddProperty(pset, id, plane->ctm.pid, plane->ctm.value));

	return 0;
}

static int remove_plane_fb(struct atomictest_context *ctx, struct atomictest_plane *plane)
{
	if (plane->bo && plane->fb_id.value) {
		CHECK_RESULT(drmModeRmFB(ctx->fd, plane->fb_id.value));
		gbm_bo_destroy(plane->bo);
		plane->bo = NULL;
		plane->fb_id.value = 0;
	}

	return 0;
}

static int add_plane_fb(struct atomictest_context *ctx, struct atomictest_plane *plane)
{
	if (plane->format_idx < plane->drm_plane.count_formats) {
		CHECK_RESULT(remove_plane_fb(ctx, plane));
		uint32_t flags = (plane->type.value == DRM_PLANE_TYPE_CURSOR) ? GBM_BO_USE_CURSOR
									      : GBM_BO_USE_SCANOUT;
		flags |= GBM_BO_USE_SW_WRITE_RARELY;
		/* TODO(gsingh): add create with modifiers option. */
		plane->bo = gbm_bo_create(gbm, plane->crtc_w.value, plane->crtc_h.value,
					  plane->drm_plane.formats[plane->format_idx], flags);

		CHECK(plane->bo);
		plane->fb_id.value = bs_drm_fb_create_gbm(plane->bo);
		CHECK(plane->fb_id.value);
		CHECK_RESULT(set_plane_props(plane, ctx->pset));
	}

	return 0;
}

static int init_plane(struct atomictest_context *ctx, struct atomictest_plane *plane,
		      uint32_t format, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
		      uint32_t crtc_id)
{
	int32_t idx = get_format_idx(plane, format);
	if (idx < 0)
		return -EINVAL;

	plane->format_idx = idx;
	plane->crtc_x.value = x;
	plane->crtc_y.value = y;
	plane->crtc_w.value = w;
	plane->crtc_h.value = h;
	plane->src_w.value = plane->crtc_w.value << 16;
	plane->src_h.value = plane->crtc_h.value << 16;
	plane->crtc_id.value = crtc_id;

	CHECK_RESULT(add_plane_fb(ctx, plane));
	return 0;
}

static int init_plane_any_format(struct atomictest_context *ctx, struct atomictest_plane *plane,
				 uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t crtc_id,
				 bool yuv)
{
	if (yuv) {
		uint32_t i;
		for (i = 0; i < BS_ARRAY_LEN(yuv_formats); i++)
			if (!init_plane(ctx, plane, yuv_formats[i], x, y, w, h, crtc_id))
				return 0;
	} else {
		// XRGB888 works well with our draw code, so try that first.
		if (!init_plane(ctx, plane, DRM_FORMAT_XRGB8888, x, y, w, h, crtc_id))
			return 0;

		for (uint32_t format_idx = 0; format_idx < plane->drm_plane.count_formats;
		     format_idx++) {
			if (!gbm_device_is_format_supported(
				gbm, plane->drm_plane.formats[format_idx], GBM_BO_USE_SCANOUT))
				continue;

			if (!init_plane(ctx, plane, plane->drm_plane.formats[format_idx], x, y, w,
					h, crtc_id))
				return 0;
		}
	}

	return -EINVAL;
}

static int disable_plane(struct atomictest_context *ctx, struct atomictest_plane *plane)
{
	plane->format_idx = 0;
	plane->crtc_x.value = 0;
	plane->crtc_y.value = 0;
	plane->crtc_w.value = 0;
	plane->crtc_h.value = 0;
	plane->src_w.value = 0;
	plane->src_h.value = 0;
	plane->crtc_id.value = 0;

	if (plane->rotation.pid)
		plane->rotation.value = DRM_ROTATE_0;
	if (plane->ctm.pid)
		plane->ctm.value = 0;

	CHECK_RESULT(remove_plane_fb(ctx, plane));
	CHECK_RESULT(set_plane_props(plane, ctx->pset));
	return 0;
}

static int move_plane(struct atomictest_context *ctx, struct atomictest_crtc *crtc,
		      struct atomictest_plane *plane, uint32_t dx, uint32_t dy)
{
	if (plane->crtc_x.value < (crtc->width - plane->crtc_w.value) &&
	    plane->crtc_y.value < (crtc->height - plane->crtc_h.value)) {
		plane->crtc_x.value += dx;
		plane->crtc_y.value += dy;
		CHECK_RESULT(set_plane_props(plane, ctx->pset));
		return 0;
	}

	return -1;
}

static int scale_plane(struct atomictest_context *ctx, struct atomictest_crtc *crtc,
		       struct atomictest_plane *plane, float dw, float dh)
{
	int32_t plane_w = (int32_t)plane->crtc_w.value + dw * plane->crtc_w.value;
	int32_t plane_h = (int32_t)plane->crtc_h.value + dh * plane->crtc_h.value;
	if (plane_w > 0 && plane_h > 0 && (plane->crtc_x.value + plane_w < crtc->width) &&
	    (plane->crtc_h.value + plane_h < crtc->height)) {
		plane->crtc_w.value = BS_ALIGN((uint32_t)plane_w, 2);
		plane->crtc_h.value = BS_ALIGN((uint32_t)plane_h, 2);
		CHECK_RESULT(set_plane_props(plane, ctx->pset));
		return 0;
	}

	return -1;
}

static void log(struct atomictest_context *ctx)
{
	printf("Committing the following configuration: \n");
	for (uint32_t i = 0; i < ctx->num_crtcs; i++) {
		struct atomictest_plane *plane;
		struct atomictest_crtc *crtc = &ctx->crtcs[i];
		uint32_t num_planes = crtc->num_primary + crtc->num_cursor + crtc->num_overlay;
		if (!crtc->active.value)
			continue;

		printf("----- [CRTC: %u] -----\n", crtc->crtc_id);
		for (uint32_t j = 0; j < num_planes; j++) {
			plane = &crtc->planes[j];
			if (plane->crtc_id.value == crtc->crtc_id && plane->fb_id.value) {
				uint32_t format = gbm_bo_get_format(plane->bo);
				char *fourcc = (char *)&format;
				printf("\t{Plane ID: %u, ", plane->drm_plane.plane_id);
				printf("Plane format: %c%c%c%c, ", fourcc[0], fourcc[1], fourcc[2],
				       fourcc[3]);
				printf("Plane type: ");
				switch (plane->type.value) {
					case DRM_PLANE_TYPE_OVERLAY:
						printf("overlay, ");
						break;
					case DRM_PLANE_TYPE_PRIMARY:
						printf("primary, ");
						break;
					case DRM_PLANE_TYPE_CURSOR:
						printf("cursor, ");
						break;
				}

				printf("CRTC_X: %u, CRTC_Y: %u, CRTC_W: %u, CRTC_H: %u}\n",
				       plane->crtc_x.value, plane->crtc_y.value,
				       plane->crtc_w.value, plane->crtc_h.value);
			}
		}
	}
}

static int test_commit(struct atomictest_context *ctx)
{
	return drmModeAtomicCommit(ctx->fd, ctx->pset,
				   DRM_MODE_ATOMIC_ALLOW_MODESET | DRM_MODE_ATOMIC_TEST_ONLY, NULL);
}

static int commit(struct atomictest_context *ctx)
{
	int ret;
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(ctx->fd, &fds);

	log(ctx);
	ret = drmModeAtomicCommit(ctx->fd, ctx->pset,
				  DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);
	CHECK_RESULT(ret);
	do {
		ret = select(ctx->fd + 1, &fds, NULL, NULL, NULL);
	} while (ret == -1 && errno == EINTR);

	CHECK_RESULT(ret);
	if (FD_ISSET(ctx->fd, &fds))
		drmHandleEvent(ctx->fd, &ctx->drm_event_ctx);

	return 0;
}

static int test_and_commit(struct atomictest_context *ctx, uint32_t sleep_micro_secs)
{
	sleep_micro_secs = automatic ? 0 : sleep_micro_secs;
	if (!test_commit(ctx)) {
		CHECK_RESULT(commit(ctx));
		usleep(sleep_micro_secs);
	} else {
		return TEST_COMMIT_FAIL;
	}

	return 0;
}

static int pageflip_formats(struct atomictest_context *ctx, struct atomictest_crtc *crtc,
			    struct atomictest_plane *plane)
{
	int ret = 0;
	uint32_t flags;
	for (uint32_t i = 0; i < plane->drm_plane.count_formats; i++) {
		flags = (plane->type.value == DRM_PLANE_TYPE_CURSOR) ? GBM_BO_USE_CURSOR
								     : GBM_BO_USE_SCANOUT;
		if (!gbm_device_is_format_supported(gbm, plane->drm_plane.formats[i], flags))
			continue;

		CHECK_RESULT(init_plane(ctx, plane, plane->drm_plane.formats[i], 0, 0, crtc->width,
					crtc->height, crtc->crtc_id));
		CHECK_RESULT(draw_to_plane(ctx->mapper, plane, DRAW_ELLIPSE));
		ret |= test_and_commit(ctx, 1e6);

		// disable, but don't commit, since we can't have an active CRTC without any planes.
		CHECK_RESULT(disable_plane(ctx, plane));
	}

	return ret;
}

static uint32_t get_connection(struct atomictest_crtc *crtc, uint32_t crtc_index)
{
	uint32_t connector_id = 0;
	uint32_t crtc_mask = 1u << crtc_index;
	struct bs_drm_pipe pipe = { 0 };
	struct bs_drm_pipe_plumber *plumber = bs_drm_pipe_plumber_new();
	bs_drm_pipe_plumber_crtc_mask(plumber, crtc_mask);
	if (bs_drm_pipe_plumber_make(plumber, &pipe))
		connector_id = pipe.connector_id;

	bs_drm_pipe_plumber_destroy(&plumber);
	return connector_id;
}

static int enable_crtc(struct atomictest_context *ctx, struct atomictest_crtc *crtc)
{
	drmModeAtomicSetCursor(ctx->pset, 0);

	for (uint32_t i = 0; i < ctx->num_connectors; i++) {
		ctx->connectors[i].crtc_id.value = 0;
		set_connector_props(&ctx->connectors[i], ctx->pset);
	}

	for (uint32_t i = 0; i < ctx->num_crtcs; i++) {
		if (&ctx->crtcs[i] == crtc) {
			uint32_t connector_id = get_connection(crtc, i);
			CHECK(connector_id);
			for (uint32_t j = 0; j < ctx->num_connectors; j++) {
				if (connector_id == ctx->connectors[j].connector_id) {
					ctx->connectors[j].crtc_id.value = crtc->crtc_id;
					set_connector_props(&ctx->connectors[j], ctx->pset);
					break;
				}
			}

			break;
		}
	}

	int ret = -EINVAL;
	int cursor = drmModeAtomicGetCursor(ctx->pset);

	for (uint32_t i = 0; i < ctx->num_modes; i++) {
		struct atomictest_mode *mode = &ctx->modes[i];
		drmModeAtomicSetCursor(ctx->pset, cursor);

		crtc->mode_id.value = mode->id;
		crtc->active.value = 1;
		crtc->width = mode->width;
		crtc->height = mode->height;

		set_crtc_props(crtc, ctx->pset);
		ret = drmModeAtomicCommit(ctx->fd, ctx->pset,
					  DRM_MODE_ATOMIC_TEST_ONLY | DRM_MODE_ATOMIC_ALLOW_MODESET,
					  NULL);
		if (!ret)
			return 0;
	}

	bs_debug_error("[CRTC:%d]: failed to find mode", crtc->crtc_id);
	return ret;
}

static int disable_crtc(struct atomictest_context *ctx, struct atomictest_crtc *crtc)
{
	for (uint32_t i = 0; i < ctx->num_connectors; i++) {
		ctx->connectors[i].crtc_id.value = 0;
		set_connector_props(&ctx->connectors[i], ctx->pset);
	}

	crtc->mode_id.value = 0;
	crtc->active.value = 0;
	if (crtc->ctm.pid)
		crtc->ctm.value = 0;
	if (crtc->gamma_lut.pid)
		crtc->gamma_lut.value = 0;

	set_crtc_props(crtc, ctx->pset);
	int ret = drmModeAtomicCommit(ctx->fd, ctx->pset, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);
	CHECK_RESULT(ret);
	return ret;
}

static struct atomictest_context *new_context(uint32_t num_connectors, uint32_t num_crtcs,
					      uint32_t num_planes)
{
	struct atomictest_context *ctx = calloc(1, sizeof(*ctx));

	ctx->mapper = bs_mapper_gem_new();
	if (ctx->mapper == NULL) {
		bs_debug_error("failed to create mapper object");
		free(ctx);
		return NULL;
	}

	ctx->connectors = calloc(num_connectors, sizeof(*ctx->connectors));
	ctx->crtcs = calloc(num_crtcs, sizeof(*ctx->crtcs));
	for (uint32_t i = 0; i < num_crtcs; i++) {
		ctx->crtcs[i].planes = calloc(num_planes, sizeof(*ctx->crtcs[i].planes));
		ctx->crtcs[i].overlay_idx = calloc(num_planes, sizeof(uint32_t));
		ctx->crtcs[i].primary_idx = calloc(num_planes, sizeof(uint32_t));
		ctx->crtcs[i].cursor_idx = calloc(num_planes, sizeof(uint32_t));
	}

	ctx->num_connectors = num_connectors;
	ctx->num_crtcs = num_crtcs;
	ctx->num_modes = 0;
	ctx->modes = NULL;
	ctx->pset = drmModeAtomicAlloc();
	ctx->drm_event_ctx.version = DRM_EVENT_CONTEXT_VERSION;
	ctx->drm_event_ctx.page_flip_handler = page_flip_handler;

	return ctx;
}

static void free_context(struct atomictest_context *ctx)
{
	for (uint32_t i = 0; i < ctx->num_crtcs; i++) {
		uint32_t num_planes = ctx->crtcs[i].num_primary + ctx->crtcs[i].num_cursor +
				      ctx->crtcs[i].num_overlay;

		for (uint32_t j = 0; j < num_planes; j++) {
			remove_plane_fb(ctx, &ctx->crtcs[i].planes[j]);
			free(ctx->crtcs[i].planes[j].drm_plane.formats);
		}

		free(ctx->crtcs[i].planes);
		free(ctx->crtcs[i].overlay_idx);
		free(ctx->crtcs[i].cursor_idx);
		free(ctx->crtcs[i].primary_idx);
	}

	drmModeAtomicFree(ctx->pset);
	free(ctx->modes);
	free(ctx->crtcs);
	free(ctx->connectors);
	bs_mapper_destroy(ctx->mapper);
	free(ctx);
}

static struct atomictest_context *query_kms(int fd)
{
	drmModeRes *res = drmModeGetResources(fd);
	if (res == NULL) {
		bs_debug_error("failed to get drm resources");
		return false;
	}

	drmModePlaneRes *plane_res = drmModeGetPlaneResources(fd);
	if (plane_res == NULL) {
		bs_debug_error("failed to get plane resources");
		drmModeFreeResources(res);
		return NULL;
	}

	struct atomictest_context *ctx =
	    new_context(res->count_connectors, res->count_crtcs, plane_res->count_planes);
	if (ctx == NULL) {
		bs_debug_error("failed to allocate atomic context");
		drmModeFreePlaneResources(plane_res);
		drmModeFreeResources(res);
		return NULL;
	}

	ctx->fd = fd;
	drmModeObjectPropertiesPtr props = NULL;

	for (uint32_t conn_index = 0; conn_index < res->count_connectors; conn_index++) {
		uint32_t conn_id = res->connectors[conn_index];
		ctx->connectors[conn_index].connector_id = conn_id;
		props = drmModeObjectGetProperties(fd, conn_id, DRM_MODE_OBJECT_CONNECTOR);
		get_connector_props(fd, &ctx->connectors[conn_index], props);

		drmModeConnector *connector = drmModeGetConnector(fd, conn_id);
		for (uint32_t mode_index = 0; mode_index < connector->count_modes; mode_index++) {
			ctx->modes =
			    realloc(ctx->modes, (ctx->num_modes + 1) * sizeof(*ctx->modes));
			drmModeCreatePropertyBlob(fd, &connector->modes[mode_index],
						  sizeof(drmModeModeInfo),
						  &ctx->modes[ctx->num_modes].id);
			ctx->modes[ctx->num_modes].width = connector->modes[mode_index].hdisplay;
			ctx->modes[ctx->num_modes].height = connector->modes[mode_index].vdisplay;
			ctx->num_modes++;
		}

		drmModeFreeConnector(connector);
		drmModeFreeObjectProperties(props);
		props = NULL;
	}

	uint32_t crtc_index;
	for (crtc_index = 0; crtc_index < res->count_crtcs; crtc_index++) {
		ctx->crtcs[crtc_index].crtc_id = res->crtcs[crtc_index];
		props =
		    drmModeObjectGetProperties(fd, res->crtcs[crtc_index], DRM_MODE_OBJECT_CRTC);
		get_crtc_props(fd, &ctx->crtcs[crtc_index], props);

		drmModeFreeObjectProperties(props);
		props = NULL;
	}

	uint32_t overlay_idx, primary_idx, cursor_idx, idx;

	for (uint32_t plane_index = 0; plane_index < plane_res->count_planes; plane_index++) {
		drmModePlane *plane = drmModeGetPlane(fd, plane_res->planes[plane_index]);
		if (plane == NULL) {
			bs_debug_error("failed to get plane id %u", plane_res->planes[plane_index]);
			continue;
		}

		uint32_t crtc_mask = 0;

		drmModeObjectPropertiesPtr props = drmModeObjectGetProperties(
		    fd, plane_res->planes[plane_index], DRM_MODE_OBJECT_PLANE);

		for (crtc_index = 0; crtc_index < res->count_crtcs; crtc_index++) {
			crtc_mask = (1 << crtc_index);
			if (plane->possible_crtcs & crtc_mask) {
				struct atomictest_crtc *crtc = &ctx->crtcs[crtc_index];
				cursor_idx = crtc->num_cursor;
				primary_idx = crtc->num_primary;
				overlay_idx = crtc->num_overlay;
				idx = cursor_idx + primary_idx + overlay_idx;
				copy_drm_plane(&crtc->planes[idx].drm_plane, plane);
				get_plane_props(fd, &crtc->planes[idx], props);
				switch (crtc->planes[idx].type.value) {
					case DRM_PLANE_TYPE_OVERLAY:
						crtc->overlay_idx[overlay_idx] = idx;
						crtc->num_overlay++;
						break;
					case DRM_PLANE_TYPE_PRIMARY:
						crtc->primary_idx[primary_idx] = idx;
						crtc->num_primary++;
						break;
					case DRM_PLANE_TYPE_CURSOR:
						crtc->cursor_idx[cursor_idx] = idx;
						crtc->num_cursor++;
						break;
					default:
						bs_debug_error("invalid plane type returned");
						return NULL;
				}

				/*
				 * The DRM UAPI states that cursor and overlay framebuffers may be
				 * present after a CRTC disable, so zero this out so we can get a
				 * clean slate.
				 */
				crtc->planes[idx].fb_id.value = 0;
			}
		}

		drmModeFreePlane(plane);
		drmModeFreeObjectProperties(props);
		props = NULL;
	}

	drmModeFreePlaneResources(plane_res);
	drmModeFreeResources(res);
	return ctx;
}

static int test_multiple_planes(struct atomictest_context *ctx, struct atomictest_crtc *crtc)
{
	int ret = 0;
	struct atomictest_plane *primary, *overlay, *cursor;
	for (uint32_t i = 0; i < crtc->num_primary; i++) {
		bool video = true;
		uint32_t x, y;
		for (uint32_t j = 0; j < crtc->num_overlay; j++) {
			overlay = get_plane(crtc, j, DRM_PLANE_TYPE_OVERLAY);
			x = crtc->width >> (j + 2);
			y = crtc->height >> (j + 2);
			// drmModeAddFB2 requires the height and width are even for sub-sampled YUV
			// formats.
			x = BS_ALIGN(x, 2);
			y = BS_ALIGN(y, 2);
			if (video &&
			    !init_plane_any_format(ctx, overlay, x, y, x, y, crtc->crtc_id, true)) {
				CHECK_RESULT(draw_to_plane(ctx->mapper, overlay, DRAW_STRIPE));
				video = false;
			} else {
				CHECK_RESULT(init_plane_any_format(ctx, overlay, x, y, x, y,
								   crtc->crtc_id, false));
				CHECK_RESULT(draw_to_plane(ctx->mapper, overlay, DRAW_LINES));
			}
		}

		for (uint32_t j = 0; j < crtc->num_cursor; j++) {
			x = crtc->width >> (j + 2);
			y = crtc->height >> (j + 2);
			cursor = get_plane(crtc, j, DRM_PLANE_TYPE_CURSOR);
			CHECK_RESULT(init_plane(ctx, cursor, DRM_FORMAT_ARGB8888, x, y, CURSOR_SIZE,
						CURSOR_SIZE, crtc->crtc_id));
			CHECK_RESULT(draw_to_plane(ctx->mapper, cursor, DRAW_CURSOR));
		}

		primary = get_plane(crtc, i, DRM_PLANE_TYPE_PRIMARY);
		CHECK_RESULT(init_plane_any_format(ctx, primary, 0, 0, crtc->width, crtc->height,
						   crtc->crtc_id, false));
		CHECK_RESULT(draw_to_plane(ctx->mapper, primary, DRAW_ELLIPSE));

		uint32_t num_planes = crtc->num_primary + crtc->num_cursor + crtc->num_overlay;
		int done = 0;
		struct atomictest_plane *plane;
		while (!done) {
			done = 1;
			for (uint32_t j = 0; j < num_planes; j++) {
				plane = &crtc->planes[j];
				if (plane->type.value != DRM_PLANE_TYPE_PRIMARY)
					done &= move_plane(ctx, crtc, plane, 40, 40);
			}

			ret |= test_and_commit(ctx, 1e6 / 60);
		}

		ret |= test_and_commit(ctx, 1e6);

		/* Disable primary plane and verify overlays show up. */
		CHECK_RESULT(disable_plane(ctx, primary));
		ret |= test_and_commit(ctx, 1e6);
	}

	return ret;
}

static int test_video_overlay(struct atomictest_context *ctx, struct atomictest_crtc *crtc)
{
	int ret = 0;
	struct atomictest_plane *overlay;
	for (uint32_t i = 0; i < crtc->num_overlay; i++) {
		overlay = get_plane(crtc, i, DRM_PLANE_TYPE_OVERLAY);
		if (init_plane_any_format(ctx, overlay, 0, 0, 800, 800, crtc->crtc_id, true))
			continue;

		CHECK_RESULT(draw_to_plane(ctx->mapper, overlay, DRAW_STRIPE));
		while (!move_plane(ctx, crtc, overlay, 40, 40))
			ret |= test_and_commit(ctx, 1e6 / 60);
	}

	return ret;
}

static int test_orientation(struct atomictest_context *ctx, struct atomictest_crtc *crtc)
{
	int ret = 0;
	struct atomictest_plane *primary, *overlay;
	for (uint32_t i = 0; i < crtc->num_overlay; i++) {
		overlay = get_plane(crtc, i, DRM_PLANE_TYPE_OVERLAY);
		if (!overlay->rotation.pid)
			continue;

		CHECK_RESULT(init_plane_any_format(ctx, overlay, 0, 0, crtc->width, crtc->height,
						   crtc->crtc_id, false));
		overlay->rotation.value = DRM_ROTATE_0;
		set_plane_props(overlay, ctx->pset);
		CHECK_RESULT(draw_to_plane(ctx->mapper, overlay, DRAW_LINES));
		ret |= test_and_commit(ctx, 1e6);

		overlay->rotation.value = DRM_REFLECT_Y;
		set_plane_props(overlay, ctx->pset);
		ret |= test_and_commit(ctx, 1e6);

		CHECK_RESULT(disable_plane(ctx, overlay));
	}

	for (uint32_t i = 0; i < crtc->num_primary; i++) {
		primary = get_plane(crtc, i, DRM_PLANE_TYPE_PRIMARY);
		if (!primary->rotation.pid)
			continue;

		CHECK_RESULT(init_plane_any_format(ctx, primary, 0, 0, crtc->width, crtc->height,
						   crtc->crtc_id, false));

		primary->rotation.value = DRM_ROTATE_0;
		set_plane_props(primary, ctx->pset);
		CHECK_RESULT(draw_to_plane(ctx->mapper, primary, DRAW_LINES));
		ret |= test_and_commit(ctx, 1e6);

		primary->rotation.value = DRM_REFLECT_Y;
		set_plane_props(primary, ctx->pset);
		ret |= test_and_commit(ctx, 1e6);

		CHECK_RESULT(disable_plane(ctx, primary));
	}

	return ret;
}

static int test_plane_ctm(struct atomictest_context *ctx, struct atomictest_crtc *crtc)
{
	int ret = 0;
	struct atomictest_plane *primary, *overlay;

	for (uint32_t i = 0; i < crtc->num_overlay; i++) {
		overlay = get_plane(crtc, i, DRM_PLANE_TYPE_OVERLAY);
		if (!overlay->ctm.pid)
			continue;

		CHECK_RESULT(init_plane(ctx, overlay, DRM_FORMAT_XRGB8888, 0, 0, crtc->width,
					crtc->height, crtc->crtc_id));

		CHECK_RESULT(drmModeCreatePropertyBlob(ctx->fd, identity_ctm, sizeof(identity_ctm),
						       &overlay->ctm.value));
		set_plane_props(overlay, ctx->pset);
		CHECK_RESULT(draw_to_plane(ctx->mapper, overlay, DRAW_LINES));
		ret |= test_and_commit(ctx, 1e6);
		CHECK_RESULT(drmModeDestroyPropertyBlob(ctx->fd, overlay->ctm.value));

		CHECK_RESULT(drmModeCreatePropertyBlob(ctx->fd, red_shift_ctm,
						       sizeof(red_shift_ctm), &overlay->ctm.value));
		set_plane_props(overlay, ctx->pset);
		ret |= test_and_commit(ctx, 1e6);
		CHECK_RESULT(drmModeDestroyPropertyBlob(ctx->fd, overlay->ctm.value));

		CHECK_RESULT(disable_plane(ctx, overlay));
	}

	for (uint32_t i = 0; i < crtc->num_primary; i++) {
		primary = get_plane(crtc, i, DRM_PLANE_TYPE_PRIMARY);
		if (!primary->ctm.pid)
			continue;

		CHECK_RESULT(init_plane_any_format(ctx, primary, 0, 0, crtc->width, crtc->height,
						   crtc->crtc_id, false));
		CHECK_RESULT(drmModeCreatePropertyBlob(ctx->fd, identity_ctm, sizeof(identity_ctm),
						       &primary->ctm.value));
		set_plane_props(primary, ctx->pset);
		CHECK_RESULT(draw_to_plane(ctx->mapper, primary, DRAW_LINES));
		ret |= test_and_commit(ctx, 1e6);
		CHECK_RESULT(drmModeDestroyPropertyBlob(ctx->fd, primary->ctm.value));

		CHECK_RESULT(drmModeCreatePropertyBlob(ctx->fd, red_shift_ctm,
						       sizeof(red_shift_ctm), &primary->ctm.value));
		set_plane_props(primary, ctx->pset);
		ret |= test_and_commit(ctx, 1e6);
		CHECK_RESULT(drmModeDestroyPropertyBlob(ctx->fd, primary->ctm.value));

		CHECK_RESULT(disable_plane(ctx, primary));
	}

	return ret;
}

static int test_video_underlay(struct atomictest_context *ctx, struct atomictest_crtc *crtc)
{
	int ret = 0;
	int i = 0;
	struct atomictest_plane *underlay = 0;
	struct atomictest_plane *primary = 0;

	for (; i < crtc->num_primary + crtc->num_overlay; ++i) {
		if (crtc->planes[i].type.value != DRM_PLANE_TYPE_CURSOR) {
			if (!underlay) {
				underlay = &crtc->planes[i];
			} else {
				primary = &crtc->planes[i];
				break;
			}
		}
	}
	if (!underlay || !primary)
		return 0;

	CHECK_RESULT(init_plane_any_format(ctx, underlay, 0, 0, crtc->width >> 2, crtc->height >> 2,
					   crtc->crtc_id, true));
	CHECK_RESULT(draw_to_plane(ctx->mapper, underlay, DRAW_LINES));

	CHECK_RESULT(init_plane(ctx, primary, DRM_FORMAT_ARGB8888, 0, 0, crtc->width, crtc->height,
				crtc->crtc_id));
	CHECK_RESULT(draw_to_plane(ctx->mapper, primary, DRAW_TRANSPARENT_HOLE));

	while (!move_plane(ctx, crtc, underlay, 50, 20))
		ret |= test_and_commit(ctx, 1e6 / 60);

	return ret;
}

static int test_fullscreen_video(struct atomictest_context *ctx, struct atomictest_crtc *crtc)
{
	int ret = 0;
	struct atomictest_plane *primary;
	for (uint32_t i = 0; i < crtc->num_primary; i++) {
		primary = get_plane(crtc, i, DRM_PLANE_TYPE_PRIMARY);
		if (init_plane_any_format(ctx, primary, 0, 0, crtc->width, crtc->height,
					  crtc->crtc_id, true))
			continue;

		CHECK_RESULT(draw_to_plane(ctx->mapper, primary, DRAW_STRIPE));
		ret |= test_and_commit(ctx, 1e6);
	}

	return ret;
}

static int test_disable_primary(struct atomictest_context *ctx, struct atomictest_crtc *crtc)
{
	int ret = 0;
	struct atomictest_plane *primary, *overlay;
	for (uint32_t i = 0; i < crtc->num_primary; i++) {
		for (uint32_t j = 0; j < crtc->num_overlay; j++) {
			overlay = get_plane(crtc, j, DRM_PLANE_TYPE_OVERLAY);
			uint32_t x = crtc->width >> (j + 2);
			uint32_t y = crtc->height >> (j + 2);
			CHECK_RESULT(
			    init_plane_any_format(ctx, overlay, x, y, x, y, crtc->crtc_id, false));
			CHECK_RESULT(draw_to_plane(ctx->mapper, overlay, DRAW_LINES));
		}

		primary = get_plane(crtc, i, DRM_PLANE_TYPE_PRIMARY);
		CHECK_RESULT(init_plane_any_format(ctx, primary, 0, 0, crtc->width, crtc->height,
						   crtc->crtc_id, false));
		CHECK_RESULT(draw_to_plane(ctx->mapper, primary, DRAW_ELLIPSE));
		ret |= test_and_commit(ctx, 1e6);

		/* Disable primary plane. */
		disable_plane(ctx, primary);
		ret |= test_and_commit(ctx, 1e6);
	}

	return ret;
}

static int test_rgba_primary(struct atomictest_context *ctx, struct atomictest_crtc *crtc)
{
	int ret = 0;
	struct atomictest_plane *primary;
	for (uint32_t i = 0; i < crtc->num_primary; i++) {
		primary = get_plane(crtc, i, DRM_PLANE_TYPE_PRIMARY);
		CHECK_RESULT(init_plane(ctx, primary, DRM_FORMAT_ARGB8888, 0, 0, crtc->width,
					crtc->height, crtc->crtc_id));

		CHECK_RESULT(draw_to_plane(ctx->mapper, primary, DRAW_LINES));

		ret |= test_and_commit(ctx, 1e6);
	}

	return ret;
}

static int test_overlay_pageflip(struct atomictest_context *ctx, struct atomictest_crtc *crtc)
{
	struct atomictest_plane *overlay;
	for (uint32_t i = 0; i < crtc->num_overlay; i++) {
		overlay = get_plane(crtc, i, DRM_PLANE_TYPE_OVERLAY);
		CHECK_RESULT(pageflip_formats(ctx, crtc, overlay));
	}

	return 0;
}

static int test_overlay_downscaling(struct atomictest_context *ctx, struct atomictest_crtc *crtc)
{
	int ret = 0;
	struct atomictest_plane *overlay;
	uint32_t w = BS_ALIGN(crtc->width / 2, 2);
	uint32_t h = BS_ALIGN(crtc->height / 2, 2);
	for (uint32_t i = 0; i < crtc->num_overlay; i++) {
		overlay = get_plane(crtc, i, DRM_PLANE_TYPE_OVERLAY);
		if (init_plane_any_format(ctx, overlay, 0, 0, w, h, crtc->crtc_id, true))
			CHECK_RESULT(init_plane(ctx, overlay, DRM_FORMAT_XRGB8888, 0, 0, w, h,
						crtc->crtc_id));
		CHECK_RESULT(draw_to_plane(ctx->mapper, overlay, DRAW_LINES));
		ret |= test_and_commit(ctx, 1e6);

		while (!scale_plane(ctx, crtc, overlay, -.1f, -.1f) && !test_commit(ctx)) {
			CHECK_RESULT(commit(ctx));
			usleep(1e6);
		}

		disable_plane(ctx, overlay);
	}

	return ret;
}

static int test_overlay_upscaling(struct atomictest_context *ctx, struct atomictest_crtc *crtc)
{
	int ret = 0;
	struct atomictest_plane *overlay;
	uint32_t w = BS_ALIGN(crtc->width / 4, 2);
	uint32_t h = BS_ALIGN(crtc->height / 4, 2);
	for (uint32_t i = 0; i < crtc->num_overlay; i++) {
		overlay = get_plane(crtc, i, DRM_PLANE_TYPE_OVERLAY);
		if (init_plane_any_format(ctx, overlay, 0, 0, w, h, crtc->crtc_id, true))
			CHECK_RESULT(init_plane(ctx, overlay, DRM_FORMAT_XRGB8888, 0, 0, w, h,
						crtc->crtc_id));
		CHECK_RESULT(draw_to_plane(ctx->mapper, overlay, DRAW_LINES));
		ret |= test_and_commit(ctx, 1e6);

		while (!scale_plane(ctx, crtc, overlay, .1f, .1f) && !test_commit(ctx)) {
			CHECK_RESULT(commit(ctx));
			usleep(1e6);
		}

		disable_plane(ctx, overlay);
	}

	return ret;
}

static int test_primary_pageflip(struct atomictest_context *ctx, struct atomictest_crtc *crtc)
{
	struct atomictest_plane *primary;
	for (uint32_t i = 0; i < crtc->num_primary; i++) {
		primary = get_plane(crtc, i, DRM_PLANE_TYPE_PRIMARY);
		CHECK_RESULT(pageflip_formats(ctx, crtc, primary));
	}

	return 0;
}

static int test_crtc_ctm(struct atomictest_context *ctx, struct atomictest_crtc *crtc)
{
	int ret = 0;
	struct atomictest_plane *primary;
	if (!crtc->ctm.pid)
		return 0;

	CHECK_RESULT(drmModeCreatePropertyBlob(ctx->fd, identity_ctm, sizeof(identity_ctm),
					       &crtc->ctm.value));
	set_crtc_props(crtc, ctx->pset);
	for (uint32_t i = 0; i < crtc->num_primary; i++) {
		primary = get_plane(crtc, i, DRM_PLANE_TYPE_PRIMARY);

		CHECK_RESULT(init_plane(ctx, primary, DRM_FORMAT_XRGB8888, 0, 0, crtc->width,
					crtc->height, crtc->crtc_id));
		CHECK_RESULT(draw_to_plane(ctx->mapper, primary, DRAW_LINES));
		ret |= test_and_commit(ctx, 1e6);

		primary->crtc_id.value = 0;
		CHECK_RESULT(set_plane_props(primary, ctx->pset));
	}

	CHECK_RESULT(drmModeDestroyPropertyBlob(ctx->fd, crtc->ctm.value));

	CHECK_RESULT(drmModeCreatePropertyBlob(ctx->fd, red_shift_ctm, sizeof(red_shift_ctm),
					       &crtc->ctm.value));
	set_crtc_props(crtc, ctx->pset);
	for (uint32_t i = 0; i < crtc->num_primary; i++) {
		primary = get_plane(crtc, i, DRM_PLANE_TYPE_PRIMARY);
		primary->crtc_id.value = crtc->crtc_id;
		CHECK_RESULT(set_plane_props(primary, ctx->pset));

		ret |= test_and_commit(ctx, 1e6);

		primary->crtc_id.value = 0;
		CHECK_RESULT(disable_plane(ctx, primary));
	}

	CHECK_RESULT(drmModeDestroyPropertyBlob(ctx->fd, crtc->ctm.value));

	return ret;
}

static void gamma_linear(struct drm_color_lut *table, int size)
{
	for (int i = 0; i < size; i++) {
		float v = (float)(i) / (float)(size - 1);
		v *= 65535.0f;
		table[i].red = (uint16_t)v;
		table[i].green = (uint16_t)v;
		table[i].blue = (uint16_t)v;
	}
}

static void gamma_step(struct drm_color_lut *table, int size)
{
	for (int i = 0; i < size; i++) {
		float v = (i < size / 2) ? 0 : 65535;
		table[i].red = (uint16_t)v;
		table[i].green = (uint16_t)v;
		table[i].blue = (uint16_t)v;
	}
}

static int test_crtc_gamma(struct atomictest_context *ctx, struct atomictest_crtc *crtc)
{
	int ret = 0;
	struct atomictest_plane *primary;
	printf("[  test_crtc_gamma  ] gamma_lut.pid %d\n", crtc->gamma_lut.pid);
	printf("[  test_crtc_gamma  ] gamma_lut_size.pid %d\n", crtc->gamma_lut_size.pid);
	printf("[  test_crtc_gamma  ] gamma_lut_size.value %d\n", crtc->gamma_lut_size.value);
	if (!crtc->gamma_lut.pid || !crtc->gamma_lut_size.pid)
		return 0;

	if (crtc->gamma_lut_size.value == 0)
		return 0;

	struct drm_color_lut *gamma_table =
	    calloc(crtc->gamma_lut_size.value, sizeof(*gamma_table));
	
	gamma_linear(gamma_table, crtc->gamma_lut_size.value);
	CHECK_RESULT(drmModeCreatePropertyBlob(
	    ctx->fd, gamma_table, sizeof(struct drm_color_lut) * crtc->gamma_lut_size.value,
	    &crtc->gamma_lut.value));

	for (uint32_t i = 0; i < crtc->num_primary; i++) {
		primary = get_plane(crtc, i, DRM_PLANE_TYPE_PRIMARY);

		CHECK_RESULT(init_plane(ctx, primary, DRM_FORMAT_XRGB8888, 0, 0, crtc->width,
					crtc->height, crtc->crtc_id));
		set_crtc_props(crtc, ctx->pset);

		CHECK_RESULT(draw_to_plane(ctx->mapper, primary, DRAW_STRIPE));
		ret |= test_and_commit(ctx, 1e6);

		CHECK_RESULT(disable_plane(ctx, primary));
	}

	gamma_step(gamma_table, crtc->gamma_lut_size.value);
	CHECK_RESULT(drmModeCreatePropertyBlob(
	    ctx->fd, gamma_table, sizeof(struct drm_color_lut) * crtc->gamma_lut_size.value,
	    &crtc->gamma_lut.value));

	for (uint32_t i = 0; i < crtc->num_primary; i++) {
		primary = get_plane(crtc, i, DRM_PLANE_TYPE_PRIMARY);

		CHECK_RESULT(init_plane(ctx, primary, DRM_FORMAT_XRGB8888, 0, 0, crtc->width,
					crtc->height, crtc->crtc_id));
		set_crtc_props(crtc, ctx->pset);

		CHECK_RESULT(draw_to_plane(ctx->mapper, primary, DRAW_STRIPE));
		ret |= test_and_commit(ctx, 1e6);

		CHECK_RESULT(disable_plane(ctx, primary));
	}

	CHECK_RESULT(drmModeDestroyPropertyBlob(ctx->fd, crtc->gamma_lut.value));
	free(gamma_table);

	return ret;
}

static const struct atomictest_testcase cases[] = {
	{ "disable_primary", test_disable_primary },
	{ "rgba_primary", test_rgba_primary },
	{ "fullscreen_video", test_fullscreen_video },
	{ "multiple_planes", test_multiple_planes },
	{ "overlay_pageflip", test_overlay_pageflip },
	{ "overlay_downscaling", test_overlay_downscaling },
	{ "overlay_upscaling", test_overlay_upscaling },
	{ "primary_pageflip", test_primary_pageflip },
	{ "video_overlay", test_video_overlay },
	{ "orientation", test_orientation },
	{ "video_underlay", test_video_underlay },
	/* CTM stands for Color Transform Matrix. */
	{ "plane_ctm", test_plane_ctm },
	{ "crtc_ctm", test_crtc_ctm },
	{ "crtc_gamma", test_crtc_gamma },
};

static int run_testcase(struct atomictest_context *ctx, struct atomictest_crtc *crtc,
			test_function func)
{
	int cursor = drmModeAtomicGetCursor(ctx->pset);
	uint32_t num_planes = crtc->num_primary + crtc->num_cursor + crtc->num_overlay;

	int ret = func(ctx, crtc);

	for (uint32_t i = 0; i < num_planes; i++)
		disable_plane(ctx, &crtc->planes[i]);

	drmModeAtomicSetCursor(ctx->pset, cursor);

	CHECK_RESULT(commit(ctx));
	usleep(1e6 / 60);

	return ret;
}

static int run_atomictest(const char *name, uint32_t crtc_mask)
{
	int ret = 0;
	uint32_t num_run = 0;
	int fd = bs_drm_open_main_display();
	CHECK_RESULT(fd);

	gbm = gbm_create_device(fd);
	if (!gbm) {
		bs_debug_error("failed to create gbm device");
		ret = -1;
		goto destroy_fd;
	}

	ret = drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	if (ret) {
		bs_debug_error("failed to enable DRM_CLIENT_CAP_UNIVERSAL_PLANES");
		ret = -1;
		goto destroy_gbm_device;
	}

	ret = drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 1);
	if (ret) {
		bs_debug_error("failed to enable DRM_CLIENT_CAP_ATOMIC");
		ret = -1;
		goto destroy_gbm_device;
	}

	struct atomictest_context *ctx = query_kms(fd);
	if (!ctx) {
		bs_debug_error("querying atomictest failed.");
		ret = -1;
		goto destroy_gbm_device;
	}

	struct atomictest_crtc *crtc;
	for (uint32_t crtc_index = 0; crtc_index < ctx->num_crtcs; crtc_index++) {
		crtc = &ctx->crtcs[crtc_index];
		if (!((1 << crtc_index) & crtc_mask))
			continue;

		for (uint32_t i = 0; i < BS_ARRAY_LEN(cases); i++) {
			if (strcmp(cases[i].name, name) && strcmp("all", name))
				continue;

			num_run++;
			ret = enable_crtc(ctx, crtc);
			if (ret)
				goto out;

			ret = run_testcase(ctx, crtc, cases[i].test_func);
			if (ret < 0)
				goto out;
			else if (ret == TEST_COMMIT_FAIL)
				bs_debug_warning("%s failed test commit, testcase not run.",
						 cases[i].name);

			ret = disable_crtc(ctx, crtc);
			if (ret)
				goto out;
		}
	}

	ret = (num_run == 0);

out:
	free_context(ctx);
destroy_gbm_device:
	gbm_device_destroy(gbm);
destroy_fd:
	close(fd);

	return ret;
}

static const struct option longopts[] = {
	{ "crtc", required_argument, NULL, 'c' },
	{ "test_name", required_argument, NULL, 't' },
	{ "help", no_argument, NULL, 'h' },
	{ "automatic", no_argument, NULL, 'a' },
	{ 0, 0, 0, 0 },
};

static void print_help(const char *argv0)
{
	printf("usage: %s -t <test_name> -c <crtc_index> -a (if running automatically)\n", argv0);
	printf("A valid name test is one the following:\n");
	for (uint32_t i = 0; i < BS_ARRAY_LEN(cases); i++)
		printf("%s\n", cases[i].name);
	printf("all\n");
}

int main(int argc, char **argv)
{
	int c;
	char *name = NULL;
	int32_t crtc_idx = -1;
	uint32_t crtc_mask = ~0;
	while ((c = getopt_long(argc, argv, "c:t:h:a", longopts, NULL)) != -1) {
		switch (c) {
			case 'a':
				automatic = true;
				break;
			case 'c':
				if (sscanf(optarg, "%d", &crtc_idx) != 1)
					goto print;
				break;
			case 't':
				if (name) {
					free(name);
					name = NULL;
				}

				name = strdup(optarg);
				break;
			case 'h':
				goto print;
			default:
				goto print;
		}
	}

	if (!name)
		goto print;

	if (crtc_idx >= 0)
		crtc_mask = 1 << crtc_idx;

	int ret = run_atomictest(name, crtc_mask);
	if (ret == 0)
		printf("[  PASSED  ] atomictest.%s\n", name);
	else if (ret < 0)
		printf("[  FAILED  ] atomictest.%s\n", name);

	free(name);

	if (ret > 0)
		goto print;

	return ret;

print:
	print_help(argv[0]);
	return 0;
}
