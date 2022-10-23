LOCAL_PATH:= $(call my-dir)

common_src_files := main.c vnn_pre_process.c vnn_post_process.c vnn_dncnn.c

common_c_includes := \
	vendor/amlogic/common/npu/sdk/inc \
	vendor/amlogic/common/npu/service/ovx_inc \
	vendor/amlogic/common/npu/applib/ovxinc/include \



common_shared_libraries := \
	libsysutils \
	libbinder \
	libcutils \
	liblog \
	libdl \
	libovxlib \
	libjpeg


include $(CLEAR_VARS)

VIV_TARGET_ABI ?= $(TARGET_ARCH)
ifneq ($(findstring 64,$(VIV_TARGET_ABI)),)
    VIV_MULTILIB=64
else
    VIV_MULTILIB=32
endif

LOCAL_SRC_FILES := $(common_src_files)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := $(common_shared_libraries)



ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := dncnn
LOCAL_MODULE_TAGS := eng debug optional
LOCAL_ARM_MODE := $(VIV_TARGET_ABI)

LOCAL_MULTILIB := $(VIV_MULTILIB)


include $(BUILD_EXECUTABLE)
