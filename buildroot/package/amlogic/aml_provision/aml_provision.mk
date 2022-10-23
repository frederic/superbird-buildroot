#############################################################
#
# AML PROVISION driver
#
#############################################################
AML_PROVISION_SITE = $(TOPDIR)/../vendor/amlogic/provision
AML_PROVISION_SITE_METHOD = local

ifeq ($(BR2_aarch64),y)
	BIN_PATH = bin64
else
	BIN_PATH = bin
endif

define AML_PROVISION_BUILD_CMDS
	@echo "aml provision comiple"
endef

define AML_PROVISION_INSTALL_TARGET_CMDS
	-mkdir -p $(TARGET_DIR)/lib/teetz/
	$(INSTALL) -D -m 0755 $(@D)/ca/$(BIN_PATH)/tee_provision $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D -m 0755 $(@D)/ta/e92a43ab-b4c8-4450-aa12b1516259613b.ta $(TARGET_DIR)/lib/teetz/
endef

$(eval $(generic-package))
