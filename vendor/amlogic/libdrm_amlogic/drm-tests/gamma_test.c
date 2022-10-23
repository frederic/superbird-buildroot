/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "bs_drm.h"

#include <getopt.h>
#include <math.h>

#define TABLE_LINEAR 0
#define TABLE_NEGATIVE 1
#define TABLE_POW 2
#define TABLE_STEP 3

#define FLAG_INTERNAL 'i'
#define FLAG_EXTERNAL 'e'
#define FLAG_GAMMA 'g'
#define FLAG_LINEAR 'l'
#define FLAG_NEGATIVE 'n'
#define FLAG_TIME 't'
#define FLAG_CRTCS 'c'
#define FLAG_PERSIST 'p'
#define FLAG_STEP 's'
#define FLAG_HELP 'h'

static struct option command_options[] = { { "internal", no_argument, NULL, FLAG_INTERNAL },
					   { "external", no_argument, NULL, FLAG_EXTERNAL },
					   { "gamma", required_argument, NULL, FLAG_GAMMA },
					   { "linear", no_argument, NULL, FLAG_LINEAR },
					   { "negative", no_argument, NULL, FLAG_NEGATIVE },
					   { "time", required_argument, NULL, FLAG_TIME },
					   { "crtcs", required_argument, NULL, FLAG_CRTCS },
					   { "persist", no_argument, NULL, FLAG_PERSIST },
					   { "step", no_argument, NULL, FLAG_STEP },
					   { "help", no_argument, NULL, FLAG_HELP },
					   { NULL, 0, NULL, 0 } };

static void gamma_linear(uint16_t *table, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		float v = (float)(i) / (float)(size - 1);
		v *= 65535.0f;
		table[i] = (uint16_t)v;
	}
}

static void gamma_inv(uint16_t *table, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		float v = (float)(size - 1 - i) / (float)(size - 1);
		v *= 65535.0f;
		table[i] = (uint16_t)v;
	}
}

static void gamma_pow(uint16_t *table, int size, float p)
{
	int i;
	for (i = 0; i < size; i++) {
		float v = (float)(i) / (float)(size - 1);
		v = pow(v, p);
		v *= 65535.0f;
		table[i] = (uint16_t)v;
	}
}

static void gamma_step(uint16_t *table, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		table[i] = (i < size / 2) ? 0 : 65535;
	}
}

static void fsleep(double secs)
{
	usleep((useconds_t)(1000000.0f * secs));
}

static drmModeModeInfoPtr find_best_mode(int mode_count, drmModeModeInfoPtr modes)
{
	assert(mode_count >= 0);
	if (mode_count == 0)
		return NULL;

	assert(modes);

	for (int m = 0; m < mode_count; m++)
		if (modes[m].type & DRM_MODE_TYPE_PREFERRED)
			return &modes[m];

	return &modes[0];
}

static bool draw_pattern(struct bs_mapper *mapper, struct gbm_bo *bo)
{
	const uint32_t stride = gbm_bo_get_stride(bo);
	const uint32_t height = gbm_bo_get_height(bo);
	const uint32_t bo_size = stride * height;
	const uint32_t stripw = gbm_bo_get_width(bo) / 256;
	const uint32_t striph = height / 4;

	void *map_data;
	uint8_t *bo_ptr = bs_mapper_map(mapper, bo, 0, &map_data);
	if (bo_ptr == MAP_FAILED) {
		bs_debug_error("failed to mmap buffer while drawing pattern");
		return false;
	}

	bool success = true;

	memset(bo_ptr, 0, bo_size);
	for (uint32_t s = 0; s < 4; s++) {
		uint8_t r = 0, g = 0, b = 0;
		switch (s) {
			case 0:
				r = g = b = 1;
				break;
			case 1:
				r = 1;
				break;
			case 2:
				g = 1;
				break;
			case 3:
				b = 1;
				break;
			default:
				assert("invalid strip" && false);
				success = false;
				goto out;
		}
		for (uint32_t y = s * striph; y < (s + 1) * striph; y++) {
			uint8_t *row_ptr = &bo_ptr[y * stride];
			for (uint32_t i = 0; i < 256; i++) {
				for (uint32_t x = i * stripw; x < (i + 1) * stripw; x++) {
					row_ptr[x * 4 + 0] = b * i;
					row_ptr[x * 4 + 1] = g * i;
					row_ptr[x * 4 + 2] = r * i;
					row_ptr[x * 4 + 3] = 0;
				}
			}
		}
	}

out:
	bs_mapper_unmap(mapper, bo, map_data);
	return success;
}

