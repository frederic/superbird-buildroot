#
# (C) COPYRIGHT 2011, 2013 ARM Limited. All rights reserved.
#
# This program is free software and is provided to you under the terms of the
# GNU General Public License version 2 as published by the Free Software
# Foundation, and any use by you of this program is subject to the terms
# of such GNU licence.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you can access it online at
# http://www.gnu.org/licenses/gpl-2.0.html.
#
# SPDX-License-Identifier: GPL-2.0
#
#



# default to building for the host
ARCH ?= $(shell uname -m)

# linux build system integration

ifneq ($(KERNELRELEASE),)
# Inside the kernel build system

EXTRA_CFLAGS += -I$(KBUILD_EXTMOD) -I$(KBUILD_EXTMOD)/../../../../..
KBUILD_EXTRA_SYMBOLS += "$(KBUILD_EXTMOD)/../../Module.symvers"

SRC += ump_kernel_import_ion.c

MODULE:=ump_ion_import.ko

obj-m := $(MODULE:.ko=.o)
$(MODULE:.ko=-y) := $(SRC:.c=.o)
$(MODULE:.ko=-objs) := $(SRC:.c=.o)

else
# Outside the kernel build system
#
#

ifeq ($(KDIR),)
$(error Must specify KDIR to point to the kernel to target))
endif

all:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR)

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) clean

endif

