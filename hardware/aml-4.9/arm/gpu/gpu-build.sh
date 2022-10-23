#!/bin/sh

PRODUCT_OUT=${OUT}
SOURCE_CODE=utgard/r10p0
MESON_GPU_DIR=./
PREFIX_CROSS_COMPILE=/opt/gcc-linaro-6.3.1-2017.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
KERNEL_ARCH=arm64
GPU_MODS_OUT=obj/lib_vendor/

	rm ${PRODUCT_OUT}/obj/mali -rf
	mkdir -p ${PRODUCT_OUT}/obj/mali
	cp ${SOURCE_CODE}/*  ${PRODUCT_OUT}/obj/mali -airf
	cp ${MESON_GPU_DIR}/utgard/platform  ${PRODUCT_OUT}/obj/mali/ -airf
	echo "make mali module KERNEL_ARCH is ${KERNEL_ARCH}"
	echo "make mali module MALI_OUT is ${PRODUCT_OUT}/obj/mali ${MALI_OUT}"
	PATH=${TARGET_HOST_TOOL_PATH}:$PATH
	make -C ${PRODUCT_OUT}/obj/KERNEL_OBJ M=${PRODUCT_OUT}/obj/mali  \
	ARCH=${KERNEL_ARCH} CROSS_COMPILE=${PREFIX_CROSS_COMPILE} CONFIG_MALI400=m  CONFIG_MALI450=m    \
	CONFIG_MALI_DMA_BUF_LAZY_MAP=y \
	EXTRA_CFLAGS="-DCONFIG_MALI400=m -DCONFIG_MALI450=m -DCONFIG_MALI_DMA_BUF_LAZY_MAP=y" \
	EXTRA_LDFLAGS+="--strip-debug" \
	CONFIG_AM_VDEC_H264_4K2K=y 2>&1 | tee mali.txt

	echo "GPU_MODS_OUT is ${GPU_MODS_OUT}"
	mkdir -p ${PRODUCT_OUT}/${GPU_MODS_OUT}
	cp  ${PRODUCT_OUT}/obj/mali/mali.ko ${PRODUCT_OUT}/${GPU_MODS_OUT}/mali.ko

	cp  ${PRODUCT_OUT}/${GPU_MODS_OUT}/mali.ko ${PRODUCT_OUT}/obj/lib_vendor/mali.ko
	echo "${GPU_ARCH}.ko build finished"

exit
