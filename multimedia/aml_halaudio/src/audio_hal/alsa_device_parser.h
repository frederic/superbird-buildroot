/*
 * Copyright (C) 2018 Amlogic Corporation.
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

#ifndef __ALSA_DEVICE_PARSER_H__
#define __ALSA_DEVICE_PARSER_H__

#include <stdint.h>
#include <sys/types.h>

/* ALSA cards for AML */
#define CARD_AMLOGIC_BOARD 0
/* ALSA ports for AML */
#define PORT_I2S           0
#define PORT_SPDIF         1
#define PORT_PCM           2
/* After axg chipset,
 * more devices are supported
 */
#define PROT_TDM           3
#define PROT_PDM           4
#define PORT_SPDIFB2HDMI   5
#define PORT_I2S2HDMI      6

bool alsa_device_is_auge(void);

int alsa_device_get_card_index();

int alsa_device_get_pcm_index(int alsaPORT);

#endif
