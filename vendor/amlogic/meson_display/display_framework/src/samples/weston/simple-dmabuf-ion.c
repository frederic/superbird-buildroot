/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <fcntl.h>

#include <drm_fourcc.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <IONmem.h>
#include <linux/input.h>

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "fullscreen-shell-unstable-v1-client-protocol.h"
#include "linux-dmabuf-unstable-v1-client-protocol.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define BUFFER_WIDTH 1920
#define BUFFER_HEIGHT 1080
#define BUFFER_STRIDES ((BUFFER_WIDTH + 31)/32) * 32
#define BUFFER_NUM_PLANES 2 //NV12

static void
redraw(void *data, struct wl_callback *callback, uint32_t time);

static uint32_t
parse_format(const char fmt[4])
{
	return fourcc_code(fmt[0], fmt[1], fmt[2], fmt[3]);
}

static inline const char *
dump_format(uint32_t format, char out[4])
{
#if BYTE_ORDER == BIG_ENDIAN
	format = __builtin_bswap32(format);
#endif
	memcpy(out, &format, 4);
	return out;
}

struct display {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_seat *seat;
	struct wl_keyboard *keyboard;
	struct xdg_wm_base *wm_base;
	struct zwp_fullscreen_shell_v1 *fshell;
	struct zwp_linux_dmabuf_v1 *dmabuf;
	bool requested_format_found;

	int v4l_fd;
	uint32_t drm_format;
};

struct buffer {
	struct wl_buffer *buffer;
	struct display *display;
	int busy;
	int index;

    int size;
    struct IONMEM_AllocParams ion_buffer_param;
	int dmabuf_fds[2];
    void* addr;
	int data_offsets[2];
};

#define NUM_BUFFERS 3

struct window {
	struct display *display;
	struct wl_surface *surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;
	struct buffer buffers[NUM_BUFFERS];
	struct wl_callback *callback;
	bool wait_for_configure;
	bool initialized;
};

static bool running = true;

static int
queue_buffer(struct display *display, struct buffer *buffer)
{
    //fake queue buffer to draw on client
    static unsigned char tmp = 0;
    tmp++;
    memset(buffer->addr, tmp, buffer->size);
    return  0;
}

static void
buffer_release(void *data, struct wl_buffer *buffer)
{
	struct buffer *mybuf = data;

	mybuf->busy = 0;

    queue_buffer(mybuf->display, mybuf);
}

static const struct wl_buffer_listener buffer_listener = {
	buffer_release
};

static void
create_succeeded(void *data,
		 struct zwp_linux_buffer_params_v1 *params,
		 struct wl_buffer *new_buffer)
{
	struct buffer *buffer = data;
	unsigned i;

	buffer->buffer = new_buffer;
	wl_buffer_add_listener(buffer->buffer, &buffer_listener, buffer);

	zwp_linux_buffer_params_v1_destroy(params);

}

static void
create_failed(void *data, struct zwp_linux_buffer_params_v1 *params)
{
	struct buffer *buffer = data;
	unsigned i;

	buffer->buffer = NULL;

	zwp_linux_buffer_params_v1_destroy(params);
    CMEM_free(&buffer->ion_buffer_param);
    munmap(buffer->addr, buffer->size);

	running = false;

	fprintf(stderr, "Error: zwp_linux_buffer_params.create failed.\n");
}

static const struct zwp_linux_buffer_params_v1_listener params_listener = {
	create_succeeded,
	create_failed
};

static void
create_dmabuf_buffer(struct display *display, struct buffer *buffer)
{
	struct zwp_linux_buffer_params_v1 *params;
	uint64_t modifier;
	uint32_t flags;
	unsigned i;

	modifier = 0;
	flags = 0;

	/* XXX: apparently some webcams may actually provide y-inverted images,
	 * in which case we should set
	 * flags = ZWP_LINUX_BUFFER_PARAMS_V1_FLAGS_Y_INVERT
	 */

	params = zwp_linux_dmabuf_v1_create_params(display->dmabuf);
    for (i = 0; i < BUFFER_NUM_PLANES; ++i) {
        zwp_linux_buffer_params_v1_add(params,
                                       buffer->dmabuf_fds[i],
                                       i, /* plane_idx */
                                       buffer->data_offsets[i], /* offset */
                                       BUFFER_STRIDES,
                                       modifier >> 32,
                                       modifier & 0xffffffff);
    }
	zwp_linux_buffer_params_v1_add_listener(params, &params_listener,
	                                        buffer);
	zwp_linux_buffer_params_v1_create(params,
	                                  BUFFER_WIDTH,
	                                  BUFFER_HEIGHT,
	                                  display->drm_format,
	                                  flags);
}

