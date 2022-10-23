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

#define LOG_TAG "aml_channel_delay"
//#define LOG_NDEBUG 0

#include <cutils/log.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "aml_audio_delay.h"

#define ALIGN(size, align) ((size + align - 1) & (~(align - 1)))


static aml_audio_delay_st g_stAudioOutputDelay[AML_DELAY_OUTPORT_BUTT];
static const int g_u32OutDelayMaxDefault[AML_DELAY_OUTPORT_BUTT] = {1000, 1000, 1000};
static bool g_bAudioDelayInit = false;

int aml_audio_delay_init()
{
    memset(&g_stAudioOutputDelay, 0, sizeof(aml_audio_delay_st)*AML_DELAY_OUTPORT_BUTT);
    for (unsigned int i=0; i<AML_DELAY_OUTPORT_BUTT; i++) {
        int s32BfferSize = 0;
        unsigned int u32ChannelCnt = 2;
        if ((AML_DELAY_OUTPORT_ALL == i) || (AML_DELAY_OUTPORT_SPEAKER == i)) {
            /*calculate the max size for 8ch */
            u32ChannelCnt = 8;
        }
        s32BfferSize = 192 * u32ChannelCnt * 4 * g_u32OutDelayMaxDefault[i];
        ring_buffer_init(&g_stAudioOutputDelay[i].stDelayRbuffer, s32BfferSize);
    }
    g_bAudioDelayInit = true;
    return 0;
}

int aml_audio_delay_deinit()
{
    if (!g_bAudioDelayInit) {
        ALOGW("[%s:%d] audio delay not initialized", __func__, __LINE__);
        return -1;
    }

    for (unsigned int i=0; i<AML_DELAY_OUTPORT_BUTT; i++) {
        ring_buffer_release(&g_stAudioOutputDelay[i].stDelayRbuffer);
    }
    g_bAudioDelayInit = false;

    return 0;
}

/*
* s32DelayTimeMs: In milliseconds
*/
int aml_audio_delay_set_time(aml_audio_delay_type_e enAudioDelayType, int s32DelayTimeMs)
{
    if (!g_bAudioDelayInit) {
        ALOGW("[%s:%d] audio delay not initialized", __func__, __LINE__);
        return -1;
    }
    if (enAudioDelayType < AML_DELAY_OUTPORT_SPEAKER || enAudioDelayType >= AML_DELAY_OUTPORT_BUTT) {
        ALOGW("[%s:%d] delay type:%d invalid, min:%d, max:%d",
            __func__, __LINE__, enAudioDelayType, AML_DELAY_OUTPORT_SPEAKER, AML_DELAY_OUTPORT_BUTT-1);
        return -1;
    }
    if (s32DelayTimeMs < 0 || s32DelayTimeMs > g_u32OutDelayMaxDefault[enAudioDelayType]) {
        ALOGW("[%s:%d] unsupport delay time:%dms, min:%dms, max:%dms",
            __func__, __LINE__, s32DelayTimeMs, 0, g_u32OutDelayMaxDefault[enAudioDelayType]);
        return -1;
    }
    g_stAudioOutputDelay[enAudioDelayType].delay_time = s32DelayTimeMs;
    ALOGI("set audio output type:%d delay time: %dms", enAudioDelayType, s32DelayTimeMs);
    return 0;
}

int aml_audio_delay_clear(aml_audio_delay_type_e enAudioDelayType)
{
    if (!g_bAudioDelayInit) {
        ALOGW("[%s:%d] audio delay not initialized", __func__, __LINE__);
        return -1;
    }
    if (enAudioDelayType < AML_DELAY_OUTPORT_SPEAKER || enAudioDelayType >= AML_DELAY_OUTPORT_BUTT) {
        ALOGW("[%s:%d] delay type:%d invalid, min:%d, max:%d",
            __func__, __LINE__, enAudioDelayType, AML_DELAY_OUTPORT_SPEAKER, AML_DELAY_OUTPORT_BUTT-1);
        return -1;
    }

    ring_buffer_reset(&g_stAudioOutputDelay[enAudioDelayType].stDelayRbuffer);
    return 0;
}

