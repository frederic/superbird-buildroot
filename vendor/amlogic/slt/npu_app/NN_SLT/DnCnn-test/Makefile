
CROSS_COMPILE=aarch64-linux-gnu-


SDK_LIB=../../build/npu-1.0
OVX_DIR=../..
LIB_DIR=$(CC)/libc/lib

INCLUDE += -I$(SDK_LIB)/sdk/inc -I$(SDK_LIB)/applib/ovxinc/include -I$(OVX_DIR)/build/libjpeg-9b
CFLAGS += $(INCLUDE)

################################################################################
# Supply necessary libraries.
LIBS += -L$(SDK_LIB)/sharelib/lib32 -l OpenVX -l OpenVXU -l CLC -l VSC -l m -lGAL  -l ovxlib
LIBS += -L$(SDK_LIB)/sharelib/lib64 -l OpenVX -l OpenVXU -l CLC -l VSC -l m -lGAL  -l ovxlib
LIBS += -L$(OVX_DIR)/target/usr/lib -l jpeg
LIBS +=-L$(LIB_DIR) -l stdc++

CFLAGS +=-I. -g -fPIE -Wall -pthread $(LIBS)
ODIR=obj
OFILE=dncnn

_OBJ=main.o
_OBJ += vnn_post_process.o
_OBJ += vnn_dncnn.o
_OBJ += vnn_pre_process.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(OFILE): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -pie

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(OFILE)
