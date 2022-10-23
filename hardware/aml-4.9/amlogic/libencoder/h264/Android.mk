LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= test.cpp

LOCAL_SHARED_LIBRARIES := \
        libvpcodec

LOCAL_C_INCLUDES:= \

LOCAL_CFLAGS += -Wno-multichar -Werror -Wall

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= testEncApi

include $(BUILD_EXECUTABLE)