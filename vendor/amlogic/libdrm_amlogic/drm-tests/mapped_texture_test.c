/*
 * Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
  The mapped_texture_test test consists of:
    * Importing external buffers as EGL Images
    * Drawing an ellipse using the CPU
    * Binding CPU drawn buffer as a texture and sampling from it
    * Using KMS to scanout the resultant framebuffer
 */

#include <getopt.h>

#include "bs_drm.h"

// double buffering
#define NUM_BUFFERS 2

struct offscreen_buffer {
	struct gbm_bo *bo;
	GLuint tex;
	EGLImageKHR image;
	const struct bs_draw_format *draw_format;
};

struct framebuffer {
	struct gbm_bo *bo;
	uint32_t fb_id;
	EGLImageKHR image;
	struct bs_egl_fb *egl_fb;
};

struct gl_resources {
	GLuint program;
	GLuint vbo;
};

// clang-format off
static const GLfloat vertices[] = {
	// x       y     u     v
	-0.25f, -0.25f, 0.0f, 0.0f, // Bottom left
	-0.25f,  0.25f, 0.0f, 1.0f, // Top left
	 0.25f,  0.25f, 1.0f, 1.0f, // Top right
	 0.25f, -0.25f, 1.0f, 0.0f, // Bottom Right
};

static const int binding_xy = 0;
static const int binding_uv = 1;

static const GLubyte indices[] = {
	0, 1, 2,
	0, 2, 3
};

// clang-format on

static const GLchar *vert =
    "attribute vec2 xy;\n"
    "attribute vec2 uv;\n"
    "varying vec2 tex_coordinate;\n"
    "void main() {\n"
    "    gl_Position = vec4(xy, 0, 1);\n"
    "    tex_coordinate = uv;\n"
    "}\n";

static const GLchar *frag =
    "precision mediump float;\n"
    "uniform sampler2D ellipse;\n"
    "varying vec2 tex_coordinate;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(ellipse, tex_coordinate);\n"
    "}\n";

static bool create_framebuffer(int display_fd, struct gbm_device *gbm, struct bs_egl *egl,
			       uint32_t width, uint32_t height, struct framebuffer *fb)
{
	fb->bo = gbm_bo_create(gbm, width, height, GBM_FORMAT_XRGB8888,
			       GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	if (!fb->bo) {
		bs_debug_error("failed to create a gbm buffer.");
		goto delete_gl_fb;
	}

	fb->fb_id = bs_drm_fb_create_gbm(fb->bo);
	if (!fb->fb_id) {
		bs_debug_error("failed to create framebuffer from buffer object");
		goto delete_gl_image;
	}

	fb->image = bs_egl_image_create_gbm(egl, fb->bo);
	if (fb->image == EGL_NO_IMAGE_KHR) {
		bs_debug_error("failed to make image from buffer object");
		goto delete_fb;
	}

	fb->egl_fb = bs_egl_fb_new(egl, fb->image);
	if (!fb->egl_fb) {
		bs_debug_error("failed to make rednering framebuffer for buffer object");
		goto delete_gbm_bo;
	}

	return true;

delete_gl_fb:
	bs_egl_fb_destroy(&fb->egl_fb);
delete_gl_image:
	bs_egl_image_destroy(egl, &fb->image);
delete_fb:
	drmModeRmFB(display_fd, fb->fb_id);
delete_gbm_bo:
	gbm_bo_destroy(fb->bo);
	return false;
}

static bool add_offscreen_texture(struct gbm_device *gbm, struct bs_egl *egl,
				  struct offscreen_buffer *buffer, uint32_t width, uint32_t height,
				  uint32_t flags)
{
	buffer->bo =
	    gbm_bo_create(gbm, width, height, bs_get_pixel_format(buffer->draw_format), flags);
	if (!buffer->bo) {
		bs_debug_error("failed to allocate offscreen buffer object: format=%s \n",
			       bs_get_format_name(buffer->draw_format));
		goto destroy_offscreen_buffer;
	}

	buffer->image = bs_egl_image_create_gbm(egl, buffer->bo);
	if (buffer->image == EGL_NO_IMAGE_KHR) {
		bs_debug_error("failed to create offscreen egl image");
		goto destroy_offscreen_bo;
	}

	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &buffer->tex);
	glBindTexture(GL_TEXTURE_2D, buffer->tex);

	if (!bs_egl_target_texture2D(egl, buffer->image)) {
		bs_debug_error("failed to import egl image as texture");
		goto destroy_offscreen_image;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return true;

destroy_offscreen_image:
	glDeleteTextures(1, &buffer->tex);
	bs_egl_image_destroy(egl, &buffer->image);
destroy_offscreen_bo:
	gbm_bo_destroy(buffer->bo);
destroy_offscreen_buffer:
	return false;
}

static bool init_gl(struct bs_egl_fb *fb, uint32_t width, uint32_t height,
		    struct gl_resources *resources)
{
	struct bs_gl_program_create_binding bindings[] = {
		{ binding_xy, "xy" }, { binding_uv, "uv" }, { 2, NULL },
	};

	resources->program = bs_gl_program_create_vert_frag_bind(vert, frag, bindings);
	if (!resources->program) {
		bs_debug_error("failed to compile shader program");
		return false;
	}

	glGenBuffers(1, &resources->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, resources->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindFramebuffer(GL_FRAMEBUFFER, bs_egl_fb_name(fb));
	glViewport(0, 0, (GLint)width, (GLint)height);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(resources->program);

	glUniform1i(glGetUniformLocation(resources->program, "ellipse"), 1);
	glEnableVertexAttribArray(binding_xy);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

	glEnableVertexAttribArray(binding_uv);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
			      (void *)(2 * sizeof(GLfloat)));
	return true;
}

static void draw_textured_quad(GLuint tex)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tex);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
}

