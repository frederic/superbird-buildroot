#
# logwrapper
#
LOGWRAPPER_VERSION = 1.36.146299
LOGWRAPPER_SITE = $(TOPDIR)/package/amlogic/logwrapper
LOGWRAPPER_SITE_METHOD = local
LOGWRAPPER_INSTALL_STAGING = YES

#define LOGCAT_BUILD_CMDS
#    $(MAKE) CC=$(TARGET_CXX) -C $(@D) all
#endef

define LOGWRAPPER_INSTALL_TARGET_CMDS
    cp -rf $(@D)/* $(TARGET_DIR)/bin/
endef

#define LOGCAT_INSTALL_CLEAN_CMDS
#    $(MAKE) CC=$(TARGET_CXX) -C $(@D) clean
#endef

$(eval $(generic-package))
