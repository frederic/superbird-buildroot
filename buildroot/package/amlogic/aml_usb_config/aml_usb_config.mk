################################################################################
#
# aml usb config
#
################################################################################
AML_USB_CONNFIG_VERSION = 20171218
AML_USB_CONFIG_SITE_METHOD = local
AML_USB_CONFIG_SITE = $(AML_USB_CONFIG_PKGDIR)/src

ifeq ($(BR2_PACKAGE_ADB_RNDIS), y)
define ADB_RNDIS_FUNCTION_CMDS
	cp $(@D)/S89usbgadget_adb_rndis  $(TARGET_DIR)/etc/init.d/S89usbgadget
endef
endif

ifeq ($(BR2_PACKAGE_MASS_STORAGE), y)
define MASS_STORAGE_FUNCTION_CMDS
	cp $(@D)/S89usbgadget_udisk  $(TARGET_DIR)/etc/init.d/S89usbgadget
endef
endif

define AML_USB_CONFIG_INSTALL_TARGET_CMDS
	$(ADB_RNDIS_FUNCTION_CMDS)
	$(MASS_STORAGE_FUNCTION_CMDS)
endef

$(eval $(generic-package))
