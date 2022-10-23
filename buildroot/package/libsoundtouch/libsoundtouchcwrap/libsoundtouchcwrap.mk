
#############################################################
#
# libsoundtouchcwrap
#
#############################################################

LIBSOUNDTOUCHCWRAP_VERSION = 2.1.1
LIBSOUNDTOUCHCWRAP_SITE_METHOD = local
LIBSOUNDTOUCHCWRAP_SITE = $(TOPDIR)/package/libsoundtouch/libsoundtouchcwrap
LIBSOUNDTOUCHCWRAP_DEPENDENCIES = libsoundtouch

define LIBSOUNDTOUCHCWRAP_BUILD_CMDS
	PKG_CONFIG_SYSROOT_DIR="$(STAGING_DIR)" PKG_CONFIG="$(PKG_CONFIG_HOST_BINARY)" PKG_CONFIG_PATH="$(STAGING_DIR)/usr/lib/pkgconfig:$(PKG_CONFIG_PATH)" $(TARGET_MAKE_ENV)$(TARGET_MAKE_ENV) $(MAKE) CC=$(TARGET_CC) CXX=$(TARGET_CXX) -C $(@D)
endef

define LIBSOUNDTOUCHCWRAP_INSTALL_TARGET_CMDS
	$(INSTALL) -m 755 -D $(@D)/libsoundtouchcwrap.so $(TARGET_DIR)/usr/lib/
endef

$(eval $(generic-package))
