/*
 * Copyright (C) 2010 Amlogic Corporation.
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



#define LOG_TAG "audio_hw_profile"
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <unistd.h>
#include "log.h"
//#include <cutils/str_parms.h>
#include "properties.h"
#include "hardware.h"
#include "audio_core.h"
#include "audio_hal.h"

#include "audio_hw_utils.h"
#include "alsa_device_parser.h"
#define SOUND_CARDS_PATH "/proc/asound/cards"
#define SOUND_PCM_PATH  "/proc/asound/pcm"

/*
  type : 0 -> playback, 1 -> capture
*/
#define MAX_CARD_NUM    2
int get_external_card(int type)
{
    int card_num = 1;       // start num, 0 is defualt sound card.
    struct stat card_stat;
    char fpath[256];
    int ret;
    while (card_num <= MAX_CARD_NUM) {
        snprintf(fpath, sizeof(fpath), "/proc/asound/card%d", card_num);
        ret = stat(fpath, &card_stat);
        if (ret < 0) {
            ret = -1;
        } else {
            snprintf(fpath, sizeof(fpath), "/dev/snd/pcmC%uD0%c", card_num,
                     type ? 'c' : 'p');
            ret = stat(fpath, &card_stat);
            if (ret == 0) {
                return card_num;
            }
        }
        card_num++;
    }
    return ret;
}

int
get_aml_card()
{
    int card = -1, err = 0;
    int fd = -1;
    unsigned fileSize = 512;
    char *read_buf = NULL, *pd = NULL;
    //    static const char *const SOUND_CARDS_PATH = "/proc/asound/cards";
    fd = open(SOUND_CARDS_PATH, O_RDONLY);
    if (fd < 0) {
        ALOGE("ERROR: failed to open config file %s error: %d\n",
              SOUND_CARDS_PATH, errno);
        close(fd);
        return -EINVAL;
    }

    read_buf = (char *) malloc(fileSize);
    if (!read_buf) {
        ALOGE("Failed to malloc read_buf");
        close(fd);
        return -ENOMEM;
    }
    memset(read_buf, 0x0, fileSize);
    err = read(fd, read_buf, fileSize);
    if (fd < 0) {
        ALOGE("ERROR: failed to read config file %s error: %d\n",
              SOUND_CARDS_PATH, errno);
        close(fd);
        free(read_buf);
        return -EINVAL;
    }
    pd = strstr(read_buf, "AML");
    card = *(pd - 3) - '0';
OUT:
    free(read_buf);
    close(fd);
    return card;
}

int
get_spdif_port()
{
    int port = -1, err = 0;
    int fd = -1;
    unsigned fileSize = 512;
    char *read_buf = NULL, *pd = NULL;
    //    static const char *const SOUND_PCM_PATH = "/proc/asound/pcm";
    fd = open(SOUND_PCM_PATH, O_RDONLY);
    if (fd < 0) {
        ALOGE("ERROR: failed to open config file %s error: %d\n",
              SOUND_PCM_PATH, errno);
        close(fd);
        return -EINVAL;
    }
    read_buf = (char *) malloc(fileSize);
    if (!read_buf) {
        ALOGE("Failed to malloc read_buf");
        close(fd);
        return -ENOMEM;
    }
    memset(read_buf, 0x0, fileSize);
    err = read(fd, read_buf, fileSize);
    if (fd < 0) {
        ALOGE("ERROR: failed to read config file %s error: %d\n",
              SOUND_PCM_PATH, errno);
        close(fd);
        free(read_buf);
        return -EINVAL;
    }
    pd = strstr(read_buf, "SPDIF");
    port = *(pd - 3) - '0';
OUT:
    free(read_buf);
    close(fd);
    return port;
}


