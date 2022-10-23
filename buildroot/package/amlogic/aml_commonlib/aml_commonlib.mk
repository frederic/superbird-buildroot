AML_COMMONLIB_VERSION=1.0
AML_COMMONLIB_SITE_METHOD=local
AML_COMMONLIB_SITE=$(TOPDIR)/../vendor/amlogic/aml_commonlib
define AML_COMMONLIB_INSTALL_TARGET_CMDS
	$(INSTALL) -m 644 -D $(@D)/socketipc/socketipc.h $(STAGING_DIR)/usr/include/
endef

$(eval $(generic-package))
