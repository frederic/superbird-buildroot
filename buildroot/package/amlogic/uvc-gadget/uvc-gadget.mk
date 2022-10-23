UVC_GADGET_VERSION = 0.1
UVC_GADGET_SITE = $(TOPDIR)/../vendor/amlogic/uvc-gadget
UVC_GADGET_SITE_METHOD = local
UVC_GADGET_INSTALL_STAGING = YES
UVC_GADGET_DEPENDENCIES += ipc-sdk

ifeq ($(BR2_PACKAGE_UVC_GADGET_USE_NN), y)
UVC_GADGET_DEPENDENCIES += ipc-property
UVC_GADGET_DEPENDENCIES += sqlite libjpeg
UVC_GADGET_DEPENDENCIES += aml_nn_detect
UVC_GADGET_CONF_OPTS += -DUVC_USE_NN="y"
endif

define UVC_GADGET_INSTALL_STAGING_CMDS
        $(INSTALL) -D -m 644 $(@D)/lib/libuvcgadget.so $(STAGING_DIR)/usr/lib/
        $(INSTALL) -D -m 644 $(@D)/include/uvcgadget/* $(STAGING_DIR)/usr/include/
endef

define UVC_GADGET_INSTALL_TARGET_CMDS
        $(INSTALL) -D -m 755 $(@D)/lib/libuvcgadget.so $(TARGET_DIR)/usr/lib/
        $(INSTALL) -D -m 755 $(@D)/uvc-gadget $(TARGET_DIR)/usr/bin/
        $(INSTALL) -D -m 755 $(@D)/S90uvcgadget $(TARGET_DIR)/etc/init.d/
	if [ "${BR2_PACKAGE_UVC_GADGET_USE_NN}" == "y" ]; then \
	      mkdir -p $(TARGET_DIR)/etc/ipc; \
              $(INSTALL) -D -m 644 $(@D)/config.json $(TARGET_DIR)/etc/ipc/; \
        fi
endef

$(eval $(cmake-package))
