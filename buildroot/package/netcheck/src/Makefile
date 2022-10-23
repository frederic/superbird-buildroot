target=netcheck
all:
	$(CC) -Wall $(target).c -o $(target)
clean:
	rm $(target)

install:
	install -m 0755 $(target) $(TARGET_DIR)/usr/bin/
	install -m 0755 netmonitor $(TARGET_DIR)/sbin/
	install -m 0755 S77netmonitor $(TARGET_DIR)/etc/init.d/
uninstall:
	rm -f $(TARGET_DIR)/usr/bin/$(target)
