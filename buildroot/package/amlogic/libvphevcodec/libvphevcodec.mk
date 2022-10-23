################################################################################
#
# libvphevcodec
#
################################################################################

ifneq ($(BR2_PACKAGE_LIBVPHEVCODEC_LOCAL_PATH),)

LIBVPHEVCODEC_VERSION = 1.0
LIBVPHEVCODEC_SITE := $(call qstrip,$(BR2_PACKAGE_LIBVPHEVCODEC_LOCAL_PATH))
LIBVPHEVCODEC_SITE_METHOD = local
LIBVPHEVCODEC_DEPENDENCIES = aml_libge2d
LIBVPHEVCODEC_LICENSE = LGPL
LIBVPHEVCODEC_INSTALL_STAGING = YES

LIBVPHEVCODEC_BUILD_DIR = EncoderAPI-HEVC/hevc_enc
# This package uses the AML_LIBS_STAGING_DIR variable to construct the
# header and library paths used when compiling
define LIBVPHEVCODEC_BUILD_CMDS
	$(TARGET_MAKE_ENV) CC=$(TARGET_CC) CXX=$(TARGET_CXX)\
	$(MAKE)	-C $(@D)/$(LIBVPHEVCODEC_BUILD_DIR)
	# build test app
	$(TARGET_MAKE_ENV) CC=$(TARGET_CC) CXX=$(TARGET_CXX)\
	$(MAKE)	-C $(@D)/$(LIBVPHEVCODEC_BUILD_DIR)/../
endef

define LIBVPHEVCODEC_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/$(LIBVPHEVCODEC_BUILD_DIR)/vp_hevc_codec_1_0.h $(STAGING_DIR)/usr/include/
	$(INSTALL) -D -m 0755 $(@D)/$(LIBVPHEVCODEC_BUILD_DIR)/libvphevcodec.so $(STAGING_DIR)/usr/lib/
endef

define LIBVPHEVCODEC_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/$(LIBVPHEVCODEC_BUILD_DIR)/libvphevcodec.so $(TARGET_DIR)/usr/lib/
endef

endif
$(eval $(generic-package))
