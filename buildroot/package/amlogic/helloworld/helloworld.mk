#
# helloworld
#
HELLOWORLD_VERSION = 1.0
HELLOWORLD_SITE = $(HELLOWORLD_PKGDIR)/src
HELLOWORLD_SITE_METHOD = local

define HELLOWORLD_BUILD_CMDS
    $(MAKE) CC=$(TARGET_CXX) -C $(@D) all
endef

define HELLOWORLD_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/helloworld $(TARGET_DIR)/usr/bin/helloworld
endef

define HELLOWORLD_INSTALL_CLEAN_CMDS
    $(MAKE) CC=$(TARGET_CXX) -C $(@D) clean
endef

$(eval $(generic-package))
