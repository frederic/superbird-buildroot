#############################################################
#
# lightdaemon
#
#############################################################
LEDRING_VERSION = 20170901
LEDRING_SITE = $(TOPDIR)/../vendor/amlogic/ledring
LEDRING_SITE_METHOD = local
LEDRING_DEPENDENCIES = popt

define LEDRING_BUILD_CMDS
    $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) all
endef

define LEDRING_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(LEDRING_PKGDIR)/S02lightDaemon \
        $(TARGET_DIR)/etc/init.d/S02lightDaemon
    $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) install
endef

$(eval $(generic-package))
