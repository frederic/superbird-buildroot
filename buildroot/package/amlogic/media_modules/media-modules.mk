################################################################################
#
# amlogic media drivers
#
################################################################################

MEDIA_MODULES_VERSION = $(call qstrip,$(BR2_PACKAGE_MEDIA_MODLUES_VERSION))
MEDIA_MODULES_SITE = $(call qstrip,$(BR2_PACKAGE_MEDIA_MODULES_GIT_REPO_URL))
CUR_PATH := $(shell cd "$(dirname $$0)";pwd)/$(dir $(lastword $(MAKEFILE_LIST)))

# modules
MEDIA_MODULES_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/kernel/media
MEDIA_MODULES_DEP = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
MEDIA_MODULES_MODULE_DIR := kernel/media

# firmware
MEDIA_MODULES_UCODE_BIN := firmware/video_ucode.bin
MEDIA_MODULES_H264_ENC_BIN := firmware/h264_enc.bin
MEDIA_MODULES_FIRMWARE_INSTALL_DIR := $(TARGET_DIR)/lib/firmware/video
MEDIA_MODULES_FIRMWARE = $(@D)/$(MEDIA_MODULES_UCODE_BIN) \
						 $(@D)/$(MEDIA_MODULES_H264_ENC_BIN)

CONFIGS := CONFIG_AMLOGIC_MEDIA_VDEC_MPEG12=m \
    CONFIG_AMLOGIC_MEDIA_VDEC_MPEG2_MULTI=m \
    CONFIG_AMLOGIC_MEDIA_VDEC_MPEG4=m \
    CONFIG_AMLOGIC_MEDIA_VDEC_MPEG4_MULTI=m \
    CONFIG_AMLOGIC_MEDIA_VDEC_VC1=m \
    CONFIG_AMLOGIC_MEDIA_VDEC_H264=m \
    CONFIG_AMLOGIC_MEDIA_VDEC_H264_MULTI=m \
    CONFIG_AMLOGIC_MEDIA_VDEC_H264_MVC=m \
    CONFIG_AMLOGIC_MEDIA_VDEC_H264_4K2K=m \
    CONFIG_AMLOGIC_MEDIA_VDEC_H264_MVC=m \
    CONFIG_AMLOGIC_MEDIA_VDEC_H265=m \
    CONFIG_AMLOGIC_MEDIA_VDEC_VP9=m \
    CONFIG_AMLOGIC_MEDIA_VDEC_MJPEG=m \
    CONFIG_AMLOGIC_MEDIA_VDEC_MJPEG_MULTI=m \
    CONFIG_AMLOGIC_MEDIA_VDEC_REAL=m \
    CONFIG_AMLOGIC_MEDIA_VDEC_AVS=m \
    CONFIG_AMLOGIC_MEDIA_VENC_H264=m \
    CONFIG_AMLOGIC_MEDIA_VENC_H265=m \
    CONFIG_AMLOGIC_MEDIA_VENC_MULTI=m \
    CONFIG_AMLOGIC_MEDIA_VENC_JPEG=m

define copy-media-modules
	$(foreach m, $(shell find $(strip $(1)) -name "*.ko"),\
		$(shell [ ! -e $(2) ] && mkdir $(2) -p;\
		cp $(m) $(strip $(2))/ -rfa;\
		echo $(4)/$(notdir $(m)): >> $(3)))
endef

define copy-firmware
	@[ ! -e $(2) ] && mkdir $(2) -p;\
	$(INSTALL) -D -m 0644 $(1) $(2)
endef

ifeq ($(BR2_PACKAGE_MEDIA_MODULES_LOCAL),y)
MEDIA_MODULES_SITE = $(call qstrip,$(BR2_PACKAGE_MEDIA_MODULES_LOCAL_PATH))
MEDIA_MODULES_SITE_METHOD = local
MEDIA_MODULES_DEPENDENCIES = linux

ifeq (,$(wildcard $(MEDIA_MODULES_FIRMWARE_INSTALL_DIR)))
$(shell mkdir $(MEDIA_MODULES_FIRMWARE_INSTALL_DIR) -p)
endif

define MEDIA_MODULES_BUILD_CMDS
	@$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) \
		M=$(@D)/drivers ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_KERNEL_CROSS) $(CONFIGS) modules
endef

define MEDIA_MODULES_CLEAN_CMDS
	@$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) clean
endef

define MEDIA_MODULES_INSTALL_TARGET_CMDS
	$(call copy-media-modules,$(@D),\
		$(shell echo $(MEDIA_MODULES_INSTALL_DIR)),\
		$(shell echo $(MEDIA_MODULES_DEP)),\
		$(MEDIA_MODULES_MODULE_DIR))
	$(call copy-firmware,$(MEDIA_MODULES_FIRMWARE),\
		$(MEDIA_MODULES_FIRMWARE_INSTALL_DIR))
endef
endif

$(eval $(generic-package))

