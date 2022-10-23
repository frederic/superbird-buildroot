M_PREFIX=$(AML_HALAUDIO_STAGING_DIR)/usr
LIB_DIR?=$(M_PREFIX)/lib
HEADERS_DIR?=$(M_PREFIX)/include

CC_OBJS=audio_hw.o \
	audio_hw_utils.o \
	audio_hwsync.o \
	audio_hw_profile.o \
	aml_hw_mixer.o \
	alsa_manager.o \
	aml_audio_stream.o \
	alsa_config_parameters.o \
	aml_ac3_parser.o \
	aml_dcv_dec_api.o \
	aml_dca_dec_api.o \
	aml_pcm_dec_api.o \
	alsa_device_parser.o \
	audio_post_process.o \
	aml_avsync_tuning.o \
	audio_format_parse.o \
	dolby_lib_api.o \
	audio_route.o \
	spdif_encoder_api.o \
	aml_output_manager.o \
	aml_input_manager.o \
	aml_callback_api.o \
	aml_dec_api.o \
	aml_datmos_api.o \
	aml_sample_conv.o \
	aml_mat_parser.o \
	aml_spdif_in.o \
	aml_channel_map.o \
	aml_audio_volume.o \
	aml_bm_api.o \
	aml_audio_level.o \
	aml_audio_ease.o \
	aml_audio_delay.o \
	standard_alsa_manager.o \
	aml_config_parser.o \
	audio_trace.o \
	audio_speex_resample_api.o \
	audio_simple_resample_api.o \
	aml_audio_resample_manager.o




ifeq ($(ENABLE_PULSEAUDIO),yes)
CC_OBJS += pulse_manager.o
endif


CXX_OBJS=audio_eq_drc_compensation.o \
	audio_eq_drc_parser.o \
	aml_datmos_config.o \


TARGET_OBJS += $(CC_OBJS) $(CXX_OBJS)

TARGET=libhalaudio.so

INSTALL_DIR=${LIB_DIR}

LDFLAGS += -shared
LDLIBS += -ltinyalsa -lamaudioutils -lstdc++ -lsupc++ -lcjson -lspeex_rate

ifeq ($(ENABLE_PULSEAUDIO),yes)
LDLIBS += -lpulse-simple
CFLAGS  += -DUSE_PULSE_AUDIO
endif

ifeq ($(ENABLE_AUDIOSERVICE_S400),yes)
CFLAGS  += -DUSE_AUDIOSERVICE_S400
endif

ifeq ($(ENABLE_AUDIOSERVICE_S400_SBR),yes)
CFLAGS  += -DUSE_AUDIOSERVICE_S400_SBR
endif

ifeq ($(ENABLE_AUDIOSERVICE_S410_SBR),yes)
CFLAGS  += -DUSE_AUDIOSERVICE_S410_SBR
endif

ifeq ($(ENABLE_ALSA_PLUGINS),yes)
CFLAGS  += -DUSE_ALSA_PLUGINS
endif

TOPDIR=$(shell pwd)
SRC?=$(TOPDIR)

CFLAGS+=-O2 -fPIC -g
CFLAGS+=-I$(SRC) -I$(SRC)/include -I$(SRC)/../include -I$(SRC)/../utils/ini/include -I$(SRC)/../utils/include  -I$(SRC)/../tinyalsa/include -I$(SRC)/../libms12/include -I$(SRC)/../speex_rate -L${LIB_DIR} -I$(HEADERS_DIR)

#marco of Dolby Atmos
DEFINES = -DDATMOS -DMAX_OUTPUT_CH=8 -DBYTES_PER_SAMPLE=4 -DOUTSIDE_SPEEX -DRANDOM_PREFIX=AML_AUDIO
CFLAGS += $(DEFINES)


ifeq ($(ENABLE_AUDIO_64bit),yes)
DEP_PROJ_LINK = $(SRC)/lib_bassmanagement64.a
else
DEP_PROJ_LINK = $(SRC)/lib_bassmanagement.a
endif

EXPORT_API = $(SRC)/../include/audio_base.h \
	$(SRC)/../include/audio_core.h \
	$(SRC)/../include/audio_effect_hal.h \
	$(SRC)/../include/audio_effect.h \
	$(SRC)/../include/audio_effect-base.h \
	$(SRC)/../include/audio_hal.h \
	$(SRC)/../include/hardware.h \
	$(SRC)/../include/bitops.h

#export CC CFLAGS
#export CXX CFLAGS

$(CC_OBJS): %.o: %.c
	$(CC) $(CFLAGS)  -c $< -o $@
$(CXX_OBJS): %.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

all: $(TARGET)

$(TARGET): $(TARGET_OBJS) $(DEP_PROJ_LINK)
	$(CC) $(LDFLAGS) -o $@ $+ $(LDLIBS)

install:
	-install -m 644 ${TARGET} $(INSTALL_DIR)
	-install -m 644 $(TARGET) $(TARGET_DIR)/usr/lib
	-install -m 644 $(EXPORT_API) $(HEADERS_DIR)/
	
force:

clean:
	-rm -rf *.o ${TARGET}



