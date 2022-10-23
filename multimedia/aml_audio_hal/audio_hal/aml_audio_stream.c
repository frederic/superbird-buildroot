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

#define LOG_TAG "audio_hw_primary"
//#define LOG_NDEBUG 0
#include <cutils/log.h>
#include <string.h>
#include <tinyalsa/asoundlib.h>

#include "aml_alsa_mixer.h"
#include "aml_audio_stream.h"
#include "audio_hw_utils.h"
#include "dolby_lib_api.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define PCM  0/*AUDIO_FORMAT_PCM_16_BIT*/
#define DD   4/*AUDIO_FORMAT_AC3*/
#define AUTO 5/*choose by sink capability/source format/Digital format*/

/*
 *@brief get sink capability
 */
static audio_format_t get_sink_capability (struct audio_stream_out *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = aml_out->dev;
    struct aml_arc_hdmi_desc *hdmi_desc = &adev->hdmi_descs;

    bool dd_is_support = hdmi_desc->dd_fmt.is_support;
    bool ddp_is_support = hdmi_desc->ddp_fmt.is_support;

    audio_format_t sink_capability = AUDIO_FORMAT_PCM_16_BIT;

    //STB case
    if (adev->is_STB)
    {
        char *cap = NULL;
        cap = (char *) get_hdmi_sink_cap (AUDIO_PARAMETER_STREAM_SUP_FORMATS,0,&(adev->hdmi_descs));
        if (cap) {
            if (strstr(cap, "AUDIO_FORMAT_E_AC3") != NULL) {
                sink_capability = AUDIO_FORMAT_E_AC3;
            } else if (strstr(cap, "AUDIO_FORMAT_AC3") != NULL) {
                sink_capability = AUDIO_FORMAT_AC3;
            }
            ALOGI ("%s mbox+dvb case sink_capability =  %d\n", __FUNCTION__, sink_capability);
            free(cap);
            cap = NULL;
        }
    } else {
        if (ddp_is_support) {
            sink_capability = AUDIO_FORMAT_E_AC3;
        } else if (dd_is_support) {
            sink_capability = AUDIO_FORMAT_AC3;
        }
        ALOGI ("%s dd support %d ddp support %d\n", __FUNCTION__, dd_is_support, ddp_is_support);
    }
    return sink_capability;
}

bool is_sink_support_dolby_passthrough(audio_format_t sink_capability)
{
    return sink_capability == AUDIO_FORMAT_E_AC3 || sink_capability == AUDIO_FORMAT_AC3;
}

/*
 *@brief get sink format by logic min(source format / digital format / sink capability)
 * For Speaker/Headphone output, sink format keep PCM-16bits
 * For optical output, min(dd, source format, digital format)
 * For HDMI_ARC output
 *      1.digital format is PCM, sink format is PCM-16bits
 *      2.digital format is dd, sink format is min (source format,  AUDIO_FORMAT_AC3)
 *      3.digital format is auto, sink format is min (source format, digital format)
 */
