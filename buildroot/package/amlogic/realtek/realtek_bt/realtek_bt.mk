################################################################################
#
# realtek wifi/bt
#
################################################################################

REALTEK_BT_VERSION = 4.9
ifeq ($(call qstrip, $(BR2_PACKAGE_REALTEK_BT_LOCAL_PATH)),)
BR2_PACKAGE_REALTEK_BT_LOCAL_PATH = $(TOPDIR)/dummy
endif
REALTEK_BT_SITE = $(call qstrip,$(BR2_PACKAGE_REALTEK_BT_LOCAL_PATH))
REALTEK_BT_SITE_METHOD = local

REALTEK_BT_MODULE_DIR = kernel/realtek/bt
REALTEK_BT_MODULE_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(REALTEK_BT_MODULE_DIR)

REALTEK_BT_SYSTEM_CONFIG_DIR = $(REALTEK_BT_PKGDIR)/.

REALTEK_BT_DEPENDENCIES = linux

ifeq ($(BR2_PACKAGE_REALTEK_UART_BT),y)
define REALTEK_UART_BT_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/uart_driver ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_KERNEL_CROSS)
	$(MAKE) -C $(@D)/rtk_hciattach CC=$(TARGET_CC)
endef

define REALTEK_UART_BT_INSTALL_CMDS
	$(INSTALL) -D -m 0644 $(@D)/uart_driver/hci_uart.ko $(REALTEK_BT_MODULE_INSTALL_DIR)/rtk_btuart.ko
	$(INSTALL) -D -m 755 $(@D)/rtk_hciattach/rtk_hciattach $(TARGET_DIR)/usr/sbin/
	echo $(REALTEK_BT_MODULE_DIR)/rtk_btuart.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
endif

ifeq ($(BR2_PACKAGE_REALTEK_USB_BT),y)
define REALTEK_USB_BT_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/usb_driver ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_KERNEL_CROSS)
endef

define REALTEK_USB_BT_INSTALL_CMDS
	$(INSTALL) -D -m 0644 $(@D)/usb_driver/rtk_btusb.ko $(REALTEK_BT_MODULE_INSTALL_DIR)
	echo $(REALTEK_BT_MODULE_DIR)/rtk_btusb.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
endif

define REALTEK_BT_BUILD_CMDS
	$(REALTEK_UART_BT_BUILD_CMDS)
	$(REALTEK_USB_BT_BUILD_CMDS)
endef

define REALTEK_BT_INSTALL_TARGET_CMDS
	mkdir -p $(REALTEK_BT_MODULE_INSTALL_DIR)
	$(REALTEK_UART_BT_INSTALL_CMDS)
	$(REALTEK_USB_BT_INSTALL_CMDS)

    #install firmware
	mkdir -p $(TARGET_DIR)/lib/firmware/rtlbt
	$(INSTALL) -D -m 0644 $(@D)/fw/* $(TARGET_DIR)/lib/firmware/rtlbt
	$(INSTALL) -D -m 755 ${REALTEK_BT_SYSTEM_CONFIG_DIR}/S44bluetooth $(TARGET_DIR)/etc/init.d
endef


$(eval $(generic-package))
