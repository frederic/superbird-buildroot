ARM=1
# 指令编译器和选项
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

ifeq ($(ARM), 1)
CFLAGS+=-DARM
else
CFLAGS+=-O2 -std=c99
endif
CFLAGS+= -g -static
# 目标文件
TARGET = ../libHevcEnc.a
LDFLAGS += -lm -lrt

INC = -I./include -I. -I./libge2d

../libHevcEnc.a:libvp_hevc_codec.o AML_HEVCEncoder.o vdi.o
	$(AR) rcs $@ $^

libvp_hevc_codec.o: libvp_hevc_codec.cpp
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

AML_HEVCEncoder.o: AML_HEVCEncoder.cpp
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

vdi.o: vdi.cpp
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
