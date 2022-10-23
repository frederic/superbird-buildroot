#############################################################
#
# Chromium
#
#############################################################
ifeq ($(BR2_PACKAGE_CHROMIUM_PREBUILT),y)
include package/amlogic/chromium/chromium-prebuilt/chromium-prebuilt.mk
endif


ifeq ($(BR2_PACKAGE_CHROMIUM_COMPILE_ALL),y)

CHROMIUM_VERSION = 69.0.3497.81

#CHROMIUM_LICENSE = GPLv3+
#CHROMIUM_LICENSE_FILES = COPYING
CHROMIUM_DEPENDENCIES = libxkbcommon gconf libexif cups libnss libdrm pciutils pulseaudio krb5 pango libplayer browser_toolchain_depot_tools

CHROMIUM_SOURCE = chromium-$(CHROMIUM_VERSION).tar.xz
CHROMIUM_SITE = file://$(TOPDIR)/../vendor/amlogic/external/packages

ifeq ($(BR2_PACKAGE_CHROMIUM_ENABLE_WIDEVINE), y)
CHROMIUM_ENABLE_WIDEVINE=true
else
CHROMIUM_ENABLE_WIDEVINE=false
endif

CHROMIUM_FLAGS = v8_use_external_startup_data=false enable_nacl=false use_kerberos=false use_cups=false use_gnome_keyring=false v8_use_snapshot=true use_system_zlib=true is_clang=false use_sysroot=true use_xkbcommon=true ffmpeg_branding="Chrome" media_use_libvpx=false proprietary_codecs=true enable_hevc_demuxing=true enable_ac3_eac3_audio_demuxing=true remove_webcore_debug_symbols=true target_os="linux" target_sysroot="$(STAGING_DIR)" symbol_level=1 remove_webcore_debug_symbols=true is_debug=false enable_linux_installer=false enable_widevine=$(CHROMIUM_ENABLE_WIDEVINE) use_ozone=true ozone_auto_platforms=false ozone_platform="wayland" ozone_platform_wayland=true ozone_platform_x11=false use_system_libdrm=true use_system_libwayland=true treat_warnings_as_errors=false

ifeq ($(BR2_aarch64), y)
CHROMIUM_DEPENDENCIES += browser_toolchain_gcc-linaro-aarch64
CHROMIUM_TOOLCHAIN_DIR = $(BROWSER_TOOLCHAIN_GCC_LINARO_AARCH64_INSTALL_DIR)/bin
CHROMIUM_FLAGS += target_cpu="arm64"

else
CHROMIUM_DEPENDENCIES += browser_toolchain_gcc-linaro-armeabihf
CHROMIUM_TOOLCHAIN_DIR = $(BROWSER_TOOLCHAIN_GCC_LINARO_ARMEABIHF_INSTALL_DIR)/bin
CHROMIUM_FLAGS += target_cpu="arm"

endif

ifeq ($(BR2_PACKAGE_CHROMIUM_ENABLE_CCACHE), y)
ifneq ($(BR2_PACKAGE_CHROMIUM_CCACHE_BIN_PATH), "")
CHROMIUM_FLAGS += cc_wrapper=$(BR2_PACKAGE_CHROMIUM_CCACHE_BIN_PATH)
endif
endif


CHROMIUM_DEPOT_TOOL_DIR = $(BROWSER_DEPOT_TOOL_PATH)
CHROMIUM_OUT_DIR = $(CHROMIUM_DIR)/src/out/chrome

define CHROMIUM_BUILD_CMDS
	export PATH=$(CHROMIUM_TOOLCHAIN_DIR):$(CHROMIUM_DEPOT_TOOL_DIR):$(PATH); \
	cd $(CHROMIUM_DIR)/src && \
	gn gen $(CHROMIUM_OUT_DIR) --args='$(CHROMIUM_FLAGS)' && \
	ninja -C $(CHROMIUM_OUT_DIR) chrome
endef

define CHROMIUM_INSTALL_STAGING_CMDS
endef

CHROMIUM_INSTALL_DIR=$(TARGET_DIR)/usr/bin/chromium-browser
define CHROMIUM_INSTALL_TARGET_CMDS
	mkdir -p $(CHROMIUM_INSTALL_DIR)
	cp -a $(CHROMIUM_OUT_DIR)/chrome            $(CHROMIUM_INSTALL_DIR)
	cp -a $(CHROMIUM_OUT_DIR)/*.pak             $(CHROMIUM_INSTALL_DIR)
	cp -a $(CHROMIUM_OUT_DIR)/*.so              $(CHROMIUM_INSTALL_DIR)
	cp -a $(CHROMIUM_OUT_DIR)/resources         $(CHROMIUM_INSTALL_DIR)
	cp -a $(CHROMIUM_OUT_DIR)/locales           $(CHROMIUM_INSTALL_DIR)
	cp -a $(CHROMIUM_OUT_DIR)/icudtl.dat        $(CHROMIUM_INSTALL_DIR)
endef

ifeq ($(BR2_PACKAGE_LAUNCHER_USE_CHROME), y)
define CHROMIUM_INSTALL_INIT_SYSV
        rm -rf $(TARGET_DIR)/etc/init.d/S90*
	$(INSTALL) -D -m 755 $(CHROMIUM_PKGDIR)/S90chrome \
		$(TARGET_DIR)/etc/init.d/S90chrome
	$(INSTALL) -D -m 755 $(CHROMIUM_PKGDIR)/amlogic.html \
		$(TARGET_DIR)/var/www/amlogic.html
endef
endif

$(eval $(generic-package))

endif
