################################################################################
#
#libtotem
#
###############################################################################


LIBTOTEM_VERSION = 3.10
LIBTOTEM_SOURCE = totem-pl-parser-$(LIBTOTEM_VERSION).6.tar.xz
LIBTOTEM_SITE = http://ftp.gnome.org/pub/gnome/sources/totem-pl-parser/$(LIBTOTEM_VERSION)
LIBTOTEM_INSTALL_STAGING = YES
LIBTOTEM_LICENSE = LGPL V2.1
LIBTOTEM_INSTALL_STAGING = YES
LIBTOTEM_DEPENDENCIES = libglib2 \
                        libtool \
                        glib-networking \
                        libsoup \
                        libxml2 \
                        libgmime \
                        libarchive \
                        libgcrypt
LIBTOTEM_CONF_OPTS = --with-libgcrypt-prefix=$(STAGING_DIR)
ifeq ($(BR2_PACKAGE_ISO_CXX),y)
LIBTOTEM_DEPENDENCIES += iso-cxx
LIBTOTEM_CONF_OPTS += --enable-iso-cxx
endif

ifeq ($(BR2_PACKAGE_GTK_DOC),y)
LIBTOTEM_DEPENDENCIES += gtk-doc
LIBTOTEM_CONF_OPTS += --enable-gtk-doc
endif



$(eval $(autotools-package))
