/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <ctype.h>
#include <getopt.h>
#include <math.h>

#include "bs_drm.h"

#define MAX_TEST_PLANES 4

static int64_t ns_since(struct timespec *since)
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	return (int64_t)(now.tv_sec - since->tv_sec) * 1000000000 + (int64_t)now.tv_nsec -
	       (int64_t)since->tv_nsec;
}

static drmModeModeInfoPtr find_best_mode(int mode_count, drmModeModeInfoPtr modes)
{
	if (mode_count <= 0 || modes == NULL)
		return NULL;

	for (int m = 0; m < mode_count; m++)
		if (modes[m].type & DRM_MODE_TYPE_PREFERRED)
			return &modes[m];

	return &modes[0];
}

static bool is_format_supported(uint32_t format_count, uint32_t *formats, uint32_t format)
{
	for (uint32_t i = 0; i < format_count; i++)
		if (formats[i] == format)
			return true;
	return false;
}

static bool find_overlay_planes(int fd, uint32_t crtc_id, size_t plane_count, uint32_t *formats,
				uint32_t *plane_ids)
{
	drmModeRes *res = drmModeGetResources(fd);
	if (res == NULL) {
		bs_debug_error("failed to get drm resources");
		return false;
	}

	uint32_t crtc_mask = 0;
	for (int crtc_index = 0; crtc_index < res->count_crtcs; crtc_index++) {
		if (res->crtcs[crtc_index] == crtc_id) {
			crtc_mask = (1 << crtc_index);
			break;
		}
	}
	if (crtc_mask == 0) {
		bs_debug_error("invalid crtc id %u", crtc_id);
		drmModeFreeResources(res);
		return false;
	}

	drmModePlaneRes *plane_res = drmModeGetPlaneResources(fd);
	if (plane_res == NULL) {
		bs_debug_error("failed to get plane resources");
		drmModeFreeResources(res);
		return false;
	}

	for (size_t out_index = 0; out_index < plane_count; out_index++)
		plane_ids[out_index] = 0;

	size_t remaining_out = plane_count;
	for (uint32_t plane_index = 0; remaining_out > 0 && plane_index < plane_res->count_planes;
	     plane_index++) {
		drmModePlane *plane = drmModeGetPlane(fd, plane_res->planes[plane_index]);
		if (plane == NULL) {
			bs_debug_error("failed to get plane id %u", plane_res->planes[plane_index]);
			continue;
		}

		size_t out_index;
		for (out_index = 0; out_index < plane_count; out_index++) {
			if (plane_ids[out_index] == 0 &&
			    is_format_supported(plane->count_formats, plane->formats,
						formats[out_index]))
				break;
		}

		if ((plane->possible_crtcs & crtc_mask) == 0 || out_index == plane_count) {
			drmModeFreePlane(plane);
			continue;
		}

		drmModeObjectPropertiesPtr props =
		    drmModeObjectGetProperties(fd, plane->plane_id, DRM_MODE_OBJECT_PLANE);
		for (uint32_t prop_index = 0; prop_index < props->count_props; ++prop_index) {
			drmModePropertyPtr prop = drmModeGetProperty(fd, props->props[prop_index]);
			if (strcmp(prop->name, "type")) {
				drmModeFreeProperty(prop);
				continue;
			}

			uint64_t desired_value = 0;
			bool has_value = false;
			for (int enum_index = 0; enum_index < prop->count_enums; enum_index++) {
				struct drm_mode_property_enum *penum = &prop->enums[enum_index];
				if (!strcmp(penum->name, "Overlay")) {
					desired_value = penum->value;
					has_value = true;
					break;
				}
			}
			drmModeFreeProperty(prop);

			if (has_value && desired_value == props->prop_values[prop_index]) {
				plane_ids[out_index] = plane->plane_id;
				remaining_out--;
			}

			break;
		}

		drmModeFreeObjectProperties(props);
		drmModeFreePlane(plane);
	}

	drmModeFreePlaneResources(plane_res);
	drmModeFreeResources(res);

	return remaining_out == 0;
}

