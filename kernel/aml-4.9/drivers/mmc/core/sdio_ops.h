/*
 *  linux/drivers/mmc/sdio_ops.c
 *
 *  Copyright 2006-2007 Pierre Ossman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#ifndef _MMC_SDIO_OPS_H
#define _MMC_SDIO_OPS_H

#include <linux/mmc/sdio.h>

int mmc_send_io_op_cond(struct mmc_host *host, u32 ocr, u32 *rocr);
int mmc_io_rw_direct(struct mmc_card *card, int write, unsigned fn,
	unsigned addr, u8 in, u8* out);
int mmc_io_rw_extended(struct mmc_card *card, int write, unsigned fn,
	unsigned addr, int incr_addr, u8 *buf, unsigned blocks, unsigned blksz);
int sdio_reset(struct mmc_host *host);

static inline bool mmc_is_io_op(u32 opcode)
{
	return opcode == SD_IO_RW_DIRECT || opcode == SD_IO_RW_EXTENDED;
}

struct wifi_clk_table {
	char m_wifi_name[20];
	unsigned short m_use_flag;
	unsigned short m_device_id;
	unsigned int m_uhs_max_dtr;
};

enum wifi_clk_table_e {
	WIFI_CLOCK_TABLE_8822BS = 0,
	WIFI_CLOCK_TABLE_8822CS = 1,
	WIFI_CLOCK_TABLE_MAX,
};

extern struct wifi_clk_table aWifi_clk[WIFI_CLOCK_TABLE_MAX];
#endif

