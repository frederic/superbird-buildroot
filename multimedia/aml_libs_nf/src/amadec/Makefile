M_PREFIX=$(AML_LIBS_NF_STAGING_DIR)/usr

LIB_DIR?=$(M_PREFIX)/lib
HEADERS_DIR?=$(M_PREFIX)/include

TARGET_OBJS=adec-external-ctrl.o adec-internal-mgt.o adec-ffmpeg-mgt.o adec-message.o adec-pts-mgt.o adec_write.o adec_read.o\
           audio_out/alsa-out.o spdif_api.o audio_out/alsa-out-raw.o audio_out/dtv_patch_out.o
TARGET=libamadec_nfx.so

INSTALL_DIR=$(LIB_DIR)/aml_libs_nf

LDFLAGS += -shared -lpthread -lm -lasound -lrt

TOPDIR=$(shell pwd)
SRC?=$(TOPDIR)

CFLAGS+=-O2 -fPIC -g
CFLAGS+=-I$(SRC) -I$(SRC)/include -I$(SRC)/../amavutils/include -I$(SRC)/../amplayer/player/include -I${HEADERS_DIR} -L${LIB_DIR}
CFLAGS+=-DALSA_OUT

ifdef DOLBY_UDC
    CFLAGS+=-DDOLBY_USE_ARMDEC
endif

export CC CFLAGS

all: $(TARGET)

$(TARGET): $(TARGET_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(TARGET_OBJS:%.o=%.c) -o $@

install:
	-install -Dm 644 include/dtv_patch_out.h $(AML_LIBS_NF_STAGING_DIR)/usr/include/aml_libs_nf/amadec/dtv_patch_out.h
	-install -m 555 ${TARGET} $(INSTALL_DIR)

force:

clean:
	-rm -rf *.o ${AMADEC_DIST} ${MP3LIB} $(PCMLIB) $(ADPCMLIB) $(AACLIB) $(A52LIB)
