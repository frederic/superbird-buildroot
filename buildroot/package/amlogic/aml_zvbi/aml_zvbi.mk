#############################################################
#
# aml zvbi libs
#
#############################################################
AML_ZVBI_VERSION = 1.0
AML_ZVBI_SITE = $(TOPDIR)/../vendor/amlogic/zvbi
AML_ZVBI_SITE_METHOD = local
AML_ZVBI_INSTALL_STAGING = YES
#AML_ZVBI_DEPENDENCIES = aml_dvb

define AML_ZVBI_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) ARCH_IS_64=$(BR2_ARCH_IS_64) $(MAKE) -C $(@D) all
endef

define AML_ZVBI_INSTALL_TARGET_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) install
endef

define AML_ZVBI_CLEAN_CMDS
	$(MAKE) -C $(@D) clean
endef

$(eval $(generic-package))
