#CC := gcc
CC := arm-linux-gnueabihf-gcc
CFLAGS := -I. -O2 -Wall -funroll-loops -ffast-math -fPIC -DPIC
#LD := gcc
LD = $(CC)
LDFLAGS := -O2 -Wall -shared
#LDLIBS_release := -lasound

SND_PCM_OBJECTS = daws_alsa_plugin.o snd_convert.o aml_ringbuffer.o
SND_PCM_LIBS = -lasound
SND_PCM_BIN = libasound_module_pcm_daws.so

.PHONY: all clean install uninstall

all: $(SND_PCM_BIN)

$(SND_PCM_BIN): $(SND_PCM_OBJECTS)
	@echo LD $@
	$(LD) $(LDFLAGS) $(SND_PCM_LIBS) $(SND_PCM_OBJECTS) -o $(SND_PCM_BIN)

%.o: %.c
	@echo GCC $<
	$(CC) -c $(CFLAGS) $<

clean:
	@echo Cleaning...
	$(Q)rm -vf *.o *.so
	$(RM) libasound_module_pcm_daws.so

install: all
	@echo Installing...
	install -m 755 $(SND_PCM_BIN) $(TARGET_DIR)/usr/lib/alsa-lib/

uninstall:
	@echo Un-installing...
	rm $(TARGET_DIR)/usr/lib/alsa-lib/$(SND_PCM_BIN)
