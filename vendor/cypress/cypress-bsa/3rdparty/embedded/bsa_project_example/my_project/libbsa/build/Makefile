#######################################
#  This makefile is written for a GNU make or equivalent.
#  For example:
#   make clean
#   make
# Copyright (c) 2010-2015, Broadcom Corp., All Rights Reserved.
# Broadcom Bluetooth Core. Proprietary and confidential.
#######################################
# Basic path definitions
GLOBALPATH := ../../../../../..
customer := bsa_project_example
project := my_project
application := libbsa
os_name := linux
BLUETOOTH_3RD_DIR := $(GLOBALPATH)/3rdparty

#
# ------------------    Executables   ----------------------
#

# include common makefile rules
include $(BLUETOOTH_3RD_DIR)/embedded/bsa_examples/linux/app_common/build/make.common

# Target definition
target := $(application)
targetfile := $(addsuffix .txt,$(basename $(target)))
targetbase := $(notdir $(basename $(target)))

# Include the configuration file for the target here
include $(targetfile)

# Global compilation define
GLOBAL_DEFINE = -DBSA_CLIENT

# Targets
cfgfile := $(cfgincdir)/buildcfg.h

# base directory source, library and target lists
srcdirs :=
incdirs := . $(STD_INCPATH)  \
             $(projectincdir)\
             $(clientincdir) \
             $(comincdir)    \
             $(appincdir)    \
             $(btaincdir)    \
             $(stackincdir)  \
             $(cfgincdir)

# Initialize the source names
appsources :=
gkisources :=
cl_drv_sources :=
bsasources :=

# include variable definitions for each component
include $(BLUETOOTH_BSA_CLIENT_DIR)/build/def/*.def
include $(BLUETOOTH_APPLICATION_DIR)/build/def/*.def

# incdirs uses srcdirs
incdirs := $(incdirs) $(srcdirs)

# All source files
srclist := $(gkisources) $(cl_drv_sources) $(bsasources)

bsastaticblddir := $(BSA_LIB_DIR)/build/$(CPU)
bsasharedblddir := $(BSA_LIB_DIR)/build/$(CPU)/sharedlib
bsastaticobjdir := $(bsastaticblddir)/obj
bsasharedobjdir := $(bsasharedblddir)/obj

vpath %.c $(srcdirs)

# Targets
.PHONY: all
all: $(bsastaticlib) $(bsasharedlib)

staticobjlist := $(patsubst %.c,$(bsastaticobjdir)/%.o,$(srclist))

sharedobjlist := $(patsubst %.c,$(bsasharedobjdir)/%.o,$(srclist))

# include all object dependency files if present
-include $(staticobjlist:.o=.d)
-include $(sharedobjlist:.o=.d)

# headers
libbsahdrdirs := $(bsadir)/include $(bsadir)/udrv $(btaincdir) $(gkidir)/common \
    $(BLUETOOTH_BSA_DIR)/gki/$(os_name) $(componentdir)/udrv/include \
    $(maindir) $(stackincdir)  $(stackdir)/brcm
libbsahdrfiles := $(wildcard $(addsuffix /*.h, $(libbsahdrdirs)))
libbsahdrfiles += $(bsadir)/common/bsa_trace.h
libbsahdrfiles += $(cfgfile)
libbsahdrfiles := $(addprefix $(cfgincdir)/, $(notdir $(libbsahdrfiles)))

vpath %.h $(libbsahdrdirs) $(bsadir)/common
$(cfgincdir)/%: %
	@$(ECHO) Updating $@
	@cp -f $^ $@

$(libbsahdrfiles): | $(cfgincdir)
headers: $(libbsahdrfiles)

$(bsastaticobjdir):
	@$(ECHO) Creating static object directory '$(bsastaticobjdir)'.
	-@$(MKDIR) -p $(bsastaticobjdir)

$(bsasharedobjdir):
	@$(ECHO) Creating shared object directory '$(bsasharedobjdir)'.
	-@$(MKDIR) -p $(bsasharedobjdir)

$(cfgincdir):
	@$(ECHO) Creating Project Include folder '$(cfgincdir)'.
	-@$(MKDIR) -p $(cfgincdir)

$(bsastaticobjdir)/%.o: %.c | headers
	@$(ECHO) Compiling [$(CC)] $<
	@$(CC) -c -MD $(CCFLAGS) $(GLOBAL_DEFINE) $(TARGET_DEF) $(CCINC) -o $@ $<

$(bsasharedobjdir)/%.o: %.c | headers
	@$(ECHO) Compiling [$(CC)] $<
	@$(CC) -fPIC -c -MD $(CCFLAGS) $(GLOBAL_DEFINE) $(TARGET_DEF) $(CCINC) -o $@ $<

$(cfgfile): $(targetfile) | $(cfgincdir)
	@$(ECHO) Creating $@ from $<
	@$(RM) -f $<.tmp; cp $< $<.tmp; chmod +w $<.tmp; dos2unix $<.tmp; \
	$(ECHO) "#ifndef BUILDCFG_H" > $@; \
	$(ECHO) "#define BUILDCFG_H" >> $@; \
	$(ECHO) >> $@; \
	$(SED) -e '/^#/d' -e '/^$$/d' -e '/# Makefile only$$/d' -e 's/^/#define /' -e 's/=/ /' $<.tmp >> $@; \
	$(ECHO) >> $@; \
	$(ECHO) "#endif" >> $@; \
	$(RM) -f $<.tmp

$(bsastaticlib): $(bsastaticobjdir) $(staticobjlist) | headers
	@$(ECHO) Creating [$(AR)] BSA static lib $(bsastaticlib)
	@$(AR) rcs $(bsastaticlib) $(staticobjlist)

$(bsasharedlib): $(bsasharedobjdir) $(sharedobjlist) | headers
	@$(ECHO) Linking [$(LINK)] BSA shared lib $(bsasharedlib)
	@$(LINK) $(LINKFLAGS) -shared $(sharedobjlist) -o $(bsasharedlib) $(LINKLIBS)

.PHONY: clean
clean:
	@$(ECHO) Cleaning up.
	@$(RM) -rf $(bsastaticblddir)
	@$(RM) -rf $(bsasharedblddir)
	@$(RM) -rf $(cfgincdir)/*.h

.PHONY: delivery
delivery:
	@$(ECHO) Creating Delivery in $(DELIVERY_CURRENT_FOLDER)
	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)
	@$(CP) -r --remove-destination $(BLUETOOTH_3RD_DIR)/embedded/$(customer)/$(project)/$(application) $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/$(application)

	@find $(DELIVERY_CURRENT_FOLDER) -name "*.o" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "*.d" | xargs rm -f
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
	@$(ECHO) Delivery done.
	

