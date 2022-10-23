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

#include <string.h>
#include <cutils/log.h>
#include <system/audio.h>
#include <hardware/audio.h>
#include <tinyalsa/asoundlib.h>

#include "audio_hw.h"
#include "alsa_manager.h"
#include "audio_hw_utils.h"
#include "alsa_device_parser.h"
#include "dolby_lib_api.h"
#include "aml_audio_stream.h"
#include "audio_virtual_buf.h"
#define AML_ZERO_ADD_MIN_SIZE 1024

#define AUDIO_EAC3_FRAME_SIZE 16
#define AUDIO_AC3_FRAME_SIZE 4
#define AUDIO_MAT_FRAME_SIZE 64
#define AUDIO_TV_PCM_FRAME_SIZE 32
#define AUDIO_DEFAULT_PCM_FRAME_SIZE 4

#define MAX_AVSYNC_GAP (10*90000)
#define MAX_AVSYNC_WAIT_TIME (3*1000*1000)

#define ALSA_OUT_BUF_NS (10000000000LL)   //10s
#define ALSA_PREFILL_BUF_NS (10000000000LL)   //10s


/*
insert bytes of into audio effect buffer to clear intermediate data when exit
*/
static int insert_eff_zero_bytes(struct aml_audio_device *adev, size_t size)
{
    int ret = 0;
    size_t insert_size = size;
    size_t once_write_size = 0;
    size_t bytes_per_frame = audio_bytes_per_sample(AUDIO_FORMAT_PCM_16_BIT)
                             * audio_channel_count_from_out_mask(AUDIO_CHANNEL_OUT_STEREO);
    int16_t *effect_tmp_buf = NULL;

    if ((size % bytes_per_frame) != 0) {
        ALOGE("%s, size= %zu , not bytes_per_frame muliplier\n", __FUNCTION__, size);
        goto exit;
    }

    effect_tmp_buf = (int16_t *) adev->effect_buf;
    while (insert_size > 0) {
        once_write_size = insert_size > adev->effect_buf_size ? adev->effect_buf_size : insert_size;
        memset(effect_tmp_buf, 0, once_write_size);
        /*aduio effect process for speaker*/
        for (int i = 0; i < adev->native_postprocess.num_postprocessors; i++) {
            audio_post_process(adev->native_postprocess.postprocessors[i], effect_tmp_buf, once_write_size / bytes_per_frame);
        }
        insert_size -= once_write_size;
    }

exit:
    return 0;
}

