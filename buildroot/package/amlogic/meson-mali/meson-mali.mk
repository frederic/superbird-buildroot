#############################################################
#
# meson-mali
#
#############################################################
MESON_MALI_VERSION = 2.0
MESON_MALI_SITE = $(TOPDIR)/../vendor/amlogic/meson_mali
MESON_MALI_SITE_METHOD = local
MESON_MALI_INSTALL_STAGING = YES
MESON_MALI_PROVIDES = libegl libgles
MESON_MALI_LIBS =

EGL_PLATFORM_HEADER =

MESON_EGL_CONF = $(TARGET_DIR)/etc/meson_egl.conf

ifneq ($(BR2_PACKAGE_MESON_MALI_VERSION),"")
MESON_MALI_VERSION = $(call qstrip,$(BR2_PACKAGE_MESON_MALI_VERSION))
else
MESON_MALI_VERSION = $(call qstrip,$(BR2_PACKAGE_GPU_VERSION))
endif

API_VERSION = $(call qstrip,$(MESON_MALI_VERSION))
ifneq ($(BR2_PACKAGE_MESON_MALI_MODEL),"")
MALI_VERSION = $(call qstrip,$(BR2_PACKAGE_MESON_MALI_MODEL))
else
MALI_VERSION = $(call qstrip,$(BR2_PACKAGE_OPENGL_MALI_VERSION))
endif

ifeq ($(BR2_PACKAGE_MESON_MALI_WAYLAND_FBDEV_EGL),y)
EGL_PLATFORM_HEADER = EGL_platform/platform_wayland
MALI_LIB_LOC = $(MALI_VERSION)/$(API_VERSION)/wayland/fbdev
MESON_MALI_DEPENDENCIES += wayland
else ifeq ($(BR2_PACKAGE_MESON_MALI_WAYLAND_DRM_EGL),y)
EGL_PLATFORM_HEADER = EGL_platform/platform_wayland
MALI_LIB_LOC = $(MALI_VERSION)/$(API_VERSION)/wayland/drm
MESON_MALI_DEPENDENCIES += wayland
else ifeq ($(BR2_PACKAGE_MESON_MALI_DUMMY_EGL),y)
EGL_PLATFORM_HEADER = EGL_platform/platform_dummy
MALI_LIB_LOC = $(MALI_VERSION)/$(API_VERSION)/dummy
else
EGL_PLATFORM_HEADER = EGL_platform/platform_fbdev
MALI_LIB_LOC = $(MALI_VERSION)/$(API_VERSION)/fbdev
endif

ifeq ($(BR2_aarch64),y)
MALI_LIB_DIR = arm64/$(MALI_LIB_LOC)
else ifeq ($(BR2_ARM_EABIHF),y)
MALI_LIB_DIR = eabihf/$(MALI_LIB_LOC)
else
MALI_LIB_DIR = $(MALI_LIB_LOC)
endif

MESON_MALI_LIBS = libEGL*,libGLE*,libwayland-egl.so

define BASE_INSTALL_STAGING
	cp -arf $(MESON_MALI_DIR)/include/EGL $(STAGING_DIR)/usr/include/
	cp -arf $(MESON_MALI_DIR)/include/GLES $(STAGING_DIR)/usr/include/
	cp -arf $(MESON_MALI_DIR)/include/GLES2 $(STAGING_DIR)/usr/include/
	cp -arf $(MESON_MALI_DIR)/include/GLES3 $(STAGING_DIR)/usr/include/
	cp -arf $(MESON_MALI_DIR)/include/KHR $(STAGING_DIR)/usr/include/
	cp -arf $(MESON_MALI_DIR)/include/$(EGL_PLATFORM_HEADER)/*.h $(STAGING_DIR)/usr/include/EGL/
	cp -arf $(MESON_MALI_DIR)/lib/{$(MESON_MALI_LIBS)} $(STAGING_DIR)/usr/lib/
	cp -arf $(MESON_MALI_DIR)/lib/$(MALI_LIB_DIR)/*.so* $(STAGING_DIR)/usr/lib/
	mkdir -p $(STAGING_DIR)/usr/lib/pkgconfig/
	cp -arf $(MESON_MALI_DIR)/lib/pkgconfig/*.pc $(STAGING_DIR)/usr/lib/pkgconfig/
endef

ifeq ($(BR2_PACKAGE_MESON_MALI_WAYLAND_DRM_EGL),y)
define WAYLAND_DRM_INSTALL_STAGING
	cp -arf $(MESON_MALI_DIR)/include/$(EGL_PLATFORM_HEADER)/gbm/gbm.h $(STAGING_DIR)/usr/include/
	cp -arf $(MESON_MALI_DIR)/lib/libgbm.so $(STAGING_DIR)/usr/lib/
	cp -arf $(MESON_MALI_DIR)/lib/pkgconfig/gbm/*.pc $(STAGING_DIR)/usr/lib/pkgconfig/
endef
endif

define MESON_MALI_INSTALL_STAGING_CMDS
	$(BASE_INSTALL_STAGING)
	$(WAYLAND_DRM_INSTALL_STAGING)
endef


define BASE_INSTALL_TARGET
	cp -df $(MESON_MALI_DIR)/lib/{$(MESON_MALI_LIBS)} $(TARGET_DIR)/usr/lib
	install -m 755 $(MESON_MALI_DIR)/lib/$(MALI_LIB_DIR)/*.so* $(TARGET_DIR)/usr/lib
	mkdir -p $(TARGET_DIR)/usr/lib/pkgconfig
	install -m 644 $(MESON_MALI_DIR)/lib/pkgconfig/*.pc $(TARGET_DIR)/usr/lib/pkgconfig
	touch $(TARGET_DIR)/etc/meson_egl.conf
endef

ifeq ($(BR2_PACKAGE_MESON_MALI_WAYLAND_DRM_EGL),y)
define WAYLAND_DRM_INSTALL_TARGET
	cp -df $(MESON_MALI_DIR)/lib/libgbm.so $(TARGET_DIR)/usr/lib
	install -m 644 $(MESON_MALI_DIR)/lib/pkgconfig/gbm/*.pc $(TARGET_DIR)/usr/lib/pkgconfig
	echo wayland_drm > $(MESON_EGL_CONF)
endef
endif

ifeq ($(BR2_PACKAGE_MESON_MALI_DUMMY_EGL),y)
define DUMMY_INSTALL_TARGET
	echo dummy > $(MESON_EGL_CONF)
endef
endif

ifeq ($(BR2_PACKAGE_MESON_MALI_WAYLAND_FBDEV_EGL),y)
define WAYLAND_FBDEV_INSTALL_TARGET
        echo wayland_fbdev > $(MESON_EGL_CONF)
endef
endif

ifeq ($(BR2_PACKAGE_MESON_MALI_FBDEV_EGL),y)
define FBDEV_INSTALL_TARGET
        echo fbdev > $(MESON_EGL_CONF)
endef
endif

define MESON_MALI_INSTALL_TARGET_CMDS
	$(BASE_INSTALL_TARGET)
	$(WAYLAND_DRM_INSTALL_TARGET)
        $(WAYLAND_FBDEV_INSTALL_TARGET)
        $(DUMMY_INSTALL_TARGET)
        $(FBDEV_INSTALL_TARGET)
endef


$(eval $(generic-package))


