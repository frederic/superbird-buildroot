################################################################################
#
# amlogic minigbm
#
################################################################################
AML_MINIGBM_VERSION = 2018
AML_MINIGBM_SITE = $(TOPDIR)/../vendor/amlogic/minigbm/src
AML_MINIGBM_SITE_METHOD = local
AML_MINIGBM_DEPENDENCIES = libdrm
AML_MINIGBM_INSTALL_STAGING = YES

GBM_VERSION_MAJOR := 1
MINIGBM_VERSION := $(GBM_VERSION_MAJOR).0.0
MINIGBM_FILENAME := libminigbm.so.$(MINIGBM_VERSION)
MINIGBM_OUT_DIR = $(AML_MINIGBM_DIR)/out

AML_MINIGBM_MAKE_ENV = \
	SYSROOT=$(STAGING_DIR) \
	CFLAGS=" -DDRV_MESON"

define AML_MINIGBM_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(AML_MINIGBM_MAKE_ENV) $(MAKE) -C $(@D) OUT="$(MINIGBM_OUT_DIR)/"
endef

define AML_MINIGBM_INSTALL_STAGING_CMDS
	cp -arf $(MINIGBM_OUT_DIR)/$(MINIGBM_FILENAME) $(STAGING_DIR)/usr/lib/
	cd $(STAGING_DIR)/usr/lib/;ln -sf $(MINIGBM_FILENAME) libgbm.so
	cd $(STAGING_DIR)/usr/lib/;ln -sf $(MINIGBM_FILENAME) libgbm.so.$(GBM_VERSION_MAJOR)
	$(INSTALL) -m 0644 $(AML_MINIGBM_DIR)/gbm.pc $(STAGING_DIR)/usr/lib/pkgconfig
	$(INSTALL) -m 0644 $(AML_MINIGBM_DIR)/gbm.h $(STAGING_DIR)/usr/include
endef

define AML_MINIGBM_INSTALL_TARGET_CMDS
	cp -arf $(MINIGBM_OUT_DIR)/$(MINIGBM_FILENAME) $(TARGET_DIR)/usr/lib/
	cd $(TARGET_DIR)/usr/lib/;ln -sf $(MINIGBM_FILENAME) libgbm.so
	cd $(TARGET_DIR)/usr/lib/;ln -sf $(MINIGBM_FILENAME) libgbm.so.$(GBM_VERSION_MAJOR)
	cd $(TARGET_DIR)/usr/lib/;ln -sf $(MINIGBM_FILENAME) libminigbm.so
	$(INSTALL) -m 0644 $(AML_MINIGBM_DIR)/gbm.pc $(TARGET_DIR)/usr/lib/pkgconfig
endef

$(eval $(generic-package))
