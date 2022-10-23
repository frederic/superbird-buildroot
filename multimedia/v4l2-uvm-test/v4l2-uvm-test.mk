#
# v4l2-uvm-test
#
V4L2_UVM_TEST_VERSION = 0.1
V4L2_UVM_TEST_SITE = $(TOPDIR)/../multimedia/v4l2-uvm-test/src
V4L2_UVM_TEST_SITE_METHOD = local

V4L2_UVM_TEST_DEPENDENCIES += ffmpeg
V4L2_UVM_TEST_DEPENDENCIES += libdrm

ifneq ($(BR2_PACKAGE_LIBSECMEM),y)
V4L2_UVM_TEST_DEPENDENCIES  += libsecmem-bin
else
V4L2_UVM_TEST_DEPENDENCIES  += libsecmem
endif

define V4L2_UVM_TEST_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) CC=$(TARGET_CC) -C $(@D)/
endef

define V4L2_UVM_TEST_INSTALL_TARGET_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) CC=$(TARGET_CC) -C $(@D)/ install
endef

define V4L2_UVM_TEST_INSTALL_CLEAN_CMDS
    $(MAKE) CC=$(TARGET_CC) -C $(@D) clean
endef

$(eval $(generic-package))
