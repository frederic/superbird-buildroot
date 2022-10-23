##########################################################
## Lines for building TAs automatically                 ##
## will only be included in Android.mk for TAs          ##
## local_module:
## local_module_ta_name:
##     need to be defined before include for this       ##
##########################################################

OPTEE_CROSS_COMPILE := arm-linux-gnueabihf-
CROSS_COMPILE_LINE := CROSS_COMPILE=$(OPTEE_CROSS_COMPILE)
OPTEE_TA_OUT_DIR ?= $(PRODUCT_OUT)/optee/ta

TOP_ROOT_ABS := $(realpath $(TOP))
TA_DEV_KIT_DIR := $(BOARD_AML_VENDOR_PATH)/tdk/ta_export

ifeq ($(TARGET_ENABLE_TA_SIGN), true)
TDK_PATH := $(TOP_ROOT_ABS)/$(BOARD_AML_VENDOR_PATH)/tdk
TA_SIGN_AUTO_TOOL := $(TDK_PATH)/ta_export/scripts/sign_ta_auto.py
endif

include $(CLEAR_VARS)
local_module_path ?= $(TARGET_OUT)/lib/optee_armtz
LOCAL_MODULE := $(local_module)
LOCAL_PREBUILT_MODULE_FILE := $(OPTEE_TA_OUT_DIR)/$(LOCAL_MODULE)
LOCAL_MODULE_PATH := $(local_module_path)
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional

TA_TMP_DIR := $(subst /,_,$(LOCAL_PATH))
TA_TMP_FILE := $(OPTEE_TA_OUT_DIR)/$(TA_TMP_DIR)/$(LOCAL_MODULE)

$(LOCAL_PREBUILT_MODULE_FILE): $(TA_TMP_FILE)
	@mkdir -p $(dir $@)
ifeq ($(TARGET_ENABLE_TA_SIGN), true)
ifeq ($(TARGET_ENABLE_TA_ENCRYPT), true)
	$(TA_SIGN_AUTO_TOOL) --in=$< --out=$@ --encrypt=1
else
	$(TA_SIGN_AUTO_TOOL) --in=$< --out=$@ --encrypt=0
endif
else #else ifeq ($(TARGET_ENABLE_TA_SIGN), true)
	cp -uvf $< $@
endif

$(TA_TMP_FILE): PRIVATE_TA_SRC_DIR := $(LOCAL_PATH)
$(TA_TMP_FILE): PRIVATE_TA_TMP_FILE := $(TA_TMP_FILE)
$(TA_TMP_FILE): PRIVATE_TA_TMP_DIR := $(TA_TMP_DIR)
$(TA_TMP_FILE): FORCE
	@echo "Start building TA for $(PRIVATE_TA_SRC_DIR) $(PRIVATE_TA_TMP_FILE)..."
	$(MAKE) -C $(TOP_ROOT_ABS)/$(PRIVATE_TA_SRC_DIR) O=$(TOP_ROOT_ABS)/$(OPTEE_TA_OUT_DIR)/$(PRIVATE_TA_TMP_DIR) \
		TA_DEV_KIT_DIR=$(TOP_ROOT_ABS)/$(TA_DEV_KIT_DIR) \
		$(CROSS_COMPILE_LINE)
	@echo "Finished building TA for $(PRIVATE_TA_SRC_DIR) $(PRIVATE_TA_TMP_FILE)..."

FORCE:

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := $(local_module_ta_name)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

LOCAL_REQUIRED_MODULES := $(local_module)

include $(BUILD_PHONY_PACKAGE)

local_module_ta_name :=

