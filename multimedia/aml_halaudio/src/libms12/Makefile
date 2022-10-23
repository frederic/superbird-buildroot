M_PREFIX=$(AML_HALAUDIO_STAGING_DIR)/usr
LIB_DIR?=$(M_PREFIX)/lib
HEADERS_DIR?=$(M_PREFIX)/include

CC_OBJS= src/aml_audio_ms12.o

CXX_OBJS=src/DolbyMS12.o       \
    src/DolbyMS12ConfigParams.o     \
    src/DolbyMS12Status.o           \
    src/dolby_ms12_config_params.o  \
    src/dolby_ms12.o                \
    src/dolby_ms12_status.o         \

TARGET_OBJS += $(CC_OBJS) $(CXX_OBJS)	
	
TARGET=libhalms12.so

INSTALL_DIR=${LIB_DIR}

LDFLAGS += -shared -ltinyalsa -lamaudioutils

TOPDIR=$(shell pwd)
SRC?=$(TOPDIR)

CFLAGS+=-O2 -fPIC -g
CFLAGS+=-I$(SRC) -I$(SRC)/include -I$(SRC)/../include -I$(SRC)/../utils/ini/include -I$(SRC)/../libms12/include -L${LIB_DIR} -I$(HEADERS_DIR)


#export CC CFLAGS
#export CXX CFLAGS

$(CC_OBJS): %.o: %.c
	$(CC) $(CFLAGS)  -c $< -o $@
$(CXX_OBJS): %.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

all: $(TARGET)

$(TARGET): $(CC_OBJS) $(CXX_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(TARGET_OBJS) -o $@
	
install:
	-install -m 555 ${TARGET} $(INSTALL_DIR)
	-install -m 555 $(TARGET) $(TARGET_DIR)/usr/lib

force:

clean:
	-rm -rf *.o ${TARGET}