/*
CodingType MaxChannels SamplingFreq SampleSize
PCM, 2 ch, 32/44.1/48/88.2/96/176.4/192 kHz, 16/20/24 bit
PCM, 8 ch, 32/44.1/48/88.2/96/176.4/192 kHz, 16/20/24 bit
AC-3, 8 ch, 32/44.1/48 kHz,  bit
DTS, 8 ch, 44.1/48 kHz,  bit
OneBitAudio, 2 ch, 44.1 kHz,  bit
Dobly_Digital+, 8 ch, 44.1/48 kHz, 16 bit
DTS-HD, 8 ch, 44.1/48/88.2/96/176.4/192 kHz, 16 bit
MAT, 8 ch, 32/44.1/48/88.2/96/176.4/192 kHz, 16 bit
*/
char*  get_hdmi_sink_cap(const char *keys, audio_format_t format)
{
    int i = 0;
    char * infobuf = NULL;
    int channel = 0;
    int dgraw = 0;
    int fd = -1;
    int size = 0;
    char *aud_cap = NULL;
    ALOGD("%s is running...\n", __func__);
    infobuf = (char *)malloc(1024 * sizeof(char));
    if (infobuf == NULL) {
        ALOGE("malloc buffer failed\n");
        goto fail;
    }
    aud_cap = (char*)malloc(1024);
    if (aud_cap == NULL) {
        ALOGE("malloc buffer failed\n");
        goto fail;
    }
    memset(aud_cap, 0, 1024);
    memset(infobuf, 0, 1024);
    fd = open("/sys/class/amhdmitx/amhdmitx0/aud_cap", O_RDONLY);
    if (fd >= 0) {
        int nread = read(fd, infobuf, 1024);
        /* check the format cap */
        if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_FORMATS)) {
            ALOGD("query hdmi format...\n");
            size += sprintf(aud_cap, "sup_formats=%s", "AUDIO_FORMAT_PCM_16_BIT");
            if (mystrstr(infobuf, "Dobly_Digital+")) {
                size += sprintf(aud_cap + size, "|%s", "AUDIO_FORMAT_E_AC3");
            }
            if (mystrstr(infobuf, "AC-3")) {
                size += sprintf(aud_cap + size, "|%s", "AUDIO_FORMAT_AC3");
            }
            if (mystrstr(infobuf, "DTS-HD")) {
                size += sprintf(aud_cap + size, "|%s", "AUDIO_FORMAT_DTS|AUDIO_FORMAT_DTS_HD");
            } else if (mystrstr(infobuf, "DTS")) {
                size += sprintf(aud_cap + size, "|%s", "AUDIO_FORMAT_DTS");
            }
            if (mystrstr(infobuf, "MAT")) {
                size += sprintf(aud_cap + size, "|%s", "AUDIO_FORMAT_DOLBY_TRUEHD");
            }
        }
        /*check the channel cap */
        else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_CHANNELS)) {
            ALOGD("query hdmi channels...\n");
            /* take the 2ch suppported as default */
            size += sprintf(aud_cap, "sup_channels=%s", "AUDIO_CHANNEL_OUT_STEREO");
            if ((!alsa_device_is_auge() && mystrstr(infobuf, "PCM, 8 ch")) ||
                (mystrstr(infobuf, "Dobly_Digital+") && format == AUDIO_FORMAT_E_AC3)) {
                size += sprintf(aud_cap + size, "|%s", "AUDIO_CHANNEL_OUT_5POINT1|AUDIO_CHANNEL_OUT_7POINT1");
            } else if ((!alsa_device_is_auge() && mystrstr(infobuf, "PCM, 6 ch")) ||
                       (mystrstr(infobuf, "AC-3") && format == AUDIO_FORMAT_AC3) ||
                       /* backward compatibility for dd, if TV only supports dd+ */
                       (mystrstr(infobuf, "Dobly_Digital+") && format == AUDIO_FORMAT_AC3)) {
                size += sprintf(aud_cap + size, "|%s", "AUDIO_CHANNEL_OUT_5POINT1");
            }
        } else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES)) {
            ALOGD("query hdmi sample_rate...\n");
            /* take the 32/44.1/48 khz suppported as default */
            size += sprintf(aud_cap, "sup_sampling_rates=%s", "32000|44100|48000");
            if (mystrstr(infobuf, "88.2")) {
                size += sprintf(aud_cap + size, "|%s", "88200");
            }
            if (mystrstr(infobuf, "96")) {
                size += sprintf(aud_cap + size, "|%s", "96000");
            }
            if (mystrstr(infobuf, "176.4")) {
                size += sprintf(aud_cap + size, "|%s", "176400");
            }
            if ((mystrstr(infobuf, "192") && format != AUDIO_FORMAT_IEC61937) ||
                ((mystrstr(infobuf, "Dobly_Digital+") || mystrstr(infobuf, "DTS-HD") ||
                mystrstr(infobuf, "MAT")) && format == AUDIO_FORMAT_IEC61937)) {
                size += sprintf(aud_cap + size, "|%s", "192000");
            }
        }
    } else {
        ALOGE("open /sys/class/amhdmitx/amhdmitx0/aud_cap failed!!\n");
    }
    if (infobuf) {
        free(infobuf);
    }
    if (fd >= 0) {
        close(fd);
    }
    return aud_cap;