int aml_alsa_output_open(struct audio_stream_out *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = aml_out->dev;
    struct pcm_config *config = &aml_out->config;
    struct pcm_config config_raw;
    unsigned int device = aml_out->device;
    struct dolby_ms12_desc *ms12 = &(adev->ms12);
    ALOGI("\n+%s stream %p,device %d", __func__, stream,device);
    if (eDolbyMS12Lib == adev->dolby_lib_type) {
        if (adev->ms12.dolby_ms12_enable) {
            config = &(adev->ms12_config);
            device = ms12->device;
            ALOGI("%s indeed choose ms12 [config and device(%d)]", __func__, ms12->device);
            if (aml_out->device != device) {
                ALOGI("%s stream device(%d) differ with current device(%d)!", __func__, aml_out->device, device);
                aml_out->is_device_differ_with_ms12 = true;
            }
        }
    } else if (eDolbyDcvLib == adev->dolby_lib_type) {
        if (is_dual_output_stream(stream) && adev->optical_format != AUDIO_FORMAT_PCM_16_BIT) {
            device = I2S_DEVICE;
            config->rate = MM_FULL_POWER_SAMPLING_RATE;
        } else if (adev->sink_format != AUDIO_FORMAT_PCM_16_BIT &&
                   aml_out->hal_format != AUDIO_FORMAT_PCM_16_BIT) {
            memset(&config_raw, 0, sizeof(struct pcm_config));
            int period_mul = (adev->sink_format  == AUDIO_FORMAT_E_AC3) ? 4 : 1;
            config_raw.channels = 2;
            config_raw.rate = aml_out->config.rate;//MM_FULL_POWER_SAMPLING_RATE ;
            config_raw.period_size = DEFAULT_PLAYBACK_PERIOD_SIZE * period_mul;
            config_raw.period_count = PLAYBACK_PERIOD_COUNT;
            config_raw.start_threshold = DEFAULT_PLAYBACK_PERIOD_SIZE * PLAYBACK_PERIOD_COUNT;
            config_raw.format = PCM_FORMAT_S16_LE;
            config = &config_raw;
            device = DIGITAL_DEVICE;
        }
    }
    int card = aml_out->card;
    struct pcm *pcm = adev->pcm_handle[device];
    struct pcm *earc_pcm = adev->pcm_handle[EARC_DEVICE];
    ALOGI("%s pcm %p earc_pcm %p", __func__, pcm, earc_pcm);

    // close former and open with configs
    // TODO: check pcm configs and if no changes, do nothing
    if (pcm && device != DIGITAL_DEVICE && device != I2S_DEVICE) {
        ALOGI("pcm device already opened,re-use pcm handle %p", pcm);
    } else {
        /*
        there are some audio format when digital output
        from dd->dd+ or dd+ --> dd,we need reopen the device.
        */
        if (pcm) {
            if (device == I2S_DEVICE)
                ALOGI("pcm device already opened,close the handle %p to reopen", pcm);
            pcm_close(pcm);
            adev->pcm_handle[device] = NULL;
            aml_out->pcm = NULL;
            pcm = NULL;
        }
        if (SUPPORT_EARC_OUT_HW && adev->bHDMIConnected && earc_pcm) {
            ALOGI("earc_pcm device already opened,close the handle %p to reopen", earc_pcm);
            pcm_close(earc_pcm);
            adev->pcm_handle[EARC_DEVICE] = NULL;
            aml_out->earc_pcm = NULL;
            earc_pcm = NULL;
        }
        int device_index = device;
        // mark: will there wil issue here? conflit with MS12 device?? zz
        if (card == alsa_device_get_card_index()) {
            device_index = alsa_device_update_pcm_index(device, PLAYBACK);
        }

        ALOGI("%s, audio open card(%d), device(%d)", __func__, card, device_index);
        ALOGI("ALSA open configs: channels %d format %d period_count %d period_size %d rate %d",
              config->channels, config->format, config->period_count, config->period_size, config->rate);
#ifndef TINYALSA_VERSION
        ALOGI("ALSA open configs: threshold start %u stop %u silence %u silence_size %d avail_min %d",
              config->start_threshold, config->stop_threshold, config->silence_threshold, config->silence_size, config->avail_min);
#else
        ALOGI("ALSA open configs: threshold start %u stop %u",
              config->start_threshold, config->stop_threshold, config->silence_threshold);
#endif
        pcm = pcm_open(card, device_index, PCM_OUT, config);
        if (!pcm || !pcm_is_ready(pcm)) {
            ALOGE("%s, pcm %p open [ready %d] failed", __func__, pcm, pcm_is_ready(pcm));
            return -ENOENT;
        }
        if (SUPPORT_EARC_OUT_HW && adev->bHDMIConnected && (!aml_out->earc_pcm)) {
            int earc_port = alsa_device_update_pcm_index(PORT_EARC, PLAYBACK);
            struct pcm_config earc_config = update_earc_out_config(config);
            earc_pcm = pcm_open(card, earc_port, PCM_OUT, &earc_config);
            if (!earc_pcm || !pcm_is_ready(earc_pcm)) {
                ALOGE("%s, earc_pcm %p open [ready %d] failed", __func__,
                        earc_pcm, pcm_is_ready(earc_pcm));
                return -ENOENT;
            }
            aml_out->earc_pcm = earc_pcm;
            adev->pcm_refs[EARC_DEVICE]++;
            adev->pcm_handle[EARC_DEVICE] = earc_pcm;
        }
    }
    aml_out->pcm = pcm;
    adev->pcm_handle[device] = pcm;
    adev->pcm_refs[device]++;
    aml_out->dropped_size = 0;
    aml_out->device = device;
    ALOGI("-%s, audio out(%p) device(%d) refs(%d) is_normal_pcm %d, pcm handle %p earc_pcm handle %p\n\n",
          __func__, aml_out, device, adev->pcm_refs[device], aml_out->is_normal_pcm, pcm, earc_pcm);

    return 0;
}

