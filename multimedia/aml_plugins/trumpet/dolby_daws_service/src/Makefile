##############################################################################
# This program is protected under international and U.S. copyright laws as
# an unpublished work. This program is confidential and proprietary to the
# copyright owners. Reproduction or disclosure, in whole or in part, or the
# production of derivative works therefrom without the express permission of
# the copyright owners is prohibited.
#
#                Copyright (C) 2017 by Dolby Laboratories
#                            All rights reserved.
##############################################################################

OBJDIR = obj
SCRIPT=S92daws_service
CONFIGXML=Audio_Effect.xml

CC = arm-linux-gnueabihf-gcc
CCDEP = arm-linux-gnueabihf-gcc

CFLAGS_release = -O3 -ftree-vectorize -fPIC -c -Wall -Wextra -Wstrict-prototypes -Wmissing-prototypes -pedantic -std=gnu99 -Wdeclaration-after-statement -Wvla -DPIC

DEFINES_release = -DDLB_BACKEND_GENERIC_FLOAT32=1 -DDLB_INTRINSICS_NEW_NAMES=1 -DNDEBUG=1

INCLUDES = -I. \
           -Iinclude
LD = $(CC)
LDFLAGS_release = -O2

LDLIBS_release = -lc -lm -lpthread \
                 -lasound \
                 -lrt -levdev -ldl

SRC_FILES_SERVICE = daws_service.c

INPUTS_COMMON_LINK__SERVICE_release = $(foreach file,$(SRC_FILES_SERVICE),$(OBJDIR)_release/$(strip $(basename $(notdir $(file)))).o)

default: daws_service

daws_service: $(INPUTS_COMMON_LINK__SERVICE_release)
	@echo 'Creating "$@" ...'
	@$(CC) $(LDFLAGS_release) -o $@ $+ $(LDLIBS_release)
$(OBJDIR)_release/%.o:%.c
	@echo 'Compiling "$@" ...'
	@mkdir -p $(OBJDIR)_release
	@$(CCDEP) -MM $(DEFINES_release) $(INCLUDES) -MT $@ -o $(OBJDIR)_release/$*.d $<
	@$(CC) $(CFLAGS_release) $(DEFINES_release) $(INCLUDES) -o $@ $<

cleandeps:
	$(RM) $(OBJDIR)_release/*.d

clean:
	$(RM) -rf $(OBJDIR)_release
	$(RM) daws_service

.PHONY: default FORCE clean cleandeps cleanself help install

install:
	install -m 0755 daws_service $(TARGET_DIR)/sbin/
	install -m 0755 ./$(SCRIPT) $(TARGET_DIR)/etc/init.d/
	install -m 0644 ./$(CONFIGXML) $(TARGET_DIR)/etc/

FORCE:

help:
	@echo "This makefile has the following targets:"
	@echo " daws_service"
	@echo " default"
	@echo " clean"
	@echo " cleandeps"
	@echo " help"

