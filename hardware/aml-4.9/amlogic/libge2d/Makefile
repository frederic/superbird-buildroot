# export CROSS_COMPILE=arm-linux-gnueabihf-
# export CROSS_COMPILE=/opt/gcc-linaro-6.3.1-2017.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
# CC = $(CROSS_COMPILE)gcc

CFLAGS += -I./libge2d/include/
CFLAGS += -I./libge2d/kernel-headers/linux/
LIBDIR:= ./libge2d
FEATURE_TEST := ge2d_feature_test
CHIP_CHECK := ge2d_chip_check

.PHONY : clean all

all:
	$(MAKE) -C $(LIBDIR)
	$(CC) $(CFLAGS) -L$(LIBDIR) -lpthread -lge2d $(addsuffix .c,$(FEATURE_TEST)) -o $(FEATURE_TEST)
	$(CC) $(CFLAGS) -L$(LIBDIR) -lpthread -lge2d $(addsuffix .c,$(CHIP_CHECK)) -o $(CHIP_CHECK)

clean:
	rm -f $(LIBDIR)/libge2d.so
	rm -f $(FEATURE_TEST) $(CHIP_CHECK)
