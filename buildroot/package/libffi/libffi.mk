################################################################################
#
# libffi
#
################################################################################

LIBFFI_VERSION = 3.3
LIBFFI_SITE = $(call github,libffi,libffi,v$(LIBFFI_VERSION))
LIBFFI_LICENSE = MIT
LIBFFI_LICENSE_FILES = LICENSE
LIBFFI_INSTALL_STAGING = YES
LIBFFI_AUTORECONF = YES

define LIBFFI_PATCH_OLDVERSION
	if [ ! -f $(TARGET_DIR)/usr/lib/libffi.so.6 ]; then \
		pushd $(TARGET_DIR)/usr/lib; \
		ln -sv libffi.so.7 libffi.so.6; \
		popd; \
	fi
endef

LIBFFI_POST_INSTALL_TARGET_HOOKS += LIBFFI_PATCH_OLDVERSION

$(eval $(autotools-package))
$(eval $(host-autotools-package))
