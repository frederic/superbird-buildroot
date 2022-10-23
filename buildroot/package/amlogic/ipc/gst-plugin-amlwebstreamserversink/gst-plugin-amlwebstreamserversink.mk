################################################################################
#
# gst-plugin-amlwebstreamserversink
#
################################################################################

GST_PLUGIN_AMLWEBSTREAMSERVERSINK_VERSION = 1.0
GST_PLUGIN_AMLWEBSTREAMSERVERSINK_SITE = $(TOPDIR)/../vendor/amlogic/ipc/ipc_plugins/gst-plugin-amlwebstreamserversink-$(GST_PLUGIN_AMLWEBSTREAMSERVERSINK_VERSION)
GST_PLUGIN_AMLWEBSTREAMSERVERSINK_SITE_METHOD = local
GST_PLUGIN_AMLWEBSTREAMSERVERSINK_LICENSE = LGPL
GST_PLUGIN_AMLWEBSTREAMSERVERSINK_INSTALL_STAGING = YES
GST_PLUGIN_AMLWEBSTREAMSERVERSINK_LICENSE_FILES = COPYING
#GST_PLUGIN_AMLWEBSTREAMSERVERSINK_CONF_OPTS = "--buildtype=debug"

GST_PLUGIN_AMLWEBSTREAMSERVERSINK_DEPENDENCIES += host-pkgconf gstreamer1 libwebsockets

$(eval $(meson-package))
