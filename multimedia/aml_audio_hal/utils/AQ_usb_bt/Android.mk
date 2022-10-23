LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := bt_usbtest

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libaudioclient \
    libmediaplayerservice \
    libcutils \
    libmedia_helper \

LOCAL_C_INCLUDES := \
    frameworks/av/media/libaudioclient/include \
    frameworks/av/media/libmediaplayerservice/include \

LOCAL_SRC_FILES := \
    bt_usb_tool.cpp

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
