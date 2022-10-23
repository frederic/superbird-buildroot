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

#ifndef AML_CHANNEL_MAP_H
#define AML_CHANNEL_MAP_H

#include "aml_audio_stream.h"

typedef struct aml_channel_mapping {
    aml_data_format_t format;
    void *map_buffer;
    size_t map_buffer_size;
    size_t out_buffer_size;
    void * map_info;
} aml_channel_map_t;

/**
  ch is same with channel number opened in ALSA,
  speaker_config is used to tell how is the hardware speaker connected.
  In some project, the 4*I2S is always connected in different speaker config,
  so we need map the speaker config to the method of I2S connecting.
  For example, 3.1--> L R C LFE will map to 8ch--> L R C LFE 0 0 0 0
  4.0--> L R LS RS will map to 8ch--> L R 0 0 LS RS 0 0
*/
int aml_channelmap_init(aml_channel_map_t ** handle, char * speaker_config);
int aml_channelmap_close(aml_channel_map_t * handle);
int aml_channelmap_process(aml_channel_map_t * handle, aml_data_format_t *src, void * in_data, size_t nframes);

void aml_channelmap_parser_init(void * json_config);
void aml_channelmap_parser_deinit( );


#endif

