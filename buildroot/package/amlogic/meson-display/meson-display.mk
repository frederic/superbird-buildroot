################################################################################
#
# mesa3d-demos
#
################################################################################

MESON_DISPLAY_VERSION = 572b93d7
MESON_DISPLAY_SITE = $(TOPDIR)/../vendor/amlogic/meson_display/display_framework
MESON_DISPLAY_SITE_METHOD = local
MESON_DISPLAY_AUTORECONF = YES
MESON_DISPLAY_DEPENDENCIES = host-pkgconf libdrm json-c
MESON_DISPLAY_LICENSE_FILES = COPYING
MESON_DISPLAY_INSTALL_STAGING = YES
MESON_DISPLAY_INSTALL_TARGET = YES

$(eval $(autotools-package))
