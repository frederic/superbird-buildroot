################################################################################
#
# amlogic 8822cs driver
#
################################################################################

RTK8822CS_VERSION = $(call qstrip,$(BR2_PACKAGE_RTK8822CS_GIT_VERSION))
RTK8822CS_SITE = $(call qstrip,$(BR2_PACKAGE_RTK8822CS_GIT_REPO_URL))
RTK8822CS_MODULE_DIR = kernel/realtek/wifi
RTK8822CS_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(RTK8822CS_MODULE_DIR)

ifeq ($(BR2_PACKAGE_RTK8822CS_LOCAL),y)
RTK8822CS_SITE = $(call qstrip,$(BR2_PACKAGE_RTK8822CS_LOCAL_PATH))
RTK8822CS_SITE_METHOD = local
endif
ifeq ($(BR2_PACKAGE_RTK8822CS_STANDALONE),y)
RTK8822CS_DEPENDENCIES = linux
endif

ifeq ($(BR2_PACKAGE_RTK8822CS_STANDALONE),y)
define RTK8822CS_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/rtl88x2CS ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_KERNEL_CROSS) modules
endef
define RTK8822CS_INSTALL_TARGET_CMDS
	mkdir -p $(RTK8822CS_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/rtl88x2CS/8822cs.ko $(RTK8822CS_INSTALL_DIR)
	echo $(RTK8822CS_MODULE_DIR)/8822cs.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
else

define RTK8822CS_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/wifi/realtek/drivers;
	ln -sf $(RTK8822CS_DIR) $(LINUX_DIR)/../hardware/wifi/realtek/drivers/8822cs
endef
endif
$(eval $(generic-package))
