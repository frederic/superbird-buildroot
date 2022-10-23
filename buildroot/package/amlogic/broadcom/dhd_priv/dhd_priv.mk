################################################################################
#
# dhd private tools
#
################################################################################
DHD_PRIV_VERSION = 1.1
ifeq ($(findstring amlogic-4.19-dev,$(BR2_LINUX_KERNEL_VERSION)),amlogic-4.19-dev)
DHD_PRIV_SITE = $(TOPDIR)/../hardware/aml-4.19/amlogic/wifi/bcm_ampak/tools/dhd_priv
else
DHD_PRIV_SITE = $(TOPDIR)/../hardware/aml-4.9/amlogic/wifi/bcm_ampak/tools/dhd_priv
endif
DHD_PRIV_SITE_METHOD = local



define DHD_PRIV_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) CC=$(TARGET_CC)
endef

define DHD_PRIV_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/dhd_priv $(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))
