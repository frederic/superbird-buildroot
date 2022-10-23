#############################################################
#
# amlogic speaker process tools for avs
#
#############################################################
AML_SPEAKER_PROCESS_VERSION = 20191114
AML_SPEAKER_PROCESS_SITE = $(AML_SPEAKER_PROCESS_PKGDIR)/src
AML_SPEAKER_PROCESS_SITE_METHOD = local
AML_SPEAKER_PROCESS_DEPENDENCIES = avs-sdk

ifeq ($(BR2_AVS_AMAZON_WWE),y)
AML_SPEAKER_PROCESS_CFLAGS = \
     "-I$(@D)/../avs-sdk/KWD/DSP/include/common/ \
     -I$(@D)/../avs-sdk/KWD/DSP/include/EXT_WWE_DSP/ \
     -I$(STAGING_DIR)/usr/include/ \
     -DEXT_WWE_DSP_KEY_WORD_DETECTOR=ON"
     AML_SPEAKER_PROCESS_LDFLAGS = "-L$(@D)/../avs-sdk/KWD/DSP/lib/EXT_WWE_DSP/ -lAWELib"

#define AML_SPEAKER_PROCESS_INSTALL_TARGET_CMDS
#$(INSTALL) -m 0755 -D $(@D)/speaker_process  $(TARGET_DIR)/usr/bin
#$(INSTALL) -m 0755 -D $(@D)/EXT_WWE_DSP/speaker_processing.awb  $(TARGET_DIR)/usr/bin
#endef

endif

define AML_SPEAKER_PROCESS_BUILD_CMDS
    $(MAKE) CC=$(TARGET_CXX) CFLAGS=$(AML_SPEAKER_PROCESS_CFLAGS) LDFLAGS=$(AML_SPEAKER_PROCESS_LDFLAGS) -C $(@D) all
endef

define AML_SPEAKER_PROCESS_INSTALL_TARGET_CMDS
    $(MAKE) -C $(@D) install
endef

$(eval $(generic-package))