void aml_alsa_output_close(struct audio_stream_out *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = aml_out->dev;
    unsigned int device = aml_out->device;
    struct dolby_ms12_desc *ms12 = &(adev->ms12);

    ALOGI("\n+%s() stream %p , dual output: %d\n",
            __func__, stream, is_dual_output_stream(stream));
    if (eDolbyMS12Lib == adev->dolby_lib_type) {
        if (aml_out->is_device_differ_with_ms12) {
            ALOGI("%s stream out device(%d) truely use device(%d)\n", __func__, aml_out->device, ms12->device);
            device = ms12->device;
            aml_out->is_device_differ_with_ms12 = false;
        }
    }  else if (eDolbyDcvLib == adev->dolby_lib_type) {
        if (is_dual_output_stream(stream) && adev->ddp.digital_raw == 1) {
            device = I2S_DEVICE;
            ALOGI("dual output,close i2s device");
        }
    }

    struct pcm *pcm = adev->pcm_handle[device];
    struct pcm *earc_pcm = adev->pcm_handle[EARC_DEVICE];

    adev->pcm_refs[device]--;
    ALOGI("+%s, audio out(%p) device(%d), refs(%d) is_normal_pcm %d,handle %p",
          __func__, aml_out, device, adev->pcm_refs[device], aml_out->is_normal_pcm, aml_out->pcm);
    if (adev->pcm_refs[device] < 0) {
        adev->pcm_refs[device] = 0;
        ALOGI("%s, device(%d) refs(%d)\n", __func__, device, adev->pcm_refs[device]);
    }

    if (SUPPORT_EARC_OUT_HW && adev->bHDMIConnected && earc_pcm) {
        adev->pcm_refs[EARC_DEVICE]--;
        if (adev->pcm_refs[EARC_DEVICE] < 0) {
            adev->pcm_refs[EARC_DEVICE] = 0;
            ALOGI("%s, EARC_DEVICE refs(%d)\n", __func__, adev->pcm_refs[EARC_DEVICE]);
        }
    }

    if (pcm && (adev->pcm_refs[device] == 0)) {
        ALOGI("%s(), pcm_close audio device[%d] pcm handle %p", __func__, device, pcm);
        // insert enough zero byte to clean audio processing buffer when exit
        // after test 8192*2 bytes should be enough, that is DEFAULT_PLAYBACK_PERIOD_SIZE*32
        insert_eff_zero_bytes(adev, DEFAULT_PLAYBACK_PERIOD_SIZE * 32);
        pcm_close(pcm);
        adev->pcm_handle[device] = NULL;
        if (SUPPORT_EARC_OUT_HW && adev->bHDMIConnected && earc_pcm && (adev->pcm_refs[EARC_DEVICE] == 0)) {
            ALOGI("%s(), pcm_close audio device[%d] earc_pcm handle %p", __func__, EARC_DEVICE, earc_pcm);
            pcm_close (earc_pcm);
            adev->pcm_handle[EARC_DEVICE] = NULL;
            earc_pcm = NULL;
        }
    }
    aml_out->pcm = NULL;
    aml_out->earc_pcm = NULL;

    /* dual output management */
    /* TODO: only for Dcv, not for MS12 */
    if (is_dual_output_stream(stream) && (eDolbyDcvLib == adev->dolby_lib_type)) {
        device = DIGITAL_DEVICE;
        pcm = adev->pcm_handle[device];
        if (pcm) {
            pcm_close(pcm);
            adev->pcm_handle[device] = NULL;
        }
    }
    if ((adev->continuous_audio_mode == 1) && (eDolbyMS12Lib == adev->dolby_lib_type)) {
        audio_virtual_buf_close(&aml_out->alsa_vir_buf_handle);
    }
    ALOGI("-%s()\n\n", __func__);
}
static int aml_alsa_add_zero(struct aml_stream_out *stream, int size)
{
    int ret = 0;
    int retry = 10;
    char *buf = NULL;
    int write_size = 0;
    int adjust_bytes = size;
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = aml_out->dev;

    while (retry--) {
        buf = malloc(AML_ZERO_ADD_MIN_SIZE);
        if (buf != NULL) {
            break;
        }
        usleep(10000);
    }
    if (buf == NULL) {
        return ret;
    }
    memset(buf, 0, AML_ZERO_ADD_MIN_SIZE);

    while (adjust_bytes > 0) {
        write_size = adjust_bytes > AML_ZERO_ADD_MIN_SIZE ? AML_ZERO_ADD_MIN_SIZE : adjust_bytes;
        ret = pcm_write(aml_out->pcm, (void*)buf, write_size);
        if (ret < 0) {
            ALOGE("%s alsa write fail when insert", __func__);
            break;
        }
        adjust_bytes -= write_size;
    }
    free(buf);
    return (size - adjust_bytes);
}

