################################################################################
#
# amlogic npu driver
#
################################################################################
ifeq ($(BR2_PACKAGE_NPU_LOCAL),y)
#BR2_PACKAGE_NPU_LOCAL_PATH=$(TOPDIR)/../hardware/aml-4.9/npu/nanoq
NPU_SITE = $(call qstrip,$(BR2_PACKAGE_NPU_LOCAL_PATH))
NPU_SITE_METHOD = local
NPU_VERSION = 1.0
ARM_NPU_MODULE_DIR = kernel/amlogic/npu
NPU_KO_INSTALL_DIR=$(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/kernel/amlogic/npu
NPU_SO_INSTALL_DIR=$(TARGET_DIR)/usr/lib
NPU_DEPENDENCIES = linux

ARM_NPU_DEP = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep

define copy-arm-npu
        $(foreach m, $(shell find $(strip $(1)) -name "*.ko"),\
                $(shell [ ! -e $(2) ] && mkdir $(2) -p;\
                cp $(m) $(strip $(2))/ -rfa;\
                echo $(4)/$(notdir $(m)): >> $(3)))
endef

define ARM_NPU_DEP_INSTALL_TARGET_CMDS
        $(call copy-arm-npu,$(@D),\
                $(shell echo $(NPU_KO_INSTALL_DIR)),\
                $(shell echo $(ARM_NPU_DEP)),\
                $(ARM_NPU_MODULE_DIR))
endef

ifeq ($(BR2_aarch64), y)
NPU_INSTALL_TARGETS_CMDS = \
	$(INSTALL) -m 0755 $(@D)/build/sdk/drivers/galcore.ko $(NPU_KO_INSTALL_DIR); \
	$(INSTALL) -m 0755 $(@D)/sharelib/lib64/* $(NPU_SO_INSTALL_DIR); \
	$(INSTALL) -m 0755 $(@D)/nnapi/lib/lib64/* $(NPU_SO_INSTALL_DIR); \
	if [ -n "$(BR2_PACKAGE_NPU_NBG_IMAGE)" ]; then \
		$(INSTALL) -m 0644 $(@D)/NBG/$(BR2_PACKAGE_NPU_NBG_IMAGE) \
			$(BINARIES_DIR)/NBG.img; \
	fi
NNAPI_SO_PATH=$(@D)/nnapi/lib/lib64
else
NPU_INSTALL_TARGETS_CMDS = \
	$(INSTALL) -m 0755 $(@D)/build/sdk/drivers/galcore.ko $(NPU_KO_INSTALL_DIR); \
	$(INSTALL) -m 0755 $(@D)/sharelib/lib32/* $(NPU_SO_INSTALL_DIR); \
	$(INSTALL) -m 0755 $(@D)/nnapi/lib/lib32/* $(NPU_SO_INSTALL_DIR); \
	if [ -n "$(BR2_PACKAGE_NPU_NBG_IMAGE)" ]; then \
		$(INSTALL) -m 0644 $(@D)/NBG/$(BR2_PACKAGE_NPU_NBG_IMAGE) \
			$(BINARIES_DIR)/NBG.img; \
	fi
NNAPI_SO_PATH=$(@D)/nnapi/lib/lib32
endif

define NPU_INSTALL_STAGING_CMDS
    $(INSTALL) -D -m 0644 $(@D)/nnapi/include/* $(STAGING_DIR)/usr/include/ \
    $(INSTALL) -m 0644 $(NNAPI_SO_PATH)/* $(NPU_SO_INSTALL_DIR);
endef


path = 	$(@D)
define NPU_BUILD_CMDS
	cd $(@D);./aml_buildroot.sh $(KERNEL_ARCH) $(LINUX_DIR) $(TARGET_KERNEL_CROSS)
endef
define NPU_INSTALL_TARGET_CMDS
	mkdir -p $(NPU_KO_INSTALL_DIR)
	$(NPU_INSTALL_TARGETS_CMDS)
	$(ARM_NPU_DEP_INSTALL_TARGET_CMDS)
endef

endif
$(eval $(generic-package))
