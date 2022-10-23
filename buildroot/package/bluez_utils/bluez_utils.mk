################################################################################
#
# bluez_utils
#
################################################################################

BLUEZ_UTILS_VERSION = 4.101
BLUEZ_UTILS_SOURCE = bluez-$(BLUEZ_UTILS_VERSION).tar.xz
BLUEZ_UTILS_SITE = $(BR2_KERNEL_MIRROR)/linux/bluetooth
BLUEZ_UTILS_INSTALL_STAGING = YES
BLUEZ_UTILS_DEPENDENCIES = host-pkgconf check dbus libglib2
BLUEZ_UTILS_CONF_OPTS = --enable-test --enable-tools
BLUEZ_UTILS_AUTORECONF = YES
BLUEZ_UTILS_LICENSE = GPL-2.0+, LGPL-2.1+
BLUEZ_UTILS_LICENSE_FILES = COPYING COPYING.LIB

# BlueZ 3.x compatibility
ifeq ($(BR2_PACKAGE_BLUEZ_UTILS_COMPAT),y)
BLUEZ_UTILS_CONF_OPTS += \
	--enable-hidd \
	--enable-pand \
	--enable-sdp \
	--enable-dund
endif

# pair agent needs check package
ifeq ($(BR2_PACKAGE_CHECK),y)
BLUEZ_UTILS_DEPENDENCIES +=	check

define BLUEZ_UTILS_INSTALL_AGENT
	$(INSTALL) -m 755 $(@D)/test/agent $(TARGET_DIR)/usr/bin
endef

endif

# audio support
ifeq ($(BR2_PACKAGE_BLUEZ_UTILS_AUDIO),y)
BLUEZ_UTILS_DEPENDENCIES += \
	alsa-lib \
	libsndfile
BLUEZ_UTILS_CONF_OPTS += \
	--enable-alsa \
	--enable-audio
else
BLUEZ_UTILS_CONF_OPTS += \
	--disable-alsa \
	--disable-audio
endif

ifeq ($(BR2_PACKAGE_BLUEZ_UTILS_GATT),y)
BLUEZ_UTILS_DEPENDENCIES += readline
BLUEZ_UTILS_CONF_OPTS += --enable-gatt
else
BLUEZ_UTILS_CONF_OPTS += --disable-gatt
endif

# USB support
ifeq ($(BR2_PACKAGE_BLUEZ_UTILS_USB),y)
BLUEZ_UTILS_DEPENDENCIES += libusb
BLUEZ_UTILS_CONF_OPTS += \
	--enable-usb
else
BLUEZ_UTILS_CONF_OPTS += \
	--disable-usb
endif

ifeq ($(BR2_TOOLCHAIN_SUPPORTS_PIE),)
BLUEZ_UTILS_CONF_OPTS += --disable-pie
endif

define BLUEZ_UTILS_INSTALL_INIT_SYSV
	$(BLUEZ_UTILS_INSTALL_AGENT)
	$(INSTALL) -D -m 755 package/bluez_utils/bluez_tool.sh $(TARGET_DIR)/usr/bin
endef

$(eval $(autotools-package))
