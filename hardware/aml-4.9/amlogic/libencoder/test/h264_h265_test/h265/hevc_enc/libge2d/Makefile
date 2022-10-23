ARM=1
# 指令编译器和选项
ifeq ($(ARM), 1)
CC=arm-none-linux-gnueabi-g++
AR=arm-none-linux-gnueabi-ar
else
ifeq ($(ARM), 2)
CC=aarch64-linux-gnu-g++
AR=aarch64-linux-gnu-ar
else
CC=gcc
AR=ar
endif
endif

ifeq ($(ARM), 1)
CFLAGS+=-DARM
else
CFLAGS+=-O2 -std=c99
endif
CFLAGS+= -g -static
# 目标文件
TARGET = ../../libge2d.a
LDFLAGS += -lm -lrt

INC = -I.

../../libge2d.a:aml_ge2d.o ge2d_port.o ion.o IONmem.o ge2d_feature_test.o
	$(AR) rcs $@ $^

aml_ge2d.o: aml_ge2d.c
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

ge2d_port.o: ge2d_port.c
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

ion.o: ion.c
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

IONmem.o: IONmem.c
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

ge2d_feature_test.o: ge2d_feature_test.c
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
