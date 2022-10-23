#----------------------------------------------------------------------------
#   The confidential and proprietary information contained in this file may
#   only be used by a person authorised under and to the extent permitted
#   by a subsisting licensing agreement from ARM Limited or its affiliates.
#
#          (C) COPYRIGHT [2018] Amlogic Limited or its affiliates.
#              ALL RIGHTS RESERVED
#
#   This entire notice must be reproduced on all copies of this file
#   and copies of this file may only be made by a person if such person is
#   permitted to do so under the terms of a subsisting license agreement
#   from ARM Limited or its affiliates.
#----------------------------------------------------------------------------

ifeq ($(ARCH),)
    $(error "Please set ARCH variable: arm64 or arm32")
else ifeq ($(ARCH), arm32)
    CROSS_COMPILE = /opt/gcc-linaro-6.3.1-2017.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
else ifeq ($(ARCH), arm64)
    CROSS_COMPILE = /opt/gcc-linaro-6.3.1-2017.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
endif

CC := $(CROSS_COMPILE)gcc

CFLAGS := -shared -o

MODULE_LIB_TARGET := libacamera_gamma_alg.so

SRC_FILES := $(wildcard ./*.c)

OBJS := $(SRC_FILES:.c=.o)

all: $(MODULE_LIB_TARGET)
	@echo Success Build

$(MODULE_LIB_TARGET): $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) $@

%.o:%.c
	$(CC) $< -fPIC -c -o $@

clean:
	rm $(OBJS) *.so