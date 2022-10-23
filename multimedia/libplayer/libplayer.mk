#############################################################
#
# libplayer
#
#############################################################
LIBPLAYER_VERSION:=2.1.0
LIBPLAYER_SITE=$(TOPDIR)/../multimedia/libplayer/src
LIBPLAYER_SITE_METHOD=local
LIBPLAYER_BUILD_DIR = $(BUILD_DIR)
LIBPLAYER_INSTALL_STAGING = YES
LIBPLAYER_DEPENDENCIES = zlib alsa-lib libcurl $(if $(BR2_PACKAGE_LIBXML2),libxml2)

ifeq ($(BR2_PACKAGE_MESON_MALI_WAYLAND_DRM_EGL),y)
LIBPLAYER_DEPENDENCIES += meson-display
endif

export LIBPLAYER_STAGING_DIR = $(STAGING_DIR)
export LIBPLAYER_TARGET_DIR = $(TARGET_DIR)

AMFFMPEG_CONF_OPTS = \
		--disable-yasm \
		--enable-debug \
		--disable-ffplay \
		--disable-ffmpeg \
		--enable-cross-compile \
		--target-os=linux \
		--disable-librtmp \
		--disable-static \
		--enable-shared \
		--disable-ffserver \
		--disable-doc

ifneq ($(call qstrip,$(BR2_GCC_TARGET_CPU)),)
AMFFMPEG_CONF_OPTS += --cpu=$(BR2_GCC_TARGET_CPU)
else ifneq ($(call qstrip,$(BR2_GCC_TARGET_ARCH)),)
AMFFMPEG_CONF_OPTS += --cpu=$(BR2_GCC_TARGET_ARCH)
endif

define LIBPLAYER_CONFIGURE_CMDS
		cd $(@D)/amffmpeg; \
		$(TARGET_CONFIGURE_OPTS) \
		./configure \
		--prefix=$(STAGING_DIR)/usr \
		--shlibdir=$(STAGING_DIR)/usr/lib/libplayer \
		--cross-prefix=$(TARGET_CROSS) \
		--arch=$(BR2_ARCH) \
		--extra-ldflags='-L$(STAGING_DIR)/usr/lib/ -L$(STAGING_DIR)/usr/lib/libplayer -lamavutils -ldl' \
		$(AMFFMPEG_CONF_OPTS) \

endef

define LIBPLAYER_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) ARCH=$(BR2_ARCH) $(MAKE) -C $(@D) all
	$(TARGET_CONFIGURE_OPTS) ARCH=$(BR2_ARCH) $(MAKE) -C $(@D) install
endef

define LIBPLAYER_CLEAN_CMDS
	$(MAKE) -C $(@D) clean
endef

$(eval $(generic-package))