void get_sink_format (struct audio_stream_out *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = aml_out->dev;
    /*set default value for sink_audio_format/optical_audio_format*/
    audio_format_t sink_audio_format = AUDIO_FORMAT_PCM_16_BIT;
    audio_format_t optical_audio_format = AUDIO_FORMAT_PCM_16_BIT;

    audio_format_t sink_capability = get_sink_capability(stream);
    audio_format_t source_format = aml_out->hal_internal_format;

    /*when device is HDMI_ARC*/
    ALOGI("!!!%s() Sink devices %#x Source format %#x digital_format(hdmi_format) %#x Sink Capability %#x\n",
          __FUNCTION__, adev->active_outport, aml_out->hal_internal_format, adev->hdmi_format, sink_capability);

    if ((source_format != AUDIO_FORMAT_PCM_16_BIT) && \
        (source_format != AUDIO_FORMAT_AC3) && \
        (source_format != AUDIO_FORMAT_E_AC3) && \
        (source_format != AUDIO_FORMAT_MAT) && \
        (source_format != AUDIO_FORMAT_AC4) && \
        (source_format != AUDIO_FORMAT_DTS) &&
        (source_format != AUDIO_FORMAT_DTS_HD)) {
        /*unsupport format [dts-hd/true-hd]*/
        ALOGI("%s() source format %#x change to %#x", __FUNCTION__, source_format, AUDIO_FORMAT_PCM_16_BIT);
        source_format = AUDIO_FORMAT_PCM_16_BIT;
    }

    // "adev->hdmi_format" is the UI selection item.
    // "adev->active_outport" was set when HDMI ARC cable plug in/off
    // condition 1: ARC port, single output.
    // condition 2: for BOX with continous mode, there are no speaker, only on HDMI outport, same use case
    // condition 3: for STB case
    if (adev->active_outport == OUTPORT_HDMI_ARC
         || ((eDolbyMS12Lib == adev->dolby_lib_type) && adev->continuous_audio_mode && !adev->is_TV)
         || adev->is_STB) {
        ALOGI("%s() HDMI ARC case", __FUNCTION__);
        switch (adev->hdmi_format) {
        case PCM:
            sink_audio_format = AUDIO_FORMAT_PCM_16_BIT;
            optical_audio_format = sink_audio_format;
            break;
        case DD:
            sink_audio_format = AUDIO_FORMAT_PCM_16_BIT;
            optical_audio_format = (source_format != AUDIO_FORMAT_DTS && source_format != AUDIO_FORMAT_DTS_HD) ? AUDIO_FORMAT_AC3 : AUDIO_FORMAT_DTS;
            break;
        case AUTO:
            sink_audio_format = (source_format != AUDIO_FORMAT_DTS && source_format != AUDIO_FORMAT_DTS_HD) ? min(source_format, sink_capability) : AUDIO_FORMAT_DTS;
            if ((source_format == AUDIO_FORMAT_PCM_16_BIT) && (adev->continuous_audio_mode == 1) && (sink_capability >= AUDIO_FORMAT_AC3)) {
                // For continous output, we need to continous output data
                // when input is PCM, we still need to output AC3/EAC3 according to sink capability
                sink_audio_format = sink_capability;
                ALOGI("%s continuous_audio_mode %d source_format %#x sink_capability %#x\n", __FUNCTION__, adev->continuous_audio_mode, source_format, sink_capability);
            }
            optical_audio_format = sink_audio_format;
            break;
        default:
            sink_audio_format = AUDIO_FORMAT_PCM_16_BIT;
            optical_audio_format = sink_audio_format;
            break;
        }
    }
    /*when device is SPEAKER/HEADPHONE*/
    else {
        ALOGI("%s() SPEAKER/HEADPHONE case", __FUNCTION__);
        if (!adev->is_STB && is_sink_support_dolby_passthrough(sink_capability))
            ALOGE("!!!%s() SPEAKER/HEADPHONE case, should no arc caps!!!", __FUNCTION__);

        switch (adev->hdmi_format) {
        case PCM:
            sink_audio_format = AUDIO_FORMAT_PCM_16_BIT;
            optical_audio_format = sink_audio_format;
            break;
        case DD:
            sink_audio_format = AUDIO_FORMAT_PCM_16_BIT;
            optical_audio_format = (source_format != AUDIO_FORMAT_DTS && source_format != AUDIO_FORMAT_DTS_HD) ? AUDIO_FORMAT_AC3 : AUDIO_FORMAT_DTS;
            break;
        case AUTO:
            if (adev->continuous_audio_mode == 0) {
                sink_audio_format = AUDIO_FORMAT_PCM_16_BIT;
                optical_audio_format = (source_format != AUDIO_FORMAT_DTS && source_format != AUDIO_FORMAT_DTS_HD)
                                       ? min(source_format, AUDIO_FORMAT_AC3)
                                       : AUDIO_FORMAT_DTS;
            } else {
                sink_audio_format = AUDIO_FORMAT_PCM_16_BIT;
                optical_audio_format = (source_format != AUDIO_FORMAT_DTS && source_format != AUDIO_FORMAT_DTS_HD) ? AUDIO_FORMAT_AC3 : AUDIO_FORMAT_DTS;
            }
            ALOGI("%s() source_format %#x sink_audio_format %#x "
                  "optical_audio_format %#x",
                  __FUNCTION__, source_format, sink_audio_format,
                  optical_audio_format);
            break;
        default:
            sink_audio_format = AUDIO_FORMAT_PCM_16_BIT;
            optical_audio_format = sink_audio_format;
            break;
        }
    }
    adev->sink_format = sink_audio_format;
    adev->optical_format = optical_audio_format;

    /* use single output for HDMI_ARC */
    if ((adev->active_outport == OUTPORT_HDMI_ARC) &&
        adev->bHDMIConnected)
        adev->sink_format = adev->optical_format;

    /* set the dual output format flag */
    //if (adev->sink_format != adev->optical_format) {
    //    aml_out->dual_output_flag = true;
    //} else {
    //    aml_out->dual_output_flag = false;
    //}
    ALOGI("%s sink_format %#x optical_format %#x, stream device %d\n",
           __FUNCTION__, adev->sink_format, adev->optical_format, aml_out->device);
    return ;
}

