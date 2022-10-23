CC=/opt/gcc-linaro-6.3.1-2017.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc
CROSS_COMPILE=/opt/gcc-linaro-6.3.1-2017.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-

CFLAGS=-I. -g -fPIE -Wall -pthread
ODIR=obj
OFILE=v4l2_test

_OBJ = v4l2_test.o capture.o renderer.o isp_metadata.o gdc.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(OFILE): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -pie

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(OFILE)
