LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
ifeq ($(TARGET_ARCH),arm)
LOCAL_SRC_FILES := bin_android/tee-supplicant
else
LOCAL_SRC_FILES := ../ca_export_arm64/bin_android/tee-supplicant
endif
LOCAL_MODULE := tee-supplicant
LOCAL_MODULE_CLASS := EXECUTABLES

LOCAL_INIT_RC := bin_android/tee-supplicant.rc

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_SRC_FILES_32 := lib_android/libteec.so
LOCAL_SRC_FILES_64 := ../ca_export_arm64/lib_android/libteec.so
LOCAL_MODULE := libteec
LOCAL_MULTILIB := both
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES_32 := lib_android/libteec_sys.so
LOCAL_SRC_FILES_64 := ../ca_export_arm64/lib_android/libteec_sys.so
LOCAL_MODULE := libteec_sys
LOCAL_MULTILIB := both
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

include $(BUILD_PREBUILT)

ifneq ($(filter userdebug eng,$(TARGET_BUILD_VARIANT)),)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := scripts/tdk_auto_test.sh
LOCAL_MODULE := tdk_auto_test
LOCAL_MODULE_SUFFIX := .sh
LOCAL_MODULE_CLASS := EXECUTABLES

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)
endif
