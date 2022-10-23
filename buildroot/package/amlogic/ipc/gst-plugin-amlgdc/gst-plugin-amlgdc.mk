################################################################################
#
# gst-plugin-amlgdc
#
################################################################################

GST_PLUGIN_AMLGDC_VERSION = 1.0
GST_PLUGIN_AMLGDC_SITE = $(TOPDIR)/../vendor/amlogic/ipc/ipc_plugins/gst-plugin-amlgdc-$(GST_PLUGIN_AMLGDC_VERSION)
GST_PLUGIN_AMLGDC_SITE_METHOD = local
GST_PLUGIN_AMLGDC_LICENSE = LGPL
GST_PLUGIN_AMLGDC_INSTALL_STAGING = YES
GST_PLUGIN_AMLGDC_LICENSE_FILES = COPYING

define GST_PLUGIN_AMLGDC_SYNC_COMMON_SRC
	rsync -avz $(GST_PLUGIN_AMLGDC_SITE)/../common/ $(@D)/src/
endef
GST_PLUGIN_AMLGDC_POST_RSYNC_HOOKS += GST_PLUGIN_AMLGDC_SYNC_COMMON_SRC

define GST_PLUGIN_AMLGDC_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh
endef
GST_PLUGIN_AMLGDC_POST_PATCH_HOOKS += GST_PLUGIN_AMLGDC_RUN_AUTOGEN
GST_PLUGIN_AMLGDC_DEPENDENCIES += host-automake host-autoconf host-libtool gstreamer1 gst1-plugins-base
GST_PLUGIN_AMLGDC_DEPENDENCIES += gst-amlion-allocator
GST_PLUGIN_AMLGDC_DEPENDENCIES += aml_libge2d aml_libgdc

$(eval $(autotools-package))
