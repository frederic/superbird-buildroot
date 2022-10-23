cur_dir := $(CURDIR)

.PHONY: all clean
all:
	$(MAKE) -C $(cur_dir)/hello_world
	$(MAKE) -C $(cur_dir)/crypto
	$(MAKE) -C $(cur_dir)/optee_test

clean:
	$(MAKE) -C $(cur_dir)/hello_world clean
	$(MAKE) -C $(cur_dir)/crypto clean
	$(MAKE) -C $(cur_dir)/optee_test clean
