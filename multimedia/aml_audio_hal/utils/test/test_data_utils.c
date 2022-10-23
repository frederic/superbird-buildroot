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
#include <aml_data_utils.h>

/******************************************************************************
 * All parttens of channel maps@xiaomi
 *****************************************************************************/
struct aml_channel_map atmos_maps[8] = {
	{AML_CH_IDX_L,   AML_I2S_PORT_IDX_01, AML_I2S_CHANNEL_0,                                    0,    AML_I2S_CHANNEL_0},
	{AML_CH_IDX_R,   AML_I2S_PORT_IDX_01, AML_I2S_CHANNEL_1,                                    0,    AML_I2S_CHANNEL_1},
	{AML_CH_IDX_C,   AML_I2S_PORT_IDX_23, AML_I2S_CHANNEL_2,                                    0,    AML_I2S_CHANNEL_2},
	{AML_CH_IDX_LFE, AML_I2S_PORT_IDX_23, AML_I2S_CHANNEL_3,                                    0,    AML_I2S_CHANNEL_3},
	{AML_CH_IDX_LS,  AML_I2S_PORT_IDX_67, AML_I2S_CHANNEL_6,                                    0,    AML_I2S_CHANNEL_6},
	{AML_CH_IDX_RS,  AML_I2S_PORT_IDX_67, AML_I2S_CHANNEL_7,                                    0,    AML_I2S_CHANNEL_7},
	{AML_CH_IDX_LT,  AML_I2S_PORT_IDX_45, AML_I2S_CHANNEL_4,                                    0,    AML_I2S_CHANNEL_4},
	{AML_CH_IDX_RT,  AML_I2S_PORT_IDX_45, AML_I2S_CHANNEL_5,                                    0,    AML_I2S_CHANNEL_5},
};

