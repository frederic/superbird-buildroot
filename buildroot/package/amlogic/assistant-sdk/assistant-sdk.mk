###########################################################################
#
#assistant-sdk
#
###########################################################################
file = $(TOPDIR)/../multimedia/assistant_sdk
ifeq ($(file), $(wildcard $(file)))
ASSISTANT_SDK_SITE = $(file)
ASSISTANT_SDK_SITE_METHOD = local

define ASSISTANT_SDK_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/bin/* $(TARGET_DIR)/usr/bin/
    $(INSTALL) -D -m 0755 $(@D)/lib/* $(TARGET_DIR)/usr/lib/
    $(INSTALL) -D -m 0644 $(@D)/config/* $(TARGET_DIR)/etc/
endef

$(eval $(generic-package))
else
$(warning " assistant_sdk file no exist")
endif
