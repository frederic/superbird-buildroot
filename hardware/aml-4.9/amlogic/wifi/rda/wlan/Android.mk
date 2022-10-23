ifeq ($(BOARD_WLAN_DEVICE),rda5995)
    include $(call all-subdir-makefiles)
endif
ifeq ($(MULTI_WIFI_SUPPORT), true)
    include $(call all-subdir-makefiles)
endif

