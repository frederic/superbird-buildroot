export V?=0
cur_dir=$(CURDIR)

export ARCH?=arm
export CROSS_COMPILE?=arm-linux-gnueabihf-
export TA_CROSS_COMPILE=arm-linux-gnueabihf-

export TDK_DIR=$(cur_dir)/../..

OUTPUT_DIR_TA = ../out/ta
OUTPUT_DIR_CA = ../out/ca

.PHONY: all
all:
	$(MAKE) -C ca O=$(OUTPUT_DIR_CA)
	$(MAKE) -C ta O=$(OUTPUT_DIR_TA) CROSS_COMPILE=$(TA_CROSS_COMPILE)

.PHONY: clean
clean:
	$(MAKE) -C ca clean
	$(MAKE) -C ta clean
