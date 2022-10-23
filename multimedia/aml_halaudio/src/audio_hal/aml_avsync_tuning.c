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

#include "log.h"
#include <linux/ioctl.h>
#include <tinyalsa/asoundlib.h>
#include <aml_android_utils.h>

#include "audio_hw.h"
#include "audio_hw_utils.h"
#include "aml_audio_stream.h"
#include "aml_avsync_tuning.h"
#include "dolby_lib_api.h"


/**
 * store src_buffer data to spker_tuning_rbuf,
 * retrieve former saved data after latency requirement is met.
 */
int tuning_spker_latency(struct aml_audio_device *adev,
                         int16_t *sink_buffer, int16_t *src_buffer, size_t bytes)
{
    ring_buffer_t *rbuf = &(adev->spk_tuning_rbuf);
    uint latency_bytes = adev->spk_tuning_lvl;
    uint ringbuf_lvl = 0;
    uint ret = 0;

    if (!sink_buffer || !src_buffer) {
        ALOGE("%s(), NULL pointer!", __func__);
        return 0;
    }

    /* clear last sink buffers */
    memset((void*)sink_buffer, 0, bytes);
    /* get buffered data from ringbuf head */
    ringbuf_lvl = get_buffer_read_space(rbuf);
    if (ringbuf_lvl == latency_bytes) {
        if (ringbuf_lvl >= bytes) {
            ret = ring_buffer_read(rbuf, (unsigned char*)sink_buffer, bytes);
            if (ret != (uint)bytes) {
                ALOGE("%s(), ringbuf read fail.", __func__);
                goto err;
            }
            ret = ring_buffer_write(rbuf, (unsigned char*)src_buffer, bytes, UNCOVER_WRITE);
            if (ret != (uint)bytes) {
                ALOGE("%s(), ringbuf write fail.", __func__);
                goto err;
            }
        } else {
            ret = ring_buffer_write(rbuf, (unsigned char*)src_buffer, bytes, UNCOVER_WRITE);
            if (ret != (uint)bytes) {
                ALOGE("%s(), ringbuf write fail..", __func__);
                goto err;
            }
            ret = ring_buffer_read(rbuf, (unsigned char*)sink_buffer, bytes);
            if (ret != (uint)bytes) {
                ALOGE("%s(), ringbuf read fail..", __func__);
                goto err;
            }
        }
    } else if (ringbuf_lvl < latency_bytes) {
        if (ringbuf_lvl + bytes < latency_bytes) {
            /* all data need to store in tmp buf */
            ret = ring_buffer_write(rbuf, (unsigned char*)src_buffer, bytes, UNCOVER_WRITE);
            if (ret != (uint)bytes) {
                ALOGE("%s(), ringbuf write fail...", __func__);
                goto err;
            }
        } else {
            /* output former data, and store new */
            uint out_bytes = ringbuf_lvl + bytes - latency_bytes;
            uint offset = bytes - out_bytes;

            if (ringbuf_lvl >= out_bytes) {
                /* first fetch data and then store the new */
                ret = ring_buffer_read(rbuf, (unsigned char*)sink_buffer + offset, out_bytes);
                if (ret != (uint)out_bytes) {
                    ALOGE("%s(), ringbuf read fail...", __func__);
                    goto err;
                }
                ret = ring_buffer_write(rbuf, (unsigned char*)src_buffer, bytes, UNCOVER_WRITE);
                if (ret != (uint)bytes) {
                    ALOGE("%s(), ringbuf write fail...", __func__);
                    goto err;
                }
            } else {
                /* first store the data then fetch */
                ret = ring_buffer_write(rbuf, (unsigned char*)src_buffer, bytes, UNCOVER_WRITE);
                if (ret != (uint)bytes) {
                    ALOGE("%s(), ringbuf write fail", __func__);
                    goto err;
                }
                ret = ring_buffer_read(rbuf, (unsigned char*)sink_buffer + offset, out_bytes);
                if (ret != (uint)out_bytes) {
                    ALOGE("%s(), ringbuf read fail....", __func__);
                    goto err;
                }
            }
        }
    } else {
        ALOGE("%s(), abnormal case, CHECK data flow please!", __func__);
        goto err;
    }
    return 0;

err:
    memcpy(sink_buffer, src_buffer, bytes);
    return ret;
}

