/*
 * Copyright (C) 2017 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <aml_conf_loader.h>
#include <aml_conf_parser.h>
#include <aml_data_utils.h>

static struct aml_audio_channel_name gChName[] = {
	{AML_CH_IDX_L,   "left"},
	{AML_CH_IDX_R,   "right"},
	{AML_CH_IDX_C,   "center"},
	{AML_CH_IDX_LFE, "lfe"},
	{AML_CH_IDX_LS,  "left_surround"},
	{AML_CH_IDX_RS,  "right_urround"},
	{AML_CH_IDX_LT,  "left_top"},
	{AML_CH_IDX_RT,  "right_top"},
};

static char *_get_ch_name(eChannelContentIdx idx)
{
	int i = 0;
	int cnt = 0;

	cnt = sizeof(gChName)/sizeof(struct aml_audio_channel_name);

	for (i=AML_CH_IDX_L; i<AML_I2S_CHANNEL_COUNT; i++) {
		if (idx == gChName[i].ch_idx) {
			return gChName[i].ch_name;
		}
	}

	switch (idx) {
	case AML_CH_IDX_5_1_ALL:
		return "5.1Ch";
	case AML_CH_IDX_7_1_ALL:
		return "7.1Ch";
	case AML_CH_IDX_5_1_2_ALL:
		return "5.1.2Ch";
	default:
		break;
	}

	return "Invalid";
}

int main(void)
{
	struct aml_channel_map *maps = NULL;
	int i;

	// test read product channel config
	maps = data_load_product_config();
	if (maps) {
		printf("idx mask i2s-idx invert ditter\n");
		printf("------------------------------\n");
		for (i=0; i<AML_CH_IDX_MAX; i++) {
			printf("%d   0x%02x       %d      %d      %d (%s)\n",
				maps[i].channel_idx,
				maps[i].bit_mask,
				maps[i].i2s_idx,
				maps[i].invert,
				maps[i].ditter,
				_get_ch_name(i));
		}
		printf("------------------------------\n");
	}

	// test android utils
	// test alsa mixer utils
	// test hw profile utils

	return 0;
}
