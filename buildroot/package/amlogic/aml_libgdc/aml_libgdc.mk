################################################################################
#
# libgdc
#
################################################################################

ifneq ($(BR2_PACKAGE_AML_LIBGDC_LOCAL_PATH),)

AML_LIBGDC_VERSION:= 1.0.0
AML_LIBGDC_SITE := $(call qstrip,$(BR2_PACKAGE_AML_LIBGDC_LOCAL_PATH))
AML_LIBGDC_SITE_METHOD:=local
AML_LIBGDC_DEPENDENCIES = aml_libion
AML_LIBGDC_INSTALL_STAGING:=YES

define AML_LIBGDC_BUILD_CMDS
    $(MAKE) CC=$(TARGET_CC) -C $(@D)
endef

define AML_LIBGDC_INSTALL_TARGET_CMDS
    mkdir -p $(TARGET_DIR)/etc/gdc_config
    $(INSTALL) -D -m 0644 $(@D)/gdc_config/* $(TARGET_DIR)/etc/gdc_config/
    $(INSTALL) -D -m 0644 $(@D)/libgdc.so $(TARGET_DIR)/usr/lib/
    $(INSTALL) -D -m 0755 $(@D)/gdc_test  $(TARGET_DIR)/usr/bin/
endef

define AML_LIBGDC_INSTALL_STAGING_CMDS
    $(INSTALL) -D -m 0644 $(@D)/libgdc.so \
        $(STAGING_DIR)/usr/lib/libgdc.so
    $(INSTALL) -m 0644 $(@D)/include/gdc/gdc_api.h \
        $(STAGING_DIR)/usr/include/
endef

define AML_LIBGDC_INSTALL_CLEAN_CMDS
    $(MAKE) CC=$(TARGET_CC) -C $(@D) clean
endef

endif
$(eval $(generic-package))
