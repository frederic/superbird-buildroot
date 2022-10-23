# object building.
ifeq ($(ARM), 1)
CC=arm-none-linux-gnueabi-gcc
AR=arm-none-linux-gnueabi-ar
else
CC=gcc
AR=ar
endif

TARGET=amlenc
AMLENC_LIB=amlenc.o output.o m8venclib.o
LDFLAGS += -lm -lrt
ifeq ($(ARM), 1)
CFLAGS+=-DARM
else
CFLAGS+=-O2 -std=c99
endif
CFLAGS+= -g -static
$(TARGET):$(AMLENC_LIB)
	$(CC) $(CFLAGS) $(AMLENC_LIB) -o $(TARGET)
#	$(AR) rcs $@ $(AMLENC_LIB)
#	mkdir example
#	cp $(TARGET) ./example
$(AMLENC_LIB):%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	-rm -f *.o
	-rm -f $(TARGET)
