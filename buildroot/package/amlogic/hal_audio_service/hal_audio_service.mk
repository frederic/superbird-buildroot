#############################################################
#
# hal_audio_service
#
#############################################################
HAL_AUDIO_SERVICE_VERSION:=0.1
HAL_AUDIO_SERVICE_SITE=$(TOPDIR)/../multimedia/hal_audio_service
HAL_AUDIO_SERVICE_SITE_METHOD=local
HAL_AUDIO_SERVICE_DEPENDENCIES=grpc boost liblog aml_amaudioutils
export HAL_AUDIO_SERVICE_BUILD_DIR = $(BUILD_DIR)
export HAL_AUDIO_SERVICE_STAGING_DIR = $(STAGING_DIR)
export HAL_AUDIO_SERVICE_TARGET_DIR = $(TARGET_DIR)
export HAL_AUDIO_SERVICE_BR2_ARCH = $(BR2_ARCH)

define HAL_AUDIO_SERVICE_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) CC=$(TARGET_CC) -C $(@D) all
	$(TARGET_MAKE_ENV) $(MAKE) CC=$(TARGET_CC) -C $(@D) install
endef
define HAL_AUDIO_SERVICE_INSTALL_SERVICE
	install -m 755 $(HAL_AUDIO_SERVICE_PKGDIR)/S56_aml_hal_audio_service $(TARGET_DIR)/etc/init.d/
endef
HAL_AUDIO_SERVICE_POST_INSTALL_TARGET_HOOKS += HAL_AUDIO_SERVICE_INSTALL_SERVICE

$(eval $(generic-package))
