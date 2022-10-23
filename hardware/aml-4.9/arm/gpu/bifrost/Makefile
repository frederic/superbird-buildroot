#
# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


GPU_TYPE:=dvalin
GPU_ARCH:=bifrost
GPU_DRV_VERSION?=r21p0

EXTRA_INCLUDE := -I$(KERNEL_SRC)/$(M)/../bifrost/$(GPU_DRV_VERSION)/kernel/drivers/gpu/arm/midgard/backend/gpu \
                 -I$(KERNEL_SRC)/$(M)/../bifrost/$(GPU_DRV_VERSION)/kernel/drivers/gpu/arm/midgard/platform/devicetree \
                 -I$(KERNEL_SRC)/$(M)/../bifrost/$(GPU_DRV_VERSION)/kernel/drivers/gpu/arm/midgard/ipa \
                 -I$(KERNEL_SRC)/$(M)/../bifrost/$(GPU_DRV_VERSION)/kernel/drivers/gpu/arm/midgard \
                 -I$(KERNEL_SRC)/$(M)/../dvalin/kernel/include
modules:
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/$(GPU_DRV_VERSION)/kernel/drivers/gpu/arm/midgard  \
	EXTRA_CFLAGS="-DCONFIG_MALI_PLATFORM_DEVICETREE -DCONFIG_MALI_MIDGARD_DVFS -DCONFIG_MALI_BACKEND=gpu " \
	EXTRA_CFLAGS+="-Wno-pointer-sign -Wno-error=larger-than=16384 -DCONFIG_MALI_DMA_BUF_MAP_ON_DEMAND=1 -DCONFIG_MALI_DMA_BUF_LEGACY_COMPAT=0 " \
	EXTRA_CFLAGS+="-Wno-error=unused-but-set-variable $(EXTRA_INCLUDE)" \
	EXTRA_LDFLAGS+="--strip-debug" \
	CONFIG_MALI_MIDGARD=m CONFIG_MALI_PLATFORM_DEVICETREE=y CONFIG_MALI_MIDGARD_DVFS=y CONFIG_MALI_BACKEND=gpu

modules_install:
	@$(MAKE) INSTALL_MOD_STRIP=1 M=$(M)/$(GPU_DRV_VERSION)/kernel/drivers/gpu/arm/midgard -C $(KERNEL_SRC) modules_install
	mkdir -p ${OUT_DIR}/../vendor_lib/modules
	cd ${OUT_DIR}/$(M)/; find -name "*.ko" -exec cp {} ${OUT_DIR}/../vendor_lib/modules/mali.ko \;

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/$(GPU_DRV_VERSION)/kernel/drivers/gpu/arm/midgard clean
