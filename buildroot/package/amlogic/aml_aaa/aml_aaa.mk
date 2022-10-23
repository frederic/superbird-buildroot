################################################################################
#
# amlogic zzz fake driver
#
################################################################################
AML_AAA_SITE = $(AML_AAA_PKGDIR).
AML_AAA_SITE_METHOD = local
AML_AAA_VERSION = 1.0
AML_AAA_DEPENDENCIES = linux

define AML_AAA_BUILD_CMDS
	echo "Fake AAA kernel driver"
endef

$(eval $(generic-package))
