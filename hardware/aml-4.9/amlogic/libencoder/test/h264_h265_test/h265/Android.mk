LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= test.cpp test_dma.cpp\
	hevc_enc/libvp_hevc_codec.cpp \
	hevc_enc/AML_HEVCEncoder.cpp \
	hevc_enc/vdi.cpp

LOCAL_SHARED_LIBRARIES :=  libutils libcutils

LOCAL_C_INCLUDES:= hevc_enc/include

LOCAL_CFLAGS += -Wno-multichar -Werror -Wall -g

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := testHevcEncApi

include $(BUILD_EXECUTABLE)