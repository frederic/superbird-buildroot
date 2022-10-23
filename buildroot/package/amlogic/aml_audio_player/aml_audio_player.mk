#############################################################
#
# amlogic audio player
#
# It can play audio with URL base on FFMPEG
#
#############################################################
AML_AUDIO_PLAYER_VERSION = 1.0
AML_AUDIO_PLAYER_SITE = $(TOPDIR)/../vendor/amlogic/aml_audio_player
AML_AUDIO_PLAYER_SITE_METHOD = local
AML_AUDIO_PLAYER_DEPENDENCIES = ffmpeg


define AML_AUDIO_PLAYER_BUILD_CMDS
    $(MAKE) CC=$(TARGET_CC) -C $(@D) all
endef

define AML_AUDIO_PLAYER_INSTALL_TARGET_CMDS
    $(MAKE) -C $(@D) install
endef

$(eval $(generic-package))
