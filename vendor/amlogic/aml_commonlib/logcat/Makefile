#
## sample Makefile for logcat
#
#
LOGCAT_DIR ?= .
OBJ = logcat.o
CFLAGS = -Wall -Wextra -I $(LOGCAT_DIR)/include -DHAVE_IOCTL
LDFLAGS = -L$(STAGING_DIR)/usr/lib -llog
TARGET=logcat

# rules
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.cpp
	$(CC) -c $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -f $(OBJ)

install:
	install -m 755 $(TARGET) $(TARGET_DIR)/usr/bin
