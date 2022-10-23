################################################################################
#
# libffi
#
################################################################################

LIBFFI_VERSION = v3.3-rc0
LIBFFI_SITE = $(call github,libffi,libffi,$(LIBFFI_VERSION))
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
