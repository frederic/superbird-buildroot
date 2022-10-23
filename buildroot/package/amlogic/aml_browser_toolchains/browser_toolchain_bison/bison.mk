#############################################################
#
# Amlogic Toolchain for Cobalt/Chromium
#
#############################################################
BROWSER_TOOLCHAIN_BISON_VERSION		= 2.7.1
BROWSER_TOOLCHAIN_BISON_SOURCE		= bison-$(BROWSER_TOOLCHAIN_BISON_VERSION)-x86_64.tar.gz
BROWSER_TOOLCHAIN_BISON_SITE		= file://$(TOPDIR)/../vendor/amlogic/external/packages

BROWSER_TOOLCHAIN_BISON_INSTALL_DIR = $(HOST_DIR)/usr/browser-toolchain/bison-$(BROWSER_TOOLCHAIN_BISON_VERSION)

define BROWSER_TOOLCHAIN_BISON_INSTALL_TARGET_CMDS
	rm -rf $(BROWSER_TOOLCHAIN_BISON_INSTALL_DIR)/*
	mkdir -p $(BROWSER_TOOLCHAIN_BISON_INSTALL_DIR)
	mv $(@D)/* $(BROWSER_TOOLCHAIN_BISON_INSTALL_DIR)/
endef

BROWSER_BISON_TOOLS_PATH=$(BROWSER_TOOLCHAIN_BISON_INSTALL_DIR)

$(eval $(generic-package))
