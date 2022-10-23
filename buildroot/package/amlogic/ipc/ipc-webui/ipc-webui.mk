#############################################################
#
# IPC Webui
#
#############################################################

IPC_WEBUI_VERSION = 1.0
IPC_WEBUI_SITE = $(TOPDIR)/../vendor/amlogic/ipc/ipc_webui
IPC_WEBUI_SITE_METHOD = local
IPC_WEBUI_DEPENDENCIES = nginx php host-gettext

define IPC_WEBUI_BUILD_CMDS
	$(HOST_DIR)/bin/msgfmt $(@D)/locale/zh_CN/LC_MESSAGES/language.po -o $(@D)/locale/zh_CN/LC_MESSAGES/language.mo
endef

define IPC_WEBUI_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/var/www/ipc-webui/locale/zh_CN/LC_MESSAGES

	$(INSTALL) -D -m 644 $(IPC_WEBUI_SITE)/default.config/nginx.conf $(TARGET_DIR)/etc/nginx/nginx.conf
	$(INSTALL) -D -m 755 $(IPC_WEBUI_SITE)/default.config/php.ini $(TARGET_DIR)/etc/php.ini
	$(INSTALL) -D -m 755 $(IPC_WEBUI_PKGDIR)/S49ipc_webui $(TARGET_DIR)/etc/init.d
	$(INSTALL) -D -m 644 $(@D)/locale/zh_CN/LC_MESSAGES/language.mo $(TARGET_DIR)/var/www/ipc-webui/locale/zh_CN/LC_MESSAGES/language.mo

	rsync -avz $(IPC_WEBUI_SITE)/ $(TARGET_DIR)/var/www/ipc-webui --exclude 'default.config' --exclude 'locale' --exclude '.git'

endef

$(eval $(generic-package))

