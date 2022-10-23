ARM=1
# object building.
ifeq ($(ARM), 1)
CC=arm-none-linux-gnueabi-g++
AR=arm-none-linux-gnueabi-ar
else
CC=gcc
AR=ar
endif

TARGET=libAvcApi.a
#LinkIn=libEnc.a
LDFLAGS += -lm -lrt

ifeq ($(ARM), 1)
CFLAGS+=-DARM
else
CFLAGS+=-O2 -std=c99
endif
CFLAGS+= -g -static

INC_PATH= -I. -I./bjunion_enc -I./bjunion_enc/include -I./bjunion_enc/enc/gx_enc_fast
$(info $(INC_PATH))

#vpath %.cpp bjunion_enc:bjunion_enc/enc/gx_enc_fast/

OBJ = test.o test_dma.o libvpcodec.o AML_HWEncoder.o \
	  enc_api.o gxvenclib_fast.o rate_control_gx_fast.o \
      parser.o

../$(TARGET): $(OBJ)
	$(AR) rcs $@ $^
#	-rm -f *.o


#$(OBJ):%.o:%.cpp
#	$(CC) $(CFLAGS) -c $< -o $@ $(INC_PATH)

test.o:test.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(INC_PATH)
test_dma.o:test_dma.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(INC_PATH)
libvpcodec.o:./bjunion_enc/libvpcodec.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(INC_PATH)
AML_HWEncoder.o:./bjunion_enc/AML_HWEncoder.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(INC_PATH)
enc_api.o:./bjunion_enc/enc_api.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(INC_PATH)
gxvenclib_fast.o:./bjunion_enc/enc/gx_enc_fast/gxvenclib_fast.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(INC_PATH)
rate_control_gx_fast.o:./bjunion_enc/enc/gx_enc_fast/rate_control_gx_fast.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(INC_PATH)
parser.o:./bjunion_enc/enc/gx_enc_fast/parser.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(INC_PATH)


.PHONY: clean
clean:
	-rm -f *.o *.a
