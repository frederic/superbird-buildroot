LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES +=             \
    $(LOCAL_PATH)/../include    \
    $(LOCAL_PATH)/include       \
    $(LOCAL_PATH)/../tinyalsa/include   \
    $(LOCAL_PATH)/ini/include


LOCAL_SRC_FILES  += resample.c
LOCAL_MODULE := libspeex_rate

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -DBUILD_IN_ANDROID -DOUTSIDE_SPEEX -DRANDOM_PREFIX=AML_AUDIO -DFIXED_POINT

LOCAL_SHARED_LIBRARIES += liblog

include $(BUILD_SHARED_LIBRARY)