static int set_gamma(int fd, uint32_t crtc_id, int gamma_size, int gamma_table, float gamma)
{
	int res;
	uint16_t *r, *g, *b;
	r = calloc(gamma_size, sizeof(*r));
	g = calloc(gamma_size, sizeof(*g));
	b = calloc(gamma_size, sizeof(*b));

	printf("Setting gamma table %d\n", gamma_table);
	switch (gamma_table) {
		case TABLE_LINEAR:
			gamma_linear(r, gamma_size);
			gamma_linear(g, gamma_size);
			gamma_linear(b, gamma_size);
			break;
		case TABLE_NEGATIVE:
			gamma_inv(r, gamma_size);
			gamma_inv(g, gamma_size);
			gamma_inv(b, gamma_size);
			break;
		case TABLE_POW:
			gamma_pow(r, gamma_size, gamma);
			gamma_pow(g, gamma_size, gamma);
			gamma_pow(b, gamma_size, gamma);
			break;
		case TABLE_STEP:
			gamma_step(r, gamma_size);
			gamma_step(g, gamma_size);
			gamma_step(b, gamma_size);
			break;
	}

	res = drmModeCrtcSetGamma(fd, crtc_id, gamma_size, r, g, b);
	if (res)
		bs_debug_error("drmModeCrtcSetGamma(%d) failed: %s", crtc_id, strerror(errno));
	free(r);
	free(g);
	free(b);
	return res;
}

void help(void)
{
	printf(
	    "\
gamma test\n\
command line options:\
\n\
--help - this\n\
--linear - set linear gamma table\n\
--negative - set negative linear gamma table\n\
--step - set step gamma table\n\
--gamma=f - set pow(gamma) gamma table with gamma=f\n\
--time=f - set test time\n\
--crtcs=n - set mask of crtcs to test\n\
--persist - do not reset gamma table at the end of the test\n\
--internal - display tests on internal display\n\
--external - display tests on external display\n\
");
}

