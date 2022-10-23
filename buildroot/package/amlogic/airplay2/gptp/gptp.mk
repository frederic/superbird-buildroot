#############################################################
#
# gptp
#
#############################################################

GPTP_VERSION = ArtAndLogic-aPTP-changes
GPTP_SITE_METHOD = git
GPTP_SITE = https://github.com/rroussel/OpenAvnu.git

define GPTP_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) ARCH=RPI CC=$(TARGET_CC) CXX=$(TARGET_CXX) -C $(@D)/daemons/gptp/linux/build/
endef

define GPTP_INSTALL_TARGET_CMDS
	$(INSTALL) -m 755 -D $(@D)/daemons/gptp/linux/build/obj/daemon_cl $(TARGET_DIR)/usr/bin/
	mkdir -p $(TARGET_DIR)/etc/airplay/
	$(INSTALL) -m 644 -D $(@D)/daemons/gptp/gptp_cfg.ini $(TARGET_DIR)/etc/airplay/
endef

$(eval $(generic-package))


