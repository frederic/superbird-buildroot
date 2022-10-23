#############################################################
#
# nagra app
#
#############################################################
NAGRA_APP_VERSION = 1.0
NAGRA_APP_SITE = $(TOPDIR)/../vendor/amlogic/nagra-app
NAGRA_APP_SITE_METHOD=local
NAGRA_APP_INSTALL_STAGING = YES
NAGRA_APP_DEPENDENCIES = aml_dvb libplayer

define NAGRA_APP_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) STAGING_DIR=$(STAGING_DIR) -C $(@D)/src -f Makefile.ut
endef

define NAGRA_APP_INSTALL_TARGET_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/src install -f Makefile.ut
endef

define NAGRA_APP_CLEAN_CMDS
	$(MAKE) -C $(@D)/src clean -f Makefile.ut
endef

$(eval $(generic-package))