size_t aml_alsa_output_write(struct audio_stream_out *stream,
                             void *buffer,
                             size_t bytes)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = aml_out->dev;
    int ret = 0;
    struct pcm_config *config = &aml_out->config;
    size_t frame_size = audio_stream_out_frame_size(stream);
    bool need_trigger = false;
    bool is_dtv = (adev->patch_src == SRC_DTV);
    bool is_dtv_live = 1;
    bool has_video = adev->is_has_video;
    unsigned int first_apts = 0;
    unsigned int first_vpts = 0;
    unsigned int cur_apts = 0;
    unsigned int cur_vpts = 0;
    unsigned int cur_pcr = 0;
    int av_diff = 0;
    int need_drop_inject = 0;
    int64_t pretime = 0;
    unsigned char*audio_data = (unsigned char*)buffer;

    switch (adev->sink_format) {
    case AUDIO_FORMAT_E_AC3:
        frame_size = AUDIO_EAC3_FRAME_SIZE;
        break;
    case AUDIO_FORMAT_AC3:
        frame_size = AUDIO_AC3_FRAME_SIZE;
        break;
    case AUDIO_FORMAT_MAT:
        frame_size = AUDIO_MAT_FRAME_SIZE;
        break;
    default:
        frame_size = (aml_out->is_tv_platform == true) ? AUDIO_TV_PCM_FRAME_SIZE : AUDIO_DEFAULT_PCM_FRAME_SIZE;
        break;
    }

    // pre-check
    if (!has_video || !is_dtv) {
        goto write;
    }
    if (!adev->first_apts_flag) {
        goto write;
    }

    // video not comming. skip audio
    get_sysfs_uint(TSYNC_FIRSTVPTS, (unsigned int *)&first_vpts);
    if (first_vpts == 0) {
        ALOGI("[audio-startup] video not comming - skip this packet. size:%zu\n", bytes);
        aml_out->dropped_size += bytes;
        //memset(audio_data, 0, bytes);
        return bytes;
    }

    // av both comming. check need add zero or skip
    //get_sysfs_uint(TSYNC_FIRSTAPTS, (unsigned int *)&(first_apts));
    first_apts = adev->first_apts;
    get_sysfs_uint(TSYNC_VPTS, (unsigned int *) & (cur_vpts));
    get_sysfs_uint(TSYNC_PCRSCR, (unsigned int *) & (cur_pcr));
    if (cur_vpts <= first_vpts) {
        cur_vpts = first_vpts;
    }

    cur_apts = (unsigned int)((int64_t)first_apts + (int64_t)(((int64_t)aml_out->dropped_size * 90) / (48 * frame_size)));
    av_diff = (int)((int64_t)cur_apts - (int64_t)cur_vpts);
    ALOGI("[audio-startup] av both comming.fa:0x%x fv:0x%x ca:0x%x cv:0x%x cp:0x%x d:%d fs:%zu diff:%d ms\n",
          first_apts, first_vpts, cur_apts, cur_vpts, cur_pcr, aml_out->dropped_size, frame_size, av_diff / 90);

    // Exception
    if (abs(av_diff) > MAX_AVSYNC_GAP) {
        adev->first_apts = cur_apts;
        aml_audio_start_trigger(stream);
        adev->first_apts_flag = false;
        ALOGI("[audio-startup] case-0: ca:0x%x cv:0x%x dr:%d\n",
              cur_apts, cur_vpts, aml_out->dropped_size);
        goto write;
    }

