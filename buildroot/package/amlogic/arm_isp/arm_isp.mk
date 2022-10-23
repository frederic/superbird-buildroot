################################################################################
#
# amlogic arm_isp drivers
#
################################################################################

ARM_ISP_VERSION = $(call qstrip,$(BR2_PACKAGE_ARM_ISP_VERSION))
ARM_ISP_SITE = $(call qstrip,$(BR2_PACKAGE_ARM_ISP_LOCAL_PATH))
CUR_PATH := $(shell cd "$(dirname $$0)";pwd)/$(dir $(lastword $(MAKEFILE_LIST)))
ARM_ISP_INSTALL_STAGING = YES

# modules
ARM_ISP_MODULE_DIR = kernel/amlogic/arm_isp
ARM_ISP_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(ARM_ISP_MODULE_DIR)
ARM_ISP_DEP = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
ARM_ISP_INSTALL_STAGING_DIR = $(STAGING_DIR)/usr/include/linux
ARM_AUTOCAP = $(@D)/isp_module/v4l2_dev/inc/api/acamera_autocap_api.h

define copy-arm-isp
        $(foreach m, $(shell find $(strip $(1)) -name "*.ko"),\
                $(shell [ ! -e $(2) ] && mkdir $(2) -p;\
                cp $(m) $(strip $(2))/ -rfa;\
                echo $(4)/$(notdir $(m)): >> $(3)))
endef

ifeq ($(BR2_PACKAGE_AML_SOC_FAMILY_NAME), "C1")
PLATFORM = C308X
else
PLATFORM = G12B
endif

ifeq ($(BR2_PACKAGE_ARM_ISP_LOCAL),y)
ARM_ISP_SITE = $(call qstrip,$(BR2_PACKAGE_ARM_ISP_LOCAL_PATH))
ARM_ISP_SITE_METHOD = local
ARM_ISP_DEPENDENCIES = linux

V4L2_DEV_BUILD_CMDS = cd $(@D)/isp_module/v4l2_dev; \
                        $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/isp_module/v4l2_dev KDIR=$(LINUX_DIR) \
                        ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(TARGET_KERNEL_CROSS) PLATFORM_VERSION=$(PLATFORM)

SENSOR_DEV_BUILD_CMDS = cd $(@D)/isp_module/subdev/sensor; \
                        $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/isp_module/subdev/sensor KDIR=$(LINUX_DIR) \
                        ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(TARGET_KERNEL_CROSS) PLATFORM_VERSION=$(PLATFORM)

IQ_DEV_BUILD_CMDS = cd $(@D)/isp_module/subdev/iq; \
                        $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/isp_module/subdev/iq KDIR=$(LINUX_DIR) \
                        ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(TARGET_KERNEL_CROSS) PLATFORM_VERSION=$(PLATFORM)

LENS_DEV_BUILD_CMDS = cd $(@D)/isp_module/subdev/lens; \
                        $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/isp_module/subdev/lens KDIR=$(LINUX_DIR) \
                        ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(TARGET_KERNEL_CROSS) PLATFORM_VERSION=$(PLATFORM)

define ARM_ISP_BUILD_CMDS
	$(V4L2_DEV_BUILD_CMDS)
	$(SENSOR_DEV_BUILD_CMDS)
	$(IQ_DEV_BUILD_CMDS)
	$(LENS_DEV_BUILD_CMDS)
endef

define ARM_ISP_CLEAN_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/isp_module/v4l2_dev clean
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/isp_module/subdev/sensor clean
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/isp_module/subdev/iq clean
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/isp_module/subdev/lens clean
endef

ifeq ($(BR2_aarch64), y)
ARM_ISP_SERVER_BIN = $(@D)/lib/lib64/iv009_isp_64.elf
else ifeq ($(BR2_ARM_KERNEL_32), y)
ARM_ISP_SERVER_BIN = $(@D)/lib/lib32/iv009_isp_32.elf
else
ARM_ISP_SERVER_BIN = $(@D)/lib/lib32_64/iv009_isp_u32_k64.elf
endif

define ARM_ISP_INSTALL_STAGING_CMDS
	if [ -f "$(ARM_AUTOCAP)" ]; then \
		$(INSTALL) -D -m 0644 $(ARM_AUTOCAP) $(ARM_ISP_INSTALL_STAGING_DIR); \
	fi
endef

define ARM_ISP_INSTALL_TARGET_CMDS
        $(call copy-arm-isp,$(@D),\
                $(shell echo $(ARM_ISP_INSTALL_DIR)),\
                $(shell echo $(ARM_ISP_DEP)),\
                $(ARM_ISP_MODULE_DIR))
		$(INSTALL) -D -m 0755 $(ARM_ISP_SERVER_BIN) $(TARGET_DIR)/usr/bin/iv009_isp
		$(INSTALL) -D -m 0755 $(ARM_ISP_PKGDIR)/S30isp $(TARGET_DIR)/etc/init.d/
endef
endif

$(eval $(generic-package))

