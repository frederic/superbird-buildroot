/*
 * drivers/amlogic/media/vout/lcd/lcd_extern/i2c_T5800Q.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/amlogic/i2c-amlogic.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/amlogic/media/vout/lcd/lcd_extern.h>
#include "lcd_extern.h"

#define LCD_EXTERN_NAME		  "i2c_CS602"
#define LCD_EXTERN_I2C_ADDR       (0x66) /* 7bit address */
#define LCD_EXTERN_I2C_BUS        LCD_EXTERN_I2C_BUS_2

static struct lcd_extern_config_s *ext_config;
static struct aml_lcd_extern_i2c_dev_s *i2c_dev;

#define LCD_EXTERN_CMD_SIZE        LCD_EXT_CMD_SIZE_DYNAMIC

static int lcd_extern_reg_read(unsigned char reg, unsigned char *buf)
{
	int ret = 0;

	return ret;
}

static int lcd_extern_reg_write(unsigned char *buf, unsigned int len)
{
	int ret = 0;

	if (buf == NULL) {
		EXTERR("%s: buf is full\n", __func__);
		return -1;
	}

	if (!len) {
		EXTERR("%s: invalid len\n", __func__);
		return -1;
	}

	ret = lcd_extern_i2c_write(i2c_dev->client, buf, len);
	return ret;
}

static int lcd_extern_init_check(int len)
{
	int ret = 0;
	unsigned char *chk_table;
	int i;

	chk_table = kmalloc((sizeof(unsigned char) * len),
		GFP_KERNEL);
	if (chk_table == NULL) {
		EXTERR("%s: failed to alloc chk_table, not enough memory\n",
			LCD_EXTERN_NAME);
		return ret;
	}
	memset(chk_table, 0, len);

	if (i2c_dev->client == NULL) {
		EXTERR("%s: invalid i2c client\n", __func__);
		kfree(chk_table);
		chk_table = NULL;
		return -1;
	}
	ret = lcd_extern_i2c_read(i2c_dev->client, chk_table, len);
	if (ret == 0) {
		for (i = 0; i < len; i++) {
			if (chk_table[i] != ext_config->table_init_on[i+3]) {
				kfree(chk_table);
				chk_table = NULL;
				return -1;
			}
		}
	}

	kfree(chk_table);
	chk_table = NULL;
	return 0;
}

static int lcd_extern_power_cmd_dynamic_size(unsigned char *table, int flag)
{
	int i = 0, j, step = 0, max_len = 0;
	unsigned char type, cmd_size;
	int delay_ms, ret = 0;

	if (flag)
		max_len = ext_config->table_init_on_cnt;
	else
		max_len = ext_config->table_init_off_cnt;

	while ((i + 1) < max_len) {
		type = table[i];
		if (type == LCD_EXT_CMD_TYPE_END)
			break;
		if (lcd_debug_print_flag) {
			EXTPR("%s: step %d: type=0x%02x, cmd_size=%d\n",
				__func__, step, type, table[i+1]);
		}
		cmd_size = table[i+1];
		if (cmd_size == 0)
			goto power_cmd_dynamic_next;
		if ((i + 2 + cmd_size) > max_len)
			break;

		if (type == LCD_EXT_CMD_TYPE_NONE) {
			/* do nothing */
		} else if (type == LCD_EXT_CMD_TYPE_GPIO) {
			if (cmd_size < 2) {
				EXTERR(
				"step %d: invalid cmd_size %d for GPIO\n",
					step, cmd_size);
				goto power_cmd_dynamic_next;
			}
			if (table[i+2] < LCD_GPIO_MAX)
				lcd_extern_gpio_set(table[i+2], table[i+3]);
			if (cmd_size > 2) {
				if (table[i+4] > 0)
					mdelay(table[i+4]);
			}
		} else if (type == LCD_EXT_CMD_TYPE_DELAY) {
			delay_ms = 0;
			for (j = 0; j < cmd_size; j++)
				delay_ms += table[i+2+j];
			if (delay_ms > 0)
				mdelay(delay_ms);
		} else if ((type == LCD_EXT_CMD_TYPE_CMD) ||
			   (type == LCD_EXT_CMD_TYPE_CMD_BIN)) {
			if (i2c_dev == NULL) {
				EXTERR("invalid i2c device\n");
				return -1;
			}
			ret = lcd_extern_reg_write(&table[i+2], cmd_size);
		} else if (type == LCD_EXT_CMD_TYPE_CMD_DELAY) {
			if (i2c_dev == NULL) {
				EXTERR("invalid i2c device\n");
				return -1;
			}
			ret = lcd_extern_reg_write(&table[i+2], (cmd_size-1));
			if (table[i+cmd_size+1] > 0)
				mdelay(table[i+cmd_size+1]);
		} else {
			EXTERR("%s: %s(%d): type 0x%02x invalid\n",
				__func__, ext_config->name,
				ext_config->index, type);
		}
power_cmd_dynamic_next:
		i += (cmd_size + 2);
		step++;
	}

	return ret;
}

