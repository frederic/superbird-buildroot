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

#ifndef _AML_DATA_UTILS_H_
#define _AML_DATA_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

enum eSAMPLE_SIZE {
	e16BitPerSample = 2,
	e32BitPerSample = 4,
};

#define MINUS_3_DB_IN_Q19_12 2896 // -3dB = 0.707 * 2^12 = 2896

typedef enum CHANNEL_CONTENT_INDEX {
	AML_CH_IDX_NULL      = -1,
	AML_CH_IDX_L         = 0,
	AML_CH_IDX_R         = 1,
	AML_CH_IDX_C         = 2,
	AML_CH_IDX_LFE       = 3,
	AML_CH_IDX_LS        = 4,
	AML_CH_IDX_RS        = 5,
	AML_CH_IDX_LT        = 6,
	AML_CH_IDX_RT        = 7,
	AML_CH_IDX_MAX       = 8,
	AML_CH_IDX_5_1_ALL   = 0x003F,
	AML_CH_IDX_7_1_ALL   = 0x00FF,
	AML_CH_IDX_5_1_2_ALL = 0x033F,
} eChannelContentIdx;

struct aml_audio_channel_name {
	int  ch_idx;
	char ch_name[50];
};

typedef enum I2S_DATALINE_INDEX {
	AML_I2S_PORT_IDX_01   = 0,
	AML_I2S_PORT_IDX_23   = 1,
	AML_I2S_PORT_IDX_45   = 2,
	AML_I2S_PORT_IDX_67   = 3,
	AML_I2S_PORT_IDX_NULL = 0x8000,
	AML_I2S_PORT_IDX_MAX  = 0xFFFF,
} eI2SDataLineIdx;

typedef enum CHANNEL_ON_I2S_BIT_MASK{
	AML_I2S_CHANNEL_NULL = 0x0,
	AML_I2S_CHANNEL_0    = 0x1<<0,
	AML_I2S_CHANNEL_1    = 0x1<<1,
	AML_I2S_CHANNEL_2    = 0x1<<2,
	AML_I2S_CHANNEL_3    = 0x1<<3,
	AML_I2S_CHANNEL_4    = 0x1<<4,
	AML_I2S_CHANNEL_5    = 0x1<<5,
	AML_I2S_CHANNEL_6    = 0x1<<6,
	AML_I2S_CHANNEL_7    = 0x1<<7,
} eChOnI2SBitMask;

#define AML_I2S_CHANNEL_COUNT  (8)
#define AML_CH_CNT_PER_PORT    (2)

struct aml_channel_map {
	eChannelContentIdx channel_idx;
	// WARNNING: support map to only one i2s data line
	eI2SDataLineIdx    i2s_idx;
	// WARNNING: may be one channel will map to 2 i2s data channel
	// eg, i2s_23 -> lfe/lfe
	eChOnI2SBitMask    bit_mask;
	// invert
	eChOnI2SBitMask    invert;
	// ditter
	eChOnI2SBitMask    ditter;
};

/******************************************************************************
 * Function: data_load_product_config()
 * Description:
 *      initial/load channel maps for current product
 * Input:  NULL
 * Output: NULL
 * Return: Hw I2S Ch Maps
 *****************************************************************************/
#if !defined (BUILDHOSTEXE)
struct aml_channel_map *data_load_product_config(void);
#endif

/******************************************************************************
 * Function: data_get_channel_i2s_port()
 * Description:
 *       get hw i2s port for channel "channelName"
 * Input:  eChannelContentIdx
 * Output:
 * Return: eI2SDataLineIdx
 *****************************************************************************/
int data_get_channel_i2s_port(
	struct aml_channel_map *map,
	eChannelContentIdx channelName);

eChannelContentIdx data_get_channel_content_idx(
	struct aml_channel_map *map,
	int bitmask);

/******************************************************************************
 * Function: data_get_channel_bit_mask()
 * Description:
 *       get hw i2s bit mask for channel "channelName"
 * Input:  eChannelContentIdx
 * Output:
 * Return: eChOnI2SBitMask
 *****************************************************************************/
int data_get_channel_bit_mask(
	struct aml_channel_map *map,
	eChannelContentIdx channelName);

/******************************************************************************
 * Function: data_empty_channels()
 * Description:
 *     clean channel data
 * Input:
 *     buf                    - input data
 *     frames                 - frame count
 *     framesz                - frame size
 *     channels               - channel count
 *     channel_empty_bit_mask - eChOnI2SBitMask, bit mask of channels which will
 *                              be empty
 * Output:
 *     buf                    - output data
 * Return: Zero if success
 *****************************************************************************/
int data_empty_channels(
	struct  aml_channel_map *map,
	void    *buf,
	size_t  frames,
	size_t  framesz,
	int     channels,
	int     channel_empty_bit_mask);

/******************************************************************************
 * Function: data_remix_to_lr_channel()
 * Description: re-mix data to LR channel
 * Input:
 *      buf                        - Input Buffer
 *      frames                     - Count of frames
 *      framesz                    - Frame size(eg, 16bits->2, 32bits->4)
 *      channels                   - Count of channels
 *      channel_remix_src_bit_mask - Which channel will be remix
 * Output:
 *      buf                        - Output Buffer
 * Return: Zero if success
 *****************************************************************************/
int data_remix_to_lr_channel(
	struct  aml_channel_map *map,
	void    *buf,
	size_t  frames,
    size_t  framesz,
	int     channels,
	eChannelContentIdx chIdx);

