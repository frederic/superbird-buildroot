################################################################################
#
# audioservice
#
################################################################################

AUDIOSERVICE_VERSION = 1.0.0
AUDIOSERVICE_SITE = $(TOPDIR)/../vendor/amlogic/audioservice
AUDIOSERVICE_SITE_METHOD = local
#AUDIOSERVICE_LICENSE = GPLv2+, GPLv2 (py-smbus)
#AUDIOSERVICE_LICENSE_FILES = COPYING
AUDIOSERVICE_INSTALL_STAGING = YES
AUDIOSERVICE_AUTORECONF = YES

AUDIOSERVICE_DEPENDENCIES = libglib2 dbus cjson aml_halaudio

AUDIOSERVICE_CONF_OPTS += --enable-halaudio

ifeq ($(BR2_PACKAGE_AUDIOSERVICE_ASR),y)
AUDIOSERVICE_CONF_OPTS += --enable-asr
endif

ifeq ($(BR2_PACKAGE_AUDIOSERVICE_PULSEAUDIO),y)
AUDIOSERVICE_CONF_OPTS += --enable-pulseaudio
AUDIOSERVICE_DEPENDENCIES += pulseaudio
endif

ifeq ($(BR2_PACKAGE_AUDIOSERVICE_FFMPEG),y)
AUDIOSERVICE_CONF_OPTS += --enable-ffmpeg
AUDIOSERVICE_DEPENDENCIES += ffmpeg
endif

ifeq ($(BR2_PACKAGE_AUDIOSERVICE_PYTHON),y)
AUDIOSERVICE_CONF_OPTS += --enable-python
AUDIOSERVICE_DEPENDENCIES += python
endif

ifeq ($(BR2_PACKAGE_AUDIOSERVICE_EXTERNAL_M6350),y)
AUDIOSERVICE_CONF_OPTS += --enable-external_m6350
EXTERNAL_M6350_LIB = libasexternal_m6350.so
endif

ifeq ($(BR2_PACKAGE_AUDIOSERVICE_EXTERNAL_962E),y)
AUDIOSERVICE_CONF_OPTS += --enable-external_962e
EXTERNAL_962E_LIB = libasexternal_962e.so
endif

ifeq ($(BR2_PACKAGE_AUDIOSERVICE_STRESSTEST),y)
AUDIOSERVICE_CONF_OPTS += --enable-stresstest
endif

ifeq ($(BR2_PACKAGE_AUDIOSERVICE_S400_SBR),y)
AUDIOSERVICE_CONF_OPTS += --enable-s400sbr
endif

ifeq ($(BR2_PACKAGE_AUDIOSERVICE_S410_SBR),y)
AUDIOSERVICE_CONF_OPTS += --enable-s410sbr
endif

ifeq ($(BR2_PACKAGE_AUDIOSERVICE_USBPLAYER),y)
AUDIOSERVICE_CONF_OPTS += --enable-usbplayer
AUDIOSERVICE_DEPENDENCIES += ffmpeg aml_audio_player
endif

ifeq ($(BR2_PACKAGE_AUDIOSERVICE_AIRPLAY),y)
AUDIOSERVICE_CONF_OPTS += --enable-airplay
AUDIOSERVICE_DEPENDENCIES += airplay2
endif

ifeq ($(BR2_PACKAGE_AUDIOSERVICE_AMLUART),y)
AUDIOSERVICE_CONF_OPTS += --enable-amluart
endif

ifeq ($(BR2_AVS_CLIENT_API),y)
AUDIOSERVICE_CONF_OPTS += --enable-avs
AUDIOSERVICE_DEPENDENCIES += avs-sdk
endif

define AUDIOSERVICE_LIB_INSTALL_CMD

endef


# AUDIOSERVICe_CONF_OPTS = --prefix=$(TARGET_DIR)/usr
AUDIOSERVICE_EXTERNAL_INPUT_LIB = libasexternal_input.so

define AUDIOSERVICE_INSTALL_INIT_SYSV
	$(INSTALL) -D -m 755 $(AUDIOSERVICE_SITE)/script/S90audioservice \
		$(TARGET_DIR)/etc/init.d/S90audioservice
	$(INSTALL) -D -m 644 \
		$(AUDIOSERVICE_SITE)/src/config/$(BR2_PACKAGE_AUDIOSERVICE_CONFIG_FILE) \
		$(TARGET_DIR)/etc/default_audioservice.conf
if [ "$(BR2_PACKAGE_AUDIOSERVICE_EXTERNAL_M6350)" == "y" ]; then \
	$(INSTALL) -d $(TARGET_DIR)/etc/mcu6350_bin/; \
	$(INSTALL) -D -m 644 \
		$(AUDIOSERVICE_SITE)/src/external/mcu6350_bin/* $(TARGET_DIR)/etc/mcu6350_bin/; \
	cd $(TARGET_DIR)/usr/lib; \
	ln -fs $(EXTERNAL_M6350_LIB) $(AUDIOSERVICE_EXTERNAL_INPUT_LIB); \
	cd -; \
fi
if [ "$(BR2_PACKAGE_AUDIOSERVICE_EXTERNAL_962E)" == "y" ]; then \
	cd $(TARGET_DIR)/usr/lib; \
	ln -fs $(EXTERNAL_962E_LIB) $(AUDIOSERVICE_EXTERNAL_INPUT_LIB); \
	cd -; \
fi
endef

# Autoreconf requires an m4 directory to exist
define AUDIOSERVICE_PATCH_M4
	mkdir -p $(@D)/m4
endef

AUDIOSERVICE_POST_PATCH_HOOKS += AUDIOSERVICE_PATCH_M4



$(eval $(autotools-package))
