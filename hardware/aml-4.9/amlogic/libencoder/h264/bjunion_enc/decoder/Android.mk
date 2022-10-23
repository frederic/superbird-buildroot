LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)
LOCAL_MODULE    := libvpcodec_decoder
LOCAL_MODULE_TAGS := samples

LOCAL_SRC_FILES := decoder.c \
				   amlv4l.c \
				   amvideo.c \

LOCAL_ARM_MODE := arm

ifneq (,$(wildcard vendor/amlogic/frameworks/av/LibPlayer))
LIBPLAYER_DIR:=$(TOP)/vendor/amlogic/frameworks/av/LibPlayer
else
LIBPLAYER_DIR:=$(TOP)/packages/amlogic/LibPlayer
endif

LOCAL_C_INCLUDES := $(LIBPLAYER_DIR)/amcodec/include \
					$(TOP)/hardware/amlogic/gralloc

LOCAL_SHARED_LIBRARIES  += libion \
					libamplayer

include $(BUILD_SHARED_LIBRARY)

