#
# aml_soundai
#
AML_SOUNDAI_VERSION = 1.0
AML_SOUNDAI_SITE = $(TOPDIR)/../vendor/amlogic/client/soundai/src
AML_SOUNDAI_SITE_METHOD = local

AML_SOUNDAI_DEPENDENCIES = vlc libidn

define AML_SOUNDAI_INSTALL_TARGET_CMDS

	$(INSTALL) -D -m 0755 $(@D)/script/S93saisound  $(TARGET_DIR)/etc/init.d/
	$(INSTALL) -D -m 0755 $(@D)/aml_soundai $(TARGET_DIR)/usr/bin/aml_soundai

	cp -rf $(@D)/thirdparty/sound_ai/lib/*.so* $(TARGET_DIR)/usr/lib/
	#cp -rf $(@D)/thirdparty/sound_ai/libvlc/* $(TARGET_DIR)/usr/lib/

	cp -rf $(@D)/thirdparty/sound_ai/sai_config $(TARGET_DIR)/etc/
endef

$(eval $(cmake-package))

