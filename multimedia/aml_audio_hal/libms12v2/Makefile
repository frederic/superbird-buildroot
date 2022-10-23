################################################################################
# Device/Makefile
#
# This Makefile builds the libs
#
################################################################################
OUT_ROOT ?= dist

AUDIO_HAL_DIR = $(AML_COMMON_DIR)/audio_hal
LIBMS12_DIR = $(AML_COMMON_DIR)/libms12
CUTILS_DIR = $(AML_COMMON_DIR)/cutils

DEPENDS := cutils
DEPENDS := $(addsuffix /build, $(addprefix $(ROKU_PORT_DIR)/dist/build/, $(DEPENDS)))
override LDFLAGS += $(addprefix -L, $(DEPENDS))
override CFLAGS += -DREPLACE_OUTPUT_BUFFER_WITH_CALLBACK

################################################################################
# libms12.so - common src files
################################################################################
LIBMS12_SRC_DIR = $(LIBMS12_DIR)/src
libms12_SRCS = \
	$(LIBMS12_SRC_DIR)/aml_audio_ms12.c \
	$(LIBMS12_SRC_DIR)/dolby_ms12_config_params.cpp \
	$(LIBMS12_SRC_DIR)/DolbyMS12ConfigParams.cpp \
	$(LIBMS12_SRC_DIR)/dolby_ms12.cpp \
	$(LIBMS12_SRC_DIR)/DolbyMS12.cpp \
	$(LIBMS12_SRC_DIR)/dolby_ms12_status.cpp \
	$(LIBMS12_SRC_DIR)/DolbyMS12Status.cpp \
	$(NULL)

#TODO: Add more include directories here, if necessary.
INCLUDES += \
	-I$(LIBMS12_DIR)/include \
	-I$(AUDIO_HAL_DIR)/include \
	-I$(CUTILS_DIR)/include

ALL_SRCS = $(libms12_SRCS)

include $(ROKU_PORT_COMMON_DIR)/build/define_target.mk

comma = ,
$(eval $(call DEFINE_TARGET,libms12.so,$(libms12_SRCS),,-lpthread -ldl -lstdc++ -lcutils -fPIC -shared -Wl$(comma)-z$(comma)defs -Wl$(comma)--build-id,lib))

# ---------------------------------------------------------------------
#  Build rules
all: $(VENDOR_ALL_TARGETS)
# Compile .c file.
# Simultaneously create .d file (see http://make.mad-scientist.net/papers/advanced-auto-dependency-generation).
$(OUT_ROOT)/%.o: %.c $(OUT_ROOT)/%.d
	mkdir -p $(dir $@)
	$(CC) -MT $@ -MMD -MP -MF $(OUT_ROOT)/$*.Td  $(CFLAGS) $(INCLUDES) -c -o $@ $<
	mv -f $(OUT_ROOT)/$*.Td $(OUT_ROOT)/$*.d && touch $@
# Compile .cpp file.
# Simultaneously create .d file (see http://make.mad-scientist.net/papers/advanced-auto-dependency-generation).
$(OUT_ROOT)/%.o: %.cpp $(OUT_ROOT)/%.d
	@mkdir -p $(dir $@)
	$(CXX) -MT $@ -MMD -MP -MF $(OUT_ROOT)/$*.Td  $(CFLAGS) $(INCLUDES) -c -o $@ $<
	mv -f $(OUT_ROOT)/$*.Td $(OUT_ROOT)/$*.d && touch $@
$(OUT_ROOT)/%.d: ;
.PRECIOUS: $(OUT_ROOT)/%.d
.PHONY: clean distclean install

include $(wildcard $(patsubst %,$(OUT_ROOT)/%.d,$(basename $(libms12_SRCS))))

clean:
	-rm -rf $(OUT_ROOT) $(CLEAN_FILES)
	mkdir -p $(OUT_ROOT)

distclean:
	-rm   $(DESTDIR)/usr/lib/libms12.so

install:
	mkdir -p $(DESTDIR)/usr/lib
	cp $(OUT_ROOT)/libms12.so  $(DESTDIR)/usr/lib/
