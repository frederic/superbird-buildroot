#######################################
#  This makefile is written for a GNU make or equivalent.
#  For example:
#   make clean
#   make
# Copyright (c) 2013, Broadcom Corp., All Rights Reserved.
# Broadcom Bluetooth Core. Proprietary and confidential.
#######################################
# Basic path definitions
GLOBALPATH := ../../../../../..
customer := bsa_examples
project := linux
application := app_hd
os_name := linux
BLUETOOTH_3RD_DIR:=$(GLOBALPATH)/3rdparty

# include common makefile rules
include $(BLUETOOTH_3RD_DIR)/embedded/$(customer)/$(project)/app_common/build/make.common

# extra source list list
srcdirs :=
appsources :=

ENABLE_AUDIO_STREAMING ?= FALSE
ifeq ($(strip $(ENABLE_AUDIO_STREAMING)),TRUE)
  GLOBAL_DEFINE += -DAPP_HD_AUDIO_STREAMING_INCLUDED=TRUE
endif

# include variable definitions for each component
include $(BLUETOOTH_APPLICATION_DIR)/build/def/*.def

# incdirs uses srcdirs
incdirs := $(incdirs) $(srcdirs)

# include the build rules for all the sample applications
include $(BLUETOOTH_3RD_DIR)/embedded/$(customer)/$(project)/app_common/build/make_sample_apps.common
