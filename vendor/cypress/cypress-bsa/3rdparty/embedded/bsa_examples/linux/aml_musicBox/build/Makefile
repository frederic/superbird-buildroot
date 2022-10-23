#######################################
#  This makefile is written for a GNU make or equivalent.
#  For example:
#   make clean
#   make
# Copyright (c) 2010, Broadcom Corp., All Rights Reserved.
# Broadcom Bluetooth Core. Proprietary and confidential.
#######################################
# Basic path definitions
GLOBALPATH := ../../../../../..
customer := bsa_examples
project := linux
application := aml_musicBox
os_name := linux
BLUETOOTH_3RD_DIR := $(GLOBALPATH)/3rdparty

# include common makefile rules
include $(BLUETOOTH_3RD_DIR)/embedded/$(customer)/$(project)/app_common/build/make.common

# extra source list list
srcdirs :=
appsources :=

# add ENABLE_ALSA=TRUE at the command line to enable
ENABLE_ALSA ?= TRUE
#ENABLE_ALSA_DSPC ?= TRUE # define on of BR2_AML_BT_DSPC
ifeq ($(strip $(ENABLE_ALSA)), TRUE)
  GLOBAL_DEFINE += -DPCM_ALSA -DPCM_ALSA_OPEN_BLOCKING -DUSE_RING_BUFFER
  ifneq ($(strip $(ENABLE_ALSA_DSPC)), TRUE)
  LINKLIBS += -lasound
  endif
  ifeq ($(strip $(ENABLE_ALSA_DSPC)), TRUE)
    GLOBAL_DEFINE += -DPCM_ALSA_DSPC
  endif
else
  GLOBAL_DEFINE += -DPCM_ALSA_DISABLE_H
endif

LINKLIBS +=  -lpthread

# include variable definitions for each component
include $(BLUETOOTH_APPLICATION_DIR)/build/def/*.def

# incdirs uses srcdirs
incdirs := $(incdirs) $(srcdirs)

# include the build rules for all the sample applications
include $(BLUETOOTH_3RD_DIR)/embedded/$(customer)/$(project)/app_common/build/make_sample_apps.common
