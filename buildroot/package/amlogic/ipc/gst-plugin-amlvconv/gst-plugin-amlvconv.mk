################################################################################
#
# gst-plugin-amlvconv
#
################################################################################

GST_PLUGIN_AMLVCONV_VERSION = 1.0
GST_PLUGIN_AMLVCONV_SITE = $(TOPDIR)/../vendor/amlogic/ipc/ipc_plugins/gst-plugin-amlvconv-$(GST_PLUGIN_AMLVCONV_VERSION)
GST_PLUGIN_AMLVCONV_SITE_METHOD = local
GST_PLUGIN_AMLVCONV_LICENSE = LGPL
GST_PLUGIN_AMLVCONV_INSTALL_STAGING = YES
GST_PLUGIN_AMLVCONV_LICENSE_FILES = COPYING

define GST_PLUGIN_AMLVCONV_SYNC_COMMON_SRC
	rsync -avz $(GST_PLUGIN_AMLVCONV_SITE)/../common/ $(@D)/src/
endef
GST_PLUGIN_AMLVCONV_POST_RSYNC_HOOKS += GST_PLUGIN_AMLVCONV_SYNC_COMMON_SRC

define GST_PLUGIN_AMLVCONV_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh
endef
GST_PLUGIN_AMLVCONV_POST_PATCH_HOOKS += GST_PLUGIN_AMLVCONV_RUN_AUTOGEN
GST_PLUGIN_AMLVCONV_DEPENDENCIES += host-automake host-autoconf host-libtool
GST_PLUGIN_AMLVCONV_DEPENDENCIES += gstreamer1 gst1-plugins-base
GST_PLUGIN_AMLVCONV_DEPENDENCIES += gst-amlion-allocator
GST_PLUGIN_AMLVCONV_DEPENDENCIES += aml_libge2d

$(eval $(autotools-package))
