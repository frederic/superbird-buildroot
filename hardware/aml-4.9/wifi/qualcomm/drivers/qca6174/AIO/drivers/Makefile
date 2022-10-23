#

export ATH_DRIVER_TOPDIR=${ATH_TOPDIR}/drivers
export ATH_QCACLD_TOPDIR=${ATH_DRIVER_TOPDIR}/qcacld-new
export ATH_FW_TOPDIR=${ATH_QCACLD_TOPDIR}/firmware_bin
export QCA_BACKPORTS_DIR=${ATH_DRIVER_TOPDIR}/backports

export FW_WLAN_NAME="qcacld-new"

export QCA_DRIVER_TOPDIR=${ATH_TOPDIR}/drivers


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

KBUILD_OPTIONS := WLAN_ROOT=$(QCA_DRIVER_TOPDIR)/$(FW_WLAN_NAME)
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

NOSTDINC_FLAGS +=-fno-pic
TARGET_ALL := qcacld_build
TARGET_CLEAN :=  qcacld_clean
MODULE_SYMBOL_DIR :=${CONFIG_KERNEL_SYMBOL_DIR}

#################################################################################################################################
all: $(TARGET_ALL)

clean: $(TARGET_CLEAN)

#
# qcacld driver build
#
qcacld_build:
	${MAKEARCH} -C $(KERNELPATH) SUBDIRS=$(ATH_QCACLD_TOPDIR) KBUILD_EXTRA_SYMBOLS=$(MODULE_SYMBOL_DIR)/Module.symvers modules  $(KBUILD_OPTIONS)

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

