 #
 #  Copyright (C) 2014-2020 Amlogic, Inc. All rights reserved.
 #
 #  All information contained herein is Amlogic confidential.
 #
 #  This software is provided to you pursuant to Software License Agreement
 #  (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 #  only in accordance with the terms of this agreement.
 #
 #  Redistribution and use in source and binary forms, with or without
 #  modification is strictly prohibited without prior written permission from
 #  Amlogic.
 #
 #  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 #  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 #  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 #  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 #  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 #  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 #  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 #  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 #  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 #  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 #  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 #

TOP := $(shell pwd)/..

export ATH_TOPDIR=$(TOP)
export INSTALL_ROOT=$(TOP)/rootfs-x86-android.build

export IF_TYPE ?= SDIO

ifneq ($(KERNELPATH)/.config, $(wildcard $(KERNELPATH)/.config))
$(error Please set environment variable KERNELPATH, e.g export KERNELPATH="path/to/kernel/dir" or make KERNELPATH="path/to/kernel/dir")
endif

CROSS_COMPILE ?= $(subst -ld,-,$(shell cat $(KERNELPATH)/.vmlinux.cmd | awk '{print $$5}'))

ifneq ($(CROSS_COMPILE)gcc, $(wildcard $(CROSS_COMPILE)gcc))
$(error Please set environment variable CROSS_COMPILE, e.g export CROSS_COMPILE=$(CROSS_COMPILE) or make CROSS_COMPILE=$(CROSS_COMPILE))
endif

ARCH ?= $(shell ls $(KERNELPATH)/arch)

export KERNELPATH
export MAKEARCH=$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)

export CONFIG_LINUX_QCMBR=y
export CONFIG_NON_QC_PLATFORM=y
export CONFIG_WLAN_THERMAL_SHUTDOWN=0

ifeq ($(IF_TYPE),PCIE)
    export CONFIG_ROME_IF=pci
    export CONFIG_WLAN_FEATURE_11W=y
    export CONFIG_WLAN_FEATURE_FILS=y
    export CONFIG_LINUX_QCMBR=y
    export CONFIG_WLAN_WAPI_MODE_11AC_DISABLE=y
    export CONFIG_WLAN_WOW_PULSE=y
else ifeq ($(IF_TYPE),USB)
    export CONFIG_ROME_IF=usb
    export CONFIG_LINUX_QCMBR=y
    export CONFIG_WLAN_FEATURE_FILS=y
    export CFG80211_CONNECT_BSS=y
    export CONFIG_FEATURE_COEX_PTA_CONFIG_ENABLE=y
    export CONFIG_QOS_FWD_SUPPORT=y
    export CONFIG_QCA_SUPPORT_TXRX_DRIVER_TCP_DEL_ACK=y
    export CONFIG_WLAN_WAPI_MODE_11AC_DISABLE=y
    export CONFIG_WLAN_WOW_PULSE=y
else ifeq ($(IF_TYPE),SDIO)
    export CONFIG_CLD_HL_SDIO_CORE=y
    export CONFIG_LINUX_QCMBR=y
    export CONFIG_PER_VDEV_TX_DESC_POOL=1
    export SAP_AUTH_OFFLOAD=1
    export CONFIG_QCA_LL_TX_FLOW_CT=1
    export CONFIG_WLAN_FEATURE_FILS=y
    export CONFIG_FEATURE_COEX_PTA_CONFIG_ENABLE=y
    export CONFIG_QCA_SUPPORT_TXRX_DRIVER_TCP_DEL_ACK=y
    export CONFIG_WLAN_WAPI_MODE_11AC_DISABLE=y
    export CONFIG_WLAN_WOW_PULSE=y
    export CONFIG_TXRX_PERF=y
endif

all:
	@echo "##############################"
	@echo KERNELPATH=$(KERNELPATH)
	@echo KERNEL_ARCH=$(ARCH)
	@echo CROSS_COMPILE=$(CROSS_COMPILE)
	@echo "##############################"
	$(MAKE) -C $(TOP)/drivers -j
	mkdir -p $(INSTALL_ROOT)/lib/modules
	find $(TOP)/drivers -type f -name 'wlan.ko' -exec cp '{}' $(INSTALL_ROOT)/lib/modules/ ';'
	$(CROSS_COMPILE)strip --strip-unneeded $(INSTALL_ROOT)/lib/modules/*
	@echo "##############################"
	ls -lht $(INSTALL_ROOT)/lib/modules/
	@echo "##############################"

clean:
	$(MAKE) -C $(TOP)/drivers clean
	rm -rf $(INSTALL_ROOT)

