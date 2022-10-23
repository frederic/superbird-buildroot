TARGET=testHevcApi
AMLENC_LIB=test.o
LDFLAGS += -lm -lrt -lge2d -Lhevc_enc -lvphevcodec
CFLAGS+=-O2 -g

$(TARGET):$(AMLENC_LIB)
	$(CXX) $(CFLAGS) $(LDFLAGS) $< -o $(TARGET) $(LinkIn)

$(AMLENC_LIB):%.o:%.cpp
	$(CXX) -c $(CFLAGS) $< -o $@
clean:
	-rm -f *.o
	-rm -f $(TARGET)
