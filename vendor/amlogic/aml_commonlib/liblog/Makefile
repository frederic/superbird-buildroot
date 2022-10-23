
LIBLOG_SRC_DIR ?= .
OBJC = logd_write.o logprint.o event_tag_map.o fake_log_device.o
CFLAGS += -fPIC -I$(LIBLOG_SRC_DIR)/include/
LDFLAGS += -L./lib -lglibc_bridge -lc++.so -lc++abi.so
TARGET=liblog.so

# rules
all: $(TARGET)

$(TARGET): $(OBJC)
	$(CC) -shared  -o $@ $^

$(OBJC):%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean

clean:
	rm -f $(OBJC) $(TARGET)

install:
	install -m 755 $(TARGET) $(STAGING_DIR)/usr/lib
	install -m 755 $(TARGET) $(TARGET_DIR)/usr/lib
	mkdir -p $(STAGING_DIR)/usr/include/android
	mkdir -p $(STAGING_DIR)/usr/include/cutils
	install -m 755 $(LIBLOG_SRC_DIR)/include/android/*  $(STAGING_DIR)/usr/include/android/
	install -m 755 $(LIBLOG_SRC_DIR)/include/cutils/*  $(STAGING_DIR)/usr/include/cutils/
