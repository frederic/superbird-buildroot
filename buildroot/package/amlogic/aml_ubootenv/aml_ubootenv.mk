#############################################################
#
# aml ubootenv
#
#############################################################
AML_UBOOTENV_VERSION = 0.1
AML_UBOOTENV_SITE = $(TOPDIR)/../vendor/amlogic/aml_commonlib/ubootenv
AML_UBOOTENV_SITE_METHOD = local
AML_UBOOTENV_DEPENDENCIES += libzlib

define AML_UBOOTENV_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) CFLAGS=$(FLAGS) -C $(@D) all
endef

define AML_UBOOTENV_INSTALL_TARGET_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) CC=$(TARGET_CC) -C $(@D) install
endef

$(eval $(generic-package))
