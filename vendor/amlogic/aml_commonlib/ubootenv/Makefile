LIB = libubootenv.a
.PHONY: all install clean

ubootenv.o: ubootenv.c
	$(CC) -c ubootenv.c

uenv_test.o: uenv_test.c
	$(CC) -c uenv_test.c

all: ubootenv.o uenv
	$(AR) rc $(LIB) ubootenv.o

uenv: ubootenv.o uenv_test.o
	$(CC) $^ -lz -o $@

clean:
	rm -f *.o $(LIB)

install:
	install -m 755 $(LIB) $(STAGING_DIR)/usr/lib
	install -m 755 uenv $(TARGET_DIR)/usr/bin
