#
#Copyright (c) 2019 Amlogic, Inc. All rights reserved.
#
#This source code is subject to the terms and conditions defined in below
#which is part of this source code package.
#
#Description:
#
#
# Copyright (C) 2019 Amlogic, Inc. All rights reserved.
#
# All information contained herein is Amlogic confidential.
#
# This software is provided to you pursuant to Software License
# Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
# used only in accordance with the terms of this agreement.
#
# Redistribution and use in source and binary forms, with or without
# modification is strictly prohibited without prior written permission
# from Amlogic.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_LDLIBS := -lm -llog

LOCAL_SRC_FILES := \
	vdi.c \
	vdi_osal.c \
	debug.c

LOCAL_SRC_FILES += \
	vpuapi.c \
	product.c \
	enc_driver.c \
	vpuapifunc.c

#define MAKEANDROID

LOCAL_SHARED_LIBRARIES += libcutils \
			libutils \
			libion \
			libdl \
			liblog

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
		$(LOCAL_PATH)/include


LOCAL_ARM_MODE := arm
LOCAL_MODULE:= libamvenc_api
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)
