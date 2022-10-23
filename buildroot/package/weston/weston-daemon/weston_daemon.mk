#############################################################
#
# weston_daemon
#
#############################################################
WESTON_DAEMON_VERSION = 1.0
WESTON_DAEMON_SITE = $(TOPDIR)/package/weston/weston-daemon
WESTON_DAEMON_SITE_METHOD = local
WESTON_DAEMON_INSTALL_STAGING = NO

define WESTON_DAEMON_INSTALL_TARGET_CMDS
	cp -df $(WESTON_DAEMON_DIR)/aml-weston.ini $(TARGET_DIR)/etc/
	cp -df $(WESTON_DAEMON_DIR)/S51Weston $(TARGET_DIR)/etc/init.d/
endef


$(eval $(generic-package))

