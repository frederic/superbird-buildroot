M_PREFIX=$(AML_HALAUDIO_STAGING_DIR)/usr
LIB_DIR?=$(M_PREFIX)/lib
HEADERS_DIR?=$(M_PREFIX)/include

CC_OBJS=aml_buffer_provider.o \
	aml_dump_debug.o              \
	aml_audio_resampler.o         \
	aml_ringbuffer.o              \
	aml_alsa_mixer.o              \
	aml_hw_profile.o              \
	aml_android_utils.o           \
	aml_data_utils.o              \
	aml_configs/aml_conf_loader.o \
	aml_configs/aml_conf_parser.o \
	aml_audio_mixer.o             \
	aml_volume_utils.o            \
	ac3_parser_utils.o            \
	str_parms.o                   \
	hashmap.o                     \
	threads.o                     \
	strlcpy.o                     \
	aml_audio_log.o		      \
	amconfigutils.o
	


CXX_OBJS=	ini/ini.o         \
	ini/IniParser.o           \
	spdifenc_wrap.o     \
	SPDIFEncoderAD.o    \
	audio_utils/spdif/AC3FrameScanner.o \
	audio_utils/spdif/BitFieldParser.o  \
	audio_utils/spdif/DTSFrameScanner.o \
	audio_utils/spdif/FrameScanner.o    \
	audio_utils/spdif/SPDIFEncoder.o 
	
TARGET_OBJS += $(CC_OBJS) $(CXX_OBJS)	
	
TARGET=libamaudioutils.so

INSTALL_DIR=${LIB_DIR}

LDFLAGS += -shared -ltinyalsa -lstdc++ -lsupc++ 

TOPDIR=$(shell pwd)
SRC?=$(TOPDIR)

CFLAGS+=-O2 -fPIC -g
CFLAGS+=-I$(SRC) -I$(SRC)/ini/include -I$(SRC)/include -I$(SRC)/../include -I$(SRC)/../tinyalsa/include -L${LIB_DIR} -I$(HEADERS_DIR)


#export CC CFLAGS
#export CPP CPPFLAGS

$(CC_OBJS): %.o: %.c
	$(CC) $(CFLAGS)  -c $< -o $@
$(CXX_OBJS): %.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

all: $(TARGET)

$(TARGET): $(CC_OBJS) $(CXX_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(TARGET_OBJS) -o $@
	
install:
	-install -m 644 ${TARGET} $(INSTALL_DIR)
	-install -m 644 $(TARGET) $(TARGET_DIR)/usr/lib


force:

clean:
	-rm -rf *.o ${TARGET}



