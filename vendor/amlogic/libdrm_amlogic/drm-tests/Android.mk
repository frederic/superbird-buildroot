# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_PATH := $(call my-dir)

#########################

bsdrm_srcs = \
	bsdrm/src/app.c \
	bsdrm/src/debug.c \
	bsdrm/src/draw.c \
	bsdrm/src/drm_connectors.c \
	bsdrm/src/drm_fb.c \
	bsdrm/src/drm_open.c \
	bsdrm/src/drm_pipe.c \
	bsdrm/src/egl.c \
	bsdrm/src/gl.c \
	bsdrm/src/mmap.c \
	bsdrm/src/open.c \
	bsdrm/src/pipe.c

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(bsdrm_srcs) atomictest.c

LOCAL_MODULE := atomictest

LOCAL_C_INCLUDES := \
	external/minigbm \
	$(LOCAL_PATH)/bsdrm/include \
	$(VENDOR_SDK_INCLUDES)
LOCAL_CFLAGS := -O2 -g -W -Wall
LOCAL_CFLAGS += -DUSE_DRM -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES
LOCAL_STATIC_LIBRARIES := libdrm
LOCAL_SHARED_LIBRARIES := libminigbm

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(bsdrm_srcs) gamma_test.c

LOCAL_MODULE := gamma_test

LOCAL_C_INCLUDES := \
	external/minigbm \
	$(LOCAL_PATH)/bsdrm/include \
	$(VENDOR_SDK_INCLUDES)
LOCAL_CFLAGS := -O2 -g -W -Wall
LOCAL_CFLAGS += -DUSE_DRM -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES
LOCAL_STATIC_LIBRARIES := libdrm
LOCAL_SHARED_LIBRARIES := libminigbm

include $(BUILD_EXECUTABLE)
