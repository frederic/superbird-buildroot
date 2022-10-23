############################################################################
#
# cypress bsa
#
############################################################################

AML_CYPRESS_BSA_VERSION = 0107_00.28.00
AML_CYPRESS_BSA_SITE = $(TOPDIR)/../vendor/cypress/cypress-bsa
AML_CYPRESS_BSA_SITE_METHOD = local

AML_CYPRESS_BSA_PATH = 3rdparty/embedded/bsa_examples/linux
AML_CYPRESS_BSA_LIBBSA = libbsa

AML_BT_DSPC_PATH = 3rdparty/embedded/bsa_examples/linux
AML_BT_DSPC_LIBDSPC = libdspc

ifeq ($(BR2_PACKAGE_DUEROS),y)
TARGET_CONFIGURE_OPTS += DUEROS_SDK=y
endif

ifeq ($(BR2_aarch64),y)
#ifeq ($(TARGET_BUILD_TYPE),64)
AML_CYPRESS_BSA_BUILD_TYPE = arm64
BT_AUDIO_CONF = alsa-bsa-64.conf
else
AML_CYPRESS_BSA_BUILD_TYPE = arm
BT_AUDIO_CONF = alsa-bsa-32.conf
endif

ifeq ($(BR2_PACKAGE_PULSEAUDIO), y)
BT_AUDIO_CONF = alsa-bsa-pulse.conf
endif

#############################################bsa_server libbsa.so {
define AML_CYPRESS_BSA_INSTALL_SERVER_SO_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/$(AML_CYPRESS_BSA_PATH)/server/build/$(AML_CYPRESS_BSA_BUILD_TYPE)/bsa_server \
		$(TARGET_DIR)/usr/bin/bsa_server
	$(INSTALL) -D -m 755 $(@D)/$(AML_CYPRESS_BSA_PATH)/$(AML_CYPRESS_BSA_LIBBSA)/build/$(AML_CYPRESS_BSA_BUILD_TYPE)/sharedlib/libbsa.so \
		$(TARGET_DIR)/usr/lib/libbsa.so
endef
#############################################bsa_server libbsa.so }


#############################################driver {
AML_CYPRESS_BSA_DEPENDENCIES += linux
define AML_CYPRESS_BSA_BTHID_DRIVER_BUILD_CMDS
$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/3rdparty/embedded/brcm/linux/bthid ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_KERNEL_CROSS) modules
endef

define AML_CYPRESS_BSA_BTHID_DRIVER_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/3rdparty/embedded/brcm/linux/bthid/bthid.ko $(TARGET_DIR)/usr/lib
endef
#############################################driver }


#############################################amlogic app {
AML_CYPRESS_MUSIC_BOX = aml_musicBox
ifeq ($(BR2_AML_BT_DSPC),y)
define AML_CYPRESS_BSA_MUSICBOX_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/$(AML_BT_DSPC_PATH)/$(AML_BT_DSPC_LIBDSPC)/ \
		CC="$(EXTERNAL_TOOLCHAIN_DIR)/bin/arm-linux-gnueabihf-g++" \
		CFLAGS="-Wall -O3 -std=c++0x -fpic -D_GLIBCXX_USE_NANOSLEEP  -I$(STAGING_DIR)/usr/include" \
		LDFLAGS="-lpthread -L$(TARGET_DIR)/usr/lib32  -lasound -L./lib -lAWELib"


	$(TARGET_CONFIGURE_OPTS) $(MAKE)  -C $(@D)/$(AML_CYPRESS_BSA_PATH)/$(AML_CYPRESS_MUSIC_BOX)/build \
		CPU=$(AML_CYPRESS_BSA_BUILD_TYPE) ARMGCC=$(TARGET_CC) ENABLE_ALSA_DSPC=TRUE BSASHAREDLIB=TRUE \
		LINKLIBS="-lpthread -L$(TARGET_DIR)/usr/lib32 -lasound -L../../$(AML_BT_DSPC_LIBDSPC)/ -ldspc -L../../$(AML_BT_DSPC_LIBDSPC)/lib -lAWELib"
endef

