#############################################################
#
# ONVIF RTSP Server
#
#############################################################

ONVIF_RTSP_VERSION = 0.1
ONVIF_RTSP_SITE = $(TOPDIR)/../vendor/amlogic/ipc/onvif_rtsp
ONVIF_RTSP_SITE_METHOD = local
ONVIF_RTSP_DEPENDENCIES = gst1-plugins-base gst1-plugins-good gst1-plugins-bad gst1-rtsp-server gst1-libav gstreamer1
ONVIF_RTSP_DEPENDENCIES += gst-plugin-amlvenc gst-plugin-amloverlay gst-plugin-amlimgcap
ONVIF_RTSP_DEPENDENCIES += gst-plugin-amlnn
ONVIF_RTSP_DEPENDENCIES += gst-plugin-amlvconv
ONVIF_RTSP_DEPENDENCIES += gst-plugin-amlgdc
ONVIF_RTSP_DEPENDENCIES += gst-plugin-amlwebstreamserversink
ONVIF_RTSP_DEPENDENCIES += ipc-property

define ONVIF_RTSP_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(TARGET_MAKE_ENV) $(MAKE) -C $(@D)
endef

define ONVIF_RTSP_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/onvif_rtsp $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D -m 755 $(ONVIF_RTSP_PKGDIR)/S91onvif_rtsp $(TARGET_DIR)/etc/init.d/
endef

$(eval $(generic-package))
