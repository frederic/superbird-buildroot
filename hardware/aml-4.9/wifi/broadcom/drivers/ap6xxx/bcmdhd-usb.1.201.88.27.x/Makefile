# bcmdhd
# 1. WL_IFACE_COMB_NUM_CHANNELS must be added if Android version is 4.4 with Kernel version 3.0~3.4,
#    otherwise please remove it.

DHDCFLAGS = -Wall -Wstrict-prototypes -Dlinux -DBCMDRIVER                \
	-DBCMDONGLEHOST -DUNRELEASEDCHIP -DBCMDMA32 -DBCMFILEIMAGE            \
	-DDHD_DEBUG -DSHOW_EVENTS -DBCMDBG                                    \
	-DWIFI_ACT_FRAME -DARP_OFFLOAD_SUPPORT                                \
	-DKEEP_ALIVE -DPKT_FILTER_SUPPORT                                     \
	-DEMBEDDED_PLATFORM -DPNO_SUPPORT                                     \
	-DOEM_ANDROID                                                         \
	-DSUPPORT_PM2_ONLY                                                    \
	-DTOE -DBDC -DDHD_BCMEVENTS -DPROP_TXSTATUS                           \
	-DWL_SUPPORT_BACKPORTED_KPATCHES                                      

DHDCFLAGS += -DUSBOS_TX_THREAD -DBCMDHDUSB -DBCMDBUS -DBCMTRXV2          \
	-DDBUS_USB_LOOPBACK -DSHOW_LOGTRACE -I$(src) -I$(src)/include -I$(src)/common/include

DHDOFILES = bcmutils.o bcmwifi_channels.o bcmxtlv.o \
	dhd_linux.o dhd_linux_platdev.o dhd_linux_sched.o dhd_pno.o           \
	dhd_common.o dhd_ip.o dhd_linux_wq.o dhd_custom_gpio.o                \
	bcmevent.o linux_osl.o wldev_common.o wl_android.o                    \
	dhd_cdc.o dhd_wlfc.o                                                  \
	hnd_pktq.o hnd_pktpool.o dbus.o dbus_usb.o dbus_usb_linux.o           \
	dhd_config.o
CONFIG_BCMDHD := m
obj-$(CONFIG_BCMDHD) += bcmdhd.o
bcmdhd-objs += $(DHDOFILES)

DHDOFILES += dhd_gpio.o
DHDCFLAGS += -DCUSTOMER_HW -DDHD_OF_SUPPORT

ifeq ($(CONFIG_DHD_USE_STATIC_BUF),y)
DHDCFLAGS += -DSTATIC_WL_PRIV_STRUCT
endif

ifneq ($(CONFIG_WIRELESS_EXT),)
DHDOFILES += wl_iw.o
DHDCFLAGS += -DSOFTAP -DWL_WIRELESS_EXT -DUSE_IW
endif
ifneq ($(CONFIG_CFG80211),)
DHDOFILES += wl_cfg80211.o wl_cfgp2p.o wl_linux_mon.o wl_cfg_btcoex.o wl_cfgnan.o
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
DHDCFLAGS += -DWLTDLS -DWL_SDO -DWL_NAN -DWL_SCHED_SCAN -DDUAL_STA -DAP_PLUS_STA
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
