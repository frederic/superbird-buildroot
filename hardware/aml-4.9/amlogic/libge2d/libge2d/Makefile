# export CROSS_COMPILE=arm-linux-gnueabihf-
# export CROSS_COMPILE=/opt/gcc-linaro-6.3.1-2017.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
# CC = $(CROSS_COMPILE)gcc

SRC  := $(wildcard *.c)
OBJS := $(SRC:%.c=%.o)
.PHONY : clean all

LIB := libge2d.so

all: $(LIB)

%.o: %.c
	$(CC) -c -fPIC -o $@ $< $(CFLAGS)

$(LIB): $(OBJS)
	$(CC) -shared -fPIC -lion -o $(LIB) $(OBJS)
	@rm -f $(OBJS)

clean:
	rm -f $(LIB)
