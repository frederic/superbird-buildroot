#############################################################
#
# aml_launcher
#
#############################################################
#AML_LAUNCHER_VERSION = 2017
AML_LAUNCHER_SITE = $(AML_LAUNCHER_PKGDIR).
AML_LAUNCHER_SITE_METHOD = local

ifeq ($(BR2_PACKAGE_LAUNCHER_NONE),y)
define AML_LAUNCHER_INSTALL_TARGET_CMDS
	rm -rf $(TARGET_DIR)/etc/init.d/S90*
endef

else ifeq ($(BR2_PACKAGE_LAUNCHER_USE_DIRECTFB),y)
define AML_LAUNCHER_INSTALL_INIT_SYSV
        rm -rf $(TARGET_DIR)/etc/init.d/S90*
        $(INSTALL) -D -m 755 $(AML_LAUNCHER_PKGDIR)/S90dfb \
                $(TARGET_DIR)/etc/init.d/S90dfb
        $(INSTALL) -D -m 755 $(AML_LAUNCHER_PKGDIR)/launcher.jpg \
                $(TARGET_DIR)/etc/launcher.jpg
endef
endif

$(eval $(generic-package))
