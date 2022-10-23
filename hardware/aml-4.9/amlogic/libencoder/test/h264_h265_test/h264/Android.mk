LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	test.cpp \
	test_dma.cpp \
	bjunion_enc/libvpcodec.cpp \
	bjunion_enc/AML_HWEncoder.cpp \
	bjunion_enc/enc_api.cpp \
	bjunion_enc/enc/gx_enc_fast/gxvenclib_fast.cpp \
	bjunion_enc/enc/gx_enc_fast/rate_control_gx_fast.cpp \
	bjunion_enc/enc/gx_enc_fast/parser.cpp

#LOCAL_SHARED_LIBRARIES := \
#        libvpcodec

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/bjunion_enc \
	$(LOCAL_PATH)/bjunion_enc/include \
	$(LOCAL_PATH)/bjunion_enc/enc/gx_enc_fast

LOCAL_CFLAGS += -Wno-multichar -Wno-error -Wall -g

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= libAvcApi

include $(BUILD_STATIC_LIBRARY)

#include $(call all-makefiles-under,$(LOCAL_PATH))
