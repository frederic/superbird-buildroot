#
# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#Supported modules:
#                bcm40183
#                bcm40181
#		 bcm43458
#                rtl8188eu
#                rt5370
#                rt8189es
#                rt8723bs
#                rtl8723au
#                mt7601
#                mt5931
#                AP62x2
#                AP6335
#                AP6441
#                AP6234
#                AP6181
#                AP6210
#                bcm43341
#                bcm43241
#                rtl8192du
#                rtl8192eu
#                rtl8192es
#                rtl8192cu
#                rtl88x1au
#                rtl8812au

$(warning WIFI_MOUDLE is $(WIFI_MODULE))
ifeq ($(WIFI_BUILD_IN), true)
$(warning WIFI_BUILD_IN is true)
else
$(warning WIFI_BUILD_IN is false)
endif

BCM_USB_COMPOSITE ?= false
ifeq ($(BCM_USB_COMPOSITE), true)
CONFIG_BCMDHD_CUSB := y
export CONFIG_BCMDHD_CUSB
endif

################################################################################## enable clang CFI for arm64
ifeq ($(ANDROID_BUILD_TYPE), 64)
PRODUCT_CFI_INCLUDE_PATHS += hardware/amlogic/wifi/bcm_ampak/wpa_supplicant_8_lib
PRODUCT_CFI_INCLUDE_PATHS += hardware/amlogic/wifi/wifi_hal/wpa_supplicant_8_lib
endif
##################################################################################

PRODUCT_PACKAGES += wpa_supplicant.conf

PRODUCT_COPY_FILES += \
	frameworks/native/data/etc/android.hardware.wifi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.xml
PRODUCT_PROPERTY_OVERRIDES += \
	ro.carrier=wifi-only

PRODUCT_PACKAGES += \
    wificond \
    wifilogd \
    libwifi-hal-common-ext

################################################################################## buildin
ifeq ($(WIFI_BUILD_IN), true)
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6255/fw_bcm43455c0_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/fw_bcm43455c0_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6255/nvram.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/nvram_ap6255.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4356/fw_bcm4356a2_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/fw_bcm4356a2_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4356/nvram_ap6356.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/nvram_ap6356.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6398/Wi-Fi/fw_bcm4359c0_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/fw_bcm4359c0_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6398/Wi-Fi/nvram_ap6398s.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/nvram_ap6398s.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6271S/Wi-Fi/clm_bcm43751a1_ag.blob:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/clm_bcm43751a1_ag.blob
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6271S/Wi-Fi/fw_bcm43751a1_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/fw_bcm43751a1_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6271S/Wi-Fi/nvram_ap6271s.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/nvram_ap6271s.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/buildin/config_bcm43456c5_ag.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/config_bcm43456c5_ag.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6236/Wi-Fi/fw_bcm43436b0.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/fw_bcm43436b0.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6236/Wi-Fi/nvram_ap6236.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/nvram_ap6236.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6256/Wi-Fi/fw_bcm43456c5_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/fw_bcm43456c5_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6256/Wi-Fi/nvram_ap6256.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/nvram_ap6256.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/buildin/config_bcm4359c0_ag.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/config_bcm4359c0_ag.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/buildin/config_bcm4356a2_ag.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/config_bcm4356a2_ag.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/buildin/config_bcm43455c0_ag.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/config_bcm43455c0_ag.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/buildin/config_bcm43438a1.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/config_bcm43438a1.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/buildin/config_bcm43438a0.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/config_bcm43438a0.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/buildin/config_bcm43436b0.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/config_bcm43436b0.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/buildin/config_bcm4339a0_ag.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/config_bcm4339a0_ag.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/multi_wifi/config/wpa_supplicant.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/wpa_supplicant.conf
PRODUCT_COPY_FILES += hardware/amlogic/wifi/multi_wifi/config/wpa_supplicant.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/p2p_supplicant.conf
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6335/fw_bcm4339a0_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/fw_bcm4339a0_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6335/nvram.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/nvram_ap6335.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6212/fw_bcm43438a0.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/fw_bcm43438a0.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6212/nvram.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/nvram_ap6212.txt
ifeq ($(BCM_USB_COMPOSITE),true)
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/USB_COMPOSITE/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/fw_bcm4358u_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/USB_COMPOSITE/nvram_ap62x8.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/nvram_ap62x8.txt
else
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/fw_bcm4358u_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/nvram_ap62x8.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/buildin/nvram_ap62x8.txt
endif
endif
################################################################################## bcm4354
ifeq ($(WIFI_MODULE),bcm4354)
WIFI_DRIVER := bcm4354
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/dhd.ko
WIFI_DRIVER_MODULE_NAME := dhd
WIFI_DRIVER_MODULE_ARG  := "firmware_path=/vendor/etc/wifi/4354/fw_bcm4354a1_ag.bin nvram_path=/vendor/etc/wifi/4354/nvram_ap6354.txt"
WIFI_DRIVER_FW_PATH_STA := /vendor/etc/wifi/4354/fw_bcm4354a1_ag.bin
WIFI_DRIVER_FW_PATH_AP  := /vendor/etc/wifi/4354/fw_bcm4354a1_ag_apsta.bin
WIFI_DRIVER_FW_PATH_P2P := /vendor/etc/wifi/4354/fw_bcm4354a1_ag_p2p.bin

BOARD_WLAN_DEVICE := bcmdhd
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd

PRODUCT_PACKAGES += \
	4354/nvram_ap6354.txt \
	4354/fw_bcm4354a1_ag.bin \
	4354/fw_bcm4354a1_ag_apsta.bin \
	4354/fw_bcm4354a1_ag_p2p.bin \
	wl \
	p2p_supplicant_overlay.conf \
	dhd

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml

PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

ifneq ($(wildcard $(TARGET_PRODUCT_DIR)/dhd.ko),)
PRODUCT_COPY_FILES += $(TARGET_PRODUCT_DIR)/dhd.ko:$(PRODUCT_OUT)/obj/lib_vendor/dhd.ko
endif

PRODUCT_PROPERTY_OVERRIDES += \
	wifi.interface=wlan0

endif

################################################################################## BCMWIFI
ifeq ($(WIFI_MODULE), BCMWIFI)
WIFI_DRIVER := AP6398
WIFI_DRIVER_FW_PATH_STA := /vendor/etc/wifi/buildin/sta.bin
WIFI_DRIVER_FW_PATH_AP  := /vendor/etc/wifi/buildin/ap.bin
WIFI_DRIVER_FW_PATH_P2P := /vendor/etc/wifi/buildin/p2p.bin
BOARD_WLAN_DEVICE := MediaTek
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd_ampak
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd_ampak
PRODUCT_PACKAGES += \
        p2p_supplicant_overlay.conf

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi_buildin.rc
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc
PRODUCT_COPY_FILES += hardware/amlogic/wifi/multi_wifi/config/wpa_supplicant.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/wpa_supplicant.conf


PRODUCT_PROPERTY_OVERRIDES += \
        wifi.interface=wlan0 \
        wifi.direct.interface=p2p-dev-wlan0
endif
################################################################################## AP6398
ifeq ($(WIFI_MODULE), AP6398)
WIFI_DRIVER := AP6398
ifneq ($(WIFI_BUILD_IN), true)
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/dhd.ko
WIFI_DRIVER_MODULE_NAME := dhd
WIFI_DRIVER_MODULE_ARG  := "firmware_path=/vendor/etc/wifi/AP6398/fw_bcm4359c0_ag_apsta.bin nvram_path=/vendor/etc/wifi/AP6398/nvram.txt"
endif
WIFI_DRIVER_FW_PATH_STA := /vendor/etc/wifi/AP6398/fw_bcm4359c0_ag.bin
WIFI_DRIVER_FW_PATH_AP  := /vendor/etc/wifi/AP6398/fw_bcm4359c0_ag_apsta.bin
WIFI_DRIVER_FW_PATH_P2P := /vendor/etc/wifi/AP6398/fw_bcm4359c0_ag_p2p.bin
ifneq ($(WIFI_BUILD_IN), true)
BOARD_WLAN_DEVICE := bcmdhd
else
BOARD_WLAN_DEVICE := MediaTek
endif
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd_ampak
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd_ampak
PRODUCT_PACKAGES += \
        p2p_supplicant_overlay.conf

PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6398/Wi-Fi/fw_bcm4359c0_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/AP6398/fw_bcm4359c0_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6398/Wi-Fi/fw_bcm4359c0_ag_apsta.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/AP6398/fw_bcm4359c0_ag_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6398/Wi-Fi/fw_bcm4359c0_ag_p2p.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/AP6398/fw_bcm4359c0_ag_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6398/Wi-Fi/nvram_ap6398s.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/AP6398/nvram.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/config.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/AP6398/config.txt
PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml
ifeq ($(WIFI_BUILD_IN), true)
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi_buildin.rc
endif
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc
PRODUCT_COPY_FILES += hardware/amlogic/wifi/multi_wifi/config/wpa_supplicant.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/wpa_supplicant.conf


PRODUCT_PROPERTY_OVERRIDES += \
        wifi.interface=wlan0 \
	wifi.direct.interface=p2p-dev-wlan0
endif
################################################################################## bcm4356
ifeq ($(WIFI_MODULE),bcm4356)
WIFI_DRIVER := bcm4356
ifneq ($(WIFI_BUILD_IN), true)
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/dhd.ko
WIFI_DRIVER_MODULE_NAME := dhd
WIFI_DRIVER_MODULE_ARG  := "firmware_path=/vendor/etc/wifi/4356/fw_bcm4356a2_ag.bin nvram_path=/vendor/etc/wifi/4356/nvram_ap6356.txt"
endif
WIFI_DRIVER_FW_PATH_STA := /vendor/etc/wifi/4356/fw_bcm4356a2_ag.bin
WIFI_DRIVER_FW_PATH_AP  := /vendor/etc/wifi/4356/fw_bcm4356a2_ag_apsta.bin
WIFI_DRIVER_FW_PATH_P2P := /vendor/etc/wifi/4356/fw_bcm4356a2_ag_p2p.bin
ifneq ($(WIFI_BUILD_IN), true)
BOARD_WLAN_DEVICE := bcmdhd
else
BOARD_WLAN_DEVICE := MediaTek
endif
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd_ampak
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd_ampak

PRODUCT_PACKAGES += \
        4356/nvram_ap6356.txt \
        4356/fw_bcm4356a2_ag.bin \
	4356/fw_bcm4356a2_ag_apsta.bin \
	4356/fw_bcm4356a2_ag_p2p.bin \
        wl \
        p2p_supplicant_overlay.conf \
        dhd

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml
ifeq ($(WIFI_BUILD_IN), true)
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi_buildin.rc
endif
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc
PRODUCT_COPY_FILES += hardware/amlogic/wifi/multi_wifi/config/wpa_supplicant.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/wpa_supplicant.conf

ifneq ($(wildcard $(TARGET_PRODUCT_DIR)/dhd.ko),)
PRODUCT_COPY_FILES += $(TARGET_PRODUCT_DIR)/dhd.ko:$(PRODUCT_OUT)/obj/lib_vendor/dhd.ko
endif

PRODUCT_PROPERTY_OVERRIDES += \
        wifi.interface=wlan0 \
	wifi.direct.interface=p2p-dev-wlan0

endif


################################################################################## bcm4358
ifeq ($(WIFI_MODULE),bcm4358)
WIFI_DRIVER := bcm4358
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/dhd.ko
WIFI_DRIVER_MODULE_NAME := dhd
WIFI_DRIVER_MODULE_ARG  := "firmware_path=/vendor/etc/wifi/4358/fw_bcm4358_ag.bin nvram_path=/vendor/etc/wifi/4358/nvram_4358.txt"
WIFI_DRIVER_FW_PATH_STA := /vendor/etc/wifi/4358/fw_bcm4358_ag.bin
WIFI_DRIVER_FW_PATH_AP  := /vendor/etc/wifi/4358/fw_bcm4358_ag_apsta.bin
WIFI_DRIVER_FW_PATH_P2P := /vendor/etc/wifi/4358/fw_bcm4358_ag_p2p.bin

BOARD_WLAN_DEVICE := bcmdhd
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd

PRODUCT_PACKAGES += \
        4358/nvram_4358.txt \
        4358/fw_bcm4358_ag.bin \
    4358/fw_bcm4358_ag_apsta.bin \
    4358/fw_bcm4358_ag_p2p.bin \
        wl \
        p2p_supplicant_overlay.conf \
        dhd

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml

PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/config.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4358/config.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4358/config.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4358/config.txt

ifneq ($(wildcard $(TARGET_PRODUCT_DIR)/dhd.ko),)
PRODUCT_COPY_FILES += $(TARGET_PRODUCT_DIR)/dhd.ko:$(PRODUCT_OUT)/obj/lib_vendor/dhd.ko
endif

PRODUCT_PROPERTY_OVERRIDES += \
        wifi.interface=wlan0

endif


################################################################################## bcm43458
ifeq ($(WIFI_MODULE),bcm43458)
WIFI_DRIVER := bcm43458
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/dhd.ko
WIFI_DRIVER_MODULE_NAME := dhd
WIFI_DRIVER_MODULE_ARG  := "firmware_path=/vendor/etc/wifi/43458/fw_bcm43455c0_ag.bin nvram_path=/vendor/etc/wifi/43458/nvram_43458.txt"
WIFI_DRIVER_FW_PATH_STA := /vendor/etc/wifi/43458/fw_bcm43455c0_ag.bin
WIFI_DRIVER_FW_PATH_AP  := /vendor/etc/wifi/43458/fw_bcm43455c0_ag_apsta.bin
WIFI_DRIVER_FW_PATH_P2P := /vendor/etc/wifi/43458/fw_bcm43455c0_ag_p2p.bin

BOARD_WLAN_DEVICE := bcmdhd
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd

PRODUCT_PACKAGES += \
        43458/nvram_43458.txt \
        43458/fw_bcm43455c0_ag.bin \
	 43458/fw_bcm43455c0_ag_apsta.bin \
	 43458/fw_bcm43455c0_ag_p2p.bin \
        wl \
	p2p_supplicant_overlay.conf \
        dhd

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml

PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

ifneq ($(wildcard $(TARGET_PRODUCT_DIR)/dhd.ko),)
PRODUCT_COPY_FILES += $(TARGET_PRODUCT_DIR)/dhd.ko:$(PRODUCT_OUT)/obj/lib_vendor/dhd.ko
endif

PRODUCT_PROPERTY_OVERRIDES += \
        wifi.interface=wlan0

endif


################################################################################## AP6269
ifeq ($(WIFI_MODULE),AP6269)
WIFI_DRIVER := AP6269
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/bcmdhd.ko
WIFI_DRIVER_MODULE_NAME := bcmdhd
WIFI_DRIVER_MODULE_ARG  := ""
WIFI_DRIVER_FW_PATH_STA := /vendor/etc/firmware/fw_bcmdhd.bin.trx
WIFI_DRIVER_FW_PATH_P2P := /vendor/etc/firmware/fw_bcmdhd_p2p.bin.trx
WIFI_DRIVER_FW_PATH_AP := /vendor/etc/firmware/fw_bcmdhd_apsta.bin.trx
BCM_USB_WIFI := true

BOARD_WLAN_DEVICE := bcmdhd
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/bcmdhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd


PRODUCT_PACKAGES += \
	AP6269/fw_bcm43569a2_ag.bin.trx \
	AP6269/nvram_ap6269a2.nvm \
	wl \
	p2p_supplicant_overlay.conf \
	dhd \
	bcmdl



PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml

PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

PRODUCT_PROPERTY_OVERRIDES += \
	wifi.interface=wlan0
endif

################################################################################## AP62x8
ifeq ($(WIFI_MODULE),AP62x8)
WIFI_DRIVER := AP62x8
ifneq ($(WIFI_BUILD_IN), true)
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/bcmdhd.ko
WIFI_DRIVER_MODULE_NAME := bcmdhd
WIFI_DRIVER_MODULE_ARG  := ""
endif
WIFI_DRIVER_FW_PATH_STA := /vendor/etc/wifi/43569/fw_bcm4358u_ag.bin
WIFI_DRIVER_FW_PATH_P2P := /vendor/etc/wifi/43569/fw_bcm4358u_ag_p2p.bin
WIFI_DRIVER_FW_PATH_AP := /vendor/etc/wifi/43569/fw_bcm4358u_ag_apsta.bin
BCM_USB_WIFI := true

ifneq ($(WIFI_BUILD_IN), true)
BOARD_WLAN_DEVICE := bcmdhd
else
BOARD_WLAN_DEVICE := MediaTek
endif

WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/bcmdhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd_ampak
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd_ampak
PRODUCT_PACKAGES += \
        wl \
        p2p_supplicant_overlay.conf \
        dhd \
        bcmdl

ifeq ($(WIFI_BUILD_IN), true)
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_ap62x8.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi_buildin.rc
endif

ifeq ($(BCM_USB_COMPOSITE),true)
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/USB_COMPOSITE/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/fw_bcm4358u_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/USB_COMPOSITE/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/fw_bcm4358u_ag_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/USB_COMPOSITE/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/fw_bcm4358u_ag_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/USB_COMPOSITE/nvram_ap62x8.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/nvram_ap62x8.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/USB_COMPOSITE/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/fw_bcm43569a2_ag.bin.trx
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/USB_COMPOSITE/nvram_ap62x8.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/nvram_ap6269a2.nvm
else
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/fw_bcm4358u_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/fw_bcm4358u_ag_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/fw_bcm4358u_ag_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/nvram_ap62x8.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/nvram_ap62x8.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/fw_bcm43569a2_ag.bin.trx
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/nvram_ap62x8.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/nvram_ap6269a2.nvm
endif


PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml

PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

PRODUCT_PROPERTY_OVERRIDES += \
        wifi.interface=wlan0 \
	wifi.direct.interface=p2p-dev-wlan0
endif
################################################################################## AP6242
ifeq ($(WIFI_MODULE),AP6242)
WIFI_DRIVER := AP6242
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/bcmdhd.ko
WIFI_DRIVER_MODULE_NAME := bcmdhd
WIFI_DRIVER_MODULE_ARG  := ""
WIFI_DRIVER_FW_PATH_STA := /vendor/etc/firmware/fw_bcmdhd.bin.trx
WIFI_DRIVER_FW_PATH_P2P := /vendor/etc/firmware/fw_bcmdhd_p2p.bin.trx
WIFI_DRIVER_FW_PATH_AP := /vendor/etc/firmware/fw_bcmdhd_apsta.bin.trx
BCM_USB_WIFI := true

BOARD_WLAN_DEVICE := bcmdhd
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/bcmdhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd


PRODUCT_PACKAGES += \
	AP6242/fw_bcm43242a1_ag.bin.trx \
	AP6242/nvram_ap6242.nvm \
	wl \
	p2p_supplicant_overlay.conf \
	dhd \
	bcmdl



PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml

PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

PRODUCT_PROPERTY_OVERRIDES += \
	wifi.interface=wlan0
endif

################################################################################## realtek wifi
ifneq ($(filter rtl8188eu rtl8188ftv rtl8192eu rtl8192es rtl8189es rtl8189fs rtl8723bs rtl8723bu rtl8723ds rtl8723du \
		 rtl88x1au rtl8812au rtl8822bu rtl8822bs,$(WIFI_MODULE)),)

WIFI_KO := $(patsubst rtl%,%,$(WIFI_MODULE))
WIFI_DRIVER             := $(WIFI_MODULE)
BOARD_WIFI_VENDOR       := realtek
ifneq ($(WIFI_BUILD_IN), true)
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/$(WIFI_KO).ko
WIFI_DRIVER_MODULE_NAME := $(WIFI_KO)
WIFI_DRIVER_MODULE_ARG  := "ifname=wlan0 if2name=p2p0"
$(warning WIFI_DRIVER_MODULE_PATH is $(WIFI_DRIVER_MODULE_PATH))
$(warning WIFI_DRIVER_MODULE_NAME is $(WIFI_DRIVER_MODULE_NAME))
$(warning WIFI_DRIVER_MODULE_ARG  is $(WIFI_DRIVER_MODULE_ARG))
endif

WPA_SUPPLICANT_VERSION           := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER      := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_rtl
BOARD_HOSTAPD_DRIVER             := NL80211
BOARD_HOSTAPD_PRIVATE_LIB        := lib_driver_cmd_rtl

ifneq ($(WIFI_BUILD_IN), true)
BOARD_WLAN_DEVICE := $(WIFI_MODULE)
else
BOARD_WLAN_DEVICE := MediaTek
endif


WIFI_FIRMWARE_LOADER      := ""
WIFI_DRIVER_FW_PATH_PARAM := ""

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml
ifeq ($(WIFI_BUILD_IN), true)
$(shell cp hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_rtk.rc  hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc)
$(shell sed -i "1a\    insmod \/vendor\/lib/modules\/$(WIFI_KO).ko ifname=wlan0 if2name=p2p0" hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc)
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi_buildin.rc
endif
PRODUCT_COPY_FILES += hardware/amlogic/wifi/multi_wifi/config/p2p_supplicant_overlay.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/p2p_supplicant_overlay.conf
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_rtk.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

PRODUCT_PACKAGES += \
        wpa_supplicant_overlay.conf \
        p2p_supplicant_overlay.conf

# 89976: Add Realtek USB WiFi support
PRODUCT_PROPERTY_OVERRIDES += \
        wifi.interface=wlan0 \
        wifi.direct.interface=p2p0

endif
################################################################################## bcm40183
ifeq ($(WIFI_MODULE),bcm40183)

WIFI_DRIVER := bcm40183
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/dhd.ko
WIFI_DRIVER_MODULE_NAME := dhd
WIFI_DRIVER_MODULE_ARG  := "firmware_path=/vendor/etc/wifi/40183/fw_bcm40183b2.bin nvram_path=/vendor/etc/wifi/40183/nvram.txt"
WIFI_DRIVER_FW_PATH_STA :=/vendor/etc/wifi/40183/fw_bcm40183b2.bin
WIFI_DRIVER_FW_PATH_AP  :=/vendor/etc/wifi/40183/fw_bcm40183b2_apsta.bin
WIFI_DRIVER_FW_PATH_P2P :=/vendor/etc/wifi/40183/fw_bcm40183b2_p2p.bin

BOARD_WLAN_DEVICE := bcmdhd
LIB_WIFI_HAL := libwifi-hal-bcm
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd_ampak
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd_ampak

PRODUCT_PACKAGES += \
	40183/nvram.txt \
	40183/fw_bcm40183b2.bin \
	40183/fw_bcm40183b2_apsta.bin \
	40183/fw_bcm40183b2_p2p.bin \
	wl \
	p2p_supplicant_overlay.conf \
	dhd

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml

PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

ifneq ($(wildcard $(TARGET_PRODUCT_DIR)/dhd.ko),)
PRODUCT_COPY_FILES += $(TARGET_PRODUCT_DIR)/dhd.ko:$(PRODUCT_OUT)/obj/lib_vendor/dhd.ko
endif

PRODUCT_PROPERTY_OVERRIDES += \
	wifi.interface=wlan0

endif

################################################################################## bcm40181
ifeq ($(WIFI_MODULE),bcm40181)
WIFI_DRIVER := bcm40181
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/dhd.ko
WIFI_DRIVER_MODULE_NAME := dhd
WIFI_DRIVER_MODULE_ARG  := "firmware_path=/vendor/etc/wifi/40181/fw_bcm40181a2.bin nvram_path=/vendor/etc/wifi/40181/nvram.txt"
WIFI_DRIVER_FW_PATH_STA :=/vendor/etc/wifi/40181/fw_bcm40181a2.bin
WIFI_DRIVER_FW_PATH_AP  :=/vendor/etc/wifi/40181/fw_bcm40181a2_apsta.bin
WIFI_DRIVER_FW_PATH_P2P :=/vendor/etc/wifi/40181/fw_bcm40181a2_p2p.bin

BOARD_WLAN_DEVICE := bcmdhd
LIB_WIFI_HAL := libwifi-hal-bcm
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd_ampak
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd_ampak

PRODUCT_PACKAGES += \
	40181/nvram.txt \
	40181/fw_bcm40181a0.bin \
	40181/fw_bcm40181a0_apsta.bin \
	40181/fw_bcm40181a2.bin \
	40181/fw_bcm40181a2_apsta.bin \
	40181/fw_bcm40181a2_p2p.bin \
	wl \
	p2p_supplicant_overlay.conf \
	dhd

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml

PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

ifneq ($(wildcard $(TARGET_PRODUCT_DIR)/dhd.ko),)
PRODUCT_COPY_FILES += $(TARGET_PRODUCT_DIR)/dhd.ko:$(PRODUCT_OUT)/obj/lib_vendor/dhd.ko
endif

PRODUCT_PROPERTY_OVERRIDES += \
	wifi.interface=wlan0

