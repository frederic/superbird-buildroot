############################################################################
#
# broadcom bsa
#
############################################################################

AML_BRCM_BSA_VERSION = 0107_00.26.00
AML_BRCM_BSA_SITE = $(TOPDIR)/../vendor/broadcom/brcm-bsa
AML_BRCM_BSA_SITE_METHOD = local

AML_BRCM_BSA_PATH = 3rdparty/embedded/bsa_examples/linux
AML_BRCM_BSA_LIBBSA = libbsa

AML_BT_DSPC_PATH = 3rdparty/embedded/bsa_examples/linux
AML_BT_DSPC_LIBDSPC = libdspc

ifeq ($(BR2_PACKAGE_FDK_AAC),y)
AML_BRCM_BSA_DEPENDENCIES += fdk-aac
endif

ifeq ($(BR2_PACKAGE_AUDIOSERVICE),y)
AML_BRCM_BSA_DEPENDENCIES += audioservice
endif

ifeq ($(BR2_PACKAGE_DUEROS),y)
TARGET_CONFIGURE_OPTS += DUEROS_SDK=y
endif

ifeq ($(BR2_aarch64),y)
AML_BRCM_BSA_BUILD_TYPE = arm64
BT_AUDIO_CONF = alsa-bsa-64.conf
else
AML_BRCM_BSA_BUILD_TYPE = arm
BT_AUDIO_CONF = alsa-bsa-32.conf
endif
ifeq ($(BR2_PACKAGE_PULSEAUDIO), y)
BT_AUDIO_CONF = alsa-bsa-pulse.conf
endif
####if mem reductin configured, only support few profiles########
ifeq ($(BR2_AML_BRCM_BSA_MEM_REDUCTION),y)
AML_BRCM_BSA_APP = app_manager app_ag app_avk app_hs app_av \
	app_switch app_tm

TARGET_CONFIGURE_OPTS += CONFIG_MEM_REDUCTION=y

define AML_BRCM_BSA_INSTALL_SERVER_SO_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/server/$(AML_BRCM_BSA_BUILD_TYPE)_mem_reduce/bsa_server \
		$(TARGET_DIR)/usr/bin/bsa_server
	$(INSTALL) -D -m 755 $(@D)/$(AML_BRCM_BSA_PATH)/$(AML_BRCM_BSA_LIBBSA)/build/$(AML_BRCM_BSA_BUILD_TYPE)_mem_reduce/sharedlib/libbsa.so \
		$(TARGET_DIR)/usr/lib/libbsa.so
endef
else
AML_BRCM_BSA_APP = app_manager app_3d app_ag app_av app_avk app_ble \
	app_ble_ancs app_ble_blp app_ble_cscc app_ble_eddystone app_ble_hrc \
	app_ble_htp app_ble_pm app_ble_rscc app_ble_tvselect app_ble_wifi aml_ble_wifi_setup\
	app_cce app_dg app_fm app_ftc app_fts app_hd app_headless \
	app_hh app_hl app_hs app_nsa app_opc app_ops app_pan \
	app_pbc app_pbs app_sac app_sc app_switch app_tm aml_socket app_audio_source

define AML_BRCM_BSA_INSTALL_SERVER_SO_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/server/$(AML_BRCM_BSA_BUILD_TYPE)/bsa_server \
		$(TARGET_DIR)/usr/bin/bsa_server
	$(INSTALL) -D -m 755 $(@D)/$(AML_BRCM_BSA_PATH)/$(AML_BRCM_BSA_LIBBSA)/build/$(AML_BRCM_BSA_BUILD_TYPE)/sharedlib/libbsa.so \
		$(TARGET_DIR)/usr/lib/libbsa.so

#	$(INSTALL) -D -m 755 $(@D)/3rdparty/embedded/brcm/linux/bthid/bthid.ko $(TARGET_DIR)/usr/lib
endef

#AML_BRCM_BSA_DEPENDENCIES += linux
#define AML_BRCM_BSA_BTHID_DRIVER_BUILD_CMDS
#$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/3rdparty/embedded/brcm/linux/bthid ARCH=$(KERNEL_ARCH) \
#		CROSS_COMPILE=$(TARGET_KERNEL_CROSS) modules
#endef

##if qt5 configured, we can use bsa qt app####
ifeq ($(BR2_PACKAGE_QT5),y)
AML_BRCM_BSA_DEPENDENCIES += qt5base
BSA_QT_APP = $(@D)/$(AML_BRCM_BSA_PATH)/qt_app

define AML_BRCM_BSA_CONFIGURE_CMDS
	(cd $(BSA_QT_APP); $(TARGET_MAKE_ENV) $(QT5_QMAKE) PREFIX=/usr BSA_QT_ARCH=$(AML_BRCM_BSA_BUILD_TYPE))
endef

define AML_BRCM_BSA_BUILD_CMDS_QT
	$(TARGET_MAKE_ENV) $(MAKE) -C $(BSA_QT_APP)
endef

define AML_BRCM_BSA_INSTALL_TARGET_CMDS_QT
	$(TARGET_MAKE_ENV) $(MAKE) -C $(BSA_QT_APP) install INSTALL_ROOT=$(TARGET_DIR)
endef

endif
#######qt app configure end#################

endif
######BR2_AML_BRCM_BSA_MEM_REDUCTION judge end###################

