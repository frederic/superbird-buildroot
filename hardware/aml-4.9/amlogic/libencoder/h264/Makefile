# object building.

TARGET=testApi
AMLENC_LIB=test.o
LDFLAGS += -lm -lrt -Lbjunion_enc -lvpcodec
CFLAGS +=-O2 -g

$(TARGET):$(AMLENC_LIB)
	$(CXX) $(CFLAGS) $(LDFLAGS) $< -o $(TARGET)

$(AMLENC_LIB):%.o:%.cpp
	$(CXX) -c $(CFLAGS) $< -o $@
clean:
	-rm -f *.o
	-rm -f $(TARGET)
