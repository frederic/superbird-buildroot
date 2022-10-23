#

export ATH_DRIVER_TOPDIR=${ATH_TOPDIR}/drivers
export ATH_QCACLD_TOPDIR=${ATH_DRIVER_TOPDIR}/qcacld-new
export ATH_FW_TOPDIR=${ATH_QCACLD_TOPDIR}/firmware_bin
export QCA_BACKPORTS_DIR=${ATH_DRIVER_TOPDIR}/backports

export FW_WLAN_NAME="qcacld-new"

export QCA_DRIVER_TOPDIR=${ATH_TOPDIR}/drivers

#################################################################################################################################
# Compat related flags
export
COMPAT_LATEST_VERSION = 1
KERNEL_VERSION := $(shell make -C $(KERNELPATH) kernelversion | sed -n 's/^\([0-9]\)\..*/\1/p')
ifneq ($(KERNEL_VERSION),2)
KERNEL_SUBLEVEL := $(shell make -C $(KERNELPATH) kernelversion | sed -n 's/^3\.\([0-9]\+\).*/\1/p')
else
COMPAT_26LATEST_VERSION = 39
KERNEL_26SUBLEVEL := $(shell make -C $(KERNELPATH) kernelversion | sed -n 's/^2\.6\.\([0-9]\+\).*/\1/p')
COMPAT_26VERSIONS := $(shell I=$(COMPAT_26LATEST_VERSION); while [ "$$I" -gt $(KERNEL_26SUBLEVEL) ]; do echo $$I; I=$$(($$I - 1)); done)
$(foreach ver,$(COMPAT_26VERSIONS),$(eval CONFIG_COMPAT_KERNEL_2_6_$(ver)=y))
KERNEL_SUBLEVEL := -1
endif
COMPAT_VERSIONS := $(shell I=$(COMPAT_LATEST_VERSION); while [ "$$I" -gt $(KERNEL_SUBLEVEL) ]; do echo $$I; I=$$(($$I - 1)); done)
$(foreach ver,$(COMPAT_VERSIONS),$(eval CONFIG_COMPAT_KERNEL_3_$(ver)=y))
#
# Need to build compat_firmware_class.ko ?
#
include $(KERNELPATH)/.config
ifdef CONFIG_COMPAT_KERNEL_2_6_33
ifdef CONFIG_FW_LOADER
 CONFIG_COMPAT_FIRMWARE_CLASS=m
endif
endif

#################################################################################################################################
# CFG80211 related flags

# Default always ON even kernel config don't support it.
ifndef CONFIG_CFG80211
CONFIG_CFG80211=m
DEF_CFG80211=y
endif

ifndef CONFIG_CFG80211_DEFAULT_PS
CONFIG_CFG80211_DEFAULT_PS=y
DEF_CFG80211_DEFAULT_PS=y
endif

# We need CONFIG_WIRELESS_EXT for CONFIG_CFG80211_WEXT for every kernel 
# version. The new way CONFIG_CFG80211_WEXT is called from the kernel 
# does not work with compat-wireless because it calls some callback 
# function on struct wiphy. This struct is shipped with compat-wireless 
# and changes from kernel version to version. We are using the 
# wireless_handlers attribute which will be activated by 
ifdef CONFIG_WIRELESS_EXT
ifndef CONFIG_CFG80211_WEXT
CONFIG_CFG80211_WEXT=y
DEF_CFG80211_WEXT=y
endif
endif

ifdef CONFIG_PMF_SUPPORT
ATH_LOCAL_CFLAGS += -DPMF_SUPPORT=y
endif

ifdef CONFIG_CFG80211_INTERNAL_REGDB
CONFIG_ATH6KL_INTERNAL_REGDB=y
ATH_LOCAL_CFLAGS += -DCONFIG_ATH6KL_INTERNAL_REGDB=y
endif

ifdef BUILD_2.6.23_SUPPORT
ATH_LOCAL_CFLAGS += -DCE_OLD_KERNEL_SUPPORT_2_6_23=y
endif

#################################################################################################################################
# ATH6KL related flags
ATH_LOCAL_CFLAGS += -DCONFIG_ATH6KL_DEBUG=y

#################################################################################################################################
# Add flags to compiler
ifdef DEF_CFG80211
ATH_LOCAL_CFLAGS += -DCONFIG_CFG80211=y
endif

