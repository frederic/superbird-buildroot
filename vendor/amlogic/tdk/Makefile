#
# Amlogic Trusted Application Development Kit
#

cur_dir=$(CURDIR)
V:=@

.PHONY: all clean driver clean-driver release

all:
	$(if $(wildcard $(cur_dir)/demos), $(MAKE) -C $(cur_dir)/demos)

clean:
	$(if $(wildcard $(cur_dir)/demos), $(MAKE) -C $(cur_dir)/demos clean)
	$(if $(wildcard $(cur_dir)/tdk.tar.gz), $(V)rm -rf $(cur_dir)/tdk.tar.gz)

driver:
	cd $(cur_dir)/linuxdriver && ./build_gx.sh

clean-driver:
	cd $(cur_dir)/linuxdriver && ./build_gx.sh clean

cleanall: clean clean-driver

release:
	$(if $(wildcard /tmp/tdk), $(V)rm -rf /tmp/tdk)
	$(if $(wildcard $(cur_dir)/tdk.tar.gz), $(V)rm -rf $(cur_dir)/tdk.tar.gz)
	$(V)mkdir -p /tmp/tdk
	$(V)cp -rf $(cur_dir)/* /tmp/tdk
	$(V)cd /tmp && tar czf $(cur_dir)/tdk.tar.gz tdk
	$(V)rm -rf /tmp/tdk
	$(V)echo "---RELEASE TDK DONE---"
