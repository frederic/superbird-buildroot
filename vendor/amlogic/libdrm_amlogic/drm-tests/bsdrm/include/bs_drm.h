/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __BS_DRM_H__
#define __BS_DRM_H__

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#define bs_rank_skip UINT32_MAX

#define BS_ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

#define BS_ALIGN(a, alignment) ((a + (alignment - 1)) & ~(alignment - 1))

// debug.c
__attribute__((format(printf, 5, 6))) void bs_debug_print(const char *prefix, const char *func,
							  const char *file, int line,
							  const char *format, ...);
#define bs_debug_error(...)                                                         \
	do {                                                                        \
		bs_debug_print("ERROR", __func__, __FILE__, __LINE__, __VA_ARGS__); \
	} while (0)

#define bs_debug_warning(...)                                                      \
	do {                                                                       \
		bs_debug_print("WARN", __func__, __FILE__, __LINE__, __VA_ARGS__); \
	} while (0)

int64_t bs_debug_gettime_ns();

// pipe.c
typedef bool (*bs_make_pipe_piece)(void *context, void *out);

bool bs_pipe_make(void *context, bs_make_pipe_piece *pieces, size_t piece_count, void *out_pipe,
		  size_t pipe_size);

// open.c

// A return value of true causes enumeration to end immediately. fd is always
// closed after the callback.
typedef bool (*bs_open_enumerate_func)(void *user, int fd);

// A return value of true causes the filter to return the given fd.
typedef bool (*bs_open_filter_func)(int fd);

// The fd with the lowest (magnitude) rank is returned. A fd with rank UINT32_MAX is skipped. A fd
// with rank 0 ends the enumeration early and is returned. On a tie, the fd returned will be
// arbitrarily chosen from the set of lowest rank fds.
typedef uint32_t (*bs_open_rank_func)(int fd);

void bs_open_enumerate(const char *format, unsigned start, unsigned end,
		       bs_open_enumerate_func body, void *user);
int bs_open_filtered(const char *format, unsigned start, unsigned end, bs_open_filter_func filter);
int bs_open_ranked(const char *format, unsigned start, unsigned end, bs_open_rank_func rank);

// drm_connectors.c
#define bs_drm_connectors_any UINT32_MAX

// Interleaved arrays in the layout { DRM_MODE_CONNECTOR_*, rank, DRM_MODE_CONNECTOR_*, rank, ... },
// terminated by { ... 0, 0 }. bs_drm_connectors_any can be used in place of DRM_MODE_CONNECTOR_* to
// match any connector. bs_rank_skip can be used in place of a rank to indicate that that connector
// should be skipped.

// Use internal connectors and fallback to any connector
extern const uint32_t bs_drm_connectors_main_rank[];
// Use only internal connectors
extern const uint32_t bs_drm_connectors_internal_rank[];
// Use only external connectors
extern const uint32_t bs_drm_connectors_external_rank[];

uint32_t bs_drm_connectors_rank(const uint32_t *ranks, uint32_t connector_type);

// drm_pipe.c
struct bs_drm_pipe {
	int fd;  // Always owned by the user of this library
	uint32_t connector_id;
	uint32_t encoder_id;
	uint32_t crtc_id;
};
struct bs_drm_pipe_plumber;

// A class that makes pipes with certain constraints.
struct bs_drm_pipe_plumber *bs_drm_pipe_plumber_new();
void bs_drm_pipe_plumber_destroy(struct bs_drm_pipe_plumber **);
// Takes ranks in the rank array format from drm_connectors.c. Lifetime of connector_ranks must
// exceed the plumber
void bs_drm_pipe_plumber_connector_ranks(struct bs_drm_pipe_plumber *,
					 const uint32_t *connector_ranks);
// crtc_mask is in the same format as drmModeEncoder.possible_crtcs
void bs_drm_pipe_plumber_crtc_mask(struct bs_drm_pipe_plumber *, uint32_t crtc_mask);
// Sets which card fd the plumber should use. The fd remains owned by the caller. If left unset,
// bs_drm_pipe_plumber_make will try all available cards.
void bs_drm_pipe_plumber_fd(struct bs_drm_pipe_plumber *, int card_fd);
// Sets a pointer to store the chosen connector in after a succesful call to
// bs_drm_pipe_plumber_make. It's optional, but calling drmModeGetConnector yourself can be slow.
void bs_drm_pipe_plumber_connector_ptr(struct bs_drm_pipe_plumber *, drmModeConnector **ptr);
// Makes the pipe based on the constraints of the plumber. Returns false if no pipe worked.
bool bs_drm_pipe_plumber_make(struct bs_drm_pipe_plumber *, struct bs_drm_pipe *pipe);

// Makes any pipe that will work for the given card fd. Returns false if no pipe worked.
bool bs_drm_pipe_make(int fd, struct bs_drm_pipe *pipe);

// drm_fb.c
struct bs_drm_fb_builder;