endif
################################################################################## AP62x2
ifeq ($(WIFI_MODULE),AP62x2)
WIFI_DRIVER := AP62x2
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/dhd.ko
WIFI_DRIVER_MODULE_NAME := dhd
WIFI_DRIVER_MODULE_ARG  := "firmware_path=/vendor/etc/wifi/62x2/fw_bcm43241b4_ag.bin nvram_path=/vendor/etc/wifi/62x2/nvram.txt"
WIFI_DRIVER_FW_PATH_STA :=/vendor/etc/wifi/62x2/fw_bcm43241b4_ag.bin
WIFI_DRIVER_FW_PATH_AP  :=/vendor/etc/wifi/62x2/fw_bcm43241b4_ag_apsta.bin
WIFI_DRIVER_FW_PATH_P2P :=/vendor/etc/wifi/62x2/fw_bcm43241b4_ag_p2p.bin

BOARD_WLAN_DEVICE := bcmdhd
LIB_WIFI_HAL := libwifi-hal-bcm
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd_ampak
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd_ampak

PRODUCT_PACKAGES += \
	62x2/nvram.txt \
	62x2/fw_bcm43241b4_ag.bin \
	62x2/fw_bcm43241b4_ag_apsta.bin \
	62x2/fw_bcm43241b4_ag_p2p.bin \
	wl \
	p2p_supplicant_overlay.conf \
	dhd

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml

PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

ifneq ($(wildcard $(TARGET_PRODUCT_DIR)/dhd.ko),)
PRODUCT_COPY_FILES += $(TARGET_PRODUCT_DIR)/dhd.ko:$(PRODUCT_OUT)/obj/lib_vendor/dhd.ko
endif

PRODUCT_PROPERTY_OVERRIDES += \
	wifi.interface=wlan0

endif
################################################################################## AP6335
ifeq ($(WIFI_MODULE),AP6335)
WIFI_DRIVER := AP6335
ifneq ($(WIFI_BUILD_IN), true)
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/dhd.ko
WIFI_DRIVER_MODULE_NAME := dhd
WIFI_DRIVER_MODULE_ARG  := "firmware_path=/vendor/etc/wifi/6335/fw_bcm4339a0_ag.bin nvram_path=/vendor/etc/wifi/6335/nvram.txt"
endif
WIFI_DRIVER_FW_PATH_STA :=/vendor/etc/wifi/6335/fw_bcm4339a0_ag.bin
WIFI_DRIVER_FW_PATH_AP  :=/vendor/etc/wifi/6335/fw_bcm4339a0_ag_apsta.bin
WIFI_DRIVER_FW_PATH_P2P :=/vendor/etc/wifi/6335/fw_bcm4339a0_ag_p2p.bin

ifneq ($(WIFI_BUILD_IN), true)
BOARD_WLAN_DEVICE := bcmdhd
else
BOARD_WLAN_DEVICE := MediaTek
endif
LIB_WIFI_HAL := libwifi-hal-bcm
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd_ampak
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd_ampak
PRODUCT_PACKAGES += \
	6335/nvram.txt \
	6335/fw_bcm4339a0_ag.bin \
	6335/fw_bcm4339a0_ag_apsta.bin \
	6335/fw_bcm4339a0_ag_p2p.bin \
	6335/nvram_ap6335e.txt   \
	6335/fw_bcm4339a0e_ag.bin \
	6335/fw_bcm4339a0e_ag_apsta.bin \
	6335/fw_bcm4339a0e_ag_p2p.bin \
	wl \
	p2p_supplicant_overlay.conf \
	dhd

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml
ifeq ($(WIFI_BUILD_IN), true)
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi_buildin.rc
endif
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

ifneq ($(wildcard $(TARGET_PRODUCT_DIR)/dhd.ko),)
PRODUCT_COPY_FILES += $(TARGET_PRODUCT_DIR)/dhd.ko:$(PRODUCT_OUT)/obj/lib_vendor/dhd.ko
endif

PRODUCT_PROPERTY_OVERRIDES += \
	wifi.interface=wlan0 \
	wifi.direct.interface=p2p-dev-wlan0

endif
################################################################################## AP6441
ifeq ($(WIFI_MODULE),AP6441)
WIFI_DRIVER := AP6441
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/dhd.ko
WIFI_DRIVER_MODULE_NAME := dhd
WIFI_DRIVER_MODULE_ARG  := "firmware_path=/vendor/etc/wifi/6441/fw_bcm43341b0_ag.bin nvram_path=/vendor/etc/wifi/6441/nvram.txt"
WIFI_DRIVER_FW_PATH_STA :=/vendor/etc/wifi/6441/fw_bcm43341b0_ag.bin
WIFI_DRIVER_FW_PATH_AP  :=/vendor/etc/wifi/6441/fw_bcm43341b0_ag_apsta.bin
WIFI_DRIVER_FW_PATH_P2P :=/vendor/etc/wifi/6441/fw_bcm43341b0_ag_p2p.bin

BOARD_WLAN_DEVICE := bcmdhd
LIB_WIFI_HAL := libwifi-hal-bcm
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd_ampak
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd_ampak
PRODUCT_PACKAGES += \
	6441/nvram.txt    \
	6441/fw_bcm43341b0_ag.bin \
	6441/fw_bcm43341b0_ag_apsta.bin \
	6441/fw_bcm43341b0_ag_p2p.bin \
	wl \
	p2p_supplicant_overlay.conf \
	dhd

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml

PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

ifneq ($(wildcard $(TARGET_PRODUCT_DIR)/dhd.ko),)
PRODUCT_COPY_FILES += $(TARGET_PRODUCT_DIR)/dhd.ko:$(PRODUCT_OUT)/obj/lib_vendor/dhd.ko
endif

PRODUCT_PROPERTY_OVERRIDES += \
	wifi.interface=wlan0

endif

################################################################################## AP6234
ifeq ($(WIFI_MODULE),AP6234)
WIFI_DRIVER := AP6234
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/dhd.ko
WIFI_DRIVER_MODULE_NAME := dhd
WIFI_DRIVER_MODULE_ARG  := "firmware_path=/vendor/etc/wifi/6234/fw_bcm43341b0_ag.bin nvram_path=/vendor/etc/wifi/6234/nvram.txt"
WIFI_DRIVER_FW_PATH_STA :=/vendor/etc/wifi/6234/fw_bcm43341b0_ag.bin
WIFI_DRIVER_FW_PATH_AP  :=/vendor/etc/wifi/6234/fw_bcm43341b0_ag_apsta.bin
WIFI_DRIVER_FW_PATH_P2P :=/vendor/etc/wifi/6234/fw_bcm43341b0_ag_p2p.bin

BOARD_WLAN_DEVICE := bcmdhd
LIB_WIFI_HAL := libwifi-hal-bcm
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd_ampak
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd_ampak
PRODUCT_PACKAGES += \
	6234/nvram.txt    \
	6234/fw_bcm43341b0_ag.bin \
	6234/fw_bcm43341b0_ag_apsta.bin \
	6234/fw_bcm43341b0_ag_p2p.bin \
	p2p_supplicant_overlay.conf \
	wl \
	dhd

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml

PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

ifneq ($(wildcard $(TARGET_PRODUCT_DIR)/dhd.ko),)
PRODUCT_COPY_FILES += $(TARGET_PRODUCT_DIR)/dhd.ko:$(PRODUCT_OUT)/obj/lib_vendor/dhd.ko
endif

PRODUCT_PROPERTY_OVERRIDES += \
	wifi.interface=wlan0
endif

################################################################################## AP6212
ifeq ($(WIFI_MODULE),AP6212)
WIFI_DRIVER := AP6212
ifneq ($(WIFI_BUILD_IN), true)
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/dhd.ko
WIFI_DRIVER_MODULE_NAME := dhd
WIFI_DRIVER_MODULE_ARG  := "firmware_path=/vendor/etc/wifi/6212/fw_bcm43438a0.bin nvram_path=/vendor/etc/wifi/6212/nvram.txt"
endif
WIFI_DRIVER_FW_PATH_STA := /vendor/etc/wifi/6212/fw_bcm43438a0.bin
WIFI_DRIVER_FW_PATH_AP  := /vendor/etc/wifi/6212/fw_bcm43438a0_apsta.bin
WIFI_DRIVER_FW_PATH_P2P := /vendor/etc/wifi/6212/fw_bcm43438a0_p2p.bin

