#############################################################
#
# Onvif WSDD Server
#
#############################################################

ONVIF_WSDD_VERSION = ca60dcc5ee52fe466fd684fdbb880832ee031fae

ONVIF_WSDD_LICENSE = BSD 3-Clause
#ONVIF_WSDD_LICENSE_FILES = COPYING
ONVIF_WSDD_DEPENDENCIES = host-openssl openssl zlib

ONVIF_WSDD_SITE = $(call github,KoynovStas,wsdd,$(ONVIF_WSDD_VERSION))

ONVIF_WSDD_SDK_VERSION = 2.8.94
ONVIF_WSDD_EXTRA_DOWNLOADS = https://sourceforge.net/projects/gsoap2/files/gsoap-2.8/gsoap_$(ONVIF_WSDD_SDK_VERSION).zip

ONVIF_WSDD_INTERNAL_SITE = $(TOPDIR)/../vendor/amlogic/ipc/onvif_wsdd

define ONVIF_WSDD_COPY_SDK
	mkdir -p $(@D)/SDK
	cp -af $(ONVIF_WSDD_DL_DIR)/gsoap_$(ONVIF_WSDD_SDK_VERSION).zip $(@D)/SDK/gsoap.zip
endef

ONVIF_WSDD_POST_EXTRACT_HOOKS += ONVIF_WSDD_COPY_SDK

define ONVIF_WSDD_COPY_INTERNAL_SRC
   if [ -d $(ONVIF_WSDD_INTERNAL_SITE) ]; then \
     rsync -av --exclude='.git' $(ONVIF_WSDD_INTERNAL_SITE)/ $(ONVIF_WSDD_DIR); \
   fi
endef

ONVIF_WSDD_PRE_BUILD_HOOKS += ONVIF_WSDD_COPY_INTERNAL_SRC

ONVIF_WSDD_GSOAP_OPENSSL = $(HOST_DIR)/usr
define ONVIF_WSDD_BUILD_CMDS
	$(TARGET_MAKE_ENV) \
	  GCC=$(TARGET_CXX) CFLAGS="$(TARGET_CFLAGS)" \
	  OPENSSL=$(ONVIF_WSDD_GSOAP_OPENSSL) \
	  $(MAKE) $(ONVIF_WSDD_MAKE_OPTS) -C $(@D) release
endef

ONVIF_WSDD_INSTALL_DIR = $(TARGET_DIR)/usr/bin/
ONVIF_WSDD_SCRIPTS_INSTALL_DIR = $(TARGET_DIR)/etc/init.d
ONVIF_WSDD_WSDL_INSTALL_DIR = $(TARGET_DIR)/etc/onvif/wsdl

define ONVIF_WSDD_INSTALL_TARGET_CMDS
	mkdir -p $(ONVIF_WSDD_INSTALL_DIR)
	$(INSTALL) -D -m 755 $(ONVIF_WSDD_DIR)/wsdd            $(ONVIF_WSDD_INSTALL_DIR)/onvif_wsdd
	$(INSTALL) -D -m 755 $(ONVIF_WSDD_DIR)/start_scripts/S90wsdd  $(ONVIF_WSDD_SCRIPTS_INSTALL_DIR)/S91onvif_wsdd
	mkdir -p $(ONVIF_WSDD_WSDL_INSTALL_DIR)
	cp -af $(ONVIF_WSDD_DIR)/wsdl/*.wsdl $(ONVIF_WSDD_WSDL_INSTALL_DIR)
endef

$(eval $(generic-package))
