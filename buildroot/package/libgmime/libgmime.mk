################################################################################
#
#libgmime
#
#################################################################################

LIBGMIME_VERSION = 2.6
LIBGMIME_SOURCE = gmime-$(LIBGMIME_VERSION).20.tar.xz
LIBGMIME_SITE = http://mirror.umd.edu/gnome/sources/gmime/$(LIBGMIME_VERSION)
LIBGMIME_LICENSE = LGPL v2.1
LIBGMIME_INSTALL_STAGING = YES
LIBGMIME_DEPENDENCIES += libgpg-error \
                         libglib2 \
                         libtool
LIBGMIME_CONF_OPTS += --enable-maintainer-mode \
                      ac_cv_have_iconv_detect_h=no

ifeq ($(BR2_PACKAGE_GTK-DOC),y)
LIBGMIME_DEPENDENCIES += gtk-doc
LIBGMIME_CONF_OPTS += enable-gtk-doc
endif

ifeq ($(BR2_PACKAGE_SMIME),y)
LIBGMIME_DEPENDENCIES += smime
LIBGMIME_CONF_OPTS += --enable-smime
endif

ifeq ($(BR2_PACKAGE_MONO),y)
LIBGMIME_DEPENDENCIES += mono
LIBGMIME_CONF_OPTS += --enable-mono
endif

$(eval $(autotools-package))