ifneq ($(WIFI_BUILD_IN), true)
BOARD_WLAN_DEVICE := bcmdhd
else
BOARD_WLAN_DEVICE := MediaTek
endif
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd_ampak
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd_ampak
PRODUCT_PACKAGES += \
	6212/nvram.txt    \
	6212/fw_bcm43438a0.bin \
	6212/fw_bcm43438a0_apsta.bin \
	6212/fw_bcm43438a0_p2p.bin \
	wl \
	p2p_supplicant_overlay.conf \
	dhd

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml
ifeq ($(WIFI_BUILD_IN), true)
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi_buildin.rc
endif
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

ifneq ($(wildcard $(TARGET_PRODUCT_DIR)/dhd.ko),)
PRODUCT_COPY_FILES += $(TARGET_PRODUCT_DIR)/dhd.ko:$(PRODUCT_OUT)/obj/lib_vendor/dhd.ko
endif

PRODUCT_PROPERTY_OVERRIDES += \
	wifi.interface=wlan0 \
	wifi.direct.interface=p2p-dev-wlan0
endif

################################################################################## AP6255
ifeq ($(WIFI_MODULE),AP6255)
WIFI_DRIVER := AP6255
ifneq ($(WIFI_BUILD_IN), true)
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/dhd.ko
WIFI_DRIVER_MODULE_NAME := dhd
WIFI_DRIVER_MODULE_ARG  := "firmware_path=/vendor/etc/wifi/6255/fw_bcm43455c0_ag.bin nvram_path=/vendor/etc/wifi/6255/nvram.txt"
endif
WIFI_DRIVER_FW_PATH_STA := /vendor/etc/wifi/6255/fw_bcm43455c0_ag.bin
WIFI_DRIVER_FW_PATH_AP  := /vendor/etc/wifi/6255/fw_bcm43455c0_ag_apsta.bin
WIFI_DRIVER_FW_PATH_P2P := /vendor/etc/wifi/6255/fw_bcm43455c0_ag_p2p.bin

ifneq ($(WIFI_BUILD_IN), true)
BOARD_WLAN_DEVICE := bcmdhd
else
BOARD_WLAN_DEVICE := MediaTek
endif

WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd_ampak
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd_ampak
PRODUCT_PACKAGES += \
	6255/nvram.txt    \
	6255/fw_bcm43455c0_ag.bin \
	6255/fw_bcm43455c0_ag_apsta.bin \
	6255/fw_bcm43455c0_ag_p2p.bin \
	wl \
	p2p_supplicant_overlay.conf \
	dhd

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml
ifeq ($(WIFI_BUILD_IN), true)
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi_buildin.rc
endif
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

ifneq ($(wildcard $(TARGET_PRODUCT_DIR)/dhd.ko),)
PRODUCT_COPY_FILES += $(TARGET_PRODUCT_DIR)/dhd.ko:$(PRODUCT_OUT)/obj/lib_vendor/dhd.ko
endif

PRODUCT_PROPERTY_OVERRIDES += \
	wifi.interface=wlan0 \
	wifi.direct.interface=p2p-dev-wlan0
endif


################################################################################## bcm43341
ifeq ($(WIFI_MODULE),bcm43341)
WIFI_DRIVER := bcm43341
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/bcmdhd.ko
WIFI_DRIVER_MODULE_NAME := bcmdhd
WIFI_DRIVER_MODULE_ARG  := "iface_name=wlan0 firmware_path=/vendor/etc/wifi/fw_bcmdhd_43341.bin nvram_path=/vendor/etc/wifi/nvram_43341.bin"
WIFI_DRIVER_FW_PATH_STA :=/vendor/etc/wifi/fw_bcmdhd_43341.bin
WIFI_DRIVER_FW_PATH_AP  :=/vendor/etc/wifi/fw_bcmdhd_43341.bin
WIFI_DRIVER_FW_PATH_P2P :=/vendor/etc/wifi/fw_bcmdhd_43341.bin

BOARD_WLAN_DEVICE := bcmdhd
LIB_WIFI_HAL := libwifi-hal-bcm
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/bcmdhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd_usi
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd_usi
PRODUCT_PACKAGES += \
	nvram_43341.bin   \
	fw_bcmdhd_43341.bin \
	wl \
	p2p_supplicant_overlay.conf \
	dhd

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml

PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

ifneq ($(wildcard $(TARGET_PRODUCT_DIR)/bcmdhd.ko),)
PRODUCT_COPY_FILES += $(TARGET_PRODUCT_DIR)/bcmdhd.ko:$(PRODUCT_OUT)/obj/lib_vendor/bcmdhd.ko
endif

PRODUCT_PROPERTY_OVERRIDES += \
	wifi.interface=wlan0

endif
################################################################################## bcm43241
ifeq ($(WIFI_MODULE),bcm43241)
WIFI_DRIVER := bcm43241
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/bcmdhd.ko
WIFI_DRIVER_MODULE_NAME := bcmdhd
WIFI_DRIVER_MODULE_ARG  := "iface_name=wlan0 firmware_path=/vendor/etc/wifi/fw_bcmdhd_43241.bin nvram_path=/vendor/etc/wifi/nvram_43241.bin"
WIFI_DRIVER_FW_PATH_STA :=/vendor/etc/wifi/fw_bcmdhd_43241.bin
WIFI_DRIVER_FW_PATH_AP  :=/vendor/etc/wifi/fw_bcmdhd_43241.bin
WIFI_DRIVER_FW_PATH_P2P :=/vendor/etc/wifi/fw_bcmdhd_43241.bin

BOARD_WLAN_DEVICE := bcmdhd
LIB_WIFI_HAL := libwifi-hal-bcm
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/bcmdhd/parameters/firmware_path"

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd_usi
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd_usi
PRODUCT_PACKAGES += \
	nvram_43241.bin   \
	fw_bcmdhd_43241.bin \
	wl \
	p2p_supplicant_overlay.conf \
	dhd

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml

PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_bcm.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

ifneq ($(wildcard $(TARGET_PRODUCT_DIR)/bcmdhd.ko),)
PRODUCT_COPY_FILES += $(TARGET_PRODUCT_DIR)/bcmdhd.ko:$(PRODUCT_OUT)/obj/lib_vendor/bcmdhd.ko
endif

PRODUCT_PROPERTY_OVERRIDES += \
        wifi.interface=wlan0

endif
################################################################################## rt5370
ifeq ($(WIFI_MODULE),rt5370)

WIFI_DRIVER             := rt5370
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/rt5370sta.ko
WIFI_DRIVER_MODULE_NAME := rt5370sta

WPA_SUPPLICANT_VERSION  := VER_0_8_X

BOARD_WPA_SUPPLICANT_PRIVATE_LIB  := lib_driver_cmd_nl80211
BOARD_WPA_SUPPLICANT_DRIVER       := NL80211

LIB_WIFI_HAL := libwifi-hal-rtl

ifneq ($(wildcard $(TARGET_PRODUCT_DIR)/rt5370sta.ko),)
PRODUCT_COPY_FILES += $(TARGET_PRODUCT_DIR)/rt5370sta.ko:$(PRODUCT_OUT)/obj/lib_vendor/rt5370sta.ko
endif

PRODUCT_PROPERTY_OVERRIDES += \
	wifi.interface=wlan0

PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_rtk.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc
endif

################################################################################## mt5931
ifeq ($(WIFI_MODULE),mt5931)

MTK_WLAN_SUPPORT        := true
WIFI_DRIVER             := mt5931
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/wlan.ko
WIFI_DRIVER_MODULE_NAME := wlan
P2P_SUPPLICANT_VERSION  := VER_0_8_X_MTK
BOARD_P2P_SUPPLICANT_DRIVER       := NL80211

LIB_WIFI_HAL := libwifi-hal-rtl

PRODUCT_PACKAGES += \
	p2p_supplicant.conf

ifneq ($(wildcard $(TARGET_PRODUCT_DIR)/wlan.ko),)
PRODUCT_COPY_FILES += $(TARGET_PRODUCT_DIR)/wlan.ko:$(PRODUCT_OUT)/obj/lib_vendor/wlan.ko
endif

PRODUCT_COPY_FILES += hardware/amlogic/wifi/mt5931/WIFI_RAM_CODE:$(TARGET_COPY_OUT_VENDOR)/etc/firmware/WIFI_RAM_CODE

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml

PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_rtk.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

PRODUCT_PROPERTY_OVERRIDES += \
	wifi.interface=wlan0

endif


