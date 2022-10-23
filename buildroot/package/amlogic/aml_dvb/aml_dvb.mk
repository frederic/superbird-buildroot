#############################################################
#
# aml dvb libs
#
#############################################################
AML_DVB_VERSION = 1.0
AML_DVB_SITE = $(TOPDIR)/../vendor/amlogic/dvb
AML_DVB_SITE_METHOD = local
AML_DVB_INSTALL_STAGING = YES
AML_DVB_DEPENDENCIES = aml_zvbi libplayer sqlite

define AML_DVB_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) all
endef

define AML_DVB_INSTALL_TARGET_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) install
endef

define AML_DVB_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0644 $(@D)/am_adp/libam_adp.so $(STAGING_DIR)/usr/lib/
	cp -rf $(@D)/include/am_adp $(STAGING_DIR)/usr/include/
	cp -rf $(@D)/include/am_mw  $(STAGING_DIR)/usr/include/
endef

define AML_DVB_CLEAN_CMDS
	$(MAKE) -C $(@D) clean
endef

$(eval $(generic-package))
