#############################################################
#
# aml_dsp_util
#
#############################################################
AMR_VERSION:=g.1
AMR_SITE=$(AMR_PKGDIR)/src
AMR_SITE_METHOD=local
AML_DSP_UTIL_BUILD_DIR = $(BUILD_DIR)
AML_DSP_UTIL_INSTALL_STAGING = YES

define AMR_BUILD_CMDS
  $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) all
endef

define AMR_INSTALL_TARGET_CMDS
  $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) install
endef

$(eval $(generic-package))
