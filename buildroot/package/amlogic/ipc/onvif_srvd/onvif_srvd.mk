#############################################################
#
# Onvif Server
#
#############################################################

ONVIF_SRVD_VERSION = 4c61623b6d3388ae1506957e62c943c71cac8545

ONVIF_SRVD_LICENSE = BSD 3-Clause
#ONVIF_SRVD_LICENSE_FILES = COPYING
ONVIF_SRVD_DEPENDENCIES = host-openssl openssl zlib
ONVIF_SRVD_DEPENDENCIES += onvif_wsdd onvif_rtsp

ONVIF_SRVD_SITE = $(call github,KoynovStas,onvif_srvd,$(ONVIF_SRVD_VERSION))

ONVIF_SRVD_SDK_VERSION = 2.8.94
ONVIF_SRVD_EXTRA_DOWNLOADS = https://sourceforge.net/projects/gsoap2/files/gsoap-2.8/gsoap_$(ONVIF_SRVD_SDK_VERSION).zip

ONVIF_SRVD_INTERNAL_SITE = $(TOPDIR)/../vendor/amlogic/ipc/onvif_srvd

define ONVIF_SRVD_COPY_SDK
	mkdir -p $(@D)/SDK
	cp -af $(ONVIF_SRVD_DL_DIR)/gsoap_$(ONVIF_SRVD_SDK_VERSION).zip $(@D)/SDK/gsoap.zip
endef
ONVIF_SRVD_POST_EXTRACT_HOOKS += ONVIF_SRVD_COPY_SDK

define ONVIF_SRVD_COPY_INTERNAL_SRC
   if [ -d $(ONVIF_SRVD_INTERNAL_SITE) ]; then \
     rsync -av --exclude='.git' $(ONVIF_SRVD_INTERNAL_SITE)/ $(ONVIF_SRVD_DIR); \
   fi
endef

ONVIF_SRVD_PRE_BUILD_HOOKS += ONVIF_SRVD_COPY_INTERNAL_SRC

ONVIF_SRVD_MAKE_OPTS = WSSE_ON=1
ONVIF_SRVD_GSOAP_OPENSSL = $(HOST_DIR)/usr
define ONVIF_SRVD_BUILD_CMDS
	$(TARGET_MAKE_ENV) \
	  GCC=$(TARGET_CXX) CFLAGS="$(TARGET_CFLAGS)" \
          PKG_CONFIG=$(PKG_CONFIG_HOST_BINARY) \
	  OPENSSL=$(ONVIF_SRVD_GSOAP_OPENSSL) \
	  $(MAKE) $(ONVIF_SRVD_MAKE_OPTS) -C $(@D) release
endef

ONVIF_SRVD_INSTALL_DIR = $(TARGET_DIR)/usr/bin/
ONVIF_SRVD_SCRIPTS_INSTALL_DIR = $(TARGET_DIR)/etc/init.d
ONVIF_SRVD_CONF_INSTALL_DIR = $(TARGET_DIR)/etc/onvif
ONVIF_SRVD_WSDL_INSTALL_DIR = $(TARGET_DIR)/etc/onvif/wsdl

define ONVIF_SRVD_INSTALL_TARGET_CMDS
	mkdir -p $(ONVIF_SRVD_INSTALL_DIR)
	$(INSTALL) -D -m 755 $(ONVIF_SRVD_DIR)/onvif_srvd $(ONVIF_SRVD_INSTALL_DIR)
	$(INSTALL) -D -m 755 $(ONVIF_SRVD_DIR)/start_scripts/S04system_restore $(ONVIF_SRVD_SCRIPTS_INSTALL_DIR)/S04system_restore
	$(INSTALL) -D -m 755 $(ONVIF_SRVD_DIR)/start_scripts/S90onvif_srvd $(ONVIF_SRVD_SCRIPTS_INSTALL_DIR)/S91onvif_srvd
	mkdir -p $(ONVIF_SRVD_WSDL_INSTALL_DIR)
	cp -af $(ONVIF_SRVD_DIR)/wsdl/*.{xsd,wsdl} $(ONVIF_SRVD_WSDL_INSTALL_DIR)
	cp $(ONVIF_SRVD_DIR)/start_scripts/ipc_backup_restore.sh $(ONVIF_SRVD_CONF_INSTALL_DIR)
	cp $(ONVIF_SRVD_DIR)/start_scripts/ipc.backup.persist.files.list $(ONVIF_SRVD_CONF_INSTALL_DIR)
endef

$(eval $(generic-package))
