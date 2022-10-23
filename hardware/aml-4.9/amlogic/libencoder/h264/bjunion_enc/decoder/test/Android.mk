LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := decoder
LOCAL_MODULE_TAGS := decoder

LOCAL_SRC_FILES := main.c \

LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/.. \

LOCAL_SHARED_LIBRARIES := libvpcodec \

include $(BUILD_EXECUTABLE)

