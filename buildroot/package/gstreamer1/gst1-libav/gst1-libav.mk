################################################################################
#
# gst1-libav
#
################################################################################

GST1_LIBAV_VERSION = 1.14.4
GST1_LIBAV_SOURCE = gst-libav-$(GST1_LIBAV_VERSION).tar.xz
GST1_LIBAV_SITE = https://gstreamer.freedesktop.org/src/gst-libav
GST1_LIBAV_CONF_OPTS =
GST1_LIBAV_DEPENDENCIES = \
	host-pkgconf gstreamer1 gst1-plugins-base \
	$(if $(BR2_PACKAGE_BZIP2),bzip2) \
	$(if $(BR2_PACKAGE_XZ),xz)
GST1_LIBAV_LICENSE = GPL-2.0+
GST1_LIBAV_LICENSE_FILES = COPYING

ifeq ($(BR2_PACKAGE_GST1_LIBAV_WITH_FFMMPEG),y)
GST1_LIBAV_CONF_OPTS += --with-system-libav
GST1_LIBAV_DEPENDENCIES += ffmpeg
endif

$(eval $(autotools-package))
