#######################################
#  This makefile is written for a GNU make or equivalent.
#  For example:
#   make clean
#   make
# Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
# Broadcom Bluetooth Core. Proprietary and confidential.
#######################################
# Basic path definitions
GLOBALPATH := ../../../../../..
customer := bsa_examples
project := linux
application := app_hh
os_name := linux
BLUETOOTH_3RD_DIR:=$(GLOBALPATH)/3rdparty

# include common makefile rules
include $(BLUETOOTH_3RD_DIR)/embedded/$(customer)/$(project)/app_common/build/make.common

# extra source list list
srcdirs :=
appsources :=

# add ENABLE_BTHID=TRUE at the command line to enable
ENABLE_BTHID ?= FALSE
ifeq ($(strip $(ENABLE_BTHID)),TRUE)
  GLOBAL_DEFINE += -DAPP_BTHID_INCLUDED=TRUE
endif

ENABLE_HID_AUDIO ?= TRUE
ifeq ($(strip $(ENABLE_HID_AUDIO)),TRUE)
  GLOBAL_DEFINE += -DAPP_HH_AUDIO_INCLUDED=TRUE
endif

ENABLE_BLE_AUDIO ?= FALSE
ifeq ($(strip $(ENABLE_BLE_AUDIO)),TRUE)
  GLOBAL_DEFINE += -DAPP_HH_BLE_AUDIO_INCLUDED=TRUE
endif
  
ENABLE_ALSA ?= FALSE
ifeq ($(strip $(ENABLE_ALSA)), TRUE)
  GLOBAL_DEFINE += -DPCM_ALSA
  LINKLIBS += -lasound
endif

ENABLE_OTAFWDL ?= TRUE
ifeq ($(strip $(ENABLE_OTAFWDL)),TRUE)
  GLOBAL_DEFINE += -DAPP_HH_OTA_FW_DL_INCLUDED=TRUE
endif

ENABLE_AUDIO_STREAMING ?= FALSE
ifeq ($(strip $(ENABLE_AUDIO_STREAMING)),TRUE)
  GLOBAL_DEFINE += -DAPP_HH_AUDIO_STREAMING_INCLUDED=TRUE
endif

# include variable definitions for each component
include $(BLUETOOTH_APPLICATION_DIR)/build/def/*.def

# incdirs uses srcdirs
incdirs := $(incdirs) $(srcdirs)

# include the build rules for all the sample applications
include $(BLUETOOTH_3RD_DIR)/embedded/$(customer)/$(project)/app_common/build/make_sample_apps.common
