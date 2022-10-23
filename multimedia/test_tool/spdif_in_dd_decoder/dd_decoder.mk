#############################################################
#
# pdif_in_dd_decoder
#
#############################################################
SPDIF_IN_DD_DECODER_VERSION:=20170901
SPDIF_IN_DD_DECODER_SITE=$(TOPDIR)/../multimedia/test_tool/spdif_in_dd_decoder/src
SPDIF_IN_DD_DECODER_SITE_METHOD=local

define SPDIF_IN_DD_DECODER_BUILD_CMDS
        $(MAKE) CC=$(TARGET_CC) CXX=$(TARGET_CXX) -C $(@D)
endef

define SPDIF_IN_DD_DECODER_CLEAN_CMDS
        $(MAKE) -C $(@D) clean
endef

define SPDIF_IN_DD_DECODER_INSTALL_TARGET_CMDS
        $(MAKE) -C $(@D) install
endef

define SPDIF_IN_DD_DECODER_UNINSTALL_TARGET_CMDS
        $(MAKE) -C $(@D) uninstall
endef

$(eval $(generic-package))
