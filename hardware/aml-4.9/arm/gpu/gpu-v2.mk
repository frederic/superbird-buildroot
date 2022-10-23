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

MESON_GPU_DIR?=hardware/arm/gpu
GPU_MODS_OUT?=system/lib
KERNEL_ARCH ?= arm
GPU_DRV_VERSION?=r6p1

$(PRODUCT_OUT)/obj/lib_vendor/mali.ko: $(PRODUCT_OUT)/$(GPU_MODS_OUT)/$(GPU_ARCH).ko
	-cp  $(PRODUCT_OUT)/$(GPU_MODS_OUT)/mali.ko $(PRODUCT_OUT)/obj/lib_vendor/mali.ko
	echo "$(GPU_ARCH).ko build finished"

#TODO rm shell cmd
# utgard-modules $(MESON_GPU_DIR) $(GPU_DRV_VERSION) $(KERNEL_ARCH)
define utgard-modules
	rm $(PRODUCT_OUT)/obj/mali -rf
	mkdir -p $(PRODUCT_OUT)/obj/mali
	cp $(2)/*  $(PRODUCT_OUT)/obj/mali -airf
	cp $(MESON_GPU_DIR)/utgard/platform  $(PRODUCT_OUT)/obj/mali/ -airf
	@echo "make mali module KERNEL_ARCH is $(3)"
	@echo "make mali module MALI_OUT is $(PRODUCT_OUT)/obj/mali $(MALI_OUT)"
	@echo "make mali module MAKE is $(MAKE)"
	@echo "GPU_DRV_VERSION is $(1)"
	PATH=$$(cd ./$(TARGET_HOST_TOOL_PATH); pwd):$$PATH \
	$(MAKE) -C $(shell pwd)/$(PRODUCT_OUT)/obj/KERNEL_OBJ M=$(shell pwd)/$(PRODUCT_OUT)/obj/mali  \
	ARCH=$(3) CROSS_COMPILE=$(PREFIX_CROSS_COMPILE) CONFIG_MALI400=m  CONFIG_MALI450=m    \
	EXTRA_CFLAGS="-DCONFIG_MALI400=m -DCONFIG_MALI450=m" \
	EXTRA_LDFLAGS+="--strip-debug" \
	CONFIG_AM_VDEC_H264_4K2K=y

	@echo "GPU_MODS_OUT is $(GPU_MODS_OUT)"
	mkdir -p $(PRODUCT_OUT)/$(GPU_MODS_OUT)
	cp  $(PRODUCT_OUT)/obj/mali/mali.ko $(PRODUCT_OUT)/$(GPU_MODS_OUT)/mali.ko
endef

#$(call midgard-modules,$(MESON_GPU_DIR),$(MESON_GPU_DIR)/midgard/$(GPU_DRV_VERSION),$(KERNEL_ARCH))
define midgard-modules
	rm $(PRODUCT_OUT)/obj/t83x -rf
	mkdir -p $(PRODUCT_OUT)/obj/t83x
	cp $(2)/* $(PRODUCT_OUT)/obj/t83x -airf
	@echo "make mali module KERNEL_ARCH is $(KERNEL_ARCH) current dir is $(shell pwd)"
	@echo "MALI is $(2), MALI_OUT is $(PRODUCT_OUT)/obj/t83x "
	PATH=$$(cd ./$(TARGET_HOST_TOOL_PATH); pwd):$$PATH \
	$(MAKE) -C $(shell pwd)/$(PRODUCT_OUT)/obj/KERNEL_OBJ M=$(shell pwd)/$(PRODUCT_OUT)/obj/t83x/kernel/drivers/gpu/arm/midgard \
	ARCH=$(3) CROSS_COMPILE=$(PREFIX_CROSS_COMPILE) \
	EXTRA_CFLAGS="-DCONFIG_MALI_PLATFORM_DEVICETREE -DCONFIG_MALI_MIDGARD_DVFS -DCONFIG_MALI_BACKEND=gpu" \
	EXTRA_LDFLAGS+="--strip-debug" \
	CONFIG_MALI_MIDGARD=m CONFIG_MALI_PLATFORM_DEVICETREE=y CONFIG_MALI_MIDGARD_DVFS=y CONFIG_MALI_BACKEND=gpu

	mkdir -p $(PRODUCT_OUT)/$(GPU_MODS_OUT)
	@echo "GPU_MODS_OUT is $(GPU_MODS_OUT)"
	cp  $(PRODUCT_OUT)/obj/t83x/kernel/drivers/gpu/arm/midgard/mali_kbase.ko $(PRODUCT_OUT)/$(GPU_MODS_OUT)/mali.ko
	@echo "make mali module finished current dir is $(shell pwd)"
endef

#$(call midgard-modules,$(MESON_GPU_DIR),$(MESON_GPU_DIR)/midgard/$(GPU_DRV_VERSION),$(KERNEL_ARCH))
define bifrost-modules
	rm $(PRODUCT_OUT)/obj/bifrost -rf
	mkdir -p $(PRODUCT_OUT)/obj/bifrost
	cp $(2)/* $(PRODUCT_OUT)/obj/bifrost -airf
	@echo "make mali module KERNEL_ARCH is $(KERNEL_ARCH) current dir is $(shell pwd)"
	@echo "MALI is $(2), MALI_OUT is $(PRODUCT_OUT)/obj/bifrost "
	PATH=$$(cd ./$(TARGET_HOST_TOOL_PATH); pwd):$$PATH \
	$(MAKE) -C $(shell pwd)/$(PRODUCT_OUT)/obj/KERNEL_OBJ M=$(shell pwd)/$(PRODUCT_OUT)/obj/bifrost/kernel/drivers/gpu/arm/midgard \
	ARCH=$(3) CROSS_COMPILE=$(PREFIX_CROSS_COMPILE) \
	EXTRA_CFLAGS="-DCONFIG_MALI_PLATFORM_DEVICETREE -DCONFIG_MALI_MIDGARD_DVFS -DCONFIG_MALI_BACKEND=gpu " \
	EXTRA_CFLAGS+="-I$(shell pwd)/$(PRODUCT_OUT)/obj/bifrost/kernel/include " \
	EXTRA_CFLAGS+="-Wno-error=larger-than=16384 " \
	EXTRA_LDFLAGS+="--strip-debug" \
	CONFIG_MALI_MIDGARD=m CONFIG_MALI_PLATFORM_DEVICETREE=y CONFIG_MALI_MIDGARD_DVFS=y CONFIG_MALI_BACKEND=gpu

	mkdir -p $(PRODUCT_OUT)/$(GPU_MODS_OUT)
	@echo "GPU_MODS_OUT is $(GPU_MODS_OUT)"
	cp  $(PRODUCT_OUT)/obj/bifrost/kernel/drivers/gpu/arm/midgard/mali_kbase.ko $(PRODUCT_OUT)/$(GPU_MODS_OUT)/mali.ko
	@echo "make mali module finished current dir is $(shell pwd)"
endef

# start from Q, only support build the module in 'out' directory
# modify to fit this requirement
$(PRODUCT_OUT)/$(GPU_MODS_OUT)/bifrost.ko: $(INSTALLED_KERNEL_TARGET)
	$(call bifrost-modules,$(MESON_GPU_DIR),$(MESON_GPU_DIR)/bifrost/$(GPU_DRV_VERSION),$(KERNEL_ARCH))

$(PRODUCT_OUT)/$(GPU_MODS_OUT)/midgard.ko: $(INSTALLED_KERNEL_TARGET)
	$(call midgard-modules,$(MESON_GPU_DIR),$(MESON_GPU_DIR)/midgard/$(GPU_DRV_VERSION),$(KERNEL_ARCH))

$(PRODUCT_OUT)/$(GPU_MODS_OUT)/utgard.ko: $(INSTALLED_KERNEL_TARGET)
	$(call utgard-modules,$(MESON_GPU_DIR),$(MESON_GPU_DIR)/utgard/$(GPU_DRV_VERSION),$(KERNEL_ARCH))
