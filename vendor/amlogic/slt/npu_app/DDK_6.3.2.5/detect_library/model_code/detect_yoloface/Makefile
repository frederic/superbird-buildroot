
CROSS_COMPILE=aarch64-linux-gnu-


SDK_LIB=../../build/npu
OVX_DIR=../..
LIB_DIR=$(CC)/libc/lib

INCLUDE += -I$(SDK_LIB)/sdk/inc -I$(SDK_LIB)/applib/ovxinc/include -I$(SDK_LIB)/applib/ovxinc/include/ops  -I./include
CFLAGS += $(INCLUDE)

################################################################################
# Supply necessary libraries.
LIBS += -L$(SDK_LIB)/sharelib/lib32  -l OpenVX -l OpenVXU -l CLC -l VSC -l m -lGAL -l LLVM_viv -l ovxlib
LIBS += -L$(SDK_LIB)/sharelib/lib64  -l OpenVX -l OpenVXU -l CLC -l VSC -l m -lGAL -l LLVM_viv -l ovxlib
LIBS +=-L$(LIB_DIR) -l stdc++  -l m

CFLAGS +=-I. -O2 -fPIE -Wall -pthread $(LIBS)
ODIR=obj
OFILE=libnn_yoloface.so

_OBJ= yoloface.o
_OBJ += yoloface_process.o
_OBJ += vnn_yoloface7d.o
_OBJ += vnn_yoloface88.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(OFILE): $(OBJ)
	#$(CC) -shared -o $(OFILE) ./obj/*.o
	$(CC) $(CFLAGS) -shared -o $(OFILE) ./obj/*.o

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(OFILE)