static int lcd_extern_power_cmd_fixed_size(unsigned char *table, int flag)
{
	int i = 0, j, step = 0, max_len = 0;
	unsigned char type, cmd_size;
	int delay_ms, ret = 0;

	cmd_size = ext_config->cmd_size;
	if (cmd_size < 2) {
		EXTERR("%s: invalid cmd_size %d\n", __func__, cmd_size);
		return -1;
	}

	if (flag)
		max_len = ext_config->table_init_on_cnt;
	else
		max_len = ext_config->table_init_off_cnt;

	while ((i + cmd_size) <= max_len) {
		type = table[i];
		if (type == LCD_EXT_CMD_TYPE_END)
			break;
		if (lcd_debug_print_flag) {
			EXTPR("%s: step %d: type=0x%02x, cmd_size=%d\n",
				__func__, step, type, cmd_size);
		}
		if (type == LCD_EXT_CMD_TYPE_NONE) {
			/* do nothing */
		} else if (type == LCD_EXT_CMD_TYPE_GPIO) {
			if (table[i+1] < LCD_GPIO_MAX)
				lcd_extern_gpio_set(table[i+1], table[i+2]);
			if (cmd_size > 3) {
				if (table[i+3] > 0)
					mdelay(table[i+3]);
			}
		} else if (type == LCD_EXT_CMD_TYPE_DELAY) {
			delay_ms = 0;
			for (j = 0; j < (cmd_size - 1); j++)
				delay_ms += table[i+1+j];
			if (delay_ms > 0)
				mdelay(delay_ms);
		} else if ((type == LCD_EXT_CMD_TYPE_CMD) ||
			   (type == LCD_EXT_CMD_TYPE_CMD_BIN)) {
			if (i2c_dev == NULL) {
				EXTERR("invalid i2c device\n");
				return -1;
			}
			ret = lcd_extern_reg_write(&table[i+1], (cmd_size-1));
		} else if (type == LCD_EXT_CMD_TYPE_CMD_DELAY) {
			if (i2c_dev == NULL) {
				EXTERR("invalid i2c device\n");
				return -1;
			}
			ret = lcd_extern_reg_write(&table[i+1], (cmd_size-2));
			if (table[i+cmd_size-1] > 0)
				mdelay(table[i+cmd_size-1]);
		} else {
			EXTERR("%s: %s(%d): type 0x%02x invalid\n",
				__func__, ext_config->name,
				ext_config->index, type);
		}
		i += cmd_size;
		step++;
	}

	return ret;
}

static int lcd_extern_power_ctrl(int flag)
{
	unsigned char *table;
	unsigned char cmd_size;
	unsigned char check_data;
	unsigned char check_len = 1;
	int ret = 0;
	int i = 0;

	cmd_size = ext_config->cmd_size;
	if (flag)
		table = ext_config->table_init_on;
	else
		table = ext_config->table_init_off;
	if (cmd_size < 1) {
		EXTERR("%s: cmd_size %d is invalid\n", __func__, cmd_size);
		return -1;
	}
	if (table == NULL) {
		EXTERR("%s: init_table %d is NULL\n", __func__, flag);
		return -1;
	}

	ret = lcd_extern_init_check(cmd_size);
	if (!ret) {
		if (cmd_size == LCD_EXT_CMD_SIZE_DYNAMIC)
			ret = lcd_extern_power_cmd_dynamic_size(table, flag);
		else
			ret = lcd_extern_power_cmd_fixed_size(table, flag);
	}

	for (i = 0; i < 10; i++) {
		ret = lcd_extern_i2c_read(i2c_dev->client,
					  &check_data, check_len);
		if (!((ret >> 6) & 0x1)) {
			if (cmd_size == LCD_EXT_CMD_SIZE_DYNAMIC)
				ret =
				lcd_extern_power_cmd_dynamic_size(table, flag);
			else
				ret =
				lcd_extern_power_cmd_fixed_size(table, flag);
		} else
			break;
	}

	EXTPR("%s: %s(%d): %d\n",
		__func__, ext_config->name, ext_config->index, flag);
	return ret;
}

static int lcd_extern_power_on(void)
{
	int ret;

	lcd_extern_pinmux_set(1);
	ret = lcd_extern_power_ctrl(1);
	return ret;
}

static int lcd_extern_power_off(void)
{
	int ret;

	ret = lcd_extern_power_ctrl(0);
	lcd_extern_pinmux_set(0);
	return ret;
}

static int lcd_extern_driver_update(struct aml_lcd_extern_driver_s *ext_drv)
{
	if (ext_drv == NULL) {
		EXTERR("%s driver is null\n", LCD_EXTERN_NAME);
		return -1;
	}

	if (ext_drv->config->table_init_loaded == 0) {
		EXTERR("%s: table_init is invalid\n", ext_drv->config->name);
		return -1;
	}

	ext_drv->power_on  = lcd_extern_power_on;
	ext_drv->power_off = lcd_extern_power_off;
	ext_drv->reg_read = lcd_extern_reg_read;

	return 0;
}

int aml_lcd_extern_i2c_CS602_probe(struct aml_lcd_extern_driver_s *ext_drv)
{
	int ret = 0;

	ext_config = ext_drv->config;

	i2c_dev = lcd_extern_get_i2c_device(ext_config->i2c_addr);
	if (i2c_dev == NULL) {
		EXTERR("invalid i2c device\n");
		return -1;
	}
	EXTPR("get i2c device: %s, addr 0x%02x OK\n",
		i2c_dev->name, i2c_dev->client->addr);

	ret = lcd_extern_driver_update(ext_drv);
	EXTPR("%s: %d\n", __func__, ret);
	return ret;
}

int aml_lcd_extern_i2c_CS602_remove(void)
{
	i2c_dev = NULL;
	ext_config = NULL;

	return 0;
}

