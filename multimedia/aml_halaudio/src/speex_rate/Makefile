M_PREFIX=$(AML_HALAUDIO_STAGING_DIR)/usr
LIB_DIR?=$(M_PREFIX)/lib
HEADERS_DIR?=$(M_PREFIX)/include

TARGET_OBJS=resample.o

TARGET=libspeex_rate.so

INSTALL_DIR=${LIB_DIR}

LDFLAGS += -shared

TOPDIR=$(shell pwd)
SRC?=$(TOPDIR)

CFLAGS+=-O2 -fPIC -g
CFLAGS+=-I$(SRC) -I$(SRC)/include -L${LIB_DIR} -I$(HEADERS_DIR) -DOUTSIDE_SPEEX -DRANDOM_PREFIX=AML_AUDIO -DFIXED_POINT



export CC CFLAGS

all: $(TARGET)

$(TARGET): $(TARGET_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(TARGET_OBJS:%.o=%.c) -o $@


install:
	-install -m 644 ${TARGET} $(INSTALL_DIR)
	-install -m 644 $(TARGET) $(TARGET_DIR)/usr/lib

force:

clean:
	-rm -rf *.o ${TARGET}

