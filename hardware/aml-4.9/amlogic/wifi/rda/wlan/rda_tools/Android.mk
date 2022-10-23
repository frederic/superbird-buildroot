LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
OBJS_C := rda_tools.c
LOCAL_MODULE := rda_tools
LOCAL_MODULE_TAGS := debug
LOCAL_SHARED_LIBRARIES := libc libcutils
LOCAL_CFLAGS := $(L_CFLAGS)
LOCAL_SRC_FILES := $(OBJS_C)
include $(BUILD_EXECUTABLE)
