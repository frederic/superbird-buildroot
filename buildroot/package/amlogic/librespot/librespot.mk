####################################################################################
#
#librespot
#
####################################################################################

LIBRESPOT_VERSION = 0.2
LIBRESPOT_SOURCE = librespot.tar.gz
LIBRESPOT_LICENSE = MIT LICENSE
LIBRESPOT_LICENSE_FILES = LICENSE
LIBRESPOT_STAGING = YES
LIBRESPOT_SITE = http://openlinux.amlogic.com:8000/download/GPL_code_release/ThirdParty
LIBRESPOT_DEPENDENCIES = alsa-lib
SYSROOT_CONFIG = $(@D)/../../host/usr/bin/gcc-sysroot
LIBRESPOT_MAKE_ENV = \
                     CC="$(TARGET_CC)" \
                     CFLAGS="$(TARGET_CFLAGS)" \
                     $(TARGET_MAKE_ENV)

ifeq ($(BR2_aarch64),y)
RUST_CC = aarch64-unknown-linux-gnu
else
RUST_CC = armv7-unknown-linux-gnueabihf
endif

ifeq ($(BR2_PACKAGE_PULSEAUDIO), y)
LIBRESPOT_BACKEND = "pulseaudio-backend"
else
LIBRESPOT_BACKEND = "alsa-backend"
endif
define LIBRESPOT_INSTALL_TARGET_CMDS

	echo -e '#!/bin/bash' > $(SYSROOT_CONFIG)
	echo  "$(TARGET_CC) --sysroot $(STAGING_DIR) \"\$$@\"" >> $(SYSROOT_CONFIG)
	chmod +x $(SYSROOT_CONFIG)
	mkdir -p ~/.cargo
	echo -e '[target.$(RUST_CC)]\nlinker = "$(SYSROOT_CONFIG)"' > $(@D)/.cargo/config
	cp $(@D)/.cargo ~/ -rf
	$(@D)/.cargo/bin/rustup default nightly
	$(@D)/.cargo/bin/rustup target add $(RUST_CC)
	($(LIBRESPOT_MAKE_ENV) $(@D)/.cargo/bin/cargo build --no-default-features --features $(LIBRESPOT_BACKEND) --target $(RUST_CC)  --release --manifest-path $(@D)/Cargo.toml)
	${INSTALL} -D -m 0755 ${@D}/target/$(RUST_CC)/release/librespot  ${TARGET_DIR}/usr/bin/librespot
	#rm ~/.cargo -rf
endef

$(eval $(generic-package))
