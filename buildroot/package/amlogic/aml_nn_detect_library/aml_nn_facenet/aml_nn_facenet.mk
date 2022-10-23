#
# AML_NN_FACENET
#
AML_NN_FACENET_VERSION = 1.0
AML_NN_FACENET_SITE = $(TOPDIR)/../vendor/amlogic/slt/npu_app/detect_library/model_code/facenet
AML_NN_FACENET_SITE_METHOD = local
AML_NN_FACENET_INSTALL_STAGING = YES
AML_NN_FACENET_DEPENDENCIES = npu
AML_NN_FACENET_DEPENDENCIES += aml_nn_detect

define AML_NN_FACENET_BUILD_CMDS
    cd $(@D);mkdir -p obj;$(MAKE) CC=$(TARGET_CC)
endef


define AML_NN_FACENET_INSTALL_TARGET_CMDS
    mkdir -p $(TARGET_DIR)/etc/nn_data
    $(INSTALL) -D -m 0644 $(@D)/libnn_facenet.so  $(TARGET_DIR)/usr/lib/
	if [ $(BR2_PACKAGE_AML_SOC_FAMILY_NAME) = "G12B" ]; then \
		$(INSTALL) -D -m 0644 $(@D)/nn_data/faceNet_7d.nb $(TARGET_DIR)/etc/nn_data/; \
		$(INSTALL) -D -m 0644 $(@D)/nn_data/faceNet_88.nb $(TARGET_DIR)/etc/nn_data/; \
	fi
	if [ $(BR2_PACKAGE_AML_SOC_FAMILY_NAME) = "SM1" ]; then \
		$(INSTALL) -D -m 0644 $(@D)/nn_data/faceNet_99.nb $(TARGET_DIR)/etc/nn_data/; \
	fi
	if [ $(BR2_PACKAGE_AML_SOC_FAMILY_NAME) = "C1" ]; then \
		$(INSTALL) -D -m 0644 $(@D)/nn_data/faceNet_a1.nb $(TARGET_DIR)/etc/nn_data/; \
	fi
endef

define AML_NN_FACENET_INSTALL_STAGING_CMDS
    $(INSTALL) -D -m 0644 $(@D)/libnn_facenet.so  $(TARGET_DIR)/usr/lib/
endef

define AML_NN_FACENET_INSTALL_CLEAN_CMDS
    $(MAKE) CC=$(TARGET_CC) -C $(@D) clean
endef

$(eval $(generic-package))
