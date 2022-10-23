#############################################################
#
# upnp av
#
#############################################################
appName=upnp-app
UPNP_APP_VERSION:=1.0.0
UPNP_APP_SITE=$(TOPDIR)/../vendor/amlogic/external/platinum/upnp-app/src
UPNP_APP_SITE_METHOD=local
UPNP_APP_BUILD_DIR = $(BUILD_DIR)
UPNP_APP_INSTALL_STAGING = YES
UPNP_APP_DEPENDENCIES = gstreamer1 gst1-plugins-base host-pkgconf host-scons
CFLAGS = -W1,-fPIC
CURRENT_DIR = $(UPNP_APP_BUILD_DIR)/$(appName)-$(UPNP_APP_VERSION)/

OUT_BIN = $(TARGET_DIR)/sbin
define UPNP_APP_BUILD_CMDS
	cd $(CURRENT_DIR);${SCONS} -c target=arm-unknown-linux outdir=$(OUT_BIN);${SCONS} target=arm-unknown-linux dir=$(STAGING_DIR) outdir=$(OUT_BIN) \
	build_config=Debug gcctool=$(TARGET_CC)
endef

define UPNP_APP_INSTALL_TARGET_CMDS
    $(INSTALL) -m 0755 -D $(TOPDIR)/../vendor/amlogic/external/platinum/upnp-app/S79dlna-sync $(TARGET_DIR)/etc/init.d
endef

define UPNP_APP_CLEAN_CMDS
	cd $(CURRENT_DIR);${SCONS} -c target=arm-unknown-linux outdir=$(OUT_BIN)
endef

$(eval $(generic-package))
