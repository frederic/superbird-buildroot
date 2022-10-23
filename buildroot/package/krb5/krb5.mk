###########################################################
#
# krb5
#
###########################################################

KRB5_VERSION_MAJOR = 1.17
KRB5_VERSION = $(KRB5_VERSION_MAJOR)
KRB5_SITE = http://web.mit.edu/kerberos/dist/krb5/$(KRB5_VERSION_MAJOR)
KRB5_LICENSE = BSD-2c, others
KRB5_LICENSE_FILES = NOTICE
KRB5_SUBDIR = src
KRB5_INSTALL_STAGING = YES

KRB5_CONF_ENV = \
       krb5_cv_attr_constructor_destructor=yes,yes \
       ac_cv_func_regcomp=yes \
       krb5_cv_sys_rcdir=/tmp \
       ac_cv_printf_positional=yes \
       WARN_CFLAGS='-Wall'

KRB5_CONF_OPTS = \
       --without-tcl \
       --without-hesiod \
       --without-ldap \
       --without-libedit \
       --without-libreadline

# No buildroot packages exist for these so use the bundled ones.
KRB5_CONF_OPTS += \
       --without-system-libverto \
       --without-system-ss \
       --without-system-et

# Buildroot's berkeleydb does not provide API 1.85
KRB5_CONF_OPTS += --without-system-db

ifeq ($(BR2_TOOLCHAIN_HAS_THREADS),y)
KRB5_CONF_OPTS += --enable-thread-support
else
KRB5_CONF_OPTS += --disable-thread-support
endif

ifeq ($(BR2_PACKAGE_OPENSSL),y)
KRB5_CONF_OPTS += --with-crypto-impl=openssl
KRB5_DEPENDENCIES += openssl
else ifeq ($(BR2_PACKAGE_LIBNSS),y)
KRB5_CONF_OPTS += --with-crypto-impl=nss
KRB5_DEPENDENCIES += libnss
else
KRB5_CONF_OPTS += --with-crypto-impl=builtin
endif

$(eval $(autotools-package))
