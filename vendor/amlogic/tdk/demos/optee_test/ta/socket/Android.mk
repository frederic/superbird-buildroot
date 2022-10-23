LOCAL_PATH := $(call my-dir)
TDK_PATH := $(realpath $(TOP))/$(BOARD_AML_VENDOR_PATH)/tdk
local_module := 873bcd08-c2c3-11e6-a937-d0bf9c45c61c.ta
# This overrides the path of the output artifact so it won't be
# included into the system image
local_module_path := $(PRODUCT_OUT)/system/lib/teetz
local_module_ta_name := tee_socket_ta

include $(TDK_PATH)/aosp_optee.mk
