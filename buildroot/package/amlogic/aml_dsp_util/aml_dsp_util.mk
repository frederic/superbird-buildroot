#############################################################
#
# aml_dsp_util
#
#############################################################
AML_DSP_UTIL_VERSION:=0.0.1
AML_DSP_UTIL_SITE=$(TOPDIR)/../vendor/amlogic/rtos/dsp_util
AML_DSP_UTIL_SITE_METHOD=local
AML_DSP_UTIL_BUILD_DIR = $(BUILD_DIR)
AML_DSP_UTIL_INSTALL_STAGING = YES

define AML_DSP_UTIL_BUILD_CMDS
  $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) all
endef

define AML_DSP_UTIL_INSTALL_TARGET_CMDS
  $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) install
endef

$(eval $(generic-package))
