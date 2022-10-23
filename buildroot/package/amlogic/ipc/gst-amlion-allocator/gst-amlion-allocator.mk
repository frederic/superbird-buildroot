################################################################################
#
# gst-amlion-allocator
#
################################################################################

GST_AMLION_ALLOCATOR_VERSION = 1.0
GST_AMLION_ALLOCATOR_SITE = $(TOPDIR)/../vendor/amlogic/ipc/ipc_plugins/gst-amlion-allocator-$(GST_AMLION_ALLOCATOR_VERSION)
GST_AMLION_ALLOCATOR_SITE_METHOD = local
GST_AMLION_ALLOCATOR_LICENSE = LGPL
GST_AMLION_ALLOCATOR_INSTALL_STAGING = YES
GST_AMLION_ALLOCATOR_LICENSE_FILES = COPYING

GST_AMLION_ALLOCATOR_DEPENDENCIES += host-pkgconf gstreamer1 gst1-plugins-base
GST_AMLION_ALLOCATOR_DEPENDENCIES += aml_libion

$(eval $(meson-package))
