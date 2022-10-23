#
# LIBLOG
#
LIBLOG_VERSION = 1.0
LIBLOG_SITE = $(TOPDIR)/../vendor/amlogic/aml_commonlib/liblog
LIBLOG_SITE_METHOD = local
LIBLOG_INSTALL_STAGING = YES

define LIBLOG_BUILD_CMDS
    $(MAKE) CC=$(TARGET_CXX) -C $(@D) all
endef

define LIBLOG_INSTALL_TARGET_CMDS
    $(MAKE) CC=$(TARGET_CXX) -C $(@D) install
endef

define LIBLOG_INSTALL_CLEAN_CMDS
    $(MAKE) CC=$(TARGET_CXX) -C $(@D) clean
endef

$(eval $(generic-package))
