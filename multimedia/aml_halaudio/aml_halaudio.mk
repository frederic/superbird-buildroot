#############################################################
#
# aml_halaudio
#
#############################################################
AML_HALAUDIO_VERSION:=0.0.1
AML_HALAUDIO_SITE=$(TOPDIR)/../multimedia/aml_halaudio/src
AML_HALAUDIO_SITE_METHOD=local
AML_HALAUDIO_BUILD_DIR = $(BUILD_DIR)
AML_HALAUDIO_INSTALL_STAGING = YES
AML_HALAUDIO_DEPENDENCIES = alsa-lib
AML_HALAUDIO_DEPENDENCIES += cjson

ifeq ($(BR2_PACKAGE_PULSEAUDIO),y)
AML_HALAUDIO_DEPENDENCIES += pulseaudio
export ENABLE_PULSEAUDIO = yes
endif

ifeq ($(BR2_PACKAGE_AUDIOSERVICE_S400),y)
export ENABLE_AUDIOSERVICE_S400 = yes
endif

ifeq ($(BR2_PACKAGE_AUDIOSERVICE_S400_SBR),y)
export ENABLE_AUDIOSERVICE_S400_SBR = yes
endif

ifeq ($(BR2_PACKAGE_AUDIOSERVICE_S410_SBR),y)
export ENABLE_AUDIOSERVICE_S410_SBR = yes
endif

ifeq ($(BR2_PACKAGE_ALSA_PLUGINS),y)
export ENABLE_ALSA_PLUGINS = yes
endif

ifeq ($(BR2_aarch64),y)
export ENABLE_AUDIO_64bit = yes
endif

export AML_HALAUDIO_STAGING_DIR = $(STAGING_DIR)
export AML_HALAUDIO_TARGET_DIR = $(TARGET_DIR)


define AML_HALAUDIO_BUILD_CMDS
	$(MAKE) CC=$(TARGET_CC) CXX=$(TARGET_CXX) -C $(@D)
endef

define AML_HALAUDIO_CLEAN_CMDS
	$(MAKE) -C $(@D) clean
endef

define AML_HALAUDIO_INSTALL_TARGET_CMDS
        $(MAKE) -C $(@D) install
endef

define AML_HALAUDIO_INSTALL_INIT_SYSV
	mkdir -p $(TARGET_DIR)/etc/halaudio
	$(INSTALL) -D -m 644 $(AML_HALAUDIO_SITE)/../config/*.json \
		$(TARGET_DIR)/etc/halaudio/
endef

define AML_HALAUDIO_UNINSTALL_TARGET_CMDS
        $(MAKE) -C $(@D) uninstall
endef

$(eval $(generic-package))
