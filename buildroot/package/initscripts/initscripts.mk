################################################################################
#
# initscripts
#
################################################################################

ifeq ($(BR2_PACKAGE_INITSCRIPTS_720P),y)
define INITSCRIPTS_INSTALL_TARGET_CMDS
        mkdir -p  $(TARGET_DIR)/etc/init.d
        $(INSTALL) -D -m 0755 package/initscripts/init.d/* $(TARGET_DIR)/etc/init.d/
        $(INSTALL) -D -m 0755 package/initscripts/etc/directfbrc720 $(TARGET_DIR)/etc/directfbrc
endef
else
define INITSCRIPTS_INSTALL_TARGET_CMDS
	mkdir -p  $(TARGET_DIR)/etc/init.d
	$(INSTALL) -D -m 0755 package/initscripts/init.d/* $(TARGET_DIR)/etc/init.d/
	$(INSTALL) -D -m 0755 package/initscripts/etc/directfbrc $(TARGET_DIR)/etc/
endef
endif

$(eval $(generic-package))
