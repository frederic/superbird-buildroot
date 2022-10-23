TAR=libdolby_atmos.so
DOLBY_FW=dolby_fw_datmos
SCRIPT=S83dsd
CC = arm-linux-gnueabihf-gcc


ifeq ($(ENABLE_ATMOS_64bit),yes)
SRC_DOLBY_FW=dolby_fw_datmos_64
else
SRC_DOLBY_FW=dolby_fw_datmos_32
endif

all: $(TAR)

clean:
	rm -f $(TARGET_DIR)/sbin/$(DOLBY_FW)
	rm -f $(TARGET_DIR)/usr/lib/$(TAR)
	rm -f $(TARGET_DIR)/etc/init.d/$(SCRIPT)
install:
	install -m 0755 ./$(SRC_DOLBY_FW) $(TARGET_DIR)/sbin/$(DOLBY_FW)
	install -m 0755 ./$(TAR) $(TARGET_DIR)/usr/lib/
	install -m 0755 ./$(SCRIPT) $(TARGET_DIR)/etc/init.d/
uninstall:
	rm -f $(TARGET_DIR)/sbin/$(DOLBY_FW)
	rm -f $(TARGET_DIR)/usr/lib/$(TAR)
	rm -f $(TARGET_DIR)/etc/init.d/$(SCRIPT)
