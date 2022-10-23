#############################################################
#
# IPC Property JSON Header
#
#############################################################

IPC_PROPERTY_JSON_VERSION = v3.5.0
IPC_PROPERTY_JSON_SITE = https://github.com/nlohmann/json/releases/download/$(IPC_PROPERTY_JSON_VERSION)
IPC_PROPERTY_JSON_SOURCE = json.hpp

define IPC_PROPERTY_JSON_EXTRACT_CMDS
endef

$(eval $(generic-package))
