#
# SPDX-License-Identifier: GPL-2.0
#
# Copyright (C) 2011-2018 ARM or its affiliates
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2.
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# for more details.
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

CONFIG_MODULE_SIG=n


#redefine ARCH
ifeq ($(ARCH),)    
    _ARCH=arm64
    $(info "set default ARCH=${_ARCH}")
else
    _ARCH=${ARCH}
endif


#redefine CROSS_COMPILE
ifeq ($(CROSS_COMPILE),)    
    $(error "Please export CROSS_COMPILE variable before build")
else
    _CROSS_COMPILE=${CROSS_COMPILE}
endif

#redefine KDIR
ifeq ($(KDIR),)    
    $(error "Please export KDIR variable before build")
else
    _KDIR=${KDIR}
endif



obj-m += iv009_isp_iq.o

ifeq ($(FW_SRC_OBJ),)
	FW_SRC := $(wildcard src/*.c src/*/*.c src/*/*/*.c app/*.c app/*/*.c)
	export FW_SRC_OBJ := $(FW_SRC:.c=.o)

	ifneq ($(wildcard src/fw_lib/libacamera_isp.a),) 
		export FW_SRC_OBJ := $(FW_SRC_OBJ) src/fw_lib/libacamera_isp.a
	endif
##$(info ### FW_SRC_OBJ is: $(FW_SRC_OBJ))	
endif


iv009_isp_iq-objs += $(FW_SRC_OBJ) 

INCLUDE_DIRS := $(addprefix -I,$(shell find ../ -type d ))

ccflags-y:=-I$(PWD)/app -I$(PWD)/inc -I$(PWD)/app/control -I$(PWD)/inc/api -I$(PWD)/inc/isp -I$(PWD)/inc/sys -I$(PWD)/src/platform -I$(PWD)/src/fw  -I$(PWD)/src/fw_lib -I$(PWD)/src/calibration -I$(PWD)/src/driver/sensor -I$(PWD)/src/driver/lens -I$(_KDIR)/include/linux/

ccflags-y += -Wno-declaration-after-statement -Wno-error

all:
		CROSS_COMPILE=${_CROSS_COMPILE} make ARCH=${_ARCH} -C $(_KDIR) M=$(PWD) modules

clean:
		CROSS_COMPILE=${_CROSS_COMPILE} make ARCH=${_ARCH} -C $(_KDIR) M=$(PWD) clean