struct test_plane {
	const struct bs_draw_format *format;

	bool has_bo_size;
	uint32_t bo_w;
	uint32_t bo_h;

	bool has_dst_position;
	int32_t dst_x;
	int32_t dst_y;

	int32_t dst_center_x;
	int32_t dst_center_y;

	int32_t dst_vx;
	int32_t dst_vy;

	bool has_dst_size;
	uint32_t dst_w;
	uint32_t dst_h;

	uint32_t dst_min_w;
	uint32_t dst_max_w;
	uint32_t dst_min_h;
	uint32_t dst_max_h;

	bool has_dst_scale;
	uint32_t dst_downscale_factor;
	uint32_t dst_upscale_factor;

	int32_t src_x;
	int32_t src_y;

	int32_t src_vx;
	int32_t src_vy;

	bool has_src_size;
	uint32_t src_w;
	uint32_t src_h;

	struct gbm_bo *bo;
	uint32_t fb_id;
};

static bool update_test_plane(int step, int32_t max_x, int32_t max_y, struct test_plane *tp)
{
	bool needs_set = (step == 0);
	if (tp->has_dst_scale) {
		float scale_factor = (sinf(step / 120.0f) / 2.0f + 0.5f);
		scale_factor = powf(scale_factor, 4);
		tp->dst_w = tp->dst_min_w + scale_factor * (tp->dst_max_w - tp->dst_min_w);
		tp->dst_h = tp->dst_min_h + scale_factor * (tp->dst_max_h - tp->dst_min_h);
		needs_set = true;
	}

	if (tp->dst_vx != 0) {
		tp->dst_center_x += tp->dst_vx;
		if (tp->dst_center_x > max_x || tp->dst_center_x < 0) {
			tp->dst_vx = -tp->dst_vx;
			tp->dst_x += tp->dst_vx * 2;
		}
		needs_set = true;
	}
	tp->dst_x = tp->dst_center_x - tp->dst_w / 2;

	if (tp->dst_vy != 0) {
		tp->dst_center_y += tp->dst_vy;
		if (tp->dst_center_y > max_y || tp->dst_center_y < 0) {
			tp->dst_vy = -tp->dst_vy;
			tp->dst_y += tp->dst_vy * 2;
		}
		needs_set = true;
	}
	tp->dst_y = tp->dst_center_y - tp->dst_h / 2;

	if (tp->src_vx != 0) {
		tp->src_x += tp->src_vx;
		if (tp->src_x + tp->src_w > gbm_bo_get_width(tp->bo) || tp->src_x < 0) {
			tp->src_vx = -tp->src_vx;
			tp->src_x += tp->src_vx * 2;
		}
		needs_set = true;
	}

	if (tp->src_vy != 0) {
		tp->src_y += tp->src_vy;
		if (tp->src_y + tp->src_h > gbm_bo_get_height(tp->bo) || tp->src_y < 0) {
			tp->src_vy = -tp->src_vy;
			tp->src_y += tp->src_vy * 2;
		}
		needs_set = true;
	}
	return needs_set;
}

static bool parse_size(const char *str, uint32_t *w, uint32_t *h)
{
	if (sscanf(str, "%ux%u", w, h) == 2)
		return true;

	printf("unrecognized size format \"%s\"\n", str);

	return false;
}

static bool parse_scale(const char *str, uint32_t *down, uint32_t *up)
{
	if (sscanf(str, "%u/%u", down, up) == 2)
		return true;

	printf("unrecognized scale format \"%s\"\n", str);

	return false;
}

static bool parse_rect(const char *str, int32_t *x, int32_t *y, uint32_t *w, uint32_t *h,
		       bool *has_position, bool *has_size)
{
	if (sscanf(str, "%d,%d,%u,%u", x, y, w, h) == 4) {
		if (has_position)
			*has_position = true;
		if (has_size)
			*has_size = true;
		return true;
	}

	if (sscanf(str, "%d,%d", x, y) == 2) {
		if (has_position)
			*has_position = true;
		if (has_size)
			*has_size = false;
		return true;
	}

	if (sscanf(str, "%ux%u", w, h) == 2) {
		if (has_position)
			*has_position = false;
		if (has_size)
			*has_size = true;
		return true;
	}

	printf("unrecognized rectangle format \"%s\"\n", str);

	return false;
}

