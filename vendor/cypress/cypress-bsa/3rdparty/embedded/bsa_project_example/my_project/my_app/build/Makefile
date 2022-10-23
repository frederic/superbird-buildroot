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
customer := bsa_project_example
project := my_project
application := my_app
os_name := linux
BLUETOOTH_3RD_DIR := $(GLOBALPATH)/3rdparty

# include common makefile rules (from the common directory)
include $(BLUETOOTH_3RD_DIR)/embedded/bsa_examples/linux/app_common/build/make.common

# extra source list list
srcdirs :=
appsources :=

# This applicaton needs HH
GLOBAL_DEFINE += -DAPP_HH_INCLUDED

# This application needs BLE
GLOBAL_DEFINE += -DAPP_BLE_INCLUDED

# This applicaton needs AV
GLOBAL_DEFINE += -DAPP_AV_INCLUDED

# This application needs AVK
GLOBAL_DEFINE += -DAPP_AVK_INCLUDED

# This applicaton needs HEADLESS
#GLOBAL_DEFINE += -DAPP_HEADLESS_INCLUDED

# This flag is used to indicate if the received RC PCM sample must be saved in Wav file
GLOBAL_DEFINE += -DRC_AUDIO_WAV_FILE

# This flag is used to remove the debug information in the app trace
#GLOBAL_DEFINE += -DAPP_TRACE_NODEBUG

# This flag is used to add timestamp in the app traces
#GLOBAL_DEFINE += -DAPP_TRACE_TIMESTAMP

# This flag is used to add color in the app traces
GLOBAL_DEFINE += -DAPP_TRACE_COLOR

GLOBAL_DEFINE += -DAPP_HH_SEC_DEFAULT=BSA_SEC_NONE

GLOBAL_DEFINE += -DAPP_HH_DEVICE_MAX=20

GLOBAL_DEFINE += -DAPP_MAX_NB_REMOTE_STORED_DEVICES=20

ENABLE_HID_AUDIO ?= TRUE
ifeq ($(strip $(ENABLE_HID_AUDIO)),TRUE)
  GLOBAL_DEFINE += -DAPP_HH_AUDIO_INCLUDED=TRUE
endif

ENABLE_BLE_AUDIO ?= TRUE
ifeq ($(strip $(ENABLE_BLE_AUDIO)),TRUE)
  GLOBAL_DEFINE += -DAPP_HH_BLE_AUDIO_INCLUDED=TRUE
endif

# add ENABLE_BTHID=TRUE at the command line to enable
ENABLE_BTHID ?= FALSE
ifeq ($(strip $(ENABLE_BTHID)),TRUE)
  GLOBAL_DEFINE += -DAPP_BTHID_INCLUDED=TRUE
  APP_BTHID_INCLUDED=TRUE
endif

# add ENABLE_ALSA=TRUE at the command line to enable
ENABLE_ALSA ?= FALSE
ifeq ($(strip $(ENABLE_ALSA)), TRUE)
  GLOBAL_DEFINE += -DPCM_ALSA
  LINKLIBS += -lasound
endif

# include the local definition files
include def/*.def

# incdirs uses srcdirs
incdirs := $(incdirs) $(srcdirs)

# include the build rules for all the sample applications
include $(BLUETOOTH_3RD_DIR)/embedded/bsa_examples/linux/app_common/build/make_sample_apps.common


