#############################################################
#
# mDNSResponder
#
#############################################################
MDNSRESPONDER_VERSION = 878.200.35
MDNSRESPONDER_SOURCE = mDNSResponder-$(MDNSRESPONDER_VERSION).tar.gz
MDNSRESPONDER_SITE = https://opensource.apple.com/tarballs/mDNSResponder
MDNSRESPONDER_DIR = $(BUILD_DIR)/mdnsresponder-$(MDNSRESPONDER_VERSION)
MDNSRESPONDER_LICENSE = \
						Apache-2.0, BSD-3c (shared library), GPLv2 (mDnsEmbedded.h), \
						NICTA Public Software Licence
MDNSRESPONDER_LICENSE_FILES = LICENSE
MDNSRESPONDER_INSTALL_STAGING = YES
strip = $(BUILD_DIR)/../host/usr/bin/arm-linux-gnueabihf-strip

define MDNSRESPONDER_BUILD_CMDS
	$(MAKE1) CC=$(TARGET_CC) os="linux" LD="$(TARGET_CC) -shared" LOCALBASE="/usr" -C $(MDNSRESPONDER_DIR)/mDNSPosix STRIP=$(strip)
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)/mDNSPosix os="linux" LD="$(TARGET_CC) -shared"
endef

define MDNSRESPONDER_INSTALL_STAGING_CMDS
	$(INSTALL) -m 644 -D $(MDNSRESPONDER_DIR)/mDNSPosix/build/prod/libdns_sd.so $(STAGING_DIR)/usr/lib/
	ln -sf $(STAGING_DIR)/usr/lib/libdns_sd.so $(STAGING_DIR)/usr/lib/libdns_sd.so.1
	$(INSTALL) -m 644 -D $(MDNSRESPONDER_DIR)/mDNSShared/dns_sd.h $(STAGING_DIR)/usr/include/
endef

define MDNSRESPONDER_INSTALL_TARGET_CMDS
	$(INSTALL) -m 755 -D $(MDNSRESPONDER_DIR)/mDNSPosix/build/prod/mDNSResponderPosix $(TARGET_DIR)/usr/sbin/
	$(INSTALL) -m 755 -D $(MDNSRESPONDER_DIR)/mDNSPosix/build/prod/mdnsd $(TARGET_DIR)/usr/sbin/

	$(INSTALL) -m 644 -D $(MDNSRESPONDER_DIR)/mDNSPosix/build/prod/libdns_sd.so $(TARGET_DIR)/usr/lib/
	ln -sf $(TARGET_DIR)/usr/lib/libdns_sd.so $(TARGET_DIR)/usr/lib/libdns_sd.so.1

	$(INSTALL) -m 0755 -D $(MDNSRESPONDER_PKGDIR)/S80mdns $(TARGET_DIR)/etc/init.d/

	$(INSTALL) -m 755 -D $(MDNSRESPONDER_DIR)/mDNSPosix/build/prod/mDNSNetMonitor $(TARGET_DIR)/usr/sbin/

	$(INSTALL) -m 755 -D $(MDNSRESPONDER_DIR)/Clients/build/dns-sd $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 755 -D $(MDNSRESPONDER_DIR)/mDNSPosix/build/prod/mDNSProxyResponderPosix $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 755 -D $(MDNSRESPONDER_DIR)/mDNSPosix/build/prod/mDNSIdentify $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 755 -D $(MDNSRESPONDER_DIR)/mDNSPosix/build/prod/mDNSClientPosix $(TARGET_DIR)/usr/bin/
endef

mdnsresponder-clean:
	rm -f $(MDNSRESPONDER_DIR)/.configured $(MDNSRESPONDER_DIR)/.built $(MDNSRESPONDER_DIR)/.staged
	-$(MAKE1) os=linux -C $(MDNSRESPONDER_DIR)/mDNSPosix clean
	rm -f $(TARGET_DIR)/usr/sbin/mDNSResponderPosix \
		$(TARGET_DIR)/usr/sbin/mDNSNetMonitor \
		$(TARGET_DIR)/usr/sbin/mdnsd \
		$(TARGET_DIR)/usr/bin/dns-sd \
		$(TARGET_DIR)/usr/bin/mDNSProxyResponderPosix \
		$(TARGET_DIR)/usr/bin/mDNSIdentify \
		$(TARGET_DIR)/usr/bin/mDNSClientPosix \
		$(TARGET_DIR)/etc/init.d/S80mdns

mdnsresponder-dirclean:
	rm -rf $(MDNSRESPONDER_DIR)

$(eval $(generic-package))
