################################################################################
#
# amlogic minigbm test
#
################################################################################
AML_MINIGBMTEST_VERSION = 2018
AML_MINIGBMTEST_SITE = $(TOPDIR)/../vendor/amlogic/minigbm/test
AML_MINIGBMTEST_SITE_METHOD = local
AML_MINIGBMTEST_DEPENDENCIES = libdrm aml_minigbm
AML_MINIGBMTEST_INSTALL_STAGING = NO

MINIGBMTEST_FILENAME := gbmtest

define AML_MINIGBMTEST_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) all
endef

define AML_MINIGBMTEST_INSTALL_TARGET_CMDS
	cp -a $(AML_MINIGBMTEST_DIR)/$(MINIGBMTEST_FILENAME) $(TARGET_DIR)/usr/bin/
endef

$(eval $(generic-package))
