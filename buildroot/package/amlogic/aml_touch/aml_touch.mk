################################################################################
#
# amlogic touch driver
#
################################################################################

AML_TOUCH_VERSION = $(call qstrip,$(BR2_PACKAGE_AML_TOUCH_GIT_VERSION))
AML_TOUCH_SITE = $(call qstrip,$(BR2_PACKAGE_AML_TOUCH_GIT_REPO_URL))

ifeq ($(BR2_PACKAGE_AML_TOUCH_LOCAL),y)
AML_TOUCH_SITE = $(call qstrip,$(BR2_PACKAGE_AML_TOUCH_LOCAL_PATH))
AML_TOUCH_SITE_METHOD = local
endif
define AML_TOUCH_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/amlogic;
	ln -sf $(AML_TOUCH_DIR) $(LINUX_DIR)/../hardware/amlogic/touch
endef
$(eval $(generic-package))
