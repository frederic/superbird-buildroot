# bcmdhd
# 1. WL_IFACE_COMB_NUM_CHANNELS must be added if Android version is 4.4 with Kernel version 3.0~3.4,
#    otherwise please remove it.

CONFIG_BCMDHD := m
CONFIG_DHD_USE_STATIC_BUF := y
CONFIG_BCMDHD_PROPTXSTATUS := y

DHDCFLAGS = -Wall -Wstrict-prototypes -Dlinux -DBCMDRIVER                \
	-DBCMDONGLEHOST -DUNRELEASEDCHIP -DBCMDMA32 -DBCMFILEIMAGE            \
	-DDHD_DEBUG -DSHOW_EVENTS -DBCMDBG                                    \
	-DWIFI_ACT_FRAME -DARP_OFFLOAD_SUPPORT                                \
	-DKEEP_ALIVE -DPKT_FILTER_SUPPORT                                     \
	-DEMBEDDED_PLATFORM -DPNO_SUPPORT                                     \
	-DOEM_ANDROID                                                         \
	-DSUPPORT_PM2_ONLY -DWL_EXT_IAPSTA                                    \
	-DTOE -DBDC -DDHD_BCMEVENTS                                           \
	-I$(src) -I$(src)/include -I$(src)/common/include

DHDCFLAGS += -DUSBOS_TX_THREAD -DBCMDHDUSB -DBCMDBUS -DBCMTRXV2          \
	-DDBUS_USB_LOOPBACK

DHDOFILES = bcmutils.o bcmwifi_channels.o bcmxtlv.o \
	dhd_linux.o dhd_linux_platdev.o dhd_linux_sched.o dhd_pno.o           \
	dhd_common.o dhd_ip.o dhd_linux_wq.o dhd_custom_gpio.o                \
	bcmevent.o linux_osl.o wldev_common.o wl_android.o                    \
	dhd_cdc.o dhd_wlfc.o                                                  \
	hnd_pktq.o hnd_pktpool.o dbus.o dbus_usb.o dbus_usb_linux.o           \
	dhd_debug.o dhd_debug_linux.o                                         \
	dhd_config.o wl_android_ext.o

ifeq ($(CONFIG_BCMDHD_PROPTXSTATUS),y)
DHDCFLAGS += -DPROP_TXSTATUS
ifneq ($(CONFIG_CFG80211),)
DHDCFLAGS += -DPROP_TXSTATUS_VSDB
endif
endif

obj-$(CONFIG_BCMDHD) += bcmdhd.o
bcmdhd-objs += $(DHDOFILES)

DHDOFILES += dhd_gpio.o
DHDCFLAGS += -DCUSTOMER_HW -DDHD_OF_SUPPORT

ifeq ($(CONFIG_DHD_USE_STATIC_BUF),y)
#obj-m += dhd_static_buf.o
DHDCFLAGS += -DSTATIC_WL_PRIV_STRUCT -DENHANCED_STATIC_BUF -DCONFIG_DHD_USE_STATIC_BUF
endif

ifneq ($(CONFIG_WIRELESS_EXT),)
DHDOFILES += wl_iw.o wl_escan.o
DHDCFLAGS += -DSOFTAP -DWL_WIRELESS_EXT -DUSE_IW -DWL_ESCAN
endif
ifneq ($(CONFIG_CFG80211),)
DHDOFILES += wl_cfg80211.o wl_cfgp2p.o wl_linux_mon.o wl_cfg_btcoex.o wl_cfgvendor.o
DHDOFILES += wl_cfgnan.o bcm_app_utils.o
DHDOFILES += dhd_cfg80211.o dhd_cfg_vendor.o
DHDCFLAGS += -DWL_CFG80211 -DWLP2P -DWL_CFG80211_STA_EVENT -DWL_ENABLE_P2P_IF
DHDCFLAGS += -DWL_IFACE_COMB_NUM_CHANNELS
DHDCFLAGS += -DCUSTOM_ROAM_TRIGGER_SETTING=-65
DHDCFLAGS += -DCUSTOM_ROAM_DELTA_SETTING=15
DHDCFLAGS += -DCUSTOM_KEEP_ALIVE_SETTING=28000
DHDCFLAGS += -DCUSTOM_PNO_EVENT_LOCK_xTIME=7
DHDCFLAGS += -DWL_SUPPORT_AUTO_CHANNEL
DHDCFLAGS += -DWL_SUPPORT_BACKPORTED_KPATCHES
DHDCFLAGS += -DESCAN_RESULT_PATCH
DHDCFLAGS += -DVSDB -DWL_CFG80211_VSDB_PRIORITIZE_SCAN_REQUEST
DHDCFLAGS += -DWLTDLS -DWL_SDO -DWL_NAN
endif
ifneq ($(CONFIG_DHD_USE_SCHED_SCAN),)
DHDCFLAGS += -DWL_SCHED_SCAN
endif
EXTRA_CFLAGS = $(DHDCFLAGS)
ifeq ($(CONFIG_BCMDHD),m)
DHDCFLAGS += -DMULTIPLE_SUPPLICANT
EXTRA_LDFLAGS += --strip-debug
endif
ARCH ?= arm64
CROSS_COMPILE ?=aarch64-linux-gnu-
KDIR ?=/mnt/fileroot/weiguang.ruan/l/out/target/product/p201/obj/KERNEL_OBJ/

dhd:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules


clean:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) clean
	$(RM) Module.markers
	$(RM) modules.order
