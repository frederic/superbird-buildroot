export V?=0
cur_dir=$(CURDIR)

export ARCH?=arm
export CROSS_COMPILE?=arm-linux-gnueabihf-
export TA_CROSS_COMPILE=arm-linux-gnueabihf-

export TDK_DIR=$(cur_dir)/../..

OUTPUT_DIR_CA = ../out/ca

.PHONY: all
all:
	$(MAKE) -C ca O=$(OUTPUT_DIR_CA)

.PHONY: clean
clean:
	$(MAKE) -C ca clean
