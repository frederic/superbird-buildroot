###############################################################################
#
# wpe-launcher
#
################################################################################

WPE_LAUNCHER_VERSION = 75524f3779973b8009c8d36c06f65006790f752a
WPE_LAUNCHER_SITE = $(call github,Metrological,wpe-launcher,$(WPE_LAUNCHER_VERSION))

WPE_LAUNCHER_DEPENDENCIES = wpe

define WPE_LAUNCHER_BINS
        mkdir -p $(TARGET_DIR)/etc/wpe
	$(INSTALL) -D -m 0644 $(WPE_LAUNCHER_PKGDIR)/wpe.{txt,conf} $(TARGET_DIR)/etc/wpe
	$(INSTALL) -D -m 0755 $(WPE_LAUNCHER_PKGDIR)/wpe $(TARGET_DIR)/usr/bin
	if [ -f $(WPE_LAUNCHER_PKGDIR)/wpe-update ]; then \
		$(INSTALL) -D -m 0755 $(WPE_LAUNCHER_PKGDIR)/wpe-update $(TARGET_DIR)/usr/bin; \
	fi
endef

ifeq ($(BR2_PACKAGE_LAUNCHER_USE_WPE), y)
define WPE_LAUNCHER_AUTOSTART
        rm -rf $(TARGET_DIR)/etc/init.d/S90*
	$(INSTALL) -D -m 0755 $(WPE_LAUNCHER_PKGDIR)/S90wpe $(TARGET_DIR)/etc/init.d
endef
endif

WPE_LAUNCHER_POST_INSTALL_TARGET_HOOKS += WPE_LAUNCHER_BINS

ifeq ($(BR2_PACKAGE_PLUGIN_WEBKITBROWSER),)
WPE_LAUNCHER_POST_INSTALL_TARGET_HOOKS += WPE_LAUNCHER_AUTOSTART
endif

$(eval $(cmake-package))
