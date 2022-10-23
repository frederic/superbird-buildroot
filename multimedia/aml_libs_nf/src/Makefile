#CC=${HOST_GCC}
#export CC BUILD_DIR STAGING_DIR TARGET_DIR
all:
	mkdir -p $(shell pwd)/../../staging/usr/lib/aml_libs_nf
	-$(MAKE) -C amadec all
	-${MAKE} -C audio_codec all
	-$(MAKE) -C amavutils all
	-$(MAKE) -C amcodec all
	echo $(BR2_TARGET_BOARD_PLATFORM)
	echo $(FIRMDIR)
install:
	-$(MAKE) -C amadec install
	-$(MAKE) -C amavutils install
	-$(MAKE) -C amcodec install
	-${MAKE} -C audio_codec install
	cp  -rf $(AML_LIBS_NF_STAGING_DIR)/usr/lib/aml_libs_nf/*.so $(TARGET_DIR)/usr/lib
	if [ $(AML_LIBS_NF_BR2_ARCH) = "arm" ]; then \
		cp  -rf audio_codec/libeac3/*.so $(TARGET_DIR)/usr/lib; \
	else \
		echo "No 64bit Dobly libraries available."; \
	fi
clean:
	-$(MAKE) -C amadec clean
	-$(MAKE) -C amavutils clean
	-$(MAKE) -C amcodec clean
	-${MAKE} -C audio_codec clean
