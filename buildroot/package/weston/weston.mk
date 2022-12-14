################################################################################
#
# weston
#
################################################################################

ifneq ($(BR2_PACKAGE_WESTON_VERSION),"")
  WESTON_VERSION = $(call qstrip,$(BR2_PACKAGE_WESTON_VERSION))
else
  WESTON_VERSION = 6.0.1
endif
WESTON_SITE = http://wayland.freedesktop.org/releases
WESTON_SOURCE = weston-$(WESTON_VERSION).tar.xz
WESTON_LICENSE = MIT
WESTON_LICENSE_FILES = COPYING

WESTON_DEPENDENCIES = host-pkgconf wayland wayland-protocols \
	libxkbcommon pixman libpng jpeg mtdev udev cairo libinput libdrm \
	$(if $(BR2_PACKAGE_WEBP),webp)

WESTON_CONF_OPTS = \
	--with-dtddir=$(STAGING_DIR)/usr/share/wayland \
	--disable-headless-compositor \
	--disable-colord \
	--disable-devdocs \
	--disable-setuid-install \
	--enable-autotools

WESTON_MAKE_OPTS = \
	WAYLAND_PROTOCOLS_DATADIR=$(STAGING_DIR)/usr/share/wayland-protocols

# Uses VIDIOC_EXPBUF, only available from 3.8+
ifeq ($(BR2_TOOLCHAIN_HEADERS_AT_LEAST_3_8),)
WESTON_CONF_OPTS += --disable-simple-dmabuf-v4l-client
endif

ifeq ($(BR2_PACKAGE_DBUS),y)
WESTON_CONF_OPTS += --enable-dbus
WESTON_DEPENDENCIES += dbus
else
WESTON_CONF_OPTS += --disable-dbus
endif

# weston-launch must be u+s root in order to work properly
ifeq ($(BR2_PACKAGE_LINUX_PAM),y)
define WESTON_PERMISSIONS
	/usr/bin/weston-launch f 4755 0 0 - - - - -
endef
define WESTON_USERS
	- - weston-launch -1 - - - - Weston launcher group
endef
WESTON_CONF_OPTS += --enable-weston-launch
WESTON_DEPENDENCIES += linux-pam
else
WESTON_CONF_OPTS += --disable-weston-launch
endif

# Needs wayland-egl, which normally only mesa provides or package opengl.
ifeq ($(BR2_PACKAGE_MESA3D_OPENGL_EGL)$(BR2_PACKAGE_MESA3D_OPENGL_ES),yy)
WESTON_CONF_OPTS += --enable-egl
WESTON_DEPENDENCIES += libegl
else ifeq ($(BR2_PACKAGE_MESON_MALI),y)
WESTON_CONF_OPTS += --enable-egl --enable-simple-egl-clients --disable-simple-dmabuf-egl-client
WESTON_DEPENDENCIES += libegl
else
WESTON_CONF_OPTS += \
	--disable-egl \
	--disable-simple-dmabuf-drm-client \
	--disable-simple-egl-clients
endif

#We need to keep some Weston 3.0.0. configuration.
ifeq ($(WESTON_VERSION), 3.0.0)
ifeq ($(BR2_PACKAGE_CAIRO)$(BR2_PACKAGE_HAS_LIBGLES),yy)
  WESTON_CONF_OPTS += --with-cairo=glesv2
endif

ifeq ($(BR2_PACKAGE_LIBUNWIND),y)
  WESTON_DEPENDENCIES += libunwind
else
  WESTON_CONF_OPTS += --disable-libunwind
endif
endif

ifeq ($(BR2_PACKAGE_WESTON_RDP),y)
WESTON_DEPENDENCIES += freerdp
WESTON_CONF_OPTS += --enable-rdp-compositor
else
WESTON_CONF_OPTS += --disable-rdp-compositor
endif

ifeq ($(BR2_PACKAGE_WESTON_FBDEV),y)
WESTON_CONF_OPTS += \
	--enable-fbdev-compositor \
	WESTON_NATIVE_BACKEND=fbdev-backend.so
else
WESTON_CONF_OPTS += --disable-fbdev-compositor
endif

ifeq ($(BR2_PACKAGE_WESTON_DRM),y)
WESTON_CONF_OPTS += \
	--enable-drm-compositor \
	WESTON_NATIVE_BACKEND=drm-backend.so
WESTON_DEPENDENCIES += libdrm
else
WESTON_CONF_OPTS += --disable-drm-compositor
endif

ifeq ($(BR2_PACKAGE_WESTON_DRM_FIX_UI_SIZE),y)
WESTON_CONF_OPTS += --enable-drm-fix-ui-size
endif

ifeq ($(BR2_PACKAGE_WESTON_X11),y)
WESTON_CONF_OPTS += \
	--enable-x11-compositor \
	WESTON_NATIVE_BACKEND=x11-backend.so
WESTON_DEPENDENCIES += libxcb xlib_libX11
else
WESTON_CONF_OPTS += --disable-x11-compositor
endif

ifeq ($(BR2_PACKAGE_WESTON_XWAYLAND),y)
WESTON_CONF_OPTS += --enable-xwayland
WESTON_DEPENDENCIES += cairo libepoxy libxcb xlib_libX11 xlib_libXcursor
else
WESTON_CONF_OPTS += --disable-xwayland
endif

ifeq ($(BR2_PACKAGE_LIBVA),y)
WESTON_CONF_OPTS += --enable-vaapi-recorder
WESTON_DEPENDENCIES += libva
else
WESTON_CONF_OPTS += --disable-vaapi-recorder
endif

ifeq ($(BR2_PACKAGE_LCMS2),y)
WESTON_CONF_OPTS += --enable-lcms
WESTON_DEPENDENCIES += lcms2
else
WESTON_CONF_OPTS += --disable-lcms
endif

ifeq ($(BR2_PACKAGE_SYSTEMD),y)
WESTON_CONF_OPTS += --enable-systemd-login --enable-systemd-notify
WESTON_DEPENDENCIES += systemd
else
WESTON_CONF_OPTS += --disable-systemd-login --disable-systemd-notify
endif

ifeq ($(BR2_PACKAGE_LIBXML2),y)
WESTON_CONF_OPTS += --enable-junit-xml
WESTON_DEPENDENCIES += libxml2
else
WESTON_CONF_OPTS += --disable-junit-xml
endif

ifeq ($(BR2_PACKAGE_WESTON_DEMO_CLIENTS),y)
WESTON_CONF_OPTS += --enable-demo-clients-install
else
WESTON_CONF_OPTS += --disable-demo-clients-install
endif

ifeq ($(BR2_PACKAGE_WESTON_DRM_HELPER),y)
WESTON_CONF_OPTS += \
	--enable-drm-helper
WESTON_DEPENDENCIES += meson-display
endif

WESTON_CONF_OPTS +=  WESTON_MAX_OUTPUT_PIPLINE=$(BR2_PACKAGE_WESTON_MAX_OUTPUT_PIPLINE)

ifeq ($(BR2_PACKAGE_WESTON_OUTPUT_DYNAMIC_SWITCH),y)
WESTON_CONF_OPTS += --enable-output-dynamic-switch
endif

$(eval $(autotools-package))


include $(sort $(wildcard package/weston/*/*.mk))
