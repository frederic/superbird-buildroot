# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Pull in chromium os defaults
OUT ?= $(PWD)/build-opt-local

include common.mk

PC_DEPS = libdrm egl gbm
PC_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PC_DEPS))
PC_LIBS := $(shell $(PKG_CONFIG) --libs $(PC_DEPS))

DRM_LIBS = -lGLESv2
CFLAGS += $(PC_CFLAGS) -DEGL_EGLEXT_PROTOTYPES -DGL_GLEXT_PROTOTYPES
CFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -Wno-error
LDLIBS += $(PC_LIBS)

all: \
	CC_BINARY(atomictest) \
	CC_BINARY(drm_cursor_test) \
	CC_BINARY(gamma_test) \
	CC_BINARY(linear_bo_test) \
	CC_BINARY(mapped_texture_test) \
	CC_BINARY(mmap_test) \
	CC_BINARY(null_platform_test) \
	CC_BINARY(plane_test) \
	CC_BINARY(stripe) \
	CC_BINARY(swrast_test) \
	CC_BINARY(vgem_test)

ifeq ($(USE_VULKAN),1)
all: CC_BINARY(vk_glow)
endif

CC_BINARY(drm_cursor_test): drm_cursor_test.o CC_STATIC_LIBRARY(libbsdrm.pic.a)

CC_BINARY(null_platform_test): null_platform_test.o CC_STATIC_LIBRARY(libbsdrm.pic.a)
CC_BINARY(null_platform_test): LDLIBS += $(DRM_LIBS)

CC_BINARY(vgem_test): vgem_test.o CC_STATIC_LIBRARY(libbsdrm.pic.a)
CC_BINARY(mmap_test): mmap_test.o CC_STATIC_LIBRARY(libbsdrm.pic.a)

CC_BINARY(linear_bo_test): linear_bo_test.o CC_STATIC_LIBRARY(libbsdrm.pic.a)
CC_BINARY(linear_bo_test): LDLIBS += -lGLESv2

CC_BINARY(swrast_test): swrast_test.o CC_STATIC_LIBRARY(libbsdrm.pic.a)
CC_BINARY(swrast_test): LDLIBS += -lGLESv2

CC_BINARY(atomictest): atomictest.o CC_STATIC_LIBRARY(libbsdrm.pic.a)
CC_BINARY(atomictest): CFLAGS += -DUSE_ATOMIC_API
CC_BINARY(atomictest): LDLIBS += $(DRM_LIBS)

CC_BINARY(gamma_test): gamma_test.o CC_STATIC_LIBRARY(libbsdrm.pic.a)
CC_BINARY(gamma_test): LDLIBS += -lm $(DRM_LIBS)

CC_BINARY(plane_test): plane_test.o CC_STATIC_LIBRARY(libbsdrm.pic.a)
CC_BINARY(plane_test): LDLIBS += -lm $(DRM_LIBS)

CC_BINARY(mapped_texture_test): mapped_texture_test.o CC_STATIC_LIBRARY(libbsdrm.pic.a)
CC_BINARY(mapped_texture_test): LDLIBS += -lGLESv2

ifeq ($(USE_VULKAN),1)
CC_BINARY(vk_glow): vk_glow.o CC_STATIC_LIBRARY(libbsdrm.pic.a)
CC_BINARY(vk_glow): LDLIBS += -lm -lvulkan $(DRM_LIBS)
endif
