LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_LDLIBS    := -lm -llog
LOCAL_SRC_FILES := \
    libvpcodec.cpp

LOCAL_SRC_FILES += AML_HWEncoder.cpp \
	enc_api.cpp \
	enc/gx_enc_fast/rate_control_gx_fast.cpp \
	enc/gx_enc_fast/gxvenclib_fast.cpp \
	enc/gx_enc_fast/parser.cpp \
	enc/common/fill_buffer.cpp \

#define MAKEANDROID

LOCAL_SRC_FILES += enc/intra_search/pred.cpp \
	enc/intra_search/pred_neon_asm.s

ifneq (,$(wildcard vendor/amlogic/frameworks/av/LibPlayer))
LIBPLAYER_DIR:=$(TOP)/vendor/amlogic/frameworks/av/LibPlayer
else
LIBPLAYER_DIR:=$(TOP)/packages/amlogic/LibPlayer
endif

LOCAL_SHARED_LIBRARIES  += libcutils \
						libutils  \
						libion \
#						libamplayer


LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
#		 $(LIBPLAYER_DIR)/amcodec/include \
		 $(TOP)/hardware/amlogic/gralloc \
#		 $(LOCAL_PATH)/decoder

LOCAL_ARM_MODE := arm
LOCAL_MODULE:= libvpcodec
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

