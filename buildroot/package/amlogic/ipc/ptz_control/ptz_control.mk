################################################################################
#
# ptz control
#
################################################################################

PTZ_CONTROL_VERSION = 1.0
PTZ_CONTROL_SITE = $(PTZ_CONTROL_PKGDIR)/ptz_control-$(PTZ_CONTROL_VERSION)
PTZ_CONTROL_SITE_METHOD = local

define PTZ_CONTROL_BUILD_CMDS
    $(MAKE) CC=$(TARGET_CXX) -C $(@D) all
endef

define PTZ_CONTROL_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/ptz_control $(TARGET_DIR)/usr/bin/ptz_control
endef

define PTZ_CONTROL_INSTALL_CLEAN_CMDS
    $(MAKE) CC=$(TARGET_CXX) -C $(@D) clean
endef

$(eval $(generic-package))
