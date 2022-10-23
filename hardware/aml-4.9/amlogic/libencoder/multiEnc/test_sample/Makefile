#
# Copyright (c) 2019 Amlogic, Inc. All rights reserved.
#
#This source code is subject to the terms and conditions defined in below
# which is part of this source code package.
#
# Description:


# copyright (c) 2019 amlogic, inc. all rights reserved.
#
# all information contained herein is amlogic confidential.
#
# this software is provided to you pursuant to software license
# agreement (sla) with amlogic inc ("amlogic"). this software may be
# used only in accordance with the terms of this agreement.
#
# redistribution and use in source and binary forms, with or without
# modification is strictly prohibited without prior written permission
# from amlogic.
#
# this software is provided by the copyright holders and contrIBUTORS
# "as is" and any express or implied warranties, including, buT NOT
# limited to, the implied warranties of merchantability and fiTNESS FOR
# a particular purpose are disclaimed. in no event shall the cOPYRIGHT
# owner or contributors be liable for any direct, indirect, inCIDENTAL,
# special, exemplary, or consequential damages (including, but NOT
# limited to, procurement of substitute goods or services; losS OF USE,
# data, or profits; or business interruption) however caused aND ON ANY
# theory of liability, whether in contract, strict liability, OR TORT
# (including negligence or otherwise) arising in any way out oF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

.PHONY: CREATE_DIR clean all

PRODUCT := VP521C

$(shell cp component_list_encoder.h component/component_list.h)

USE_FFMPEG  = no
USE_PTHREAD = yes
USE_RTL_SIM = no
LINT_HOME   = etc/lint
BUILD_CONFIGURATION = EmbeddedLinux

UNAME = $(shell uname -a)
ifneq (,$(findstring i386, $(UNAME)))
    USE_32BIT = yes
endif

ifeq ($(RTL_SIM), 1)
USE_RTL_SIM = yes
endif


ifeq ($(USE_32BIT), yes)
PLATFORM    = nativelinux
else
PLATFORM    = nativelinux_64bit
endif

#VDI_C           = vdi/linux/vdi.c
#VDI_OSAL_C      = vdi/linux/vdi_osal.c
MM_C            =
PLATFORM_FLAGS  =

VDI_VPATH       =

INCLUDES = -I../vpuapi -I../vpuapi/include -I./helper -I./helper/misc -I./component
INCLUDES += -I./component_encoder -I./ffmpeg/include
DEFINES += -D$(PRODUCT) 

ifeq ($(USE_RTL_SIM), yes)
DEFINES += -DCNM_SIM_PLATFORM -DCNM_SIM_DPI_INTERFACE -DSUPPORT_ENCODER
MM_C            = ../vdi/mm.c
else
endif	#USE_RTL_SIM
DEFINES += $(USER_DEFINES)

CFLAGS  += -g -I. -Wno-implicit-function-declaration -Wno-format -Wl,--fatal-warning $(INCLUDES) $(DEFINES) $(PLATFORM_FLAGS)
ifeq ($(USE_RTL_SIM), yes)
ifeq ($(IUS), 1)
CFLAGS  += -fPIC # ncverilog is 64bit version
endif
endif
ARFLAGS += cru

LDFLAGS  += $(PLATFORM_FLAGS)


ifeq ($(USE_FFMPEG), yes)
LDLIBS  += -lavformat -lavcodec -lavutil
LDFLAGS += -L./ffmpeg/lib/$(PLATFORM)
ifneq ($(USE_32BIT), yes)
LDLIBS  += -lz
endif #USE_32BIT
endif #USE_FFMPEG

ifeq ($(USE_PTHREAD), yes)
LDLIBS  += -lpthread
endif
LDLIBS  += -lm

TARGET=vp5_enc_test
OBJDIR=obj
ALLOBJS=*.o
ALLDEPS=*.dep
ALLLIBS=*.a
RM=rm -f
MKDIR=mkdir -p

SOURCES = main_enc_test.c              \
          component_enc_encoder.c      \
          component_enc_feeder.c       \
          component_enc_reader.c       \
          encoder_listener.c           \
          cnm_app.c                    \
          cnm_task.c                   \
          component.c                  \
          main_helper.c                \
          vpuhelper.c                  \
          bitstreamreader.c            \
          bin_comparator_impl.c        \
          comparator.c                 \
          md5_comparator_impl.c        \
          yuv_comparator_impl.c        \
          cfgParser.c                  \
          cnm_video_helper.c           \
          container.c                  \
          datastructure.c              \
          yuvfeeder.c                  \
          yuvLoaderfeeder.c            \
          yuvCfbcfeeder.c              \
          bw_monitor.c                 \
          pf_monitor.c


#SOURCES += bitstreamfeeder.c

ifeq ($(USE_RTL_SIM), yes)
	SOURCES += sample/main_sim.c
endif

VPATH = component_encoder:
#VPATH += component_decoder:
VPATH += helper:
VPATH += helper/bitstream:
VPATH += helper/comparator:
VPATH += helper/display:helper/misc:helper/yuv:component:
LDLIBS  += -L../vpuapi/ -lamvenc_api

OBJECTNAMES=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))
OBJECTPATHS=$(addprefix $(OBJDIR)/,$(notdir $(OBJECTNAMES)))

LINT_SRC_INCLUDES = -I. -I./component -I./component_decoder -I./component_encoder
LINT_SRC_INCLUDES += -I./helper -I./helper/bitstream -I./helper/comparator -I./helper/display
LINT_SRC_INCLUDES += -I./helper/misc -I./helper/yuv -I./helper/comparator -I./helper/display

ifeq ($(USE_RTL_SIM), yes)
all: CREATE_DIR $(OBJECTPATHS)
else
all: CREATE_DIR $(OBJECTPATHS)
	$(CC) -o $(TARGET) $(LDFLAGS) -Wl,-gc-section -Wl,--start-group $(OBJECTPATHS) $(LDLIBS) -Wl,--end-group
endif

-include $(OBJECTPATHS:.o=.dep)

clean:
	$(RM) $(TARGET)
	$(RM) $(OBJDIR)/$(ALLOBJS)
	$(RM) $(OBJDIR)/$(ALLDEPS)

CREATE_DIR:
	-mkdir -p $(OBJDIR)

obj/%.o: %.c $(MAKEFILE)
	$(CC) $(CFLAGS) -Wall -Werror -c $< -o $@ -MD -MF $(@:.o=.dep)


lint: 
	"$(LINT_HOME)/flint" -i"$(LINT_HOME)" $(DEFINES) $(INCLUDES) $(LINT_SRC_INCLUDES) linux_std.lnt $(HAPS_RULE) $(NONOS_RULE)  $(SOURCES) 

