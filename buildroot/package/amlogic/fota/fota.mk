#############################################################
#
# amlogic fota
#
# It can download related ota package from ota server
#
#############################################################
FOTA_VERSION = 1.0
FOTA_SITE = $(TOPDIR)/../vendor/amlogic/ota/fota
FOTA_SITE_METHOD = local

define FOTA_BUILD_CMDS
    $(MAKE) CC=$(TARGET_CC) ARCH=$(ARCH) -C $(@D) all
endef

define FOTA_INSTALL_TARGET_CMDS
    $(MAKE) -C $(@D) install
endef

$(eval $(generic-package))
