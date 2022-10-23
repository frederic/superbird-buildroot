#############################################################
#
# HDCP driver
#
#############################################################
AML_HDCP_SITE = $(TOPDIR)/../vendor/amlogic/hdcp
AML_HDCP_SITE_METHOD = local

ifeq ($(BR2_aarch64),y)
	BIN_PATH = bin64
else
	BIN_PATH = bin
endif

define AML_HDCP_BUILD_CMDS
	@echo "aml hdcp comiple"
endef

define AML_HDCP_INSTALL_TARGET_CMDS
	-mkdir -p $(TARGET_DIR)/lib/teetz/
	$(INSTALL) -D -m 0755 $(@D)/ca/$(BIN_PATH)/tee_hdcp $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D -m 0755 $(@D)/ta/ff2a4bea-ef6d-11e6-89ccd4ae52a7b3b3.ta $(TARGET_DIR)/lib/teetz/
	install -m 755 $(AML_HDCP_PKGDIR)/S60hdcp $(TARGET_DIR)/etc/init.d/S60hdcp
endef

$(eval $(generic-package))
