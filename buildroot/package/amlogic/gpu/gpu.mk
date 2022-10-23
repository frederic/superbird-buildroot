##############################################################################
#
# amlogic gpu driver
#
##############################################################################
ifneq ($(BR2_PACKAGE_MESON_MALI_VERSION),"")
GPU_VERSION = $(call qstrip,$(BR2_PACKAGE_MESON_MALI_VERSION))
else
GPU_VERSION = $(call qstrip,$(BR2_PACKAGE_GPU_VERSION))
endif
ifneq ($(BR2_PACKAGE_GPU_CUSTOM_TARBALL_LOCATION),"")
GPU_TARBALL = $(call qstrip,$(BR2_PACKAGE_GPU_CUSTOM_TARBALL_LOCATION))
GPU_SITE    = $(patsubst %/,%,$(dir $(GPU_TARBALL)))
GPU_SOURCE  = $(notdir $(GPU_TARBALL))
else ifeq ($(BR2_PACKAGE_GPU_LOCAL),y)
GPU_SITE_METHOD = local
GPU_SITE    = $(call qstrip,$(BR2_PACKAGE_GPU_LOCAL_PATH))
else
GPU_SITE = $(call qstrip,$(BR2_PACKAGE_GPU_GIT_URL))
GPU_SITE_METHOD = git
endif
GPU_MODEL=$(call qstrip,$(BR2_PACKAGE_GPU_MODEL))

#GPU_MODULE_DIR = kernel/amlogic/gpu
GPU_MODULE_DIR = hardware/aml-4.9/arm/gpu/

GPU_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(GPU_MODULE_DIR)
GPU_DEPENDENCIES = linux

UTGARD_BUILD_CMD = \
	if [ -e $(@D)/utgard/Makefile ]; then \
		cd $(@D)/utgard ;\
		$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/utgard KDIR=$(LINUX_DIR) \
		ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(TARGET_KERNEL_CROSS) \
		GPU_DRV_VERSION=$(GPU_VERSION); \
		cp $(@D)/utgard/$(GPU_VERSION)/mali.ko $(@D)/mali.ko; \
	else \
		$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/utgard/$(GPU_VERSION) ARCH=$(KERNEL_ARCH) \
               CROSS_COMPILE=$(TARGET_KERNEL_CROSS) CONFIG_MALI400=m CONFIG_MALI450=m \
                EXTRA_CFLAGS="-DCONFIG_MALI400=m -DCONFIG_MALI450=m" modules; \
		cp $(@D)/utgard/$(GPU_VERSION)/mali.ko $(@D)/mali.ko; \
	fi;

MIDGARD_BUILD_CMD = \
	if [ -e $(@D)/midgard/$(GPU_VERSION) ]; then \
		cd $(@D)/midgard/$(GPU_VERSION)/kernel/drivers/gpu/arm/midgard ; \
		$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=`pwd` \
		ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(TARGET_KERNEL_CROSS) \
		EXTRA_CFLAGS="-DCONFIG_MALI_PLATFORM_DEVICETREE -DCONFIG_MALI_MIDGARD_DVFS -DCONFIG_MALI_BACKEND=gpu" \
		CONFIG_MALI_MIDGARD=m CONFIG_MALI_PLATFORM_DEVICETREE=y CONFIG_MALI_MIDGARD_DVFS=y CONFIG_MALI_BACKEND=gpu modules; \
		cp $(@D)/midgard/$(GPU_VERSION)/kernel/drivers/gpu/arm/midgard/mali_kbase.ko $(@D)/mali.ko; \
	fi;

BIFROST_BUILD_CMD = \
	if [ -e $(@D)/bifrost/$(GPU_VERSION) ]; then \
		cd $(@D)/bifrost/$(GPU_VERSION)/kernel/drivers/gpu/arm/midgard ; \
		echo "dvalin build" ; \
		$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=`pwd` \
		ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(TARGET_KERNEL_CROSS) \
		EXTRA_CFLAGS="-DCONFIG_MALI_PLATFORM_DEVICETREE -DCONFIG_MALI_MIDGARD_DVFS -DCONFIG_MALI_BACKEND=gpu" \
		CONFIG_MALI_MIDGARD=m CONFIG_MALI_PLATFORM_DEVICETREE=y CONFIG_MALI_MIDGARD_DVFS=y modules; \
		cp $(@D)/bifrost/$(GPU_VERSION)/kernel/drivers/gpu/arm/midgard/mali_kbase.ko $(@D)/mali.ko; \
	fi;

MALI_INSTALL_TARGETS_CMDS = \
	$(INSTALL) -m 0666 $(@D)/mali.ko $(GPU_INSTALL_DIR); \
	echo $(GPU_MODULE_DIR)/mali.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep;

define GPU_BUILD_CMDS
$(Q) echo "GPU_MODEL is $(GPU_MODEL)"; \
	case "$(GPU_MODEL)" in \
        mali450)	$(UTGARD_BUILD_CMD) ;; \
        t83x)		$(MIDGARD_BUILD_CMD) ;; \
        t82x)		$(MIDGARD_BUILD_CMD) ;;	\
        dvalin)		$(BIFROST_BUILD_CMD) ;; \
        gondul)		$(BIFROST_BUILD_CMD) ;; \
        bifrost)	$(BIFROST_BUILD_CMD) ;; \
	esac; \
	echo "case finished"
endef
define GPU_INSTALL_TARGET_CMDS
	mkdir -p $(GPU_INSTALL_DIR);
	$(MALI_INSTALL_TARGETS_CMDS)
endef
$(eval $(generic-package))
