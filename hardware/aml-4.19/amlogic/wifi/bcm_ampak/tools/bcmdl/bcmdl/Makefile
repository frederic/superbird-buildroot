CC = $(CROSS_COMPILE)gcc

IFLAGS := -g -Wstrict-prototypes -o2 -Wall -Iinclude -Ishared/zlib
DFLAGS := -DBCMTRXV2
#IFLAGS += -DIL_BIGENDIAN

obj-bcmdl = bcmdl.o usb_linux.o \
          shared/bcmutils.o \
          shared/zlib/adler32.o \
          shared/zlib/inffast.o \
          shared/zlib/inflate.o \
          shared/zlib/infcodes.o \
          shared/zlib/infblock.o \
          shared/zlib/inftrees.o \
          shared/zlib/infutil.o \
          shared/zlib/zutil.o \
          shared/zlib/crc32.o

BCMDL = bcmdl
LIBLINK = -lusb
ALL = all

CFLAGS += $(IFLAGS) $(DFLAGS)

all: $(ALL) 

$(ALL):
	make clean
	make $(BCMDL)

$(BCMDL): $(obj-bcmdl)
	$(CC) -o $@ $(obj-bcmdl) $(LIBLINK)

#$(LIBWL): $(obj-wl)
#	ar rscv $(LIBWL) $(obj-wl) $(LIBLINK)
#	$(CC) $(obj-wl) -shared -o $(LIBWL) $(LIBLINK)

# for sequance
#	$(CC) -Wl,-elf2flt -o $@ $(obj-wl)

clean:
	-rm -f $(BCMDL) *.o shared/*.o shared/zlib/*.o
