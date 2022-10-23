#############################################################
#
# Dolby Audio for Wireless Speakers
#
#############################################################
DOLBY_ATMOS_RELEASE_VERSION:=1.0.0
DOLBY_ATMOS_RELEASE_SITE=$(TOPDIR)/../multimedia/dolby_atmos_release/src
DOLBY_ATMOS_RELEASE_SITE_METHOD=local


ifeq ($(BR2_aarch64),y)
export ENABLE_ATMOS_64bit = yes
endif


define DOLBY_ATMOS_RELEASE_BUILD_CMDS
	$(MAKE) CC=$(TARGET_CC) -C $(@D) all
endef

define DOLBY_ATMOS_RELEASE_CLEAN_CMDS
	$(MAKE) -C $(@D) clean
endef

define DOLBY_ATMOS_RELEASE_INSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) install
endef

define DOLBY_ATMOS_RELEASE_UNINSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) uninstall
endef

$(eval $(generic-package))