define AML_CYPRESS_BSA_MUSICBOX_INSTALL_TARGET_CMDS

	$(INSTALL) -D -m 755 $(@D)/$(AML_CYPRESS_BSA_PATH)/$(AML_CYPRESS_MUSIC_BOX)/build/$(AML_CYPRESS_BSA_BUILD_TYPE)/$(AML_CYPRESS_MUSIC_BOX) $(TARGET_DIR)/usr/bin/$(AML_CYPRESS_MUSIC_BOX)


	$(INSTALL) -D -m 755 $(@D)/$(AML_BT_DSPC_PATH)/$(AML_BT_DSPC_LIBDSPC)/libdspc.so \
		$(TARGET_DIR)/usr/lib/libdspc.so
	$(INSTALL) -D -m 755 $(@D)/$(AML_BT_DSPC_PATH)/$(AML_BT_DSPC_LIBDSPC)/AMLogic_VUI_Solution_v6a_VoiceOnly_release.awb  \
		$(TARGET_DIR)/etc/bsa/config/AMLogic_VUI_Solution_v6a_VoiceOnly_release.awb
	$(INSTALL) -D -m 755 $(@D)/$(AML_BT_DSPC_PATH)/$(AML_BT_DSPC_LIBDSPC)/netfile.raw \
		$(TARGET_DIR)/etc/bsa/config/netfile.raw
	$(INSTALL) -D -m 755 $(@D)/$(AML_BT_DSPC_PATH)/$(AML_BT_DSPC_LIBDSPC)/searchfile.raw \
		$(TARGET_DIR)/etc/bsa/config/searchfile.raw
endef
else
define AML_CYPRESS_BSA_MUSICBOX_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE)  -C $(@D)/$(AML_CYPRESS_BSA_PATH)/$(AML_CYPRESS_MUSIC_BOX)/build \
		CPU=$(AML_CYPRESS_BSA_BUILD_TYPE) ARMGCC=$(TARGET_CC) BSASHAREDLIB=TRUE
endef
define AML_CYPRESS_BSA_MUSICBOX_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/$(AML_CYPRESS_BSA_PATH)/$(AML_CYPRESS_MUSIC_BOX)/build/$(AML_CYPRESS_BSA_BUILD_TYPE)/$(AML_CYPRESS_MUSIC_BOX) $(TARGET_DIR)/usr/bin/$(AML_CYPRESS_MUSIC_BOX)
endef

endif #end BR2_AML_BT_DSPC
#############################################amlogic app }


#############################################extra apps {
AML_CYPRESS_BSA_APP = aml_ble_wifi_setup aml_socket app_manager app_avk app_hs

define AML_CYPRESS_BSA_APP_BUILD_CMDS
	for ff in $(AML_CYPRESS_BSA_APP); do \
		$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/$(AML_CYPRESS_BSA_PATH)/$$ff/build \
			CPU=$(AML_CYPRESS_BSA_BUILD_TYPE) ARMGCC=$(TARGET_CC) BSASHAREDLIB=TRUE; \
	done
endef

define AML_CYPRESS_BSA_APP_INSTALL_CMDS
	for ff in $(AML_CYPRESS_BSA_APP); do \
		$(INSTALL) -D -m 755 $(@D)/$(AML_CYPRESS_BSA_PATH)/$${ff}/build/$(AML_CYPRESS_BSA_BUILD_TYPE)/$${ff} $(TARGET_DIR)/usr/bin/${ff}; \
	done
endef
#############################################extra apps }


define AML_CYPRESS_BSA_BUILD_CMDS
	$(AML_CYPRESS_BSA_APP_BUILD_CMDS)
	$(AML_CYPRESS_BSA_MUSICBOX_BUILD_CMDS)
#	$(AML_CYPRESS_BSA_BTHID_DRIVER_BUILD_CMDS)
endef


define AML_CYPRESS_BSA_INSTALL_TARGET_CMDS
	$(AML_CYPRESS_BSA_INSTALL_SERVER_SO_TARGET_CMDS)
	$(AML_CYPRESS_BSA_APP_INSTALL_CMDS)
	$(AML_CYPRESS_BSA_MUSICBOX_INSTALL_TARGET_CMDS)
#	$(AML_CYPRESS_BSA_BTHID_DRIVER_TARGET_CMDS)
	mkdir -p $(TARGET_DIR)/etc/bsa
	mkdir -p $(TARGET_DIR)/etc/bsa/config
	$(INSTALL) -D -m 755 $(AML_CYPRESS_BSA_PKGDIR)/S44bluetooth $(TARGET_DIR)/etc/init.d
	$(INSTALL) -D -m 755 $(AML_CYPRESS_BSA_PKGDIR)/$(BT_AUDIO_CONF) $(TARGET_DIR)/etc/alsa_bsa.conf
endef




$(eval $(generic-package))
