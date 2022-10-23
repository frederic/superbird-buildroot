################################################################################
#
# marvell wifi/bt driver
#
################################################################################

MRVL_WIFIBT_VERSION = 4.9
ifeq ($(call qstrip, $(BR2_PACKAGE_MRVL_WIFI_LOCAL_PATH)),)
BR2_PACKAGE_MRVL_WIFI_LOCAL_PATH = $(TOPDIR)/dummy
endif
MRVL_WIFIBT_SITE = $(call qstrip,$(BR2_PACKAGE_MRVL_WIFI_LOCAL_PATH))
MRVL_BT_LOCAL_PATH = $(call qstrip,$(BR2_PACKAGE_MRVL_BT_LOCAL_PATH))
MRVL_FW_LOCAL_PATH = $(call qstrip,$(BR2_PACKAGE_MRVL_FW_LOCAL_PATH))
MRVL_WIFIBT_SITE_METHOD = local

MRVL_WIFIBT_MODULE_DIR = kernel/mrvl
MRVL_WIFIBT_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(MRVL_WIFIBT_MODULE_DIR)

MRVL_WIFIBT_DEPENDENCIES = linux

ifeq ($(BR2_PACKAGE_SD8977),y)
define SD8977_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/sd8977 ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_KERNEL_CROSS) modules

	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/bt/sd8977/bluez ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_KERNEL_CROSS) modules
endef

define SD8977_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0666 $(@D)/sd8977/mlan.ko $(MRVL_WIFIBT_INSTALL_DIR)/mlan_8977.ko
	$(INSTALL) -m 0666 $(@D)/sd8977/sd8xxx.ko $(MRVL_WIFIBT_INSTALL_DIR)/sd8xxx_8977.ko
	$(INSTALL) -m 0666 $(@D)/bt/sd8977/bluez/bt8xxx.ko $(MRVL_WIFIBT_INSTALL_DIR)/bt8xxx_8977.ko
	$(INSTALL) -m 0666 $(MRVL_FW_LOCAL_PATH)/sd8977/*.bin $(TARGET_DIR)/lib/firmware/mrvl
	echo $(MRVL_WIFIBT_MODULE_DIR)/mlan_8977.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
	echo $(MRVL_WIFIBT_MODULE_DIR)/sd8xxx_8977.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
	echo $(MRVL_WIFIBT_MODULE_DIR)/bt8xxx_8977.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
endif

ifeq ($(BR2_PACKAGE_SD8987),y)
define SD8987_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/sd8987 ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_KERNEL_CROSS) modules

	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/bt/sd8987/bluez ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_KERNEL_CROSS) modules
endef

define SD8987_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0666 $(@D)/sd8987/mlan.ko $(MRVL_WIFIBT_INSTALL_DIR)/mlan_8987.ko
	$(INSTALL) -m 0666 $(@D)/sd8987/sd8xxx.ko $(MRVL_WIFIBT_INSTALL_DIR)/sd8xxx_8987.ko
	$(INSTALL) -m 0666 $(@D)/bt/sd8987/bluez/bt8xxx.ko $(MRVL_WIFIBT_INSTALL_DIR)/bt8xxx_8987.ko
	$(INSTALL) -m 0666 $(MRVL_FW_LOCAL_PATH)/sd8987/*.bin $(TARGET_DIR)/lib/firmware/mrvl
	echo $(MRVL_WIFIBT_MODULE_DIR)/mlan_8987.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
	echo $(MRVL_WIFIBT_MODULE_DIR)/sd8xxx_8987.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
	echo $(MRVL_WIFIBT_MODULE_DIR)/bt8xxx_8987.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
endif

define MRVL_WIFIBT_BUILD_CMDS
	cp $(MRVL_BT_LOCAL_PATH) $(@D) -rf
	$(SD8977_BUILD_CMDS)
	$(SD8987_BUILD_CMDS)
endef

define MRVL_WIFIBT_INSTALL_TARGET_CMDS
	mkdir -p $(MRVL_WIFIBT_INSTALL_DIR)
	mkdir -p $(TARGET_DIR)/lib/firmware/mrvl
	$(SD8977_INSTALL_TARGET_CMDS)
	$(SD8987_INSTALL_TARGET_CMDS)
endef

$(eval $(generic-package))
