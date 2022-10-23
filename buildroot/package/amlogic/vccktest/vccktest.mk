#
# vccktest
#
VCCKTEST_VERSION = 1.0
VCCKTEST_SITE = $(TOPDIR)/../vendor/amlogic/vccktest
VCCKTEST_SITE_METHOD = local

define VCCKTEST_BUILD_CMDS
	$(MAKE) CC=$(TARGET_CC) -C $(@D)
endef
define VCCKTEST_INSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) install
endef


$(eval $(generic-package))
