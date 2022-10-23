################################################################################
# Android optee-hello-world makefile                                           #
################################################################################
LOCAL_PATH := $(call my-dir)

CFG_TEEC_PUBLIC_PATH = $(LOCAL_PATH)/../../../ca_export_$(TARGET_ARCH)

################################################################################
# Build hello world                                                            #
################################################################################
include $(CLEAR_VARS)
LOCAL_CFLAGS += -DANDROID_BUILD
LOCAL_CFLAGS += -Wall

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_SRC_FILES += hello_world.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../ta/include \
		$(CFG_TEEC_PUBLIC_PATH)/include

LOCAL_SHARED_LIBRARIES := libteec
LOCAL_MODULE := tee_helloworld
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