ifdef DEF_CFG80211_DEFAULT_PS
ATH_LOCAL_CFLAGS += -DCONFIG_CFG80211_DEFAULT_PS=y
endif

ifdef DEF_CFG80211_WEXT
ATH_LOCAL_CFLAGS += -DCONFIG_CFG80211_WEXT=y
endif

#NOSTDINC_FLAGS := -I${WLAN_DRIVER_TOPDIR}/cfg80211/include/


KBUILD_OPTIONS := WLAN_ROOT=$(PWD)/$(FW_WLAN_NAME)
ifdef CONFIG_ANDROID_APQ8084_PLATFORM_TUFELLO
KBUILD_OPTIONS += MODNAME=Tuffelo_wlan
else
KBUILD_OPTIONS += MODNAME=wlan
endif

WLAN_SELECT := CONFIG_QCA_CLD_WLAN=m
KBUILD_OPTIONS += CONFIG_QCA_WIFI_ISOC=0
KBUILD_OPTIONS += CONFIG_QCA_WIFI_2_0=1
KBUILD_OPTIONS += $(WLAN_SELECT)
KBUILD_OPTIONS += WLAN_OPEN_SOURCE=1


ifeq ($(IF_TYPE),PCIE)
KBUILD_OPTIONS += CONFIG_PRIMA_WLAN_11AC_HIGH_TP=$(CONFIG_PRIMA_WLAN_11AC_HIGH_TP)
endif 
ifeq ($(IF_TYPE),SDIO)
KBUILD_OPTIONS += CONFIG_CLD_HL_SDIO_CORE=$(CONFIG_CLD_HL_SDIO_CORE)
endif
KBUILD_OPTIONS += CONFIG_LINUX_QCMBR=$(CONFIG_LINUX_QCMBR)
KBUILD_OPTIONS += $(KBUILD_EXTRA) # Extra config if any

ifneq ($(CONFIG_NO_USE_BACKPORTS),y)

NOSTDINC_FLAGS += \
	-I$(ATH_DRIVER_TOPDIR)/$(CFG80211_NAME)/backport-include/ \
	-I$(ATH_DRIVER_TOPDIR)/$(CFG80211_NAME)/backport-include/uapi \
	-I$(ATH_DRIVER_TOPDIR)/$(CFG80211_NAME)/include/ \
	-I$(ATH_DRIVER_TOPDIR)/$(CFG80211_NAME)/include/uapi \
	-I$(ATH_DRIVER_TOPDIR)/$(CFG80211_NAME)/include/drm \
	-include $(ATH_DRIVER_TOPDIR)/$(CFG80211_NAME)/backport-include/backport/backport.h \
	$(call backport-cc-disable-warning, unused-but-set-variable) \
	-DBACKPORTS_VERSION=\"$(BACKPORTS_VERSION)\" \
	-DBACKPORTED_KERNEL_VERSION=\"$(BACKPORTED_KERNEL_VERSION)\" \
	-DBACKPORTED_KERNEL_NAME=\"$(BACKPORTED_KERNEL_NAME)\" \
	$(BACKPORTS_GIT_TRACKER_DEF) \
	$(CFLAGS)


	TARGET_ALL := backports_build qcacld_build
	TARGET_CLEAN := backports_clean qcacld_clean

KERNEL_BACKPORT_PWD := $(ATH_DRIVER_TOPDIR)/$(CFG80211_NAME)
MODULE_SYMBOL_DIR := $(ATH_DRIVER_TOPDIR)/$(CFG80211_NAME)
else ifeq ($(CONFIG_CFG80211_DEPEND_ON_KERNEL),y)
	NOSTDINC_FLAGS +=-fno-pic
	TARGET_ALL := qcacld_build
	TARGET_CLEAN :=  qcacld_clean
	MODULE_SYMBOL_DIR :=${CONFIG_KERNEL_SYMBOL_DIR}
else
	NOSTDINC_FLAGS += \
       	-I$(CFG80211_DIR)/include/ \
       	-I$(CFG80211_DIR)/net/wireless \
       	-DCONFIG_NL80211_TESTMODE=y \
       	$(CFLAGS)
	TARGET_ALL := cfg80211_build qcacld_build
	TARGET_CLEAN := cfg80211_clean qcacld_clean
	MODULE_SYMBOL_DIR := $(CFG80211_DIR)/net/wireless
