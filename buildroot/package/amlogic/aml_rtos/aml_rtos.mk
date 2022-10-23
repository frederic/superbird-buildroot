BR2_PACKAGE_AML_RTOS_LOCAL_PATH:= $(call qstrip,$(BR2_PACKAGE_AML_RTOS_LOCAL_PATH))
ifneq ($(BR2_PACKAGE_AML_RTOS_LOCAL_PATH),)

AML_RTOS_VERSION = 1.0.0
AML_RTOS_SITE := $(call qstrip,$(BR2_PACKAGE_AML_RTOS_LOCAL_PATH))
AML_RTOS_SITE_METHOD = local
AML_RTOS_SOC_NAME = $(strip $(BR2_PACKAGE_AML_SOC_FAMILY_NAME))
AML_RTOS_PREBUILT = rtos-prebuilt-$(AML_RTOS_SOC_NAME)
AML_RTOS_DEPENDENCIES += aml_dsp_util

define AML_RTOS_BUILD_CMDS
	mkdir -p $(BINARIES_DIR)
	if [ -n "$(BR2_PACKAGE_AML_RTOS_ARM_BUILD_OPTION)" ]; then \
		pushd $(@D);  \
			set -e; ./scripts/amlogic/mk.sh $(BR2_PACKAGE_AML_RTOS_ARM_BUILD_OPTION); \
			$(INSTALL) -D -m 644 ./out_armv8/rtos-uImage $(BINARIES_DIR)/;\
		popd; \
	fi
	if [ -n "$(BR2_PACKAGE_AML_RTOS_DSPA_BUILD_OPTION)" ]; then \
		pushd $(@D);  \
			set -e; ./scripts/amlogic/mk.sh $(BR2_PACKAGE_AML_RTOS_DSPA_BUILD_OPTION); \
			test -f ./demos/amlogic/xcc/xtensa_hifi4/dspboot.bin &&  $(INSTALL) -D -m 644 ./demos/amlogic/xcc/xtensa_hifi4/dspboot.bin $(BINARIES_DIR)/dspbootA.bin || true;\
			test -f ./demos/amlogic/xcc/xtensa_hifi4/dspboot_sram.bin &&  $(INSTALL) -D -m 644 ./demos/amlogic/xcc/xtensa_hifi4/dspboot_sram.bin $(BINARIES_DIR)/dspbootA_sram.bin || true;\
			test -f ./demos/amlogic/xcc/xtensa_hifi4/dspboot_sdram.bin &&  $(INSTALL) -D -m 644 ./demos/amlogic/xcc/xtensa_hifi4/dspboot_sdram.bin $(BINARIES_DIR)/dspbootA_sdram.bin || true;\
		popd; \
	fi
	if [ -n "$(BR2_PACKAGE_AML_RTOS_DSPB_BUILD_OPTION)" ]; then \
		pushd $(@D);  \
			set -e; ./scripts/amlogic/mk.sh $(BR2_PACKAGE_AML_RTOS_DSPB_BUILD_OPTION); \
			test -f ./demos/amlogic/xcc/xtensa_hifi4/dspboot.bin &&  $(INSTALL) -D -m 644 ./demos/amlogic/xcc/xtensa_hifi4/dspboot.bin $(BINARIES_DIR)/dspbootB.bin || true;\
			test -f ./demos/amlogic/xcc/xtensa_hifi4/dspboot_sram.bin &&  $(INSTALL) -D -m 644 ./demos/amlogic/xcc/xtensa_hifi4/dspboot_sram.bin $(BINARIES_DIR)/dspbootB_sram.bin || true;\
			test -f ./demos/amlogic/xcc/xtensa_hifi4/dspboot_sdram.bin &&  $(INSTALL) -D -m 644 ./demos/amlogic/xcc/xtensa_hifi4/dspboot_sdram.bin $(BINARIES_DIR)/dspbootB_sdram.bin || true;\
		popd; \
	fi
endef

