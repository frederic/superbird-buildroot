#############################################################
#
# bt_setup
#
#############################################################

BT_SETUP_VERSION = 20171212
BT_SETUP_SITE = $(BT_SETUP_PKGDIR)/src
BT_SETUP_SITE_METHOD = local
BT_SETUP_DEPENDENCIES = cjson


BT_SETUP_INCLUDE = "-I$(@D)/include -I$(TOPDIR)/../vendor/broadcom/brcm-bsa/3rdparty/embedded/bsa_examples/linux/app_common/include/ -I$(TOPDIR)/../vendor/broadcom/brcm-bsa/3rdparty/embedded/bsa_examples/linux/libbsa/include"
BT_SETUP_LDFLAGS = "${TARGET_DIR}/usr/lib"

define BT_SETUP_BUILD_CMDS
  $(TARGET_CC) $(BT_SETUP_PKGDIR)/bt_con.c -o $(@D)/bt_con.cgi
  (cd $(@D) && $(TARGET_MAKE_ENV) $(MAKE) CC=$(TARGET_CC) CXX=$(TARGET_CXX) INCLUDE=$(BT_SETUP_INCLUDE) LDFLAGS=-L${BT_SETUP_LDFLAGS})
endef

define BT_SETUP_INSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) install
endef

$(eval $(generic-package))
