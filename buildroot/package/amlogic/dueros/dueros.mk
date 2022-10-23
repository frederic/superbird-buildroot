################################################################################
#
# duerOS
#
################################################################################

DUEROS_VERSION = 20180808
DUEROS_SITE_METHOD = local
DUEROS_SITE = $(DUEROS_PKGDIR)/src


define DUEROS_INSTALL_TARGET_CMDS
	mkdir -p ${TARGET_DIR}/usr/bin
	mkdir -p ${TARGET_DIR}/DuerOS_pre
	mkdir -p ${TARGET_DIR}/etc/init.d

	$(INSTALL) -D -m 755 $(@D)/connect_ap.sh $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D -m 644 $(@D)/asound.conf $(TARGET_DIR)/DuerOS_pre/
	$(INSTALL) -D -m 755 $(@D)/bluez_tool_dueros.sh $(TARGET_DIR)/DuerOS_pre/
	$(INSTALL) -D -m 755 $(@D)/S42wifi-dueros $(TARGET_DIR)/DuerOS_pre/
	$(INSTALL) -D -m 755 $(@D)/S44bluetooth-dueros $(TARGET_DIR)/DuerOS_pre/
	$(INSTALL) -D -m 644 $(@D)/wpa_supplicant.conf $(TARGET_DIR)/DuerOS_pre/
	$(INSTALL) -D -m 755 $(@D)/adckey_function.sh $(TARGET_DIR)/DuerOS_pre/
	$(INSTALL) -D -m 755 $(@D)/S04duerOS $(TARGET_DIR)/etc/init.d/
endef

$(eval $(generic-package))
