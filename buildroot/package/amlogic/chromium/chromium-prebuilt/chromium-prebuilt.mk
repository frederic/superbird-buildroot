#############################################################
#
# Chromium-prebuilt
#
#############################################################
ifeq ($(BR2_PACKAGE_CHROMIUM_PREBUILT),y)

CHROMIUM_PREBUILT_VERSION = 69.0.3497.81

CHROMIUM_PREBUILT_DEPENDENCIES = libxkbcommon gconf libexif cups libnss libdrm pciutils pulseaudio krb5 pango libplayer

#prebuilt defines.
CHROMIUM_PREBUILT_SITE = $(TOPDIR)/../vendor/amlogic/chromium
CHROMIUM_PREBUILT_SITE_METHOD = local

ifeq ($(BR2_aarch64),y)
CHROMIUM_PREBUILT_DIRECTORY = $(CHROMIUM_PREBUILT_SITE)/chromium-$(CHROMIUM_PREBUILT_VERSION)/arm64
else
CHROMIUM_PREBUILT_DIRECTORY = $(CHROMIUM_PREBUILT_SITE)/chromium-$(CHROMIUM_PREBUILT_VERSION)/arm
endif

CHROMIUM_INSTALL_DIR=$(TARGET_DIR)/usr/bin/chromium-browser
CHROMIUM_PREBUILT_COMMON_DIRECTORY = $(CHROMIUM_PREBUILT_DIRECTORY)/../common

define CHROMIUM_PREBUILT_INSTALL_TARGET_CMDS
	mkdir -p $(CHROMIUM_INSTALL_DIR)
	cp -a $(CHROMIUM_PREBUILT_DIRECTORY)/chrome            $(CHROMIUM_INSTALL_DIR)
	cp -a $(CHROMIUM_PREBUILT_DIRECTORY)/*.so              $(CHROMIUM_INSTALL_DIR)
	cp -a $(CHROMIUM_PREBUILT_COMMON_DIRECTORY)/*.pak   	      $(CHROMIUM_INSTALL_DIR)
	cp -a $(CHROMIUM_PREBUILT_COMMON_DIRECTORY)/resources         $(CHROMIUM_INSTALL_DIR)
	cp -a $(CHROMIUM_PREBUILT_COMMON_DIRECTORY)/locales           $(CHROMIUM_INSTALL_DIR)
	cp -a $(CHROMIUM_PREBUILT_COMMON_DIRECTORY)/icudtl.dat        $(CHROMIUM_INSTALL_DIR)
endef

ifeq ($(BR2_PACKAGE_LAUNCHER_USE_CHROME), y)
define CHROMIUM_PREBUILT_INSTALL_INIT_SYSV
        rm -rf $(TARGET_DIR)/etc/init.d/S90*
	$(INSTALL) -D -m 755 $(CHROMIUM_PREBUILT_PKGDIR)/../S90chrome \
		$(TARGET_DIR)/etc/init.d/S90chrome
	$(INSTALL) -D -m 755 $(CHROMIUM_PREBUILT_PKGDIR)/../amlogic.html \
		$(TARGET_DIR)/var/www/amlogic.html
endef
endif

$(eval $(generic-package))

endif
