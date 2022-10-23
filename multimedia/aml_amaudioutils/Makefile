OBJS=src/primitives.o \
	src/resampler.o \
	src/speexresample/resample.o \
	src/spdif/AC3FrameScanner.o \
	src/spdif/BitFieldParser.o \
	src/spdif/DTSFrameScanner.o \
	src/spdif/FrameScanner.o \
	src/spdif/SPDIFEncoder.o

CUTILS_OBJS=src/cutils/hashmap.o \
	src/cutils/properties.o \
	src/cutils/str_parms.o \
	src/cutils/threads.o \
	src/cutils/strlcpy.o

CFLAGS+=-fPIC -O2 -I./include -I./include/speex -I. -I./src -I./src/ -mfpu=neon -DFIXED_POINT -DRESAMPLE_FORCE_FULL_SINC_TABLE -D_USE_NEON -DEXPORT=
LDFLAGS+=-llog -ldl -lrt -lpthread -lstdc++

%.o: %.cpp
	$(CC) -c $(CFLAGS) $(CXXFLAGS) -o $@ $<

%.o: %.cc
	$(CC) -c $(CFLAGS) $(CXXFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

all: libamaudioutils.so libcutils.so

libamaudioutils.so: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -o $@ $^

libcutils.so: $(CUTILS_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -o $@ $^

.PHONY: install
install:
	install -m 644 -D libamaudioutils.so -t $(STAGING_DIR)/usr/lib
	install -m 644 -D libamaudioutils.so -t $(TARGET_DIR)/usr/lib
	install -m 644 -D libcutils.so -t $(STAGING_DIR)/usr/lib
	install -m 644 -D libcutils.so -t $(TARGET_DIR)/usr/lib
	for f in $(@D)/include/audio_utils/*.h; do \
		install -m 644 -D $${f} -t $(STAGING_DIR)/usr/include/audio_utils; \
	done
	for f in $(@D)/include/audio_utils/spdif/*.h; do \
		install -m 644 -D $${f} -t $(STAGING_DIR)/usr/include/audio_utils/spdif; \
	done
	for f in $(@D)/include/cutils/*.h; do \
		install -m 644 -D $${f} -t $(STAGING_DIR)/usr/include/cutils; \
	done
	for f in $(@D)/include/android/*.h; do \
		install -m 644 -D $${f} -t $(STAGING_DIR)/usr/include/android; \
	done

.PHONY: clean
clean:
	rm -rf $(STAGING_DIR)/usr/include/audio_utils
	rm -rf $(STAGING_DIR)/usr/include/cutils
	rm -f libamaudioutils.so
	rm -f $(TARGET_DIR)/usr/lib/libamaudioutils.so
	rm -f $(STAGING_DIR)/usr/lib/libamaudioutils.so

