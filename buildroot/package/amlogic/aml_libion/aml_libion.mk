#
# aml_libion
#
ifneq ($(BR2_PACKAGE_AML_LIBION_LOCAL_PATH),)

AML_LIBION_VERSION = 1.0
AML_LIBION_SITE := $(call qstrip,$(BR2_PACKAGE_AML_LIBION_LOCAL_PATH))
AML_LIBION_SITE_METHOD = local
AML_LIBION_INSTALL_STAGING := YES

define AML_LIBION_BUILD_CMDS
    $(TARGET_MAKE_ENV) $(MAKE) CC=$(TARGET_CC) -C $(@D)
endef

define AML_LIBION_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0644 $(@D)/libion.so $(TARGET_DIR)/usr/lib/libion.so
    $(INSTALL) -D -m 0755 $(@D)/iontest $(TARGET_DIR)/usr/bin/iontest
endef

define AML_LIBION_INSTALL_STAGING_CMDS
    $(INSTALL) -D -m 0644 $(@D)/libion.so $(STAGING_DIR)/usr/lib/libion.so
    $(INSTALL) -m 0644 $(@D)/include/ion/ion.h $(STAGING_DIR)/usr/include/
    $(INSTALL) -m 0644 $(@D)/include/ion/IONmem.h $(STAGING_DIR)/usr/include/
endef

define AML_LIBION_INSTALL_CLEAN_CMDS
    $(MAKE) CC=$(TARGET_CXX) -C $(@D) clean
endef

endif
$(eval $(generic-package))
