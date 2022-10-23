#
# AML_NN_DETECT_YOLOFACE
#
AML_NN_DETECT_YOLOFACE_VERSION = 1.0
AML_NN_DETECT_YOLOFACE_SITE = $(TOPDIR)/../vendor/amlogic/slt/npu_app/detect_library/model_code/detect_yoloface
AML_NN_DETECT_YOLOFACE_SITE_METHOD = local
AML_NN_DETECT_YOLOFACE_INSTALL_STAGING = YES
AML_NN_DETECT_YOLOFACE_DEPENDENCIES = npu
AML_NN_DETECT_YOLOFACE_DEPENDENCIES += aml_nn_detect

define AML_NN_DETECT_YOLOFACE_BUILD_CMDS
    cd $(@D);mkdir -p obj;$(MAKE) CC=$(TARGET_CC)
endef


define AML_NN_DETECT_YOLOFACE_INSTALL_TARGET_CMDS
    mkdir -p $(TARGET_DIR)/etc/nn_data
    $(INSTALL) -D -m 0644 $(@D)/libnn_yoloface.so  $(TARGET_DIR)/usr/lib/
	if [ $(BR2_PACKAGE_AML_SOC_FAMILY_NAME) = "G12B" ]; then \
		$(INSTALL) -D -m 0644 $(@D)/nn_data/yolo_face_7d.nb $(TARGET_DIR)/etc/nn_data/; \
		$(INSTALL) -D -m 0644 $(@D)/nn_data/yolo_face_88.nb $(TARGET_DIR)/etc/nn_data/; \
	fi
	if [ $(BR2_PACKAGE_AML_SOC_FAMILY_NAME) = "SM1" ]; then \
		$(INSTALL) -D -m 0644 $(@D)/nn_data/yolo_face_99.nb $(TARGET_DIR)/etc/nn_data/; \
	fi
	if [ $(BR2_PACKAGE_AML_SOC_FAMILY_NAME) = "C1" ]; then \
		$(INSTALL) -D -m 0644 $(@D)/nn_data/yolo_face_a1.nb $(TARGET_DIR)/etc/nn_data/; \
	fi
endef

define AML_NN_DETECT_YOLOFACE_INSTALL_STAGING_CMDS
    $(INSTALL) -D -m 0644 $(@D)/libnn_yoloface.so  $(TARGET_DIR)/usr/lib/
endef

define AML_NN_DETECT_YOLOFACE_INSTALL_CLEAN_CMDS
    $(MAKE) CC=$(TARGET_CC) -C $(@D) clean
endef

$(eval $(generic-package))
