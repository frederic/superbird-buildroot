LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)	\
                    external/zlib

LOCAL_SRC_FILES := ubootenv.c

LOCAL_REQUIRED_MODULES += libz
LOCAL_SHARED_LIBRARIES += libz

LOCAL_ARM_MODE := arm
LOCAL_MODULE := libubootenv
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_SRC_FILES := uenv_test.c

LOCAL_SHARED_LIBRARIES += libz libubootenv

LOCAL_REQUIRED_MODULES += libubootenv
LOCAL_MODULE := uenv
LOCAL_ARM_MODE := arm
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