################################################################################## qualcomm wifi
ifneq ($(filter qca6174 qca9377 qca9379,$(WIFI_MODULE)),)
WIFI_KO := $(patsubst qca%,%,$(WIFI_MODULE))
WIFI_DRIVER             := $(WIFI_MODULE)
BOARD_WIFI_VENDOR       := qualcomm
ifneq ($(WIFI_BUILD_IN), true)
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/wlan_$(WIFI_KO).ko
WIFI_DRIVER_MODULE_NAME := wlan
WIFI_DRIVER_MODULE_ARG  := "country_code=CN"
$(warning WIFI_DRIVER_MODULE_PATH is $(WIFI_DRIVER_MODULE_PATH))
$(warning WIFI_DRIVER_MODULE_NAME is $(WIFI_DRIVER_MODULE_NAME))
$(warning WIFI_DRIVER_MODULE_ARG  is $(WIFI_DRIVER_MODULE_ARG))
endif

WPA_SUPPLICANT_VERSION           := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER      := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_qcom
BOARD_HOSTAPD_DRIVER             := NL80211
BOARD_HOSTAPD_PRIVATE_LIB        := lib_driver_cmd_qcom

ifneq ($(WIFI_BUILD_IN), true)
BOARD_WLAN_DEVICE := $(WIFI_MODULE)
else
BOARD_WLAN_DEVICE := MediaTek
endif

WIFI_FIRMWARE_LOADER      := ""
WIFI_DRIVER_FW_PATH_PARAM := ""

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml
ifeq ($(WIFI_BUILD_IN), true)
$(shell rm hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc)
$(shell touch hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc)
$(shell echo "on boot" > hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc)
$(shell sed -i "1a\    insmod \/vendor\/lib/modules\/wlan_$(WIFI_KO).ko country_code=CN" hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc)
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi_buildin.rc
endif
PRODUCT_COPY_FILES += hardware/amlogic/wifi/qcom/config/wpa_supplicant_overlay.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/wpa_supplicant_overlay.conf
PRODUCT_COPY_FILES += hardware/amlogic/wifi/qcom/config/p2p_supplicant_overlay.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/p2p_supplicant_overlay.conf
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_rtk.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc

PRODUCT_COPY_FILES += \
    hardware/amlogic/wifi/qcom/config/$(WIFI_MODULE)/wifi/bdwlan30.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/$(WIFI_MODULE)/bdwlan30.bin \
    hardware/amlogic/wifi/qcom/config/$(WIFI_MODULE)/wifi/otp30.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/$(WIFI_MODULE)/otp30.bin \
    hardware/amlogic/wifi/qcom/config/$(WIFI_MODULE)/wifi/qwlan30.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/$(WIFI_MODULE)/qwlan30.bin \
    hardware/amlogic/wifi/qcom/config/$(WIFI_MODULE)/wifi/utf30.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/$(WIFI_MODULE)/utf30.bin \
    hardware/amlogic/wifi/qcom/config/$(WIFI_MODULE)/wifi/wlan/cfg.dat:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/$(WIFI_MODULE)/wlan/cfg.dat \
    hardware/amlogic/wifi/qcom/config/$(WIFI_MODULE)/wifi/wlan/qcom_cfg.ini:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/$(WIFI_MODULE)/wlan/qcom_cfg.ini

PRODUCT_PROPERTY_OVERRIDES += wifi.interface=wlan0

endif

################################################################################## mtk wifi
ifneq ($(filter mt76x8_usb mt76x8_sdio,$(WIFI_MODULE)),)
WIFI_KO := wlan_$(WIFI_MODULE)
WIFI_DRIVER             := $(WIFI_MODULE)
BOARD_WIFI_VENDOR       := mtk
ifneq ($(WIFI_BUILD_IN), true)
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/$(WIFI_KO).ko
WIFI_DRIVER_MODULE_NAME := $(WIFI_KO)
WIFI_DRIVER_MODULE_ARG  := ""
$(warning WIFI_DRIVER_MODULE_PATH is $(WIFI_DRIVER_MODULE_PATH))
$(warning WIFI_DRIVER_MODULE_NAME is $(WIFI_DRIVER_MODULE_NAME))
$(warning WIFI_DRIVER_MODULE_ARG  is $(WIFI_DRIVER_MODULE_ARG))
endif

WPA_SUPPLICANT_VERSION           := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER      := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_mtk
BOARD_HOSTAPD_DRIVER             := NL80211
BOARD_HOSTAPD_PRIVATE_LIB        := lib_driver_cmd_mtk

ifneq ($(WIFI_BUILD_IN), true)
BOARD_WLAN_DEVICE := $(WIFI_MODULE)
else
BOARD_WLAN_DEVICE := MediaTek
endif

WIFI_FIRMWARE_LOADER      := ""
WIFI_DRIVER_FW_PATH_PARAM := ""

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml
ifeq ($(WIFI_BUILD_IN), true)
$(shell rm hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc)
$(shell touch hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc)
$(shell echo "on boot" > hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc)
$(shell sed -i "1a\    insmod \/vendor\/lib/modules\/$(WIFI_KO).ko" hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc)
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi_buildin.rc
endif
PRODUCT_COPY_FILES += hardware/amlogic/wifi/multi_wifi/config/p2p_supplicant_overlay.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/p2p_supplicant_overlay.conf
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_rtk.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc
PRODUCT_COPY_FILES += \
       hardware/wifi/mtk/drivers/mt7668/7668_firmware/EEPROM_MT7668.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/EEPROM_MT7668.bin \
       hardware/wifi/mtk/drivers/mt7668/7668_firmware/mt7668_patch_e1_hdr.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/mt7668_patch_e1_hdr.bin \
       hardware/wifi/mtk/drivers/mt7668/7668_firmware/TxPwrLimit_MT76x8.dat:$(TARGET_COPY_OUT_VENDOR)/firmware/TxPwrLimit_MT76x8.dat \
       hardware/wifi/mtk/drivers/mt7668/7668_firmware/wifi.cfg:$(TARGET_COPY_OUT_VENDOR)/firmware/wifi.cfg \
       hardware/wifi/mtk/drivers/mt7668/7668_firmware/WIFI_RAM_CODE2_USB_MT7668.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/WIFI_RAM_CODE2_USB_MT7668.bin \
       hardware/wifi/mtk/drivers/mt7668/7668_firmware/WIFI_RAM_CODE2_SDIO_MT7668.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/WIFI_RAM_CODE2_SDIO_MT7668.bin \
       hardware/wifi/mtk/drivers/mt7668/7668_firmware/WIFI_RAM_CODE_MT7668.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/WIFI_RAM_CODE_MT7668.bin \
       hardware/wifi/mtk/drivers/mt7668/7668_firmware/mt7668_patch_e2_hdr.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/mt7668_patch_e2_hdr.bin
PRODUCT_PACKAGES += \
        wpa_supplicant_overlay.conf \
        p2p_supplicant_overlay.conf

# 89976: Add Realtek USB WiFi support
PRODUCT_PROPERTY_OVERRIDES += \
        wifi.interface=wlan0 \
        wifi.direct.interface=p2p0

endif

################################################################################## mtk wifi
ifneq ($(filter mt7601u mt7603u,$(WIFI_MODULE)),)
WIFI_KO := $(WIFI_MODULE)sta
WIFI_DRIVER             := $(WIFI_MODULE)
BOARD_WIFI_VENDOR       := mtk
ifneq ($(WIFI_BUILD_IN), true)
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/$(WIFI_KO).ko
WIFI_DRIVER_MODULE_NAME := $(WIFI_KO)
WIFI_DRIVER_MODULE_ARG  := ""
$(warning WIFI_DRIVER_MODULE_PATH is $(WIFI_DRIVER_MODULE_PATH))
$(warning WIFI_DRIVER_MODULE_NAME is $(WIFI_DRIVER_MODULE_NAME))
$(warning WIFI_DRIVER_MODULE_ARG  is $(WIFI_DRIVER_MODULE_ARG))
endif

WPA_SUPPLICANT_VERSION           := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER      := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_mtk
BOARD_HOSTAPD_DRIVER             := NL80211
BOARD_HOSTAPD_PRIVATE_LIB        := lib_driver_cmd_mtk

ifneq ($(WIFI_BUILD_IN), true)
BOARD_WLAN_DEVICE := $(WIFI_MODULE)
else
BOARD_WLAN_DEVICE := MediaTek
endif

