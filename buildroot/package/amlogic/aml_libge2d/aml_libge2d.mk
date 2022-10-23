################################################################################
#
# libge2d
#
################################################################################

ifneq ($(BR2_PACKAGE_AML_LIBGE2D_LOCAL_PATH),)

AML_LIBGE2D_VERSION:= 1.0.0
AML_LIBGE2D_SITE := $(call qstrip,$(BR2_PACKAGE_AML_LIBGE2D_LOCAL_PATH))
AML_LIBGE2D_SITE_METHOD:=local
AML_LIBGE2D_DEPENDENCIES = aml_libion
AML_LIBGE2D_INSTALL_STAGING:=YES

define AML_LIBGE2D_BUILD_CMDS
    $(MAKE) CC=$(TARGET_CC) -C $(@D)
endef

define AML_LIBGE2D_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0644 $(@D)/libge2d/libge2d.so $(TARGET_DIR)/usr/lib/
    $(INSTALL) -D -m 0755 $(@D)/ge2d_feature_test  $(TARGET_DIR)/usr/bin/
endef

define AML_LIBGE2D_INSTALL_STAGING_CMDS
    $(INSTALL) -D -m 0644 $(@D)/libge2d/libge2d.so \
        $(STAGING_DIR)/usr/lib/libge2d.so
    $(INSTALL) -m 0644 $(@D)/libge2d/include/aml_ge2d.h \
        $(STAGING_DIR)/usr/include/
    $(INSTALL) -m 0644 $(@D)/libge2d/include/ge2d_port.h \
        $(STAGING_DIR)/usr/include/
endef

define AML_LIBGE2D_INSTALL_CLEAN_CMDS
    $(MAKE) CC=$(TARGET_CC) -C $(@D) clean
endef

endif
$(eval $(generic-package))
