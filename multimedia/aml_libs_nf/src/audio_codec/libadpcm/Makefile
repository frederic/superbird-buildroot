M_PREFIX=$(AML_LIBS_NF_STAGING_DIR)/usr
LIB_DIR?=$(M_PREFIX)/lib
HEADERS_DIR?=$(M_PREFIX)/include

TARGET_OBJS=adpcm_decode.o

TARGET=libadpcm_nfx.so

INSTALL_DIR=${LIB_DIR}/aml_libs_nf

LDFLAGS += -shared 

TOPDIR=$(shell pwd)
SRC?=$(TOPDIR)

CFLAGS+=-O2 -fPIC -g
CFLAGS+=-I$(SRC)  -I$(SRC)/../../amadec/include -I$(SRC)/../../amadec/



export CC CFLAGS

all: $(TARGET) 

$(TARGET): $(TARGET_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(TARGET_OBJS:%.o=%.c) -o $@  

    
    

install:
	-install -m 555 ${TARGET} $(INSTALL_DIR)

force:

clean:
	-rm  $(TARGET) $(TARGET_OBJS)

    








