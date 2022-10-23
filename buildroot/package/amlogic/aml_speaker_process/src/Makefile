target=speaker_process
all:
	$(CC) $(CFLAGS) $(LDFLAGS) $(target).cpp -o $(target) -lasound
clean:
	rm $(target)

install:
	install -m 0755 $(target) $(TARGET_DIR)/usr/bin/
	install -m 0755 speaker_processing.awb $(TARGET_DIR)/usr/bin/
uninstall:
	rm -f $(TARGET_DIR)/usr/bin/$(target)
