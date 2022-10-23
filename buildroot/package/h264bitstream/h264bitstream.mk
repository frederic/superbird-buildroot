################################################################################
#
# H264BITSTREAM
#
################################################################################


#MYPKG_VERSION = some_commit_id_or_tag_or_branch_name
#MYPKG_SITE = git://thegitrepository
#MYPKG_SITE_METHOD = git

H264BITSTREAM_VERSION = 34f3c58afa3c47b6cf0a49308a68cbf89c5e0bff
H264BITSTREAM_SITE = git://github.com/aizvorski/h264bitstream
H264BITSTREAM_METHOD = git

H264BITSTREAM_INSTALL_STAGING = YES
# No configure provided, so we need to autoreconf.
H264BITSTREAM_AUTORECONF = YES
H264BITSTREAM_LICENSE = LGPL-2.1+
H264BITSTREAM_LICENSE_FILES = LICENSE

ifeq ($(BR2_PACKAGE_LIBICONV),y)
H264BITSTREAM_DEPENDENCIES += libiconv
H264BITSTREAM_CONF_ENV += LIBS="-liconv"
endif

$(eval $(autotools-package))
