/*
 This file is used for tuning audio.
 */

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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <system/audio.h>
#include <media/AudioSystem.h>
#include <media/AudioParameter.h>

#ifdef LOG
#undef LOG
#endif
#define LOG(x...) printf("[AQ] " x)

using namespace android;

typedef enum {
    ATV_GAIN = 0,
    DTV_GAIN,
    HDMI_GAIN,
    AV_GAIN
} S_gain;

typedef enum {
    SPEAKER_GAIN = 0,
    SPDIF_ARC_GAIN,
    HEADPHONE_GAIN
} P_gain;

typedef enum {
    LINEIN = 0,
    ATV,
    HDMIIN,
    SPDIFIN
} AQ_source;

typedef enum {
    SPEAKER = 0,
    HDMI_ARC,
    SPDIF,
    AUX_LINE,
    HEADPHONE
} AQ_sink;

struct audio_source_gain_s {
    float atv;
    float dtv;
    float hdmi;
    float av;
};

struct audio_source_mute_s {
    int mute;
};

struct audio_post_gain_s {
    float speaker;
    float spdif_arc;
    float headphone;
};

static inline float DbToAmpl(float decibels)
{
    if (decibels <= -758) {
        return 0.0f;
    }
    return exp( decibels * 0.115129f); // exp( dB * ln(10) / 20 )
}

static int audio_source_post_gain_func(int id, int device, float gain)
{
    struct audio_source_gain_s s_gain;
    struct audio_post_gain_s   p_gain;

    String8 keyValuePairs = String8("");
    String8 mString = String8("");

    char value[64] = {0};
    AudioParameter param = AudioParameter();

    audio_io_handle_t handle = AUDIO_IO_HANDLE_NONE;

    if (id == 0) {
        mString = AudioSystem::getParameters(handle, String8("SOURCE_GAIN"));
        if (mString.isEmpty()) {
            LOG("SourcePostGain: Get source gain failed\n");
            return -1;
        }
        sscanf(mString.string(), "source_gain = %f %f %f %f", &s_gain.atv, &s_gain.dtv, &s_gain.hdmi, &s_gain.av);
        LOG("input device: [%d];  source gain: [%f dB]\n", device, gain);
        gain = DbToAmpl(gain);
        switch (device) {
        case ATV_GAIN:
            s_gain.atv = gain;
            break;
        case DTV_GAIN:
            s_gain.dtv = gain;
            break;
        case HDMI_GAIN:
            s_gain.hdmi = gain;
            break;
        case AV_GAIN:
            s_gain.av = gain;
            break;
        default:
            LOG("SourcePostGain: unknown source device %d\n", device);
            return -1;
        }
        LOG("Set source gain value: atv = %f, dtv = %f, hdmi = %f, av = %f\n", s_gain.atv, s_gain.dtv, s_gain.hdmi, s_gain.av);
        snprintf(value, 50, "%f %f %f %f", s_gain.atv, s_gain.dtv, s_gain.hdmi, s_gain.av);
        param.add(String8("SOURCE_GAIN"), String8(value));
        keyValuePairs = param.toString();
        if (AudioSystem::setParameters(handle, keyValuePairs) != NO_ERROR) {
            LOG("SourcePostGain: Set source gain failed\n");
            return -1;
        }
        param.remove(String8("SOURCE_GAIN"));
    } else if (id == 1) {
        mString = AudioSystem::getParameters(handle, String8("POST_GAIN"));
        if (mString.isEmpty()) {
            LOG("SourcePostGain: Get post gain failed\n");
            return -1;
        }
        sscanf(mString.string(), "post_gain = %f %f %f\n", &p_gain.speaker, &p_gain.spdif_arc, &p_gain.headphone);
        LOG("output device: [%d];  post gain: [%f dB]\n", device, gain);
        gain = DbToAmpl(gain);
        switch (device) {
        case SPEAKER_GAIN:
            p_gain.speaker = gain;
            break;
        case SPDIF_ARC_GAIN:
            p_gain.spdif_arc = gain;
            break;
        case HEADPHONE_GAIN:
            p_gain.headphone = gain;
            break;
        default:
            LOG("SourcePostGain: unknown post device %d\n", device);
            return -1;
        }
        LOG("Set post gain value: speaker = %f, spdif_arc = %f, headphone = %f\n", p_gain.speaker, p_gain.spdif_arc, p_gain.headphone);
        snprintf(value, 40, "%f %f %f", p_gain.speaker, p_gain.spdif_arc, p_gain.headphone);
        param.add(String8("POST_GAIN"), String8(value));
        keyValuePairs = param.toString();
        if (AudioSystem::setParameters(handle, keyValuePairs) != NO_ERROR) {
            LOG("SourcePostGain: Set post gain failed\n");
            return -1;
        }
        param.remove(String8("POST_GAIN"));
    } else {
        LOG("SourcePostGain: unknown id = %d\n", id);
        return -1;
    }

    return 0;
}

