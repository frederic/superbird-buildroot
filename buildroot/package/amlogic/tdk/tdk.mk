#############################################################
#
# TDK driver
#
#############################################################
TDK_VERSION = $(call qstrip,$(BR2_PACKAGE_TDK_GIT_VERSION))
ifneq ($(BR2_PACKAGE_TDK_GIT_REPO_URL),"")
TDK_SITE = $(call qstrip,$(BR2_PACKAGE_TDK_GIT_REPO_URL))
TDK_SITE_METHOD = git
else
TDK_SITE = $(TOPDIR)/../vendor/amlogic/tdk
TDK_SITE_METHOD = local
endif
TDK_INSTALL_STAGING = YES
TDK_DEPENDENCIES = linux host-python-pycrypto

ifeq ($(BR2_aarch64), y)
_ARCH = arm64
_CROSS_COMPILE = aarch64-linux-gnu-
else
_ARCH = arm
_CROSS_COMPILE = arm-linux-gnueabihf-
endif

SECUROS_IMAGE_DIR = "gx"

ifeq ($(BR2_TARGET_UBOOT_PLATFORM), "axg")
SECUROS_IMAGE_DIR = "axg"
else ifeq ($(BR2_TARGET_UBOOT_PLATFORM), "txlx")
SECUROS_IMAGE_DIR = "txlx"
else ifeq ($(BR2_TARGET_UBOOT_PLATFORM), "g12a")
SECUROS_IMAGE_DIR = "g12a"
else ifeq ($(BR2_TARGET_UBOOT_PLATFORM), "g12b")
SECUROS_IMAGE_DIR = "g12a"
else ifeq ($(BR2_TARGET_UBOOT_PLATFORM), "sm1")
SECUROS_IMAGE_DIR = "g12a"
else ifeq ($(BR2_TARGET_UBOOT_PLATFORM), "tl1")
SECUROS_IMAGE_DIR = "tl1"
else ifeq ($(BR2_TARGET_UBOOT_PLATFORM), "tm2")
SECUROS_IMAGE_DIR = "tm2"
else ifeq ($(BR2_TARGET_UBOOT_PLATFORM), "a1")
SECUROS_IMAGE_DIR = "a1"
else ifeq ($(BR2_TARGET_UBOOT_PLATFORM), "c1")
SECUROS_IMAGE_DIR = "c1"
else
SECUROS_IMAGE_DIR = "gx"
endif

define TDK_SECUROS_SIGN
    # pack root rsa into  bl32.img
    $(@D)/ta_export/scripts/pack_kpub.py \
		--rsk=$(@D)/ta_export/keys/root_rsa_pub_key.pem \
		--rek=$(@D)/ta_export/keys/root_aes_key.bin \
		--in=$(@D)/secureos/$(call qstrip,$(SECUROS_IMAGE_DIR))/bl32.img \
		--out=$(@D)/secureos/$(call qstrip,$(SECUROS_IMAGE_DIR))/bl32.img

endef

ifeq ($(BR2_ARM_KERNEL_32), y)
TARGET_CONFIGURE_OPTS += KERNEL_A32_SUPPORT=true
endif

define TDK_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/linuxdriver ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_KERNEL_CROSS) modules
	$(TDK_SECUROS_SIGN)
endef

ifeq ($(BR2_aarch64), y)
define TDK_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0644 $(@D)/ca_export_arm64/include/*.h $(STAGING_DIR)/usr/include
	$(INSTALL) -D -m 0644 $(@D)/ca_export_arm64/lib/* $(STAGING_DIR)/usr/lib/
	cp -rf $(@D)/ta_export $(STAGING_DIR)/usr/share/
endef
define TDK_INSTALL_LIBS
	$(INSTALL) -D -m 0644 $(@D)/ca_export_arm64/lib/*.so $(TARGET_DIR)/usr/lib/
	$(INSTALL) -D -m 0644 $(@D)/ca_export_arm64/lib/*.so.* $(TARGET_DIR)/usr/lib/
	$(INSTALL) -D -m 0755 $(@D)/ca_export_arm64/bin/tee-supplicant $(TARGET_DIR)/usr/bin/
endef
else
define TDK_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0644 $(@D)/ca_export_arm/include/*.h $(STAGING_DIR)/usr/include
	$(INSTALL) -D -m 0644 $(@D)/ca_export_arm/lib/* $(STAGING_DIR)/usr/lib/
	cp -rf $(@D)/ta_export $(STAGING_DIR)/usr/share/
endef
define TDK_INSTALL_LIBS
	$(INSTALL) -D -m 0644 $(@D)/ca_export_arm/lib/*.so $(TARGET_DIR)/usr/lib/
	$(INSTALL) -D -m 0644 $(@D)/ca_export_arm/lib/*.so.* $(TARGET_DIR)/usr/lib/
	$(INSTALL) -D -m 0755 $(@D)/ca_export_arm/bin/tee-supplicant $(TARGET_DIR)/usr/bin/
endef
endif
define TDK_INSTALL_TARGET_CMDS
	$(TDK_INSTALL_LIBS)
	cd $(@D); find . -name *_32 | xargs -i $(INSTALL) -m 0755 {} $(TARGET_DIR)/usr/bin
	cd $(@D); find . -name *_64 | xargs -i $(INSTALL) -m 0755 {} $(TARGET_DIR)/usr/bin

	mkdir -p $(TARGET_DIR)/lib/teetz/
	cd $(@D); find . -name *.ta | xargs -i $(INSTALL) -m 0644 {} $(TARGET_DIR)/lib/teetz

	mkdir -p $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/kernel/drivers/trustzone/
	$(INSTALL) -D -m 0644 $(@D)/linuxdriver/optee/optee_armtz.ko $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/kernel/drivers/trustzone/
	$(INSTALL) -D -m 0644 $(@D)/linuxdriver/optee.ko $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/kernel/drivers/trustzone/
	echo kernel/drivers/trustzone/optee_armtz.ko: kernel/drivers/trustzone/optee.ko >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
	echo kernel/drivers/trustzone/optee.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
	echo "#OPTEE" >> $(TARGET_DIR)/etc/modules
	echo optee >> $(TARGET_DIR)/etc/modules
	echo optee_armtz >> $(TARGET_DIR)/etc/modules
	install -m 755 $(TDK_PKGDIR)/S49optee $(TARGET_DIR)/etc/init.d/S49optee
	$(TDK_TA_TO_TARGET)
endef

$(eval $(generic-package))
