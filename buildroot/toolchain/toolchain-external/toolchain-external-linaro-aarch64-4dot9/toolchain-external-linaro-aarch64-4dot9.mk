################################################################################
#
# toolchain-external-linaro-aarch64-201409
#
################################################################################

TOOLCHAIN_EXTERNAL_LINARO_AARCH64_4DOT9_SITE = https://releases.linaro.org/components/toolchain/binaries/4.9-2016.02/aarch64-linux-gnu

ifeq ($(HOSTARCH),x86)
TOOLCHAIN_EXTERNAL_LINARO_AARCH64_4DOT9_SOURCE = gcc-linaro-4.9-2016.02-i686-mingw32_aarch64-linux-gnu.tar.xz
else
TOOLCHAIN_EXTERNAL_LINARO_AARCH64_4DOT9_SOURCE = gcc-linaro-4.9-2016.02-x86_64_aarch64-linux-gnu.tar.xz
endif

$(eval $(toolchain-external-package))
