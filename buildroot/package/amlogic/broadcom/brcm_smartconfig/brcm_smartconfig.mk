################################################################################
#
# bcm_smartconfig
#
################################################################################

BRCM_SMARTCONFIG_VERSION = 3.70
ifeq ($(call qstrip, $(BR2_PACKAGE_BRCM_SMARTCONFIG_LOCAL_PATH)),)
BR2_PACKAGE_BRCM_SMARTCONFIG_LOCAL_PATH = $(TOPDIR)/dummy
endif
BRCM_SMARTCONFIG_SITE = $(call qstrip,$(BR2_PACKAGE_BRCM_SMARTCONFIG_LOCAL_PATH))
BRCM_SMARTCONFIG_SITE_METHOD = local


define BRCM_SMARTCONFIG_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/jni CC=$(TARGET_CC)
endef

define BRCM_SMARTCONFIG_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/jni/setup $(TARGET_DIR)/usr/bin/brcm_smartconfig
	$(INSTALL) -D -m 755 $(BRCM_SMARTCONFIG_PKGDIR)/brcm_smartconfig.sh $(TARGET_DIR)/usr/bin
endef


$(eval $(generic-package))
