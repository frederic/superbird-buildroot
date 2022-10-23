#######################################
#  This makefile is written for a GNU make or equivalent.
#  For example:
#   make clean
#   make
# Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
# Broadcom Bluetooth Core. Proprietary and confidential.
#######################################
# Basic path definitions
GLOBALPATH := ../../../../../..
customer := bsa_project_example
project := my_project
application := bsa_server
os_name := linux
BLUETOOTH_3RD_DIR := $(GLOBALPATH)/3rdparty

# Extract GCC Major version
GCC_VERSION_MAJOR := $(shell gcc --version | grep ^gcc | sed 's/^.* //g' | cut -f1 -d.)
# Extract GCC Minor version
GCC_VERSION_MINOR := $(shell gcc --version | grep ^gcc | sed 's/^.* //g' | cut -f2 -d.)

# Test if Major is Equal to 4
GCC_MAJOR_EQ4 := $(shell expr $(GCC_VERSION_MAJOR) = 4)
# Test if Major is Greater Than 4
GCC_MAJOR_GT4 := $(shell expr $(GCC_VERSION_MAJOR) \> 4)
# Test if Minor is Greater Or Equal to 6
GCC_MINOR_GTEQ6 := $(shell expr $(GCC_VERSION_MINOR) \>= 6)

# If GCC Major Version is Equal to 4
ifeq "$(GCC_MAJOR_EQ4)" "1"
# And if GCC Minor Version is Greater or Equal to 6 
ifeq "$(GCC_MINOR_GTEQ6)" "1"
    ARMCCFLAGS += -Wno-unused-but-set-variable
    X86CCFLAGS += -Wno-unused-but-set-variable
    MIPSCCFLAGS += -Wno-unused-but-set-variable
endif
else # GCC Major Version is Not Equal to 4
# If Major Version is Greater Than 4
ifeq "$(GCC_MAJOR_GT4)" "1"
    ARMCCFLAGS += -Wno-unused-but-set-variable
    X86CCFLAGS += -Wno-unused-but-set-variable
    MIPSCCFLAGS += -Wno-unused-but-set-variable
endif
endif

# include common makefile rules (from the common directory)
include $(BLUETOOTH_3RD_DIR)/embedded/bsa_examples/linux/app_common/build/make.common

# Target definition
target := $(application)
targetfile := $(addsuffix .txt,$(basename $(target)))
targetbase := $(notdir $(basename $(target)))

# Include the configuration file for the target here
include $(targetfile)

# Global compilation define
GLOBAL_DEFINE := -DBSA_SERVER

# Targets
cfgfile := $(cfgincdir)/buildcfg.h
bluetooth_app := $(exedir)/$(target)

# base directory source, library, and target lists
srcdirs :=
incdirs := . $(STD_INCPATH)  \
             $(stackincdir)  \
             $(builddir)     \
             $(btaincdir)    \
             $(projectincdir)\
             $(serverincdir) \
             $(comincdir)    \
             $(cfgincdir)

# Initialize
appsources :=
gkisources :=
compresssrcs :=
stacksources := 
idlsrc := 
bcspsources :=
sv_drv_sources :=
btesources :=
btasources :=
applibs := 
libs := 
btui_serial_sources :=
btapp_sources :=
bsasources :=

all_targets := $(cfgfile)

