LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := aml_socketipc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := FAKE
LOCAL_SRC_FILES := socketipc.h
include $(BUILD_PREBUILT)
$(built_module) : all_copied_headers
LOCAL_COPY_HEADERS := socketipc.h
include $(BUILD_COPY_HEADERS)


