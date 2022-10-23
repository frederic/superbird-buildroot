LOCAL_PATH := $(call my-dir)
TDK_PATH := $(realpath $(TOP))/$(BOARD_AML_VENDOR_PATH)/tdk
local_module := e6a33ed4-562b-463a-bb7e-ff5e15a493c8.ta
# This overrides the path of the output artifact so it won't be
# included into the system image
local_module_path := $(PRODUCT_OUT)/system/lib/teetz
local_module_ta_name := tee_sims_ta

include $(TDK_PATH)/aosp_optee.mk