int main(int argc, char **argv)
{
	int internal = 1;
	int persist = 0;
	float time = 5.0;
	float gamma = 2.2f;
	float table = TABLE_LINEAR;
	uint32_t crtcs = 0xFFFF;

	for (;;) {
		int c = getopt_long(argc, argv, "", command_options, NULL);

		if (c == -1)
			break;

		switch (c) {
			case FLAG_HELP:
				help();
				return 0;

			case FLAG_INTERNAL:
				internal = 1;
				break;

			case FLAG_EXTERNAL:
				internal = 0;
				break;

			case FLAG_GAMMA:
				gamma = strtof(optarg, NULL);
				table = TABLE_POW;
				break;

			case FLAG_LINEAR:
				table = TABLE_LINEAR;
				break;

			case FLAG_NEGATIVE:
				table = TABLE_NEGATIVE;
				break;

			case FLAG_STEP:
				table = TABLE_STEP;
				break;

			case FLAG_TIME:
				time = strtof(optarg, NULL);
				break;

			case FLAG_CRTCS:
				crtcs = strtoul(optarg, NULL, 0);
				break;

			case FLAG_PERSIST:
				persist = 1;
				break;
		}
	}

	drmModeConnector *connector = NULL;
	struct bs_drm_pipe pipe = { 0 };
	struct bs_drm_pipe_plumber *plumber = bs_drm_pipe_plumber_new();
	bs_drm_pipe_plumber_connector_ptr(plumber, &connector);
	bs_drm_pipe_plumber_crtc_mask(plumber, crtcs);
	if (!internal)
		bs_drm_pipe_plumber_connector_ranks(plumber, bs_drm_connectors_external_rank);
	if (!bs_drm_pipe_plumber_make(plumber, &pipe)) {
		bs_debug_error("failed to make pipe");
		return 1;
	}

	int fd = pipe.fd;
	bs_drm_pipe_plumber_fd(plumber, fd);
	drmModeRes *resources = drmModeGetResources(fd);
	if (!resources) {
		bs_debug_error("failed to get drm resources");
		return 1;
	}

	struct gbm_device *gbm = gbm_create_device(fd);
	if (!gbm) {
		bs_debug_error("failed to create gbm");
		return 1;
	}

	struct bs_mapper *mapper = bs_mapper_gem_new();
	if (mapper == NULL) {
		bs_debug_error("failed to create mapper object");
		return 1;
	}

	uint32_t num_success = 0;
	for (int c = 0; c < resources->count_crtcs && (crtcs >> c); c++) {
		int ret;
		drmModeCrtc *crtc;
		uint32_t crtc_mask = 1u << c;

		if (!(crtcs & crtc_mask))
			continue;

		if (connector != NULL) {
			drmModeFreeConnector(connector);
			connector = NULL;
		}

		bs_drm_pipe_plumber_crtc_mask(plumber, crtc_mask);
		if (!bs_drm_pipe_plumber_make(plumber, &pipe)) {
			printf("unable to make pipe with crtc mask: %x\n", crtc_mask);
			continue;
		}

		crtc = drmModeGetCrtc(fd, pipe.crtc_id);
		if (!crtc) {
			bs_debug_error("drmModeGetCrtc(%d) failed: %s\n", pipe.crtc_id,
				       strerror(errno));
			return 1;
		}
		int gamma_size = crtc->gamma_size;
		drmModeFreeCrtc(crtc);

		if (!gamma_size) {
			bs_debug_error("CRTC %d has no gamma table", crtc->crtc_id);
			continue;
		}

		printf("CRTC:%d gamma size:%d\n", pipe.crtc_id, gamma_size);

		printf("Using CRTC:%u ENCODER:%u CONNECTOR:%u\n", pipe.crtc_id, pipe.encoder_id,
		       pipe.connector_id);

		drmModeModeInfoPtr mode = find_best_mode(connector->count_modes, connector->modes);
		if (!mode) {
			bs_debug_error("Could not find mode for CRTC %d", pipe.crtc_id);
			continue;
		}

		printf("Using mode %s\n", mode->name);

		printf("Creating buffer %ux%u\n", mode->hdisplay, mode->vdisplay);
		struct gbm_bo *bo =
		    gbm_bo_create(gbm, mode->hdisplay, mode->vdisplay, GBM_FORMAT_XRGB8888,
				  GBM_BO_USE_SCANOUT | GBM_BO_USE_SW_WRITE_RARELY);
		if (!bo) {
			bs_debug_error("failed to create buffer object");
			return 1;
		}

		uint32_t fb_id = bs_drm_fb_create_gbm(bo);
		if (!fb_id) {
			bs_debug_error("failed to create frame buffer for buffer object");
			return 1;
		}

		if (!draw_pattern(mapper, bo)) {
			bs_debug_error("failed to draw pattern on buffer object");
			return 1;
		}

		ret = drmModeSetCrtc(fd, pipe.crtc_id, fb_id, 0, 0, &pipe.connector_id, 1, mode);
		if (ret < 0) {
			bs_debug_error("Could not set mode on CRTC %d %s", pipe.crtc_id,
				       strerror(errno));
			return 1;
		}

		ret = set_gamma(fd, pipe.crtc_id, gamma_size, table, gamma);
		if (ret)
			return ret;

		fsleep(time);

		if (!persist) {
			ret = set_gamma(fd, pipe.crtc_id, gamma_size, TABLE_LINEAR, 0.0f);
			if (ret)
				return ret;
		}

		ret = drmModeSetCrtc(fd, pipe.crtc_id, 0, 0, 0, NULL, 0, NULL);
		if (ret < 0) {
			bs_debug_error("Could disable CRTC %d %s\n", pipe.crtc_id, strerror(errno));
			return 1;
		}

		drmModeRmFB(fd, fb_id);
		gbm_bo_destroy(bo);
		num_success++;
	}
	bs_mapper_destroy(mapper);

	if (connector != NULL) {
		drmModeFreeConnector(connector);
		connector = NULL;
	}

	drmModeFreeResources(resources);
	bs_drm_pipe_plumber_destroy(&plumber);

	if (!num_success) {
		bs_debug_error("unable to set gamma table on any CRTC");
		return 1;
	}

	return 0;
}
