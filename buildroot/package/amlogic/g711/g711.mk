#############################################################
#
# aml_dsp_util
#
#############################################################
G711_SITE=$(G711_PKGDIR)/src
G711_SITE_METHOD=local
AML_DSP_UTIL_BUILD_DIR = $(BUILD_DIR)
AML_DSP_UTIL_INSTALL_STAGING = YES

define G711_BUILD_CMDS
  $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) all
endef

define G711_INSTALL_TARGET_CMDS
  $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) install
endef

$(eval $(generic-package))