static const struct option longopts[] = {
	{ "plane", no_argument, NULL, 'p' },
	{ "format", required_argument, NULL, 'f' },
	{ "size", required_argument, NULL, 'z' },
	{ "scale", required_argument, NULL, 'c' },
	{ "translate", no_argument, NULL, 't' },
	{ "src", required_argument, NULL, 's' },
	{ "dst", required_argument, NULL, 'd' },
	{ "help", no_argument, NULL, 'h' },
	{ 0, 0, 0, 0 },
};

static void print_help(const char *argv0)
{
	// clang-format off
	printf("usage: %s [OPTIONS]\n", argv0);
	printf("  -p, --plane               indicates that subsequent parameters are for a new plane\n");
	printf("  -f, --format FOURCC       format of source buffer (defaults to NV12)\n");
	printf("  -z, --size WIDTHxHEIGHT   size of the source buffer (defaults to screen size)\n");
	printf("  -c, --scale DOWN/UP       scale plane over time between (1/DOWN)x and UPx\n");
	printf("  -t, --translate           translate plane over time\n");
	printf("  -s, --src RECT            source rectangle (defaults to full buffer)\n");
	printf("  -d, --dst RECT            destination rectangle (defaults to buffer size centered in screen)\n");
	printf("  -h, --help                show help\n");
	printf("\n");
	printf("The format of RECT arguments is X,Y or X,Y,WIDTH,HEIGHT or WIDTHxHEIGHT.\n");
	printf("To test more than one plane, separate plane arguments with -p. For example:\n");
	printf("  %s --format NV12 --size 400x400 -p --format XR24 --size 100x100 --translate\n", argv0);
	printf("\n");
	// clang-format on
}

