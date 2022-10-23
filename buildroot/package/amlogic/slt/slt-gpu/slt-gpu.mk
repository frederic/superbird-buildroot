#
# SLT GPU
#
SLT_GPU_VERSION = 1.0
SLT_GPU_SITE = $(TOPDIR)/../vendor/amlogic/slt/gpu_slt_test/slt
SLT_GPU_SITE_METHOD = local
SLT_GPU_DEPENDENCIES = gpu

ifeq ($(BR2_aarch64),y)
FILL_BUFFER = fill_buffer/64bit/gpu.fill_buffer
FLATLAND = flatland/64bit/flatland
LIB_PATH=$(STAGING_DIR)/usr/lib64
else ifeq ($(BR2_ARM_EABIHF),y)
FILL_BUFFER = fill_buffer/32bit/gpu.fill_buffer
FLATLAND = flatland/32bit/flatland
LIB_PATH=$(STAGING_DIR)/usr/lib
endif

define SLT_GPU_BUILD_CMDS
    $(TARGET_MAKE_ENV) $(MAKE) CC=$(TARGET_CXX) CROSS_COMPILE=$(TARGET_CROSS) \
    INC=$(STAGING_DIR)/usr/include LIB_PATH=$(LIB_PATH) -C $(@D)
endef

define SLT_GPU_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/check_err/gpu.check_err $(TARGET_DIR)/usr/bin/
    $(INSTALL) -D -m 0755 $(@D)/opengles2_basic/gpu.gl2_basic $(TARGET_DIR)/usr/bin/
    $(INSTALL) -D -m 0755 $(@D)/fill_buffer/sample.bmp $(TARGET_DIR)/usr/
    $(INSTALL) -D -m 0755 $(@D)/$(FILL_BUFFER) $(TARGET_DIR)/usr/bin/
    $(INSTALL) -D -m 0755 $(@D)/$(FLATLAND) $(TARGET_DIR)/usr/bin/
    ln  -s /usr/share/arm/opengles_31/compute_particles/assets  $(TARGET_DIR)/assets
endef

define SLT_GPU_INSTALL_CLEAN_CMDS
    $(MAKE) CC=$(TARGET_CXX) -C $(@D) clean
endef

$(eval $(generic-package))