# include variable definitions for each component
include $(BLUETOOTH_PRJ_DIR)/bte/build/def/*.def
include $(BLUETOOTH_BSA_SERVER_DIR)/build/def/*.def
include $(BLUETOOTH_APPLICATION_DIR)/build/def/*.def


# incdirs uses srcdirs (therefore must be defined after included .def files)
incdirs := $(incdirs) $(srcdirs)


# Objects
app_objs := $(patsubst %.c,$(objdir)/%.o,$(appsources))

bteobjs := $(patsubst %.c,$(objdir)/%.o,$(btesources))

stackobjs := $(patsubst %.c,$(objdir)/%.o,$(stacksources))

gkiobjs := $(patsubst %.c,$(objdir)/%.o,$(gkisources))

btaobjs := $(patsubst %.c,$(objdir)/%.o,$(btasources))

sv_drv_objs := $(patsubst %.c,$(objdir)/%.o,$(sv_drv_sources))

btapp_objs := $(patsubst %.c,$(objdir)/%.o,$(btapp_sources))

bsaobjs := $(patsubst %.c,$(objdir)/%.o,$(bsasources))

# Platform specific objects files generation
btplatobjs := $(patsubst %.c,$(objdir)/%.o,$(btplatformsources))

vpath %.c $(srcdirs)
vpath %.o $(objdir)

# Targets
.PHONY: all
all: $(cfgfile) $(bluetooth_app) $(xml_config_builder)

$(bluetooth_app): $(cfgfile)

objlist := $(gkiobjs) \
           $(sv_drv_objs) \
           $(stackobjs) \
           $(btaobjs) \
           $(bteobjs) \
           $(bsaobjs) \
           $(btapp_objs) \
           $(btplatobjs) \
           $(btudrvobjs) \
           $(app_objs)

# include all object dependency files if present
-include $(objlist:.o=.d)

$(objdir):
	@$(ECHO) Creating object directory '$(objdir)'.
	-@$(MKDIR) -p $(objdir)

$(cfgincdir):
	@$(ECHO) Creating Project Include folder '$(cfgincdir)'.
	-@$(MKDIR) -p $(cfgincdir)

$(objdir)/%.o: %.c $(cfgfile)
	@$(ECHO) Compiling [$(CC)] $<
	@$(CC) -c -MD $(CCFLAGS) $(GLOBAL_DEFINE) $(TARGET_DEF) $(CCINC) -o $@ $<

$(cfgfile): $(targetfile) | $(cfgincdir)
	@$(ECHO) Creating $@ from $<
	@rm -f $<.tmp; cp $< $<.tmp; chmod +w $<.tmp; dos2unix $<.tmp; \
	$(ECHO) "#ifndef BUILDCFG_H" > $@; \
	$(ECHO) "#define BUILDCFG_H" >> $@; \
	$(SED) -e '/^#/d' -e '/^$$/d' -e '/# Makefile only$$/d' -e 's/^/#define /' -e 's/=/ /' $<.tmp >> $@; \
	$(ECHO) "#endif" >> $@; \
	rm -f $<.tmp

$(objlist): | $(objdir)

$(bluetooth_app): $(objlist)
	@$(ECHO) Linking application $@
	@$(LINK) $(LINKFLAGS) -o $@ $(objlist) $(LINKLIBS)

.PHONY: clean
clean:
	@$(ECHO) Cleaning up.
	@$(RM) -rf $(objdir) $(cfgfile)
	@$(RM) -rf $(bluetooth_app) $(cfgfile)

# The project delivery target a regular delivery (bsa_example delivey ) 
.PHONY: delivery
delivery: $(DELIVERY_CURRENT_FOLDER) $(DELIVERY_CURRENT_FOLDER)
	@$(ECHO) Adding $(customer)/$(project)/$(application) Delivery in $(DELIVERY_CURRENT_FOLDER)
	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)
	@find $(BLUETOOTH_3RD_DIR)/embedded/$(customer)/$(project)/$(application) -name "bt-daemon-socket" | xargs rm -f
	@find $(BLUETOOTH_3RD_DIR)/embedded/$(customer)/$(project)/$(application) -name "bt-*-fifo" | xargs rm -f
	@$(CP) -r --remove-destination $(BLUETOOTH_3RD_DIR)/embedded/$(customer)/$(project)/$(application) $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/$(application)
	@$(ECHO) Removing files not needed \(sockets, fifo, xml, object, etc.\) 
	@find $(DELIVERY_CURRENT_FOLDER) -name "bt-*-socket" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "bt-*-fifo" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "*.xml" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "*.bin" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "*.o" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "*.d" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "*.keep" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name ".copyarea.db" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "*~" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "core" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name ".*.cmd" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "*.ko" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "*.mod.c" | xargs rm -f 
	@find $(DELIVERY_CURRENT_FOLDER) -name ".tmp_versions" | xargs rm -f -r
	@find $(DELIVERY_CURRENT_FOLDER) -name "Module.markers" | xargs rm -f -r
	@find $(DELIVERY_CURRENT_FOLDER) -name "Module.symvers" | xargs rm -f -r
	@find $(DELIVERY_CURRENT_FOLDER) -name "modules.order" | xargs rm -f -r
	@find $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/$(application)/build/test_files -type f -size +1024k | xargs rm -f
	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/release_notes
	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/release_notes/$(customer)
	@if [ -f $(GLOBALPATH)/release_notes/$(customer)/Release_$(application).txt ]; then \
		echo "    Binary Release notes delivery from source delivery"; \
		$(CP) $(GLOBALPATH)/release_notes/$(customer)/Release_$(application).txt $(DELIVERY_CURRENT_FOLDER)/release_notes/$(customer); \
	elif [ -f $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/$(application)/build/Release_$(application).txt ]; then \
		echo "    Binary Release notes delivery from vobs"; \
		$(MV) $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/$(application)/build/Release_$(application).txt $(DELIVERY_CURRENT_FOLDER)/release_notes/$(customer); \
	else \
		echo "    Nothing to do for release notes"; \
	fi
	@$(ECHO) Delivery $(customer) done.

.PHONY: builddelivery
builddelivery: cleandelivery
	@$(ECHO) Compiling the delivery for verification
	$(MAKE) -C $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/$(application)/build -f Makefile.all all

.PHONY: cleandelivery
cleandelivery:
	@$(ECHO) Removing Delivery objects
	$(MAKE) -C $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/$(application)/build -f Makefile.all clean

