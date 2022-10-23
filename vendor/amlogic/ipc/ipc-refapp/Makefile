#LOCAL_BUILD=y
ifeq ($(LOCAL_BUILD),y)
export CC=$(TARGET_OUTPUT_DIR)/host/bin/aarch64-linux-gnu-gcc
export PKG_CONFIG_PATH=$(TARGET_OUTPUT_DIR)/host/aarch64-linux-gnu/sysroot/usr/lib/pkgconfig
export PKG_CONFIG=$(TARGET_OUTPUT_DIR)/host/bin/pkg-config
export CFLAGS += -I../ipc-sdk/inc
export LDFLAGS += -L../ipc-sdk/src
endif

SRC_DIRS = . \
					 common \
					 nn \
					 osd \
					 config \
					 pipeline \
					 recording \
					 isp_config \
					 imgenc \
					 day_night_switch \

CFLAGS += $(shell $(PKG_CONFIG) --cflags gstreamer-1.0 gstreamer-app-1.0 libwebsockets gstreamer-rtsp-server-1.0 sqlite3 libgpiod)
LDFLAGS += $(shell $(PKG_CONFIG) --libs gstreamer-1.0 gstreamer-app-1.0 libwebsockets gstreamer-rtsp-server-1.0 sqlite3 libgpiod)

CFLAGS += $(addprefix -I, $(SRC_DIRS))
CFLAGS += -Wall -Werror -fPIC -g -O2

LDFLAGS += -laml_ipc_sdk
LDFLAGS += -lnn_detect
LDFLAGS += -lipc-property
LDFLAGS += -lpthread -lm -ljpeg

VPATH = $(foreach dir,$(SRC_DIRS),$(dir):)
SRCS = $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))

OBJDIR = objs
OBJS = $(addprefix $(OBJDIR)/,$(notdir $(SRCS:.c=.o)))

SUFFIXES += .d
DEPFILES := $(OBJS:.o=.d)
NODEPS := clean distclean

TARGET = ipc-refapp
all: $(TARGET)

ifeq "$(findstring $(MAKECMDGOALS),$(NODEPS))"  ""
  -include $(DEPFILES)
endif

$(TARGET): $(OBJS)
	@echo " [LINK] $@"
	@$(CC) -o $@ $(LDFLAGS) $^

$(OBJDIR)/%.o: %.c $(OBJDIR)/%.d
	@echo " [CC] $<"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/%.d: %.c
	@echo " [DEP] $<"
	@mkdir -p $(OBJDIR)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -M -MT "$(patsubst %.c,$(OBJDIR)/%.o,$<)" $< -MF $@

.PHONY:clean

clean:
	@echo " [CLEAN] ..."
	@rm -rf $(OBJDIR) $(TARGET)
