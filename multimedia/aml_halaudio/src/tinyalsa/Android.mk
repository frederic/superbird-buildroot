LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    mixer.c \
    pcm.c

LOCAL_C_INCLUDES := \
  $(LOCAL_PATH)/include

LOCAL_CFLAGS += -Wall 

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_MODULE := libtinyalsa
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


