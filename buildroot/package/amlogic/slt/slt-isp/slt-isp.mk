#
# SLT ISP
#
SLT_ISP_VERSION = 1.0
SLT_ISP_SITE = $(TOPDIR)/../hardware/aml-4.9/amlogic/arm_isp/slt_test
SLT_ISP_SITE_METHOD = local
SLT_ISP_DEPENDENCIES = arm_isp

define SLT_ISP_BUILD_CMDS
    $(MAKE) CC=$(TARGET_CC) -C $(@D)
endef

define SLT_ISP_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/slt-isp $(TARGET_DIR)/usr/bin/
endef

define SLT_ISP_INSTALL_CLEAN_CMDS
    $(MAKE) CC=$(TARGET_CC) -C $(@D) clean
endef

$(eval $(generic-package))
