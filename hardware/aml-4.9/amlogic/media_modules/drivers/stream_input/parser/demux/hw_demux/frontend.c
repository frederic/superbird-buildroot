/*
* Copyright (C) 2017 Amlogic, Inc. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Description:
*/
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/fcntl.h>
//#include <asm/irq.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/pinctrl/consumer.h>
#include <linux/reset.h>
#include <linux/of_gpio.h>
#include <linux/amlogic/media/utils/amstream.h>
//#include <linux/clk.h>
#include "c_stb_define.h"
#include "c_stb_regs_define.h"
#include "../aml_dvb.h"
#include "dvb_reg.h"

#include "demod_gt.h"
#include "../../../../common/media_clock/switch/amports_gate.h"

#define pr_error(fmt, args...) printk("DVB: " fmt, ## args)
#define pr_inf(fmt, args...)   printk("DVB: " fmt, ## args)

static struct dvb_frontend *frontend[FE_DEV_COUNT] = {NULL, NULL};
static enum dtv_demod_type s_demod_type[FE_DEV_COUNT] = {AM_DTV_DEMOD_NONE, AM_DTV_DEMOD_NONE};
static enum tuner_type s_tuner_type[FE_DEV_COUNT] = {AM_TUNER_NONE, AM_TUNER_NONE};


ssize_t stb_show_tuner_setting(struct class *class,
				   struct class_attribute *attr, char *buf)
{
	struct aml_dvb *dvb = aml_get_dvb_device();

	if (dvb->tuner_cur >= 0)
		pr_inf("dvb current attatch tuner %d, id: %d\n",
				dvb->tuner_cur, dvb->tuners[dvb->tuner_cur].cfg.id);
	else
		pr_inf("dvb has no attatch tuner.\n");

	return 0;
}

ssize_t stb_store_tuner_setting(struct class *class,
				    struct class_attribute *attr,
				    const char *buf, size_t count)
{
	int n = 0, i = 0, val = 0;
	unsigned long tmp = 0;
	char *buf_orig = NULL, *ps = NULL, *token = NULL;
	char *parm[4] = { NULL };
	struct aml_dvb *dvb = aml_get_dvb_device();
	int tuner_id = 0;
	struct aml_tuner *tuner = NULL;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	ps = buf_orig;

	while (1) {
		token = strsep(&ps, "\n ");
		if (token == NULL)
			break;
		if (*token == '\0')
			continue;
		parm[n++] = token;
	}

	if (parm[0] && kstrtoul(parm[0], 10, &tmp) == 0) {
		val = tmp;

		for (i = 0; i < dvb->tuner_num; ++i) {
			if (dvb->tuners[i].cfg.id == val) {
				tuner_id = dvb->tuners[i].cfg.id;
				break;
			}
		}

		if (tuner_id == 0 || dvb->tuner_cur == i) {
			pr_error("%s: set nonsupport or the same tuner %d.\n",
					__func__, val);
			goto EXIT;
		}

		dvb->tuner_cur = i;

		for (i = 0; i < FE_DEV_COUNT; i++) {
			tuner = &dvb->tuners[dvb->tuner_cur];

			if (frontend[i] == NULL)
				continue;

			if (aml_attach_tuner(tuner->cfg.id, frontend[i], &tuner->cfg) == NULL) {
				s_tuner_type[i] = AM_TUNER_NONE;
				pr_error("tuner[%d] [type = %d] attach error.\n", dvb->tuner_cur, tuner->cfg.id);
				goto EXIT;
			} else {
				s_tuner_type[i] = tuner->cfg.id;
				pr_error("tuner[%d] [type = %d] attach sucess.\n", dvb->tuner_cur, tuner->cfg.id);
			}
		}

		pr_error("%s: attach tuner %d done.\n", __func__, dvb->tuners[dvb->tuner_cur].cfg.id);
	}

EXIT:

	return count;
}


