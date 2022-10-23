DEPS = rc_common.h keydefine.h
OBJ = main.o device.o parse.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

remotecfg: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ remotecfg

.PHONY: install

install:
	cp -f remotecfg $(TARGET_DIR)/usr/bin/
	cp -f remote.tab $(TARGET_DIR)/etc
	cp -f remote-mouse.tab $(TARGET_DIR)/etc

.PHONY: uninstall

uninstall:
	rm -f $(TARGET_DIR)/usr/bin/remotecfg
