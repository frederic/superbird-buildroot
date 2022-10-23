ARM=1

ifeq ($(ARM), 1)
CC=arm-none-linux-gnueabi-g++
AR=arm-none-linux-gnueabi-ar
else
	ifeq ($(ARM), 2)
	CC=aarch64-linux-gnu-g++
	AR=aarch64-linux-gnu-ar
else
	CC=gcc
	AR=ar
endif
endif


TARGET=encoder_test

RAWDATAYUV=nv21_480p.yuv
LinkIn=libAvcApi.a libHevcApi.a
OBJ=test_main.o

TARGET_DIR=$(LIBPLAYER_TARGET_DIR)/usr

SUBDIRS=h264 h265

LDFLAGS += -lm -lrt

INSTALL_FLAGS= -m 755

ifeq ($(ARM), 1)
	CFLAGS+=-DARM
else
	CFLAGS+=-O2 -std=c99
endif

CFLAGS+= -g -static

all: $(TARGET)

$(SUBDIRS): ECHO
	make -C $@

ECHO:
	@echo $(SUBDIRS)

$(TARGET):$(SUBDIRS) $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LinkIn) -o $(TARGET)

install:$(TARGET)
	install $(INSTALL_FLAGS) $(TARGET) $(TARGET_DIR)/bin
	cp $(RAWDATAYUV) $(TARGET_DIR)/bin

$(OBJ): test_main.c
	$(CC) -c $(CFLAGS) $<  -o $@


.PHONY:clean
clean:
	-rm -f *.o
	-rm -f $(TARGET)
	-rm -f $(LinkIn)
	$(foreach i, $(SUBDIRS), make clean -C $i;)




