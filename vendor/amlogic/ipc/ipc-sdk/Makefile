ifeq ($(LOCAL_BUILD),y)
TARGET_OUTPUT_DIR=/mnt/fileroot/daogao.xu/buildroot/output/mesonc1_ae400_a64_release
export CC=$(TARGET_OUTPUT_DIR)/host/bin/aarch64-linux-gnu-gcc
export PKG_CONFIG_PATH=$(TARGET_OUTPUT_DIR)/host/aarch64-linux-gnu/sysroot/usr/lib/pkgconfig
export PKG_CONFIG=$(TARGET_OUTPUT_DIR)/host/bin/pkg-config
IPC_SDK_BUILD_EXAMPLE=y
export CFLAGS += -I../inc
export LDFLAGS += -L../src
endif

# special CFLAGS:
#  -DIPC_SDK_VENC_LEGACY build with legacy venc library instead of multienc
#

SUB_DIRS := $(CURDIR)/src

ifeq ($(IPC_SDK_BUILD_EXAMPLE), y)
SUB_DIRS += $(CURDIR)/example
endif

ifeq ($(IPC_SDK_BUILD_TEST), y)
SUB_DIRS += $(CURDIR)/test
endif

all:
	for d in $(SUB_DIRS); do ($(MAKE) -C $$d ) || exit 1; done

clean:
	for d in $(SUB_DIRS); do ($(MAKE) $@ -C $$d ) || exit 1; done

example/%:
	$(MAKE) -C example $(@:example/%=%)
