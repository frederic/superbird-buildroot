TARGET := libsoundtouchcwrap.so
OBJS := libsoundtouchcwrap.o

$(TARGET):LDLIBS=$(shell $(PKG_CONFIG) --libs soundtouch)

$(TARGET):$(OBJS)
	$(CXX) -shared $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@
