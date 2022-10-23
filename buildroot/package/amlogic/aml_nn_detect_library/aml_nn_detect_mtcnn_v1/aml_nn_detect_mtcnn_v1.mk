#
# AML_NN_DETECT_MTCNN_V1
#
AML_NN_DETECT_MTCNN_V1_VERSION = 1.0
AML_NN_DETECT_MTCNN_V1_SITE = $(TOPDIR)/../vendor/amlogic/slt/npu_app/detect_library/model_code/detect_mtcnn
AML_NN_DETECT_MTCNN_V1_SITE_METHOD = local
AML_NN_DETECT_MTCNN_V1_INSTALL_STAGING = YES


define AML_NN_DETECT_MTCNN_V1_BUILD_CMDS
    cd $(@D);
endef

ifeq ($(BR2_aarch64), y)
AML_NN_DETECT_MTCNN_V1_INSTALL_TARGETS_CMDS = \
    $(INSTALL) -D -m 0644 $(@D)/so/lib64/*  $(TARGET_DIR)/usr/lib/
	CONFIG_PATH=$(@D)/config/64bit
else
AML_NN_DETECT_MTCNN_V1_INSTALL_TARGETS_CMDS = \
    $(INSTALL) -D -m 0644 $(@D)/so/lib32/*  $(TARGET_DIR)/usr/lib/
	CONFIG_PATH=$(@D)/config/32bit
endif

define AML_NN_DETECT_MTCNN_V1_INSTALL_TARGET_CMDS
    $(AML_NN_DETECT_MTCNN_V1_INSTALL_TARGETS_CMDS)
	if [ $(BR2_PACKAGE_AML_SOC_FAMILY_NAME) = "G12B" ]; then \
		mkdir -p $(TARGET_DIR)/etc/nn_data/config/revA; \
		mkdir -p $(TARGET_DIR)/etc/nn_data/config/revB; \
		$(INSTALL) -D -m 0644 $(CONFIG_PATH)/revA/* $(TARGET_DIR)/etc/nn_data/config/revA/; \
		$(INSTALL) -D -m 0644 $(CONFIG_PATH)/revB/* $(TARGET_DIR)/etc/nn_data/config/revB/; \
	fi
	if [ $(BR2_PACKAGE_AML_SOC_FAMILY_NAME) = "SM1" ]; then \
		mkdir -p $(TARGET_DIR)/etc/nn_data/config/sm1; \
		$(INSTALL) -D -m 0644 $(CONFIG_PATH)/sm1/* $(TARGET_DIR)/etc/nn_data/config/sm1/; \
	fi
	if [ $(BR2_PACKAGE_AML_SOC_FAMILY_NAME) = "C1" ]; then \
		mkdir -p $(TARGET_DIR)/etc/nn_data/config/a1; \
		$(INSTALL) -D -m 0644 $(CONFIG_PATH)/a1/* $(TARGET_DIR)/etc/nn_data/config/a1/; \
	fi
	mkdir -p $(TARGET_DIR)/etc/nn_data/config/sdk/nnvxc_kernels
    mkdir -p $(TARGET_DIR)/etc/nn_data/config/sdk/include/CL
    $(INSTALL) -D -m 0644 $(@D)/nn_data/* $(TARGET_DIR)/etc/nn_data/
    $(INSTALL) -D -m 0644 $(@D)/config/sdk/include/CL/* $(TARGET_DIR)/etc/nn_data/config/sdk/include/CL/
    $(INSTALL) -D -m 0644 $(@D)/config/sdk/nnvxc_kernels/* $(TARGET_DIR)/etc/nn_data/config/sdk/nnvxc_kernels/
endef

define AML_NN_DETECT_MTCNN_V1_INSTALL_STAGING_CMDS
    $(AML_NN_DETECT_MTCNN_V1_INSTALL_TARGETS_CMDS)
endef

$(eval $(generic-package))