fail:
    if (aud_cap) {
        free(aud_cap);
    }
    if (infobuf) {
        free(infobuf);
    }
    return NULL;
}
char*  get_hdmi_arc_cap(unsigned *ad, int maxsize, const char *keys)
{
    int i = 0;
    int channel = 0;
    int dgraw = 0;
    int fd = -1;
    int size = 0;
    int raw_support = 0;
    int iec_added = 0;
    char *aud_cap = NULL;
    unsigned char format, ch, sr;
    aud_cap = (char*)malloc(1024);
    if (aud_cap == NULL) {
        ALOGE("malloc buffer failed\n");
        goto fail;
    }
    memset(aud_cap, 0, 1024);
    ALOGI("get_hdmi_arc_cap\n");
    /* check the format cap */
    if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_FORMATS)) {
        size += sprintf(aud_cap, "sup_formats=%s", "AUDIO_FORMAT_PCM_16_BIT");
    }
    /*check the channel cap */
    else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_CHANNELS)) {
        //ALOGI("check channels\n");
        /* take the 2ch suppported as default */
        size += sprintf(aud_cap, "sup_channels=%s", "AUDIO_CHANNEL_OUT_STEREO");
    } else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES)) {
        /* take the 32/44.1/48 khz suppported as default */
        size += sprintf(aud_cap, "sup_sampling_rates=%s", "32000|44100|48000");
        //ALOGI("check sample rate\n");
    }
    for (i = 0; i < maxsize; i++) {
        if (ad[i] != 0) {
            format = (ad[i] >> 19) & 0xf;
            ch = (ad[i] >> 16) & 0x7;
            sr = (ad[i] > 8) & 0xf;
            ALOGI("ad %x,format %d,ch %d,sr %d\n", ad[i], format, ch, sr);
            /* check the format cap */
            if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_FORMATS)) {
                //ALOGI("check format\n");
                if (format == 10) {
                    raw_support = 1;
                    size += sprintf(aud_cap + size, "|%s", "AUDIO_FORMAT_E_AC3");
                } else if (format == 2) {
                    raw_support = 1;
                    size += sprintf(aud_cap + size, "|%s", "AUDIO_FORMAT_AC3");
                } else if (format == 11) {
                    raw_support = 1;
                    size += sprintf(aud_cap + size, "|%s", "AUDIO_FORMAT_DTS|AUDIO_FORMAT_DTSHD");
                } else if (format == 7) {
                    raw_support = 1;
                    size += sprintf(aud_cap + size, "|%s", "AUDIO_FORMAT_DTS");
                } else if (format == 12) {
                    raw_support = 1;
                    size += sprintf(aud_cap + size, "|%s", "AUDIO_FORMAT_DOLBY_TRUEHD");
                }
                if (raw_support == 1 && iec_added == 0) {
                    size += sprintf(aud_cap + size, "|%s", "AUDIO_FORMAT_IEC61937");
                }
            }
            /*check the channel cap */
            else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_CHANNELS)) {
                //ALOGI("check channels\n");
                if (/*format == 1 && */ch == 7) {
                    size += sprintf(aud_cap + size, "|%s", "AUDIO_CHANNEL_OUT_5POINT1|AUDIO_CHANNEL_OUT_7POINT1");
                } else if (/*format == 1 && */ch == 5) {
                    size += sprintf(aud_cap + size, "|%s", "AUDIO_CHANNEL_OUT_5POINT1");
                }
            } else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES)) {
                ALOGI("check sample rate\n");
                if (format == 1 && sr == 4) {
                    size += sprintf(aud_cap + size, "|%s", "88200");
                }
                if (format == 1 && sr == 5) {
                    size += sprintf(aud_cap + size, "|%s", "96000");
                }
                if (format == 1 && sr == 6) {
                    size += sprintf(aud_cap + size, "|%s", "176400");
                }
                if (format == 1 && sr == 7) {
                    size += sprintf(aud_cap + size, "|%s", "192000");
                }
            }

        } else {
            format = 0;
            ch = 0;
            sr = 0;
        }
    }
    return aud_cap;
fail:
    if (aud_cap) {
        free(aud_cap);
    }
    return NULL;
}

char *strdup_hdmi_arc_cap_default(const char *keys, audio_format_t format)
{
    char fmt[] = "sup_formats=AUDIO_FORMAT_PCM_16_BIT|AUDIO_FORMAT_AC3|AUDIO_FORMAT_E_AC3|AUDIO_FORMAT_IEC61937";
    char ch_mask[128] = "sup_channels=AUDIO_CHANNEL_OUT_STEREO";
    char sr[64] = "sup_sampling_rates=48000|44100";
    char *cap = NULL;

    ALOGI("++%s, format = %#x, keys(%s)", __FUNCTION__, format, keys);
    /* check the format cap */
    if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_FORMATS)) {
        cap = strdup(fmt);
    } else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_CHANNELS)) {
        /* take the 2ch suppported as default */
        switch (format) {
        case AUDIO_FORMAT_E_AC3:
             strcat(ch_mask, "|AUDIO_CHANNEL_OUT_7POINT1");
        case AUDIO_FORMAT_IEC61937:
        case AUDIO_FORMAT_AC3:
              strcat(ch_mask, "|AUDIO_CHANNEL_OUT_5POINT1");
              cap = strdup(ch_mask);
              break;
        case AUDIO_FORMAT_PCM_16_BIT:
            cap = strdup(ch_mask);
            break;
        default:
            ALOGE("%s, unsupport format: %#x", __FUNCTION__, format);
            break;
        }
    } else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES)) {
        /* take the 48 khz suppported as default */
        switch (format) {
        case AUDIO_FORMAT_E_AC3:
            cap = strdup(sr);
            break;
        case AUDIO_FORMAT_IEC61937:
            strcat(sr, "|32000|192000");
            cap = strdup(sr);
            break;
        case AUDIO_FORMAT_PCM_16_BIT:
        case AUDIO_FORMAT_AC3:
            strcat(sr, "|32000");
            cap = strdup(sr);
            break;
        default:
            ALOGE("%s, unsupport format: %#x", __FUNCTION__, format);
            break;
        }
    } else {
        ALOGE("NOT support yet");
    }

    if (!cap) {
        cap = strdup("");
    }

    return cap;
}
