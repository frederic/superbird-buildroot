LOCAL_PATH := $(call my-dir)
TDK_PATH := $(realpath $(TOP))/$(BOARD_AML_VENDOR_PATH)/tdk
local_module := b689f2a7-8adf-477a-9f99-32e90c0ad0a2.ta
# This overrides the path of the output artifact so it won't be
# included into the system image
local_module_path := $(PRODUCT_OUT)/system/lib/teetz
local_module_ta_name := tee_storage_ta

include $(TDK_PATH)/aosp_optee.mk
