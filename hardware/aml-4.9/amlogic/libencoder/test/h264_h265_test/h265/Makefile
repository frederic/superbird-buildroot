ARM=1
# object building.
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

TARGET=libHevcApi.a
#LinkIn=libge2d.a libHevcEnc.a
LDFLAGS += -lm -lrt
ifeq ($(ARM), 1)
CFLAGS+=-DARM
else
CFLAGS+=-O2 -std=c99
endif
CFLAGS+= -g -static

OBJ = test.o test_dma.o libvp_hevc_codec.o AML_HEVCEncoder.o vdi.o \
	  aml_ge2d.o ge2d_port.o ion.o IONmem.o ge2d_feature_test.o

INCLUDE_PATH=.
INCLUDE_PATH+=./hevc_enc/include
INCLUDE_PATH+=./hevc_enc/libge2d

../$(TARGET): $(OBJ)
	$(AR) rcs $@  $(OBJ)
#	-rm -f *.o


test.o: test.cpp
	$(CC) $(CFLAGS) -I$(INCLUDE_PATH) -c $< -o $@

test_dma.o: test_dma.cpp
	$(CC) $(CFLAGS) -I$(INCLUDE_PATH) -c $< -o $@

libvp_hevc_codec.o:./hevc_enc/libvp_hevc_codec.cpp
	$(CC) $(CFLAGS) -I$(INCLUDE_PATH) -c $< -o $@

AML_HEVCEncoder.o:./hevc_enc/AML_HEVCEncoder.cpp
	$(CC) $(CFLAGS) -I$(INCLUDE_PATH) -c $< -o $@

vdi.o:./hevc_enc/vdi.cpp
	$(CC) $(CFLAGS) -I$(INCLUDE_PATH) -c $< -o $@

aml_ge2d.o:./hevc_enc/libge2d/aml_ge2d.c
	$(CC) $(CFLAGS) -I$(INCLUDE_PATH) -c $< -o $@

ge2d_port.o:./hevc_enc/libge2d/ge2d_port.c
	$(CC) $(CFLAGS) -I$(INCLUDE_PATH) -c $< -o $@

ion.o:./hevc_enc/libge2d/ion.c
	$(CC) $(CFLAGS) -I$(INCLUDE_PATH) -c $< -o $@

IONmem.o:./hevc_enc/libge2d/IONmem.c
	$(CC) $(CFLAGS) -I$(INCLUDE_PATH) -c $< -o $@

ge2d_feature_test.o:./hevc_enc/libge2d/ge2d_feature_test.c
	$(CC) $(CFLAGS) -I$(INCLUDE_PATH) -c $< -o $@



clean:
	-rm -f *.o
	-rm -f $(TARGET)
