LOCAL_PATH := $(call my-dir)
TDK_PATH := $(realpath $(TOP))/$(BOARD_AML_VENDOR_PATH)/tdk
local_module := f157cda0-550c-11e5-a6fa-0002a5d5c51b.ta
# This overrides the path of the output artifact so it won't be
# included into the system image
local_module_path := $(PRODUCT_OUT)/system/lib/teetz
local_module_ta_name := tee_storage_benchmark_ta

include $(TDK_PATH)/aosp_optee.mk