WIFI_FIRMWARE_LOADER      := ""
WIFI_DRIVER_FW_PATH_PARAM := ""

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml
ifeq ($(WIFI_BUILD_IN), true)
$(shell rm hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc)
$(shell touch hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc)
$(shell echo "on boot" > hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc)
$(shell sed -i "1a\    insmod \/vendor\/lib/modules\/$(WIFI_KO).ko" hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc)
$(shell sed -i "1a\    insmod \/vendor\/lib/modules\/mtprealloc.ko" hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc)
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_$(WIFI_MODULE).rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi_buildin.rc
endif
PRODUCT_COPY_FILES += hardware/amlogic/wifi/multi_wifi/config/p2p_supplicant_overlay.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/p2p_supplicant_overlay.conf
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_mtk.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc
PRODUCT_COPY_FILES += hardware/wifi/mtk/drivers/mt7601/MT7601USTA.dat:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/MT7601USTA.dat
PRODUCT_COPY_FILES += hardware/amlogic/wifi/mediatek/RT2870STA_7601.dat:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/RT2870STA_7603.dat
PRODUCT_PACKAGES += \
        wpa_supplicant_overlay.conf \
        p2p_supplicant_overlay.conf

# 89976: Add Realtek USB WiFi support
PRODUCT_PROPERTY_OVERRIDES += \
        wifi.interface=wlan0 \
        wifi.direct.interface=p2p0

endif

################################################################################## AP6xxx
ifeq ($(WIFI_AP6xxx_MODULE),AP6181)

PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6181/Wi-Fi/fw_bcm40181a2.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40181/fw_bcm40181a2.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6181/Wi-Fi/fw_bcm40181a2_apsta.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40181/fw_bcm40181a2_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6181/Wi-Fi/fw_bcm40181a2_p2p.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40181/fw_bcm40181a2_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6181/Wi-Fi/nvram_ap6181.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40181/nvram.txt

endif

ifeq ($(WIFI_AP6xxx_MODULE),AP6210)

PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6210/Wi-Fi/fw_bcm40181a2.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40181/fw_bcm40181a2.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6210/Wi-Fi/fw_bcm40181a2_apsta.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40181/fw_bcm40181a2_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6210/Wi-Fi/fw_bcm40181a2_p2p.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40181/fw_bcm40181a2_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6210/Wi-Fi/nvram_ap6210.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40181/nvram.txt

endif

ifeq ($(WIFI_AP6xxx_MODULE),AP6476)

PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6476/Wi-Fi/fw_bcm40181a2.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40181/fw_bcm40181a2.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6476/Wi-Fi/fw_bcm40181a2_apsta.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40181/fw_bcm40181a2_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6476/Wi-Fi/fw_bcm40181a2_p2p.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40181/fw_bcm40181a2_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6476/Wi-Fi/nvram_ap6476.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40181/nvram.txt

endif

ifeq ($(WIFI_AP6xxx_MODULE),AP6493)

PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6493/Wi-Fi/fw_bcm40183b2.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40183/fw_bcm40183b2.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6493/Wi-Fi/fw_bcm40183b2_apsta.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40183/fw_bcm40183b2_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6493/Wi-Fi/fw_bcm40183b2_p2p.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40183/fw_bcm40183b2_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6493/Wi-Fi/nvram_ap6493.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40183/nvram.txt

endif

ifeq ($(WIFI_AP6xxx_MODULE),AP6330)

PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6330/Wi-Fi/fw_bcm40183b2.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40183/fw_bcm40183b2.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6330/Wi-Fi/fw_bcm40183b2_apsta.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40183/fw_bcm40183b2_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6330/Wi-Fi/fw_bcm40183b2_p2p.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40183/fw_bcm40183b2_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6330/Wi-Fi/nvram_ap6330.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/40183/nvram.txt

endif

################################################################################## multiwifi
ifeq ($(WIFI_MODULE), multiwifi)

MULTI_WIFI_SUPPORT := true
WIFI_DRIVER_MODULE_PATH := /vendor/lib/modules/
WIFI_DRIVER_MODULE_NAME := dhd
BOARD_WLAN_DEVICE := MediaTek
WPA_SUPPLICANT_VERSION			:= VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER	:= NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_multi
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_multi
BOARD_HOSTAPD_DRIVER				:= NL80211

WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"
PRODUCT_COPY_FILES += \
        frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml
PRODUCT_PROPERTY_OVERRIDES += \
        wifi.interface=wlan0 \
	wifi.direct.interface=p2p0

PRODUCT_PACKAGES += \
    bcmdl \
	wpa_cli

PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi_buildin_rtk.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi_buildin.rc
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6212/fw_bcm43438a0.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6212/fw_bcm43438a0.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6212/fw_bcm43438a0_apsta.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6212/fw_bcm43438a0_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6212/fw_bcm43438a0_p2p.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6212/fw_bcm43438a0_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6212/nvram.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6212/nvram.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/config.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6212/config.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/62x2/fw_bcm43241b4_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/62x2/fw_bcm43241b4_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/62x2/fw_bcm43241b4_ag_apsta.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/62x2/fw_bcm43241b4_ag_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/62x2/fw_bcm43241b4_ag_p2p.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/62x2/fw_bcm43241b4_ag_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/62x2/nvram.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/62x2/nvram.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/config.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/62x2/config.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6255/fw_bcm43455c0_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6255/fw_bcm43455c0_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6255/fw_bcm43455c0_ag_apsta.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6255/fw_bcm43455c0_ag_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6255/fw_bcm43455c0_ag_p2p.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6255/fw_bcm43455c0_ag_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6255/nvram.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6255/nvram_ap6255.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/config.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6255/config.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6335/fw_bcm4339a0_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6335/fw_bcm4339a0_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6335/fw_bcm4339a0_ag_apsta.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6335/fw_bcm4339a0_ag_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6335/fw_bcm4339a0_ag_p2p.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6335/fw_bcm4339a0_ag_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6335/nvram.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6335/nvram_ap6335.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6335/config.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6335/config.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4356/fw_bcm4356a2_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4356/fw_bcm4356a2_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4356/fw_bcm4356a2_ag_apsta.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4356/fw_bcm4356a2_ag_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4356/fw_bcm4356a2_ag_p2p.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4356/fw_bcm4356a2_ag_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4356/nvram_ap6356.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4356/nvram_ap6356.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4356/config.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4356/config.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4354/fw_bcm4354a1_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4354/fw_bcm4354a1_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4354/fw_bcm4354a1_ag_apsta.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4354/fw_bcm4354a1_ag_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4354/fw_bcm4354a1_ag_p2p.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4354/fw_bcm4354a1_ag_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4354/nvram_ap6354.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4354/nvram_ap6354.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4354/config.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4354/config.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/43458/fw_bcm43455c0_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43458/fw_bcm43455c0_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/43458/fw_bcm43455c0_ag_apsta.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43458/fw_bcm43455c0_ag_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/43458/fw_bcm43455c0_ag_p2p.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43458/fw_bcm43455c0_ag_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/43458/nvram_43458.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43458/nvram_43458.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/43458/config.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43458/config.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4358/fw_bcm4358_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4358/fw_bcm4358_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4358/fw_bcm4358_ag_apsta.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4358/fw_bcm4358_ag_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4358/fw_bcm4358_ag_p2p.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4358/fw_bcm4358_ag_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4358/nvram_4358.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4358/nvram_4358.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4358/config.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4358/config.txt
ifeq ($(BCM_USB_COMPOSITE),true)
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/USB_COMPOSITE/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/fw_bcm4358u_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/USB_COMPOSITE/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/fw_bcm4358u_ag_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/USB_COMPOSITE/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/fw_bcm4358u_ag_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/USB_COMPOSITE/nvram_ap62x8.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/nvram_ap62x8.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/USB_COMPOSITE/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/fw_bcm43569a2_ag.bin.trx
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/USB_COMPOSITE/nvram_ap62x8.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/nvram_ap6269a2.nvm
else
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/fw_bcm4358u_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/fw_bcm4358u_ag_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/fw_bcm4358u_ag_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/nvram_ap62x8.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/nvram_ap62x8.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/fw_bcm4358u_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/fw_bcm43569a2_ag.bin.trx
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/nvram_ap62x8.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/43569/nvram_ap6269a2.nvm
endif
PRODUCT_COPY_FILES += hardware/amlogic/wifi/configs/init_rc/init.amlogic.wifi.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.wifi.rc
PRODUCT_COPY_FILES += hardware/amlogic/wifi/multi_wifi/config/bcm_supplicant.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/bcm_supplicant.conf
PRODUCT_COPY_FILES += hardware/amlogic/wifi/multi_wifi/config/bcm_supplicant_overlay.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/bcm_supplicant_overlay.conf
PRODUCT_COPY_FILES += hardware/amlogic/wifi/multi_wifi/config/wpa_supplicant.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/wpa_supplicant.conf
PRODUCT_COPY_FILES += hardware/amlogic/wifi/multi_wifi/config/wpa_supplicant.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/p2p_supplicant.conf
PRODUCT_COPY_FILES += hardware/amlogic/wifi/multi_wifi/config/wpa_supplicant_overlay.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/wpa_supplicant_overlay.conf
PRODUCT_COPY_FILES += hardware/amlogic/wifi/multi_wifi/config/p2p_supplicant_overlay.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/p2p_supplicant_overlay.conf
PRODUCT_COPY_FILES += hardware/amlogic/wifi/mediatek/iwpriv:$(TARGET_COPY_OUT_VENDOR)/bin/iwpriv
PRODUCT_COPY_FILES += hardware/amlogic/wifi/mediatek/RT2870STA_7601.dat:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/RT2870STA_7603.dat
PRODUCT_COPY_FILES += hardware/amlogic/wifi/mediatek/dhcpcd.conf:$(TARGET_COPY_OUT_VENDOR)/etc/dhcpcd/dhcpcd.conf
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6398/Wi-Fi/fw_bcm4359c0_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4359/fw_bcm4359c0_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6398/Wi-Fi/fw_bcm4359c0_ag_apsta.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4359/fw_bcm4359c0_ag_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6398/Wi-Fi/fw_bcm4359c0_ag_p2p.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4359/fw_bcm4359c0_ag_p2p.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6398/Wi-Fi/nvram_ap6398s.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/4359/nvram_ap6398s.txt
PRODUCT_COPY_FILES += hardware/wifi/mtk/drivers/mt7601/MT7601USTA.dat:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/MT7601USTA.dat
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6271S/Wi-Fi/clm_bcm43751a1_ag.blob:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/AP6271/clm_bcm43751a1_ag.blob
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6271S/Wi-Fi/fw_bcm43751a1_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/AP6271/fw_bcm43751a1_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6271S/Wi-Fi/fw_bcm43751a1_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/AP6271/fw_bcm43751a1_ag_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6271S/Wi-Fi/nvram_ap6271s.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/AP6271/nvram_ap6271s.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6236/Wi-Fi/fw_bcm43436b0.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6212/fw_bcm43436b0.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6236/Wi-Fi/nvram_ap6236.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6212/nvram_ap6236.txt
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6236/Wi-Fi/fw_bcm43436b0_apsta.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6212/fw_bcm43436b0_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6256/Wi-Fi/fw_bcm43456c5_ag.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6255/fw_bcm43456c5_ag.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6256/Wi-Fi/fw_bcm43456c5_ag_apsta.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6255/fw_bcm43456c5_ag_apsta.bin
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6256/Wi-Fi/nvram_ap6256.txt:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/6255/nvram_ap6256.txt
PRODUCT_COPY_FILES += hardware/wifi/atbm/atbm602x/firmware/fw_usb.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/fw_usb.bin
PRODUCT_COPY_FILES += hardware/wifi/icomm/drivers/ssv6xxx/ssv6x5x/image/ssv6x5x-sw.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/ssv6x5x/ssv6x5x-sw.bin
PRODUCT_COPY_FILES += hardware/wifi/icomm/drivers/ssv6xxx/ssv6x5x/image/ssv6x5x-wifi.cfg:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/ssv6x5x/ssv6x5x-wifi.cfg
PRODUCT_COPY_FILES += hardware/wifi/icomm/drivers/ssv6xxx/ssv6051/image/ssv6051-sw.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/ssv6051/ssv6051-sw.bin
PRODUCT_COPY_FILES += hardware/wifi/icomm/drivers/ssv6xxx/ssv6051/ssv6051-wifi.cfg:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/ssv6051/ssv6051-wifi.cfg
PRODUCT_COPY_FILES += \
       hardware/wifi/mtk/drivers/mt7668/7668_firmware/EEPROM_MT7668.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/EEPROM_MT7668.bin \
       hardware/wifi/mtk/drivers/mt7668/7668_firmware/mt7668_patch_e1_hdr.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/mt7668_patch_e1_hdr.bin \
       hardware/wifi/mtk/drivers/mt7668/7668_firmware/TxPwrLimit_MT76x8.dat:$(TARGET_COPY_OUT_VENDOR)/firmware/TxPwrLimit_MT76x8.dat \
       hardware/wifi/mtk/drivers/mt7668/7668_firmware/wifi.cfg:$(TARGET_COPY_OUT_VENDOR)/firmware/wifi.cfg \
       hardware/wifi/mtk/drivers/mt7668/7668_firmware/WIFI_RAM_CODE2_USB_MT7668.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/WIFI_RAM_CODE2_USB_MT7668.bin \
       hardware/wifi/mtk/drivers/mt7668/7668_firmware/WIFI_RAM_CODE2_SDIO_MT7668.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/WIFI_RAM_CODE2_SDIO_MT7668.bin \
       hardware/wifi/mtk/drivers/mt7668/7668_firmware/WIFI_RAM_CODE_MT7668.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/WIFI_RAM_CODE_MT7668.bin \
       hardware/wifi/mtk/drivers/mt7668/7668_firmware/mt7668_patch_e2_hdr.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/mt7668_patch_e2_hdr.bin
PRODUCT_COPY_FILES += \
    hardware/amlogic/wifi/qcom/config/qca9377/wifi/bdwlan30.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca9377/bdwlan30.bin \
    hardware/amlogic/wifi/qcom/config/qca9377/wifi/otp30.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca9377/otp30.bin \
    hardware/amlogic/wifi/qcom/config/qca9377/wifi/qwlan30.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca9377/qwlan30.bin \
    hardware/amlogic/wifi/qcom/config/qca9377/wifi/utf30.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca9377/utf30.bin \
    hardware/amlogic/wifi/qcom/config/qca9377/wifi/wlan/cfg.dat:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca9377/wlan/cfg.dat \
    hardware/amlogic/wifi/qcom/config/qca9377/wifi/wlan/qcom_cfg.ini:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca9377/wlan/qcom_cfg.ini \
    hardware/amlogic/wifi/qcom/config/qca9377/wifi/wlan/qcom_wlan_nv.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca9377/wlan/qcom_wlan_nv.bin \
    hardware/amlogic/wifi/qcom/config/qca9379/wifi/athwlan.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca9379/athwlan.bin \
    hardware/amlogic/wifi/qcom/config/qca9379/wifi/otp.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca9379/otp.bin \
    hardware/amlogic/wifi/qcom/config/qca9379/wifi/fakeboar.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca9379/fakeboar.bin \
    hardware/amlogic/wifi/qcom/config/qca9379/wifi/utf.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca9379/utf.bin \
    hardware/amlogic/wifi/qcom/config/qca9379/wifi/wlan/cfg.dat:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca9379/wlan/cfg.dat \
    hardware/amlogic/wifi/qcom/config/qca9379/wifi/wlan/qcom_cfg.ini:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca9379/wlan/qcom_cfg.ini \
    hardware/amlogic/wifi/qcom/config/qca6174/wifi/bdwlan30.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca6174/bdwlan30.bin \
    hardware/amlogic/wifi/qcom/config/qca6174/wifi/athwlan.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca6174/athwlan.bin \
    hardware/amlogic/wifi/qcom/config/qca6174/wifi/otp30.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca6174/otp30.bin \
    hardware/amlogic/wifi/qcom/config/qca6174/wifi/utf30.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca6174/utf30.bin \
    hardware/amlogic/wifi/qcom/config/qca6174/wifi/qwlan30.bin:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca6174/qwlan30.bin \
    hardware/amlogic/wifi/qcom/config/qca6174/wifi/wlan/cfg.dat:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca6174/wlan/cfg.dat \
    hardware/amlogic/wifi/qcom/config/qca6174/wifi/wlan/qcom_cfg.ini:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca6174/wlan/qcom_cfg.ini \
    hardware/amlogic/wifi/qcom/config/qca6174/wifi/wlan/qcom_cfg.ini:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/qca6174/wlan/qcom_cfg.ini.ok
endif