define AML_RTOS_INSTALL_TARGET_CMDS
	if [ -n "$(BR2_PACKAGE_AML_RTOS_DSPA_INSTALL)" ]; then \
		mkdir -p $(TARGET_DIR)/lib/firmware/; \
		$(INSTALL) -D -m 644 $(BINARIES_DIR)/dspbootA*.bin $(TARGET_DIR)/lib/firmware/;\
		if [ -n "$(BR2_PACKAGE_AML_RTOS_DSPA_AUTOLOAD)" ]; then \
			$(INSTALL) -D -m 755 \
			$(AML_RTOS_PKGDIR)/S71_load_dspa \
			$(TARGET_DIR)/etc/init.d/;\
		fi \
	fi
	if [ -n "$(BR2_PACKAGE_AML_RTOS_DSPB_INSTALL)" ]; then \
		mkdir -p $(TARGET_DIR)/lib/firmware/; \
		$(INSTALL) -D -m 644 $(BINARIES_DIR)/dspbootB*.bin $(TARGET_DIR)/lib/firmware/;\
		if [ -n "$(BR2_PACKAGE_AML_RTOS_DSPB_AUTOLOAD)" ]; then \
			$(INSTALL) -D -m 755 \
			$(AML_RTOS_PKGDIR)/S71_load_dspb \
			$(TARGET_DIR)/etc/init.d/;\
		fi \
	fi
	#Package RTOS build result
	pushd $(BINARIES_DIR); \
		mkdir -p $(AML_RTOS_PREBUILT); \
		test -f rtos-uImage && cp -fv rtos-uImage $(AML_RTOS_PREBUILT); \
		cp -f dspboot*.bin $(AML_RTOS_PREBUILT)/ || true; \
		tar -zcf $(AML_RTOS_PREBUILT).tgz -C $(AML_RTOS_PREBUILT) ./; \
	popd
endef

endif

BR2_PACKAGE_AML_RTOS_PREBUILT_PATH:= $(call qstrip,$(BR2_PACKAGE_AML_RTOS_PREBUILT_PATH))
ifneq ($(BR2_PACKAGE_AML_RTOS_PREBUILT_PATH),)

AML_RTOS_VERSION = 1.0.0
AML_RTOS_SITE := $(call qstrip,$(BR2_PACKAGE_AML_RTOS_PREBUILT_PATH))
AML_RTOS_SITE_METHOD = local
AML_RTOS_DEPENDENCIES += aml_dsp_util

define AML_RTOS_INSTALL_TARGET_CMDS
	mkdir -p $(BINARIES_DIR)
	if [ -f $(@D)/rtos-uImage ]; then \
		$(INSTALL) -D -m 644 $(@D)/rtos-uImage $(BINARIES_DIR)/; \
	fi
	if [ -f $(@D)/dspbootA.bin ]; then \
		$(INSTALL) -D -m 644 $(@D)/dspbootA.bin $(BINARIES_DIR)/; \
	fi
	if [ -f $(@D)/dspbootB.bin ]; then \
		$(INSTALL) -D -m 644 $(@D)/dspbootB.bin $(BINARIES_DIR)/; \
	fi
	if [ -n "$(BR2_PACKAGE_AML_RTOS_DSPA_INSTALL)" ]; then \
		mkdir -p $(TARGET_DIR)/lib/firmware/; \
		$(INSTALL) -D -m 644 $(@D)/dspbootA.bin $(TARGET_DIR)/lib/firmware/;\
		if [ -n "$(BR2_PACKAGE_AML_RTOS_DSPA_AUTOLOAD)" ]; then \
			$(INSTALL) -D -m 755 \
			$(AML_RTOS_PKGDIR)/S71_load_dspa \
			$(TARGET_DIR)/etc/init.d/;\
		fi \
	fi
	if [ -n "$(BR2_PACKAGE_AML_RTOS_DSPB_INSTALL)" ]; then \
		mkdir -p $(TARGET_DIR)/lib/firmware/; \
		$(INSTALL) -D -m 644 $(@D)/dspbootB.bin $(TARGET_DIR)/lib/firmware/;\
		if [ -n "$(BR2_PACKAGE_AML_RTOS_DSPB_AUTOLOAD)" ]; then \
			$(INSTALL) -D -m 755 \
			$(AML_RTOS_PKGDIR)/S71_load_dspb \
			$(TARGET_DIR)/etc/init.d/;\
		fi \
	fi
endef

endif
$(eval $(generic-package))

