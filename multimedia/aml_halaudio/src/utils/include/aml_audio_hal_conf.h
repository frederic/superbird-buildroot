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

#ifndef _AML_AUDIO_HAL_CONF_H_
#define _AML_AUDIO_HAL_CONF_H_

enum eAudioHalConfType {
	eAmlConfTypeChMap = 0,
	eAmlConfTypeChInv = 1,
	eAmlConfTypeChDit = 2,
};

/* channel map need move to *.dts in kernel */
// hardware channel map@i2s port
#define AML_I2S_CHANNEL0 "i2s.channel0"
#define AML_I2S_CHANNEL1 "i2s.channel1"
#define AML_I2S_CHANNEL2 "i2s.channel2"
#define AML_I2S_CHANNEL3 "i2s.channel3"
#define AML_I2S_CHANNEL4 "i2s.channel4"
#define AML_I2S_CHANNEL5 "i2s.channel5"
#define AML_I2S_CHANNEL6 "i2s.channel6"
#define AML_I2S_CHANNEL7 "i2s.channel7"

// if need invert the data before send to i2s port
#define AML_I2S_INVERT_CH0 "i2s.invert.channel0"
#define AML_I2S_INVERT_CH1 "i2s.invert.channel1"
#define AML_I2S_INVERT_CH2 "i2s.invert.channel2"
#define AML_I2S_INVERT_CH3 "i2s.invert.channel3"
#define AML_I2S_INVERT_CH4 "i2s.invert.channel4"
#define AML_I2S_INVERT_CH5 "i2s.invert.channel5"
#define AML_I2S_INVERT_CH6 "i2s.invert.channel6"
#define AML_I2S_INVERT_CH7 "i2s.invert.channel7"

/* dap */
// enable aml software dap process
#define AML_SW_DAP_ENABLE  "aml.dap.enable"

/* lfe */
// enable aml iir
#define AML_SW_IIR_ENABLE  "aml.iir.enable"

/* others */
// if have real center channel
#define AML_SPK_HAVE_CENTER "speaker.center.enable"

/* ditter, only need on ATMOS dsp now */
// if need ditter on i2s data
#define AML_DITTER_ENABLE   "i2s.ditter.enable"
// enable ditter@i2s channel
#define AML_DITTER_I2S_CH0  "i2s.ditter.channel0"
#define AML_DITTER_I2S_CH1  "i2s.ditter.channel1"
#define AML_DITTER_I2S_CH2  "i2s.ditter.channel2"
#define AML_DITTER_I2S_CH3  "i2s.ditter.channel3"
#define AML_DITTER_I2S_CH4  "i2s.ditter.channel4"
#define AML_DITTER_I2S_CH5  "i2s.ditter.channel5"
#define AML_DITTER_I2S_CH6  "i2s.ditter.channel6"
#define AML_DITTER_I2S_CH7  "i2s.ditter.channel7"

#endif