int set_stream_dual_output(struct audio_stream_out *stream, bool en)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream;
    aml_out->dual_output_flag = en;
    return 0;
}

int update_stream_dual_output(struct audio_stream_out *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = aml_out->dev;

    /* set the dual output format flag */
    if (adev->sink_format != adev->optical_format) {
        set_stream_dual_output(stream, true);
    } else {
        set_stream_dual_output(stream, false);
    }
    return 0;
}

bool is_dual_output_stream(struct audio_stream_out *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream;
    return aml_out->dual_output_flag;
}

bool is_hdmi_in_stable_hw (struct audio_stream_in *stream)
{
    struct aml_stream_in *in = (struct aml_stream_in *) stream;
    struct aml_audio_device *aml_dev = in->dev;
    int type = 0;
    int stable = 0;
    int txl_chip = is_txl_chip();
    hdmiin_audio_packet_t audio_packet = AUDIO_PACKET_NONE;


    stable = aml_mixer_ctrl_get_int (&aml_dev->alsa_mixer, AML_MIXER_ID_HDMI_IN_AUDIO_STABLE);
    if (!stable)
        return false;

    if (txl_chip) {
        audio_packet = get_hdmiin_audio_packet(&aml_dev->alsa_mixer);
        /*txl chip, we don't use hardware format detect for HBR*/
        if (audio_packet == AUDIO_PACKET_HBR) {
            return true;
        }
    }

    type = aml_mixer_ctrl_get_int (&aml_dev->alsa_mixer, AML_MIXER_ID_SPDIFIN_AUDIO_TYPE);
    if (type != in->spdif_fmt_hw) {
        ALOGV ("%s(), in type changed from %d to %d", __func__, in->spdif_fmt_hw, type);
        in->spdif_fmt_hw = type;
        return false;
    }

    return true;
}

bool is_hdmi_in_stable_sw (struct audio_stream_in *stream)
{
    struct aml_stream_in *in = (struct aml_stream_in *) stream;
    struct aml_audio_device *aml_dev = in->dev;
    struct aml_audio_patch *patch = aml_dev->audio_patch;
    audio_format_t fmt;

    /* now, only hdmiin->(spk, hp, arc) cases init the soft parser thread
     * TODO: init hdmiin->mix soft parser too
     */
    if (!patch)
        return true;

    fmt = audio_parse_get_audio_type (patch->audio_parse_para);
    if (fmt != in->spdif_fmt_sw) {
        ALOGV ("%s(), in type changed from %#x to %#x", __func__, in->spdif_fmt_sw, fmt);
        in->spdif_fmt_sw = fmt;
        return false;
    }

    return true;
}


