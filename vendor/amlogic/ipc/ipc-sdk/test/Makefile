SUB_DIRS := \
	$(CURDIR)/isp \
	$(CURDIR)/ge2d \
	$(CURDIR)/venc \

all:
	for d in $(SUB_DIRS); do ($(MAKE) -C $$d ) || exit 1; done

clean:
	for d in $(SUB_DIRS); do ($(MAKE) $@ -C $$d ) || exit 1; done

