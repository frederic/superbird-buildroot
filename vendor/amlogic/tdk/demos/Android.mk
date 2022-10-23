ifneq ($(filter userdebug eng,$(TARGET_BUILD_VARIANT)),)
include $(call all-subdir-makefiles)
#include $(call all-named-subdir-makefiles, hello_world)
endif
