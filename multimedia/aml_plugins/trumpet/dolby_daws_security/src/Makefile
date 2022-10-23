TAR=dolby_fw
SCRIPT=S91dsd
CC = arm-linux-gnueabihf-gcc


all: $(TAR)

clean:
	rm -f $(TARGET_OUTPUT_DIR)/target/sbin/$(TAR)
	rm -f $(TARGET_OUTPUT_DIR)/target/etc/init.d/$(SCRIPT)
install:
	install -m 0755 ./$(TAR) $(TARGET_OUTPUT_DIR)/target/sbin/
	install -m 0755 ./$(SCRIPT) $(TARGET_OUTPUT_DIR)/target/etc/init.d/
uninstall:
	rm -f $(TARGET_OUTPUT_DIR)/target/sbin/$(TAR)
	rm -f $(TARGET_OUTPUT_DIR)/target/etc/init.d/$(SCRIPT)
