################################################################################
#
# Embed the ubifs image into an ubi image
#
################################################################################

UBI_UBINIZE_OPTS := -m $(BR2_TARGET_ROOTFS_UBIFS_MINIOSIZE)
UBI_UBINIZE_OPTS += -p $(BR2_TARGET_ROOTFS_UBI_PEBSIZE)
ifneq ($(BR2_TARGET_ROOTFS_UBI_SUBSIZE),0)
UBI_UBINIZE_OPTS += -s $(BR2_TARGET_ROOTFS_UBI_SUBSIZE)
endif

UBI_UBINIZE_OPTS += $(call qstrip,$(BR2_TARGET_ROOTFS_UBI_OPTS))

ROOTFS_UBI_DEPENDENCIES = rootfs-ubifs

ifeq ($(BR2_TARGET_ROOTFS_UBI_USE_CUSTOM_CONFIG),y)
UBI_UBINIZE_CONFIG_FILE_PATH = $(call qstrip,$(BR2_TARGET_ROOTFS_UBI_CUSTOM_CONFIG_FILE))
else
UBI_UBINIZE_CONFIG_FILE_PATH = fs/ubi/ubinize.cfg
endif

# don't use sed -i as it misbehaves on systems with SELinux enabled when this is
# executed through fakeroot (see #9386)
define ROOTFS_UBI_CMD
	sed 's;BR2_ROOTFS_UBIFS_PATH;$@fs;' \
		$(UBI_UBINIZE_CONFIG_FILE_PATH) > $(BUILD_DIR)/ubinize.cfg
	$(HOST_DIR)/sbin/ubinize -o $@ $(UBI_UBINIZE_OPTS) $(BUILD_DIR)/ubinize.cfg
	rm $(BUILD_DIR)/ubinize.cfg
endef


DEVICE_DIR := $(patsubst "%",%,$(BR2_ROOTFS_OVERLAY))
UPGRADE_DIR := $(patsubst "%",%,$(BR2_ROOTFS_UPGRADE_DIR))
UPGRADE_DIR_OVERLAY := $(patsubst "%",%,$(BR2_ROOTFS_UPGRADE_DIR_OVERLAY))
ifeq ($(BR2_TARGET_USBTOOL_UBI_AMLOGIC),y)
ifneq ($(UPGRADE_DIR_OVERLAY),)
define ROOTFS-USB-IMAGE-PACK-UBI
	cp -rf $(UPGRADE_DIR)/* $(BINARIES_DIR)
	cp -rf $(UPGRADE_DIR_OVERLAY)/* $(BINARIES_DIR)
	BINARIES_DIR=$(BINARIES_DIR) \
	TOOL_DIR=$(HOST_DIR)/usr/bin \
	$(HOST_DIR)/usr/bin/aml_upgrade_pkg_gen.sh \
	$(BR2_TARGET_UBOOT_PLATFORM) $(BR2_TARGET_UBOOT_ENCRYPTION) $(BR2_PACKAGE_SWUPDATE_AB_SUPPORT)
endef
else
define ROOTFS-USB-IMAGE-PACK-UBI
	cp -rf $(UPGRADE_DIR)/* $(BINARIES_DIR)
	BINARIES_DIR=$(BINARIES_DIR) \
	TOOL_DIR=$(HOST_DIR)/usr/bin \
	$(HOST_DIR)/usr/bin/aml_upgrade_pkg_gen.sh \
	$(BR2_TARGET_UBOOT_PLATFORM) $(BR2_TARGET_UBOOT_ENCRYPTION) $(BR2_PACKAGE_SWUPDATE_AB_SUPPORT)
endef
endif
ROOTFS_UBI_POST_GEN_HOOKS += ROOTFS-USB-IMAGE-PACK-UBI
endif #BR2_TARGET_USBTOOL_UBI_AMLOGIC

RECOVERY_OTA_DIR := $(patsubst "%",%,$(BR2_RECOVERY_OTA_DIR))
ifneq ($(RECOVERY_OTA_DIR),)
ifeq ($(BR2_TARGET_UBOOT_ENCRYPTION),y)
	RECOVERY_ENC_FLAG="-enc"
endif
ifeq ($(BR2_PACKAGE_SWUPDATE_AB_SUPPORT),"absystem")
define ROOTFS-OTA-SWU-PACK-UBI
	$(INSTALL) -m 0755 $(RECOVERY_OTA_DIR)/../swu/* $(BINARIES_DIR)/
	$(INSTALL) -m 0644 $(RECOVERY_OTA_DIR)/sw-description-nand-ab $(BINARIES_DIR)/sw-description
	$(INSTALL) -m 0644 $(RECOVERY_OTA_DIR)/ota-package-filelist-nand$(RECOVERY_ENC_FLAG) $(BINARIES_DIR)/ota-package-filelist
	$(BINARIES_DIR)/ota_package_create.sh
endef
else
define ROOTFS-OTA-SWU-PACK-UBI
	$(INSTALL) -m 0755 $(RECOVERY_OTA_DIR)/../swu/* $(BINARIES_DIR)/
	$(INSTALL) -m 0644 $(RECOVERY_OTA_DIR)/sw-description-nand$(RECOVERY_ENC_FLAG) $(BINARIES_DIR)/sw-description
	$(INSTALL) -m 0644 $(RECOVERY_OTA_DIR)/sw-description-nand-increment$(RECOVERY_ENC_FLAG) $(BINARIES_DIR)/sw-description-increment
	$(INSTALL) -m 0644 $(RECOVERY_OTA_DIR)/ota-package-filelist-nand$(RECOVERY_ENC_FLAG) $(BINARIES_DIR)/ota-package-filelist
	$(BINARIES_DIR)/ota_package_create.sh
endef
endif
ROOTFS_UBI_POST_GEN_HOOKS += ROOTFS-OTA-SWU-PACK-UBI
endif

ifeq ($(BR2_TARGET_UBOOT_AMLOGIC_2015),y)
SD_BOOT = $(BINARIES_DIR)/u-boot.bin.sd.bin
else
SD_BOOT = $(BINARIES_DIR)/u-boot.bin
endif
$(eval $(rootfs))