/******************************************************************************
 * Function: data_exchange_i2s_channels()
 * Description:
 *     exchange data between i2s_idx1 and i2s_idx2
 *     only support exchange between different i2s port
 * Input:
 *     buf      - input data
 *     channels - channel count
 *     frames   - frame count
 *     framesz  - frame size
 *     i2s_idx1 - index of i2s data line 1 for exchange
 *     i2s_idx2 - index of i2s data line 2 for exchange
 * Output:
 *     buf      - output data
 * Return: Zero if success
 *****************************************************************************/
int data_exchange_i2s_channels(
	void    *buf,
	size_t  frames,
	size_t  framesz,
	size_t  channels,
	eI2SDataLineIdx i2s_idx1,
	eI2SDataLineIdx i2s_idx2);

/******************************************************************************
 * Function: data_replace_lfe_data()
 * Description:
 *     replace lfe data
 * Input:
 *     out_channles            - channel count of putput data
 *     out_framesz             - frame size of output
 *     input_lfe_buffer        - input lfe data
 *     in_channles             - channel count of input data
 *     in_framesz              - frame size of input
 *     frames                  - frame count
 *     channel_insert_bit_mask - eChOnI2SBitMask, bit mask of lfe channel
 * Output:
 *     out_buf                 - output data
 * Return: Zero if success
 *****************************************************************************/
int data_replace_lfe_data(
	void    *out_buf,
	size_t  out_channles,
	size_t  out_framesz,
	void    *input_lfe_buffer,
	size_t  in_channles,
	size_t  in_framesz,
	size_t  frames,
	int     channel_insert_bit_mask);

/******************************************************************************
 * Function: data_invert_channels()
 * Description:
 *     Invert the data of masked channels
 * Input:
 *     buf                     - input data
 *     channels                - channel count
 *     frames                  - frame count
 *     framesz                 - frame size
 *     channel_invert_bit_mask - eChOnI2SBitMask, bit mask of channels which will
 *                               be invert
 * Output:
 *     buf                     - output data
 * Return: Zero if success
 *****************************************************************************/
int data_invert_channels(
	void    *buf,
	size_t  frames,
	size_t  framesz,
	int     channels,
	int     channel_invert_bit_mask);

/******************************************************************************
 * Function: data_concat_channels()
 * Description:
 *     connect 4*2ch data to one 8ch data.
 * Input:
 *     out_channels - channel count of output
 *     out_framesz  - frame size of output data
 *     in_buf1      - input 2ch buffer 1
 *     in_buf2      - input 2ch buffer 2
 *     in_buf3      - input 2ch buffer 3
 *     in_buf4      - input 2ch buffer 4
 *     in_channels  - channel count of all input data
 *     in_framesz   - frame size of input data
 *     frames       - frame count
 * Output:
 *     out_buf      - output data
 * Return: Zero if success
 *****************************************************************************/
int data_concat_channels(
	void    *out_buf,
	size_t  out_channels,
	size_t  out_framesz,
	void    *in_buf1, void *in_buf2, void *in_buf3, void *in_buf4,
	size_t  in_channels,
	size_t  in_framesz,
	size_t  frames);

/******************************************************************************
 * Function: data_add_ditter_to_channels()
 * Description:
 *     add ditter
 * Input:
 *     buffer                  - input data
 *     channels                - channel count
 *     frames                  - frame count
 *     framesz                 - frame size
 *     channel_ditter_bit_mask - eChOnI2SBitMask, bit mask of channels which will
 *                               be add ditter
 * Output:
 *     buffer                  - output data
 * Return: Zero if success
 *****************************************************************************/
int data_add_ditter_to_channels(
	void    *buffer,
	size_t  frames,
	size_t  framesz,
	int     channels,
	int     channel_ditter_bit_mask);

/******************************************************************************
 * Function: data_extend_to_channels()
 * Description: extend the data
 * Input:
 *     out_channels - channel count of output
 *     out_framesz  - Frame size(eg, 16bits->2, 32bits->4)
 *     in_buf       - Input Buffer
 *     in_channels  - channel count of input
 *     in_framesz   - Frame size(eg, 16bits->2, 32bits->4)
 *     frames       - frame count
 * Output:
 *     out_buf      - output buffer
 * Return: Zero if success
 *****************************************************************************/
int data_extend_channels(
	void    *out_buf,
	size_t  out_channels,
	size_t  out_framesz,
	void    *in_buf,
	size_t  in_channels,
	size_t  in_framesz,
	size_t  frames);

/******************************************************************************
 * Function: data_extract_channels()
 * Description: extract channel data
 * Input:
 *     out_channels             - channel count of output
 *     out_framesz              - frame size of output data
 *     in_buf                   - input buffer
 *     in_channels              - channel count of input
 *     in_framesz               - frame szie of input data
 *     frames                   - frame count
 *     channel_extract_bit_mask - eChOnI2SBitMask, ch mask will be extract
 * Output:
 *     out_buf                  - output buffer
 * Return: Zero if success
 *****************************************************************************/
int data_extract_channels(
	struct  aml_channel_map *map,
	void    *out_buf,
	size_t  out_channels,
	size_t  out_framesz,
	void    *in_buf,
	size_t  in_channels,
	size_t  in_framesz,
	size_t  frames,
	int     channel_extract_bit_mask);

int audio_effect_real_lfe_gain(short* buffer, int frame_size, int LPF_Gain);

#ifdef __cplusplus
}
#endif

#endif
