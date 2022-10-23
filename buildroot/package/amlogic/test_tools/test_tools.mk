################################################################################
#
# test tools
#
################################################################################
TEST_TOOLS_VERSION = 20170616
TEST_TOOLS_SITE = $(TOPDIR)/../vendor/amlogic/test_tools
TEST_TOOLS_SITE_METHOD = local

define TEST_TOOLS_INSTALL_TARGET_CMDS
	pushd $(@D)/test_plan; ./install_prebuilt_bin.sh $(BR2_ARCH); popd
	rsync -arv  $(@D)/test_plan  ${TARGET_DIR}/ --exclude=prebuilt --exclude=src --exclude=install_prebuilt_bin.sh
endef

$(eval $(generic-package))
