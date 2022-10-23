DAEMON_NAME           = ptz_control
DAEMON_MAJOR_VERSION  = 1
DAEMON_MINOR_VERSION  = 1
DAEMON_PATCH_VERSION  = 0

COMMON_DIR        = ./

CFLAGS           := -DDAEMON_NAME='"$(DAEMON_NAME)"'
CFLAGS           += -DDAEMON_MAJOR_VERSION=$(DAEMON_MAJOR_VERSION)
CFLAGS           += -DDAEMON_MINOR_VERSION=$(DAEMON_MINOR_VERSION)
CFLAGS           += -DDAEMON_PATCH_VERSION=$(DAEMON_PATCH_VERSION)
CFLAGS           += -I$(COMMON_DIR)
CFLAGS           += -O2  -Wall  -pipe


# Add your source files to the list.
# Supported *.c  *.cpp  *.S files.
# For other file types write a template rule for build, see below.
SOURCES  = $(COMMON_DIR)main.c         \
           $(COMMON_DIR)ptzimpl.c

OBJECTS  := $(patsubst %.c,  %.o, $(SOURCES) )


.PHONY: all
all: release


.PHONY: release
release: CFLAGS := -s  $(CFLAGS)
release: $(DAEMON_NAME)


# release
$(DAEMON_NAME): $(OBJECTS)
	$(call build_bin, $(OBJECTS))


# Build release objects
%.o: %.c
	$(build_object)

.PHONY: clean
clean:
	-@rm -f $(DAEMON_NAME)
	-@rm -f $(OBJECTS)
	-@rm -f *.*~

.PHONY: install
install:
	-@cp $(DAEMON_NAME) $(DESTDIR)/bin/$(DAEMON_NAME)


# Common commands
BUILD_ECHO = echo "\n  [build]  $@:"


define build_object
	@$(BUILD_ECHO)
	$(CC) -c $< -o $@  $(CFLAGS)
endef


define build_bin
	@$(BUILD_ECHO)
	$(CC)  $1 -o $@  $(CFLAGS)
	@echo "\n---- Compiled $@ ver $(DAEMON_MAJOR_VERSION).$(DAEMON_MINOR_VERSION).$(DAEMON_PATCH_VERSION) ----\n"
endef


.PHONY: help
help:
	@echo "make [command]"
	@echo "command is:"
	@echo "   all       -  build daemon in release and debug mode"
	@echo "   release   -  build in release mode (strip)"
	@echo "   clean     -  remove all generated files"
	@echo "   help      -  this help"

