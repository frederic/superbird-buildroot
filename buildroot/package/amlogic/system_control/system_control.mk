SYSTEM_CONTROL_VERSION = 0.1
SYSTEM_CONTROL_SITE = $(TOPDIR)/../vendor/amlogic/system_control/src
SYSTEM_CONTROL_SITE_METHOD = local
SYSTEM_CONTROL_HDCPTX22_DIRECTORY = $(SYSTEM_CONTROL_SITE)/firmware

SYSTEM_CONTROL_DEPENDENCIES += libzlib
#SYSTEM_CONTROL_DEPENDENCIES += aml_ubootenv

ifeq ($(BR2_PACKAGE_MESON_MALI_WAYLAND_DRM_EGL), y)
SYSTEM_CONTROL_CONF_OPTS += -DAML_OSD_USE_DRM=ON
endif

ifeq ($(BR2_PACKAGE_INITSCRIPTS_720P),y)
define SYSTEM_CONTROL_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/systemcontrol $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D -m 755 $(@D)/display_util $(TARGET_DIR)/usr/bin/
	mkdir -p $(TARGET_DIR)/etc/firmware
	cp -a $(SYSTEM_CONTROL_HDCPTX22_DIRECTORY)/mesondisplay720.cfg   $(TARGET_DIR)/etc/mesondisplay.cfg
endef
else
define SYSTEM_CONTROL_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/systemcontrol $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D -m 755 $(@D)/display_util $(TARGET_DIR)/usr/bin/
	mkdir -p $(TARGET_DIR)/etc/firmware
	cp -a $(SYSTEM_CONTROL_HDCPTX22_DIRECTORY)/mesondisplay.cfg   $(TARGET_DIR)/etc/
endef
endif

define SYSTEM_CONTROL_INSTALL_INIT_SYSV
	rm -rf $(TARGET_DIR)/etc/init.d/S60systemcontrol
	$(INSTALL) -D -m 755 $(SYSTEM_CONTROL_SITE)/S60systemcontrol \
		$(TARGET_DIR)/etc/init.d/S60systemcontrol
endef

$(eval $(cmake-package))