static int audio_patch_func()
{
    status_t status;

    FILE *fp = NULL;
    char buf[50] = {0};

    int source = 2;
    int sink = 0;
    int out_gain = 100;
    int last_source = -1;
    int last_sink = -1;
    int last_out_gain = -1;

    unsigned int i;
    unsigned int generation1;
    unsigned int generation;
    unsigned int numPorts = 0;

    struct audio_patch nPatch;
    struct audio_port *nPorts = NULL;

    audio_devices_t in_device = AUDIO_DEVICE_IN_HDMI;
    audio_devices_t out_device = AUDIO_DEVICE_OUT_SPEAKER;

    audio_patch_handle_t handle = (audio_patch_handle_t)0;

    LOG("AudioPatch: Start...\n");
    fp = fopen("/data/audio_AQ.txt", "a+");
    if (fp == NULL) {
        LOG("AudioPatch: open %s failed\n", "/data/audio_AQ.txt");
        return -1;
    }

    while (1) {
        fseek(fp, 0L, SEEK_SET);
        fread(buf, 1, 50, fp);

        sscanf(buf, "%d %d %d", &source, &sink, &out_gain);

        if (source == 27) {
            LOG("AudioPatch: Exit...\n");
            goto exit;
        }

        switch (source) {
        case LINEIN:
            in_device = AUDIO_DEVICE_IN_LINE;
            LOG("AudioPatch: source: lineIn\n");
            break;
        case ATV:
            in_device = AUDIO_DEVICE_IN_TV_TUNER;
            LOG("AudioPatch: source: TV tuner\n");
            break;
        case HDMIIN:
            LOG("AudioPatch: source: HDMI IN\n");
            in_device = AUDIO_DEVICE_IN_HDMI;
            break;
        default:
            LOG("AudioPatch: unknown source %d, use HDMI as default source\n", source);
            in_device = AUDIO_DEVICE_IN_HDMI;
            break;
        }

        switch (sink) {
        case SPEAKER:
            LOG("AudioPatch: sink: speaker\n");
            out_device = AUDIO_DEVICE_OUT_SPEAKER;
            AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_WIRED_HEADPHONE,
                        AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, NULL, "HEADPHONE");
            break;
        case HEADPHONE:
            LOG("AudioPatch: sink: headphone\n");
            AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_WIRED_HEADPHONE,
                        AUDIO_POLICY_DEVICE_STATE_AVAILABLE, NULL, "HEADPHONE");
            out_device = AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
            break;
        default:
            LOG("AudioPatch: unknown sink %d, use SPEAKER as default sink\n", sink);
            out_device = AUDIO_DEVICE_OUT_SPEAKER;
            break;
        }

        if (out_gain < 0)
            out_gain = 0;
        else if (out_gain > 100)
            out_gain = 100;

        if (last_source != source || last_sink != sink) {
            if (handle != 0)
                AudioSystem::releaseAudioPatch(handle);
            if (nPorts != NULL)
                free(nPorts);
            handle = (audio_patch_handle_t)0;
            nPorts = NULL;
            numPorts = 0;

            LOG("AudioPatch: in_device: 0x%x out_device: 0x%x out_gain: %d\n", in_device, out_device, out_gain);

            status = AudioSystem::listAudioPorts(AUDIO_PORT_ROLE_NONE, AUDIO_PORT_TYPE_NONE, &numPorts, NULL, &generation1);
            if (status != NO_ERROR) {
                LOG("AudioPatch: Get list audio ports failed %d\n", status);
                goto exit;
            }
            if (numPorts == 0) {
                LOG("AudioPatch: Get numPorts failed\n");
                goto exit;
            }
            nPorts = (struct audio_port *)calloc(numPorts, sizeof(struct audio_port));
            if (nPorts == NULL) {
                LOG("AudioPatch: Allocate audio port failed");
                goto exit;
            } else
                LOG("AudioPatch: nPorts pointer: %p\n", nPorts);
            status = AudioSystem::listAudioPorts(AUDIO_PORT_ROLE_NONE, AUDIO_PORT_TYPE_NONE, &numPorts, nPorts, &generation);
            if (status != NO_ERROR) {
                LOG("AudioPatch: Get list audio ports data failed %d\n", status);
                goto exit;
            } else
                LOG("AudioPatch: numPorts: %d generation: %d generation1: %d\n", numPorts, generation, generation1);

            nPatch.id = handle;
            nPatch.num_sources = 0;
            nPatch.num_sinks = 0;

            for (i = 0; i < numPorts; i++) {
                struct audio_port *aport = &nPorts[i];
                if (nPatch.num_sources != 1 && aport->role == AUDIO_PORT_ROLE_SOURCE && aport->type == AUDIO_PORT_TYPE_DEVICE) {
                    if (aport->ext.device.type == in_device) {
                        LOG("AudioPatch: Init audioPatch src: 0x%x\n", aport->ext.device.type);
                        nPatch.sources[0].id = aport->id;
                        nPatch.sources[0].role = aport->role;
                        nPatch.sources[0].type = aport->type;
                        nPatch.sources[0].channel_mask = AUDIO_CHANNEL_IN_MONO;
                        nPatch.sources[0].sample_rate = 48000;
                        nPatch.sources[0].format = AUDIO_FORMAT_PCM_16_BIT;
                        nPatch.sources[0].config_mask = AUDIO_PORT_CONFIG_ALL;
                        nPatch.num_sources = 1;
                    }
                }

                if (nPatch.num_sinks != 1 && aport->role == AUDIO_PORT_ROLE_SINK && aport->type == AUDIO_PORT_TYPE_DEVICE) {
                    if (aport->ext.device.type == out_device) {
                        LOG("AudioPatch: Init audioPatch sink: 0x%x\n", aport->ext.device.type);
                        nPatch.sinks[0].id = aport->id;
                        nPatch.sinks[0].role = aport->role;
                        nPatch.sinks[0].type = aport->type;
                        nPatch.sinks[0].channel_mask = AUDIO_CHANNEL_IN_MONO;
                        nPatch.sinks[0].sample_rate = 48000;
                        nPatch.sinks[0].format = AUDIO_FORMAT_PCM_16_BIT;
                        nPatch.sinks[0].config_mask = AUDIO_PORT_CONFIG_ALL;
                        nPatch.num_sinks = 1;
                    }
                }
            }

            if (nPatch.num_sinks == 1 && nPatch.num_sources ==1) {
                LOG("AudioPatch: Init audio patch successs\n");
            } else {
                LOG("AudioPatch: Init audio patch failed\n");
                status = -1;
                goto exit;
            }

            status = AudioSystem::createAudioPatch(&nPatch, &handle);
            if (status != NO_ERROR) {
                LOG("AudioPatch: Create audio patch failed\n");
                goto exit;
            } else
                LOG("AudioPatch: Create audio patch successs status: %d hande: %d\n\n", status, handle);

            nPatch.sinks[0].config_mask = AUDIO_PORT_CONFIG_GAIN;
            nPatch.sinks[0].gain.values[0] = out_gain;
            status = AudioSystem::setAudioPortConfig(&nPatch.sinks[0]);
            if (status != NO_ERROR) {
                LOG("AudioPatch: Set audio port config failed\n");
                goto exit;
            }
        } else if (last_out_gain != out_gain) {
            nPatch.sinks[0].config_mask = AUDIO_PORT_CONFIG_GAIN;
            nPatch.sinks[0].gain.values[0] = out_gain;
            status = AudioSystem::setAudioPortConfig(&nPatch.sinks[0]);
            if (status != NO_ERROR) {
                LOG("AudioPatch: Set audio port config failed\n");
                goto exit;
            }
        }

        last_source = source;
        last_sink = sink;
        last_out_gain = out_gain;

        usleep(1000 * 1000);
    }