void  release_audio_stream(struct audio_stream_out *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream;
    if (aml_out->is_tv_platform == 1) {
        free(aml_out->tmp_buffer_8ch);
        aml_out->tmp_buffer_8ch = NULL;
        free(aml_out->audioeffect_tmp_buffer);
        aml_out->audioeffect_tmp_buffer = NULL;
    }
    free(stream);
}
bool is_atv_in_stable_hw (struct audio_stream_in *stream)
{
    struct aml_stream_in *in = (struct aml_stream_in *) stream;
    struct aml_audio_device *aml_dev = in->dev;
    int type = 0;
    int stable = 0;

    stable = aml_mixer_ctrl_get_int (&aml_dev->alsa_mixer, AML_MIXER_ID_ATV_IN_AUDIO_STABLE);
    if (!stable)
        return false;

    return true;
}
bool is_av_in_stable_hw(struct audio_stream_in *stream)
{
    struct aml_stream_in *in = (struct aml_stream_in *)stream;
    struct aml_audio_device *aml_dev = in->dev;
    int type = 0;
    int stable = 0;

    stable = aml_mixer_ctrl_get_int (&aml_dev->alsa_mixer, AML_MIXER_ID_AV_IN_AUDIO_STABLE);
    if (!stable)
        return false;

    return true;
}

bool is_spdif_in_stable_hw (struct audio_stream_in *stream)
{
    struct aml_stream_in *in = (struct aml_stream_in *) stream;
    struct aml_audio_device *aml_dev = in->dev;
    int type = 0;

    type = aml_mixer_ctrl_get_int (&aml_dev->alsa_mixer, AML_MIXER_ID_SPDIFIN_AUDIO_TYPE);
    if (type != in->spdif_fmt_hw) {
        ALOGV ("%s(), in type changed from %d to %d", __func__, in->spdif_fmt_hw, type);
        in->spdif_fmt_hw = type;
        return false;
    }

    return true;
}

int set_audio_source(struct aml_mixer_handle *mixer_handle,
        enum input_source audio_source, bool is_auge)
{
    int src = audio_source;

    if (is_auge) {
        switch (audio_source) {
        case LINEIN:
            src = TDMIN_A;
            break;
        case ATV:
            src = FRATV;
            break;
        case HDMIIN:
            src = FRHDMIRX;
            break;
        case ARCIN:
            src = EARCRX_DMAC;
            break;
        case SPDIFIN:
            src = SPDIFIN_AUGE;
            break;
        default:
            ALOGW("%s(), src: %d not support", __func__, src);
            src = FRHDMIRX;
            break;
        }
    }

    return aml_mixer_ctrl_set_int(mixer_handle, AML_MIXER_ID_AUDIO_IN_SRC, src);
}

int set_spdifin_pao(struct aml_mixer_handle *mixer_handle,int enable)
{
    return aml_mixer_ctrl_set_int(mixer_handle,AML_MIXER_ID_SPDIFIN_PAO, enable);
}

int get_spdifin_samplerate(struct aml_mixer_handle *mixer_handle)
{
    int index = aml_mixer_ctrl_get_int(mixer_handle, AML_MIXER_ID_SPDIF_IN_SAMPLERATE);

    return index;
}

int get_hdmiin_samplerate(struct aml_mixer_handle *mixer_handle)
{
    int stable = 0;

    stable = aml_mixer_ctrl_get_int(mixer_handle, AML_MIXER_ID_HDMI_IN_AUDIO_STABLE);
    if (!stable) {
        return -1;
    }

    return aml_mixer_ctrl_get_int(mixer_handle, AML_MIXER_ID_HDMI_IN_SAMPLERATE);
}

int get_hdmiin_channel(struct aml_mixer_handle *mixer_handle)
{
    int stable = 0;
    int channel_index = 0;

    stable = aml_mixer_ctrl_get_int(mixer_handle, AML_MIXER_ID_HDMI_IN_AUDIO_STABLE);
    if (!stable) {
        return -1;
    }

    /*hmdirx audio support: N/A, 2, 3, 4, 5, 6, 7, 8*/
    channel_index = aml_mixer_ctrl_get_int(mixer_handle, AML_MIXER_ID_HDMI_IN_CHANNELS);
    if (channel_index != 7)
        return 2;
    else
        return 8;
}

int get_HW_resample(struct aml_mixer_handle *mixer_handle)
{
    return aml_mixer_ctrl_get_int(mixer_handle, AML_MIXER_ID_HW_RESAMPLE_ENABLE);
}

