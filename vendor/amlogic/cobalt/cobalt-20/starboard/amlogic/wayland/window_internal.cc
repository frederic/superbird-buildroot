// Copyright 2018 The Cobalt Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "starboard/shared/wayland/window_internal.h"

#include "starboard/common/log.h"
#ifndef AML_OSD_USE_DRM
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <asm/types.h>
#include <linux/fb.h>
#include <unistd.h>
#endif
namespace {

const int kWindowWidth = 1920;
const int kWindowHeight = 1080;

// shell_surface_listener
void ShellSurfacePing(void*,
                      struct wl_shell_surface* shell_surface,
                      uint32_t serial) {
  wl_shell_surface_pong(shell_surface, serial);
}

void ShellSurfaceConfigure(void* data,
                           struct wl_shell_surface*,
                           uint32_t,
                           int32_t width,
                           int32_t height) {
  SB_DLOG(INFO) << "shell_surface_configure width(" << width << "), height("
                << height << ")";
  if (width && height) {
    SbWindowPrivate* window = reinterpret_cast<SbWindowPrivate*>(data);
    wl_egl_window_resize(window->egl_window, width, height, 0, 0);
  } else {
    SB_DLOG(INFO) << "width and height is 0. we don't resize that";
  }
}

void ShellSurfacePopupDone(void*, struct wl_shell_surface*) {}

struct wl_shell_surface_listener shell_surface_listener = {
    &ShellSurfacePing, &ShellSurfaceConfigure, &ShellSurfacePopupDone};
}  // namespace

SbWindowPrivate::SbWindowPrivate(wl_compositor* compositor,
                                 wl_shell* shell,
                                 const SbWindowOptions* options,
                                 float pixel_ratio) {
  width = kWindowWidth;
  height = kWindowHeight;
  video_pixel_ratio = pixel_ratio;
#ifndef AML_OSD_USE_DRM
  int fh;
  if ((fh = open("/dev/fb0", O_RDONLY)) > 0)
  {
    struct fb_var_screeninfo var;
    if (ioctl(fh, FBIOGET_VSCREENINFO, &var)==0)
    {
      if ((var.xres == 1280)&& (var.yres==720))
      {
        width=var.xres;
        height=var.yres;
        SB_LOG(INFO) << "FB is 720P " << var.xres << var.yres ;
      }
    }
    close(fh);
  }
#endif
  if (options && options->size.width > 0 && options->size.height > 0) {
    width = options->size.width;
    height = options->size.height;
  }

  FILE * fp = fopen("/sys/class/video/device_resolution", "r");
  if (fp != NULL) {
      int display_width = width;
      int display_height = height;
      fscanf(fp, "%dx%d", &display_width, &display_height);
      fclose(fp);
      video_pixel_ratio = std::max(
              static_cast<float>(display_width) / width,
              static_cast<float>(display_height) / height);
  }

  surface = wl_compositor_create_surface(compositor);
  shell_surface = wl_shell_get_shell_surface(shell, surface);
  wl_shell_surface_add_listener(shell_surface, &shell_surface_listener, this);
  wl_shell_surface_set_title(shell_surface, "cobalt");

  /*
  struct wl_region* region;
  region = wl_compositor_create_region(compositor);
  wl_region_add(region, 0, 0, width, height);
  wl_surface_set_opaque_region(surface, region);
  wl_region_destroy(region);
  */

  egl_window = wl_egl_window_create(surface, width, height);

  WindowRaise();
}

SbWindowPrivate::~SbWindowPrivate() {
  wl_egl_window_destroy(egl_window);
  wl_shell_surface_destroy(shell_surface);
  wl_surface_destroy(surface);
}

void SbWindowPrivate::WindowRaise() {
  if (shell_surface)
    wl_shell_surface_set_toplevel(shell_surface);
}
