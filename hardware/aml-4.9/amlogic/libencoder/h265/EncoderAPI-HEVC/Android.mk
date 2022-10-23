LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_SRC_FILES:= test.cpp \
	hevc_enc/libvp_hevc_codec.cpp \
	hevc_enc/AML_HEVCEncoder.cpp \
	hevc_enc/vdi.cpp

LOCAL_SHARED_LIBRARIES := libutils libcutils libge2d
#LOCAL_STATIC_LIBRARIES := libge2d

LOCAL_C_INCLUDES:= \
	hevc_enc/include \
	$(TOP)/vendor/amlogic/common/system/libge2d/include
	

LOCAL_CFLAGS += -Wno-multichar -Werror -Wall -g

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := testHevcEncApi

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))
