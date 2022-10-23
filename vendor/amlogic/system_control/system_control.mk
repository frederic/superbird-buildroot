SYSTEM_CONTROL_VERSION = 0.1
SYSTEM_CONTROL_SITE = $(TOPDIR)/../vendor/amlogic/system_control/src
SYSTEM_CONTROL_SITE_METHOD = local
SYSTEM_CONTROL_HDCPTX22_DIRECTORY = $(SYSTEM_CONTROL_SITE)/firmware

define SYSTEM_CONTROL_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) all
endef

ifeq ($(BR2_PACKAGE_INITSCRIPTS_720P),y)
define SYSTEM_CONTROL_INSTALL_TARGET_CMDS
    mkdir -p $(TARGET_DIR)/etc/firmware
	$(TARGET_CONFIGURE_OPTS) $(MAKE) CC=$(TARGET_CC) -C $(@D) install
	cp -a $(SYSTEM_CONTROL_HDCPTX22_DIRECTORY)/mesondisplay720.cfg   $(TARGET_DIR)/etc/mesondisplay.cfg
endef
else
define SYSTEM_CONTROL_INSTALL_TARGET_CMDS
    mkdir -p $(TARGET_DIR)/etc/firmware
	$(TARGET_CONFIGURE_OPTS) $(MAKE) CC=$(TARGET_CC) -C $(@D) install
	cp -a $(SYSTEM_CONTROL_HDCPTX22_DIRECTORY)/mesondisplay.cfg   $(TARGET_DIR)/etc/
endef
endif

define SYSTEM_CONTROL_INSTALL_INIT_SYSV
	rm -rf $(TARGET_DIR)/etc/init.d/S60systemcontrol
	$(INSTALL) -D -m 755 $(SYSTEM_CONTROL_SITE)/S60systemcontrol \
		$(TARGET_DIR)/etc/init.d/S60systemcontrol
endef

$(eval $(generic-package))
