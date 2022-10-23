#############################################################
#
# IPC SDK
#
#############################################################

IPC_SDK_VERSION = 1.0
IPC_SDK_SITE = $(TOPDIR)/../vendor/amlogic/ipc/ipc-sdk
IPC_SDK_SITE_METHOD = local
IPC_SDK_INSTALL_STAGING = YES
IPC_SDK_DEPENDENCIES = libpng libjpeg freetype alsa-lib fdk-aac speexdsp
IPC_SDK_DEPENDENCIES += aml_libge2d aml_libgdc aml_libion

IPC_SDK_CFLAGS := $(TARGET_CFLAGS)
ifeq ($(BR2_PACKAGE_AML_SOC_USE_MULTIENC), y)
IPC_SDK_DEPENDENCIES += libmultienc
else
IPC_SDK_CFLAGS += -DIPC_SDK_VENC_LEGACY
IPC_SDK_DEPENDENCIES += libvpcodec libvphevcodec
endif

ifeq ($(BR2_PACKAGE_IPC_SDK_BUILD_EXAMPLE), y)
IPC_SDK_DEPENDENCIES += gstreamer1 gst1-rtsp-server gst1-plugins-base libwebsockets aml_nn_detect
endif

define IPC_SDK_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(TARGET_MAKE_ENV) CFLAGS="$(IPC_SDK_CFLAGS)" $(MAKE) -C $(@D)
endef

define IPC_SDK_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 644 $(@D)/src/libaml_ipc_sdk.so $(STAGING_DIR)/usr/lib/
	$(INSTALL) -D -m 644 $(@D)/inc/* $(STAGING_DIR)/usr/include/
endef

define IPC_SDK_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/src/libaml_ipc_sdk.so $(TARGET_DIR)/usr/lib/
	$(INSTALL) -D -m 755 $(@D)/example/ipc_launch $(TARGET_DIR)/usr/bin/
endef

IPC_SDK_EXTRA_BUILD :=

ifeq ($(BR2_PACKAGE_IPC_SDK_BUILD_EXAMPLE), y)
	IPC_SDK_EXTRA_BUILD += IPC_SDK_BUILD_EXAMPLE="y"
else
define IPC_SDK_BUILD_IPC_LAUNCH
	$(TARGET_CONFIGURE_OPTS) $(TARGET_MAKE_ENV) CFLAGS="$(IPC_SDK_CFLAGS)" $(MAKE) -C $(@D) example/ipc_launch
endef
IPC_SDK_POST_INSTALL_STAGING_HOOKS += IPC_SDK_BUILD_IPC_LAUNCH
endif

ifeq ($(BR2_PACKAGE_IPC_SDK_BUILD_TEST), y)
	IPC_SDK_EXTRA_BUILD += IPC_SDK_BUILD_TEST="y"
	IPC_SDK_DEPENDENCIES += gtest
endif

define IPC_SDK_BUILD_EXTRA
	$(TARGET_CONFIGURE_OPTS) $(TARGET_MAKE_ENV) $(IPC_SDK_EXTRA_BUILD) CFLAGS="$(IPC_SDK_CFLAGS)" $(MAKE) -C $(@D)
endef

define IPC_SDK_RELEASE_PACKAGE
    if ls $(@D)/src/*.c 2>&1 >> /dev/null;  then \
		mv $(@D)/src/Makefile $(@D)/src/Makefile.sdk; \
		echo "all clean:" > $(@D)/src/Makefile; \
		echo "	@echo \"Nothing todo\"" >> $(@D)/src/Makefile; \
		tar -cf $(TARGET_DIR)/../images/ipc-sdk-release.tar -C $(@D) --exclude="*.o" --exclude="*.d" --exclude='.[^/]*' --exclude=objs --exclude=src .; \
		tar -uf $(TARGET_DIR)/../images/ipc-sdk-release.tar -C $(@D) src/Makefile src/libaml_ipc_sdk.so; \
		gzip -f $(TARGET_DIR)/../images/ipc-sdk-release.tar; \
		mv $(@D)/src/Makefile.sdk $(@D)/src/Makefile; \
	fi
endef

ifneq ($(IPC_SDK_EXTRA_BUILD), '')
IPC_SDK_POST_INSTALL_STAGING_HOOKS += IPC_SDK_BUILD_EXTRA
endif
IPC_SDK_POST_INSTALL_STAGING_HOOKS += IPC_SDK_RELEASE_PACKAGE

$(eval $(generic-package))
