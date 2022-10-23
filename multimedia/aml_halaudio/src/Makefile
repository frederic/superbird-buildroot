#CC=${HOST_GCC}

#export CC BUILD_DIR STAGING_DIR TARGET_DIR
all:
	-$(MAKE) -C tinyalsa all
	-$(MAKE) -C tinyalsa install
	-$(MAKE) -C speex_rate all
	-$(MAKE) -C speex_rate install
	-$(MAKE) -C utils all
	-$(MAKE) -C utils install
	-$(MAKE) -C audio_hal all
	-$(MAKE) -C audio_hal install
	-$(MAKE) -C test all
	-$(MAKE) -C test install
install:
	-$(MAKE) -C tinyalsa install	
clean:
	-$(MAKE) -C tinyalsa clean
	-$(MAKE) -C speex_rate clean
	-$(MAKE) -C utils clean
	-$(MAKE) -C audio_hal clean
	-$(MAKE) -C test clean