int enable_HW_resample(struct aml_mixer_handle *mixer_handle, int enable_sr)
{
    if (enable_sr == 0)
        aml_mixer_ctrl_set_int(mixer_handle, AML_MIXER_ID_HW_RESAMPLE_ENABLE, HW_RESAMPLE_DISABLE);
    else
        aml_mixer_ctrl_set_int(mixer_handle, AML_MIXER_ID_HW_RESAMPLE_ENABLE, enable_sr);
    return 0;
}

bool Stop_watch(struct timespec start_ts, int64_t time) {
    struct timespec end_ts;
    int64_t start_ms, end_ms;
    int64_t interval_ms;

    clock_gettime (CLOCK_MONOTONIC, &end_ts);
    start_ms = start_ts.tv_sec * 1000LL +
               start_ts.tv_nsec / 1000000LL;
    end_ms = end_ts.tv_sec * 1000LL +
             end_ts.tv_nsec / 1000000LL;
    interval_ms = end_ms - start_ms;
    if (interval_ms < time) {
        return true;
    }
    return false;
}

bool signal_status_check(audio_devices_t in_device, int *mute_time,
                        struct audio_stream_in *stream) {
    if ((in_device & AUDIO_DEVICE_IN_HDMI) &&
            (!is_hdmi_in_stable_hw(stream) ||
            !is_hdmi_in_stable_sw(stream))) {
        *mute_time = 100;
        return false;
    }
    if ((in_device & AUDIO_DEVICE_IN_TV_TUNER) &&
            !is_atv_in_stable_hw (stream)) {
        *mute_time = 500;
        return false;
    }
    if (((in_device & AUDIO_DEVICE_IN_SPDIF) ||
            ((in_device & AUDIO_DEVICE_IN_HDMI_ARC) &&
                    (access(SYS_NODE_EARC_RX, F_OK) == -1))) &&
           !is_spdif_in_stable_hw(stream)) {
        *mute_time = 1000;
        return false;
    }
    if ((in_device & AUDIO_DEVICE_IN_LINE) &&
            !is_av_in_stable_hw(stream)) {
       *mute_time = 1000;
       return false;
    }
    return true;
}


hdmiin_audio_packet_t get_hdmiin_audio_packet(struct aml_mixer_handle *mixer_handle)
{
    int audio_packet = 0;
    audio_packet = aml_mixer_ctrl_get_int(mixer_handle,AML_MIXER_ID_HDMIIN_AUDIO_PACKET);
    if (audio_packet < 0) {
        return AUDIO_PACKET_NONE;
    }
    return (hdmiin_audio_packet_t)audio_packet;
}

unsigned int inport_to_device(enum IN_PORT inport)
{
    unsigned int device = 0;
    switch (inport) {
    case INPORT_TUNER:
        device = AUDIO_DEVICE_IN_TV_TUNER;
        break;
    case INPORT_HDMIIN:
        device = AUDIO_DEVICE_IN_AUX_DIGITAL;
        break;
    case INPORT_ARCIN:
        device = AUDIO_DEVICE_IN_HDMI_ARC;
        break;
    case INPORT_SPDIF:
        device = AUDIO_DEVICE_IN_SPDIF;
        break;
    case INPORT_LINEIN:
        device = AUDIO_DEVICE_IN_LINE;
        break;
    case INPORT_REMOTE_SUBMIXIN:
        device = AUDIO_DEVICE_IN_REMOTE_SUBMIX;
        break;
    case INPORT_WIRED_HEADSETIN:
        device = AUDIO_DEVICE_IN_WIRED_HEADSET;
        break;
    case INPORT_BUILTIN_MIC:
        device = AUDIO_DEVICE_IN_BUILTIN_MIC;
        break;
    case INPORT_BT_SCO_HEADSET_MIC:
        device = AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET;
        break;
    default:
        ALOGE("%s(), unsupport %s", __func__, inport2String(inport));
        device = AUDIO_DEVICE_IN_BUILTIN_MIC;
        break;
    }

    return device;
}

