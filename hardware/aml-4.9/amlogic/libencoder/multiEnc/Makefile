#
#Copyright (c) 2019 Amlogic, Inc. All rights reserved.
#
#This source code is subject to the terms and conditions defined in below
#which is part of this source code package.
#
#Description:
#
#
# Copyright (C) 2019 Amlogic, Inc. All rights reserved.
#
# All information contained herein is Amlogic confidential.
#
# This software is provided to you pursuant to Software License
# Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
# used only in accordance with the terms of this agreement.
#
# Redistribution and use in source and binary forms, with or without
# modification is strictly prohibited without prior written permission
# from Amlogic.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


 
.PHONY: ALL BIN_LIB clean BINLIBCOPY

BIN = bin
LIB = lib

RM = rm -rf


ifneq ($(findstring $(AML)$(CNM),1),)
export CROSS_CC_PREFIX=aarch64-linux-gnu-
export CC=$(CROSS_CC_PREFIX)gcc
export CXX=$(CROSS_CC_PREFIX)g++
export AR=$(CROSS_CC_PREFIX)ar
endif


ifeq ($(AML), 1)

SUBDIRS = vpuapi \
		  amvenc_lib \
		  amvenc_test \


ALL: $(SUBDIRS) BINLIBCOPY


$(SUBDIRS): BIN_LIB
	make -C $@

BIN_LIB:
	mkdir -p $(BIN)
	mkdir -p $(LIB)

BINLIBCOPY:
	-cp ./vpuapi/libamvenc_api.so ./lib/
	-cp ./amvenc_lib/libvpcodec.so ./lib/
	-cp ./amvenc_test/aml_enc_test ./bin/

clean:
	$(RM) $(BIN) $(LIB)
	$(foreach i, $(SUBDIRS), make clean -C $i;)
	@echo clean all...

endif






ifeq ($(CNM), 1)


SUBDIRS = vpuapi \
		  test_sample \


ALL: $(SUBDIRS) BINLIBCOPY


$(SUBDIRS): BIN_LIB
	make -C $@

BIN_LIB:
	mkdir -p $(BIN)
	mkdir -p $(LIB)

BINLIBCOPY:
	-cp ./vpuapi/libamvenc_api.so ./lib/
	-cp ./test_sample/vp5_enc_test ./bin/


clean:
	$(RM) $(BIN) $(LIB)
	$(foreach i, $(SUBDIRS), make clean -C $i;)
	@echo clean all...

endif
