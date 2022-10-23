################################################################################
#
# libMULTIENC
#
################################################################################

ifneq ($(BR2_PACKAGE_LIBMULTIENC_LOCAL_PATH),)

LIBMULTIENC_VERSION = 1.0
LIBMULTIENC_SITE := $(call qstrip,$(BR2_PACKAGE_LIBMULTIENC_LOCAL_PATH))
LIBMULTIENC_SITE_METHOD = local
LIBMULTIENC_DEPENDENCIES = aml_libge2d
LIBMULTIENC_LICENSE = LGPL
LIBMULTIENC_INSTALL_STAGING = YES

# This package uses the AML_LIBS_STAGING_DIR variable to construct the
# header and library paths used when compiling
define LIBMULTIENC_BUILD_CMDS
	#
	$(TARGET_MAKE_ENV) CC=$(TARGET_CC) CXX=$(TARGET_CXX)\
	$(MAKE)	-C $(@D)/vpuapi
	#
	$(TARGET_MAKE_ENV) CC=$(TARGET_CC) CXX=$(TARGET_CXX)\
	$(MAKE)	-C $(@D)/amvenc_lib
	# build test app
	$(TARGET_MAKE_ENV) CC=$(TARGET_CC) CXX=$(TARGET_CXX)\
	$(MAKE)	-C $(@D)/amvenc_test
endef

define LIBMULTIENC_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/vpuapi/libamvenc_api.so  $(STAGING_DIR)/usr/lib/
	$(INSTALL) -D -m 0755 $(@D)/amvenc_lib/libvpcodec.so $(STAGING_DIR)/usr/lib/
	$(INSTALL) -D -m 0755 $(@D)/amvenc_lib/include/vp_multi_codec_1_0.h   $(STAGING_DIR)/usr/include/
endef

define LIBMULTIENC_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/vpuapi/libamvenc_api.so  $(TARGET_DIR)/usr/lib/
	$(INSTALL) -D -m 0755 $(@D)/amvenc_lib/libvpcodec.so  $(TARGET_DIR)/usr/lib/
	$(INSTALL) -D -m 0755 $(@D)/amvenc_test/aml_enc_test  $(TARGET_DIR)/usr/bin/
endef

endif
$(eval $(generic-package))
