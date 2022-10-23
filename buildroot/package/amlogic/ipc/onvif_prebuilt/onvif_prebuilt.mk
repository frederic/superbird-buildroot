#############################################################
#
# ONVIF PREBUILT
#
#############################################################

ifeq ($(BR2_aarch64),y)
ONVIF_ARCH = arm64
else
ONVIF_ARCH = arm32
endif

ifneq ($(BR2_PACKAGE_AML_SOC_FAMILY_NAME), "")
ONVIF_SOC_NAME = $(strip $(BR2_PACKAGE_AML_SOC_FAMILY_NAME))
endif

ONVIF_SUB_PATH = $(ONVIF_SOC_NAME)/$(ONVIF_ARCH)
ONVIF_PREBUILT_SITE = $(ONVIF_PREBUILT_PKGDIR).
ONVIF_PREBUILT_SITE_METHOD = local

ifeq ($(BR2_PACKAGE_ONVIF_APPLY_PREBUILT),y)
ONVIF_PREBUILT_SITE = $(TOPDIR)/../vendor/amlogic/ipc/onvif_prebuilt
ONVIF_PREBUILT_DIRECTORY=$(ONVIF_PREBUILT_SITE)/$(ONVIF_SUB_PATH)
#We will only apply onvif prebuilt packages, so we need to make sure the original package's dependency can meet.
ONVIF_PREBUILT_DEPENDENCIES = openssl zlib libjpeg directfb sqlite
ONVIF_PREBUILT_DEPENDENCIES += gstreamer1 gst1-plugins-base gst1-plugins-good gst1-plugins-bad gst1-rtsp-server
ONVIF_PREBUILT_DEPENDENCIES += aml_nn_detect aml_libge2d aml_libgdc
ONVIF_PREBUILT_DEPENDENCIES += libwebsockets
ONVIF_PREBUILT_DEPENDENCIES += nginx php
ifeq ($(BR2_PACKAGE_AML_SOC_USE_MULTIENC), y)
GST_PLUGIN_AMLVENC_DEPENDENCIES += libmultienc
else
GST_PLUGIN_AMLVENC_DEPENDENCIES += libvpcodec libvphevcodec
endif
define ONVIF_PREBUILT_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)
	cp -av $(ONVIF_PREBUILT_DIRECTORY)/*  $(TARGET_DIR)/
endef

endif

ifeq ($(BR2_PACKAGE_ONVIF_GENERATE_PREBUILT),y)
#We will package the onvif prebuitl pacakge, so we need to wait for onvif libraries/binraries ready, here we list the onvif binraies.
ONVIF_PREBUILT_DEPENDENCIES = onvif_srvd
ONVIF_PREBUILT_DEPENDENCIES += ipc-webui

ifeq ($(BR2_PACKAGE_AML_SOC_FAMILY_NAME), "C1")
ONVIF_PREBUILT_DEPENDENCIES += ipc_autocap
endif

define ONVIF_PREBUILT_INSTALL_TARGET_CMDS
	rm    -fr $(@D)/$(ONVIF_SUB_PATH)/
	mkdir -p $(@D)/$(ONVIF_SUB_PATH)/

	mkdir -p $(@D)/$(ONVIF_SUB_PATH)/etc/init.d/
	mkdir -p $(@D)/$(ONVIF_SUB_PATH)/usr/bin
	mkdir -p $(@D)/$(ONVIF_SUB_PATH)/usr/lib/gstreamer-1.0/
	mkdir -p $(@D)/$(ONVIF_SUB_PATH)/var/www
	mkdir -p $(@D)/$(ONVIF_SUB_PATH)/etc/nginx

	cp -a $(TARGET_DIR)/etc/ipc.json                       $(@D)/$(ONVIF_SUB_PATH)/etc/
	cp -a $(TARGET_DIR)/etc/onvif                          $(@D)/$(ONVIF_SUB_PATH)/etc/
	cp -a $(TARGET_DIR)/etc/init.d/S45ipc-property-service $(@D)/$(ONVIF_SUB_PATH)/etc/init.d/
	cp -a $(TARGET_DIR)/etc/init.d/S91onvif_rtsp           $(@D)/$(ONVIF_SUB_PATH)/etc/init.d/
	cp -a $(TARGET_DIR)/etc/init.d/S91onvif_srvd           $(@D)/$(ONVIF_SUB_PATH)/etc/init.d/
	cp -a $(TARGET_DIR)/etc/init.d/S91onvif_wsdd           $(@D)/$(ONVIF_SUB_PATH)/etc/init.d/
	cp -a $(TARGET_DIR)/etc/init.d/S49ipc_webui            $(@D)/$(ONVIF_SUB_PATH)/etc/init.d/
	[ -f $(TARGET_DIR)/etc/init.d/S80ipc_autocap ] && \
	cp -a $(TARGET_DIR)/etc/init.d/S80ipc_autocap          $(@D)/$(ONVIF_SUB_PATH)/etc/init.d/ || true

	cp -a $(TARGET_DIR)/usr/bin/onvif_rtsp                 $(@D)/$(ONVIF_SUB_PATH)/usr/bin/
	cp -a $(TARGET_DIR)/usr/bin/onvif_srvd                 $(@D)/$(ONVIF_SUB_PATH)/usr/bin/
	cp -a $(TARGET_DIR)/usr/bin/onvif_wsdd                 $(@D)/$(ONVIF_SUB_PATH)/usr/bin/
	cp -a $(TARGET_DIR)/usr/bin/ipc-property               $(@D)/$(ONVIF_SUB_PATH)/usr/bin/
	cp -a $(TARGET_DIR)/usr/bin/ipc-property-service       $(@D)/$(ONVIF_SUB_PATH)/usr/bin/
	[ -f $(TARGET_DIR)/usr/bin/ipc_autocap ] && \
	cp -a $(TARGET_DIR)/usr/bin/ipc_autocap                $(@D)/$(ONVIF_SUB_PATH)/usr/bin/ || true

	cp -a $(TARGET_DIR)/usr/lib/libipc-property.so                   $(@D)/$(ONVIF_SUB_PATH)/usr/lib/
	cp -a $(TARGET_DIR)/usr/lib/gstreamer-1.0/libgstamlimgcap.so     $(@D)/$(ONVIF_SUB_PATH)/usr/lib/gstreamer-1.0/
	cp -a $(TARGET_DIR)/usr/lib/gstreamer-1.0/libgstamlnn.so         $(@D)/$(ONVIF_SUB_PATH)/usr/lib/gstreamer-1.0/
	cp -a $(TARGET_DIR)/usr/lib/gstreamer-1.0/libgstamloverlay.so    $(@D)/$(ONVIF_SUB_PATH)/usr/lib/gstreamer-1.0/
	cp -a $(TARGET_DIR)/usr/lib/gstreamer-1.0/libgstamlvenc.so       $(@D)/$(ONVIF_SUB_PATH)/usr/lib/gstreamer-1.0/
	cp -a $(TARGET_DIR)/usr/lib/gstreamer-1.0/libgstamlvconv.so      $(@D)/$(ONVIF_SUB_PATH)/usr/lib/gstreamer-1.0/
	cp -a $(TARGET_DIR)/usr/lib/gstreamer-1.0/libgstamlgdc.so        $(@D)/$(ONVIF_SUB_PATH)/usr/lib/gstreamer-1.0/
	cp -a $(TARGET_DIR)/usr/lib/gstreamer-1.0/libgstamlwsss.so       $(@D)/$(ONVIF_SUB_PATH)/usr/lib/gstreamer-1.0/

	cp -a $(TARGET_DIR)/etc/nginx/nginx.conf       $(@D)/$(ONVIF_SUB_PATH)/etc/nginx
	cp -a $(TARGET_DIR)/etc/php.ini                $(@D)/$(ONVIF_SUB_PATH)/etc/php.ini
	cp -a $(TARGET_DIR)/var/www/ipc-webui          $(@D)/$(ONVIF_SUB_PATH)/var/www

	tar -zcf $(TARGET_DIR)/../images/onvif-prebuilt-$(ONVIF_SOC_NAME)-$(ONVIF_ARCH).tgz -C $(@D) $(ONVIF_SUB_PATH)
endef

endif

$(eval $(generic-package))
