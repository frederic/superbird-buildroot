################################################################################
#
# libsoundtouch
#
################################################################################

LIBSOUNDTOUCH_VERSION = 2.1.1
#LIBSOUNDTOUCH_SITE = https://freeswitch.org/stash/scm/sd/libsoundtouch.git
LIBSOUNDTOUCH_SITE = https://gitlab.com/soundtouch/soundtouch.git
LIBSOUNDTOUCH_SITE_METHOD = git
LIBSOUNDTOUCH_LICENSE = LGPL-2.1+
LIBSOUNDTOUCH_LICENSE_FILES = COPYING.TXT
LIBSOUNDTOUCH_AUTORECONF = YES
LIBSOUNDTOUCH_INSTALL_STAGING = YES

define LIBSOUNDTOUCH_CREATE_CONFIG_M4
	mkdir -p $(@D)/config/m4
endef
LIBSOUNDTOUCH_POST_PATCH_HOOKS += LIBSOUNDTOUCH_CREATE_CONFIG_M4
LIBSOUNDTOUCH_CONF_OPTS = --enable-integer-samples

$(eval $(autotools-package))

include package/libsoundtouch/libsoundtouchcwrap/libsoundtouchcwrap.mk
