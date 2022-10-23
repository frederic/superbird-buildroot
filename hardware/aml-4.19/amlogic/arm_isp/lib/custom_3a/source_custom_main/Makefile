#----------------------------------------------------------------------------
#   The confidential and proprietary information contained in this file may
#   only be used by a person authorised under and to the extent permitted
#   by a subsisting licensing agreement from ARM Limited or its affiliates.
#
#          (C) COPYRIGHT [2018] ARM Limited or its affiliates.
#              ALL RIGHTS RESERVED
#
#   This entire notice must be reproduced on all copies of this file
#   and copies of this file may only be made by a person if such person is
#   permitted to do so under the terms of a subsisting license agreement
#   from ARM Limited or its affiliates.
#----------------------------------------------------------------------------

LINK_TARGET = iv009_isp.elf

ifeq ($(ARCH),)
    $(error "Please set ARCH variable: arm64 or arm32")
else ifeq ($(ARCH), arm32)
    CROSS_COMPILE = /opt/gcc-linaro-7.3.1-2018.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
else ifeq ($(ARCH), arm64)
    CROSS_COMPILE = /opt/gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
endif

#redefine CROSS_COMPILE
ifeq ($(CROSS_COMPILE),)
    $(error "Please export CROSS_COMPILE variable before build")
else
    _CROSS_COMPILE=${CROSS_COMPILE}
endif

CC=$(_CROSS_COMPILE)gcc

CFLAGS=-I src -I app/ -I app/control -I src/fw -I src/fw_lib -I src/driver/sensor -I src/driver/lens -I inc -I inc/isp -I inc/api -I inc/sys -Os -c  -Wno-unused-function -fPIE
ifeq ($(matchbit),1)
    CFLAGS+= -DMATCHBIT
endif
ODIR=obj
SOURCES = $(wildcard app/*.c app/control/*.c src/*.c src/fw/*.c src/fw_lib/*.c src/platform/*.c  src/driver/*/*.c src/calibration/*.c )
TOBJ = $(notdir $(SOURCES:.c=.o))
OBJ = $(addprefix $(ODIR)/, $(TOBJ) )
LDFLAGS=-ldl

ifneq ($(wildcard src/fw_lib/libacamera_isp.a),)
CFLAGS+=-fPIC
LDFLAGS+=-Lsrc/fw_lib -lacamera_isp
endif

REBUILDABLES = $(OBJ) $(LINK_TARGET)

.PHONY: all
all : $(LINK_TARGET)
	echo All done

$(shell  mkdir -p $(ODIR))

$(LINK_TARGET) : $(OBJ)
	$(CC) -pthread $(OBJ) $(LDFLAGS) -pie -o $@

$(ODIR)/%.o: app/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
$(ODIR)/%.o: app/control/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
$(ODIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
$(ODIR)/%.o: src/calibration/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
$(ODIR)/%.o: src/driver/sensor/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
$(ODIR)/%.o: src/driver/lens/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
$(ODIR)/%.o: src/platform/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
$(ODIR)/%.o: src/fw/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
$(ODIR)/%.o: src/fw_lib/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean :
	rm -f $(REBUILDABLES)
	echo Clean done
