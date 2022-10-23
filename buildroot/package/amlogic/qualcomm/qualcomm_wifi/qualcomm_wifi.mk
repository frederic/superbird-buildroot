################################################################################
#
# qualcomm wifi/bt
#
################################################################################

QUALCOMM_WIFI_VERSION = 4.9
ifeq ($(call qstrip, $(BR2_PACKAGE_QUALCOMM_WIFI_LOCAL_PATH)),)
BR2_PACKAGE_QUALCOMM_WIFI_LOCAL_PATH = $(TOPDIR)/dummy
endif
QUALCOMM_WIFI_SITE = $(call qstrip,$(BR2_PACKAGE_QUALCOMM_WIFI_LOCAL_PATH))
QUALCOMM_WIFI_SITE_METHOD = local

QUALCOMM_MODULE_DIR = kernel/qualcomm/wifi
QUALCOMM_MODULE_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(QUALCOMM_MODULE_DIR)

QUALCOMM_WIFI_DEPENDENCIES = linux

ifeq ($(BR2_PACKAGE_QUALCOMM_QCA9377),y)
define QUALCOMM_QCA9377_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE)  -C $(@D)/qca9377/AIO/build ARCH=$(KERNEL_ARCH) \
		KERNELPATH=$(LINUX_DIR)	CROSS_COMPILE=$(TARGET_KERNEL_CROSS) CONFIG_BUILDROOT=y
endef

define QUALCOMM_QCA9377_INSTALL_CMDS
	mkdir -p $(QUALCOMM_MODULE_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/qca9377/AIO/rootfs-x86-android.build/lib/modules/wlan.ko $(QUALCOMM_MODULE_INSTALL_DIR)
	echo $(QUALCOMM_MODULE_DIR)/wlan.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.de
	mkdir -p $(TARGET_DIR)/lib/firmware/qca
	mkdir -p $(TARGET_DIR)/lib/firmware/qca9377
	mkdir -p $(TARGET_DIR)/lib/firmware/qca9377/wlan
	$(INSTALL) -D -m 0644 $(WIFI_FW_SITE)/qcom/config/qca9377/wifi_49/*.bin $(TARGET_DIR)/lib/firmware/qca9377/
	$(INSTALL) -D -m 0644 $(WIFI_FW_SITE)/qcom/config/qca9377/wifi_49/wlan/* $(TARGET_DIR)/lib/firmware/qca9377/wlan/
	$(INSTALL) -D -m 0644 $(WIFI_FW_SITE)/qcom/config/qca9377/bt_bluez/* $(TARGET_DIR)/lib/firmware/qca/
endef
endif

ifeq ($(BR2_PACKAGE_QUALCOMM_QCA6174),y)
define QUALCOMM_QCA6174_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE)  -C $(@D)/qca6174/AIO/build ARCH=$(KERNEL_ARCH) \
		KERNELPATH=$(LINUX_DIR)	CROSS_COMPILE=$(TARGET_KERNEL_CROSS)  CONFIG_BUILDROOT=y
endef

define QUALCOMM_QCA6174_INSTALL_CMDS
	mkdir -p $(QUALCOMM_MODULE_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/qca6174/AIO/rootfs-x86-android.build/lib/modules/wlan.ko $(QUALCOMM_MODULE_INSTALL_DIR)
	echo $(QUALCOMM_MODULE_DIR)/wlan.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
	mkdir -p $(TARGET_DIR)/lib/firmware/qca
	mkdir -p $(TARGET_DIR)/lib/firmware/qca6174
	mkdir -p $(TARGET_DIR)/lib/firmware/qca6174/wlan
	$(INSTALL) -D -m 0644 $(WIFI_FW_SITE)/qcom/config/qca6174/wifi/*.bin $(TARGET_DIR)/lib/firmware/qca6174/
	$(INSTALL) -D -m 0644 $(WIFI_FW_SITE)/qcom/config/qca6174/wifi/wlan/* $(TARGET_DIR)/lib/firmware/qca6174/wlan/
	$(INSTALL) -D -m 0644 $(WIFI_FW_SITE)/qcom/config/qca6174/bt/* $(TARGET_DIR)/lib/firmware/qca/
endef
endif

define QUALCOMM_WIFI_BUILD_CMDS
	$(QUALCOMM_QCA6174_BUILD_CMDS)
	$(QUALCOMM_QCA9377_BUILD_CMDS)
endef

define QUALCOMM_WIFI_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 755 $(QUALCOMM_WIFI_PKGDIR)/S44bluetooth $(TARGET_DIR)/etc/init.d
	$(QUALCOMM_QCA6174_INSTALL_CMDS)
	$(QUALCOMM_QCA9377_INSTALL_CMDS)
endef


$(eval $(generic-package))
