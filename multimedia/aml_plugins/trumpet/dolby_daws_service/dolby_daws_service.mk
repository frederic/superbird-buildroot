#############################################################
#
# Dolby Audio for Wireless Speakers
#
#############################################################
DOLBY_DAWS_SERVICE_VERSION:=1.0.0
DOLBY_DAWS_SERVICE_SITE=$(TOPDIR)/../multimedia/aml_plugins/trumpet/dolby_daws_service/src
DOLBY_DAWS_SERVICE_SITE_METHOD=local

define DOLBY_DAWS_SERVICE_BUILD_CMDS
	$(MAKE) CC=$(TARGET_CC) CXX=$(TARGET_CXX) -C $(@D)
endef

define DOLBY_DAWS_SERVICE_CLEAN_CMDS
	$(MAKE) -C $(@D) clean
endef

define DOLBY_DAWS_SERVICE_INSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) install
endef

define DOLBY_DAWS_SERVICE_UNINSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) uninstall
endef

$(eval $(generic-package))