########## amlogic app ##########################################
AML_BRCM_MUSIC_BOX = aml_musicBox
ifeq ($(BR2_AML_BT_DSPC),y)
define AML_BRCM_BSA_MUSICBOX_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/$(AML_BT_DSPC_PATH)/$(AML_BT_DSPC_LIBDSPC)/ \
		CC="$(EXTERNAL_TOOLCHAIN_DIR)/bin/arm-linux-gnueabihf-g++" \
		CFLAGS="-Wall -O3 -std=c++0x -fpic -D_GLIBCXX_USE_NANOSLEEP  -I$(STAGING_DIR)/usr/include" \
		LDFLAGS="-lpthread -L$(TARGET_DIR)/usr/lib32  -lasound -L./lib -lAWELib"


	$(TARGET_CONFIGURE_OPTS) $(MAKE)  -C $(@D)/$(AML_BRCM_BSA_PATH)/$(AML_BRCM_MUSIC_BOX)/build \
		CPU=$(AML_BRCM_BSA_BUILD_TYPE) ARMGCC=$(TARGET_CC) ENABLE_ALSA_DSPC=TRUE BSASHAREDLIB=TRUE \
		LINKLIBS="-lpthread -L$(TARGET_DIR)/usr/lib32 -lasound -L../../$(AML_BT_DSPC_LIBDSPC)/ -ldspc -L../../$(AML_BT_DSPC_LIBDSPC)/lib -lAWELib"
endef

define AML_BRCM_BSA_MUSICBOX_INSTALL_TARGET_CMDS

	$(INSTALL) -D -m 755 $(@D)/$(AML_BRCM_BSA_PATH)/$(AML_BRCM_MUSIC_BOX)/build/$(AML_BRCM_BSA_BUILD_TYPE)/$(AML_BRCM_MUSIC_BOX) $(TARGET_DIR)/usr/bin/$(AML_BRCM_MUSIC_BOX)


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
ifeq ($(BR2_PACKAGE_AUDIOSERVICE),y)
define AML_BRCM_BSA_MUSICBOX_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE)  -C $(@D)/$(AML_BRCM_BSA_PATH)/$(AML_BRCM_MUSIC_BOX)/build \
		CPU=$(AML_BRCM_BSA_BUILD_TYPE) ARMGCC=$(TARGET_CC) BSASHAREDLIB=TRUE \
		ENABLE_AUDIOSERVICE=TRUE
endef
else
define AML_BRCM_BSA_MUSICBOX_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE)  -C $(@D)/$(AML_BRCM_BSA_PATH)/$(AML_BRCM_MUSIC_BOX)/build \
		CPU=$(AML_BRCM_BSA_BUILD_TYPE) ARMGCC=$(TARGET_CC) BSASHAREDLIB=TRUE
endef
endif
define AML_BRCM_BSA_MUSICBOX_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/$(AML_BRCM_BSA_PATH)/$(AML_BRCM_MUSIC_BOX)/build/$(AML_BRCM_BSA_BUILD_TYPE)/$(AML_BRCM_MUSIC_BOX) $(TARGET_DIR)/usr/bin/$(AML_BRCM_MUSIC_BOX)
endef

endif #end BR2_AML_BT_DSPC
AML_BRCM_AUDIO_SOURCE = app_audio_source

######## amlogic app end #######################################

define AML_BRCM_BSA_APP_BUILD_CMDS
	for ff in $(AML_BRCM_BSA_APP); do \
		$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/$(AML_BRCM_BSA_PATH)/$$ff/build \
			CPU=$(AML_BRCM_BSA_BUILD_TYPE) ARMGCC=$(TARGET_CC) BSASHAREDLIB=TRUE; \
	done
endef

define AML_BRCM_BSA_APP_INSTALL_CMDS
	for ff in $(AML_BRCM_BSA_APP); do \
		$(INSTALL) -D -m 755 $(@D)/$(AML_BRCM_BSA_PATH)/$${ff}/build/$(AML_BRCM_BSA_BUILD_TYPE)/$${ff} $(TARGET_DIR)/usr/bin/${ff}; \
	done
endef



define AML_BRCM_BSA_BUILD_CMDS
	$(AML_BRCM_BSA_APP_BUILD_CMDS)
	$(AML_BRCM_BSA_MUSICBOX_BUILD_CMDS)
	$(AML_BRCM_BSA_BUILD_CMDS_QT)
	$(AML_BRCM_BSA_BTHID_DRIVER_BUILD_CMDS)

endef


define AML_BRCM_BSA_INSTALL_TARGET_CMDS
	$(AML_BRCM_BSA_INSTALL_SERVER_SO_TARGET_CMDS)
	$(AML_BRCM_BSA_APP_INSTALL_CMDS)
	$(AML_BRCM_BSA_MUSICBOX_INSTALL_TARGET_CMDS)
	$(AML_BRCM_BSA_INSTALL_TARGET_CMDS_QT)
	mkdir -p $(TARGET_DIR)/etc/bsa
	mkdir -p $(TARGET_DIR)/etc/bsa/config
	$(INSTALL) -D -m 755 $(@D)/test_files/av/44k8bpsStereo.wav $(TARGET_DIR)/etc/bsa
	$(INSTALL) -D -m 755 $(@D)/test_files/dg/tx_test_file.txt $(TARGET_DIR)/etc/bsa
	$(INSTALL) -D -m 755 $(AML_BRCM_BSA_PKGDIR)/S44bluetooth $(TARGET_DIR)/etc/init.d
	$(INSTALL) -D -m 755 $(AML_BRCM_BSA_PKGDIR)/$(BT_AUDIO_CONF) $(TARGET_DIR)/etc/alsa_bsa.conf
endef




$(eval $(generic-package))
