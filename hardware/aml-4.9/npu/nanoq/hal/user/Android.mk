##############################################################################
#
#    Copyright (c) 2005 - 2019 by Vivante Corp.  All rights reserved.
#
#    The material in this file is confidential and contains trade secrets
#    of Vivante Corporation. This is proprietary information owned by
#    Vivante Corporation. No part of this work may be disclosed,
#    reproduced, copied, transmitted, or used in any way for any purpose,
#    without the express written permission of Vivante Corporation.
#
##############################################################################


LOCAL_PATH := $(call my-dir)
include $(LOCAL_PATH)/../../Android.mk.def

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    gc_hal_user_brush.c \
    gc_hal_user_brush_cache.c \
    gc_hal_user_dump.c \
    gc_hal_user.c \
    gc_hal_user_raster.c \
    gc_hal_user_hash.c \
    gc_hal_user_heap.c \
    gc_hal_user_query.c \
    gc_hal_user_rect.c \
    gc_hal_user_buffer.c \
    gc_hal_user_surface.c \
    gc_hal_user_queue.c \
    gc_hal_user_profiler.c \
    gc_hal_user_bitmask.c \


ifeq ($(VIVANTE_ENABLE_3D),1)
LOCAL_SRC_FILES += \
    gc_hal_user_engine.c \
    gc_hal_user_index.c \
    gc_hal_user_vertex_array.c \
    gc_hal_user_vertex.c \
    gc_hal_user_format.c \
    gc_hal_user_texture.c \
    gc_hal_user_mem.c \
    gc_hal_user_bufobj.c \
    gc_hal_user_statistics.c \
    gc_hal_user_shader.c

ifeq ($(USE_OPENCL),1)
LOCAL_SRC_FILES += \
    gc_hal_user_cl.c
endif
else
endif

LOCAL_CFLAGS := \
    $(CFLAGS) \
    -Werror

LOCAL_C_INCLUDES := \
    $(AQROOT)/hal/inc \
    $(AQROOT)/hal/user \
    $(AQROOT)/hal/os/linux/user


ifeq ($(USE_OPENVX),1)
LOCAL_SRC_FILES += \
    gc_hal_user_vx.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)
endif


ifeq ($(VIVANTE_ENABLE_3D),1)
LOCAL_C_INCLUDES +=\
    $(AQROOT)/compiler/libVSC/include
endif

LOCAL_STATIC_LIBRARIES := \
    libhalarchuser

LOCAL_STATIC_LIBRARIES += \
    libhalosuser


LOCAL_LDFLAGS := \
    -Wl,-z,defs \
    -Wl,--version-script=$(LOCAL_PATH)/libGAL.map

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libdl

ifeq ($(VIVANTE_ENABLE_VSIMULATOR),1)
LOCAL_SHARED_LIBRARIES += \
    libEmulator

endif

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 17),1)
LOCAL_SHARED_LIBRARIES += \
    libsync
endif

LOCAL_MODULE         := libGAL
LOCAL_MODULE_TAGS    := optional
LOCAL_PRELINK_MODULE := false
ifeq ($(PLATFORM_VENDOR),1)
LOCAL_VENDOR_MODULE  := true
endif
include $(BUILD_SHARED_LIBRARY)

include $(AQROOT)/copy_installed_module.mk

ifeq ($(VIVANTE_ENABLE_VSIMULATOR),0)


# libhalarchuser
include $(AQROOT)/hal/user/arch/Android.mk

# libhalosuser
include $(AQROOT)/hal/os/linux/user/Android.mk

else

# libhalarchuser
include $(AQROOT)/hal/user/arch/Android.mk

# libhalosuser
include $(AQROOT)/vsimulator/os/linux/user/Android.mk

endif

