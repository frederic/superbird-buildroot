#export CROSS_CC_PREFIX=aarch64-linux-gnu-
#export CC=$(CROSS_CC_PREFIX)gcc
#export CXX=$(CROSS_CC_PREFIX)g++
#export AR=$(CROSS_CC_PREFIX)ar

TARGET=testApi
AMLENC_LIB=test.o
#LDFLAGS += -lm -lrt -Lbjunion_enc -lvpcodec -Lbjunion_enc/h264bitstream -lh264bitstream
LDFLAGS += -lm -lrt -Lbjunion_enc -lvpcodec -lh264bitstream
CFLAGS +=-O2 -g

$(TARGET):$(AMLENC_LIB)
	$(CXX) $(CFLAGS) $< -o $(TARGET) $(LDFLAGS)

$(AMLENC_LIB):%.o:%.cpp
	$(CXX) -c $(CFLAGS) $< -o $@
clean:
	-rm -f *.o
	-rm -f $(TARGET)
