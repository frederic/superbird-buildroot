################################################################################
#
# shairport-sync
#
################################################################################

SHAIRPORT_SYNC_VERSION = 3.3.5
SHAIRPORT_SYNC_SITE = $(call github,mikebrady,shairport-sync,$(SHAIRPORT_SYNC_VERSION))

SHAIRPORT_SYNC_LICENSE = MIT, BSD-3-Clause
SHAIRPORT_SYNC_LICENSE_FILES = LICENSES
SHAIRPORT_SYNC_DEPENDENCIES = alsa-lib libconfig popt host-pkgconf

# git clone, no configure
SHAIRPORT_SYNC_AUTORECONF = YES

SHAIRPORT_SYNC_CONF_OPTS = --with-alsa \
	--with-metadata \
	--with-pipe \
	--with-stdout \
	--with-libdaemon

SHAIRPORT_SYNC_CONF_ENV += LIBS="$(SHAIRPORT_SYNC_CONF_LIBS)"

# Avahi or tinysvcmdns (shaiport-sync bundles its own version of tinysvcmdns).
# Avahi support needs libavahi-client, which is built by avahi if avahi-daemon
# and dbus is selected. Since there is no BR2_PACKAGE_LIBAVAHI_CLIENT config
# option yet, use the avahi-daemon and dbus congig symbols to check for
# libavahi-client.
ifeq ($(BR2_PACKAGE_AVAHI_DAEMON)$(BR2_PACKAGE_DBUS),yy)
SHAIRPORT_SYNC_DEPENDENCIES += avahi
SHAIRPORT_SYNC_CONF_OPTS += --with-avahi
else
SHAIRPORT_SYNC_CONF_OPTS += --with-tinysvcmdns
endif

ifeq ($(BR2_PACKAGE_PULSEAUDIO), y)
SHAIRPORT_SYNC_CONFIG_ARCH = shairport-sync-pulse.conf
else
ifeq ($(BR2_aarch64),y)
SHAIRPORT_SYNC_CONFIG_ARCH = shairport-sync-64.conf
else
SHAIRPORT_SYNC_CONFIG_ARCH = shairport-sync-32.conf
endif
endif

ifeq ($(BR2_PACKAGE_LIBDAEMON),y)
SHAIRPORT_SYNC_DEPENDENCIES += libdaemon
SHAIRPORT_SYNC_CONF_OPTS += --with-libdaemon
endif

# OpenSSL or mbedTLS
ifeq ($(BR2_PACKAGE_OPENSSL),y)
SHAIRPORT_SYNC_DEPENDENCIES += openssl
SHAIRPORT_SYNC_CONF_OPTS += --with-ssl=openssl
else
SHAIRPORT_SYNC_DEPENDENCIES += mbedtls
SHAIRPORT_SYNC_CONF_OPTS += --with-ssl=mbedtls
SHAIRPORT_SYNC_CONF_LIBS += -lmbedx509 -lmbedcrypto
ifeq ($(BR2_PACKAGE_MBEDTLS_COMPRESSION),y)
SHAIRPORT_SYNC_CONF_LIBS += -lz
endif
endif

ifeq ($(BR2_PACKAGE_SHAIRPORT_SYNC_CONVOLUTION),y)
SHAIRPORT_SYNC_DEPENDENCIES += libsndfile
SHAIRPORT_SYNC_CONF_OPTS += --with-convolution
endif

ifeq ($(BR2_PACKAGE_SHAIRPORT_SYNC_DBUS),y)
SHAIRPORT_SYNC_DEPENDENCIES += libglib2
SHAIRPORT_SYNC_CONF_OPTS += --with-dbus-interface --with-mpris-interface
endif

ifeq ($(BR2_PACKAGE_SHAIRPORT_SYNC_LIBSOXR),y)
SHAIRPORT_SYNC_DEPENDENCIES += libsoxr
SHAIRPORT_SYNC_CONF_OPTS += --with-soxr
endif

ifeq ($(BR2_PACKAGE_SHAIRPORT_SYNC_MQTT),y)
SHAIRPORT_SYNC_DEPENDENCIES += avahi dbus mosquitto
SHAIRPORT_SYNC_CONF_OPTS += --with-mqtt-client
endif

define SHAIRPORT_SYNC_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/shairport-sync \
		$(TARGET_DIR)/usr/bin/shairport-sync
	$(INSTALL) -D -m 0644 package/shairport-sync/$(SHAIRPORT_SYNC_CONFIG_ARCH) \
		$(TARGET_DIR)/etc/shairport-sync.conf
endef

define SHAIRPORT_SYNC_INSTALL_INIT_SYSV
	$(INSTALL) -D -m 0755 package/shairport-sync/S78shairport-sync \
		$(TARGET_DIR)/etc/init.d/S78shairport-sync
endef

$(eval $(autotools-package))