#if 0
    // avsync inside 100ms, start
    if (abs(av_diff) < 100 * 90) {
        adev->first_apts = cur_apts;
        aml_audio_start_trigger(stream);
        adev->first_apts_flag = false;
        ALOGI("[audio-startup] case-1: ca:0x%x cv:0x%x dr:%d\n",
              cur_apts, cur_vpts, aml_out->dropped_size);
        goto write;
    }
#endif
    // drop case
    if (av_diff < 0) {
        need_drop_inject = abs(av_diff) / 90 * 48 * frame_size;
        need_drop_inject &= ~(frame_size - 1);
        if (need_drop_inject >= (int)bytes) {
            aml_out->dropped_size += bytes;
            ALOGI("[audio-startup] av sync drop %d pcm. total dropped:%d need_drop:%d\n",
                  (int)bytes, aml_out->dropped_size, need_drop_inject);
            if (cur_pcr > cur_vpts) {
                // seems can not cache video in time
                adev->first_apts = cur_apts;
                aml_audio_start_trigger(stream);
                adev->first_apts_flag = false;
                ALOGI("[audio-startup] case-2: drop %d pcm. total dropped:%d need_drop:%d\n",
                      (int)bytes, aml_out->dropped_size, need_drop_inject);
            }
            return bytes;
        } else {
            //emset(audio_data, 0, need_drop_inject);
            if (SUPPORT_EARC_OUT_HW && adev->bHDMIConnected && aml_out->earc_pcm && adev->bHDMIARCon) {
                ret = pcm_write(aml_out->earc_pcm, audio_data + need_drop_inject, bytes - need_drop_inject);
            } else {
                ret = pcm_write(aml_out->pcm, audio_data + need_drop_inject, bytes - need_drop_inject);
            }
            aml_out->dropped_size += bytes;
            cur_apts = first_apts + (aml_out->dropped_size * 90) / (48 * frame_size);
            adev->first_apts = cur_apts;
            aml_audio_start_trigger(stream);
            adev->first_apts_flag = false;
            ALOGI("[audio-startup] case-3: drop done. ca:0x%x cv:0x%x dropped:%d\n",
                  cur_apts, cur_vpts, aml_out->dropped_size);
            return bytes;
        }
    }

    // wait video sync
    cur_apts = (unsigned int)((int64_t)first_apts + (int64_t)(((int64_t)aml_out->dropped_size * 90) / (48 * frame_size)));
    pretime = aml_gettime();
    while (1) {
        usleep(MIN_WRITE_SLEEP_US);
        get_sysfs_uint(TSYNC_PCRSCR, (unsigned int *) & (cur_vpts));
        if (cur_vpts > cur_apts - 10 * 90) {
            break;
        }
        if (aml_gettime() - pretime >= MAX_AVSYNC_WAIT_TIME) {
            ALOGI("[audio-startup] add zero exceed %d ms quit.\n", MAX_AVSYNC_WAIT_TIME / 1000);
            break;
        }
    }
    adev->first_apts = cur_apts;
    aml_audio_start_trigger(stream);
    adev->first_apts_flag = false;
    ALOGI("[audio-startup] case-4: ca:0x%x cv:0x%x dropped:%d add zero:%d\n",
          cur_apts, cur_vpts, aml_out->dropped_size, need_drop_inject);

