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

#ifndef _ALSA_CONFIG_PARAMETERS_H_
#define _ALSA_CONFIG_PARAMETERS_H_

/*
 *@brief get the hardware config parameters
 * input parameters
 *
 * final_config:
 *                 used to config the hardware, then get the proper pcm handle.
 * output_format:
 *                 output audio data format
 * channel:
 *                 used to set the channels of pcm_config
 * rate:
 *                 used to set the rate of pcm_config
 * platform_is_tv:
 *                 true when platform is TV
 *                 false when platform is BOX
 */
int get_hardware_config_parameters(
    struct pcm_config *final_config
    , audio_format_t output_format
    , unsigned int channels
    , unsigned int rate
    , bool platform_is_tv);

#endif // _ALSA_MANAGER_H_

