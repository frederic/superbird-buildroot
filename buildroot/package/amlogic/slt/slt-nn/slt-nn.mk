#
# SLT NN
#
SLT_NN_VERSION = 1.0
SLT_NN_SITE = $(TOPDIR)/../vendor/amlogic/slt/npu_app/NN_SLT/DnCnn-test
SLT_NN_SITE_METHOD = local
SLT_NN_DEPENDENCIES = npu jpeg

define SLT_NN_BUILD_CMDS
    cd $(@D);mkdir -p obj;$(MAKE) CC=$(TARGET_CC)
endef

define SLT_NN_INSTALL_TARGET_CMDS
    mkdir -p $(TARGET_DIR)/usr/bin/NN_SLT
    $(INSTALL) -D -m 0755 $(@D)/run.sh $(TARGET_DIR)/usr/bin/NN_SLT
    $(INSTALL) -D -m 0755 $(@D)/DnCNN.export.data $(TARGET_DIR)/usr/bin/NN_SLT
    $(INSTALL) -D -m 0755 $(@D)/dncnn $(TARGET_DIR)/usr/bin/NN_SLT
    $(INSTALL) -D -m 0755 $(@D)/output.bin $(TARGET_DIR)/usr/bin/NN_SLT
    $(INSTALL) -D -m 0755 $(@D)/input.bin $(TARGET_DIR)/usr/bin/NN_SLT
    $(INSTALL) -D -m 0755 $(@D)/inp_2.tensor $(TARGET_DIR)/usr/bin/NN_SLT
endef

define SLT_NN_INSTALL_CLEAN_CMDS
    $(MAKE) CC=$(TARGET_CC) -C $(@D) clean
endef

$(eval $(generic-package))
