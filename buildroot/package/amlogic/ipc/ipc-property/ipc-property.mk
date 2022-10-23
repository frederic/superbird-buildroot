#############################################################
#
# IPC Property
#
#############################################################

IPC_PROPERTY_VERSION = 1.0
IPC_PROPERTY_SITE = $(TOPDIR)/../vendor/amlogic/ipc/ipc_property
IPC_PROPERTY_SITE_METHOD = local
IPC_PROPERTY_DEPENDENCIES = ipc-property-json
IPC_PROPERTY_INSTALL_STAGING = YES

define IPC_PROPERTY_PREPARE_JSON_LIB
	mkdir -p $(@D)/json
	$(INSTALL) -D -m 644 $(IPC_PROPERTY_JSON_DL_DIR)/json.hpp $(@D)/json/
endef

IPC_PROPERTY_PRE_CONFIGURE_HOOKS += IPC_PROPERTY_PREPARE_JSON_LIB

define IPC_PROPERTY_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 644 $(@D)/libipc-property.so $(STAGING_DIR)/usr/lib/
	$(INSTALL) -D -m 644 $(@D)/include/ipc_property.h $(STAGING_DIR)/usr/include/
endef

define IPC_PROPERTY_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/ipc-property $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D -m 755 $(@D)/ipc-property-service $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D -m 644 $(@D)/libipc-property.so $(TARGET_DIR)/usr/lib/
	$(INSTALL) -D -m 755 $(IPC_PROPERTY_PKGDIR)/S45ipc-property-service $(TARGET_DIR)/etc/init.d/
	$(INSTALL) -D -m 644 $(IPC_PROPERTY_PKGDIR)/ipc.json $(TARGET_DIR)/etc/
endef

$(eval $(cmake-package))

include $(IPC_PROPERTY_PKGDIR)/ipc-property-json/*.mk
