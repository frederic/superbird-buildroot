################################################################################
#
# amlogic 8723DS driver
#
################################################################################

RTK8723DS_VERSION = $(call qstrip,$(BR2_PACKAGE_RTK8723DS_GIT_VERSION))
RTK8723DS_SITE = $(call qstrip,$(BR2_PACKAGE_RTK8723DS_GIT_REPO_URL))
RTK8723DS_MODULE_DIR = kernel/realtek/wifi
RTK8723DS_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(RTK8723DS_MODULE_DIR)

ifeq ($(BR2_PACKAGE_RTK8723DS_LOCAL),y)
RTK8723DS_SITE = $(call qstrip,$(BR2_PACKAGE_RTK8723DS_LOCAL_PATH))
RTK8723DS_SITE_METHOD = local
endif
ifeq ($(BR2_PACKAGE_RTK8723DS_STANDALONE),y)
RTK8723DS_DEPENDENCIES = linux
define RTK8723DS_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/rtl8723DS ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_KERNEL_CROSS) modules
endef
define RTK8723DS_INSTALL_TARGET_CMDS
	mkdir -p $(RTK8723DS_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/rtl8723DS/8723ds.ko $(RTK8723DS_INSTALL_DIR)
	echo $(RTK8723DS_MODULE_DIR)/8723ds.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
else
define RTK8723DS_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/wifi/realtek/drivers;
	ln -sf $(RTK8723DS_DIR) $(LINUX_DIR)/../hardware/wifi/realtek/drivers/8723dS
endef
endif
$(eval $(generic-package))
