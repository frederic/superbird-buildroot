# Copyright (C) 2014 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

##################################
# Audio bluetooth RC HAL
##################################

LOCAL_PATH := $(call my-dir)

# The default audio HAL module, which is a stub, that is loaded if no other
# device specific modules are present. The exact load order can be seen in
# libhardware/hardware.c
#
# The format of the name is audio.<type>.<hardware/etc>.so where the only
# required type is 'primary'. Other possibilites are 'a2dp', 'usb', etc.


include $(CLEAR_VARS)

LOCAL_MODULE := libamlaudiorc
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
    LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_SRC_FILES := \
    audio_hal_thunks.cpp \
    AudioHardwareInput.cpp \
    AudioStreamIn.cpp \
    AudioHotplugThread.cpp \
    hidraw/AudioHidrawStreamIn.cpp \
    soundcard/AudioSoundCardStreamIn.cpp

LOCAL_C_INCLUDES += \
        hardware/libhardware/include \
        hardware/libhardware_legacy/include \
        external/tinyalsa/include \
        frameworks/av/media/libaudioclient/include \
        $(call include-path-for, audio-utils) \
        $(LOCAL_PATH)/hidraw \
        $(LOCAL_PATH)/soundcard \
        $(LOCAL_PATH)/../utils/include

LOCAL_STATIC_LIBRARIES += libmedia_helper

LOCAL_LDFLAGS_arm += $(LOCAL_PATH)/hidraw/audio.bt.remote-arm.a
LOCAL_LDFLAGS_arm64 += $(LOCAL_PATH)/hidraw/audio.bt.remote-arm64.a

LOCAL_SHARED_LIBRARIES := \
    vendor.amlogic.hardware.remotecontrol@1.0 \
    libhidlbase \
    libcutils \
    liblog \
    libutils \
    libtinyalsa \
    libaudioutils


LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_CLASS := STATIC_LIBRARIES

LOCAL_CFLAGS += -Wno-error=date-time

#include $(BUILD_STATIC_LIBRARY)
include $(BUILD_SHARED_LIBRARY)

