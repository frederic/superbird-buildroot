################################################################################
#
# chromiumos drm tests
#
################################################################################
DRM_TESTS_VERSION = 2018
DRM_TESTS_SITE = $(TOPDIR)/../vendor/amlogic/libdrm_amlogic/drm-tests
DRM_TESTS_SITE_METHOD = local
DRM_TESTS_DEPENDENCIES = libdrm aml_minigbm
DRM_TESTS_INSTALL_STAGING = NO

DRM_TESTS_OUT_DIR = $(DRM_TESTS_DIR)/out

DRM_TESTS_MAKE_ENV = \
	SYSROOT=$(STAGING_DIR) \
	CFLAGS=" -I$(DRM_TESTS_DIR)/kernel/v4.9/include/uapi/ "

define DRM_TESTS_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(DRM_TESTS_MAKE_ENV) $(MAKE) -C $(@D) OUT="$(DRM_TESTS_OUT_DIR)/"
endef

define DRM_TESTS_INSTALL_TARGET_CMDS
	cp -af $(DRM_TESTS_OUT_DIR)/atomictest $(TARGET_DIR)/usr/bin/
	cp -af $(DRM_TESTS_OUT_DIR)/drm_cursor_test $(TARGET_DIR)/usr/bin/
	cp -af $(DRM_TESTS_OUT_DIR)/gamma_test $(TARGET_DIR)/usr/bin/
	cp -af $(DRM_TESTS_OUT_DIR)/linear_bo_test $(TARGET_DIR)/usr/bin/
	cp -af $(DRM_TESTS_OUT_DIR)/mapped_texture_test $(TARGET_DIR)/usr/bin/
	cp -af $(DRM_TESTS_OUT_DIR)/mmap_test $(TARGET_DIR)/usr/bin/
	cp -af $(DRM_TESTS_OUT_DIR)/null_platform_test $(TARGET_DIR)/usr/bin/
	cp -af $(DRM_TESTS_OUT_DIR)/plane_test $(TARGET_DIR)/usr/bin/
	cp -af $(DRM_TESTS_OUT_DIR)/swrast_test $(TARGET_DIR)/usr/bin/
	cp -af $(DRM_TESTS_OUT_DIR)/vgem_test $(TARGET_DIR)/usr/bin/
	cp -af $(DRM_TESTS_OUT_DIR)/stripe $(TARGET_DIR)/usr/bin/
endef

$(eval $(generic-package))
