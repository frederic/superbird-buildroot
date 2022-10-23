################################################################################
#
# alsaplayer - PCM audio player for Linux and compatible OSes
#
################################################################################

ALSAPLAYER_SITE = https://sourceforge.net/projects/alsaplayer/files/latest/download
ALSAPLAYER_SOURCE = alsaplayer-$(ALSAPLAYER_VERSION).tar.bz2
ALSAPLAYER_VERSION = 0.99.81
ALSAPLAYER_CONF_OPTS = --enable-esd=no \
					   --enable-oss=no \
					   --with-ogg-prefix=${STAGING_DIR}/usr/lib \
					   --with-vorbis-prefix=${STAGING_DIR}/usr/lib
ALSAPLAYER_AUTORECONF = YES
ALSAPLAYER_DEPENDENCIES = host-pkgconf libglib2 libid3tag libmad libogg flac libvorbis libsndfile
ALSAPLAYER_LICENSE = GPLv2+
ALSAPLAYER_LICENSE_FILES = COPYING
#ALSAPLAYER_CFLAGS=$(TARGET_CFLAGS)
#ALSAPLAYER_LDFLAGS=$(TARGET_LDFLAGS)
ALSAPLAYER_INSTALL_STAGING = yes


$(eval $(autotools-package))