static int
queue_initial_buffers(struct display *display,
                      struct buffer buffers[NUM_BUFFERS])
{
	struct buffer *buffer;
	int index;
    int buffer_size;

    buffer_size = BUFFER_WIDTH * BUFFER_HEIGHT * 3 / 2; //for NV12
	for (index = 0; index < NUM_BUFFERS; ++index) {
		buffer = &buffers[index];
		buffer->display = display;
		buffer->index = index;

        if (CMEM_alloc(buffer_size, &buffer->ion_buffer_param) < 0) {
			fprintf(stderr, "Failed to CMEM_alloc %d bytes\n", buffer_size);
            buffer->size = 0;
        }

		assert(!buffer->size);
        buffer->size = buffer_size;
        buffer->addr = mmap(NULL, buffer->size, PROT_READ | PROT_WRITE, MAP_SHARED, buffer->ion_buffer_param.mImageFd, 0);
        buffer->dmabuf_fds[0] = buffer->ion_buffer_param.mImageFd;
        buffer->dmabuf_fds[1] = buffer->ion_buffer_param.mImageFd;
        buffer->data_offsets[0] = 0;
        buffer->data_offsets[1] = BUFFER_WIDTH * BUFFER_HEIGHT;

		if (queue_buffer(display, buffer)) {
			fprintf(stderr, "Failed to draw buffer\n");
			return 0;
		}
		create_dmabuf_buffer(display, buffer);
	}

	return 1;
}

static int
dequeue(struct window *window)
{
	int index;
//TODO: Add buffer queue.
//we draw on queue buffer/ send on dequeue. same as v4l
	for (index = 0; index < NUM_BUFFERS; ++index)
		if (!window->buffers[index].busy)
            return index;
}


//Init ion buffer, export to dmabuffer
static int
ion_buffer_init(struct display *display, struct buffer buffers[NUM_BUFFERS]) {
    CMEM_init();

	if (!queue_initial_buffers(display, buffers)) {
		fprintf(stderr, "Failed to queue initial buffers\n");
		return 0;
	}

	return 1;
}


static void
xdg_surface_handle_configure(void *data, struct xdg_surface *surface,
			     uint32_t serial)
{
	struct window *window = data;

	xdg_surface_ack_configure(surface, serial);

	if (window->initialized && window->wait_for_configure)
		redraw(window, NULL, 0);
	window->wait_for_configure = false;
}

static const struct xdg_surface_listener xdg_surface_listener = {
	xdg_surface_handle_configure,
};

static void
xdg_toplevel_handle_configure(void *data, struct xdg_toplevel *toplevel,
			      int32_t width, int32_t height,
			      struct wl_array *states)
{
}

static void
xdg_toplevel_handle_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
	running = 0;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	xdg_toplevel_handle_configure,
	xdg_toplevel_handle_close,
};

static struct window *
create_window(struct display *display)
{
	struct window *window;

	window = calloc(1, sizeof *window);
	if (!window)
		return NULL;

	window->callback = NULL;
	window->display = display;
	window->surface = wl_compositor_create_surface(display->compositor);

	if (display->wm_base) {
		window->xdg_surface =
			xdg_wm_base_get_xdg_surface(display->wm_base,
						    window->surface);

		assert(window->xdg_surface);

		xdg_surface_add_listener(window->xdg_surface,
					 &xdg_surface_listener, window);

		window->xdg_toplevel =
			xdg_surface_get_toplevel(window->xdg_surface);

		assert(window->xdg_toplevel);

		xdg_toplevel_add_listener(window->xdg_toplevel,
					  &xdg_toplevel_listener, window);

		xdg_toplevel_set_title(window->xdg_toplevel, "simple-dmabuf-ion");

		window->wait_for_configure = true;
		wl_surface_commit(window->surface);
	} else if (display->fshell) {
		zwp_fullscreen_shell_v1_present_surface(display->fshell,
		                                        window->surface,
		                                        ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_DEFAULT,
		                                        NULL);
	} else {
		assert(0);
	}

	return window;
}

