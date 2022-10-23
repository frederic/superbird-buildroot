CC ?= gcc

src_list := main_linux.c easy_setup_linux.c easy_setup/easy_setup.c easy_setup/scan.c proto/mcast.c proto/neeze.c proto/akiss.c proto/changhong.c proto/jingdong.c proto/jd.c proto/xiaoyi.c proto/ap.c
obj_list := $(src_list:%.c=%.o)

CFLAGS := -Ieasy_setup -Iproto

.PHONY: setup

setup:$(obj_list)
	$(CC) -o $@ $^
	
%.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $^

clean:
	rm $(obj_list) setup


