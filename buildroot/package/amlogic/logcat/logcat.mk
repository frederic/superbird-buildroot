#
# logcat
#
LOGCAT_VERSION = 1.0
LOGCAT_SITE = $(TOPDIR)/../vendor/amlogic/aml_commonlib/logcat
LOGCAT_SITE_METHOD = local
LOGCAT_DEPENDENCIES += liblog

define LOGCAT_BUILD_CMDS
    $(MAKE) CC=$(TARGET_CXX) -C $(@D) all
endef

define LOGCAT_INSTALL_TARGET_CMDS
    $(MAKE) CC=$(TARGET_CXX) -C $(@D) install
	install -m 755 $(LOGCAT_PKGDIR)/S82remaplog $(TARGET_DIR)/etc/init.d/
endef

define LOGCAT_INSTALL_CLEAN_CMDS
    $(MAKE) CC=$(TARGET_CXX) -C $(@D) clean
endef

$(eval $(generic-package))
