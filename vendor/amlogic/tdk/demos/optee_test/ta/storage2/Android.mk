LOCAL_PATH := $(call my-dir)
TDK_PATH := $(realpath $(TOP))/$(BOARD_AML_VENDOR_PATH)/tdk
local_module := 731e279e-aafb-4575-a771-38caa6f0cca6.ta
# This overrides the path of the output artifact so it won't be
# included into the system image
local_module_path := $(PRODUCT_OUT)/system/lib/teetz
local_module_ta_name := tee_storage2_ta

include $(TDK_PATH)/aosp_optee.mk
