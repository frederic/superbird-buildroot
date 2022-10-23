#
# Copyright (C) 2008 The Android Open Source Project
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
ifeq ($(MULTI_WIFI_SUPPORT),true)
LOCAL_PATH := $(call my-dir)

L_CFLAGS += -DSYSFS_PATH_MAX=256 -DWIFI_DRIVER_MODULE_PATH=\"\/system\/lib\"

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := lib_driver_load
LOCAL_SHARED_LIBRARIES := libc libcutils
LOCAL_CFLAGS := $(L_CFLAGS)
LOCAL_SRC_FILES := driver_load_rtl8192cu.c \
					driver_load_rtl8192du.c \
					driver_load_rtl8188eu.c \
					driver_load_rtl8188ftv.c \
					driver_load_rtl8821au.c \
					driver_load_rtl8812au.c \
					driver_load_rtl8822bu.c \
					driver_load_mt7601.c \
					driver_load_mt7603.c \
					driver_load_mt7662.c \
					driver_load_mt7668.c \
					driver_load_rtl8192eu.c \
					driver_load_rtl8192es.c \
					driver_load_rtl8723bu.c \
					driver_load_rtl8723du.c \
					driver_load_rtl8723ds.c \
					driver_load_qca9377.c \
					driver_load_qca9379.c \
					driver_load_qca6174.c \
					driver_load_bcm43569.c \
					driver_load_rda5995.c
include $(BUILD_SHARED_LIBRARY)
endif