int main(int argc, char **argv)
{
	int ret = 0;
	bool help_flag = false;
	size_t test_planes_count = 1;
	struct test_plane test_planes[MAX_TEST_PLANES] = { { 0 } };
	uint32_t test_planes_formats[MAX_TEST_PLANES] = { 0 };
	uint32_t test_planes_ids[MAX_TEST_PLANES] = { 0 };

	int c;
	while ((c = getopt_long(argc, argv, "pf:z:c:ts:d:h", longopts, NULL)) != -1) {
		struct test_plane *current_plane = &test_planes[test_planes_count - 1];
		switch (c) {
			case 'p':
				test_planes_count++;
				if (test_planes_count > MAX_TEST_PLANES) {
					printf("only %d planes are allowed\n", MAX_TEST_PLANES);
					return 1;
				}
				break;
			case 'f':
				if (!bs_parse_draw_format(optarg, &current_plane->format))
					return 1;
				break;
			case 'z':
				if (!parse_size(optarg, &current_plane->bo_w, &current_plane->bo_h))
					return 1;
				current_plane->has_bo_size = true;
				break;
			case 'c':
				if (!parse_scale(optarg, &current_plane->dst_downscale_factor,
						 &current_plane->dst_upscale_factor))
					return 1;
				current_plane->has_dst_scale = true;
				break;
			case 't':
				current_plane->dst_vx = 7;
				current_plane->dst_vy = 7;
				break;
			case 's':
				if (!parse_rect(optarg, &current_plane->src_x,
						&current_plane->src_y, &current_plane->src_w,
						&current_plane->src_h, NULL,
						&current_plane->has_src_size))
					return 1;
				break;
			case 'd':
				if (!parse_rect(optarg, &current_plane->dst_x,
						&current_plane->dst_y, &current_plane->dst_w,
						&current_plane->dst_h,
						&current_plane->has_dst_position,
						&current_plane->has_dst_size))
					return 1;
				break;
			case '?':
				ret = 1;
			case 'h':
				help_flag = true;
				break;
		}
	}

	if (help_flag)
		print_help(argv[0]);

	if (ret)
		return ret;

	drmModeConnector *connector;
	struct bs_drm_pipe pipe = { 0 };
	struct bs_drm_pipe_plumber *plumber = bs_drm_pipe_plumber_new();
	bs_drm_pipe_plumber_connector_ranks(plumber, bs_drm_connectors_internal_rank);
	bs_drm_pipe_plumber_connector_ptr(plumber, &connector);
	if (!bs_drm_pipe_plumber_make(plumber, &pipe)) {
		bs_debug_error("failed to make pipe");
		return 1;
	}
	bs_drm_pipe_plumber_destroy(&plumber);

	drmModeModeInfo *mode_ptr = find_best_mode(connector->count_modes, connector->modes);
	if (!mode_ptr) {
		bs_debug_error("failed to find preferred mode");
		return 1;
	}
	drmModeModeInfo mode = *mode_ptr;
	drmModeFreeConnector(connector);
	printf("Using mode %s\n", mode.name);

	printf("Using CRTC:%u ENCODER:%u CONNECTOR:%u\n", pipe.crtc_id, pipe.encoder_id,
	       pipe.connector_id);

	struct gbm_device *gbm = gbm_create_device(pipe.fd);
	if (!gbm) {
		bs_debug_error("failed to create gbm");
		return 1;
	}

	struct gbm_bo *bg_bo = gbm_bo_create(gbm, mode.hdisplay, mode.vdisplay, GBM_FORMAT_XRGB8888,
					     GBM_BO_USE_SCANOUT | GBM_BO_USE_SW_WRITE_RARELY);
	if (!bg_bo) {
		bs_debug_error("failed to create background buffer ojbect");
		return 1;
	}

	struct bs_mapper *mapper = bs_mapper_gem_new();
	if (mapper == NULL) {
		bs_debug_error("failed to create mapper object");
		return 1;
	}

	void *map_data;
	void *bo_ptr = bs_mapper_map(mapper, bg_bo, 0, &map_data);
	if (bo_ptr == MAP_FAILED) {
		bs_debug_error("failed to mmap background buffer object");
		return 1;
	}
	memset(bo_ptr, 0, gbm_bo_get_height(bg_bo) * gbm_bo_get_stride(bg_bo));
	bs_mapper_unmap(mapper, bg_bo, map_data);

	uint32_t crtc_fb_id = bs_drm_fb_create_gbm(bg_bo);
	if (!crtc_fb_id) {
		bs_debug_error("failed to create frame buffer for buffer object");
		return 1;
	}

	for (size_t test_plane_index = 0; test_plane_index < test_planes_count;
	     test_plane_index++) {
		struct test_plane *tp = &test_planes[test_plane_index];

		if (!tp->format)
			tp->format = bs_get_draw_format(GBM_FORMAT_NV12);

		test_planes_formats[test_plane_index] = bs_get_pixel_format(tp->format);

		if (!tp->has_bo_size) {
			tp->bo_w = mode.hdisplay;
			tp->bo_h = mode.vdisplay;
		}

		if (!tp->has_src_size) {
			tp->src_w = tp->bo_w;
			tp->src_h = tp->bo_h;
		}

		if (!tp->has_dst_size) {
			tp->dst_w = tp->bo_w;
			tp->dst_h = tp->bo_h;
		}

		if (tp->has_dst_position) {
			tp->dst_center_x = tp->dst_x + mode.hdisplay / 2;
			tp->dst_center_y = tp->dst_y + mode.vdisplay / 2;
		} else {
			tp->dst_center_x = mode.hdisplay / 2;
			tp->dst_center_y = mode.vdisplay / 2;
			tp->dst_x = tp->dst_center_x - tp->dst_w / 2;
			tp->dst_y = tp->dst_center_y - tp->dst_h / 2;
		}

		if (tp->has_dst_scale) {
			tp->dst_min_w = tp->dst_w / tp->dst_downscale_factor;
			tp->dst_max_w = tp->dst_w * tp->dst_upscale_factor;
			tp->dst_min_h = tp->dst_h / tp->dst_downscale_factor;
			tp->dst_max_h = tp->dst_h * tp->dst_upscale_factor;
		}

		printf("Creating buffer %ux%u %s\n", tp->bo_w, tp->bo_h,
		       bs_get_format_name(tp->format));
		tp->bo = gbm_bo_create(gbm, tp->bo_w, tp->bo_h, bs_get_pixel_format(tp->format),
				       GBM_BO_USE_SCANOUT | GBM_BO_USE_SW_WRITE_RARELY);
		if (!tp->bo) {
			bs_debug_error("failed to create buffer object");
			return 1;
		}

		tp->fb_id = bs_drm_fb_create_gbm(tp->bo);
		if (!tp->fb_id) {
			bs_debug_error("failed to create plane frame buffer for buffer object");
			return 1;
		}

		if (!bs_draw_stripe(mapper, tp->bo, tp->format)) {
			bs_debug_error("failed to draw pattern to buffer object");
			return 1;
		}
	}

	if (!find_overlay_planes(pipe.fd, pipe.crtc_id, test_planes_count, test_planes_formats,
				 test_planes_ids)) {
		bs_debug_error("failed to find overlay planes for given formats");
		return 1;
	}

	ret = drmModeSetCrtc(pipe.fd, pipe.crtc_id, crtc_fb_id, 0, 0, &pipe.connector_id, 1, &mode);
	if (ret < 0) {
		bs_debug_error("Could not set mode on CRTC %d %s", pipe.crtc_id, strerror(errno));
		return 1;
	}

	struct timespec start;
	clock_gettime(CLOCK_MONOTONIC, &start);
	const int64_t ten_seconds_in_ns = 10000000000;
	for (int i = 0; ns_since(&start) < ten_seconds_in_ns; i++) {
		for (size_t test_plane_index = 0; test_plane_index < test_planes_count;
		     test_plane_index++) {
			struct test_plane *current_plane = &test_planes[test_plane_index];

			bool needs_set =
			    update_test_plane(i, mode.hdisplay, mode.vdisplay, current_plane);
			if (!needs_set)
				continue;

			ret = drmModeSetPlane(
			    pipe.fd, test_planes_ids[test_plane_index], pipe.crtc_id,
			    current_plane->fb_id, 0 /* flags */, current_plane->dst_x,
			    current_plane->dst_y, current_plane->dst_w, current_plane->dst_h,
			    current_plane->src_x << 16, current_plane->src_y << 16,
			    current_plane->src_w << 16, current_plane->src_h << 16);

			if (ret) {
				bs_debug_error(
				    "failed to set plane "
				    "%d:\ndst[x,y,w,h]=%d,%d,%u,%u\nsrc[x,y,w,h]=%d,%d,%u,%u",
				    ret, current_plane->dst_x, current_plane->dst_y,
				    current_plane->dst_w, current_plane->dst_h,
				    current_plane->src_x, current_plane->src_y,
				    current_plane->src_w, current_plane->src_h);
				return 1;
			}
		}
		usleep(1000000 / 60);
	}

	ret = drmModeSetCrtc(pipe.fd, pipe.crtc_id, 0, 0, 0, NULL, 0, NULL);
	if (ret < 0) {
		bs_debug_error("Could not disable CRTC %d %s", pipe.crtc_id, strerror(errno));
		return 1;
	}

	bs_mapper_destroy(mapper);
	for (size_t test_plane_index = 0; test_plane_index < test_planes_count;
	     test_plane_index++) {
		struct test_plane *current_plane = &test_planes[test_plane_index];
		drmModeRmFB(pipe.fd, current_plane->fb_id);
		gbm_bo_destroy(current_plane->bo);
	}
	drmModeRmFB(pipe.fd, crtc_fb_id);
	gbm_bo_destroy(bg_bo);
	gbm_device_destroy(gbm);
	close(pipe.fd);

	return 0;
}