write:

    //code here to handle audio start issue to make higher a/v sync precision
    //if (adev->first_apts_flag) {
    //    aml_audio_start_trigger(stream);
    //    adev->first_apts_flag = false;
    //}
#if 0
    FILE *fp1 = fopen("/data/pcm_write.pcm", "a+");
    if (fp1) {
        int flen = fwrite((char *)buffer, 1, bytes, fp1);
        //ALOGD("flen = %d---outlen=%d ", flen, out_frames * frame_size);
        fclose(fp1);
    } else {
        ALOGE("could not open file:/data/pcm_write.pcm");
    }
#endif

    // SWPL-412, when input source is DTV, and UI set "parental_control_av_mute" command to audio hal
    // we need to mute audio output for PCM output here
    if (adev->patch_src == SRC_DTV && adev->parental_control_av_mute) {
        memset(buffer,0x0,bytes);
    }
    if (getprop_bool("media.audiohal.outdump")) {
        aml_audio_dump_audio_bitstreams("/data/audio/pcm_write.raw",
            buffer, bytes);
    }
    if (SUPPORT_EARC_OUT_HW && adev->bHDMIConnected && aml_out->earc_pcm && adev->bHDMIARCon) {
        ret = pcm_write(aml_out->earc_pcm, buffer, bytes);
        if (ret < 0) {
            ALOGE("%s write failed,aml_out->earc_pcm handle:%p, ret:%#x, err info:%s",
                    __func__, aml_out->earc_pcm, ret, strerror(errno));
        }
    } else {
        ret = pcm_write(aml_out->pcm, buffer, bytes);
        if (ret < 0) {
            ALOGE("%s write failed,pcm handle:%p, ret:%#x, err info:%s", __func__,
                    aml_out->pcm, ret, strerror(errno));
        }
    }

    if ((adev->continuous_audio_mode == 1) && (eDolbyMS12Lib == adev->dolby_lib_type) && (bytes != 0)) {
        uint64_t input_ns = 0;
        int mutex_lock_success = 0;
        int sample_rate = 48000;
        /*we will go to sleep, unlock mutex*/
        if(pthread_mutex_unlock(&adev->alsa_pcm_lock) == 0) {
            mutex_lock_success = 1;
        }
        if (adev->ms12_config.rate != 0) {
            sample_rate = adev->ms12_config.rate;
        }
        input_ns = (uint64_t)(bytes) * 1000000000LL / frame_size / sample_rate;

        if (aml_out->alsa_vir_buf_handle == NULL) {
            /*set the buf to 10s, and then fill the buff, we will use this to make the data consuming to stable*/
            audio_virtual_buf_open(&aml_out->alsa_vir_buf_handle, "alsa out", ALSA_OUT_BUF_NS, ALSA_OUT_BUF_NS, 0);
            audio_virtual_buf_process(aml_out->alsa_vir_buf_handle, ALSA_OUT_BUF_NS - input_ns/2);
        }


        audio_virtual_buf_process(aml_out->alsa_vir_buf_handle, input_ns);
        if(mutex_lock_success) {
            pthread_mutex_lock(&adev->alsa_pcm_lock);
        }
    }

    return ret;
}

int aml_alsa_output_get_letancy(struct audio_stream_out *stream)
{
    // TODO: add implementation
    (void) stream;
    return 0;
}

