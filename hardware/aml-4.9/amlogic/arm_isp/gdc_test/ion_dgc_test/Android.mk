# Copyright (c) 2018 Google Inc. All rights reserved.
LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := gdc_test.c gdc.c ion.c
LOCAL_MODULE := gdc_test
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include $(LOCAL_PATH)/kernel-headers
LOCAL_LDFLAGS += -Wl

LOCAL_SHARED_LIBRARIES += liblog

include $(BUILD_EXECUTABLE)

