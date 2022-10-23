/**
 ** aml_data_utils.c
 **
 ** This program is the factory of PCM data. Such as,
 **        re-mix
 **        extend
 **        extract
 **        exchange
 **        invert
 **        concat
 **        empty
 **        ditter
 **        replace
 ** author: shen pengru
 */
#define LOG_TAG "audio_data_utils"
//#define LOG_NDEBUG 0

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "log.h"
#include <fcntl.h>
#include <math.h>
#include <aml_data_utils.h>
#include <aml_audio_hal_conf.h>
#include <aml_conf_parser.h>
#include <aml_conf_loader.h>

#if defined (BUILDHOSTEXE)
//#define DEBUG
#ifdef DEBUG
#define AMLOGE printf
#define AMLOGD printf
#else
#define AMLOGE
#define AMLOGD
#endif
#else
#define AMLOGE ALOGE
#define AMLOGD ALOGD
#endif

static struct aml_audio_channel_name gAudioChName[] = {
	{AML_CH_IDX_L,   "left"},
	{AML_CH_IDX_R,   "right"},
	{AML_CH_IDX_C,   "center"},
	{AML_CH_IDX_LFE, "lfe"},
	{AML_CH_IDX_LS,  "left_surround"},
	{AML_CH_IDX_RS,  "right_urround"},
	{AML_CH_IDX_LT,  "left_top"},
	{AML_CH_IDX_RT,  "right_top"},
};

static int _name_trans_to_i2s_chidx(const char *name)
{
	if (!strcmp(name, gAudioChName[0].ch_name)) {
		return gAudioChName[0].ch_idx;
	} else if (!strcmp(name, gAudioChName[1].ch_name)) {
		return gAudioChName[1].ch_idx;
	} else if (!strcmp(name, gAudioChName[2].ch_name)) {
		return gAudioChName[2].ch_idx;
	} else if (!strcmp(name, gAudioChName[3].ch_name)) {
		return gAudioChName[3].ch_idx;
	} else if (!strcmp(name, gAudioChName[4].ch_name)) {
		return gAudioChName[4].ch_idx;
	} else if (!strcmp(name, gAudioChName[5].ch_name)) {
		return gAudioChName[5].ch_idx;
	} else if (!strcmp(name, gAudioChName[6].ch_name)) {
		return gAudioChName[6].ch_idx;
	} else if (!strcmp(name, gAudioChName[7].ch_name)) {
		return gAudioChName[7].ch_idx;
	}
	return AML_CH_IDX_NULL;
}

static inline int16_t _clamp16(int32_t sample)
{
    if ((sample>>15) ^ (sample>>31))
        sample = 0x7FFF ^ (sample>>31);
    return sample;
}

static inline int32_t _clamp32(int64_t sample)
{
    //TODO:
    return sample;
}

