#####################################################################################
#
#avs-sdk
#
####################################################################################

AVS_SDK_SITE = $(TOPDIR)/../multimedia/avs
AVS_SDK_SITE_METHOD = local
AVS_SDK_LICENSE = Apache License 2.0
AVS_SDK_LICENSE = LICENSE
AVS_SDK_DEPENDENCIES = libgmime \
                       libtotem \
                       libcurl \
                       nghttp2 \
                       openssl \
                       sqlite \
                       gstreamer1 \
                       gst1-plugins-base \
                       dbus

AVS_SDK_CONF_OPTS = \
                    -DBUILD_OUT_OF_TREE=ON \
                    -DGSTREAMER_MEDIA_PLAYER=ON \
                    -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
		    -DBUILD_TESTING=ON \
                    -DTOTEM_PLPARSER=ON

ifeq ($(BR2_PACKAGE_LIBXML2),y)
AVS_SDK_DEPENDENCIES += libxml2
endif

ifeq ($(BR2_PACKAGE_LIBFFI),y)
AVS_SDK_DEPENDENCIES += libffi
endif

ifeq ($(BR2_PACKAGE_LIBSOUP),y)
AVS_SDK_DEPENDENCIES += libsoup
endif

ifeq ($(BR2_PACKAGE_ZLIB),y)
AVS_SDK_DEPENDENCIES += zlib
endif

ifeq ($(BR2_PACKAGE_UTIL_LINUX),y)
AVS_SDK_DEPENDENCIES += util-linux
endif

ifeq ($(BR2_PACKAGE_LIBFAAD_DEV),y)
AVS_SDK_DEPENDENCIES += libfaad-dev
endif

ifeq ($(BR2_PACKAGE_PORTAUDIO),y)
AVS_SDK_DEPENDENCIES += portaudio
AVS_SDK_CONF_OPTS += -DPORTAUDIO:BOOL=ON
AVS_SDK_CONF_OPTS += -DPORTAUDIO_LIB_PATH=${BUILD_DIR}/portaudio-v190600_20161030/lib/.libs/libportaudio.so
AVS_SDK_CONF_OPTS += -DPORTAUDIO_INCLUDE_DIR=$(BUILD_DIR)/portaudio-v190600_20161030/include
endif

ifeq ($(BR2_PACKAGE_SENSORY),y)
AVS_SDK_DEPENDENCIES += sensory
AVS_SDK_CONF_OPTS += -DSENSORY_KEY_WORD_DETECTOR=ON
endif

ifeq ($(BR2_PACKAGE_KITT_AI),y)
AVS_SDK_DEPENDENCIES += kitt-ai
AVS_SDK_CONF_OPTS += -DKITTAI_KEY_WORD_DETECTOR=ON
endif

ifeq ($(BR2_AVS_DSPC_AEC),y)
AVS_SDK_CONF_OPTS += -DEXT_WWE_DSP_KEY_WORD_DETECTOR=ON
AVS_SDK_CONF_OPTS += -DDSP_KEY_WORD_DETECTOR_LIB_PATH=$(BUILD_DIR)/avs-sdk/KWD/DSP/lib/
AVS_SDK_CONF_OPTS += -DDSP_KEY_WORD_DETECTOR_INCLUDE_DIR=$(BUILD_DIR)/avs-sdk/KWD/DSP/include/
endif

ifeq ($(BR2_AVS_AMAZON_WWE),y)
AVS_SDK_CONF_OPTS += -DAMAZON_KEY_WORD_DETECTOR=ON
AVS_SDK_CONF_OPTS += -DAMAZON_KEY_WORD_DETECTOR_LIB_PATH=$(BUILD_DIR)/avs-sdk/KWD/Amazon/lib/libpryon_lite.so.1.13
AVS_SDK_CONF_OPTS += -DAMAZON_KEY_WORD_DETECTOR_INCLUDE_DIR=$(BUILD_DIR)/avs-sdk/KWD/Amazon/include
endif

ifeq ($(BR2_AVS_RDSPC),y)
AVS_SDK_CONF_OPTS += -DRDSP_KEY_WORD_DETECTOR=ON
AVS_SDK_CONF_OPTS += -DRDSP_KEY_WORD_DETECTOR_LIB_PATH=$(BUILD_DIR)/avs-sdk/KWD/RDSP/lib/librdsp.so
AVS_SDK_CONF_OPTS += -DRDSP_KEY_WORD_DETECTOR_INCLUDE_DIR=$(BUILD_DIR)/avs-sdk/KWD/RDSP/include
endif

ifeq ($(BR2_PACKAGE_DISPLAYCARD_AML_IMP),y)
AVS_SDK_CONF_OPTS += -DDISPLAYCARD_AML_IMPL=ON
AVS_SDK_CONF_OPTS += -DDISPLAYCARD_LIB_PATH=$(BUILD_DIR)/avs-sdk/DisplayCard/libDisplayCardImpCom.so
AVS_SDK_CONF_OPTS += -DDISPLAYCARD_INCLUDE_DIR=$(BUILD_DIR)/avs-sdk/DisplayCard
endif

ifeq ($(BR2_AVS_CLIENT_API),y)
AVS_SDK_CONF_OPTS += -DAVS_CLIENT_API=ON
AVS_SDK_CONF_OPTS += -DSERVER_INCLUDE_DIR=$(BUILD_DIR)/avs-sdk/Server
endif

ifeq ($(BR2_PACKAGE_LEDRING),y)
AVS_SDK_DEPENDENCIES += ledring
AVS_SDK_CONF_OPTS += -DLEDRING_LIB_PATH=$(STAGING_DIR)/usr/lib/libledring.so
AVS_SDK_CONF_OPTS += -DLEDRING_INCLUDE_DIR=$(STAGING_DIR)/usr/include
endif

define AVS_SDK_POST_INSTALL_TARGET
cp $(AVS_SDK_PKGDIR)/S48avs $(TARGET_DIR)/etc/init.d
cp $(AVS_SDK_PKGDIR)/avs_moniter.sh $(TARGET_DIR)/etc/init.d
mkdir -p $(TARGET_DIR)/var/www/cgi-bin
cp $(AVS_SDK_PKGDIR)/cgi-bin/* $(TARGET_DIR)/var/www/cgi-bin/
if [ "$(BR2_PACKAGE_AVAHI)" == "n" ] || [ -z "$(BR2_PACKAGE_AVAHI)" ]; then \
	$(INSTALL) -D -m 755 $(AVS_SDK_PKGDIR)/avs_mdns.sh \
		$(TARGET_DIR)/etc/init.d; \
fi
endef
AVS_SDK_POST_INSTALL_TARGET_HOOKS += AVS_SDK_POST_INSTALL_TARGET

$(eval $(cmake-package))