static void
destroy_window(struct window *window)
{
	int i;
	unsigned j;

	if (window->callback)
		wl_callback_destroy(window->callback);

	if (window->xdg_toplevel)
		xdg_toplevel_destroy(window->xdg_toplevel);
	if (window->xdg_surface)
		xdg_surface_destroy(window->xdg_surface);
	wl_surface_destroy(window->surface);

	for (i = 0; i < NUM_BUFFERS; i++) {
		if (!window->buffers[i].buffer)
			continue;

		wl_buffer_destroy(window->buffers[i].buffer);

        CMEM_free(&window->buffers[i].ion_buffer_param);
        munmap(window->buffers[i].addr, window->buffers[i].size);
	}

	free(window);
}

static const struct wl_callback_listener frame_listener;

static void
redraw(void *data, struct wl_callback *callback, uint32_t time)
{
	struct window *window = data;
	struct buffer *buffer;
	int index, num_busy = 0;

	/* Check for a deadlock situation where we would block forever trying
	 * to dequeue a buffer while all of them are locked by the compositor.
	 */
	for (index = 0; index < NUM_BUFFERS; ++index)
		if (window->buffers[index].busy)
			++num_busy;

	/* A robust application would just postpone redraw until it has queued
	 * a buffer.
	 */
	assert(num_busy < NUM_BUFFERS);

	//index = dequeue(window->display);
	index = dequeue(window);

	buffer = &window->buffers[index];
	assert(!buffer->busy);

	wl_surface_attach(window->surface, buffer->buffer, 0, 0);
	wl_surface_damage(window->surface, 0, 0,
	                  BUFFER_WIDTH,
                      BUFFER_HEIGHT);

	if (callback)
		wl_callback_destroy(callback);

	window->callback = wl_surface_frame(window->surface);
	wl_callback_add_listener(window->callback, &frame_listener, window);
	wl_surface_commit(window->surface);
	buffer->busy = 1;
}

static const struct wl_callback_listener frame_listener = {
	redraw
};

static void
dmabuf_format(void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf,
              uint32_t format)
{
	struct display *d = data;

	if (format == d->drm_format) {
		d->requested_format_found = true;
    }
}

static const struct zwp_linux_dmabuf_v1_listener dmabuf_listener = {
	dmabuf_format
};

static void
keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
                       uint32_t format, int fd, uint32_t size)
{
	/* Just so we don’t leak the keymap fd */
	close(fd);
}

static void
keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface,
                      struct wl_array *keys)
{
}

static void
keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface)
{
}

static void
keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
                    uint32_t serial, uint32_t time, uint32_t key,
                    uint32_t state)
{
	struct display *d = data;

	if (!d->wm_base)
		return;

	if (key == KEY_ESC && state)
		running = false;
}

static void
keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
                          uint32_t serial, uint32_t mods_depressed,
                          uint32_t mods_latched, uint32_t mods_locked,
                          uint32_t group)
{
}

static const struct wl_keyboard_listener keyboard_listener = {
	keyboard_handle_keymap,
	keyboard_handle_enter,
	keyboard_handle_leave,
	keyboard_handle_key,
	keyboard_handle_modifiers,
};

static void
seat_handle_capabilities(void *data, struct wl_seat *seat,
                         enum wl_seat_capability caps)
{
	struct display *d = data;

	if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !d->keyboard) {
		d->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(d->keyboard, &keyboard_listener, d);
	} else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && d->keyboard) {
		wl_keyboard_destroy(d->keyboard);
		d->keyboard = NULL;
	}
}

static const struct wl_seat_listener seat_listener = {
	seat_handle_capabilities,
};

