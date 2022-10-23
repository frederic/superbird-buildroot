PREFIX=$(LIBPLAYER_STAGING_DIR)/usr
TARGET_DIR=$(LIBPLAYER_TARGET_DIR)/usr
TARGET_FIRWARE_DIR=$(LIBPLAYER_TARGET_DIR)
LIB_DIR=$(PREFIX)/bin
BUILD_DIR=$(shell pwd)
INC_DIR=$(LIBPLAYER_STAGING_DIR)/usr/include
CROSS=$(CC:%gcc=%)

spdif_in_dd_decoder_s=spdif_in_dd_decoder.c

INSTALL_FLAGS=-m 755

TARGET=spdif_in_dd_decoder
LDFLAGS= -L$(PREFIX)/lib/libplayer -lavutil -lavformat -lavcodec -lm -lz -lpthread -lamcodec -ldl -lamplayer -lamadec -lasound -lamstreaming  -lamavutils -lrt -lstdc++
CFLAGS+= -I${BUILD_DIR}/../amffmpeg -I${BUILD_DIR}/../amcodec/include -I${BUILD_DIR}/../amadec/include -I${BUILD_DIR}/../../../libplayer/src/amavutils/include

CFLAGS+= -I${BUILD_DIR}/../amplayer/player/include -I${BUILD_DIR}/../amplayer/player -I${BUILD_DIR}/../streamsource
CFLAGS+= -I$(INC_DIR)
CFLAGS+=-O2 -g


ifeq ($(BR2_TARGET_BOARD_PLATFORM),"meson6")
    FIRMWARE_TYPE=firmware-m6
else ifeq ($(BR2_TARGET_BOARD_PLATFORM),"meson8")
    FIRMWARE_TYPE=firmware-m8
else
    FIRMWARE_TYPE=firmware
endif

.PHONY: all config  before_cmd install clean distclean

all: ${TARGET}
	mkdir -p $(TARGET_DIR)/../lib/firmware
install:all
	install $(INSTALL_FLAGS) $(TARGET) $(TARGET_DIR)/bin

distclean:clean

clean:
	rm $(TARGET_DIR)/bin/$(TARGET)
	$(RM) -rf spdif_in_dd_decoder

${TARGET}:spdif_in_dd_decoder.o
	$(CC) $(CFLAGS) $(LDFLAGS) $($(@:%=%_s)) -o $@
