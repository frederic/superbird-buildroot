LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES +=             \
    $(LOCAL_PATH)/../include    \
    $(LOCAL_PATH)/include       \
    $(LOCAL_PATH)/../tinyalsa/include   \
    $(LOCAL_PATH)/ini/include


LOCAL_SRC_FILES  +=               \
    aml_buffer_provider.c         \
	aml_dump_debug.c              \
	aml_audio_resampler.c         \
	aml_ringbuffer.c              \
	aml_alsa_mixer.c              \
	aml_hw_profile.c              \
	aml_android_utils.c           \
	aml_data_utils.c              \
	aml_configs/aml_conf_loader.c \
	aml_configs/aml_conf_parser.c \
	aml_audio_mixer.c             \
	aml_volume_utils.c            \
	ac3_parser_utils.c            \
	str_parms.c                   \
	hashmap.c                     \
	threads.c                     \
	strlcpy.c                     \
	aml_audio_log.c               \
	amconfigutils.c               \
	ini/ini.cpp         \
	ini/IniParser.cpp           \
	spdifenc_wrap.cpp     \
	SPDIFEncoderAD.cpp    \
	audio_utils/spdif/AC3FrameScanner.cpp \
	audio_utils/spdif/BitFieldParser.cpp  \
	audio_utils/spdif/DTSFrameScanner.cpp \
	audio_utils/spdif/FrameScanner.cpp    \
	audio_utils/spdif/SPDIFEncoder.cpp
LOCAL_MODULE := libamaudioutils

LOCAL_SHARED_LIBRARIES += \
    liblog libtinyalsa

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -DBUILD_IN_ANDROID

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
