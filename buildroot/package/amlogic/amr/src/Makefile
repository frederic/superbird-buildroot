LIBS:=libamrnb.a
BINS:=amrnbenc amrnbdec
LIBS+=libamrwb.a
BINS+=amrwbenc amrwbdec

AMRNB_OBJ:=interf_dec.o interf_enc.o sp_dec.o sp_enc.o
AMRNB_FULL_OBJ=$(addprefix amrnb/src/,$(AMRNB_OBJ))
APIS+=$(wildcard amrnb/include/*.h)

AMRWB_OBJ:= dec_acelp.o dec_dtx.o dec_gain.o dec_if.o dec_lpc.o \
	dec_main.o dec_rom.o dec_util.o enc_acelp.o enc_dtx.o \
	enc_gain.o enc_if.o enc_lpc.o enc_main.o enc_rom.o \
	enc_util.o if_rom.o
AMRWB_FULL_OBJ=$(addprefix amrwb/src/,$(AMRWB_OBJ))
APIS+=$(wildcard amrwb/include/*.h)

CFLAGS+=-Iamrnb/include -Iarmnb/src
CFLAGS+=-Iamrwb/include -Iarmwb/src
LDFLAGS+=-lm

all: $(BINS) $(LIBS)

libamrnb.a: $(AMRNB_FULL_OBJ)
	$(AR) -cr $@ $^

libamrwb.a: $(AMRWB_FULL_OBJ)
	$(AR) -cr $@ $^

amrnbenc: libamrnb.a amrnb/bin/encoder.c
	$(CC) $(CFLAGS) $(LDFLAGS) amrnb/bin/encoder.c -lamrnb -o $@

amrnbdec: libamrnb.a amrnb/bin/decoder.c
	$(CC) $(CFLAGS) $(LDFLAGS) amrnb/bin/decoder.c -lamrnb -o $@

amrwbenc: libamrwb.a amrwb/bin/encoder.c
	$(CC) $(CFLAGS) $(LDFLAGS) amrwb/bin/encoder.c -lamrwb -o $@

amrwbdec: libamrwb.a amrwb/bin/decoder.c
	$(CC) $(CFLAGS) $(LDFLAGS) amrwb/bin/decoder.c -lamrwb -o $@

clean:
	rm -rf interf_dec.o interf_enc.o sp_dec.o sp_enc.o libamrnb.a

install:
	mkdir -p $(STAGING_DIR)/usr/include/amr
	-install -m 644 $(APIS) $(STAGING_DIR)/usr/include/amr
	-install -m 644 $(LIBS) $(STAGING_DIR)/usr/lib
	-install -m 744 $(BINS) $(TARGET_DIR)/usr/bin
