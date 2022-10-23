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

ifeq ($(strip $(BOARD_ALSA_AUDIO)),tiny)

    LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libnano
LOCAL_SRC_FILES_arm := ../bt_voice/nano/32/libnano.so
LOCAL_SRC_FILES_arm64 := ../bt_voice/nano/64/libnano.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := nanosic
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_TARGET_ARCH:= arm arm64
LOCAL_MULTILIB := both
include $(BUILD_PREBUILT)

# The default audio HAL module, which is a stub, that is loaded if no other
# device specific modules are present. The exact load order can be seen in
# libhardware/hardware.c
#
# The format of the name is audio.<type>.<hardware/etc>.so where the only
# required type is 'primary'. Other possibilites are 'a2dp', 'usb', etc.
	include $(CLEAR_VARS)

    LOCAL_MODULE := audio.primary.amlogic
    ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
        LOCAL_PROPRIETARY_MODULE := true
    endif
    LOCAL_MODULE_RELATIVE_PATH := hw
    LOCAL_SRC_FILES := \
        audio_hw.c \
        aml_audio_delay.c \
        audio_hw_utils.c \
        audio_hwsync.c \
        audio_hw_profile.c \
        aml_hw_mixer.c \
        alsa_manager.c \
        audio_hw_ms12.c \
        audio_hw_dtv.c \
        aml_audio_stream.c \
        alsa_config_parameters.c \
        spdif_encoder_api.c \
        aml_ac3_parser.c \
        aml_dcv_dec_api.c \
        aml_dca_dec_api.c \
        audio_post_process.c \
        aml_avsync_tuning.c \
        audio_format_parse.c \
        dolby_lib_api.c \
        amlAudioMixer.c \
        hw_avsync.c \
        hw_avsync_callbacks.c \
        audio_port.c \
        sub_mixing_factory.c \
        audio_data_process.c \
        ../../../../frameworks/av/media/libaudioprocessing/AudioResampler.cpp.arm \
        ../../../../frameworks/av/media/libaudioprocessing/AudioResamplerCubic.cpp.arm \
        ../../../../frameworks/av/media/libaudioprocessing/AudioResamplerSinc.cpp.arm \
        ../../../../frameworks/av/media/libaudioprocessing/AudioResamplerDyn.cpp.arm \
        aml_resample_wrap.cpp \
        audio_simple_resample_api.c \
        aml_audio_resample_manager.c \
        audio_dtv_ad.c \
        audio_android_resample_api.c \
        aml_audio_timer.c \
        audio_virtual_buf.c \
        aml_audio_ease.c \
        ../amlogic_AQ_tools/audio_eq_drc_compensation.c \
        ../amlogic_AQ_tools/audio_eq_drc_parser.c \
        ../amlogic_AQ_tools/ini/dictionary.c \
        ../amlogic_AQ_tools/ini/iniparser.c

    LOCAL_C_INCLUDES += \
        external/tinyalsa/include \
        system/media/audio_utils/include \
        system/media/audio_effects/include \
        system/media/audio_route/include \
        system/core/include \
        hardware/libhardware/include \
        $(LOCAL_PATH)/../libms12/include \
        hardmare/amlogic/audio/libms12/include \
        $(LOCAL_PATH)/../utils/include \
        $(LOCAL_PATH)/../utils/ini/include \
        $(LOCAL_PATH)/../rcaudio \
        $(LOCAL_PATH)/../../LibAudio/amadec/include \
        $(LOCAL_PATH)/../bt_voice/kehwin \
        frameworks/native/include \
        vendor/amlogic/common/external/dvb/include/am_adp \
        frameworks/av/include \
        $(LOCAL_PATH)/../amlogic_AQ_tools \
        $(LOCAL_PATH)/../amlogic_AQ_tools/ini

    LOCAL_LDFLAGS_arm += $(LOCAL_PATH)/../amlogic_AQ_tools/lib_aml_ng.a
    LOCAL_LDFLAGS_arm += $(LOCAL_PATH)/../amlogic_AQ_tools/Amlogic_EQ_Param_Generator.a
    LOCAL_LDFLAGS_arm += $(LOCAL_PATH)/../amlogic_AQ_tools/Amlogic_DRC_Param_Generator.a
    LOCAL_LDFLAGS_arm64 += $(LOCAL_PATH)/../amlogic_AQ_tools/Amlogic_EQ_Param_Generator64.a
    LOCAL_LDFLAGS_arm64 += $(LOCAL_PATH)/../amlogic_AQ_tools/Amlogic_DRC_Param_Generator64.a
    LOCAL_LDFLAGS_arm += $(LOCAL_PATH)/../bt_voice/kehwin/32/btmic.a
    LOCAL_LDFLAGS_arm64 += $(LOCAL_PATH)/../bt_voice/kehwin/64/btmic.a

    LOCAL_SHARED_LIBRARIES := \
        liblog libcutils libtinyalsa \
        libaudioutils libdl libaudioroute libutils \
        libdroidaudiospdif libamaudioutils libamlaudiorc libamadec \
        libnano