void aml_close_continuous_audio_device(struct aml_audio_device *adev)
{
    int pcm_index = 0;
    int spdif_index = 1;
    struct pcm *continuous_pcm_device = adev->pcm_handle[pcm_index];
    struct pcm *earc_pcm = adev->pcm_handle[EARC_DEVICE];
    struct pcm *continuous_spdif_device = adev->pcm_handle[1];
    ALOGI("\n+%s() choose device %d pcm %p\n", __FUNCTION__, pcm_index, continuous_pcm_device);
    ALOGI("%s maybe also choose device %d pcm %p\n", __FUNCTION__, spdif_index, continuous_spdif_device);
    if (continuous_pcm_device) {
        pcm_close(continuous_pcm_device);
        continuous_pcm_device = NULL;
        adev->pcm_handle[pcm_index] = NULL;
        adev->pcm_refs[pcm_index] = 0;
        if (SUPPORT_EARC_OUT_HW && adev->bHDMIConnected && earc_pcm) {
            pcm_close (earc_pcm);
            adev->pcm_handle[EARC_DEVICE] = NULL;
            adev->pcm_refs[EARC_DEVICE] = 0;
        }
    }
    if (continuous_spdif_device) {
        pcm_close(continuous_spdif_device);
        continuous_spdif_device = NULL;
        adev->pcm_handle[spdif_index] = NULL;
        adev->pcm_refs[spdif_index] = 0;
    }
    ALOGI("-%s(), when continuous is at end, the pcm/spdif devices(single/dual output) are closed!\n\n", __FUNCTION__);
    return ;
}

int alsa_depop(int card)
{
    struct pcm_config pcm_cfg_out = {
        .channels = 2,
        .rate = MM_FULL_POWER_SAMPLING_RATE,
        .period_size = DEFAULT_PLAYBACK_PERIOD_SIZE,
        .period_count = PLAYBACK_PERIOD_COUNT,
        .start_threshold = DEFAULT_PLAYBACK_PERIOD_SIZE,
        .format = PCM_FORMAT_S16_LE,
    };
    int port = alsa_device_update_pcm_index(PORT_I2S, PLAYBACK);

    struct pcm *pcm = pcm_open(card, port, PCM_OUT, &pcm_cfg_out);
    char *buf = calloc(1, 2048);
    if (buf == NULL)
        return -ENOMEM;

    if (pcm_is_ready(pcm)) {
        pcm_write(pcm, buf, 2048);
        pcm_close(pcm);
    } else {
        ALOGE("%s(), open fail", __func__);
    }
    usleep(10000);
    if (buf)
        free(buf);

    ALOGI("%s, card %d, device %d", __func__, card, port);
    return 0;
}

size_t aml_alsa_input_read(struct audio_stream_in *stream,
                        void *buffer,
                        size_t bytes)
{
    struct aml_stream_in *in = (struct aml_stream_in *)stream;
    struct aml_audio_device *aml_dev = in->dev;
    struct aml_audio_patch *patch = aml_dev->audio_patch;
    char  *read_buf = (char *)buffer;
    int ret = 0;
    size_t  read_bytes = 0;
    struct pcm *pcm_handle = in->pcm;
    size_t frame_size = in->config.channels * pcm_format_to_bits(in->config.format) / 8;
    while (read_bytes < bytes) {
        size_t remaining = bytes - read_bytes;
        ret = pcm_read(pcm_handle, (unsigned char *)buffer + read_bytes, remaining);

        if (patch && patch->input_thread_exit) {
            memset(buffer,0,bytes);
            return 0;
        }
        if (!ret) {
            read_bytes += remaining;
            ALOGV("ret:%d read_bytes:%d, bytes:%d ",ret,read_bytes,bytes);
        } else if (ret != -EAGAIN ) {
            ALOGE("%s:%d, pcm_read fail, ret:%#x, error info:%s", __func__, __LINE__, ret, strerror(errno));
            return ret;
        } else {
             usleep( (bytes - read_bytes) * 1000000 / audio_stream_in_frame_size(stream) /
                in->requested_rate / 2);
        }
    }
    return 0;
}
