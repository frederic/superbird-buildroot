LOCAL_PATH      := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := libamlutil
LOCAL_SRC_FILES := unifykey.c
LOCAL_COPY_HEADERS := unifykey.h
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
