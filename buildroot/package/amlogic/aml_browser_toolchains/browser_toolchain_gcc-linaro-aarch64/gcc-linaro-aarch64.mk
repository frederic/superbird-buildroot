#############################################################
#
# Amlogic Toolchain for Cobalt/Chromium
#
#############################################################
BROWSER_TOOLCHAIN_GCC_LINARO_AARCH64_VERSION	= 7.3.1-2018.05
BROWSER_TOOLCHAIN_GCC_LINARO_AARCH64_SOURCE		= gcc-linaro-$(BROWSER_TOOLCHAIN_GCC_LINARO_AARCH64_VERSION)-x86_64_aarch64-linux-gnu.tar.xz
BROWSER_TOOLCHAIN_GCC_LINARO_AARCH64_SITE		= file://$(TOPDIR)/../vendor/amlogic/external/packages

BROWSER_TOOLCHAIN_GCC_LINARO_AARCH64_INSTALL_DIR = $(HOST_DIR)/usr/browser-toolchain/gcc-linaro-$(BROWSER_TOOLCHAIN_GCC_LINARO_AARCH64_VERSION)-x86_64_aarch64-linux-gnu

define BROWSER_TOOLCHAIN_GCC_LINARO_AARCH64_INSTALL_TARGET_CMDS
	rm -rf $(BROWSER_TOOLCHAIN_GCC_LINARO_AARCH64_INSTALL_DIR)/*
	mkdir -p $(BROWSER_TOOLCHAIN_GCC_LINARO_AARCH64_INSTALL_DIR)
	mv $(@D)/* $(BROWSER_TOOLCHAIN_GCC_LINARO_AARCH64_INSTALL_DIR)/
endef

$(eval $(generic-package))
