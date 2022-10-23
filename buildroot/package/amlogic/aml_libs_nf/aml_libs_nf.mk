#############################################################
#
# aml_libs_nf
#
#############################################################
AML_LIBS_NF_VERSION:=0.4.0
AML_LIBS_NF_SITE=$(TOPDIR)/../multimedia/aml_libs_nf/src
AML_LIBS_NF_SITE_METHOD=local
AML_LIBS_NF_DEPENDENCIES=zlib alsa-lib
export AML_LIBS_NF_BUILD_DIR = $(BUILD_DIR)
export AML_LIBS_NF_STAGING_DIR = $(STAGING_DIR)
export AML_LIBS_NF_TARGET_DIR = $(TARGET_DIR)
export AML_LIBS_NF_BR2_ARCH = $(BR2_ARCH)

define AML_LIBS_NF_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) CC=$(TARGET_CC) -C $(@D) all
	$(TARGET_MAKE_ENV) $(MAKE) CC=$(TARGET_CC) -C $(@D) install
endef

$(eval $(generic-package))
