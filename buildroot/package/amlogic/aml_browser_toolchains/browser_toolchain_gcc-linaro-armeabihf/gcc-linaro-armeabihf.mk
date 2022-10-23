#############################################################
#
# Amlogic Toolchain for Cobalt/Chromium
#
#############################################################
BROWSER_TOOLCHAIN_GCC_LINARO_ARMEABIHF_VERSION	= 7.3.1-2018.05
BROWSER_TOOLCHAIN_GCC_LINARO_ARMEABIHF_SOURCE	= gcc-linaro-$(BROWSER_TOOLCHAIN_GCC_LINARO_ARMEABIHF_VERSION)-x86_64_arm-linux-gnueabihf.tar.xz
BROWSER_TOOLCHAIN_GCC_LINARO_ARMEABIHF_SITE		= file://$(TOPDIR)/../vendor/amlogic/external/packages

BROWSER_TOOLCHAIN_GCC_LINARO_ARMEABIHF_INSTALL_DIR = $(HOST_DIR)/usr/browser-toolchain/gcc-linaro-$(BROWSER_TOOLCHAIN_GCC_LINARO_ARMEABIHF_VERSION)-x86_64_arm-linux-gnueabihf

define BROWSER_TOOLCHAIN_GCC_LINARO_ARMEABIHF_INSTALL_TARGET_CMDS
	rm -rf $(BROWSER_TOOLCHAIN_GCC_LINARO_ARMEABIHF_INSTALL_DIR)/*
	mkdir -p $(BROWSER_TOOLCHAIN_GCC_LINARO_ARMEABIHF_INSTALL_DIR)
	mv $(@D)/* $(BROWSER_TOOLCHAIN_GCC_LINARO_ARMEABIHF_INSTALL_DIR)/
endef

$(eval $(generic-package))
