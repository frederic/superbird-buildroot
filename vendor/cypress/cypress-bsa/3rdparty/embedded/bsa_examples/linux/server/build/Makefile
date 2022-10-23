#######################################
#  This makefile is written for a GNU make or equivalent.
#  For example:
#   make clean
#   make
# Copyright (c) 2010, Broadcom Corp., All Rights Reserved.
# Broadcom Bluetooth Core. Proprietary and confidential.
#######################################
# Basic path definitions
GLOBALPATH := ../../../../../..
customer := bsa_examples
project := linux
application := server
os_name := linux
BLUETOOTH_3RD_DIR := $(GLOBALPATH)/3rdparty

# include common makefile rules
include $(BLUETOOTH_3RD_DIR)/embedded/$(customer)/$(project)/app_common/build/make.common

# Target definition
target := bsa_$(application)
targetfile := $(addsuffix .txt,$(basename $(target)))
targetbase := $(notdir $(basename $(target)))

# Include the configuration file for the target here.
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

# initialize the source lists
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
all: $(cfgfile) $(bluetooth_app)

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

$(cfgfile): $(targetfile) $(cfgincdir)
	@$(ECHO)
	@$(ECHO) Creating $@ from $<
	@rm -f $<.tmp; cp $< $<.tmp; chmod +w $<.tmp; dos2unix $<.tmp; \
	$(ECHO) "#ifndef BUILDCFG_H" > $@; \
	$(ECHO) "#define BUILDCFG_H" >> $@; \
	$(SED) -e '/^#/d' -e '/^$$/d' -e '/# Makefile only$$/d' -e 's/^/#define /' -e 's/=/ /' $<.tmp >> $@; \
	$(ECHO) "#endif" >> $@; \
	rm -f $<.tmp
	@$(ECHO)

$(objlist): | $(objdir)

$(bluetooth_app): $(objlist)
	@$(ECHO) Linking application $@
	@$(LINK) $(LINKFLAGS) -o $@ $(objlist) $(LINKLIBS)

.PHONY: clean
clean:
	@$(ECHO) Cleaning up.
	@$(RM) -rf $(objdir) $(cfgfile)
	@$(RM) -rf $(bluetooth_app) $(cfgfile)

# define the delivery folder to copy
DELIVERY_BTA_FOLDER_LIST :=  3ds ag av avk ar bav bi brcm dg dm fs ft gatt gp gps hd hh hl hs op pan pb rc rt sc sys ma mip fm include
DELIVERY_STACK_FOLDER_LIST := a2dp avct avdt avrc bip bnep pan brcm btm btu dun ftp gap gatt goep hcic hid hsp2 l2cap mcap obx opp rfcomm sap sdp smp spp vdp srvc wbt xml fm include
DELIVERY_EMBDRV_FOLDER_LIST := sbc/encoder/srce sbc/encoder/include sbc/decoder/srce sbc/decoder/include slip

