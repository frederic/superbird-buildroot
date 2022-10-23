#############################################################
#
# IPC System Service
#
#############################################################

IPC_SYSTEM_SERVICE_VERSION = 1.0
IPC_SYSTEM_SERVICE_SITE = $(TOPDIR)/../vendor/amlogic/ipc/ipc_system_service
IPC_SYSTEM_SERVICE_SITE_METHOD = local

define IPC_SYSTEM_SERVICE_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/etc/ipc
	$(INSTALL) -D -m 755 $(@D)/ipc-system-service $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D -m 755 $(@D)/startup.sh $(TARGET_DIR)/etc/init.d/S45ipc-system-service
	cp -af $(@D)/scripts $(TARGET_DIR)/etc/ipc/
endef

$(eval $(cmake-package))
