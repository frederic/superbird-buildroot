
CROSS_COMPILE=aarch64-linux-gnu-


SDK_LIB=../../build/npu
OVX_DIR=../..
LIB_DIR=$(CC)/libc/lib

INCLUDE += -I$(SDK_LIB)/sdk/inc -I$(SDK_LIB)/applib/ovxinc/include -I$(SDK_LIB)/applib/ovxinc/include/ops  -I./include
CFLAGS += $(INCLUDE)

################################################################################
# Supply necessary libraries.
LIBS += -L$(SDK_LIB)/sharelib -l OpenVX -l OpenVXU -l CLC -l VSC -l m -lGAL -l LLVM_viv
LIBS += -L$(SDK_LIB)/sharelib -l ovxlib
LIBS +=-L$(LIB_DIR) -l stdc++  -l m

CFLAGS +=-I. -g -fPIE -Wall -pthread $(LIBS)
ODIR=obj
OFILE=libnn_detect.so

_OBJ= detect.o
_OBJ += detect_log.o
_OBJ += vnn_yoloface7d.o
_OBJ += vnn_yoloface88.o
_OBJ += vnn_yolov27d.o
_OBJ += vnn_yolov288.o
_OBJ += vnn_yolov37d.o
_OBJ += vnn_yolov388.o
_OBJ += vnn_facenet7d.o
_OBJ += vnn_facenet88.o
_OBJ += yoloface_postprocess.o
_OBJ += yoloface_preprocess.o
_OBJ += yolov2_postprocess.o
_OBJ += yolov2_preprocess.o
_OBJ += yolov3_postprocess.o
_OBJ += yolov3_preprocess.o
_OBJ += facenet_process.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(OFILE): $(OBJ)
	#$(CC) -shared -o $(OFILE) ./obj/*.o
	$(CC) $(CFLAGS) -shared -o $(OFILE) ./obj/*.o

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(OFILE)