ifeq ($(BOARD_COMPILE_IN_SYSTEM), true)
    LOCAL_SHARED_LIBRARIES += libam_adp_vendor
else
    LOCAL_SHARED_LIBRARIES += libam_adp
endif

ifeq ($(BOARD_ENABLE_NANO), true)
    LOCAL_SHARED_LIBRARIES += libnano
endif

    LOCAL_MODULE_TAGS := optional

    LOCAL_CFLAGS += -Werror
ifneq ($(TARGET_BUILD_VARIANT),user)
    LOCAL_CFLAGS += -DDEBUG_VOLUME_CONTROL
endif
ifeq ($(BOARD_ENABLE_NANO), true)
	LOCAL_CFLAGS += -DENABLE_NANO_PATCH=1
endif

ifeq ($(strip $(TARGET_WITH_TV_AUDIO_MODE)),true)
$(info "---------tv audio mode, compiler configured 8 channels output by default--------")
LOCAL_CFLAGS += -DTV_AUDIO_OUTPUT
else
$(info "---------ott audio mode, compiler configure 2 channels output by default--------")
LOCAL_CFLAGS += -DSUBMIXER_V1_1
endif
    #LOCAL_CFLAGS += -Wall -Wunknown-pragmas

#add dolby ms12support
    LOCAL_SHARED_LIBRARIES += libms12api
    LOCAL_CFLAGS += -DDOLBY_MS12_ENABLE
    LOCAL_CFLAGS += -DREPLACE_OUTPUT_BUFFER_WITH_CALLBACK
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libms12/include

#For atom project
ifeq ($(strip $(TARGET_BOOTLOADER_BOARD_NAME)), atom)
    LOCAL_CFLAGS += -DIS_ATOM_PROJECT
    LOCAL_SRC_FILES += \
        audio_aec_process.cpp
    LOCAL_C_INCLUDES += \
        $(TOPDIR)vendor/harman/atom/google_aec \
        $(TOPDIR)vendor/harman/atom/harman_api
    LOCAL_SHARED_LIBRARIES += \
        libgoogle_aec libharman_api
endif

#For ATV Far Field AEC
ifeq ($(BOARD_ENABLE_FAR_FIELD_AEC), true)
    $(info "audio: ATV far field enabled, compile and link aec lib")
	LOCAL_CFLAGS += -DENABLE_AEC_FUNC
    LOCAL_SRC_FILES += \
        audio_aec_process.cpp
    LOCAL_C_INCLUDES += \
         $(TOPDIR)vendor/google/google_aec
    LOCAL_SHARED_LIBRARIES += \
         libgoogle_aec
endif

    include $(BUILD_SHARED_LIBRARY)

endif # BOARD_ALSA_AUDIO

#########################################################
# Audio Policy Manager
ifeq ($(USE_CUSTOM_AUDIO_POLICY),1)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    DLGAudioPolicyManager.cpp

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libutils \
    libmedia \
    libbinder \
    libaudiopolicymanagerdefault \
    libutils \
    libaudioclient \
    libmedia_helper

LOCAL_C_INCLUDES := \
    external/tinyalsa/include \
    hardware/libhardware/include \
    $(TOPDIR)frameworks/av/services/audiopolicy \
    $(TOPDIR)frameworks/av/services/audiopolicy/managerdefault \
    $(TOPDIR)frameworks/av/services/audiopolicy/engine/interface \
    $(TOPDIR)frameworks/av/services/audiopolicy/common/managerdefinitions/include \
    $(TOPDIR)frameworks/av/services/audiopolicy/common/include \
    $(TOPDIR)frameworks/av/media/libaudioclient/include

LOCAL_MODULE := libaudiopolicymanager
LOCAL_MODULE_TAGS := optional
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
    LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
endif # USE_CUSTOM_AUDIO_POLICY

include $(call all-makefiles-under,$(LOCAL_PATH))
