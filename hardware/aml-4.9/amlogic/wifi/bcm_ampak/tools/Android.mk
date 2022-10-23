LOCAL_PATH:=$(call my-dir)


include $(CLEAR_VARS)
LOCAL_MODULE := wl
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := dhd
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

ifeq ($(BCM_USB_WIFI),true)
    include $(LOCAL_PATH)/bcmdl/bcmdl/Android.mk $(LOCAL_PATH)/bcmdl/libusb2/Android.mk $(LOCAL_PATH)/bcmdl/libusb-compat2/Android.mk
endif

ifeq ($(MULTI_WIFI_SUPPORT),true)
    include $(LOCAL_PATH)/bcmdl/bcmdl/Android.mk $(LOCAL_PATH)/bcmdl/libusb2/Android.mk $(LOCAL_PATH)/bcmdl/libusb-compat2/Android.mk
endif
