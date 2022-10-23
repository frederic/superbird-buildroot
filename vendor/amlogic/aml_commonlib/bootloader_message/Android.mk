LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH) external/zlib
LOCAL_SRC_FILES := urlmisc.c bootloader_message.c

LOCAL_REQUIRED_MODULES += libz
LOCAL_SHARED_LIBRARIES += libz

LOCAL_ARM_MODE := arm
LOCAL_MODULE := libbootloader_message
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