struct aml_channel_map mi_maps[8] = {
	{AML_CH_IDX_L,   AML_I2S_PORT_IDX_01,   AML_I2S_CHANNEL_0,                                   0,                   0},
	{AML_CH_IDX_R,   AML_I2S_PORT_IDX_01,   AML_I2S_CHANNEL_1,                                   0,                   0},
	{AML_CH_IDX_C,   AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
	{AML_CH_IDX_LFE, AML_I2S_PORT_IDX_23,   AML_I2S_CHANNEL_2|AML_I2S_CHANNEL_3,                 0,                   0},
	{AML_CH_IDX_LS,  AML_I2S_PORT_IDX_45,   AML_I2S_CHANNEL_4,                                   0,                   0},
	{AML_CH_IDX_RS,  AML_I2S_PORT_IDX_45,   AML_I2S_CHANNEL_5,                                   0,                   0},
	{AML_CH_IDX_LT,  AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
	{AML_CH_IDX_RT,  AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
};

struct aml_channel_map rainman_maps[8] = {
	{AML_CH_IDX_L,   AML_I2S_PORT_IDX_01,   AML_I2S_CHANNEL_0,                                   0,                   0},
	{AML_CH_IDX_R,   AML_I2S_PORT_IDX_01,   AML_I2S_CHANNEL_1,                                   0,                   0},
	{AML_CH_IDX_C,   AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
	{AML_CH_IDX_LFE, AML_I2S_PORT_IDX_23,   AML_I2S_CHANNEL_2|AML_I2S_CHANNEL_3, AML_I2S_CHANNEL_2,                   0},
	{AML_CH_IDX_LS,  AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
	{AML_CH_IDX_RS,  AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
	{AML_CH_IDX_LT,  AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
	{AML_CH_IDX_RT,  AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
};

struct aml_channel_map pulpfiction_maps[8] = {
	{AML_CH_IDX_L,   AML_I2S_PORT_IDX_01,   AML_I2S_CHANNEL_0,                                   0,                   0},
	{AML_CH_IDX_R,   AML_I2S_PORT_IDX_01,   AML_I2S_CHANNEL_1,                                   0,                   0},
	{AML_CH_IDX_C,   AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
	{AML_CH_IDX_LFE, AML_I2S_PORT_IDX_23,   AML_I2S_CHANNEL_2|AML_I2S_CHANNEL_3,                 0,                   0},
	{AML_CH_IDX_LS,  AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
	{AML_CH_IDX_RS,  AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
	{AML_CH_IDX_LT,  AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
	{AML_CH_IDX_RT,  AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
};
struct aml_channel_map matrix_maps[8] = {
	{AML_CH_IDX_L,   AML_I2S_PORT_IDX_23,   AML_I2S_CHANNEL_2,                                   0,                   0},
	{AML_CH_IDX_R,   AML_I2S_PORT_IDX_23,   AML_I2S_CHANNEL_3,                                   0,                   0},
	{AML_CH_IDX_C,   AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
	{AML_CH_IDX_LFE, AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
	{AML_CH_IDX_LS,  AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
	{AML_CH_IDX_RS,  AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
	{AML_CH_IDX_LT,  AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
	{AML_CH_IDX_RT,  AML_I2S_PORT_IDX_NULL, AML_I2S_CHANNEL_NULL,                                0,                   0},
};

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

static int dump_channel_map(const char *prj, struct aml_channel_map *maps)
{
	int i;

	if (maps) {
		printf("\n\n=============================================================\n");
		printf("idx i2s-idx mask invert ditter [PRJ: %s]\n", prj);
		printf("------------------------------\n");
		for (i=0; i<AML_CH_IDX_MAX; i++) {
			if (AML_I2S_PORT_IDX_NULL == maps[i].i2s_idx
				|| AML_I2S_CHANNEL_NULL == maps[i].bit_mask) {
				printf("%d      -    ----   ----   ---- (%s)\n",
					maps[i].channel_idx,
					_get_ch_name(i));
			} else {
				printf("%d      %d    0x%02x   0x%02x   0x%02x (%s)\n",
					maps[i].channel_idx,
					maps[i].i2s_idx,
					maps[i].bit_mask,
					maps[i].invert,
					maps[i].ditter,
					_get_ch_name(i));
			}
		}
		printf("------------------------------\n\n");
	}

	return 0;
}

static int testpartten_chmap(const char *prj, struct aml_channel_map *maps)
{
	int num, i;
	int port;
	int bit_ch;
	int bit;
	int idx;

	dump_channel_map(prj, maps);
	for (num=AML_CH_IDX_L; num<AML_CH_IDX_MAX; num++) {
		port = data_get_channel_i2s_port(maps, num);
		if (port != AML_I2S_PORT_IDX_NULL) {
			printf("----------------------------------------------\n");
			bit_ch = data_get_channel_bit_mask(maps, num);
			printf("[chan_idx][%d] %s <-> i2s:%02d <-> mask:0x%02x\n", num,
				_get_ch_name(num),
				port,
				bit_ch);
			for (i=0; i<8; i++) {
				bit = bit_ch&(0x1<<i);
				if (bit) {
					idx = data_get_channel_content_idx(maps, bit);
					printf("\n\t[bit_mask] 0x%02x <-> %s\n", bit, _get_ch_name(idx));
				}
			}
		}
	}

	return 0;
}

/******************************************************************************
 *  DATA TEST PARTTENS
 *****************************************************************************/
int16_t data16[8*8] = {
	0x1101, 0x1102, 0x1103, 0x1104, 0x1105, 0x1106, 0x1107, 0x1108,
	0x1111, 0x1112, 0x1113, 0x1114, 0x1115, 0x1116, 0x1117, 0x1118,
	0x1121, 0x1122, 0x1123, 0x1124, 0x1125, 0x1126, 0x1127, 0x1128,
	0x1131, 0x1132, 0x1133, 0x1134, 0x1135, 0x1136, 0x1137, 0x1138,
	0x1141, 0x1142, 0x1143, 0x1144, 0x1145, 0x1146, 0x1147, 0x1148,
	0x1151, 0x1152, 0x1153, 0x1154, 0x1155, 0x1156, 0x1157, 0x1158,
	0x1161, 0x1162, 0x1163, 0x1164, 0x1165, 0x1166, 0x1167, 0x1168,
	0x1171, 0x1172, 0x1173, 0x1174, 0x1175, 0x1176, 0x1177, 0x1178,
};

int32_t data32[8*8] = {
	0x11111101, 0x11111102, 0x11111103, 0x11111104, 0x11111105, 0x11111106, 0x11111107, 0x11111108,
	0x11111111, 0x11111112, 0x11111113, 0x11111114, 0x11111115, 0x11111116, 0x11111117, 0x11111118,
	0x11111121, 0x11111122, 0x11111123, 0x11111124, 0x11111125, 0x11111126, 0x11111127, 0x11111128,
	0x11111131, 0x11111132, 0x11111133, 0x11111134, 0x11111135, 0x11111136, 0x11111137, 0x11111138,
	0x11111141, 0x11111142, 0x11111143, 0x11111144, 0x11111145, 0x11111146, 0x11111147, 0x11111148,
	0x11111151, 0x11111152, 0x11111153, 0x11111154, 0x11111155, 0x11111156, 0x11111157, 0x11111158,
	0x11111161, 0x11111162, 0x11111163, 0x11111164, 0x11111165, 0x11111166, 0x11111167, 0x11111168,
	0x11111171, 0x11111172, 0x11111173, 0x11111174, 0x11111175, 0x11111176, 0x11111177, 0x11111178,
};

int _dump_data(void *buf, int channels, int framesz, int frames)
{
	int16_t *buf16 = (int16_t *)buf;
	int32_t *buf32 = (int32_t *)buf;
	int i, j;

	for (i=0; i<frames; i++) {
		if (framesz == e16BitPerSample) {
			for (j=0; j<channels; j++) {
				printf(" 0x%08x", buf16[i*channels + j]);
			}
			printf("\n");
		} else if (framesz == e32BitPerSample) {
			for (j=0; j<channels; j++) {
				printf(" 0x%08x", buf32[i*channels + j]);
			}
			printf("\n");
		}
	}

	return 0;
}
/******************************************
 *  SINGLE BUFFER
 *****************************************/
/**
 **        empty
 */
static int testpartten_empty(struct  aml_channel_map *map)
{
	int bit_empty = 0;

	printf("\n==16==\n");
	_dump_data((void *)data16, 8, e16BitPerSample, 8);
	bit_empty  = 0;
	bit_empty  = data_get_channel_bit_mask(map, AML_CH_IDX_L);
	bit_empty |= data_get_channel_bit_mask(map, AML_CH_IDX_LFE);
	data_empty_channels(map, (void *)data16, 8, e16BitPerSample, 8, bit_empty);
	printf("->after empty:\n");
	_dump_data((void *)data16, 8, e16BitPerSample, 8);


	printf("\n==32==\n");
	_dump_data((void *)data32, 8, e32BitPerSample, 8);
	bit_empty  = 0;
	bit_empty  = data_get_channel_bit_mask(map, AML_CH_IDX_C);
	bit_empty |= data_get_channel_bit_mask(map, AML_CH_IDX_LFE);
	data_empty_channels(map, (void *)data32, 8, e32BitPerSample, 8, bit_empty);
	printf("->after empty:\n");
	_dump_data((void *)data32, 8, e32BitPerSample, 8);

	return 0;
}
/**
 **        invert
 */
static int testpartten_invert(struct  aml_channel_map *map)
{
	int bit_mask = 0;

	printf("\n==16==\n\n");
	_dump_data((void *)data16, 8, e16BitPerSample, 8);
	bit_mask  = 0;
	bit_mask  = data_get_channel_bit_mask(map, AML_CH_IDX_L);
	bit_mask |= data_get_channel_bit_mask(map, AML_CH_IDX_LFE);
	data_invert_channels((void *)data16, 8, e16BitPerSample, 8, bit_mask);
	printf("->after invert:\n");
	_dump_data((void *)data16, 8, e16BitPerSample, 8);

	printf("\n==32==\n\n");
	_dump_data((void *)data32, 8, e32BitPerSample, 8);
	bit_mask  = 0;
	bit_mask  = data_get_channel_bit_mask(map, AML_CH_IDX_L);
	bit_mask |= data_get_channel_bit_mask(map, AML_CH_IDX_LFE);
	data_invert_channels((void *)data32, 8, e32BitPerSample, 8, bit_mask);
	printf("->after invert:\n");
	_dump_data((void *)data32, 8, e32BitPerSample, 8);

	return 0;
}
/**
 **        exchange
 */
static int testpartten_exchange(struct  aml_channel_map *map)
{
	if (map == NULL)
		return -1;

	printf("\n==16==\n\n");
	_dump_data((void *)data16, 8, e16BitPerSample, 8);
	data_exchange_i2s_channels((void *)data16, 8, e16BitPerSample, 8, AML_I2S_PORT_IDX_01, AML_I2S_PORT_IDX_23);
	printf("\n->after exchange:\n");
	_dump_data((void *)data16, 8, e16BitPerSample, 8);

	printf("\n==32==\n\n");
	_dump_data((void *)data32, 8, e32BitPerSample, 8);
	data_exchange_i2s_channels((void *)data32, 8, e32BitPerSample, 8, AML_I2S_PORT_IDX_01, AML_I2S_PORT_IDX_67);
	printf("->after exchange:\n");
	_dump_data((void *)data32, 8, e32BitPerSample, 8);

	return 0;
}
/**
 **        re-mix
 */
int16_t remix16[6*8] = {
	0x1101, 0x1102, 0x1103, 0x1104, 0x1105, 0x1106,
	0x1111, 0x1112, 0x1113, 0x1114, 0x1115, 0x1116,
	0x1121, 0x1122, 0x1123, 0x1124, 0x1125, 0x1126,
	0x1131, 0x1132, 0x1133, 0x1134, 0x1135, 0x1136,
	0x1141, 0x1142, 0x1143, 0x1144, 0x1145, 0x1146,
	0x1151, 0x1152, 0x1153, 0x1154, 0x1155, 0x1156,
	0x1161, 0x1162, 0x1163, 0x1164, 0x1165, 0x1166,
	0x1171, 0x1172, 0x1173, 0x1174, 0x1175, 0x1176,
};

static int testpartten_remix(struct  aml_channel_map *map)
{
	int16_t rdata_16[6*8] = {0};
	int i;

	if (map == NULL)
		return -1;

	printf("\n==16==\n\n");
	for (i=0; i<6*8; i++) {
		rdata_16[i] = remix16[i];
	}
	_dump_data((void *)rdata_16, 6, e16BitPerSample, 8);
	data_remix_to_lr_channel(map, (void *)rdata_16, 8, e16BitPerSample, 6, AML_CH_IDX_C);
	printf("\n->after remix c to lr:\n");
	_dump_data((void *)rdata_16, 6, e16BitPerSample, 8);

	printf("\n==16==\n\n");
	for (i=0; i<6*8; i++) {
		rdata_16[i] = remix16[i];
	}
	_dump_data((void *)rdata_16, 6, e16BitPerSample, 8);
	data_remix_to_lr_channel(map, (void *)rdata_16, 8, e16BitPerSample, 6, AML_CH_IDX_5_1_ALL);
	printf("\n->after remix all to lr:\n");
	_dump_data((void *)rdata_16, 6, e16BitPerSample, 8);

	return 0;
}
/**
 **        ditter
 */
static int testpartten_ditter(struct aml_channel_map *map)
{
	if (map == NULL)
		return -1;

	printf("\n==16==\n\n");
	_dump_data((void *)data16, 8, e16BitPerSample, 8);
	data_add_ditter_to_channels((void *)data16, 8, e16BitPerSample, 8, AML_CH_IDX_5_1_2_ALL);
	printf("\n->after ditter:\n");
	_dump_data((void *)data16, 8, e16BitPerSample, 8);

	return 0;
}

/******************************************
 *  MULTI  BUFFER
 *****************************************/
/**
 **        concat
 */
#define T_FRAMES     (8)
#define T_CHNUM_ALL  (8)
#define T_CHNUM_SUB1 (2)
#define T_CHNUM_SUB2 (2)
#define T_CHNUM_SUB3 (2)
#define T_CHNUM_SUB4 (2)
int32_t d_all_32[T_FRAMES*T_CHNUM_ALL] = {
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

int32_t sub1_32[T_FRAMES*T_CHNUM_SUB1] = {
	0x11111101, 0x11111102,
	0x11111121, 0x11111122,
	0x11111131, 0x11111132,
	0x11111141, 0x11111142,
	0x11111151, 0x11111152,
	0x11111161, 0x11111162,
	0x11111171, 0x11111172,
	0x11111111, 0x11111112,
};
int32_t sub2_32[T_FRAMES*T_CHNUM_SUB2] = {
	0x11111103, 0x11111104,
	0x11111123, 0x11111124,
	0x11111133, 0x11111134,
	0x11111143, 0x11111144,
	0x11111153, 0x11111154,
	0x11111163, 0x11111164,
	0x11111173, 0x11111174,
	0x11111113, 0x11111114,
};
int32_t sub3_32[T_FRAMES*T_CHNUM_SUB3] = {
	0x11111105, 0x11111106,
	0x11111125, 0x11111126,
	0x11111135, 0x11111136,
	0x11111145, 0x11111146,
	0x11111155, 0x11111156,
	0x11111165, 0x11111166,
	0x11111175, 0x11111176,
	0x11111115, 0x11111116,
};
int32_t sub4_32[T_FRAMES*T_CHNUM_SUB4] = {
	0x11111107, 0x11111108,
	0x11111127, 0x11111128,
	0x11111137, 0x11111138,
	0x11111147, 0x11111148,
	0x11111157, 0x11111158,
	0x11111167, 0x11111168,
	0x11111177, 0x11111178,
	0x11111117, 0x11111118,
};

static int testpartten_concat(struct aml_channel_map *map)
{
	if (map == NULL)
		return -1;

	printf("\n==16==\n\n");
	_dump_data((void *)d_all_32, 8, e32BitPerSample, 8);
	_dump_data((void *)sub1_32,  2, e32BitPerSample, 8);
	_dump_data((void *)sub2_32,  2, e32BitPerSample, 8);
	_dump_data((void *)sub3_32,  2, e32BitPerSample, 8);
	_dump_data((void *)sub4_32,  2, e32BitPerSample, 8);
	//data_add_ditter_to_channels((void *)data16, 8, e16BitPerSample, 8, AML_CH_IDX_5_1_2_ALL);
	data_concat_channels(
		d_all_32, T_CHNUM_ALL, e32BitPerSample,
		sub1_32, sub2_32, sub3_32, sub4_32,
		(T_CHNUM_SUB1+T_CHNUM_SUB2+T_CHNUM_SUB3+T_CHNUM_SUB4),
		e32BitPerSample,
		T_FRAMES
		);
	printf("\n->after cancat:\n");
	_dump_data((void *)d_all_32, 8, e32BitPerSample, 8);

	return 0;
}

/******************************************
 *  TWO    BUFFER
 *****************************************/
/**
 **        replace
 */
#define T_RP_FRAMES     (8)
#define T_RP_CHNUM_ALL  (6)
#define T_RP_LFE        (2)
int16_t rep_d_16[T_RP_CHNUM_ALL*T_RP_FRAMES] = {
	0x1101, 0x1102, 0x1103, 0x1104, 0x1105, 0x1106,
	0x1111, 0x1112, 0x1113, 0x1114, 0x1115, 0x1116,
	0x1121, 0x1122, 0x1123, 0x1124, 0x1125, 0x1126,
	0x1131, 0x1132, 0x1133, 0x1134, 0x1135, 0x1136,
	0x1141, 0x1142, 0x1143, 0x1144, 0x1145, 0x1146,
	0x1151, 0x1152, 0x1153, 0x1154, 0x1155, 0x1156,
	0x1161, 0x1162, 0x1163, 0x1164, 0x1165, 0x1166,
	0x1171, 0x1172, 0x1173, 0x1174, 0x1175, 0x1176,
};

int16_t lfe_d_16[T_RP_LFE*T_RP_FRAMES] = {
	0x0000, 0x0000,
	0x0222, 0x0222,
	0x0444, 0x0444,
	0x0666, 0x0666,
	0x0888, 0x0888,
	0x0aaa, 0x0aaa,
	0x0ccc, 0x0bbb,
	0x0eee, 0x0ccc,
};

static int testpartten_replace(struct  aml_channel_map *map)
{
	if (map == NULL)
		return -1;

	printf("\n==16==\n\n");
	_dump_data((void *)rep_d_16, T_RP_CHNUM_ALL, e16BitPerSample, T_RP_FRAMES);
	printf("\n");
	_dump_data((void *)lfe_d_16, T_RP_LFE,       e16BitPerSample, T_RP_FRAMES);

	data_replace_lfe_data(
		(void *)rep_d_16, T_RP_CHNUM_ALL, e16BitPerSample,
		(void *)lfe_d_16, T_RP_LFE,       e16BitPerSample,
		(T_RP_FRAMES),
		data_get_channel_bit_mask(map, AML_CH_IDX_LFE)
		);

	printf("\n->after cancat:\n");
	_dump_data((void *)rep_d_16, T_RP_CHNUM_ALL, e16BitPerSample, T_RP_FRAMES);

	return 0;
}

/**
 **        extend
 */
int16_t ex_d_16[8*8] = {
	0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001,
};

int16_t ex_in_d_16[8*2] = {
	0x1101, 0x1102,
	0x1111, 0x1112,
	0x1121, 0x1122,
	0x1131, 0x1132,
	0x1141, 0x1142,
	0x1151, 0x1152,
	0x1161, 0x1162,
	0x1171, 0x1172,
};

static int testpartten_extend(struct  aml_channel_map *map)
{
	if (map == NULL)
		return -1;

	printf("\n==16==\n\n");
	_dump_data((void *)ex_d_16, 8/* channels */, e16BitPerSample, 8/*frames*/);
	printf("\n");
	_dump_data((void *)ex_in_d_16, 2/* channels */, e16BitPerSample, 8/*frames*/);
	printf("\n");

	data_extend_channels(
		(void *)ex_d_16,    8, e16BitPerSample,
		(void *)ex_in_d_16, 2, e16BitPerSample,
		8/*frames*/
		);

	printf("\n->after extend:\n");
	_dump_data((void *)ex_d_16, 8/* channels */, e16BitPerSample, 8/*frames*/);

	return 0;
}

/**
 **        extract
 */
int16_t ext_d_16[8*6] = {
	0x1101, 0x1102, 0x1103, 0x1104, 0x1105, 0x1106,
	0x1111, 0x1112, 0x1113, 0x1114, 0x1115, 0x1116,
	0x1121, 0x1122, 0x1123, 0x1124, 0x1125, 0x1126,
	0x1131, 0x1132, 0x1133, 0x1134, 0x1135, 0x1136,
	0x1141, 0x1142, 0x1143, 0x1144, 0x1145, 0x1146,
	0x1151, 0x1152, 0x1153, 0x1154, 0x1155, 0x1156,
	0x1161, 0x1162, 0x1163, 0x1164, 0x1165, 0x1166,
	0x1171, 0x1172, 0x1173, 0x1174, 0x1175, 0x1176,
};

int16_t ext_16[8*2] = {
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
};

static int testpartten_extract(struct  aml_channel_map *map)
{
	if (map == NULL)
		return -1;

	printf("\n==16==\n\n");
	_dump_data((void *)ext_d_16, 6/* channels */, e16BitPerSample, 8/*frames*/);
	printf("\n");
	_dump_data((void *)ext_16,   2/* channels */, e16BitPerSample, 8/*frames*/);
	printf("\n");

	data_extract_channels(
		map,
		(void *)ext_16,   2/*channels*/, e16BitPerSample,
		(void *)ext_d_16, 6/*channels*/, e16BitPerSample,
		8/* frames */,
		data_get_channel_bit_mask(map, AML_CH_IDX_LFE)
		);

	printf("\n->after extract:\n");

	_dump_data((void *)ext_16,   2/* channels */, e16BitPerSample, 8/*frames*/);
	printf("\n");

	return 0;
}


int main(void)
{
	struct  aml_channel_map *map = NULL;

	map = atmos_maps;
	//map = mi_maps;

#if 0
	// case1: channel map, PASS
#if 0
	testpartten_chmap("Missionimpossible", mi_maps);
	testpartten_chmap("Pulpfiction", pulpfiction_maps);
	testpartten_chmap("Matrix", matrix_maps);
	testpartten_chmap("Rainman", rainman_maps);
#else
	testpartten_chmap("Atmos", atmos_maps);
#endif
#endif

#if 0
	// case2: empty, PASS
	testpartten_empty(map);
	// case3: invert, PASS
	testpartten_invert(map);
	// case4: exchange, PASS
	testpartten_exchange(map);
	// case5: remix,                 !!!Support 16bit only
	testpartten_remix(map);
#endif
	// case6: ditter,                !!!Support 16bit/8ch only
	testpartten_ditter(map);

#if 0
	// case7: concat,                !!!only support same frame sz
	testpartten_concat(map);
#endif

#if 0
	// case8: replace lfe,           !!!only support all_6ch&lfe_2ch.
	testpartten_replace(map);
	// case9: extend,                !!!16bit only now
	testpartten_extend(map);
	// case10: extract,
	map = mi_maps;
	testpartten_extract(map);
#endif

	return 0;
}
