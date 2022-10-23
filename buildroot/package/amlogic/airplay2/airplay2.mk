#############################################################
#
# AirPlay 2
#
#############################################################

AIRPLAY2_VERSION = master
AIRPLAY2_DEPENDENCIES = mdnsresponder alsa-lib fdk-aac aml_commonlib
AIRPLAY2_MAKE_OPTS := sdkptp=1
ifeq ($(BR2_PACKAGE_GPTP),y)
	AIRPLAY2_DEPENDENCIES += gptp
	AIRPLAY2_MAKE_OPTS = sdkptp=0
endif
AIRPLAY2_SITE_METHOD = local
#AIRPLAY2_ENABLE_DEBUG := y
AIRPLAY2_BUILD_DIR := Release-linux
ifeq ($(AIRPLAY2_ENABLE_DEBUG),y)
	AIRPLAY2_MAKE_OPTS += debug=1
	AIRPLAY2_BUILD_DIR := Debug-linux
endif
AIRPLAY2_MAKE_OPTS += EXT_INCLUDES="-I$(STAGING_DIR)/usr/include/alsa -I$(STAGING_DIR)/usr/include/fdk-aac" EXT_LINKFLAGS="-ldns_sd -lasound -lfdk-aac"

AIRPLAY2_LOCAL_SRC = $(wildcard $(TOPDIR)/../vendor/amlogic/airplayv2/airplayv2)
AIRPLAY2_LOCAL_PREBUILT = $(TOPDIR)/../vendor/amlogic/airplayv2/airplayv2_prebuilt
AIRPLAY2_PREBUILT_DIRECTORY = $(call qstrip,$(BR2_ARCH)).$(call qstrip,$(BR2_GCC_TARGET_ABI)).$(call qstrip,$(BR2_GCC_TARGET_FLOAT_ABI))
AIRPLAY2_SITE=$(AIRPLAY2_LOCAL_PREBUILT)

ifneq ($(AIRPLAY2_LOCAL_SRC),)
AIRPLAY2_SITE=$(AIRPLAY2_LOCAL_SRC)
define AIRPLAY2_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) CROSSPREFIX=$(TARGET_CROSS) $(AIRPLAY2_MAKE_OPTS) -C $(@D)/AirPlaySDK/PlatformPOSIX
	$(TARGET_MAKE_ENV) $(MAKE) CC=$(TARGET_CC) LD=$(TARGET_LD) STRIP=$(TARGET_STRIP) debug=1 platform_makefile=Platform/Platform.include.mk -C $(@D)/WAC
	$(TARGET_CC) -x c -g -DSIPC_IMPLEMENTATION -DAIRPLAY_CLIENT_DEMO $(@D)/AirPlaySDK/Platform/airplayipc.h -o $(@D)/AirPlaySDK/build/$(AIRPLAY2_BUILD_DIR)/airplayctrl
endef

define AIRPLAY2_INSTALL_TARGET_CMDS
	$(INSTALL) -m 755 -D $(@D)/AirPlaySDK/build/$(AIRPLAY2_BUILD_DIR)/airplaydemo $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 755 -D $(@D)/WAC/WACServer $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 644 -D $(@D)/AirPlaySDK/Platform/airplayipc.h $(STAGING_DIR)/usr/include/
	$(INSTALL) -m 755 -D $(AIRPLAY2_PKGDIR)S82airplay2 $(TARGET_DIR)/etc/init.d/
	$(INSTALL) -m 755 -D $(AIRPLAY2_PKGDIR)wac.sh $(TARGET_DIR)/usr/bin/
endef

ifneq ($(AIRPLAY2_LOCAL_PREBUILT),)
define AIRPLAY2_UPDATE_PREBUILT_CMDS

	# DO NOT remove the first empty line
	$(INSTALL) -m 755 -D $(@D)/AirPlaySDK/build/$(AIRPLAY2_BUILD_DIR)/airplaydemo $(AIRPLAY2_LOCAL_PREBUILT)/$(AIRPLAY2_PREBUILT_DIRECTORY)/usr/bin/airplaydemo
	$(INSTALL) -m 755 -D $(@D)/WAC/WACServer $(AIRPLAY2_LOCAL_PREBUILT)/$(AIRPLAY2_PREBUILT_DIRECTORY)/usr/bin/WACServer
	$(INSTALL) -m 644 -D $(@D)/AirPlaySDK/Platform/airplayipc.h $(AIRPLAY2_LOCAL_PREBUILT)/include/airplayipc.h
endef
	AIRPLAY2_INSTALL_TARGET_CMDS += $(AIRPLAY2_UPDATE_PREBUILT_CMDS)
endif

else # prebuilt

AIRPLAY2_SITE=$(AIRPLAY2_LOCAL_PREBUILT)
define AIRPLAY2_INSTALL_TARGET_CMDS
	$(INSTALL) -m 755 -D $(@D)/$(AIRPLAY2_PREBUILT_DIRECTORY)/usr/bin/airplaydemo $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 755 -D $(@D)/$(AIRPLAY2_PREBUILT_DIRECTORY)/usr/bin/WACServer $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 644 -D $(@D)/include/airplayipc.h $(STAGING_DIR)/usr/include/
	$(INSTALL) -m 755 -D $(AIRPLAY2_PKGDIR)S82airplay2 $(TARGET_DIR)/etc/init.d/
	$(INSTALL) -m 755 -D $(AIRPLAY2_PKGDIR)wac.sh $(TARGET_DIR)/usr/bin/
endef

endif

$(eval $(generic-package))

include package/amlogic/airplay2/gptp/gptp.mk
