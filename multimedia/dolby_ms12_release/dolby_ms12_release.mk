#############################################################
#
# Dolby Audio Release for Soundbar Product.
#
#############################################################
DOLBY_MS12_RELEASE_VERSION:=1.0.0
DOLBY_MS12_RELEASE_SITE=$(TOPDIR)/../multimedia/dolby_ms12_release/src
DOLBY_MS12_RELEASE_SITE_METHOD=local


ifeq ($(BR2_aarch64),y)
export ENABLE_MS12_64bit = yes
endif


define DOLBY_MS12_RELEASE_BUILD_CMDS
	$(MAKE) CC=$(TARGET_CC) -C $(@D) all
endef

define DOLBY_MS12_RELEASE_CLEAN_CMDS
	$(MAKE) -C $(@D) clean
endef

define DOLBY_MS12_RELEASE_INSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) install
endef

define DOLBY_MS12_RELEASE_UNINSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) uninstall
endef

$(eval $(generic-package))
