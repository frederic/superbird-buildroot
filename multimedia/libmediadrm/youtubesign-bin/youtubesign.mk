################################################################################
#
# youtubesign prebuilt
#
################################################################################


YOUTUBESIGN_BIN_VERSION = 1.0
YOUTUBESIGN_BIN_SITE = $(TOPDIR)/../multimedia/libmediadrm/youtubesign-bin/prebuilt
YOUTUBESIGN_BIN_SITE_METHOD = local
YOUTUBESIGN_BIN_INSTALL_TARGET := YES
YOUTUBESIGN_BIN_INSTALL_STAGING := YES
YOUTUBESIGN_BIN_DEPENDENCIES = tdk

CC_TARGET_ABI_:= $(call qstrip,$(BR2_GCC_TARGET_ABI))
CC_TARGET_FLOAT_ABI_ := $(call qstrip,$(BR2_GCC_TARGET_FLOAT_ABI))

define YOUTUBESIGN_BIN_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0644 $(@D)/$(BR2_ARCH).$(CC_TARGET_ABI_).$(CC_TARGET_FLOAT_ABI_)/*.so $(STAGING_DIR)/usr/lib/
	$(INSTALL) -D -m 0644 $(@D)/noarch/include/* $(STAGING_DIR)/usr/include/
endef

define YOUTUBESIGN_BIN_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/noarch/ta/*.ta $(TARGET_DIR)/lib/teetz/
	$(INSTALL) -D -m 0644 $(@D)/$(BR2_ARCH).$(CC_TARGET_ABI_).$(CC_TARGET_FLOAT_ABI_)/*.so $(TARGET_DIR)/usr/lib/
endef

$(eval $(generic-package))