static char *_get_ch_name(eChannelContentIdx idx)
{
	int i = 0;
	int cnt = 0;

	cnt = sizeof(gAudioChName)/sizeof(struct aml_audio_channel_name);

	for (i=AML_CH_IDX_L; i<AML_I2S_CHANNEL_COUNT; i++) {
		if (idx == gAudioChName[i].ch_idx) {
			return gAudioChName[i].ch_name;
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

#define SHENPENGRU_TEST (0)

#if SHENPENGRU_TEST
struct aml_channel_map test_maps[] = {
	{AML_CH_IDX_L,   AML_I2S_PORT_IDX_01,   AML_I2S_CHANNEL_0},
	{AML_CH_IDX_R,   AML_I2S_PORT_IDX_01,   AML_I2S_CHANNEL_1},
	{AML_CH_IDX_C,   AML_I2S_PORT_IDX_23,   AML_I2S_CHANNEL_2},
	{AML_CH_IDX_LFE, AML_I2S_PORT_IDX_23,   AML_I2S_CHANNEL_3},
	{AML_CH_IDX_LS,  AML_I2S_PORT_IDX_67,   AML_I2S_CHANNEL_6},
	{AML_CH_IDX_RS,  AML_I2S_PORT_IDX_67,   AML_I2S_CHANNEL_7},
	{AML_CH_IDX_LT,  AML_I2S_PORT_IDX_45,   AML_I2S_CHANNEL_4},
	{AML_CH_IDX_RT,  AML_I2S_PORT_IDX_45,   AML_I2S_CHANNEL_5},
};
#endif

static char *_get_ch_conf_name(int type, int content_idx)
{
	switch (type) {
	case eAmlConfTypeChMap:
		switch (content_idx) {
		case AML_CH_IDX_L:
			return AML_I2S_CHANNEL0;
		case AML_CH_IDX_R:
			return AML_I2S_CHANNEL1;
		case AML_CH_IDX_C:
			return AML_I2S_CHANNEL2;
		case AML_CH_IDX_LFE:
			return AML_I2S_CHANNEL3;
		case AML_CH_IDX_LS:
			return AML_I2S_CHANNEL4;
		case AML_CH_IDX_RS:
			return AML_I2S_CHANNEL5;
		case AML_CH_IDX_LT:
			return AML_I2S_CHANNEL6;
		case AML_CH_IDX_RT:
			return AML_I2S_CHANNEL7;
		default:
			break;
		}
		break;
	case eAmlConfTypeChInv:
		switch (content_idx) {
		case AML_CH_IDX_L:
			return AML_I2S_INVERT_CH0;
		case AML_CH_IDX_R:
			return AML_I2S_INVERT_CH1;
		case AML_CH_IDX_C:
			return AML_I2S_INVERT_CH2;
		case AML_CH_IDX_LFE:
			return AML_I2S_INVERT_CH3;
		case AML_CH_IDX_LS:
			return AML_I2S_INVERT_CH4;
		case AML_CH_IDX_RS:
			return AML_I2S_INVERT_CH5;
		case AML_CH_IDX_LT:
			return AML_I2S_INVERT_CH6;
		case AML_CH_IDX_RT:
			return AML_I2S_INVERT_CH7;
		default:
			break;
		}
		break;
	case eAmlConfTypeChDit:
		switch (content_idx) {
		case AML_CH_IDX_L:
			return AML_DITTER_I2S_CH0;
		case AML_CH_IDX_R:
			return AML_DITTER_I2S_CH1;
		case AML_CH_IDX_C:
			return AML_DITTER_I2S_CH2;
		case AML_CH_IDX_LFE:
			return AML_DITTER_I2S_CH3;
		case AML_CH_IDX_LS:
			return AML_DITTER_I2S_CH4;
		case AML_CH_IDX_RS:
			return AML_DITTER_I2S_CH5;
		case AML_CH_IDX_LT:
			return AML_DITTER_I2S_CH6;
		case AML_CH_IDX_RT:
			return AML_DITTER_I2S_CH7;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return NULL;
}

static int _save_conf_to_maps(struct aml_channel_map *maps,
			int content_idx, int ch_num, int invert, int ditter)
{
	int i = 0;

	for (i=AML_CH_IDX_L; i<AML_CH_IDX_MAX; i++) {
		if (content_idx == maps[i].channel_idx) {
			maps[i].i2s_idx   = ch_num/2;
			maps[i].bit_mask |= AML_I2S_CHANNEL_0<<ch_num;
			maps[i].invert   |= invert<<ch_num;
			maps[i].ditter   |= ditter<<ch_num;
		}
	}

	return 0;
}

#if !defined (BUILDHOSTEXE)
/* WARNNING: initial function, only need called once!! */
struct aml_channel_map *data_load_product_config(void)
{
	struct parser *gParser = NULL;
	char   chname[50];
	struct aml_channel_map *maps = NULL;
	int i = 0;
	int find_idx, invert, ditter;

	maps = malloc(sizeof(struct aml_channel_map)*AML_I2S_CHANNEL_COUNT);
	if (!maps) {
		return NULL;
	} else {
		maps[AML_CH_IDX_L].channel_idx   = AML_CH_IDX_L;
		maps[AML_CH_IDX_R].channel_idx   = AML_CH_IDX_R;
		maps[AML_CH_IDX_C].channel_idx   = AML_CH_IDX_C;
		maps[AML_CH_IDX_LFE].channel_idx = AML_CH_IDX_LFE;
		maps[AML_CH_IDX_LS].channel_idx  = AML_CH_IDX_LS;
		maps[AML_CH_IDX_RS].channel_idx  = AML_CH_IDX_RS;
		maps[AML_CH_IDX_LT].channel_idx  = AML_CH_IDX_LT;
		maps[AML_CH_IDX_RT].channel_idx  = AML_CH_IDX_RT;
	}

	gParser = aml_config_load(AML_PARAM_AUDIO_HAL_PARAM);
	if (gParser != NULL) {
		// loop of i2s channel [0, 8]
		for (i=0; i<AML_CH_IDX_MAX; i++) {
			strcpy(chname, aml_config_get_str(gParser, AML_SECTION_AUDIO_HAL,
						_get_ch_conf_name(eAmlConfTypeChMap, i), NULL));
			invert   = aml_config_get_int(gParser, AML_SECTION_AUDIO_HAL,
						_get_ch_conf_name(eAmlConfTypeChInv, i), 0);
			ditter   = aml_config_get_int(gParser, AML_SECTION_AUDIO_HAL,
						_get_ch_conf_name(eAmlConfTypeChDit, i), 0);
			find_idx = _name_trans_to_i2s_chidx(chname);
			_save_conf_to_maps(maps, find_idx, i, invert, ditter);
		}
		// unload
		aml_config_unload(gParser);
	}

	return maps;
}
#endif

int data_get_channel_i2s_port(
	struct aml_channel_map *map, eChannelContentIdx channelName)
{
	int i = 0;

	if (map == NULL) {
		return AML_I2S_PORT_IDX_NULL;
	}

	for (i=AML_CH_IDX_L; i<AML_CH_IDX_MAX; i++) {
		if (map[i].channel_idx == channelName) {
			AMLOGD("%s: %s <-> i2s-port: %d\n", __func__,
				_get_ch_name(channelName),
				map[i].i2s_idx);
			return map[i].i2s_idx;
		}
	}

	return AML_I2S_PORT_IDX_NULL;
}

int data_get_channel_bit_mask(
	struct aml_channel_map *map, eChannelContentIdx channelName)
{
	int i = 0;
	int bit_mask = AML_I2S_CHANNEL_NULL;

	if (map == NULL) {
		return bit_mask;
	}

	for (i=AML_CH_IDX_L; i<AML_CH_IDX_MAX; i++) {
		if (map[i].channel_idx == channelName) {
			bit_mask = map[i].bit_mask;
			AMLOGD("%s: %s <-> i2s-bit-mask: 0x%08x\n", __func__,
				_get_ch_name(channelName),
				bit_mask);
			return bit_mask;
		}
	}

	switch (channelName) {
	case AML_CH_IDX_5_1_ALL:
	case AML_CH_IDX_7_1_ALL:
	case AML_CH_IDX_5_1_2_ALL:
		for (i=0; i<AML_CH_IDX_MAX; i++) {
			bit_mask |= map[i].bit_mask;
		}
		AMLOGD("%s: %s <-> i2s-bit-mask: 0x%08x\n", __func__,
			_get_ch_name(channelName),
			bit_mask);
		return bit_mask;
	default:
		break;
	}

	return bit_mask;
}

eChannelContentIdx data_get_channel_content_idx(
	struct aml_channel_map *map, int bitmask)
{
	int idx = AML_CH_IDX_NULL;
	int i = 0;
	int bit_mask = AML_I2S_CHANNEL_NULL;

	if (map == NULL) {
		return idx;
	}

	for (i=AML_CH_IDX_L; i<AML_CH_IDX_MAX; i++) {
		if (map[i].bit_mask & bitmask) {
			idx = map[i].channel_idx;
			AMLOGD("%s: i2s-bit-mask: 0x%08x <-> %s\n", __func__,
				bitmask,
				_get_ch_name(idx));
			break;
		}
	}

	return idx;
}

static int _data_remix_center_to_lr(void *buf, size_t frames, size_t framesz, int channels, int bitmask)
{
	int16_t center16;
	int32_t center32;
	int16_t *buf16  = (int16_t *)buf;
	int32_t *buf32  = (int32_t *)buf;
	int     i, tmp;

	///< TODO: should use bitmask to find the center channel
	if (bitmask == AML_I2S_CHANNEL_NULL)
		return 0;

	///< TODO: buf is 6ch pcm(L,R,C,lfe,Lr,Rs)
	if (channels != 6) {
		AMLOGD("%s: only support 6 ch now!\n", __func__);
		return -1;
	}

	///< TODO:
	if (framesz != e16BitPerSample) {
		AMLOGD("%s: only support 16bit now!\n", __func__);
		return -1;
	}

	switch (framesz) {
	case e16BitPerSample:
		///< 3/0 input L_out/R_out =  = 0.707*(L/R + 0.707*C);
		for (i=0; i<(int)frames; i++) {
			/* save data of center */
			center16 = buf16[channels*i + 2];
			/* calculate L */
			tmp                   = buf16[channels*i + 0] << 12;
			buf16[channels*i + 0] = _clamp16((MINUS_3_DB_IN_Q19_12 * ((tmp + MINUS_3_DB_IN_Q19_12*center16) >>12))>>12);
			/* calculate R */
			tmp                   = buf16[channels*i + 1] << 12;
			buf16[channels*i + 1] = _clamp16((MINUS_3_DB_IN_Q19_12 * ((tmp + MINUS_3_DB_IN_Q19_12*center16) >>12))>>12);
		}
		break;
	case e32BitPerSample:
		//TODO:
		break;
	default:
		break;
	}

	return 0;
}

static int _data_remix_all_to_lr(void *buf,	size_t frames, size_t framesz, int channels, int bitmask)
{
    /*
     * --------------------------------------
     * 3/2 input module:
     * L_out = 0.707*(L + 0.707*C + 0.707*Ls)
     * R_out = 0.707*(R + 0.707*C + 0.707*Rs)
     * --------------------------------------
     * our channel sequeces:
     * 0->L
     * 1->R
     * 2->C
     * 3->lfe
     * 4->Ls
     * 5->Rs
     * --------------------------------------
     */
	int16_t l_16, r_16, c_16, ls_16, rs_16;
	int16_t l_32, r_32, c_32, ls_32, rs_32;
	int16_t *buf16  = (int16_t *)buf;
	int32_t *buf32  = (int32_t *)buf;
	int     i, j, tmp;

	if (!bitmask)
		return 0;

	///< buf is 6ch pcm(L,R,C,lfe,Lr,Rs)
	if (channels != 6) {
		AMLOGD("%s: only support 6 ch now!\n", __func__);
		return -1;
	}

	///< TODO:
	if (framesz != e16BitPerSample) {
		AMLOGD("%s: only support 16bit now!\n", __func__);
		return -1;
	}

	switch (framesz) {
	case e16BitPerSample:
		for (i = 0; i < (int)frames; i++) {
			/* save l/r/c/ls/rs */
			l_16  = buf16[channels*i + 0];
			r_16  = buf16[channels*i + 1];
			c_16  = buf16[channels*i + 2];
			ls_16 = buf16[channels*i + 4];
			rs_16 = buf16[channels*i + 5];
			/* handle L channel */
			tmp = l_16 << 12;
			buf16[channels*i] =
				_clamp16((MINUS_3_DB_IN_Q19_12 * ((tmp + MINUS_3_DB_IN_Q19_12 * c_16 + MINUS_3_DB_IN_Q19_12 * ls_16) >> 12)) >> 12);
			/* handle R channel */
			tmp = r_16 << 12;
			buf16[channels*i + 1] =
				_clamp16((MINUS_3_DB_IN_Q19_12 * ((tmp + MINUS_3_DB_IN_Q19_12 * c_16 + MINUS_3_DB_IN_Q19_12 * rs_16) >> 12)) >> 12);
		}
		break;
	case e32BitPerSample:
		//TODO:
		break;
	default:
		break;
	}

	return 0;
}

int data_remix_to_lr_channel(
	struct  aml_channel_map *map,
	void    *buf,
	size_t  frames,
	size_t  framesz,
	int     channels,
	eChannelContentIdx chIdx)
{
    int bit_mask = data_get_channel_bit_mask(map, chIdx);

	switch (chIdx) {
	case AML_CH_IDX_C:
		return _data_remix_center_to_lr(buf, frames, framesz, channels, bit_mask);
	case AML_CH_IDX_5_1_ALL:
	case AML_CH_IDX_7_1_ALL:
	case AML_CH_IDX_5_1_2_ALL:
		return _data_remix_all_to_lr(buf, frames, framesz, channels, bit_mask);
	default:
		break;
	}

	return 0;
}

int data_empty_channels(
	struct  aml_channel_map *map,
	void    *buf,
	size_t  frames,
	size_t  framesz,
	int     channels,
	int     channel_empty_bit_mask)
{
	int i, j;
	int16_t *buf16 = (int16_t *)buf;
	int32_t *buf32 = (int32_t *)buf;

	if (!map)
		return 0;

	switch (framesz) {
	case e16BitPerSample:
		for (i=0; i<(int)frames; i++) {
			for (j=0; j<channels; j++) {
				if (channel_empty_bit_mask & (AML_I2S_CHANNEL_0<<j)) {
					buf16[channels*i + j] = 0x00;
				}
			}
		}
		break;
	case e32BitPerSample:
		for (i=0; i<(int)frames; i++) {
			for (j=0; j<channels; j++) {
				if (channel_empty_bit_mask & (AML_I2S_CHANNEL_0<<j)) {
					buf32[channels*i + j] = 0x00;
				}
			}
		}
		break;
	default:
		break;
	}

	return 0;
}

int data_exchange_i2s_channels(
	void    *buf,
	size_t  frames,
	size_t  framesz,
	size_t  channels,
	eI2SDataLineIdx     i2s_idx1,
	eI2SDataLineIdx     i2s_idx2)
{
	int i, j;
	int16_t *buf16 = (int16_t *)buf;
	int32_t *buf32 = (int32_t *)buf;
	int16_t tmp16[AML_CH_CNT_PER_PORT];
	int32_t tmp32[AML_CH_CNT_PER_PORT];

	if (channels < 2*AML_CH_CNT_PER_PORT) {
		AMLOGE("%s: at least 2 i2s port is needed!\n", __func__);
		return -1;
	}

	//printf("\t%d <-> %d\n", i2s_idx1, i2s_idx2);

	switch (framesz) {
	case e16BitPerSample:
		for (i=0; i<(int)frames; i++) {
			for (j=0; j<AML_CH_CNT_PER_PORT; j++) {
				tmp16[j]                                             = buf16[i*channels + i2s_idx1*AML_CH_CNT_PER_PORT + j];
				buf16[i*channels + i2s_idx1*AML_CH_CNT_PER_PORT + j] = buf16[i*channels + i2s_idx2*AML_CH_CNT_PER_PORT + j];
				buf16[i*channels + i2s_idx2*AML_CH_CNT_PER_PORT + j] = tmp16[j];

			}
		#if 0
			printf("0x%08x 0x%08x <-> 0x%08x 0x%08x\n\n",
				buf16[i*channels + i2s_idx1*AML_CH_CNT_PER_PORT + 0],
				buf16[i*channels + i2s_idx1*AML_CH_CNT_PER_PORT + 1],
				buf16[i*channels + i2s_idx2*AML_CH_CNT_PER_PORT + 0],
				buf16[i*channels + i2s_idx2*AML_CH_CNT_PER_PORT + 1]);
		#endif
		}
		break;
	case e32BitPerSample:
		for (i=0; i<(int)frames; i++) {
			for (j=0; j<AML_CH_CNT_PER_PORT; j++) {
				tmp32[j]                                             = buf32[i*channels + i2s_idx1*AML_CH_CNT_PER_PORT + j];
				buf32[i*channels + i2s_idx1*AML_CH_CNT_PER_PORT + j] = buf32[i*channels + i2s_idx2*AML_CH_CNT_PER_PORT + j];
				buf32[i*channels + i2s_idx2*AML_CH_CNT_PER_PORT + j] = tmp32[j];
			}
		}
		break;
	default:
		break;
	}

	return 0;
}

int data_replace_lfe_data(
	void    *out_buf,
	size_t  out_channels,
	size_t  out_framesz,
	void    *input_lfe_buffer,
	size_t  in_channles,
	size_t  in_framesz,
	size_t  frames,
	int     channel_insert_bit_mask)
{
	int i, j;
	int16_t *buf_out16 = (int16_t *)out_buf;
	int32_t *buf_out32 = (int32_t *)out_buf;
	int16_t *buf_in16  = (int16_t *)input_lfe_buffer;
	int32_t *buf_in32  = (int32_t *)input_lfe_buffer;
	int     lfe_base;
	int     lfe_cnt;

	//TODO:
	if (out_channels != 6) {
		AMLOGE("%s: only support 5.1 channels\n", __func__);
		return -1;
	}

	//TODO:
	if (in_channles != 2) {
		AMLOGE("%s: only support replace 2 channels\n", __func__);
		return -1;
	}

	//TODO: should get from channel_insert_bit_mask
	//case_1: ch2,ch3 are all lfe
	if ((AML_I2S_CHANNEL_2|AML_I2S_CHANNEL_3) == channel_insert_bit_mask) {
		lfe_base = 2;
		lfe_cnt  = 2;
	}
	//case_1:     ch3  is lfe
	if (AML_I2S_CHANNEL_3 == channel_insert_bit_mask) {
		lfe_base = 3;
		lfe_cnt  = 1;
	}

	switch (out_framesz) {
	case e16BitPerSample:
		switch (in_framesz) {
		case e16BitPerSample:
			for (i=0; i<(int)frames; i++) {
				for (j=0; j<lfe_cnt; j++) {
					buf_out16[out_channels*i + lfe_base + j] = buf_in16[in_channles*i + j];
				}
			}
			break;
		case e32BitPerSample:
			for (i=0; i<(int)frames; i++) {
				for (j=0; j<lfe_cnt; j++) {
					buf_out16[out_channels*i + lfe_base + j] = (int16_t)(buf_in16[in_channles*i + j] >> 16);
				}
			}
			break;
		default:
			break;
		}
		break;
	case e32BitPerSample:
		switch (in_framesz) {
		case e16BitPerSample:
			for (i=0; i<(int)frames; i++) {
				for (j=0; j<lfe_cnt; j++) {
					buf_out32[out_channels*i + lfe_base + j] = ((int32_t)buf_in16[in_channles*i + j]) << 16;
				}
			}
			break;
		case e32BitPerSample:
			for (i=0; i<(int)frames; i++) {
				for (j=0; j<lfe_cnt; j++) {
					buf_out32[out_channels*i + lfe_base + j] = buf_in32[in_channles*i + j];
				}
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return 0;
}

int data_invert_channels(
	void    *buf,
	size_t  frames,
	size_t  framesz,
	int     channels,
	int     channel_invert_bit_mask)
{
	int i, j;
	int16_t *buf16 = (int16_t *)buf;
	int32_t *buf32 = (int32_t *)buf;
	int16_t tmp16;
	int32_t tmp32;

	//_find_index_need_effect(channel_invert_bit_mask);
	switch (framesz) {
	case e16BitPerSample:
		for (i=0; i<(int)frames; i++) {
			for (j=0; j<channels; j++) {
				if (channel_invert_bit_mask & (AML_I2S_CHANNEL_0<<j)) {
					tmp16                 = buf16[channels*i + j];
					buf16[channels*i + j] = -tmp16;
				}
			}
		}
		break;
	case e32BitPerSample:
		for (i=0; i<(int)frames; i++) {
			for (j=0; j<channels; j++) {
				if (channel_invert_bit_mask & (AML_I2S_CHANNEL_0<<j)) {
					tmp32                 = buf32[channels*i + j];
					buf32[channels*i + j] = -tmp32;
				}
			}
		}
		break;
	default:
		break;
	}

	return 0;
}

int data_concat_channels(
	void    *out_buf,
	size_t  out_channels,
	size_t  out_framesz,
	void    *in_buf1, void *in_buf2, void *in_buf3, void *in_buf4,
	size_t  in_channels,
	size_t  in_framesz,
	size_t  frames)
{
	int i, j;
	int16_t *buf_out16  = (int16_t *)out_buf;
	int32_t *buf_out32  = (int32_t *)out_buf;

	int16_t *buf_in1_16 = (int16_t *)in_buf1;
	int16_t *buf_in2_16 = (int16_t *)in_buf2;
	int16_t *buf_in3_16 = (int16_t *)in_buf3;
	int16_t *buf_in4_16 = (int16_t *)in_buf4;

	int32_t *buf_in1_32 = (int32_t *)in_buf1;
	int32_t *buf_in2_32 = (int32_t *)in_buf2;
	int32_t *buf_in3_32 = (int32_t *)in_buf3;
	int32_t *buf_in4_32 = (int32_t *)in_buf4;

	if (in_channels != 2 && in_channels != 4 && in_channels != 6 && in_channels != 8) {
		AMLOGE("%s: only support concat 2/4/6/8 channels together!\n", __func__);
		return -EINVAL;
	}

	if (in_channels > out_channels) {
		AMLOGE("%s: out_channels %zu < %zu inclannels\n", __func__, out_channels, in_channels);
		return -EINVAL;
	}

	switch (out_framesz) {
	case e16BitPerSample:
		switch (in_framesz) {
		case e16BitPerSample:
			for (i=0; i<(int)frames; i++) {
				if (in_channels >= 2) {
					buf_out16[out_channels*i + 0] = buf_in1_16[2*i + 0];
					buf_out16[out_channels*i + 1] = buf_in1_16[2*i + 1];
				}
				if (in_channels >= 4) {
					buf_out16[out_channels*i + 2] = buf_in2_16[2*i + 0];
					buf_out16[out_channels*i + 3] = buf_in2_16[2*i + 1];
				}
				if (in_channels >= 6) {
					buf_out16[out_channels*i + 4] = buf_in3_16[2*i + 0];
					buf_out16[out_channels*i + 5] = buf_in3_16[2*i + 1];
				}
				if (in_channels >= 8) {
					buf_out16[out_channels*i + 6] = buf_in4_16[2*i + 0];
					buf_out16[out_channels*i + 7] = buf_in4_16[2*i + 1];
				}
			}
			break;
		case e32BitPerSample:
			for (i=0; i<(int)frames; i++) {
				if (in_channels >= 2) {
					buf_out16[out_channels*i + 0] = (int16_t)(buf_in1_32[2*i + 0]>>16);
					buf_out16[out_channels*i + 1] = (int16_t)(buf_in1_32[2*i + 1]>>16);
				}
				if (in_channels >= 4) {
					buf_out16[out_channels*i + 2] = (int16_t)(buf_in2_32[2*i + 0]>>16);
					buf_out16[out_channels*i + 3] = (int16_t)(buf_in2_32[2*i + 1]>>16);
				}
				if (in_channels >= 6) {
					buf_out16[out_channels*i + 4] = (int16_t)(buf_in3_32[2*i + 0]>>16);
					buf_out16[out_channels*i + 5] = (int16_t)(buf_in3_32[2*i + 1]>>16);
				}
				if (in_channels >= 8) {
					buf_out16[out_channels*i + 6] = (int16_t)(buf_in4_32[2*i + 0]>>16);
					buf_out16[out_channels*i + 7] = (int16_t)(buf_in4_32[2*i + 1]>>16);
				}
			}
			break;
		default:
			break;
		}
		break;
	case e32BitPerSample:
		switch (in_framesz) {
		case e16BitPerSample:
			for (i=0; i<(int)frames; i++) {
				if (in_channels >= 2) {
					buf_out32[out_channels*i + 0] = ((int32_t)(buf_in1_16[2*i + 0]))<<16;
					buf_out32[out_channels*i + 1] = ((int32_t)(buf_in1_16[2*i + 1]))<<16;
				}
				if (in_channels >= 4) {
					buf_out32[out_channels*i + 2] = ((int32_t)(buf_in2_16[2*i + 0]))<<16;
					buf_out32[out_channels*i + 3] = ((int32_t)(buf_in2_16[2*i + 1]))<<16;
				}
				if (in_channels >= 6) {
					buf_out32[out_channels*i + 4] = ((int32_t)(buf_in3_16[2*i + 0]))<<16;
					buf_out32[out_channels*i + 5] = ((int32_t)(buf_in3_16[2*i + 1]))<<16;
				}
				if (in_channels >= 8) {
					buf_out32[out_channels*i + 6] = ((int32_t)(buf_in4_16[2*i + 0]))<<16;
					buf_out32[out_channels*i + 7] = ((int32_t)(buf_in4_16[2*i + 1]))<<16;
				}
			}
			break;
		case e32BitPerSample:
			for (i=0; i<(int)frames; i++) {
				if (in_channels >= 2) {
					buf_out32[out_channels*i + 0] = buf_in1_32[2*i + 0];
					buf_out32[out_channels*i + 1] = buf_in1_32[2*i + 1];
				}
				if (in_channels >= 4) {
					buf_out32[out_channels*i + 2] = buf_in2_32[2*i + 0];
					buf_out32[out_channels*i + 3] = buf_in2_32[2*i + 1];
				}
				if (in_channels >= 6) {
					buf_out32[out_channels*i + 4] = buf_in3_32[2*i + 0];
					buf_out32[out_channels*i + 5] = buf_in3_32[2*i + 1];
				}
				if (in_channels >= 8) {
					buf_out32[out_channels*i + 6] = buf_in4_32[2*i + 0];
					buf_out32[out_channels*i + 7] = buf_in4_32[2*i + 1];
				}
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return 0;
}

#define COUNT_DITTER_FRAMES   (4)
#define COUNT_DITTER_CHANNELS (8)
int16_t ditter_8ch_16[COUNT_DITTER_FRAMES*COUNT_DITTER_CHANNELS] = {
	0xffff, 0x0001, 0xffff, 0x0000, 0xffff, 0x0000, 0x0001, 0xffff,
	0x0001, 0xffff, 0x0001, 0xffff, 0x0001, 0xffff, 0xffff, 0x0001,
	0xffff, 0x0001, 0xffff, 0x0000, 0xffff, 0x0001, 0x0000, 0x0000,
	0x0001, 0xffff, 0x0001, 0x0001, 0x0000, 0x0001, 0xffff, 0xffff
};
int32_t ditter_8ch_32[COUNT_DITTER_FRAMES*COUNT_DITTER_CHANNELS] = {
	0xffffffff, 0x00000001, 0xffffffff, 0x00000000, 0xffffffff, 0x00000000, 0x00000001, 0xffffffff,
	0x00000001, 0xffffffff, 0x00000001, 0xffffffff, 0x00000001, 0xffffffff, 0xffffffff, 0x00000001,
	0xffffffff, 0x00000001, 0xffffffff, 0x00000000, 0xffffffff, 0x00000001, 0x00000000, 0x00000000,
	0x00000001, 0xffffffff, 0x00000001, 0x00000001, 0x00000000, 0x00000001, 0xffffffff, 0xffffffff
};

int data_add_ditter_to_channels(
	void    *buffer,
	size_t  frames,
	size_t  framesz,
	int     channels,
	int     channel_ditter_bit_mask)
{
	int i, j;
	int16_t *buf16  = (int16_t *)buffer;
	int32_t *buf32  = (int32_t *)buffer;

    int16_t *ditter16;
    int32_t *ditter32;

    int32_t tmpbuf32;
    int64_t tmpbuf64;

	if (channels > 8) {
		AMLOGE("%s: only support 5.1.2 ch(8channels) now\n", __func__);
		return -1;
	}

	//TODO: need remove this
	switch (channel_ditter_bit_mask) {
	case AML_CH_IDX_5_1_ALL:
		channel_ditter_bit_mask = AML_I2S_CHANNEL_0|AML_I2S_CHANNEL_1
			|AML_I2S_CHANNEL_2|AML_I2S_CHANNEL_3|AML_I2S_CHANNEL_4|AML_I2S_CHANNEL_5;
		break;
	case AML_CH_IDX_7_1_ALL:
	case AML_CH_IDX_5_1_2_ALL:
		channel_ditter_bit_mask = AML_I2S_CHANNEL_0|AML_I2S_CHANNEL_1
			|AML_I2S_CHANNEL_2|AML_I2S_CHANNEL_3|AML_I2S_CHANNEL_4|AML_I2S_CHANNEL_5
			|AML_I2S_CHANNEL_6|AML_I2S_CHANNEL_7;
		break;
	default:
		break;
	}

	switch (framesz) {
	case e16BitPerSample:
		for (i=0; i<(int)frames; i++) {
            ditter16 = &ditter_8ch_16[i%(COUNT_DITTER_FRAMES+1)];
			for (j=0; j<channels; j++) {
				if (channel_ditter_bit_mask & (AML_I2S_CHANNEL_0<<j)) {
					tmpbuf32 = (int32_t)buf16[channels*i + j] + (int32_t)ditter16[j];
					// [-2^15, 2^15-1]
					if (tmpbuf32 > 32767 || tmpbuf32 < -32768) {
						continue;
					} else {
						buf16[channels*i + j] = (int16_t)tmpbuf32;
					}
				}
			}
		}
		break;
	case e32BitPerSample:
		for (i=0; i<(int)frames; i++) {
            ditter32 = &ditter_8ch_32[i%(COUNT_DITTER_FRAMES+1)];
			for (j=0; j<channels; j++) {
				if (channel_ditter_bit_mask & (AML_I2S_CHANNEL_0<<j)) {
					tmpbuf64 = (int64_t)buf32[channels*i + j] + (int64_t)ditter32[j];
					// [-2^31, 2^31-1] // is it right here?
					if (tmpbuf64 > 2147483647 || tmpbuf64 < -2147483647) {
						continue;
					} else {
						buf32[channels*i + j] = (int32_t)tmpbuf64;
					}
				}
			}
		}
		break;
	default:
		break;
	}

	return 0;
}

int data_extend_channels(
	void    *out_buf,
	size_t  out_channels,
	size_t  out_framesz,
	void    *in_buf,
	size_t  in_channels,
	size_t  in_framesz,
	size_t  frames)
{
	int i, j;
	int16_t *buf_in16   = (int16_t *)in_buf;
	int32_t *buf_in32   = (int32_t *)in_buf;
	int16_t *buf_out16  = (int16_t *)out_buf;
	int32_t *buf_out32  = (int32_t *)out_buf;

	//TODO: use one interfcae for data_extend_channels/data_extract_channels
	if (out_channels < in_channels) {
		AMLOGE("%s: only support extend channels\n", __func__);
		return -1;
	}

	// initial out buffer first
	memset((char *)out_buf, 0x00, frames*out_channels*out_framesz);

	switch (out_framesz) {
	case e16BitPerSample:
		switch (in_framesz) {
		case e16BitPerSample:
			for (i=0; i<(int)frames; i++) {
				for (j=0; j<(int)out_channels; j++) {
					if (j < (int)in_channels) {
						buf_out16[out_channels*i + j] = buf_in16[in_channels*i + j];
					} else {
						buf_out16[out_channels*i + j] = 0x00;
					}
				}
			}
			break;
		case e32BitPerSample:
			for (i=0; i<(int)frames; i++) {
				for (j=0; j<(int)out_channels; j++) {
					if (j < (int)in_channels) {
						buf_out16[out_channels*i + j] = (int16_t)(buf_in32[in_channels*i + j]>>16);
					} else {
						buf_out16[out_channels*i + j] = 0x00;
					}
				}
			}
			break;
		default:
			break;
		}
		break;
	case e32BitPerSample:
		switch (in_framesz) {
		case e16BitPerSample:
			for (i=0; i<(int)frames; i++) {
				for (j=0; j<(int)out_channels; j++) {
					if (j < (int)in_channels) {
						buf_out32[out_channels*i + j] = ((int32_t)buf_in16[in_channels*i + j])<<16;
					} else {
						buf_out32[out_channels*i + j] = 0x00;
					}
				}
			}
			break;
		case e32BitPerSample:
			for (i=0; i<(int)frames; i++) {
				for (j=0; j<(int)out_channels; j++) {
					if (j < (int)in_channels) {
						buf_out32[out_channels*i + j] = buf_in32[in_channels*i + j];
					} else {
						buf_out32[out_channels*i + j] = 0x00;
					}
				}
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return 0;
}

int data_extract_channels(
	struct  aml_channel_map *map,
	void    *out_buf,
	size_t  out_channels,
	size_t  out_framesz,
	void    *in_buf,
	size_t  in_channels,
	size_t  in_framesz,
	size_t  frames,
	int     channel_extract_bit_mask)
{
	int i, j, k;
	int16_t *buf_in16   = (int16_t *)in_buf;
	int32_t *buf_in32   = (int32_t *)in_buf;
	int16_t *buf_out16  = (int16_t *)out_buf;
	int32_t *buf_out32  = (int32_t *)out_buf;
	int cnt;

	if (!map)
		return 0;

	//TODO: use one interfcae for data_extend_channels/data_extract_channels
	if (out_channels > in_channels) {
		AMLOGE("%s: only support extract channels\n", __func__);
		return -1;
	}

	//check param
	cnt = 0;
	for (i=0; i<8; i++)
		if (channel_extract_bit_mask & (AML_I2S_CHANNEL_0<<i))
			cnt++;
	if (cnt > (int)out_channels) {
		AMLOGE("%s: need extract %d channels, but buf only have %zu chanels\n",
			__func__, cnt, out_channels);
		return -1;
	}

	switch (out_framesz) {
	case e16BitPerSample:
		switch (in_framesz) {
		case e16BitPerSample:
			for (i=0; i<(int)frames; i++) {
				cnt = 0;
				for (k=0; k<(int)in_channels; k++) {
					if (channel_extract_bit_mask & (AML_I2S_CHANNEL_0<<k)) {
						buf_out16[out_channels*i + cnt] = buf_in16[in_channels*i + k];
						cnt++;
					}
				}
			}
			break;
		case e32BitPerSample:
			for (i=0; i<(int)frames; i++) {
				cnt = 0;
				for (k=0; k<(int)in_channels; k++) {
					if (channel_extract_bit_mask & (AML_I2S_CHANNEL_0<<k)) {
						buf_out16[out_channels*i + cnt] = (int16_t)(buf_in32[in_channels*i + k]>>16);
						cnt++;
					}
				}
			}
			break;
		default:
			break;
		}
		break;
	case e32BitPerSample:
		switch (in_framesz) {
		case e16BitPerSample:
			for (i=0; i<(int)frames; i++) {
				cnt = 0;
				for (k=0; k<(int)in_channels; k++) {
					if (channel_extract_bit_mask & (AML_I2S_CHANNEL_0<<k)) {
						buf_out32[out_channels*i + cnt] = ((int32_t)buf_in16[in_channels*i + k])<<16;
						cnt++;
					}
				}
			}
			break;
		case e32BitPerSample:
			for (i=0; i<(int)frames; i++) {
				cnt = 0;
				for (k=0; k<(int)in_channels; k++) {
					if (channel_extract_bit_mask & (AML_I2S_CHANNEL_0<<k)) {
						buf_out32[out_channels*i + cnt] = buf_in32[in_channels*i + k];
						cnt++;
					}
				}
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return 0;
}

static int delay[4096];
static int delay_frame = 1440 * 4;
static char *delay_start = (char *)delay;

int audio_effect_real_lfe_gain(short* buffer, int frame_size, int LPF_Gain)
{
    int i;
    short data_left, data_right;
    int output_size = frame_size << 2;
    float gain;
    int32_t tmp_sample;
    memcpy((delay_start + delay_frame), buffer, output_size);
    memcpy(buffer, delay_start, output_size);
    memmove(delay_start, (delay_start + output_size), delay_frame);

    gain = powf(10, (float)LPF_Gain/20);
    //ALOGE("audio_effect_real_lfe_gain gain %f\n", gain);
    for (i = 0; i < frame_size; i++) {
        tmp_sample = (int32_t)buffer[i * 2 + 0] * gain;
        buffer[i * 2 + 0] = _clamp16(tmp_sample);
        tmp_sample = (int32_t)buffer[i * 2 + 1] * gain;
        buffer[i * 2 + 1] = _clamp16(tmp_sample);
    }

	return 0;
}

