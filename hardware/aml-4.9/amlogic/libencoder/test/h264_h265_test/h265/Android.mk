LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= test.cpp \
	test_dma.cpp \
	hevc_enc/libvp_hevc_codec.cpp \
	hevc_enc/AML_HEVCEncoder.cpp \
	hevc_enc/vdi.cpp \
	hevc_enc/libge2d/aml_ge2d.c \
	hevc_enc/libge2d/ge2d_port.c \
	hevc_enc/libge2d/ion.c \
	hevc_enc/libge2d/IONmem.c \
	hevc_enc/libge2d/ge2d_feature_test.c


LOCAL_SHARED_LIBRARIES :=  libutils libcutils

LOCAL_C_INCLUDES:= $(LOCAL_PATH) \
	$(LOCAL_PATH)/hevc_enc/include \
	$(LOCAL_PATH)/hevc_enc/libge2d

LOCAL_CFLAGS += -Wno-multichar -Wno-error -Wall -g

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libHevcApi

include $(BUILD_STATIC_LIBRARY)