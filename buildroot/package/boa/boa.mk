################################################################################
#
# boa
#
################################################################################

BOA_VERSION = 0.94.14rc21
BOA_SITE = http://www.boa.org
BOA_LICENSE = GPL-2.0+
BOA_LICENSE_FILES = COPYING
BOA_EXTER_FILE = $(TOPDIR)/package/boa

BOA_CONF_ENV=CFLAGS+=-DINET6

define BOA_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/src/boa $(TARGET_DIR)/usr/sbin/boa
	$(INSTALL) -D -m 755 $(@D)/src/boa_indexer $(TARGET_DIR)/usr/lib/boa/boa_indexer
	$(INSTALL) -D -m 644 $(BOA_EXTER_FILE)/boa.conf $(TARGET_DIR)/etc/boa/boa.conf
	$(INSTALL) -D -m 644 package/boa/mime.types $(TARGET_DIR)/etc/mime.types
	$(INSTALL) -D -m 755 $(BOA_EXTER_FILE)/S43webserver  $(TARGET_DIR)/etc/init.d/S43webserver
endef

$(eval $(autotools-package))
