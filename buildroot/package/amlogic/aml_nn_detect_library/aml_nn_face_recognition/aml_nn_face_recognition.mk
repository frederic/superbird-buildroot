#
# AML_NN_FACE_RECOGNITION
#
AML_NN_FACE_RECOGNITION_VERSION = 1.0
AML_NN_FACE_RECOGNITION_SITE = $(TOPDIR)/../vendor/amlogic/slt/npu_app/detect_library/model_code/aml_face_recognition
AML_NN_FACE_RECOGNITION_SITE_METHOD = local
AML_NN_FACE_RECOGNITION_DEPENDENCIES = opencv3

define AML_NN_FACE_RECOGNITION_BUILD_CMDS
    cd $(@D);
endef

ifeq ($(BR2_aarch64), y)
AML_NN_FACE_RECOGNITION_INSTALL_TARGETS_CMDS = \
    $(INSTALL) -D -m 0644 $(@D)/so/lib64/*  $(TARGET_DIR)/usr/lib/
else
AML_NN_FACE_RECOGNITION_INSTALL_TARGETS_CMDS = \
    $(INSTALL) -D -m 0644 $(@D)/so/lib32/*  $(TARGET_DIR)/usr/lib/
endif

define AML_NN_FACE_RECOGNITION_INSTALL_TARGET_CMDS
    mkdir -p $(TARGET_DIR)/etc/nn_data
    $(AML_NN_FACE_RECOGNITION_INSTALL_TARGETS_CMDS)
	if [ $(BR2_PACKAGE_AML_SOC_FAMILY_NAME) = "G12B" ]; then \
		$(INSTALL) -D -m 0644 $(@D)/nn_data/aml_face_recognition_7d.nb $(TARGET_DIR)/etc/nn_data/; \
		$(INSTALL) -D -m 0644 $(@D)/nn_data/aml_face_recognition_88.nb $(TARGET_DIR)/etc/nn_data/; \
	fi
	if [ $(BR2_PACKAGE_AML_SOC_FAMILY_NAME) = "SM1" ]; then \
		$(INSTALL) -D -m 0644 $(@D)/nn_data/aml_face_recognition_99.nb $(TARGET_DIR)/etc/nn_data/; \
	fi
	if [ $(BR2_PACKAGE_AML_SOC_FAMILY_NAME) = "C1" ]; then \
		$(INSTALL) -D -m 0644 $(@D)/nn_data/aml_face_recognition_a1.nb $(TARGET_DIR)/etc/nn_data/; \
	fi
endef

define AML_NN_FACE_RECOGNITION_INSTALL_STAGING_CMDS
    $(AML_NN_FACE_RECOGNITION_INSTALL_TARGETS_CMDS)
endef

define AML_NN_FACE_RECOGNITION_INSTALL_CLEAN_CMDS
    $(MAKE) CC=$(TARGET_CC) -C $(@D) clean
endef

$(eval $(generic-package))
