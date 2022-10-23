CC=aarch64-linux-gnu-gcc

CFLAG=

SRCFILE=c1_encoder_test.c
TARGET=c1_encoder_test
OBJ=c1_encoder_test.o

RAWDATAYUV=../yuv/nv21_480p.yuv
AMLAPP=./bin/aml_enc_test
AMLLIB=./lib/libamvenc_api.so ./lib/libvpcodec.so

TARGET_DIR=$(LIBPLAYER_TARGET_DIR)/usr

LDFLAGS += -lm -lrt
INSTALL_FLAGS= -m 755


all: $(TARGET)


$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ)  -o $(TARGET)

$(OBJ): $(SRCFILE)
	$(CC) -c $(CFLAGS) $<  -o $@


install:$(TARGET)
	install $(INSTALL_FLAGS) $(TARGET) $(TARGET_DIR)/bin
	install $(INSTALL_FLAGS) $(RAWDATAYUV) $(TARGET_DIR)/bin
	install $(INSTALL_FLAGS) $(AMLAPP) $(TARGET_DIR)/bin
	install $(INSTALL_FLAGS) $(AMLLIB) $(TARGET_DIR)/lib


.PHONY:clean
clean:
	-rm -f *.o
	-rm -f $(TARGET)





