#############################################################
#
# remotecfg
#
#############################################################
REMOTECFG_VERSION:=v1.1.0
REMOTECFG_SITE=$(TOPDIR)/../vendor/amlogic/remotecfg
REMOTECFG_SITE_METHOD=local

define REMOTECFG_BUILD_CMDS
	$(MAKE) CC=$(TARGET_CC) -C $(@D)/remotecfg
endef

define REMOTECFG_CLEAN_CMDS
	$(MAKE) -C $(@D)/remotecfg clean
endef

define REMOTECFG_INSTALL_TARGET_CMDS
	$(MAKE) -C $(@D)/remotecfg install
endef

define REMOTECFG_UNINSTALL_TARGET_CMDS
        $(MAKE) -C $(@D)/remotecfg uninstall
endef

$(eval $(generic-package))
