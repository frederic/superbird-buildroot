################################################################################
# Device/Makefile
#
# This Makefile builds the libs
#
################################################################################
OUT_ROOT ?= dist

CUTILS_DIR = $(AML_COMMON_DIR)/cutils

################################################################################
# libcutils.so - common src files
################################################################################
CUTILS_SRC_DIR = $(CUTILS_DIR)
cutils_SRCS = \
	$(CUTILS_SRC_DIR)/hashmap.cpp \
	$(CUTILS_SRC_DIR)/strlcpy.c \
	$(CUTILS_SRC_DIR)/str_parms.cpp \
	$(CUTILS_SRC_DIR)/threads.cpp \
	$(CUTILS_SRC_DIR)/properties.cpp \
	$(NULL)

liblog_SRCS += $(wildcard $(CUTILS_SRC_DIR)/liblog/*.c)
cutils_SRCS += $(liblog_SRCS)

logcat_SRCS = $(CUTILS_SRC_DIR)/logcat.cpp $(liblog_SRCS)

#TODO: Add more include directories here, if necessary.
INCLUDES += \
	-I$(CUTILS_DIR)/include \
	-I$(CUTILS_DIR)/liblog/include
CFLAGS += -DHAVE_SYS_UIO_H -DHAVE_IOCTL

ALL_SRCS = $(libcutils_SRCS) $(logcat_SRCS)

include $(ROKU_PORT_COMMON_DIR)/build/define_target.mk

comma = ,
$(eval $(call DEFINE_TARGET,libcutils.so,$(cutils_SRCS),,-lpthread -lstdc++ -fPIC -shared -Wl$(comma)-z$(comma)defs -Wl$(comma)--build-id,lib))
$(eval $(call DEFINE_TARGET,logcat,$(logcat_SRCS),,-lpthread -lstdc++ -fPIC -Wl$(comma)-z$(comma)defs -Wl$(comma)--build-id,lib))

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

include $(wildcard $(patsubst %,$(OUT_ROOT)/%.d,$(basename $(cutils_SRCS))))

clean:
	-rm -rf $(OUT_ROOT) $(CLEAN_FILES)
	mkdir -p $(OUT_ROOT)

distclean:
	-rm   $(DESTDIR)/usr/lib/libcutils.so
	-rm   $(DESTDIR)/usr/bin/logcat

install:
	mkdir -p $(DESTDIR)/usr/lib
	cp $(OUT_ROOT)/libcutils.so  $(DESTDIR)/usr/lib/
	cp $(OUT_ROOT)/logcat $(DESTDIR)/usr/bin
