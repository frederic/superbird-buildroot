LOCAL_PATH := $(call my-dir)
ifeq ($(BOARD_KERNEL_VERSION), 4.9)
KERNEL_DIR := kernel/common_4.9
else ifeq ($(BOARD_KERNEL_VERSION), 3.14)
KERNEL_DIR := kernel/common_3.14
else
KERNEL_DIR := common
endif
KERNEL_OUT_DIR := $(PRODUCT_OUT)/obj/KERNEL_OBJ
ifeq ($(KERNEL_A32_SUPPORT), true)
KERNEL_ARCH := arm
KERNEL_DRIVER_CROSS_COMPILE := /opt/gcc-linaro-6.3.1-2017.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
KERNEL_CONFIG=meson64_a32_defconfig
else
KERNEL_ARCH := arm64
KERNEL_DRIVER_CROSS_COMPILE := aarch64-linux-gnu-
KERNEL_CONFIG=meson64_defconfig
endif

OPTEE_MODULES := $(shell pwd)/$(PRODUCT_OUT)/obj/optee_modules
include $(CLEAR_VARS)
$(info $(shell if [ ! -d $(KERNEL_OUT_DIR) ]; then mkdir -p $(KERNEL_OUT_DIR); fi))

$(info $(shell if [ ! -e $(KERNEL_OUT_DIR)/include/generated/autoconf.h ]; then $(MAKE) -C $(KERNEL_DIR) O=../$(KERNEL_OUT_DIR) ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(KERNEL_DRIVER_CROSS_COMPILE) $(KERNEL_CONFIG); fi))


$(info $(shell if [ ! -e $(KERNEL_OUT_DIR)/include/generated/autoconf.h ]; then $(MAKE) -C $(KERNEL_DIR) O=../$(KERNEL_OUT_DIR) ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(KERNEL_DRIVER_CROSS_COMPILE) modules_prepare; fi))

$(info $(shell if [ ! -d $(OPTEE_MODULES) ]; then mkdir -p $(OPTEE_MODULES); fi))
$(info $(shell cp $(LOCAL_PATH)/* $(OPTEE_MODULES) -rfa))
$(info $(shell $(MAKE) -C $(shell pwd)/$(KERNEL_OUT_DIR) M=$(OPTEE_MODULES) KERNEL_A32_SUPPORT=$(KERNEL_A32_SUPPORT) ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(KERNEL_DRIVER_CROSS_COMPILE) modules))


include $(CLEAR_VARS)
LOCAL_MODULE := optee_armtz
LOCAL_PREBUILT_MODULE_FILE := $(OPTEE_MODULES)/optee/optee_armtz.ko
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .ko
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib
LOCAL_STRIP_MODULE := false
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := optee
LOCAL_PREBUILT_MODULE_FILE := $(OPTEE_MODULES)/optee.ko
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .ko
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib
LOCAL_STRIP_MODULE := false
include $(BUILD_PREBUILT)
