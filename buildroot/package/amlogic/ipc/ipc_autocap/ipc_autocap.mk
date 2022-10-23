#############################################################
#
# IPC AUTOCAP
#
#############################################################

IPC_AUTOCAP_VERSION = 0.1
IPC_AUTOCAP_SITE = $(TOPDIR)/../vendor/amlogic/ipc/ipc_autocap
IPC_AUTOCAP_SITE_METHOD = local
IPC_AUTOCAP_DEPENDENCIES = gstreamer1 gst1-plugins-good gst1-plugins-base
IPC_AUTOCAP_DEPENDENCIES += gst-plugin-amlvenc
IPC_AUTOCAP_DEPENDENCIES += ipc-property arm_isp

define IPC_AUTOCAP_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(TARGET_MAKE_ENV) $(MAKE) -C $(@D)
endef

define IPC_AUTOCAP_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/ipc_autocap $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D -m 755 $(IPC_AUTOCAP_PKGDIR)/S80ipc_autocap $(TARGET_DIR)/etc/init.d/
endef

$(eval $(generic-package))