.PHONY: delivery
delivery:
	@$(ECHO) Creating Delivery in $(DELIVERY_CURRENT_FOLDER)
	@$(MKDIR) -p $(DELIVERIES_MAIN_FOLDER)
	@$(RM)   -rf $(DELIVERY_CURRENT_FOLDER)
	@$(MKDIR) $(DELIVERY_CURRENT_FOLDER)
	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/Components/embdrv
	@$(CD) $(DELIVERY_CURRENT_FOLDER)/Components/embdrv ; \
	$(MKDIR) -p $(DELIVERY_EMBDRV_FOLDER_LIST)

	@$(ECHO) Copy Components folder
	@for dir in $(DELIVERY_BTA_FOLDER_LIST); do \
		$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/Components/bta/$$dir; \
		$(CP) -r $(BLUETOOTH_COMP_DIR)/bta/$$dir/* $(DELIVERY_CURRENT_FOLDER)/Components/bta/$$dir; \
	done

	@for dir in $(DELIVERY_STACK_FOLDER_LIST); do \
		$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/Components/stack/$$dir; \
		$(CP) -r $(BLUETOOTH_COMP_DIR)/stack/$$dir/* $(DELIVERY_CURRENT_FOLDER)/Components/stack/$$dir; \
	done

	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/Components/hcis $(DELIVERY_CURRENT_FOLDER)/Components/gki/common $(DELIVERY_CURRENT_FOLDER)/Components/udrv/include
	@$(CP) -r $(BLUETOOTH_COMP_DIR)/hcis/* $(DELIVERY_CURRENT_FOLDER)/Components/hcis
	@$(CP) -r $(BLUETOOTH_COMP_DIR)/embdrv/sbc/decoder/include/* $(DELIVERY_CURRENT_FOLDER)/Components/embdrv/sbc/decoder/include
	@$(CP) -r $(BLUETOOTH_COMP_DIR)/embdrv/sbc/encoder/include/* $(DELIVERY_CURRENT_FOLDER)/Components/embdrv/sbc/encoder/include
	@$(CP) -r $(BLUETOOTH_COMP_DIR)/embdrv/sbc/decoder/srce/* $(DELIVERY_CURRENT_FOLDER)/Components/embdrv/sbc/decoder/srce
	@$(CP) -r $(BLUETOOTH_COMP_DIR)/embdrv/sbc/encoder/srce/* $(DELIVERY_CURRENT_FOLDER)/Components/embdrv/sbc/encoder/srce
	@$(CP) -r $(BLUETOOTH_COMP_DIR)/gki/common/* $(DELIVERY_CURRENT_FOLDER)/Components/gki/common
	@$(CP) -r $(BLUETOOTH_COMP_DIR)/udrv/include/* $(DELIVERY_CURRENT_FOLDER)/Components/udrv/include

	@$(ECHO) Copy Projects folder
	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/Projects/bte/main $(DELIVERY_CURRENT_FOLDER)/Projects/bte/build/def
	@$(CP) $(BLUETOOTH_PRJ_DIR)/bte/main/bt*.h $(DELIVERY_CURRENT_FOLDER)/Projects/bte/main/
	@$(CP) $(BLUETOOTH_PRJ_DIR)/bte/main/gki_target.h $(DELIVERY_CURRENT_FOLDER)/Projects/bte/main/
	@$(CP) $(BLUETOOTH_PRJ_DIR)/bte/main/bte_hcisu.c $(DELIVERY_CURRENT_FOLDER)/Projects/bte/main/
	@$(CP) -r $(BLUETOOTH_PRJ_DIR)/bte/build/def/* $(DELIVERY_CURRENT_FOLDER)/Projects/bte/build/def

	@$(ECHO) Copy 3rdparty folder
	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/brcm
	@$(CP) -r $(BLUETOOTH_3RD_DIR)/embedded/brcm/bsa $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/brcm/bsa

	@$(ECHO) Copy qt_app
	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/bsa_examples/linux
	@$(CP) -r $(BLUETOOTH_3RD_DIR)/embedded/bsa_examples/linux/qt_app $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/bsa_examples/linux/qt_app

	@$(ECHO) Copy app_ble_test_v2
	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/bsa_examples/linux
	@$(CP) -r $(BLUETOOTH_3RD_DIR)/embedded/bsa_examples/linux/app_ble_test_v2 $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/bsa_examples/linux/app_ble_test_v2
	@$(ECHO) Copy app_manager_v2
	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/bsa_examples/linux
	@$(CP) -r $(BLUETOOTH_3RD_DIR)/embedded/bsa_examples/linux/app_manager_v2 $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/bsa_examples/linux/app_manager_v2
	@$(ECHO) Copy app_ble_v2
	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/bsa_examples/linux
	@$(CP) -r $(BLUETOOTH_3RD_DIR)/embedded/bsa_examples/linux/app_ble_v2 $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/bsa_examples/linux/app_ble_v2
	@$(ECHO) Copy app_ble_pm_v2
	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/bsa_examples/linux
	@$(CP) -r $(BLUETOOTH_3RD_DIR)/embedded/bsa_examples/linux/app_ble_pm_v2 $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/bsa_examples/linux/app_ble_pm_v2
	@$(ECHO) Copy app_ble_scan_v2
	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/bsa_examples/linux
	@$(CP) -r $(BLUETOOTH_3RD_DIR)/embedded/bsa_examples/linux/app_ble_scan_v2 $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/bsa_examples/linux/app_ble_scan_v2
	@$(ECHO) Copy OS drivers folders
	@if [ -d $(BLUETOOTH_3RD_DIR)/embedded/brcm/linux ]; then \
		echo "    .... Linux"; \
		$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/brcm/linux; \
		$(CP) -r $(BLUETOOTH_3RD_DIR)/embedded/brcm/linux/btusb $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/brcm/linux/btusb; \
		$(CP) -r $(BLUETOOTH_3RD_DIR)/embedded/brcm/linux/bthid $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/brcm/linux/bthid; \
		$(CP) -r $(BLUETOOTH_3RD_DIR)/embedded/brcm/linux/btdl $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/brcm/linux/btdl; \
	fi
	@if [ -d $(BLUETOOTH_3RD_DIR)/embedded/brcm/freebsd ]; then \
		echo "    .... FreeBSD"; \
		$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/brcm/freebsd; \
		$(CP) -r $(BLUETOOTH_3RD_DIR)/embedded/brcm/freebsd/sys $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/brcm/freebsd/sys; \
	fi

	@$(ECHO) Copy bsa_examples/server folder
	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)
	@$(CP) -r $(BLUETOOTH_3RD_DIR)/embedded/$(customer)/$(project)/app_common $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/app_common
	@find $(BLUETOOTH_3RD_DIR)/embedded/$(customer)/$(project)/$(application) -name "bt-daemon-socket" | xargs rm -f
	@find $(BLUETOOTH_3RD_DIR)/embedded/$(customer)/$(project)/$(application) -name "bt-*-fifo" | xargs rm -f
	@$(CP) -r $(BLUETOOTH_3RD_DIR)/embedded/$(customer)/$(project)/$(application) $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/server

	@find $(DELIVERY_CURRENT_FOLDER) -type p | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.xml" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "app_hh_voice_*.wav" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.log" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "ble_local_keys" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "hogp_devices" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.bin" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.o" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.d" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.keep" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name ".copyarea.db" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*~" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.log" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.merged" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.0" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.1" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.2" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.contrib" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "core" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name ".*.cmd" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.ko" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.mod.c" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name ".tmp_versions" | xargs rm -f -r
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "Module.markers" | xargs rm -f -r
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "Module.symvers" | xargs rm -f -r
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "modules.order" | xargs rm -f -r
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "callgrind.*" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/$(application)/build/test_files -type f -size +1024k | xargs rm -f
	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/release_notes
	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/release_notes/$(customer)
	@mv $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/$(application)/build/Release_$(application).txt $(DELIVERY_CURRENT_FOLDER)/release_notes/$(customer)
	@$(ECHO) Delivery done.

.PHONY: zipdelivery
zipdelivery:
	@$(ECHO) Creating delivery tgz file: $(DELIVERY_CURRENT_FOLDER:$(DELIVERIES_MAIN_FOLDER)/%=%).tgz
	@cd $(DELIVERIES_MAIN_FOLDER); \
	tar zcf $(DELIVERY_CURRENT_FOLDER:$(DELIVERIES_MAIN_FOLDER)/%=%).tgz $(DELIVERY_CURRENT_FOLDER:$(DELIVERIES_MAIN_FOLDER)/%=%)

.PHONY: builddelivery
builddelivery: cleandelivery
	@$(ECHO) Compiling the delivery for verification
	$(MAKE) -C $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/server/build -f Makefile.all all

.PHONY: cleandelivery
cleandelivery:
	@$(ECHO) Removing Delivery objects
	$(MAKE) -C $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/server/build -f Makefile.all clean

.PHONY: removedeliveryfolder
removedeliveryfolder:
	@$(ECHO) Removing Delivery folder $(DELIVERY_CURRENT_FOLDER)
	@$(RM) -rf $(DELIVERY_CURRENT_FOLDER)

.PHONY: removedeliveryfile
removedeliveryfile:
	@$(ECHO) Removing Delivery file $(DELIVERY_CURRENT_FOLDER).tgz
	@$(RM) $(DELIVERY_CURRENT_FOLDER).tgz

# customer CPU and Local-CPU server must be compiled before creating Binary delivery
.PHONY: binarydelivery
binarydelivery: $(BLUETOOTH_APPLICATION_BUILD_DIR)/$(CPU)/bsa_server $(BLUETOOTH_APPLICATION_BUILD_DIR)/$(BUILD_CPU)/bsa_server
	@$(ECHO) Creating Binary Delivery in $(DELIVERY_CURRENT_FOLDER)
	@$(MKDIR) -p $(DELIVERIES_MAIN_FOLDER)
	@$(RM)   -rf $(DELIVERY_CURRENT_FOLDER)
	@$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)

	@$(ECHO) Copy OS drivers folders
	@if [ -d $(BLUETOOTH_3RD_DIR)/embedded/brcm/linux ]; then \
		echo "    .... Linux"; \
		$(MKDIR) -p $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/brcm/linux; \
		$(CP) -r --remove-destination $(BLUETOOTH_3RD_DIR)/embedded/brcm/linux/btusb $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/brcm/linux/btusb; \
		$(CP) -r --remove-destination $(BLUETOOTH_3RD_DIR)/embedded/brcm/linux/bthid $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/brcm/linux/bthid; \
	fi

	@$(ECHO) Copy $(customer)/server folder
	@$(CP) -r --remove-destination $(BLUETOOTH_3RD_DIR)/embedded/$(customer)/$(project)/server $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/server

	@$(ECHO) Replace server/build/Makefile.all by Replace server/build/MakefileBinary.all
	@$(RM) -rf $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/$(application)/build/Makefile.all
	@$(MV) $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/$(application)/build/MakefileBinary.all $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/$(application)/build/Makefile.all

	@$(ECHO) Copy $(customer)/app_common folder
	@$(CP) -r --remove-destination $(BLUETOOTH_3RD_DIR)/embedded/$(customer)/$(project)/app_common $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/app_common

	@find $(DELIVERY_CURRENT_FOLDER) -name "bt-daemon-socket" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "bt-*-fifo" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "*.xml" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "*.bin" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "*.o" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "*.d" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "*.keep" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name ".copyarea.db" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -name "*~" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.log" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.merged" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.0" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.1" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.2" | xargs rm -f
	@find $(DELIVERY_CURRENT_FOLDER) -type f -name "*.contrib" | xargs rm -f
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
		echo "    Binary delivery from source delivery"; \
		$(CP) $(GLOBALPATH)/release_notes/$(customer)/Release_$(application).txt $(DELIVERY_CURRENT_FOLDER)/release_notes/$(customer); \
	elif [ -f $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/$(application)/build/Release_$(application).txt ]; then \
		echo "    Binary delivery from vobs"; \
		$(MV) $(DELIVERY_CURRENT_FOLDER)/3rdparty/embedded/$(customer)/$(project)/$(application)/build/Release_$(application).txt $(DELIVERY_CURRENT_FOLDER)/release_notes/$(customer); \
	else \
		echo "    Nothing to do for release notes"; \
	fi
	@$(ECHO) Binary Delivery done.

.PHONY: renamebinarydelivery
renamebinarydelivery:
	@$(ECHO) Renaming binary delivery : $(DELIVERY_CURRENT_FOLDER) to $(BINARY_DELIVERY_CURRENT_FOLDER)
	@$(RM) -rf $(BINARY_DELIVERY_CURRENT_FOLDER)
	@$(MV) $(DELIVERY_CURRENT_FOLDER) $(BINARY_DELIVERY_CURRENT_FOLDER)

.PHONY: zipbinarydelivery
zipbinarydelivery:
	@$(ECHO) Creating binary delivery tgz file: $(BINARY_DELIVERY_CURRENT_FOLDER:$(DELIVERIES_MAIN_FOLDER)/%=%).tgz
	@cd $(DELIVERIES_MAIN_FOLDER); \
	tar zcf $(BINARY_DELIVERY_CURRENT_FOLDER:$(DELIVERIES_MAIN_FOLDER)/%=%).tgz $(BINARY_DELIVERY_CURRENT_FOLDER:$(DELIVERIES_MAIN_FOLDER)/%=%)

.PHONY: removebinarydeliveryfolder
removebinarydeliveryfolder:
	@$(ECHO) Removing Binary Delivery folder $(BINARY_DELIVERY_CURRENT_FOLDER)
	@$(RM) -rf $(BINARY_DELIVERY_CURRENT_FOLDER)


