################################################################################
#
#secure firmware load
#
################################################################################

SECFIRMLOAD_VERSION = 1.0
SECFIRMLOAD_SITE = $(TOPDIR)/../multimedia/secfirmload/secloadbin
SECFIRMLOAD_SITE_METHOD = local

define SECFIRMLOAD_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/$(BR2_ARCH).$(CC_TARGET_ABI_).$(CC_TARGET_FLOAT_ABI_)/tee_preload_fw     $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D -m 644 $(@D)/$(BR2_ARCH).$(CC_TARGET_ABI_).$(CC_TARGET_FLOAT_ABI_)/tee_preload_fw.so  $(TARGET_DIR)/usr/lib/
	$(INSTALL) -D -m 644 $(@D)/noarch/ta/526fc4fc-7ee6-4a12-96e3-83da9565bce8.ta   $(TARGET_DIR)/lib/teetz/

	$(SECVIDEO_PREBUILT_INSTALL_INIT_SYSV)
endef

ifeq ($(BR2_PACKAGE_LAUNCHER_USE_SECFIRMLOAD), y)
define SECVIDEO_PREBUILT_INSTALL_INIT_SYSV
	$(INSTALL) -D -m 755 $(@D)/noarch/etc/S60secload $(TARGET_DIR)/etc/init.d/
endef
endif

$(eval $(generic-package))
