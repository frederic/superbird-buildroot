TAR=libdolbyms12.so
DMS12_FW=dolby_fw_dms12
SCRIPT=S84dsms12
CC = arm-linux-gnueabihf-gcc


ifeq ($(ENABLE_MS12_64bit),yes)
SRC_DMS12_FW=dolby_fw_dms12_64bits
else
SRC_DMS12_FW=dolby_fw_dms12_32bits
endif

all: $(TAR)

clean:
	rm -f $(TARGET_DIR)/sbin/$(DMS12_FW)
	rm -f $(TARGET_DIR)/usr/lib/$(TAR)
	rm -f $(TARGET_DIR)/etc/init.d/$(SCRIPT)
install:
	install -m 0755 ./$(SRC_DMS12_FW) $(TARGET_DIR)/sbin/$(DMS12_FW)
	install -m 0755 ./$(TAR) $(TARGET_DIR)/usr/lib/
	install -m 0755 ./$(SCRIPT) $(TARGET_DIR)/etc/init.d/
uninstall:
	rm -f $(TARGET_DIR)/sbin/$(DOLBY_FW)
	rm -f $(TARGET_DIR)/usr/lib/$(TAR)
	rm -f $(TARGET_DIR)/etc/init.d/$(SCRIPT)
