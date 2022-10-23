BUILD_TARGETS = systemcontrol display_util

.PHONY: all install clean

systemcontrol: main_systemcontrol.cpp SystemControl.cpp DisplayMode.cpp TxUEventCallback.cpp \
	HDCPTxAuth.cpp UEventObserver.cpp SysWrite.cpp FrameRateAutoAdaption.cpp \
	SysTokenizer.cpp ubootenv.c FormatColorDepth.cpp
	$(CXX) $(CFLAGS) $(LDFLAGS) -lpthread -lz -o $@ $^ $(LDLIBS)

display_util: main_client.cpp
	$(CXX) $(CFLAGS) $(LDFLAGS) -lpthread -o $@ $^ $(LDLIBS)

all: $(BUILD_TARGETS)

clean:
	rm -f *.o $(BUILD_TARGETS)

install:
	install -m 755 $(BUILD_TARGETS) $(TARGET_DIR)/usr/bin/

uninstall:
	rm -f $(TARGET_DIR)/usr/bin/$(BUILD_TARGETS)