static int aml_dev_sample_audio_path_latency(struct aml_audio_device *aml_dev)
{
    struct aml_stream_in *in = aml_dev->active_input;
    struct aml_audio_patch *patch = aml_dev->audio_patch;
    int rbuf_ltcy = 0, spk_tuning_ltcy = 0, ms12_ltcy = 0, alsa_in_ltcy = 0;
    int alsa_out_i2s_ltcy = 0, alsa_out_spdif_ltcy = 0;
    int whole_path_ltcy = 0, in_path_ltcy = 0, out_path_ltcy = 0;
    snd_pcm_sframes_t frames = 0;
    int frame_size = 0;
    int ret = 0;

    frame_size = CHANNEL_CNT * audio_bytes_per_sample(AUDIO_FORMAT_PCM_16_BIT);
    if (patch) {
        size_t rbuf_avail = 0;

        rbuf_avail = get_buffer_read_space(&patch->aml_ringbuffer);
        frames = rbuf_avail / frame_size;
        rbuf_ltcy = frames / SAMPLE_RATE_MS;
        ALOGV("  audio ringbuf latency = %d", rbuf_ltcy);
    }

    if (aml_dev->spk_tuning_lvl) {
        size_t rbuf_avail = 0;

        rbuf_avail = get_buffer_read_space(&aml_dev->spk_tuning_rbuf);
        frames = rbuf_avail / frame_size;
        spk_tuning_ltcy = frames / SAMPLE_RATE_MS;
        ALOGV("  audio spk tuning latency = %d", spk_tuning_ltcy);
    }
#ifdef ENABLE_MS12
    if (eDolbyMS12Lib == aml_dev->dolby_lib_type) {
        if (aml_dev->ms12.dolby_ms12_enable == true) {
            int dolby_main_avail = dolby_ms12_get_main_buffer_avail();
            if (dolby_main_avail > 0) {
                ms12_ltcy = dolby_main_avail / frame_size / SAMPLE_RATE_MS;
            }
            ALOGV("  audio ms12 latency = %d", ms12_ltcy);
        }
    }
#endif
    if (aml_dev->pcm_handle[I2S_DEVICE]) {
        ret = pcm_ioctl(aml_dev->pcm_handle[I2S_DEVICE], SNDRV_PCM_IOCTL_DELAY, &frames);
        if (ret >= 0) {
            alsa_out_i2s_ltcy = frames / SAMPLE_RATE_MS;
        }
        ALOGV("  audio i2s latency = %d", alsa_out_i2s_ltcy);
    }

    if (aml_dev->pcm_handle[DIGITAL_DEVICE]) {
        ret = pcm_ioctl(aml_dev->pcm_handle[DIGITAL_DEVICE], SNDRV_PCM_IOCTL_DELAY, &frames);
        if (ret >= 0) {
            alsa_out_spdif_ltcy = frames / SAMPLE_RATE_MS;
        }
        ALOGV("  audio spdif latency = %d", alsa_out_spdif_ltcy);
    }

    if (in) {
        ret = pcm_ioctl(in->pcm, SNDRV_PCM_IOCTL_DELAY, &frames);
        if (ret >= 0) {
            alsa_in_ltcy = frames / SAMPLE_RATE_MS;
        }
        ALOGV("  audio alsa in latency = %d", alsa_in_ltcy);
    }

    /* calc whole path latency considering with format */
    if (patch) {
        int in_path_ltcy = alsa_in_ltcy + rbuf_ltcy + ms12_ltcy;
        int out_path_ltcy = 0;

        if (patch->aformat == AUDIO_FORMAT_E_AC3) {
            in_path_ltcy /= EAC3_MULTIPLIER;
        }

        if (aml_dev->sink_format == AUDIO_FORMAT_PCM_16_BIT) {
            out_path_ltcy = alsa_out_i2s_ltcy + spk_tuning_ltcy;
        } else if (aml_dev->sink_format == AUDIO_FORMAT_AC3) {
            out_path_ltcy = alsa_out_spdif_ltcy;
        } else if (aml_dev->sink_format == AUDIO_FORMAT_E_AC3) {
            out_path_ltcy = alsa_out_spdif_ltcy / EAC3_MULTIPLIER;
        }

        whole_path_ltcy = in_path_ltcy + out_path_ltcy;
    }

    return whole_path_ltcy;
}

static int aml_dev_sample_video_path_latency(int *vlatency)
{
    int vltcy = 0;

    vltcy = aml_sysfs_get_int("/sys/class/video/vframe_walk_delay");
    if (vltcy >= 250 || vltcy <= 0) {
        ALOGV("%s(), invalid video latency: %d", __func__, vltcy);
        *vlatency = 0;
        return -EINVAL;
    }

    *vlatency = vltcy;
    return 0;
}

