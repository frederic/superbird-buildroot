################################################################################
#
# web_ui_wifi
#
################################################################################

WEB_UI_WIFI_VERSION = 20170915
WEB_UI_WIFI_SITE_METHOD = local
WEB_UI_WIFI_SITE = $(WEB_UI_WIFI_PKGDIR)/src

ifeq ($(BR2_aarch64),y)
	MAIN_CGI_BIN = main64.cgi
else
	MAIN_CGI_BIN = main32.cgi
endif

define WEB_UI_WIFI_INSTALL_TARGET_CMDS
	mkdir -p ${TARGET_DIR}/var/www/cgi-bin
	cp $(@D)/Html/* ${TARGET_DIR}/var/www/
	cp -rf $(@D)/css ${TARGET_DIR}/var/www/
	cp -rf $(@D)/fonts ${TARGET_DIR}/var/www/
	cp -rf $(@D)/images ${TARGET_DIR}/var/www/
	cp -rf $(@D)/js ${TARGET_DIR}/var/www/
	cp $(@D)/cgi-bin/${MAIN_CGI_BIN} ${TARGET_DIR}/var/www/cgi-bin/main.cgi
	cp $(@D)/cgi-bin/soundbar.cgi ${TARGET_DIR}/var/www/cgi-bin/
	cp -rf $(@D)/cgi-bin/scripts/* ${TARGET_DIR}/var/www/cgi-bin/
	if [ "$(BR2_PACKAGE_AUDIOSERVICE)" == "y" ] ; then						\
		echo " do nothing";																					\
	else																													\
		echo "delete linkage of soundbar on main page";							\
		sed -i '/soundbar/d' ${TARGET_DIR}/var/www/index.html;			\
	fi
endef

$(eval $(generic-package))