exit:
    if (handle != 0)
        AudioSystem::releaseAudioPatch(handle);
    if (nPorts != NULL)
        free(nPorts);
    if (fp != NULL)
        fclose(fp);
    return status;
}

static int audio_source_mute(int gain)
{
    String8 keyValuePairs = String8("");
    String8 mString = String8("");
    audio_source_mute_s s_mute;

    char value[64] = {0};
    AudioParameter param = AudioParameter();

    audio_io_handle_t handle = AUDIO_IO_HANDLE_NONE;
    mString = AudioSystem::getParameters(handle, String8("SOURCE_MUTE"));
    if (mString.isEmpty()) {
        LOG("Sourcemute: Get source mute failed\n");
         return -1;
    }
    sscanf(mString.string(), "source_mute = %d\n", &s_mute.mute);
    s_mute.mute = gain;
    snprintf(value, 40, "%d", s_mute.mute);
    param.add(String8("SOURCE_MUTE"), String8(value));
    keyValuePairs = param.toString();
    if (AudioSystem::setParameters(handle, keyValuePairs) != NO_ERROR) {
        LOG("Sourcemute: Set mute failed\n");
        return -1;
    }
    param.remove(String8("SOURCE_MUTE"));
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 1 && argc != 4 && argc != 2) {
        LOG("*****************Audio Patch**********************\n");
        LOG("Usage: %s\n", argv[0]);
        LOG("**************************************************\n\n");

        LOG("***************Source Post Gain*******************\n");
        LOG("Usage: %s <id> <device> <gain>\n", argv[0]);
        LOG("**************************************************\n\n");

        return -1;
    }

    if (argc == 1) {
        LOG("****************Audio Patch PARAM*****************\n");
        LOG("source: 0 -> LINEIN   1 -> ATV   2 -> HDMI-IN\n");
        LOG("sink:   0 -> SPEAKER  4 -> HEADPHONE\n");
        LOG("out_gain: 0 ~ 100\n");
        LOG("**************************************************\n\n");

        LOG("*****************Audio Patch CMD******************\n");
        LOG("start: echo <source> <sink> <out_gain> > /data/audio_AQ.txt\n");
        LOG("exit : echo 27 > /data/audio_AQ.txt\n");
        LOG("**************************************************\n\n");

        audio_patch_func();
    } else if (argc == 4) {
        LOG("*************Source Post Gain PARAM***************\n");
        LOG("id:     0 -> Source\n");
        LOG("device: 0 ->  ATV 1 -> DTV 2 -> HDMI 3 -> AV\n");
        LOG("id:     1 -> Post\n");
        LOG("device: 0 ->  SPEAKER 1 -> SPDIF_ARC 2 -> HEADPHONE\n");
        LOG("**************************************************\n\n");

        int id = 0;
        int device = 0;
        float gain = 0.0;

        sscanf(argv[1], "%d", &id);
        sscanf(argv[2], "%d", &device);
        sscanf(argv[3], "%f", &gain);

        LOG("id = %d device = %d gain = %f dB\n", id, device, gain);
        audio_source_post_gain_func(id, device, gain);
    } else if (argc ==2) {
        LOG("*************Source mute PARAM***************\n");
        LOG("mute:  1 ->mute  0 ->unmute\n");
        LOG("**************************************************\n\n");

        int gain = 0;
        sscanf(argv[1], "%d", &gain);
        LOG("gain = %d \n", gain);
        audio_source_mute(gain);
    }

    return 0;
}
