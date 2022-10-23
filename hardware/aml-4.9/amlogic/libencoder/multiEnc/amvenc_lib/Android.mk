LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS := \
	-fPIC -D_POSIX_SOURCE

LOCAL_LDLIBS := -lm -llog

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../vpuapi/include \
	$(LOCAL_PATH)/../vpuapi

LOCAL_SRC_FILES := \
	aml_v4l2.cpp libvpmulti_codec.cpp AML_MultiEncoder.cpp 

LOCAL_ARM_MODE := arm
LOCAL_SHARED_LIBRARIES += libutils libcutils libamvenc_api

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_MODULE := libvpcodec
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
