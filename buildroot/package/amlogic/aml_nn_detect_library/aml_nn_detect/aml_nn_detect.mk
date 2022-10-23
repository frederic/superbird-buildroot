#
# AML_NN_DETECT
#
AML_NN_DETECT_VERSION = 1.0
AML_NN_DETECT_SITE = $(TOPDIR)/../vendor/amlogic/slt/npu_app/detect_library/source_code
AML_NN_DETECT_SITE_METHOD = local
AML_NN_DETECT_INSTALL_STAGING = YES

define AML_NN_DETECT_BUILD_CMDS
    cd $(@D);mkdir -p obj;$(MAKE) CC=$(TARGET_CC)
endef


define AML_NN_DETECT_INSTALL_TARGET_CMDS
    mkdir -p $(TARGET_DIR)/etc/nn_data
    $(INSTALL) -D -m 0644 $(@D)/libnn_detect.so $(TARGET_DIR)/usr/lib/libnn_detect.so
endef

define AML_NN_DETECT_INSTALL_STAGING_CMDS
    $(INSTALL) -D -m 0644 $(@D)/libnn_detect.so $(STAGING_DIR)/usr/lib/libnn_detect.so
    $(INSTALL) -D -m 0644 $(@D)/include/nn_detect.h $(STAGING_DIR)/usr/include/nn_detect.h
    $(INSTALL) -D -m 0644 $(@D)/include/nn_detect_common.h $(STAGING_DIR)/usr/include/nn_detect_common.h
    $(INSTALL) -D -m 0644 $(@D)/include/nn_detect_utils.h $(STAGING_DIR)/usr/include/nn_detect_utils.h
endef

define AML_NN_DETECT_INSTALL_CLEAN_CMDS
    $(MAKE) CC=$(TARGET_CC) -C $(@D) clean
endef

$(eval $(generic-package))


