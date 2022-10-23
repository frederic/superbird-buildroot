###############################################################################
# Firmware Download (BTDL) tool Makefile
# This makefile is written for a GNU make or equivalent.
# For example:
#   make clean
#   make
# Copyright (c) 2012, Broadcom Corp., All Rights Reserved.
# Broadcom Bluetooth Core. Proprietary and confidential.
###############################################################################


#
# Tools configuration
#
MKDIR      := mkdir
CP         := cp
RM         := rm
ECHO       := echo
SED        := sed
TOUCH      := touch
MV         := mv
CD         := cd
TAR        := tar

#
# Compiler configuration
#
OPTIM ?= TIME
DEBUG ?= TRUE

#
# CPU configuration (MIPS, ARM, X86, ...)
#
ifeq ($(CPU), mips)
  # Based on environment MIPSGCC and MIPSGCCLIBPATH variables
  ifdef MIPSGCC
    CC := $(MIPSGCC)
    LINK := $(MIPSGCC)
  else
    $(error MIPSGCC Not defined))
  endif
  ifndef MIPSGCCLIBPATH
    $(error MIPSGCCLIBPATH Not defined)
  endif

  CCCPUFLAGS := $(MIPSCCFLAGS)
  LINKCPUFLAGS := $(patsubst %,-L %,$(MIPSGCCLIBPATH)) $(MIPSLINKFLAG)

else
ifeq ($(CPU), arm)

 # Based on environment ARMGCC and ARMGCCLIBPATH variables
  ifdef ARMGCC
    CC = $(ARMGCC)
    LINK = $(ARMGCC)
  else
    $(error ARMGCC Not defined))
  endif
  ifndef ARMGCCLIBPATH
    $(error ARMGCCLIBPATH Not defined)
  endif

  CCCPUFLAGS := $(ARMCCFLAGS)
  LINKCPUFLAGS := $(patsubst %,-L %,$(ARMGCCLIBPATH)) $(ARMLINKFLAG)
  
else
  # Default is Intel
  # Check if the local machine is 32 or 64 bits
  LBIT := $(shell getconf LONG_BIT)
  ifeq ($(LBIT), 64)
    CPU := x86_64
    CCCPUFLAGS := $(X86_64CCFLAGS)
    LINKCPUFLAGS := $(patsubst %,-L %,$(X86_64GCCLIBPATH)) $(X86_64LINKFLAG)
  else
    #$(warning Defaulting CPU to x86)
    CPU := x86
    CCCPUFLAGS := $(X86CCFLAGS)
    LINKCPUFLAGS := $(patsubst %,-L %,$(X86GCCLIBPATH)) $(X86LINKFLAG)
  endif
  TARGET_DEF := 

  LINK := gcc
  CC := gcc

endif
endif


# CC inclusion flags
CCINC = $(patsubst %,-I%,$(patsubst /,/,$(incdir)))

# check if the debug is enabled
ifeq ($(DEBUG),TRUE)
  CCDBGFLAGS := -g
endif

# check if the compilation optimization is enabled
ifeq ($(OPTIM),TIME)
  CCOPTIMFLAGS := -O3
else
ifeq ($(OPTIM),SIZE)
  CCOPTIMFLAGS := -Os
else
ifeq ($(OPTIM),NONE)
  CCOPTIMFLAGS := -O0
endif
endif
endif

# Compiler flags
CCFLAGS = $(CCDBGFLAGS) $(CCOPTIMFLAGS) $(CCCPUFLAGS) -Wall -Wno-unused-parameter -c

# Extract GCC Major version
GCC_VERSION_MAJOR := '$(shell $(CC) --version | grep ^gcc | sed 's/^.* //g' | cut -f1 -d.)'
# Extract GCC Minor version
GCC_VERSION_MINOR := '$(shell $(CC) --version | grep ^gcc | sed 's/^.* //g' | cut -f2 -d.)'

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
    CCFLAGS += -Wno-unused-but-set-variable
endif
else # GCC Major Version is Not Equal to 4
# If Major Version is Greater Than 4
ifeq "$(GCC_MAJOR_GT4)" "1"
    CCFLAGS += -Wno-unused-but-set-variable
endif
endif


# LINK flags (command line can specify EXTRALIBS=XXX to include specific libraries)
LINKFLAGS = $(LINKCPUFLAGS)
LINKLIBS = $(patsubst %,-l%,$(EXTRALIBS))

#######################################
#
# Application
#
#######################################

# Application related directories
appdir       := ..
builddir     := $(appdir)/build
exedir       := $(builddir)/$(CPU)
objdir       := $(exedir)/obj
srcdir       := $(appdir)/source
incdir       := $(appdir)/include

# name of the application
target       := $(exedir)/btdl

# When not specified, search the files in the source directories
vpath %.c $(srcdir)

#######################################
#
# Sources
#
#######################################
SRCS :=  btdl_main.c    \
         btdl_hex.c     \
         btdl_utils.c   \
         hci_cmd.c      \
         hci_drv.c

srclist := $(patsubst %.c,$(srcdir)/%.c,$(SRCS))

#######################################
#
# Sources
#
#######################################
inclist :=  $(incdir)/*.h

#######################################
#
# Objects
#
#######################################
objlist := $(patsubst %.c,$(objdir)/%.o,$(SRCS))


#######################################
#
# Targets
#
#######################################

.PHONY: all clean delivery

all: $(target)
	@$(ECHO) Done building $@

$(objdir):
	@$(ECHO) Creating object directory '$(objdir)'.
	-@$(MKDIR) -p $(objdir)

$(objdir)/%.o: %.c $(inclist)
	@$(ECHO) Compiling [$(CC)] $<
	@$(CC) $(CCFLAGS) $(CCINC) -o $@ $<
	
$(target): $(builddir) $(objdir) $(objlist)
	@$(ECHO) Linking application $@
	@$(LINK) $(LINKFLAGS) $(TARGET_LINK_FLAGS) -o $@ $(objlist) $(LINKLIBS)

clean:
	@$(ECHO) Erasing up : $(exedir)
	@$(RM) -rf $(exedir)

help:
	@$(ECHO) "Makefile supported targets"
	@$(ECHO) "  help               : this menu"
	@$(ECHO) "  all                : build the default btdl"
	@$(ECHO) "  clean              : clean the current btld target and objects"
	@$(ECHO) ""
	@$(ECHO) "Makefile compiler configuration"
	@$(ECHO) "  use CPU=<cpu> to select a particular CPU" 
	@$(ECHO) "  current CPU        : $(CPU)" 
	@$(ECHO) "  Possible cpu : mips, arm, x86 or x86_64"
	@$(ECHO) "  for mips make sure you define MIPSGCC, MIPSGCCLIBPATH, MIPSCCFLAGS, MIPSLINKFLAG"
	@$(ECHO) "  for arm make sure you define ARMGCC, ARMGCCLIBPATH, ARMCCFLAGS, ARMLINKFLAG"
	@$(ECHO) ""
	@$(ECHO) "Examples:"
	@$(ECHO) "  make all"
	@$(ECHO) "  make clean"
	@$(ECHO) "  make all CPU=mips"
	@$(ECHO) "  make CPU=mips MIPSGCC=~/stbgcc-4.5.3-2.4/bin/mipsel-linux-gcc MIPSGCCLIBPATH=~/stbgcc-4.5.3-2.4/lib all"

