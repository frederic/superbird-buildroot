 # Copyright (C) 2009 The Android Open Source Project
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
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=                     \
    src/DolbyMS12.cpp                 \
    src/DolbyMS12ConfigParams.cpp     \
    src/DolbyMS12Status.cpp           \
    src/dolby_ms12_config_params.cpp  \
    src/dolby_ms12.cpp                \
    src/dolby_ms12_status.cpp         \
    src/aml_audio_ms12.c


LOCAL_C_INCLUDES := \
    system/media/audio/include \
    hardware/libhardware/include \
    $(LOCAL_PATH)/include/ \
    $(call include-path-for, audio-utils)


LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libdl \
    liblog

LOCAL_MODULE := libms12api

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
    LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_CFLAGS := -Werror -Wall
LOCAL_MODULE_TAGS := optional
# uncomment to disable NEON on architectures that actually do support NEON, for benchmarking
#LOCAL_CFLAGS += -DUSE_NEON=false
LOCAL_CFLAGS += -DDOLBY_MS12_ENABLE
LOCAL_CFLAGS += -DREPLACE_OUTPUT_BUFFER_WITH_CALLBACK

include $(BUILD_SHARED_LIBRARY)
