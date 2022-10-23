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

 
.PHONY: CREATE_DIR clean all

ifeq ($(ARM), 0)
CFLAGS += -O2 -std=c99
else
CFLAGS += -DARM
endif

CFLAGS+= -fPIC -g

LDFLAGS =

#CFLAGS += -static
#CLFAGS += -Wno-implicit-function-declaration -Wno-format -Wl,--fatal-warning
#CLFAGS += -Wall -Werror

INC = -I./include -I.


CFLAGS  +=  $(INC) $(DEFINES) $(PLATFORM_FLAGS)
ARFLAGS += rcs

TARGET = libamvenc_api.so
LDLIBS += -lm -lrt
# pthread 
LDLIBS += -lpthread 
LDFLAGS += -shared

INC = -I. -I./include
	
OBJDIR=obj
ALLOBJS=*.o
ALLDEPS=*.dep
ALLLIBS=*.a
RM=rm -f
MKDIR=mkdir -p

SOURCES = product.c \
		  vdi.c \
		  vdi_osal.c \
		  vpuapifunc.c \
		  vpuapi.c \
		  enc_driver.c \
		  debug.c

VPATH = .:

OBJECTNAMES=$(patsubst %.c,%.o, $(SOURCES))
OBJECTPATHS=$(addprefix $(OBJDIR)/,$(notdir $(OBJECTNAMES)))


all: CREATE_DIR $(OBJECTPATHS)
	$(CC) -o $(TARGET) $(LDFLAGS) -Wl,-gc-section -Wl,--start-group $(OBJECTPATHS) $(LDLIBS) -Wl,--end-group

-include $(OBJECTPATHS:.o=.dep)


CREATE_DIR:
	-mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@ -MD -MF $(@:.o=.dep)

clean: 
	$(RM) $(TARGET)
	$(RM) $(OBJDIR)/$(ALLOBJS)
	$(RM) $(OBJDIR)/$(ALLDEPS)