// A builder class used to collect drm framebuffer parameters and then build it.
struct bs_drm_fb_builder *bs_drm_fb_builder_new();
void bs_drm_fb_builder_destroy(struct bs_drm_fb_builder **);
// Copies all available framebuffer parameters from the given buffer object.
void bs_drm_fb_builder_gbm_bo(struct bs_drm_fb_builder *, struct gbm_bo *bo);
// Sets the drm format parameter of the resulting framebuffer.
void bs_drm_fb_builder_format(struct bs_drm_fb_builder *, uint32_t format);
// Creates the framebuffer ID from the previously set parameters and returns it or 0 if there was a
// failure.
uint32_t bs_drm_fb_builder_create_fb(struct bs_drm_fb_builder *);

// Creates a drm framebuffer from the given buffer object and returns the framebuffer's ID on
// success or 0 on failure.
uint32_t bs_drm_fb_create_gbm(struct gbm_bo *bo);

// drm_open.c
// Opens an arbitrary display's card.
int bs_drm_open_for_display();
// Opens the main display's card. This falls back to bs_drm_open_for_display().
int bs_drm_open_main_display();
int bs_drm_open_vgem();

// egl.c
struct bs_egl;
struct bs_egl_fb;

struct bs_egl *bs_egl_new();
void bs_egl_destroy(struct bs_egl **egl);
bool bs_egl_setup(struct bs_egl *self);
bool bs_egl_make_current(struct bs_egl *self);

EGLImageKHR bs_egl_image_create_gbm(struct bs_egl *self, struct gbm_bo *bo);
void bs_egl_image_destroy(struct bs_egl *self, EGLImageKHR *image);
bool bs_egl_image_flush_external(struct bs_egl *self, EGLImageKHR image);

EGLSyncKHR bs_egl_create_sync(struct bs_egl *self, EGLenum type, const EGLint *attrib_list);
EGLint bs_egl_wait_sync(struct bs_egl *self, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout);
EGLBoolean bs_egl_destroy_sync(struct bs_egl *self, EGLSyncKHR sync);

struct bs_egl_fb *bs_egl_fb_new(struct bs_egl *self, EGLImageKHR image);
bool bs_egl_target_texture2D(struct bs_egl *self, EGLImageKHR image);
bool bs_egl_has_extension(const char *extension, const char *extensions);
void bs_egl_fb_destroy(struct bs_egl_fb **fb);
GLuint bs_egl_fb_name(struct bs_egl_fb *self);

// gl.c
// The entry after the last valid binding should have name == NULL. The binding array is terminated
// by a NULL name.
struct bs_gl_program_create_binding {
	// These parameters are passed to glBindAttribLocation
	GLuint index;
	const GLchar *name;
};

GLuint bs_gl_shader_create(GLenum type, const GLchar *src);
// bindings can be NULL.
GLuint bs_gl_program_create_vert_frag_bind(const GLchar *vert_src, const GLchar *frag_src,
					   struct bs_gl_program_create_binding *bindings);

// app.c
struct bs_app;

struct bs_app *bs_app_new();
void bs_app_destroy(struct bs_app **app);
int bs_app_fd(struct bs_app *self);
size_t bs_app_fb_count(struct bs_app *self);
void bs_app_set_fb_count(struct bs_app *self, size_t fb_count);
struct gbm_bo *bs_app_fb_bo(struct bs_app *self, size_t index);
uint32_t bs_app_fb_id(struct bs_app *self, size_t index);
bool bs_app_setup(struct bs_app *self);
int bs_app_display_fb(struct bs_app *self, size_t index);

// mmap.c
struct bs_mapper;
struct bs_mapper *bs_mapper_dma_buf_new();
struct bs_mapper *bs_mapper_gem_new();
struct bs_mapper *bs_mapper_dumb_new(int device_fd);
void bs_mapper_destroy(struct bs_mapper *mapper);
void *bs_mapper_map(struct bs_mapper *mapper, struct gbm_bo *bo, size_t plane, void **map_data);
void bs_mapper_unmap(struct bs_mapper *mapper, struct gbm_bo *bo, void *map_data);

// draw.c

struct bs_draw_format;

bool bs_draw_stripe(struct bs_mapper *mapper, struct gbm_bo *bo,
		    const struct bs_draw_format *format);
bool bs_draw_transparent_hole(struct bs_mapper *mapper, struct gbm_bo *bo,
			      const struct bs_draw_format *format);

bool bs_draw_ellipse(struct bs_mapper *mapper, struct gbm_bo *bo,
		     const struct bs_draw_format *format, float progress);
bool bs_draw_cursor(struct bs_mapper *mapper, struct gbm_bo *bo,
		    const struct bs_draw_format *format);
bool bs_draw_lines(struct bs_mapper *mapper, struct gbm_bo *bo,
		   const struct bs_draw_format *format);
const struct bs_draw_format *bs_get_draw_format(uint32_t pixel_format);
const struct bs_draw_format *bs_get_draw_format_from_name(const char *str);
uint32_t bs_get_pixel_format(const struct bs_draw_format *format);
const char *bs_get_format_name(const struct bs_draw_format *format);
bool bs_parse_draw_format(const char *str, const struct bs_draw_format **format);

#endif
