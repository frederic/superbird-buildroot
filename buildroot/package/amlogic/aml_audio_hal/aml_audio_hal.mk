#############################################################
#
# aml_audio_service
#
#############################################################
AML_AUDIO_HAL_VERSION:=1.0
AML_AUDIO_HAL_SITE=$(TOPDIR)/../multimedia/aml_audio_hal
AML_AUDIO_HAL_SITE_METHOD=local
AML_AUDIO_HAL_DEPENDENCIES=android-tools aml_commonlib aml_amaudioutils libplayer liblog tinyalsa aml_dvb expat
AML_AUDIO_HAL_INSTALL_STAGING = NO
AML_AUDIO_HAL_INSTALL_TARGET = YES

export AML_AUDIO_HAL_BUILD_DIR = $(BUILD_DIR)
export AML_AUDIO_HAL_STAGING_DIR = $(STAGING_DIR)
export AML_AUDIO_HAL_TARGET_DIR = $(TARGET_DIR)
export AML_AUDIO_HAL_BR2_ARCH = $(BR2_ARCH)

$(eval $(cmake-package))