endif

#################################################################################################################################
all: $(TARGET_ALL)

clean: $(TARGET_CLEAN)

# backport build for cfg80211
backports_build:
	${MAKEARCH} -C $(KERNEL_BACKPORT_PWD) SUBDIRS=$(KERNEL_BACKPORT_PWD) modules

backports_clean:
	${MAKEARCH} -C $(KERNEL_BACKPORT_PWD) SUBDIRS=$(KERNEL_BACKPORT_PWD) clean

#
# build for cfg80211 without backports
#
cfg80211_build:
	${MAKEARCH} -C $(KERNELPATH) SUBDIRS=$(CFG80211_DIR)/net/wireless modules

cfg80211_clean:
	${MAKEARCH} -C $(KERNELPATH) SUBDIRS=$(CFG80211_DIR)/net/wireless clean

#
# qcacld driver build
#
qcacld_build:
	${MAKEARCH} -C $(KERNELPATH) SUBDIRS=$(ATH_QCACLD_TOPDIR) KBUILD_EXTRA_SYMBOLS=$(MODULE_SYMBOL_DIR)/Module.symvers modules  $(KBUILD_OPTIONS) -j8

qcacld_clean:
	${MAKEARCH} -C $(KERNELPATH) SUBDIRS=$(ATH_QCACLD_TOPDIR) clean


firmware_install: 
	@echo install QCA WLAN firmware
	@rm -rf ${INSTALL_ROOT}/lib/firmware/wlan/
	@mkdir -p ${INSTALL_ROOT}/lib/firmware/wlan/
	@mkdir -p ${INSTALL_ROOT}/lib/firmware/WLAN-firmware
	@mkdir -p ${INSTALL_ROOT}/lib/firmware/BT-firmware
ifeq ($(IF_TYPE),PCIE)
	@cp $(ATH_DRIVER_TOPDIR)/qcacld-new/firmware_bin/WCNSS_cfg.dat ${INSTALL_ROOT}/lib/firmware/wlan/cfg.dat;
	@cp $(ATH_DRIVER_TOPDIR)/qcacld-new/firmware_bin/WCNSS_qcom_cfg.ini ${INSTALL_ROOT}/lib/firmware/wlan/qcom_cfg.ini;
	@cp -rf $(ATH_DRIVER_TOPDIR)/firmware/WLAN-firmware  ${INSTALL_ROOT}/lib/firmware
	@cp -rf $(ATH_DRIVER_TOPDIR)/firmware/BT-firmware  ${INSTALL_ROOT}/lib/firmware
else ifeq ($(IF_TYPE),USB)
	@cp $(ATH_DRIVER_TOPDIR)/qcacld-new/firmware_bin/WCNSS_cfg.dat ${INSTALL_ROOT}/lib/firmware/wlan/cfg.dat;
	@cp $(ATH_DRIVER_TOPDIR)/qcacld-new/firmware_bin/WCNSS_qcom_cfg.usb.ini ${INSTALL_ROOT}/lib/firmware/wlan/qcom_cfg.ini;
	@cp -rf $(ATH_DRIVER_TOPDIR)/firmware/WLAN-firmware  ${INSTALL_ROOT}/lib/firmware/
	@cp -rf $(ATH_DRIVER_TOPDIR)/firmware/BT-firmware  ${INSTALL_ROOT}/lib/firmware/
else ifeq ($(IF_TYPE),SDIO)
	@cp $(ATH_DRIVER_TOPDIR)/qcacld-new/firmware_bin/WCNSS_cfg.dat ${INSTALL_ROOT}/lib/firmware/wlan/cfg.dat;
	@cp $(ATH_DRIVER_TOPDIR)/qcacld-new/firmware_bin/WCNSS_qcom_cfg.usb.ini ${INSTALL_ROOT}/lib/firmware/wlan/qcom_cfg.ini;
	@cp -rf $(ATH_DRIVER_TOPDIR)/firmware/WLAN-firmware  ${INSTALL_ROOT}/lib/firmware/
	@cp -rf $(ATH_DRIVER_TOPDIR)/firmware/BT-firmware  ${INSTALL_ROOT}/lib/firmware/
endif

