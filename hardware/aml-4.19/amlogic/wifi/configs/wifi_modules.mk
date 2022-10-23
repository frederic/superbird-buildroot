######################################################################################## realtek
ifneq ($(filter rtl8188eu rtl8188ftv rtl8192eu rtl8192es rtl8189es rtl8189fs \
				rtl8723bs rtl8723bu rtl8723ds rtl8723du \
				rtl88x1au rtl8812au rtl8822bu rtl8822bs,$(WIFI_MODULE)),)

WIFI_KO := $(patsubst rtl%,%,$(WIFI_MODULE))
DEFAULT_WIFI_KERNEL_MODULES := $(PRODUCT_OUT)/obj/lib_vendor/$(WIFI_KO).ko
endif

######################################################################################## qualcomm
ifneq ($(filter qca6174 qca9377 qca9379,$(WIFI_MODULE)),)
WIFI_KO := $(patsubst qca%,%,$(WIFI_MODULE))
DEFAULT_WIFI_KERNEL_MODULES := $(PRODUCT_OUT)/obj/lib_vendor/wlan_$(WIFI_KO).ko
endif

######################################################################################## bcm sdio
ifneq ($(filter bcm4354 bcm4356 bcm4358 bcm43458  bcm43341 bcm43241 bcm40183 bcm40181 \
				AP62x2 AP6335 AP6441 AP6234 AP6212 AP6398 AP6255 \
				BCMWIFI ,$(WIFI_MODULE)),)
DEFAULT_WIFI_KERNEL_MODULES := $(PRODUCT_OUT)/obj/lib_vendor/dhd.ko
endif

######################################################################################## bcm usb
ifneq ($(filter AP6269 AP62x8 AP6242,$(WIFI_MODULE)),)
DEFAULT_WIFI_KERNEL_MODULES := $(PRODUCT_OUT)/obj/lib_vendor/bcmdhd.ko
endif

######################################################################################## mtk 76x8
ifneq ($(filter mt76x8_usb mt76x8_sdio,$(WIFI_MODULE)),)
WIFI_KO := wlan_$(WIFI_MODULE)
DEFAULT_WIFI_KERNEL_MODULES := $(PRODUCT_OUT)/obj/lib_vendor/$(WIFI_KO).ko
endif

######################################################################################## mtk 7601u
ifneq ($(filter mt7601u mt7603u,$(WIFI_MODULE)),)
WIFI_KO := $(WIFI_MODULE)sta
DEFAULT_WIFI_KERNEL_MODULES := \
        $(PRODUCT_OUT)/obj/lib_vendor/$(WIFI_KO).ko \
	$(PRODUCT_OUT)/obj/lib_vendor/mtprealloc.ko \

endif

######################################################################################## multiwifi
ifeq ($(WIFI_MODULE),multiwifi)
DEFAULT_WIFI_KERNEL_MODULES := \
	$(PRODUCT_OUT)/obj/lib_vendor/dhd.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/bcmdhd.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/wlan_mt76x8_sdio.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/wlan_mt76x8_usb.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/8822bu.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/8189es.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/8189fs.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/8188eu.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/mt7601usta.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/mt7603usta.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/mtprealloc.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/8822bs.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/8822cs.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/8821cs.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/8821cu.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/8723bs.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/8723ds.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/8723bu.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/8723du.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/wlan_9377.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/wlan_9379.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/wlan_6174.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/ssv6x5x.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/ssv6051.ko \
	$(PRODUCT_OUT)/obj/lib_vendor/ssv_hwif_ctrl.ko \

endif
