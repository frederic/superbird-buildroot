#############################################################
#
# IPC REFAPP
#
#############################################################

IPC_REFAPP_VERSION = 1.0
IPC_REFAPP_SITE = $(TOPDIR)/../vendor/amlogic/ipc/ipc-refapp
IPC_REFAPP_SITE_METHOD = local
IPC_REFAPP_INSTALL_STAGING = NO
IPC_REFAPP_DEPENDENCIES = host-pkgconf gstreamer1 gst1-rtsp-server gst1-plugins-base gst1-plugins-good
IPC_REFAPP_DEPENDENCIES += ipc-property ipc-sdk
IPC_REFAPP_DEPENDENCIES += sqlite libjpeg libwebsockets
IPC_REFAPP_DEPENDENCIES += aml_nn_detect
IPC_REFAPP_DEPENDENCIES += libgpiod

define IPC_REFAPP_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(TARGET_MAKE_ENV) $(MAKE) -C $(@D)
endef

define IPC_REFAPP_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/etc/ipc
	$(INSTALL) -D -m 755 $(@D)/ipc-refapp $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D -m 755 $(@D)/startup.sh $(TARGET_DIR)/etc/init.d/S81ipc-refapp
	$(INSTALL) -D -m 644 $(@D)/config.json $(TARGET_DIR)/etc/ipc/
	$(INSTALL) -D -m 755 $(@D)/ircut-gpio-detect.sh $(TARGET_DIR)/etc/ipc/
endef

$(eval $(generic-package))
