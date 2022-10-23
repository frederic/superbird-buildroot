LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= test_main.cpp

LOCAL_SHARED_LIBRARIES := libutils libcutils
LOCAL_STATIC_LIBRARIES :=  libHevcApi libAvcApi
#LOCAL_C_INCLUDES:= $(LOCAL_PATH) \
#	$(LOCAL_PATH)/hevc_enc/include \
#	$(LOCAL_PATH)/hevc_enc/libge2d

LOCAL_CFLAGS += -Wno-multichar -Wno-error -Wall

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := encoder_test

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))
