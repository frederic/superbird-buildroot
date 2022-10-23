# SPDX-License-Identifier: GPL-2.0-or-later

obj-$(CONFIG_EXFAT_FS) += exfat.o

exfat-y :=	exfat_core.o	\
		exfat_super.o	\
		exfat_blkdev.o	\
		exfat_cache.o	\
		exfat_nls.o	\
		exfat_upcase.o

ifndef CONFIG_KASAN
KBUILD_CFLAGS += -Wframe-larger-than=900
endif