int aml_audio_delay_process(aml_audio_delay_type_e enAudioDelayType, void *pData, int s32Size, audio_format_t enFormat, int nChannel)
{
    if (!g_bAudioDelayInit) {
        ALOGW("[%s:%d] audio delay not initialized", __func__, __LINE__);
        return -1;
    }
    if (enAudioDelayType < AML_DELAY_OUTPORT_SPEAKER || enAudioDelayType >= AML_DELAY_OUTPORT_BUTT) {
        ALOGW("[%s:%d] delay type:%d invalid, min:%d, max:%d",
            __func__, __LINE__, enAudioDelayType, AML_DELAY_OUTPORT_SPEAKER, AML_DELAY_OUTPORT_BUTT-1);
        return -1;
    }

    unsigned int    u32OneMsSize = 0;
    int             s32CurNeedDelaySize = 0;
    int             s32AvailDataSize = 0;

    if (AML_DELAY_OUTPORT_ALL == enAudioDelayType) {
        if (enFormat == AUDIO_FORMAT_E_AC3) {
            u32OneMsSize = 192 * 4;// * abs(adev->delay_ms);
        } else if (enFormat == AUDIO_FORMAT_AC3) {
            u32OneMsSize = 48 * 4;// * abs(adev->delay_ms);
        } else if (enFormat == AUDIO_FORMAT_MAT) {
            u32OneMsSize = 192 * 4 * 4;// * abs(adev->delay_ms);
        } else {
            u32OneMsSize = 48 * 8 * 4; // 48k * 8ch * 4Byte
        }
    } else if (AML_DELAY_OUTPORT_SPEAKER == enAudioDelayType){
        u32OneMsSize = 48 * nChannel * 2;  // 48k * 2ch * 2Byte
    } else if (AML_DELAY_OUTPORT_SPDIF == enAudioDelayType){
        if (AUDIO_FORMAT_IEC61937 == enFormat) {
            u32OneMsSize = 48 * 2 * 2;  // 48k * 2ch * 2Byte
        } else {
            u32OneMsSize = 48 * 2 * 4;  // 48k * 2ch * 4Byte [Notes: alsa only support 32bit(4Byte)]
        }
    }

    // calculate need delay total size
    s32CurNeedDelaySize = ALIGN(g_stAudioOutputDelay[enAudioDelayType].delay_time * u32OneMsSize, 16);
    // get current ring buffer delay data size
    s32AvailDataSize = ALIGN((get_buffer_read_space(&g_stAudioOutputDelay[enAudioDelayType].stDelayRbuffer) / u32OneMsSize) * u32OneMsSize, 16);

    ring_buffer_write(&g_stAudioOutputDelay[enAudioDelayType].stDelayRbuffer, (unsigned char *)pData, s32Size, UNCOVER_WRITE);
    ALOGV("%s:%d AvailDataSize:%d, enFormat:%#x, u32OneMsSize:%d", __func__, __LINE__, s32AvailDataSize, enFormat, u32OneMsSize);

    // accumulate this delay data
    if (s32CurNeedDelaySize > s32AvailDataSize) {
        int s32NeedAddDelaySize = s32CurNeedDelaySize - s32AvailDataSize;
        if (s32NeedAddDelaySize >= s32Size) {
            memset(pData, 0, s32Size);
            ALOGD("%s:%d type:%d,accumulate Data, CurNeedDelaySize:%d, need more DelaySize:%d, size:%d", __func__, __LINE__,
                enAudioDelayType, s32CurNeedDelaySize, s32NeedAddDelaySize, s32Size);
        } else {
            // splicing this pData data
            memset(pData, 0, s32NeedAddDelaySize);
            ring_buffer_read(&g_stAudioOutputDelay[enAudioDelayType].stDelayRbuffer, (unsigned char *)pData+s32NeedAddDelaySize, s32Size-s32NeedAddDelaySize);
            ALOGD("%s:%d type:%d accumulate part pData CurNeedDelaySize:%d, need more DelaySize:%d, size:%d", __func__, __LINE__,
                enAudioDelayType, s32CurNeedDelaySize, s32NeedAddDelaySize, s32Size);
        }
    // decrease this delay data
    } else if (s32CurNeedDelaySize < s32AvailDataSize) {
        unsigned int u32NeedDecreaseDelaySize = s32AvailDataSize - s32CurNeedDelaySize;
        // drop this delay data
        unsigned int    u32ClearedSize = 0;
        for (;u32ClearedSize < u32NeedDecreaseDelaySize; ) {
            unsigned int u32ResidualClearSize = u32NeedDecreaseDelaySize - u32ClearedSize;
            if (u32ResidualClearSize > (unsigned int)s32Size) {
                ring_buffer_read(&g_stAudioOutputDelay[enAudioDelayType].stDelayRbuffer, (unsigned char *)pData, s32Size);
                u32ClearedSize += s32Size;
            } else {
                ring_buffer_read(&g_stAudioOutputDelay[enAudioDelayType].stDelayRbuffer, (unsigned char *)pData, u32ResidualClearSize);
                break;
            }
        }
        ring_buffer_read(&g_stAudioOutputDelay[enAudioDelayType].stDelayRbuffer, (unsigned char *)pData, s32Size);
        ALOGD("%s:%d type:%d drop delay data, CurNeedDelaySize:%d, NeedDecreaseDelaySize:%d, size:%d", __func__, __LINE__,
            enAudioDelayType, s32CurNeedDelaySize, u32NeedDecreaseDelaySize, s32Size);
    } else {
        ring_buffer_read(&g_stAudioOutputDelay[enAudioDelayType].stDelayRbuffer, (unsigned char *)pData, s32Size);
        ALOGV("%s:%d do nothing, CurNeedDelaySize:%d, size:%d", __func__, __LINE__, s32CurNeedDelaySize, s32Size);
    }

    return 0;
}

