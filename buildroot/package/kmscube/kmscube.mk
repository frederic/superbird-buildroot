################################################################################
#
# mesa3d-demos
#
################################################################################

KMSCUBE_VERSION = 572b93d7
KMSCUBE_SITE = https://gitlab.freedesktop.org/mesa/kmscube.git
KMSCUBE_SITE_METHOD = git
KMSCUBE_AUTORECONF = YES
KMSCUBE_DEPENDENCIES = host-pkgconf libdrm meson-mali
KMSCUBE_LICENSE = MIT
KMSCUBE_LICENSE_FILES = COPYING

ifeq ($(BR2_PACKAGE_GSTREAMER1)$(BR2_PACKAGE_GST1_PLUGINS_BASE_PLUGIN_APP),yy)
KMSCUBE_DEPENDENCIES += gstreamer1 gst1-plugins-base libglib2
endif

$(eval $(autotools-package))