int frontend_probe(struct platform_device *pdev)
{
	struct demod_config config;
	struct tuner_config *tuner_cfg = NULL;
	char buf[32];
	const char *str = NULL;
	struct device_node *node_tuner = NULL;
	struct device_node *node_i2c = NULL;
	u32 i2c_addr = 0xFFFFFFFF;
	u32 value = 0;
	int i = 0;
	int ret =0;
	int j = 0;
	struct aml_dvb *advb = aml_get_dvb_device();

	for (i=0; i<FE_DEV_COUNT; i++) {
		memset(&config, 0, sizeof(struct demod_config));

		memset(buf, 0, 32);
		snprintf(buf, sizeof(buf), "fe%d_mode", i);
		ret = of_property_read_string(pdev->dev.of_node, buf, &str);
		if (ret) {
			continue;
		}

		if (!strcmp(str, "internal")) {
			config.mode = 0;
			config.id = AM_DTV_DEMOD_AMLDTV;
			frontend[i] = aml_attach_dtvdemod(config.id, &config);
			if (frontend[i] == NULL) {
				s_demod_type[i] = AM_DTV_DEMOD_NONE;
				pr_error("internal dtvdemod [type = %d] attach error.\n", config.id);
				goto error_fe;
			} else {
				s_demod_type[i] = config.id;
				pr_error("internal dtvdemod [type = %d] attach success.\n", config.id);
			}

			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "fe%d_tuner",i);
			node_tuner = of_parse_phandle(pdev->dev.of_node, buf, 0);
			if (!node_tuner){
				pr_err("can't find tuner.\n");
				goto error_fe;
			}
			ret = of_property_read_u32(node_tuner, "tuner_num", &value);
			if (ret) {
				pr_err("can't find tuner_num.\n");
				goto error_fe;
			} else
				advb->tuner_num = value;

			advb->tuners = kzalloc(sizeof(struct aml_tuner) * advb->tuner_num, GFP_KERNEL);
			if (!advb->tuners) {
				pr_err("can't kzalloc for tuners.\n");
				goto error_fe;
			}

			ret = of_property_read_u32(node_tuner, "tuner_cur", &value);
			if (ret) {
				pr_err("can't find tuner_cur, use default 0.\n");
				advb->tuner_cur = -1;
			} else
				advb->tuner_cur = value;

			for (j = 0; j < advb->tuner_num; ++j) {
				snprintf(buf, sizeof(buf), "tuner_name_%d", j);
				ret = of_property_read_string(node_tuner, buf, &str);
				if (ret) {
					//pr_error("tuner%d type error\n",i);
					ret = 0;
					continue;
				} else {
					advb->tuners[j].cfg.id = aml_get_tuner_type(str);
					if (advb->tuners[j].cfg.id == AM_TUNER_NONE)
						pr_err("can't support tuner: %s.\n", str);
				}

				snprintf(buf, sizeof(buf), "tuner_i2c_adap_%d", j);
				node_i2c = of_parse_phandle(node_tuner, buf, 0);
				if (!node_i2c) {
					pr_error("tuner_i2c_adap_id error\n");
				} else {
					advb->tuners[j].cfg.i2c_adap = of_find_i2c_adapter_by_node(node_i2c);
					of_node_put(node_i2c);
					if (advb->tuners[j].cfg.i2c_adap == NULL) {
						pr_error("i2c_get_adapter error\n");
						of_node_put(node_tuner);
						goto error_fe;
					}
				}

				snprintf(buf, sizeof(buf), "tuner_i2c_addr_%d", j);
				ret = of_property_read_u32(node_tuner, buf, &i2c_addr);
				if (ret) {
					pr_error("i2c_addr error\n");
				}
				else
					advb->tuners[j].cfg.i2c_addr = i2c_addr;

				snprintf(buf, sizeof(buf), "tuner_xtal_%d", j);
				ret = of_property_read_u32(node_tuner, buf, &value);
				if (ret)
					pr_err("tuner_xtal error.\n");
				else
					advb->tuners[j].cfg.xtal = value;

				snprintf(buf, sizeof(buf), "tuner_xtal_mode_%d", j);
				ret = of_property_read_u32(node_tuner, buf, &value);
				if (ret)
					pr_err("tuner_xtal_mode error.\n");
				else
					advb->tuners[j].cfg.xtal_mode = value;

				snprintf(buf, sizeof(buf), "tuner_xtal_cap_%d", j);
				ret = of_property_read_u32(node_tuner, buf, &value);
				if (ret)
					pr_err("tuner_xtal_cap error.\n");
				else
					advb->tuners[j].cfg.xtal_cap = value;
			}

			of_node_put(node_tuner);

			/* define general-purpose callback pointer */
			frontend[i]->callback = NULL;

			if (advb->tuner_cur >= 0) {
				tuner_cfg = &advb->tuners[advb->tuner_cur].cfg;
				if (aml_attach_tuner(tuner_cfg->id, frontend[i], tuner_cfg) == NULL) {
					s_tuner_type[i] = AM_TUNER_NONE;
					pr_error("tuner [type = %d] attach error.\n", tuner_cfg->id);
					goto error_fe;
				} else {
					s_tuner_type[i] = tuner_cfg->id;
					pr_error("tuner [type = %d] attach sucess.\n", tuner_cfg->id);
				}
			}

			ret = dvb_register_frontend(&advb->dvb_adapter, frontend[i]);
			if (ret) {
				pr_error("register dvb frontend failed\n");
				goto error_fe;
			}
		} else if(!strcmp(str, "external")) {
			config.mode = 1;
			config.id = AM_DTV_DEMOD_NONE;
			ret = aml_get_dts_demod_config(pdev->dev.of_node, &config, i);
			if (ret) {
				pr_err("can't find demod %d.\n", i);
				continue;
			}

			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "fe%d_tuner", i);
			node_tuner = of_parse_phandle(pdev->dev.of_node, buf, 0);
			if (node_tuner) {
				aml_get_dts_tuner_config(node_tuner, &config.tuner0, 0);
				aml_get_dts_tuner_config(node_tuner, &config.tuner1, 1);
			} else
				pr_err("can't find %s.\n", buf);

			of_node_put(node_tuner);

			frontend[i] = aml_attach_dtvdemod(config.id, &config);
			if (frontend[i] == NULL) {
				s_demod_type[i] = AM_DTV_DEMOD_NONE;
				pr_error("external dtvdemod [type = %d] attach error.\n", config.id);
				goto error_fe;
			} else {
				s_demod_type[i] = config.id;
				pr_error("external dtvdemod [type = %d] attach success.\n", config.id);
			}

			if (frontend[i]) {
				ret = dvb_register_frontend(&advb->dvb_adapter, frontend[i]);
				if (ret) {
					pr_error("register dvb frontend failed\n");
					goto error_fe;
				}
			}
		}
	}
	if (advb->tuners)
		kfree(advb->tuners);
	return 0;
