################################################################################
#
# gst-plugin-amlvenc
#
################################################################################

GST_PLUGIN_AMLVENC_VERSION = 1.0
GST_PLUGIN_AMLVENC_SITE = $(TOPDIR)/../vendor/amlogic/ipc/ipc_plugins/gst-plugin-amlvenc-$(GST_PLUGIN_AMLVENC_VERSION)
GST_PLUGIN_AMLVENC_SITE_METHOD = local
GST_PLUGIN_AMLVENC_LICENSE = LGPL
GST_PLUGIN_AMLVENC_INSTALL_STAGING = YES
GST_PLUGIN_AMLVENC_LICENSE_FILES = COPYING

define GST_PLUGIN_AMLVENC_SYNC_COMMON_SRC
	rsync -avz $(GST_PLUGIN_AMLVENC_SITE)/../common/ $(@D)/src/
endef
GST_PLUGIN_AMLVENC_POST_RSYNC_HOOKS += GST_PLUGIN_AMLVENC_SYNC_COMMON_SRC

define GST_PLUGIN_AMLVENC_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh
endef
GST_PLUGIN_AMLVENC_POST_PATCH_HOOKS += GST_PLUGIN_AMLVENC_RUN_AUTOGEN
GST_PLUGIN_AMLVENC_DEPENDENCIES += host-automake host-autoconf host-libtool
GST_PLUGIN_AMLVENC_DEPENDENCIES += gstreamer1 gst1-plugins-base
GST_PLUGIN_AMLVENC_DEPENDENCIES += aml_libge2d
ifeq ($(BR2_PACKAGE_AML_SOC_USE_MULTIENC), y)
GST_PLUGIN_AMLVENC_DEPENDENCIES += libmultienc
GST_PLUGIN_AMLVENC_DEPENDENCIES += gst-amlion-allocator
GST_PLUGIN_AMLVENC_CONF_OPTS += --with-libmultienc
else
GST_PLUGIN_AMLVENC_DEPENDENCIES += libvpcodec libvphevcodec
endif

$(eval $(autotools-package))
