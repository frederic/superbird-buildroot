################################################################################
#
# libvpcodec
#
################################################################################

ifneq ($(BR2_PACKAGE_LIBVPCODEC_LOCAL_PATH),)

LIBVPCODEC_VERSION = 1.0
LIBVPCODEC_SITE := $(call qstrip,$(BR2_PACKAGE_LIBVPCODEC_LOCAL_PATH))
LIBVPCODEC_SITE_METHOD = local
LIBVPCODEC_DEPENDENCIES =
LIBVPCODEC_LICENSE = LGPL
LIBVPCODEC_INSTALL_STAGING = YES

LIBVPCODEC_BUILD_DIR = bjunion_enc
# This package uses the AML_LIBS_STAGING_DIR variable to construct the
# header and library paths used when compiling
define LIBVPCODEC_BUILD_CMDS
	$(TARGET_MAKE_ENV) CC=$(TARGET_CC) CXX=$(TARGET_CXX)\
	$(MAKE)	-C $(@D)/$(LIBVPCODEC_BUILD_DIR)
	# build test app
	$(TARGET_MAKE_ENV) CC=$(TARGET_CC) CXX=$(TARGET_CXX)\
	$(MAKE)	-C $(@D)/$(LIBVPCODEC_BUILD_DIR)/../
endef

define LIBVPCODEC_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/$(LIBVPCODEC_BUILD_DIR)/vpcodec_1_0.h $(STAGING_DIR)/usr/include/
	$(INSTALL) -D -m 0755 $(@D)/$(LIBVPCODEC_BUILD_DIR)/libvpcodec.so $(STAGING_DIR)/usr/lib/
endef

define LIBVPCODEC_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/$(LIBVPCODEC_BUILD_DIR)/libvpcodec.so $(TARGET_DIR)/usr/lib/
endef

endif
$(eval $(generic-package))
