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

#ifndef _SUB_MIXING_FACTORY_H_
#define _SUB_MIXING_FACTORY_H_

#include <utils/Timers.h>
#include <linux/ioctl.h>
#include <system/audio.h>
#include <sound/asound.h>
#include <tinyalsa/asoundlib.h>
#include <hardware/audio.h>

enum MIXER_TYPE {
    MIXER_LPCM = 1,
    MIXER_MS12 = 2,
};

struct subMixing;
struct aml_stream_out;
struct aml_audio_device;

typedef int (*writeSubMixing_t)(
            struct subMixing *sm,
            void *buf,
            size_t bytes);
typedef int (*writeMainBuf_t)(
            struct subMixing *sm,
            void *buf,
            size_t bytes);
typedef int (*writeSysBuf_t)(
            struct subMixing *sm,
            void *buffer,
            size_t bytes);
struct audioCfg {
    int card;
    int device;
    uint32_t sampleRate;
    uint32_t channelCnt;
    audio_format_t format;
    uint32_t frame_size;
};

struct subMixing {
    enum MIXER_TYPE type;
    char name[16];
    int (*setName)(struct subMixing *sm, char *str);
    char *(*getName)(struct subMixing *sm);
    writeSubMixing_t write;
    /* main write related */
    writeMainBuf_t writeMain;
    struct audioCfg mainCfg;
    void *mainData;
    /* system write related */
    writeSysBuf_t writeSys;
    struct audioCfg sysCfg;
    void *sysData;
    /* output device related */
    struct audioCfg outputCfg;
    // which mixer is using, ms12 or pcm mixer
    void *mixerData;
    struct aml_audio_device *adev;
    int cnt_stream_using_mixer;
};

int initHalSubMixing(struct subMixing **smixer,
        enum MIXER_TYPE type,
        struct aml_audio_device *adev,
        bool isTV);
int deleteHalSubMixing(struct subMixing *smixer);

int initSubMixingInput(struct aml_stream_out *out,
        struct audio_config *config);
int deleteSubMixingInput(struct aml_stream_out *out);
int usecase_change_validate_l_sm(struct aml_stream_out *out, bool is_standby);
int out_standby_subMixingPCM(struct audio_stream *stream);
int switchNormalStream(struct aml_stream_out *aml_out, bool on);

#endif /* _SUB_MIXING_FACTORY_H_ */
