LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=   \
	main.c	    \
	parse.c \
	device.c   \

LOCAL_SHARED_LIBRARIES:=  \
	libcutils \
	libc      \
	liblog	  \

LOCAL_C_INCLUDES += \
    bionic/libc/include \

LOCAL_MODULE:= remotecfg

include $(BUILD_EXECUTABLE)