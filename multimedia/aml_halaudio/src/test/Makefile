PREFIX=$(AML_HALAUDIO_STAGING_DIR)/usr
TARGET_DIR=$(AML_HALAUDIO_TARGET_DIR)/usr
TARGET_FIRWARE_DIR=$(AML_HALAUDIO_TARGET_DIR)
LIB_DIR=$(PREFIX)/lib
BUILD_DIR=$(shell pwd)
INC_DIR=$(AML_HALAUDIO_STAGING_DIR)/usr/include
CROSS=$(CC:%gcc=%)

TARGET_OBJS=test.o

INSTALL_FLAGS=-m 755

TARGET=test
LDFLAGS= -L$(PREFIX)/lib -lpthread -ldl -lasound -lrt -ltinyalsa -lamaudioutils -lhalaudio -lstdc++ -lsupc++ -lspeex_rate

CFLAGS+=-O2 -fPIC -g
CFLAGS+=-I${BUILD_DIR}/ -I${BUILD_DIR}/../include  -L${LIB_DIR} 

export CC CFLAGS

all: ${TARGET}

$(TARGET): $(TARGET_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(TARGET_OBJS:%.o=%.c) -o $@

install:all
	install $(INSTALL_FLAGS) $(TARGET) $(TARGET_DIR)/bin

distclean:clean

clean:
	rm $(TARGET_DIR)/bin/$(TARGET)
	$(RM) -rf test_spdif