static void
xdg_wm_base_ping(void *data, struct xdg_wm_base *shell, uint32_t serial)
{
	xdg_wm_base_pong(shell, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
	xdg_wm_base_ping,
};

static void
registry_handle_global(void *data, struct wl_registry *registry,
                       uint32_t id, const char *interface, uint32_t version)
{
	struct display *d = data;

	if (strcmp(interface, "wl_compositor") == 0) {
		d->compositor =
			wl_registry_bind(registry,
			                 id, &wl_compositor_interface, 1);
	} else if (strcmp(interface, "wl_seat") == 0) {
		d->seat = wl_registry_bind(registry,
		                           id, &wl_seat_interface, 1);
		wl_seat_add_listener(d->seat, &seat_listener, d);
	} else if (strcmp(interface, "xdg_wm_base") == 0) {
		d->wm_base = wl_registry_bind(registry,
					      id, &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(d->wm_base, &wm_base_listener, d);
	} else if (strcmp(interface, "zwp_fullscreen_shell_v1") == 0) {
		d->fshell = wl_registry_bind(registry,
		                             id, &zwp_fullscreen_shell_v1_interface,
		                             1);
	} else if (strcmp(interface, "zwp_linux_dmabuf_v1") == 0) {
		d->dmabuf = wl_registry_bind(registry,
		                             id, &zwp_linux_dmabuf_v1_interface,
		                             1);
		zwp_linux_dmabuf_v1_add_listener(d->dmabuf, &dmabuf_listener,
		                                 d);
	}
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
                              uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
	registry_handle_global,
	registry_handle_global_remove
};

static struct display *
create_display(uint32_t requested_format)
{
	struct display *display;

	display = malloc(sizeof *display);
	if (display == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	display->display = wl_display_connect(NULL);
	assert(display->display);

	display->drm_format = requested_format;

	display->registry = wl_display_get_registry(display->display);
	wl_registry_add_listener(display->registry,
	                         &registry_listener, display);
	wl_display_roundtrip(display->display);
	if (display->dmabuf == NULL) {
		fprintf(stderr, "No zwp_linux_dmabuf global\n");
		exit(1);
	}

	wl_display_roundtrip(display->display);

	/* XXX: fake, because the compositor does not yet advertise anything */
	display->requested_format_found = true;

	if (!display->requested_format_found) {
		fprintf(stderr, "DRM_FORMAT_YUYV not available\n");
		exit(1);
	}

	return display;
}

static void
destroy_display(struct display *display)
{
	if (display->dmabuf)
		zwp_linux_dmabuf_v1_destroy(display->dmabuf);

	if (display->wm_base)
		xdg_wm_base_destroy(display->wm_base);

	if (display->fshell)
		zwp_fullscreen_shell_v1_release(display->fshell);

	if (display->compositor)
		wl_compositor_destroy(display->compositor);

	wl_registry_destroy(display->registry);
	wl_display_flush(display->display);
	wl_display_disconnect(display->display);
	free(display);
}

static void
usage(const char *argv0)
{
	printf("Usage: %s [DRM format]\n"
	       "\n"
	       "The default ion dmabuffer useage \n"
	       "\n"
	       "Both formats are FOURCC values (see http://fourcc.org/)\n"
	       "DRM formats are defined in <libdrm/drm_fourcc.h>\n"
	       "The default for both formats is YUYV.\n"
	       "reinterpreted rather than converted.\n", argv0);

	printf("\n"
	       "How to set up Vivid the virtual video driver for testing:\n"
	       "- build your kernel with CONFIG_VIDEO_VIVID=m\n"
	       "- add this to a /etc/modprobe.d/ file:\n"
	       "    options vivid node_types=0x1 num_inputs=1 input_types=0x00\n"
	       "- modprobe vivid and check which device was created,\n"
	       "  here we assume /dev/video0\n"
	       "- set the pixel format:\n"
	       "- launch the demo:\n"
	       "You should see a test pattern with color bars, and some text.\n"
	       "\n"
	       "More about vivid: https://www.kernel.org/doc/Documentation/video4linux/vivid.txt\n"
	       "\n", argv0);

	exit(0);
}

static void
signal_int(int signum)
{
	running = false;
}

int
main(int argc, char **argv)
{
	struct sigaction sigint;
	struct display *display;
	struct window *window;
	uint32_t ion_format;
	int ret = 0;

	if (!strcmp(argv[1], "--help")) {
	//	usage(argv[0]);
	}

    ion_format = parse_format("NV12");

	display = create_display(ion_format);

	window = create_window(display);
	if (!window)
		return 1;
	if (!ion_buffer_init(display, window->buffers))
		return 1;

	sigint.sa_handler = signal_int;
	sigemptyset(&sigint.sa_mask);
	sigint.sa_flags = SA_RESETHAND;
	sigaction(SIGINT, &sigint, NULL);

	/* Here we retrieve the linux-dmabuf objects, or error */
	wl_display_roundtrip(display->display);

	/* In case of error, running will be 0 */
	if (!running)
		return 1;


	window->initialized = true;

	if (!window->wait_for_configure)
		redraw(window, NULL, 0);

	while (running /*&& ret != -1*/)
		ret = wl_display_dispatch(display->display);

	fprintf(stderr, "simple-dmabuf-v4l exiting\n");
	destroy_window(window);
	destroy_display(display);

	return 0;
}
