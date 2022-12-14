################################################################################
#
# widevine prebuilt
#
################################################################################

ifeq ($(BR2_PACKAGE_WIDEVINE_BIN_VERSION_10),y)
WIDEVINE_BIN_VERSION = 10
else ifeq ($(BR2_PACKAGE_WIDEVINE_BIN_VERSION_14),y)
WIDEVINE_BIN_VERSION = 14
else ifeq ($(BR2_PACKAGE_WIDEVINE_BIN_VERSION_15),y)
WIDEVINE_BIN_VERSION = 15
endif
WIDEVINE_BIN_SITE = $(TOPDIR)/../multimedia/libmediadrm/widevine-bin/prebuilt-v$(WIDEVINE_BIN_VERSION)
WIDEVINE_BIN_SITE_METHOD = local
WIDEVINE_BIN_INSTALL_TARGET := YES
WIDEVINE_BIN_INSTALL_STAGING := YES
WIDEVINE_BIN_DEPENDENCIES = tdk
CC_TARGET_ABI_:= $(call qstrip,$(BR2_GCC_TARGET_ABI))
CC_TARGET_FLOAT_ABI_ := $(call qstrip,$(BR2_GCC_TARGET_FLOAT_ABI))

define WIDEVINE_BIN_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0644 $(@D)/$(BR2_ARCH).$(CC_TARGET_ABI_).$(CC_TARGET_FLOAT_ABI_)/*.so $(STAGING_DIR)/usr/lib/
	mkdir -p $(STAGING_DIR)/usr/include/widevine
        $(INSTALL) -D -m 0644 $(@D)/noarch/include/* $(STAGING_DIR)/usr/include/widevine/
endef

define WIDEVINE_BIN_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/noarch/ta/*.ta $(TARGET_DIR)/lib/teetz/
	$(INSTALL) -D -m 0644 $(@D)/$(BR2_ARCH).$(CC_TARGET_ABI_).$(CC_TARGET_FLOAT_ABI_)/*.so $(TARGET_DIR)/usr/lib/
	$(INSTALL) -D -m 0755 $(@D)/$(BR2_ARCH).$(CC_TARGET_ABI_).$(CC_TARGET_FLOAT_ABI_)/widevine_ce_cdm_unittest $(TARGET_DIR)/usr/bin/
endef

$(eval $(generic-package))
