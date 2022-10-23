#############################################################
#
# Dolby Audio for Wireless Speakers
#
#############################################################
DOLBY_DAWS_PLUGIN_VERSION:=1.0.0
DOLBY_DAWS_PLUGIN_SITE=$(TOPDIR)/../multimedia/aml_plugins/trumpet/dolby_daws_plugin/src
DOLBY_DAWS_PLUGIN_SITE_METHOD=local

define DOLBY_DAWS_PLUGIN_BUILD_CMDS
	$(MAKE) CC=$(TARGET_CC) CXX=$(TARGET_CXX) -C $(@D)
endef

define DOLBY_DAWS_PLUGIN_CLEAN_CMDS
	$(MAKE) -C $(@D) clean
endef

define DOLBY_DAWS_PLUGIN_INSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) install
endef

define DOLBY_DAWS_PLUGIN_UNINSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) uninstall
endef

$(eval $(generic-package))