static inline int calc_diff(int altcy, int vltcy)
{
    return altcy - vltcy;
}

static int aml_dev_avsync_diff_in_path(struct aml_audio_patch *patch, int *av_diff)
{
    struct aml_audio_device *aml_dev = NULL;
    int altcy = 0, vltcy = 0;
    int src_diff_err = 0;
    int ret = 0;

    if (!patch || !av_diff) {
        ret = -EINVAL;
        goto err;
    }

    aml_dev = (struct aml_audio_device *)patch->dev;

    altcy = aml_dev_sample_audio_path_latency(aml_dev);
    ret = aml_dev_sample_video_path_latency(&vltcy);
    if (ret < 0) {
        ret = -EINVAL;
    }

    if (patch->input_src == AUDIO_DEVICE_IN_LINE) {
        src_diff_err = -30;
    }
    ALOGV("%s(), altcy: %dms, vltcy: %dms", __func__, altcy, vltcy);
    *av_diff = altcy - vltcy + src_diff_err ;

    return 0;

err:
    return ret;
}

static inline void aml_dev_accumulate_avsync_diff(struct aml_audio_patch *patch, int av_diff)
{
    patch->av_diffs += av_diff;
    patch->avsync_sample_accumed++;
}

static int aml_dev_tune_video_path_latency(int tune_val)
{
    char tmp[32];
    int ret = 0;

    ALOGV("%s(): tuning video total latency: value %dms", __func__, tune_val);
    if (tune_val > 200 || tune_val < -200) {
        ALOGE("%s():  unsupport tuning value: %dms", __func__, tune_val);
        ret = -EINVAL;
        goto err;
    }

    if ((tune_val < 16 && tune_val >= 0) || (tune_val <= 0 && tune_val > -16)) {
        return 0;
    }

    memset(tmp, 0, 32);
    snprintf(tmp, 32, "%d", tune_val);
    ret = aml_sysfs_set_str("/sys/class/video/hdmin_delay_duration", tmp);
    if (ret < 0) {
        ALOGE("%s() fail,  err: %s", __func__, strerror(errno));
        goto err;
    }
    ret = aml_sysfs_set_str("/sys/class/video/hdmin_delay_start", "1");
    if (ret < 0) {
        ALOGE("%s() fail,  err: %s", __func__, strerror(errno));
        goto err;
    }

    return 0;
err:
    return ret;
}

static inline void aml_dev_avsync_reset(struct aml_audio_patch *patch)
{
    patch->avsync_sample_accumed = 0;
    patch->av_diffs = 0;
    patch->do_tune = 0;
}

int aml_dev_try_avsync(struct aml_audio_patch *patch)
{
    int av_diff = 0, factor = (patch->aformat == AUDIO_FORMAT_E_AC3) ? 2 : 1;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *)patch->dev;
    int ret = 0;

    if (!patch) {
        return 0;
    }

    /* if source unstable, reaccumulate avsync diff from start */
    if (!patch->is_src_stable) {
        aml_dev_avsync_reset(patch);
        return 0;
    }

    ret = aml_dev_avsync_diff_in_path(patch, &av_diff);
    if (ret < 0) {
        return 0;
    }

    /* To tune immediately */
    //if (av_diff > 16)
    //    patch->do_tune = 1;

    aml_dev_accumulate_avsync_diff(patch, av_diff);
    if (patch->avsync_sample_accumed >= patch->avsync_sample_max_cnt) {
        patch->do_tune = 1;
    }

    if (patch->do_tune) {
        int tune_val = patch->av_diffs / patch->avsync_sample_accumed;
        //tune_val += 50/factor;
        int user_tune_val = aml_audio_get_src_tune_latency(aml_dev->patch_src);
        ALOGV("%s(), av user tuning latency = %dms",
              __func__, user_tune_val);
        tune_val += user_tune_val;
        if (patch->input_src != AUDIO_DEVICE_IN_LINE) {
            ret = aml_dev_tune_video_path_latency(tune_val);
        }
        if (ret < 0) {
            ALOGE("%s() fail, err = %d", __func__, ret);
        }
        ALOGV("%s(): tuning video total latency: value %dms", __func__, tune_val);
        aml_dev_avsync_reset(patch);
    }
    return 0;
}

