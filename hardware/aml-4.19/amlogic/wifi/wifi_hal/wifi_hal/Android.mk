# Copyright (C) 2011 The Android Open Source Project
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
LOCAL_PATH := $(call my-dir)

# Make the HAL library
# ============================================================
include $(CLEAR_VARS)

LOCAL_CFLAGS := \
    -Wall \
    -Werror \
    -Wno-format \
    -Wno-reorder \
    -Wno-unused-function \
    -Wno-unused-parameter \
    -Wno-unused-private-field \
    -Wno-unused-variable \

ifeq ($(BOARD_WIFI_VENDOR), realtek)
LOCAL_CFLAGS += -DREALTEK_WIFI_SUPPORT
endif
ifeq ($(BOARD_WIFI_VENDOR), mtk)
LOCAL_CFLAGS += -DMTK_WIFI_SUPPORT
endif
ifeq ($(BOARD_WIFI_VENDOR), qualcomm)
LOCAL_CFLAGS += -DQCA_WIFI_SUPPORT
endif
LOCAL_C_INCLUDES += \
	external/libnl/include \
	$(call include-path-for, libhardware_legacy)/hardware_legacy \
	system/core/libutils/include \
	system/core/liblog/include \
	external/wpa_supplicant_8/src/drivers

LOCAL_SRC_FILES := \
	wifi_hal.cpp \
	rtt.cpp \
	common.cpp \
	cpp_bindings.cpp \
	gscan.cpp \
	link_layer_stats.cpp \
	wifi_logger.cpp \
	wifi_offload.cpp

LOCAL_MODULE := libwifi-hal-mt66xx
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_STATIC_LIBRARY)