static const struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "format", required_argument, NULL, 'f' },
	{ "dma-buf", no_argument, NULL, 'b' },
	{ "gem", no_argument, NULL, 'g' },
	{ "dumb", no_argument, NULL, 'd' },
	{ "tiled", no_argument, NULL, 't' },
	{ 0, 0, 0, 0 },
};

static void print_help(const char *argv0)
{
	printf("Usage: %s [OPTIONS]\n", argv0);
	printf(" -h, --help             Print help.\n");
	printf(" -f, --format FOURCC    format of texture (defaults to ARGB8888)\n");
	printf(" -b, --dma-buf  Use dma-buf mmap.\n");
	printf(" -g, --gem      Use GEM map(by default).\n");
	printf(" -d, --dumb     Use dump map.\n");
}

static void page_flip_handler(int fd, unsigned int frame, unsigned int sec, unsigned int usec,
			      void *data)
{
	int *waiting_for_flip = data;
	*waiting_for_flip = 0;
}

void flush_egl(struct bs_egl *egl, EGLImageKHR image)
{
	bs_egl_image_flush_external(egl, image);
	EGLSyncKHR sync = bs_egl_create_sync(egl, EGL_SYNC_FENCE_KHR, NULL);
	bs_egl_wait_sync(egl, sync, 0, EGL_FOREVER_KHR);
	bs_egl_destroy_sync(egl, sync);
}

