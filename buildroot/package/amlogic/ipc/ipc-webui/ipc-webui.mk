#############################################################
#
# IPC Webui
#
#############################################################

IPC_WEBUI_VERSION = 1.0
IPC_WEBUI_SITE = $(TOPDIR)/../vendor/amlogic/ipc/ipc_webui
IPC_WEBUI_SITE_METHOD = local
IPC_WEBUI_DEPENDENCIES = nginx php host-gettext
IPC_WEBUI_DEPENDENCIES += ipc-system-service

define IPC_WEBUI_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/etc/ipc/
	mkdir -p $(TARGET_DIR)/var/www/ipc-webui

	$(INSTALL) -D -m 644 $(@D)/default.config/nginx.conf $(TARGET_DIR)/etc/nginx/
	$(INSTALL) -D -m 755 $(@D)/default.config/php.ini $(TARGET_DIR)/etc/
	$(INSTALL) -D -m 755 $(@D)/default.config/php-fpm.conf $(TARGET_DIR)/etc/
	$(INSTALL) -D -m 755 $(@D)/default.config/scripts/isp_sensor_detect.sh $(TARGET_DIR)/etc/ipc/
	$(INSTALL) -D -m 755 $(@D)/default.config/scripts/startup.sh $(TARGET_DIR)/etc/init.d/S49ipc_webui
	$(INSTALL) -D -m 644 $(@D)/default.config/log_modules $(TARGET_DIR)/etc/ipc/
	cp -af $(@D)/default.config/resolution $(TARGET_DIR)/etc/ipc/

	rsync -avz $(@D)/ $(TARGET_DIR)/var/www/ipc-webui --exclude 'default.config' --exclude '.*'

endef

$(eval $(generic-package))

