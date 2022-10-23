################################################################################
# Android optee-crypto makefile                                                #
################################################################################
LOCAL_PATH := $(call my-dir)

CFG_TEEC_PUBLIC_PATH = $(LOCAL_PATH)/../../../ca_export_$(TARGET_ARCH)
TA_DEV_KIT_DIR=$(LOCAL_PATH)/../../../ta_export

################################################################################
# Build crypto                                                                 #
################################################################################
include $(CLEAR_VARS)
LOCAL_CFLAGS += -DANDROID_BUILD
LOCAL_CFLAGS += -Wall

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_SRC_FILES += crypto.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../optee_test/ta/crypt/include/ \
		$(TA_DEV_KIT_DIR)/host_include \
		$(CFG_TEEC_PUBLIC_PATH)/include

LOCAL_SHARED_LIBRARIES := libteec
LOCAL_MODULE := tee_crypto
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
