################################################################################
#
# Build the ext2 root filesystem image
#
################################################################################
EXT2_SIZE = $(call qstrip,$(BR2_TARGET_ROOTFS_EXT2_SIZE))
ifeq ($(BR2_TARGET_ROOTFS_EXT2)-$(EXT2_SIZE),y-)
$(error BR2_TARGET_ROOTFS_EXT2_SIZE cannot be empty)
endif

EXT2_MKFS_OPTS = $(call qstrip,$(BR2_TARGET_ROOTFS_EXT2_MKFS_OPTIONS))

# qstrip results in stripping consecutive spaces into a single one. So the
# variable is not qstrip-ed to preserve the integrity of the string value.
EXT2_LABEL := $(subst ",,$(BR2_TARGET_ROOTFS_EXT2_LABEL))
#" Syntax highlighting... :-/ )

EXT2_OPTS = \
	-d $(TARGET_DIR) \
	-r $(BR2_TARGET_ROOTFS_EXT2_REV) \
	-N $(BR2_TARGET_ROOTFS_EXT2_INODES) \
	-m $(BR2_TARGET_ROOTFS_EXT2_RESBLKS) \
	-L "$(EXT2_LABEL)" \
	$(EXT2_MKFS_OPTS)

ROOTFS_EXT2_DEPENDENCIES = host-e2fsprogs host-parted

define ROOTFS_EXT2_CMD
	rm -f $@
	$(HOST_DIR)/sbin/mkfs.ext$(BR2_TARGET_ROOTFS_EXT2_GEN) $(EXT2_OPTS) $@ \
		"$(EXT2_SIZE)" \
	|| { ret=$$?; \
	     echo "*** Maybe you need to increase the filesystem size (BR2_TARGET_ROOTFS_EXT2_SIZE)" 1>&2; \
	     exit $$ret; \
	}
endef

ifneq ($(BR2_TARGET_ROOTFS_EXT2_GEN),2)
define ROOTFS_EXT2_SYMLINK
	ln -sf rootfs.ext2$(ROOTFS_EXT2_COMPRESS_EXT) $(BINARIES_DIR)/rootfs.ext$(BR2_TARGET_ROOTFS_EXT2_GEN)$(ROOTFS_EXT2_COMPRESS_EXT)
endef
ROOTFS_EXT2_POST_GEN_HOOKS += ROOTFS_EXT2_SYMLINK
endif

ifeq ($(BR2_PACKAGE_AML_VENDOR_PARTITION),y)
define VENDOR_EXT2_CREATE
	rm -f $(BINARIES_DIR)/vendor.ext2
	$(HOST_DIR)/sbin/mkfs.ext$(BR2_TARGET_ROOTFS_EXT2_GEN) -d $(BR2_PACKAGE_AML_VENDOR_PARTITION_PATH) $(EXT2_MKFS_OPTS) $(BINARIES_DIR)/vendor.ext2 \
		"$(BR2_PACKAGE_AML_VENDOR_PARTITION_SIZE)"
	 $(HOST_DIR)/bin/img2simg $(BINARIES_DIR)/vendor.ext2 $(BINARIES_DIR)/vendor.ext2.img2simg
endef
ROOTFS_EXT2_POST_GEN_HOOKS += VENDOR_EXT2_CREATE
endif

DEVICE_DIR := $(patsubst "%",%,$(BR2_ROOTFS_OVERLAY))
UPGRADE_DIR := $(patsubst "%",%,$(BR2_ROOTFS_UPGRADE_DIR))
UPGRADE_DIR_OVERLAY := $(patsubst "%",%,$(BR2_ROOTFS_UPGRADE_DIR_OVERLAY))
ifeq ($(BR2_TARGET_USBTOOL_AMLOGIC),y)
ifeq ($(filter y,$(BR2_TARGET_UBOOT_AMLOGIC_2015) $(BR2_TARGET_UBOOT_AMLOGIC_REPO)),y)
ifneq ($(UPGRADE_DIR_OVERLAY),)
define ROOTFS-USB-IMAGE-PACK
	cp -rf $(UPGRADE_DIR)/* $(BINARIES_DIR)
	cp -rf $(UPGRADE_DIR_OVERLAY)/* $(BINARIES_DIR)
	BINARIES_DIR=$(BINARIES_DIR) \
	TOOL_DIR=$(HOST_DIR)/bin \
	$(HOST_DIR)/bin/aml_upgrade_pkg_gen.sh \
	$(BR2_TARGET_UBOOT_PLATFORM) $(BR2_TARGET_UBOOT_ENCRYPTION) $(BR2_PACKAGE_SWUPDATE_AB_SUPPORT)
endef
else
define ROOTFS-USB-IMAGE-PACK
	cp -rf $(UPGRADE_DIR)/* $(BINARIES_DIR)
	BINARIES_DIR=$(BINARIES_DIR) \
	TOOL_DIR=$(HOST_DIR)/bin \
	$(HOST_DIR)/bin/aml_upgrade_pkg_gen.sh \
	$(BR2_TARGET_UBOOT_PLATFORM) $(BR2_TARGET_UBOOT_ENCRYPTION) $(BR2_PACKAGE_SWUPDATE_AB_SUPPORT)
endef
endif

else #BR2_TARGET_UBOOT_AMLOGIC_2015
define ROOTFS-USB-IMAGE-PACK
	cp -rf $(UPGRADE_DIR)/* $(BINARIES_DIR)
	$(HOST_DIR)/bin/aml_image_v2_packer_new -r $(BINARIES_DIR)/aml_upgrade_package.conf $(BINARIES_DIR)/ $(BINARIES_DIR)/aml_upgrade_package.img
endef
endif #BR2_TARGET_UBOOT_AMLOGIC_2015
ROOTFS_EXT2_POST_GEN_HOOKS += ROOTFS-USB-IMAGE-PACK
endif #BR2_TARGET_USBTOOL_AMLOGIC

RECOVERY_OTA_DIR := $(patsubst "%",%,$(BR2_RECOVERY_OTA_DIR))
ifneq ($(RECOVERY_OTA_DIR),)
ifeq ($(BR2_TARGET_UBOOT_ENCRYPTION),y)
	RECOVERY_ENC_FLAG="-enc"
endif
define ROOTFS-OTA-SWU-PACK-EXT4FS
	$(INSTALL) -m 0755 $(RECOVERY_OTA_DIR)/../swu/* $(BINARIES_DIR)
	$(INSTALL) -m 0644 $(RECOVERY_OTA_DIR)/sw-description-emmc$(RECOVERY_ENC_FLAG) $(BINARIES_DIR)/sw-description
	$(INSTALL) -m 0644 $(RECOVERY_OTA_DIR)/sw-description-emmc-increment$(RECOVERY_ENC_FLAG) $(BINARIES_DIR)/sw-description-increment
	$(INSTALL) -m 0644 $(RECOVERY_OTA_DIR)/ota-package-filelist-emmc$(RECOVERY_ENC_FLAG) $(BINARIES_DIR)/ota-package-filelist
	$(BINARIES_DIR)/ota_package_create.sh
endef
ROOTFS_EXT2_POST_GEN_HOOKS += ROOTFS-OTA-SWU-PACK-EXT4FS
endif

ifeq ($(BR2_TARGET_UBOOT_AMLOGIC_2015),y)
SD_BOOT = $(BINARIES_DIR)/u-boot.bin.sd.bin
else
SD_BOOT = $(BINARIES_DIR)/u-boot.bin
endif
define MBR-IMAGE
	@$(call MESSAGE,"Creating mbr image")
	filesz=`stat -c '%s' $(BINARIES_DIR)/rootfs.ext2`; \
	dd if=/dev/zero of=$(BINARIES_DIR)/disk.img bs=512 count=$$(($$filesz/512+2048)); \
	parted $(BINARIES_DIR)/disk.img mklabel msdos; \
	parted $(BINARIES_DIR)/disk.img mkpart primary ext2 2048s 100%; \
	dd if=$(BINARIES_DIR)/rootfs.ext2 of=$(BINARIES_DIR)/disk.img bs=512 count=$$(($$filesz/512)) seek=2048;
endef

define ODROID-UBOOT
	dd if=$(BINARIES_DIR)/bl1.bin.hardkernel of=$(BINARIES_DIR)/disk.img bs=1 count=442 conv=notrunc; \
	dd if=$(BINARIES_DIR)/bl1.bin.hardkernel of=$(BINARIES_DIR)/disk.img bs=512 skip=1 seek=1 conv=notrunc; \
	dd if=$(BINARIES_DIR)/u-boot.bin of=$(BINARIES_DIR)/disk.img bs=512 seek=64 conv=notrunc;
endef

define AMLOGIC-UBOOT
	dd if=$(SD_BOOT) of=$(BINARIES_DIR)/disk.img bs=1 count=442 conv=notrunc; \
	dd if=$(SD_BOOT) of=$(BINARIES_DIR)/disk.img bs=512 skip=1 seek=1 conv=notrunc;
endef

ifeq ($(BR2_TARGET_MBR_IMAGE),y)
ROOTFS_EXT2_POST_GEN_HOOKS += MBR-IMAGE
ifeq ($(BR2_TARGET_UBOOT_ODROID),y)
ROOTFS_EXT2_POST_GEN_HOOKS += ODROID-UBOOT
else
ROOTFS_EXT2_POST_GEN_HOOKS += AMLOGIC-UBOOT
endif
endif

$(eval $(rootfs))