int main(int argc, char **argv)
{
	int ret = 1;
	int display_fd = bs_drm_open_main_display();
	if (display_fd < 0) {
		bs_debug_error("failed to open card for display");
		goto out;
	}

	struct offscreen_buffer buffer;
	buffer.draw_format = bs_get_draw_format_from_name("ARGB8888");
	struct bs_mapper *mapper = NULL;
	uint32_t flags = GBM_BO_USE_TEXTURING;

	int c;
	while ((c = getopt_long(argc, argv, "f:bgdh", longopts, NULL)) != -1) {
		switch (c) {
			case 'f':
				if (!bs_parse_draw_format(optarg, &buffer.draw_format)) {
					printf("choose the default format ARGB8888\n");
				}
				printf("format=%s\n", bs_get_format_name(buffer.draw_format));
				break;
			case 'b':
				mapper = bs_mapper_dma_buf_new();
				flags |= GBM_BO_USE_LINEAR;
				printf("using dma-buf mmap\n");
				break;
			case 'g':
				mapper = bs_mapper_gem_new();
				flags |= GBM_BO_USE_SW_WRITE_OFTEN;
				printf("using GEM map\n");
				break;
			case 'd':
				mapper = bs_mapper_dumb_new(display_fd);
				flags |= GBM_BO_USE_LINEAR;
				printf("using dumb map\n");
				break;
			case 'h':
			default:
				print_help(argv[0]);
				goto destroy_display_fd;
		}
	}

	// Use gem map mapper by default, in case any arguments aren't selected.
	if (!mapper) {
		mapper = bs_mapper_gem_new();
		flags |= GBM_BO_USE_SW_WRITE_OFTEN;
		printf("using GEM map\n");
	}

	if (!mapper) {
		bs_debug_error("failed to create mapper object");
		goto destroy_display_fd;
	}

	uint32_t width;
	uint32_t height;

	struct gbm_device *gbm = gbm_create_device(display_fd);
	if (!gbm) {
		bs_debug_error("failed to create gbm device");
		goto destroy_mapper;
	}

	struct bs_drm_pipe pipe = { 0 };
	if (!bs_drm_pipe_make(display_fd, &pipe)) {
		bs_debug_error("failed to make pipe");
		goto destroy_gbm_device;
	}

	drmModeConnector *connector = drmModeGetConnector(display_fd, pipe.connector_id);
	drmModeModeInfo *mode = &connector->modes[0];
	width = mode->hdisplay;
	height = mode->vdisplay;

	struct bs_egl *egl = bs_egl_new();
	if (!bs_egl_setup(egl)) {
		bs_debug_error("failed to setup egl context");
		goto destroy_gbm_device;
	}

	struct framebuffer fbs[NUM_BUFFERS] = {};
	uint32_t front_buffer = 0;
	for (size_t i = 0; i < NUM_BUFFERS; i++) {
		if (!create_framebuffer(display_fd, gbm, egl, width, height, &fbs[i])) {
			bs_debug_error("failed to create framebuffer");
			goto delete_framebuffers;
		}
	}

	if (!add_offscreen_texture(gbm, egl, &buffer, width / 4, height / 4, flags)) {
		bs_debug_error("failed to create offscreen texture");
		goto destroy_offscreen_buffer;
	}

	const struct framebuffer *back_fb = &fbs[front_buffer ^ 1];
	struct gl_resources resources;
	if (!init_gl(back_fb->egl_fb, width, height, &resources)) {
		bs_debug_error("failed to initialize GL resources.\n");
		goto destroy_gl_resources;
	}

	flush_egl(egl, back_fb->image);

	ret = drmModeSetCrtc(display_fd, pipe.crtc_id, fbs[front_buffer].fb_id, 0 /* x */,
			     0 /* y */, &pipe.connector_id, 1 /* connector count */, mode);
	if (ret) {
		bs_debug_error("failed to set crtc: %d", ret);
		goto destroy_gl_resources;
	}

	drmEventContext evctx = {
		.version = DRM_EVENT_CONTEXT_VERSION, .page_flip_handler = page_flip_handler,
	};
	fd_set fds;

	// The test takes about 2 seconds to complete.
	const size_t test_frames = 120;
	for (size_t i = 0; i < test_frames; i++) {
		int waiting_for_flip = 1;

		const struct framebuffer *back_fb = &fbs[front_buffer ^ 1];
		glBindFramebuffer(GL_FRAMEBUFFER, bs_egl_fb_name(back_fb->egl_fb));
		if (!bs_draw_ellipse(mapper, buffer.bo, buffer.draw_format,
				     (float)i / test_frames)) {
			bs_debug_error("failed to draw to buffer");
			goto destroy_gl_resources;
		}

		draw_textured_quad(buffer.tex);

		flush_egl(egl, back_fb->image);

		ret = drmModePageFlip(display_fd, pipe.crtc_id, back_fb->fb_id,
				      DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip);
		if (ret) {
			bs_debug_error("failed to queue page flip");
			goto destroy_gl_resources;
		}

		while (waiting_for_flip) {
			FD_ZERO(&fds);
			FD_SET(0, &fds);
			FD_SET(display_fd, &fds);
			int ret = select(display_fd + 1, &fds, NULL, NULL, NULL);
			if (ret < 0) {
				bs_debug_error("select err: %s", strerror(errno));
				goto destroy_gl_resources;
			} else if (FD_ISSET(0, &fds)) {
				bs_debug_error("exit due to user-input");
				goto destroy_gl_resources;
			} else if (FD_ISSET(display_fd, &fds)) {
				drmHandleEvent(display_fd, &evctx);
			}
		}

		front_buffer ^= 1;
	}

destroy_gl_resources:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDeleteProgram(resources.program);
	glDeleteBuffers(1, &resources.vbo);
destroy_offscreen_buffer:
	glDeleteTextures(1, &buffer.tex);
	bs_egl_image_destroy(egl, &buffer.image);
	gbm_bo_destroy(buffer.bo);
delete_framebuffers:
	for (size_t i = 0; i < NUM_BUFFERS; i++) {
		bs_egl_fb_destroy(&fbs[i].egl_fb);
		bs_egl_image_destroy(egl, &fbs[i].image);
		drmModeRmFB(display_fd, fbs[i].fb_id);
		gbm_bo_destroy(fbs[i].bo);
	}
	bs_egl_destroy(&egl);
destroy_gbm_device:
	gbm_device_destroy(gbm);
destroy_mapper:
	bs_mapper_destroy(mapper);
destroy_display_fd:
	close(display_fd);
out:
	return ret;
}
