# Copyright 2005 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    hardware/$(PLATFORM_NAME)/audio/utils/include

LOCAL_SRC_FILES:= \
    test.c

LOCAL_MODULE:= testamlconf

#LOCAL_FORCE_STATIC_EXECUTABLE := true
#LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
#LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_UNSTRIPPED)

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        libc \
        libamaudioutils

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    hardware/$(PLATFORM_NAME)/audio/utils/include

LOCAL_CFLAGS := -DBUILDHOSTEXE

LOCAL_SRC_FILES:= \
    test_data_utils.c \
    ../aml_data_utils.c

LOCAL_MODULE:= testdatautils

LOCAL_SHARED_LIBRARIES :=

include $(BUILD_HOST_EXECUTABLE)
