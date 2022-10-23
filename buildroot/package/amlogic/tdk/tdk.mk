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

TDK_SECUROS_TA_FILE =  e626662e-c0e2-485c-b8c8-09fbce6edf3d \
			e13010e0-2ae1-11e5-896a-0002a5d5c51b \
			5ce0c432-0ab0-40e5-a056-782ca0e6aba2 \
			c3f6e2c0-3548-11e1-b86c-0800200c9a66 \
			cb3e5ba0-adf1-11e0-998b-0002a5d5c51b \
			5b9e0e40-2636-11e1-ad9e-0002a5d5c51b \
			d17f73a0-36ef-11e1-984a-0002a5d5c51b \
			614789f2-39c0-4ebf-b235-92b32ac107ed \
			e6a33ed4-562b-463a-bb7e-ff5e15a493c8 \
			873bcd08-c2c3-11e6-a937-d0bf9c45c61c \
			b689f2a7-8adf-477a-9f99-32e90c0ad0a2 \
			731e279e-aafb-4575-a771-38caa6f0cca6 \
			f157cda0-550c-11e5-a6fa-0002a5d5c51b \
			8aaaf200-2450-11e4-abe2-0002a5d5c51b

define TDK_SECUROS_COPY_ALL_TA
	mkdir $(@D)/target_ta_dir
	cd $(@D)/demos/optee_test/out/ta ; find . -name *.ta | xargs -i $(INSTALL) -m 0644 {} $(@D)/target_ta_dir
	cp $(@D)/demos/hello_world/out/ta/8aaaf200-2450-11e4-abe2-0002a5d5c51b.ta $(@D)/target_ta_dir
endef

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

	for f in $(TDK_SECUROS_TA_FILE); do \
	$(@D)/ta_export/scripts/gen_cert_key.py \
		--root_rsa_key=$(@D)/ta_export/keys/root_rsa_prv_key.pem \
		--ta_rsa_key=$(@D)/ta_export/keys/ta_rsa_pub_key.pem \
		--uuid=$$f \
		--ta_rsa_key_sig=$(@D)/ta_rsa_key.sig \
		--root_aes_key=$(@D)/ta_export/keys/root_aes_key.bin \
		--ta_aes_key=$(@D)/ta_export/keys/ta_aes_key.bin \
		--ta_aes_iv=$(@D)/ta_export/keys/ta_aes_iv.bin \
		--ta_aes_key_iv_enc=$(@D)/ta_aes_key_enc.bin; \
	$(@D)/ta_export/scripts/sign_ta.py \
	    --ta_rsa_key=$(@D)/ta_export/keys/ta_rsa_prv_key.pem \
	    --ta_rsa_key_sig=$(@D)/ta_rsa_key.sig \
	    --ta_aes_key=$(@D)/ta_export/keys/ta_aes_key.bin \
	    --ta_aes_iv=$(@D)/ta_export/keys/ta_aes_iv.bin \
	    --ta_aes_key_iv_enc=$(@D)/ta_aes_key_enc.bin \
	    --in=$(@D)/target_ta_dir/$$f.ta  \
	    --out=$(@D)/target_ta_dir/$$f.ta ; \
		rm -rf $(@D)/ta_aes_key_enc.bin $(@D)/ta_rsa_key.sig ; \
	done
endef

define TDK_TA_TO_TARGET
$(INSTALL) -D -m 0755 $(@D)/target_ta_dir/* $(TARGET_DIR)/lib/teetz
endef



define XTEST
	$(TARGET_CONFIGURE_OPTS) $(MAKE1) ARCH=$(_ARCH) CROSS_COMPILE=$(_CROSS_COMPILE) \
					-C $(@D)/demos/optee_test
endef

ifeq ($(BR2_ARM_KERNEL_32), y)
TARGET_CONFIGURE_OPTS += KERNEL_A32_SUPPORT=true
endif

define TDK_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/linuxdriver ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_KERNEL_CROSS) modules
	$(XTEST)
	$(TARGET_CONFIGURE_OPTS) $(MAKE1) ARCH=$(_ARCH) CROSS_COMPILE=$(_CROSS_COMPILE) -C $(@D)/demos/hello_world
	$(TDK_SECUROS_COPY_ALL_TA)
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
	install -m 755 $(TDK_PKGDIR)/S59optee $(TARGET_DIR)/etc/init.d/S59optee
	$(INSTALL) -D -m 0755 $(@D)/demos/hello_world/out/ca/tee_helloworld $(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/demos/optee_test/out/xtest/tee_xtest $(TARGET_DIR)/usr/bin
	$(TDK_TA_TO_TARGET)
endef

$(eval $(generic-package))
