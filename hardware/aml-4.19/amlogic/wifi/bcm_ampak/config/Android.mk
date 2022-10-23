LOCAL_PATH:=$(call my-dir)

###################################################
#       wpa_supplicant_overlay.conf
###################################################

include $(CLEAR_VARS)
LOCAL_MODULE       := p2p_supplicant_overlay.conf
LOCAL_MODULE_TAGS  := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif

include $(BUILD_PREBUILT)

###################################################
#	40181
###################################################

ifeq ($(strip $(WIFI_DRIVER)),bcm40181)
include $(CLEAR_VARS)
LOCAL_MODULE := 40181/nvram.txt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 40181/fw_bcm40181a0.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 40181/fw_bcm40181a0_apsta.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 40181/fw_bcm40181a2.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 40181/fw_bcm40181a2_apsta.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 40181/fw_bcm40181a2_p2p.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

endif
###################################################
#	40183
###################################################
ifeq ($(strip $(WIFI_DRIVER)),bcm40183)
include $(CLEAR_VARS)
LOCAL_MODULE := 40183/nvram.txt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 40183/fw_bcm40183b2.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE :=40183/fw_bcm40183b2_apsta.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 40183/fw_bcm40183b2_p2p.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

endif

###################################################
#       43458
###################################################
ifeq ($(strip $(WIFI_DRIVER)),bcm43458)
include $(CLEAR_VARS)
LOCAL_MODULE := 43458/nvram_43458.txt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 43458/fw_bcm43455c0_ag.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 43458/fw_bcm43455c0_ag_apsta.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 43458/fw_bcm43455c0_ag_p2p.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)


endif


###################################################
#       4356
###################################################
ifeq ($(strip $(WIFI_DRIVER)),bcm4356)
include $(CLEAR_VARS)
LOCAL_MODULE := 4356/nvram_ap6356.txt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 4356/fw_bcm4356a2_ag.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 4356/fw_bcm4356a2_ag_apsta.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 4356/fw_bcm4356a2_ag_p2p.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

endif



###################################################
#       4358
###################################################
ifeq ($(strip $(WIFI_DRIVER)),bcm4358)
include $(CLEAR_VARS)
LOCAL_MODULE := 4358/nvram_4358.txt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 4358/fw_bcm4358_ag.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 4358/fw_bcm4358_ag_apsta.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 4358/fw_bcm4358_ag_p2p.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

endif


 ###################################################
#	4354
###################################################
ifeq ($(strip $(WIFI_DRIVER)),bcm4354)
include $(CLEAR_VARS)
LOCAL_MODULE := 4354/nvram_ap6354.txt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := 4354/fw_bcm4354a1_ag.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE :=4354/fw_bcm4354a1_ag_apsta.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 4354/fw_bcm4354a1_ag_p2p.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

endif

###################################################
#	62x2
###################################################
ifeq ($(strip $(WIFI_DRIVER)),AP62x2)
include $(CLEAR_VARS)
LOCAL_MODULE := 62x2/nvram.txt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 62x2/fw_bcm43241b4_ag.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 62x2/fw_bcm43241b4_ag_apsta.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 62x2/fw_bcm43241b4_ag_p2p.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

endif

###################################################
#	6335
###################################################
ifeq ($(strip $(WIFI_DRIVER)),AP6335)
include $(CLEAR_VARS)
LOCAL_MODULE := 6335/nvram.txt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6335/fw_bcm4339a0_ag.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6335/fw_bcm4339a0_ag_apsta.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6335/fw_bcm4339a0_ag_p2p.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6335/nvram_ap6335e.txt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6335/fw_bcm4339a0e_ag.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6335/fw_bcm4339a0e_ag_apsta.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6335/fw_bcm4339a0e_ag_p2p.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)
endif
###################################################
#	6441
###################################################
ifeq ($(strip $(WIFI_DRIVER)),AP6441)
include $(CLEAR_VARS)
LOCAL_MODULE := 6441/nvram.txt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6441/fw_bcm43341b0_ag.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6441/fw_bcm43341b0_ag_apsta.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6441/fw_bcm43341b0_ag_p2p.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

endif
###################################################
#	6234
###################################################
ifeq ($(strip $(WIFI_DRIVER)),AP6234)
include $(CLEAR_VARS)
LOCAL_MODULE := 6234/nvram.txt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6234/fw_bcm43341b0_ag.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6234/fw_bcm43341b0_ag_apsta.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6234/fw_bcm43341b0_ag_p2p.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

endif

###################################################
#	6212
###################################################
ifeq ($(strip $(WIFI_DRIVER)),AP6212)
include $(CLEAR_VARS)
LOCAL_MODULE := 6212/nvram.txt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6212/fw_bcm43438a0.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6212/fw_bcm43438a0_apsta.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6212/fw_bcm43438a0_p2p.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)
endif

###################################################
#       6255
###################################################
ifeq ($(strip $(WIFI_DRIVER)),AP6255)
include $(CLEAR_VARS)
LOCAL_MODULE := 6255/nvram.txt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6255/fw_bcm43455c0_ag.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6255/fw_bcm43455c0_ag_apsta.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 6255/fw_bcm43455c0_ag_p2p.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)
endif

###################################################
#	AP6269
###################################################
ifeq ($(strip $(WIFI_DRIVER)),AP6269)
include $(CLEAR_VARS)
LOCAL_MODULE := AP6269/nvram_ap6269a2.nvm
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := AP6269/fw_bcm43569a2_ag.bin.trx
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

endif


###################################################
#	AP6242
###################################################
ifeq ($(strip $(WIFI_DRIVER)),AP6242)
include $(CLEAR_VARS)
LOCAL_MODULE := AP6242/nvram_ap6242.nvm
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := AP6242/fw_bcm43242a1_ag.bin.trx
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

endif

###################################################
#	AP6256
###################################################
ifeq ($(strip $(WIFI_DRIVER)),AP6256)
include $(CLEAR_VARS)
LOCAL_MODULE := AP6256/nvram_ap6256.txt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := AP6256/fw_bcm43456c5_ag.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := AP6256/fw_bcm43456c5_ag_apsta.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := AP6256/fw_bcm43456c5_ag_p2p.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR)/etc/wifi
else
LOCAL_MODULE_PATH  := $(TARGET_OUT_ETC)/wifi
endif
include $(BUILD_PREBUILT)

endif