error_fe:
	for (i=0; i<FE_DEV_COUNT; i++) {
		aml_detach_dtvdemod(s_demod_type[i]);
		frontend[i] = NULL;
		s_demod_type[i] = AM_DTV_DEMOD_NONE;

		aml_detach_tuner(s_tuner_type[i]);
		s_tuner_type[i] = AM_TUNER_NONE;
	}

	if (advb->tuners)
		kfree(advb->tuners);

	return 0;
}

int frontend_remove(void)
{
	int i;

	for (i=0; i<FE_DEV_COUNT; i++) {
		aml_detach_dtvdemod(s_demod_type[i]);

		aml_detach_tuner(s_tuner_type[i]);

		if (frontend[i] &&
			((s_tuner_type[i] == AM_TUNER_SI2151)
			|| (s_tuner_type[i] == AM_TUNER_MXL661)
			|| (s_tuner_type[i] == AM_TUNER_SI2159)
			|| (s_tuner_type[i] == AM_TUNER_R842)
			|| (s_tuner_type[i] == AM_TUNER_R840)
			|| (s_tuner_type[i] == AM_TUNER_ATBM2040))) {
			dvb_unregister_frontend(frontend[i]);
			dvb_frontend_detach(frontend[i]);
		}

		frontend[i] = NULL;
		s_demod_type[i] = AM_DTV_DEMOD_NONE;
		s_tuner_type[i] = AM_TUNER_NONE;

	}
	return 0;
}

