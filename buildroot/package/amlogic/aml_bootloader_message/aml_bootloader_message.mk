#############################################################
#
# aml bootloader message
#
#############################################################
AML_BOOTLOADER_MESSAGE_VERSION = 0.1
AML_BOOTLOADER_MESSAGE_SITE = $(TOPDIR)/../vendor/amlogic/aml_commonlib/bootloader_message
AML_BOOTLOADER_MESSAGE_SITE_METHOD = local
AML_BOOTLOADER_MESSAGE_DEPENDENCIES = zlib

define AML_BOOTLOADER_MESSAGE_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) CFLAGS=$(FLAGS) -C $(@D) all
endef

define AML_BOOTLOADER_MESSAGE_INSTALL_TARGET_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) CC=$(TARGET_CC) -C $(@D) install
endef

$(eval $(generic-package))
