/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description:
*/


#define LOG_TAG "audio_hw_subMixingFactory"
//#define LOG_NDEBUG 0
#define __USE_GNU

#include <errno.h>
#include <sched.h>
#include <cutils/log.h>
#include <system/audio.h>
#include <inttypes.h>
#include <aml_volume_utils.h>

#include "sub_mixing_factory.h"
#include "amlAudioMixer.h"
#include "audio_hw.h"
#include "audio_hw_utils.h"
#include "hw_avsync_callbacks.h"
#include "../libms12v2/include/aml_audio_ms12.h"
#include "dolby_lib_api.h"
#include "alsa_device_parser.h"

//#define DEBUG_TIME
static int on_notify_cbk(void *data);
static int on_input_avail_cbk(void *data);
static ssize_t out_write_subMixingPCM(struct audio_stream_out *stream,
                      const void *buffer,
                      size_t bytes);
static int out_pause_subMixingPCM(struct audio_stream_out *stream);
static int out_resume_subMixingPCM(struct audio_stream_out *stream);
static int out_flush_subMixingPCM(struct audio_stream_out *stream);

static int startMixingThread(struct subMixing *sm)
{
    return pcm_mixer_thread_run(sm->mixerData);
}

static int exitMixingThread(struct subMixing *sm)
{
    return pcm_mixer_thread_exit(sm->mixerData);
}

static int initSubMixngOutput(
        struct subMixing *sm,
        struct audioCfg cfg,
        struct aml_audio_device *adev)
{
    struct pcm *pcm = NULL;
    int card = alsa_device_get_card_index();
    int device = alsa_device_update_pcm_index(PORT_I2S, PLAYBACK);
    int res = 0;

    if (sm == NULL) {
        ALOGE("%s(), NULL pointer", __func__);
        return -EINVAL;
    }

    ALOGI("%s(), open ALSA hw:%d,%d", __func__, card, device);
    cfg.card = card;
    cfg.device = device;
    if (sm->type == MIXER_LPCM) {
        struct amlAudioMixer *amixer = NULL;
        amixer = newAmlAudioMixer(cfg, adev);
        if (amixer == NULL) {
            res = -ENOMEM;
            goto err;
        }
        sm->mixerData = amixer;
        startMixingThread(sm);
    } else if (sm->type == MIXER_MS12) {
        //TODO
        ALOGW("%s(), not support yet, in TODO list", __func__);
    } else {
        ALOGE("%s(), not support", __func__);
        res = -EINVAL;
        goto err;
    }
    return 0;
err:
    return res;
};

static int releaseSubMixingOutput(struct subMixing *sm)
{
    ALOGI("++%s()", __func__);
    if (!sm) {
        ALOGE("%s(), null pointer", __func__);
        return -EINVAL;
    }
    exitMixingThread(sm);
    freeAmlAudioMixer(sm->mixerData);

    return 0;
}

static ssize_t aml_out_write_to_mixer(struct audio_stream_out *stream, const void* buffer,
                                    size_t bytes)
{
    struct aml_stream_out *out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = out->dev;
    struct subMixing *sm = adev->sm;
    struct amlAudioMixer *audio_mixer = sm->mixerData;
    const char *data = (char *)buffer;
    size_t written_total = 0, frame_size = 4;
    uint32_t latency_frames = 0;
    struct timespec ts;

    do {
        ssize_t written = 0;
        ALOGV("%s(), stream usecase: %s, written_total %d, bytes %d",
            __func__,  usecase_to_str(out->usecase), written_total, bytes);

        written = mixer_write_inport(audio_mixer,
                out->port_index, data, bytes - written_total);
        if (written < 0) {
            ALOGE("%s(), write failed, errno = %d", __func__, written);
            return written;
        }

        if (written > 0) {
            written_total += written;
            data += written;
            //latency_frames = mixer_get_inport_latency_frames(audio_mixer, out->port_index) +
             //       mixer_get_outport_latency_frames(audio_mixer);
            //pthread_mutex_lock(&out->lock);
            //clock_gettime(CLOCK_MONOTONIC, &out->timestamp);
            //out->last_frames_postion += written / frame_size - latency_frames;
            //pthread_mutex_unlock(&out->lock);
        }
        ALOGV("%s(), portindex(%d) written(%d), written_total(%d), bytes(%d)",
            __func__, out->port_index, written, written_total, bytes);

        if (written_total >= bytes) {
            ALOGV("%s(), exit", __func__);
            break;
        }

        //usleep((bytes- written_total) * 1000 / 5 / 48);
        //if (out->port_index == 1) {
            ts_wait_time_us(&ts, 5000);
            ALOGV("-%s() wait....", __func__);
            pthread_mutex_lock(&out->cond_lock);
            pthread_cond_timedwait(&out->cond, &out->cond_lock, &ts);
            ALOGV("--%s() wait wakeup", __func__);
            pthread_mutex_unlock(&out->cond_lock);
        //}
    } while (!out->exiting);

    return written_total;
}

static int consume_meta_data(void *cookie,
        uint32_t frame_size, int64_t pts, uint64_t offset)
{
    struct aml_stream_out *out = (struct aml_stream_out *)cookie;
    struct aml_audio_device *adev = out->dev;
    //struct aml_audio_mixer *audio_mixer = adev->audio_mixer;
    struct subMixing *sm = adev->sm;
    struct amlAudioMixer *audio_mixer = sm->mixerData;
    struct meta_data_list *mdata_list = calloc(1, sizeof(struct meta_data_list));

    if (!mdata_list) {
        ALOGE("%s(), no memory", __func__);
        return -ENOMEM;
    }
    if (out->pause_status) {
        ALOGE("%s(), write in pause status", __func__);
    }

    mdata_list->mdata.frame_size = frame_size;
    mdata_list->mdata.pts = pts;
    mdata_list->mdata.payload_offset = offset;

    if (out->debug_stream) {
        ALOGD("%s(), frame_size %d, pts %lldms, payload offset %lld",
                __func__, frame_size, pts/1000000, offset);
    }
    if (get_mixer_hwsync_frame_size(audio_mixer) != frame_size) {
        ALOGV("%s(), resize frame_size %d", __func__, frame_size);
        set_mixer_hwsync_frame_size(audio_mixer, frame_size);
    }
    pthread_mutex_lock(&out->mdata_lock);
    list_add_tail(&out->mdata_list, &mdata_list->list);
    pthread_mutex_unlock(&out->mdata_lock);
    return 0;
}

static int consume_output_data(void *cookie, const void* buffer, size_t bytes)
{
    ssize_t written = 0;
    uint64_t latency_frames = 0;
    struct audio_stream_out *stream = (struct audio_stream_out *)cookie;
    struct aml_stream_out *out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = out->dev;
    struct subMixing *sm = adev->sm;
    struct amlAudioMixer *audio_mixer = sm->mixerData;
    uint16_t *in_buf_16 = (uint16_t *)buffer;
    struct timespec tval, new_tval;
    uint64_t us_since_last_write = 0;
    int64_t throttle_timeus = 0;
    int frame_size = 4;
    void * out_buf = (void*)buffer;
    size_t out_size = bytes;
    int bResample = 0;

    ALOGV("++%s(), bytes = %d", __func__, bytes);
    if (out->pause_status) {
        ALOGE("%s(), write in pause status", __func__);
    }

    clock_gettime(CLOCK_MONOTONIC, &tval);
    apply_volume(out->volume_l, in_buf_16, sizeof(uint16_t), bytes);
    if (out->hw_sync_mode && out->resample_handle != NULL) {
        int ret;
        ret = aml_audio_resample_process(out->resample_handle, in_buf_16, bytes);
        if (ret < 0) {
            ALOGE("resample process error\n");
            written = -1;
            goto exit;
        }
        out_buf = out->resample_handle->resample_buffer;
        out_size = out->resample_handle->resample_size;
        bResample = 1;
    }
    written = aml_out_write_to_mixer(stream, out_buf, out_size);
    if (written < 0) {
        ALOGE("%s(), written failed, %d", __func__, written);
        goto exit;
    }

    /*here may be a problem, after resample, the size write to mixer is changed,
      to avoid some problem, we assume it is totally wirtten.
    */
    if (bResample) {
        written = bytes;
    }

    clock_gettime(CLOCK_MONOTONIC, &new_tval);
    us_since_last_write = (new_tval.tv_sec - out->timestamp.tv_sec) * 1000000 +
            (new_tval.tv_nsec - out->timestamp.tv_nsec) / 1000;
    //out->timestamp = new_tval;

    int used_this_write = (new_tval.tv_sec - tval.tv_sec) * 1000000 +
            (new_tval.tv_nsec - tval.tv_nsec) / 1000;
    int target_us = bytes * 1000 / 4 / 48;
    // calculate presentation frames and timestamps
    //clock_gettime(CLOCK_MONOTONIC, &out->timestamp);
    //latency_frames = mixer_get_inport_latency_frames(audio_mixer, out->port_index) +
    //    mixer_get_outport_latency_frames(audio_mixer);
    //latency_frames = mixer_get_inport_latency_frames(audio_mixer, out->port_index);
    //out->frame_write_sum += written;

    //if (out->last_frames_postion > out->frame_write_sum)
    //    out->last_frames_postion = out->frame_write_sum - latency_frames;
    //else
    //    out->last_frames_postion = out->frame_write_sum;
    ALOGV("++%s(), written = %d", __func__, written);
    if (getprop_bool("media.audiohal.hwsync")) {
        aml_audio_dump_audio_bitstreams("/data/audio/consumeout.raw", buffer, written);
    }
    if (0) {
        ALOGD("%s(), last_frames_postion(%lld) latency_frames(%lld)",
            __func__, out->last_frames_postion, latency_frames);
    }
    throttle_timeus = target_us - us_since_last_write;
    if (throttle_timeus > 0 && throttle_timeus < 200000) {
        ALOGV("throttle time %lld us", throttle_timeus);
        if (throttle_timeus > 1000)
            usleep(throttle_timeus - 1000);
    }

    //throttle simply 4/5 duration
    //usleep(bytes * 1000 / 4 / 48 * 1 / 2);
exit:
    clock_gettime(CLOCK_MONOTONIC, &out->timestamp);
    out->lasttimestamp.tv_sec = out->timestamp.tv_sec;
    out->lasttimestamp.tv_nsec = out->timestamp.tv_nsec;
    if (written >= 0) {
        //TODO
        latency_frames = mixer_get_inport_latency_frames(audio_mixer, out->port_index);
                + mixer_get_outport_latency_frames(audio_mixer);
        out->frame_write_sum += written / frame_size;

        if (out->frame_write_sum > latency_frames)
            out->last_frames_postion = out->frame_write_sum - latency_frames;
        else
            out->last_frames_postion = 0;//out->frame_write_sum;
    }
    if (out->debug_stream) {
        ALOGD("%s(), frames sum %lld, last frames %lld", __func__, out->frame_write_sum, out->last_frames_postion);
    }
    return written;
}

static int set_thread_affinity(void)
{
    cpu_set_t cpuSet;
    int sastat = 0;

    CPU_ZERO(&cpuSet);
    CPU_SET(2, &cpuSet);
    CPU_SET(3, &cpuSet);
    sastat = sched_setaffinity(0, sizeof(cpu_set_t), &cpuSet);
    if (sastat) {
        ALOGW("%s(), failed to set cpu affinity", __FUNCTION__);
        return sastat;
    }

    return 0;
}

static ssize_t out_write_hwsync_lpcm(struct audio_stream_out *stream, const void* buffer,
                                    size_t bytes)
{
    struct aml_stream_out *out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = out->dev;
    struct subMixing *sm = adev->sm;
    size_t channel_count = audio_channel_count_from_out_mask(out->hal_channel_mask);
    size_t frame_size = audio_bytes_per_frame(channel_count, out->hal_format);;
    int written_total = 0;
    struct timespec ts;
    memset(&ts, 0, sizeof(struct timespec));

    ALOGV("++%s() out(%p)", __func__, out);
    if (!is_stream_using_mixer(out)) {
        ALOGE("%s(), stream must be mixing stream", __func__);
        return -EINVAL;
    }

    if (out->standby) {
        ALOGI("%s(), start hwsync lpcm stream: %p", __func__, out);
        set_thread_affinity();
        out->hwsync_extractor = new_hw_avsync_header_extractor(consume_meta_data,
                consume_output_data, out);
        out->first_pts_set = false;
        pthread_mutex_init(&out->mdata_lock, NULL);
        list_init(&out->mdata_list);
        init_mixer_input_port(sm->mixerData, &out->audioCfg, out->flags,
            on_notify_cbk, out, on_input_avail_cbk, out,
            on_meta_data_cbk, out, out->volume_l);
        out->port_index = get_input_port_index(&out->audioCfg, out->flags);
        ALOGI("%s(), hwsync port index = %d", __func__, out->port_index);
        out->standby = false;
        mixer_set_continuous_output(sm->mixerData, false);
    }
    if (out->pause_status) {
        ALOGW("%s(), write in pause status!!", __func__);
        out->pause_status = false;
    }
    written_total = header_extractor_write(out->hwsync_extractor, buffer, bytes);
    ALOGV("%s() bytes %d, out->last_frames_postion %lld frame_sum %lld",
            __func__, bytes, out->last_frames_postion, out->frame_write_sum);

    if (getprop_bool("media.audiohal.hwsync")) {
        aml_audio_dump_audio_bitstreams("/data/audio/audiomain.raw", buffer, written_total);
    }

    if (written_total > 0) {
        ALOGV("--%s(), out(%p)written %d, write_sum after %lld",
                __func__, out, written_total, out->frame_write_sum);
        if ((size_t)written_total != bytes)
            ALOGE("--%s(), written %d, but bytes = %d", __func__, written_total, bytes);
        return written_total;
    } else {
        ALOGE("--%s(), written %d, but return bytes", __func__, written_total);
        //return 1;
        return bytes;
    }
    return written_total;
}

static ssize_t out_write_system(struct audio_stream_out *stream, const void *buffer,
                                    size_t bytes)
{
    struct aml_stream_out *out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = out->dev;
    //struct aml_audio_mixer *audio_mixer = adev->audio_mixer;
    struct subMixing *sm = adev->sm;
    struct amlAudioMixer *audio_mixer = sm->mixerData;
    struct timespec tval, new_tval;
    uint64_t us_since_last_write = 0;
    //uint64_t begain_time, end_time;
    ssize_t written = 0;
    size_t remain = 0;
    size_t channel_count = audio_channel_count_from_out_mask(out->hal_channel_mask);
    size_t frame_size = audio_bytes_per_frame(channel_count, out->hal_format);;
    //uint64_t throttle_timeus = THROTLE_TIME_US;//aml_audio_get_throttle_timeus();
    int64_t throttle_timeus = 0;//aml_audio_get_throttle_timeus(bytes);

    if (out->standby) {
        ALOGI("%s(), standby to unstandby", __func__);
        out->standby = false;
    }

    if (bytes == 0) {
        ALOGW("%s(), inval to write bytes 0", __func__);
        usleep(512 * 1000 / 48 / frame_size);
        written = 0;
        goto exit;
        //return 0;
    }

    clock_gettime(CLOCK_MONOTONIC, &tval);
    //begain_time = get_systime_ns();
    written = aml_out_write_to_mixer(stream, buffer, bytes);
    if (written >= 0) {
        remain = bytes - written;
        out->frame_write_sum += written / frame_size;
        if (remain > 0) {
            ALOGE("INVALID partial written");
        }
        clock_gettime(CLOCK_MONOTONIC, &new_tval);
        if (tval.tv_sec > new_tval.tv_sec)
            ALOGE("%s(), FATAL ERROR", __func__);
        ALOGV("++%s() bytes %d, out->port_index %d", __func__, bytes, out->port_index);
        //ALOGD(" %lld us, %lld", new_tval.tv_sec, tval.tv_sec);

        us_since_last_write = (new_tval.tv_sec - out->timestamp.tv_sec) * 1000000 +
                (new_tval.tv_nsec - out->timestamp.tv_nsec) / 1000;
        //out->timestamp = new_tval;

        int used_this_write = (new_tval.tv_sec - tval.tv_sec) * 1000000 +
                (new_tval.tv_nsec - tval.tv_nsec) / 1000;
        int target_us = bytes * 1000 / frame_size / 48;

        ALOGV("time spent on write %lld us, written %d", us_since_last_write, written);
        ALOGV("used_this_write %d us, target %d us", used_this_write, target_us);
        throttle_timeus = target_us - us_since_last_write;
        if (throttle_timeus > 0 && throttle_timeus < 200000) {
            ALOGV("throttle time %lld us", throttle_timeus);
            if (throttle_timeus > 1800) {
                //usleep(throttle_timeus - 1800);
                ALOGV("actual throttle %lld us, since last %lld us",
                        throttle_timeus, us_since_last_write);
            } else {
                ALOGV("%lld us, but un-throttle", throttle_timeus);
            }
        } else if (throttle_timeus != 0) {
            // first time write, sleep
            //usleep(target_us - 100);
            ALOGV("invalid throttle time %lld us, us since last %lld us", throttle_timeus, us_since_last_write);
            ALOGV("\n\n");
        }
    } else {
        ALOGE("%s(), write fail, err = %d", __func__, written);
    }

    // TODO: means first write, need check this by method
    if (us_since_last_write > 500000) {
        usleep(bytes * 1000 / 48 / frame_size);
        ALOGV("%s(), invalid duration %llu us", __func__, us_since_last_write);
        //ALOGE("last   write %ld s,  %ld ms", out->timestamp.tv_sec, out->timestamp.tv_nsec/1000000);
        //ALOGE("before write %ld s,  %ld ms", tval.tv_sec, tval.tv_nsec/1000000);
        //ALOGE("after  write %ld s,  %ld ms", new_tval.tv_sec, new_tval.tv_nsec/1000000);
    }

exit:
    // update new timestamp
    clock_gettime(CLOCK_MONOTONIC, &out->timestamp);
    out->lasttimestamp.tv_sec = out->timestamp.tv_sec;
    out->lasttimestamp.tv_nsec = out->timestamp.tv_nsec;
    if (written >= 0) {
        uint32_t latency_frames = mixer_get_inport_latency_frames(audio_mixer, out->port_index);
                //+ mixer_get_outport_latency_frames(audio_mixer);
        if (out->frame_write_sum > latency_frames)
            out->last_frames_postion = out->frame_write_sum - latency_frames;
        else
            out->last_frames_postion = out->frame_write_sum;

        if (0) {
            ALOGI("last position %lld, latency_frames %d", out->last_frames_postion, latency_frames);
        }
    }

    return written;
}

static ssize_t out_write_direct_pcm(struct audio_stream_out *stream, const void *buffer,
                                    size_t bytes)
{
    struct aml_stream_out *out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = out->dev;
    struct subMixing *sm = adev->sm;
    struct amlAudioMixer *audio_mixer = sm->mixerData;
    struct timespec tval, new_tval;
    uint64_t us_since_last_write = 0;
    //uint64_t begain_time, end_time;
    ssize_t written = 0;
    size_t remain = 0;
    int frame_size = 4;
    int64_t throttle_timeus = 0;//aml_audio_get_throttle_timeus(bytes);

    if (out->standby) {
        ALOGI("%s(), direct %p", __func__, out);
        init_mixer_input_port(sm->mixerData, &out->audioCfg, out->flags,
            on_notify_cbk, out, on_input_avail_cbk, out,
            NULL, NULL, 1.0);
        out->port_index = get_input_port_index(&out->audioCfg, out->flags);
        ALOGI("%s(), direct port index = %d", __func__, out->port_index);
        out->standby = false;
    }

    if (bytes == 0) {
        ALOGW("%s(), inval to write bytes 0", __func__);
        usleep(512 * 1000 / 48 / frame_size);
        written = 0;
        goto exit;
        //return 0;
    }

    clock_gettime(CLOCK_MONOTONIC, &tval);
    //begain_time = get_systime_ns();
    written = aml_out_write_to_mixer(stream, buffer, bytes);
    if (written >= 0) {
        remain = bytes - written;
        out->frame_write_sum += written / frame_size;
        if (remain > 0) {
            ALOGE("INVALID partial written");
        }
        clock_gettime(CLOCK_MONOTONIC, &new_tval);
        if (tval.tv_sec > new_tval.tv_sec)
            ALOGE("%s(), FATAL ERROR", __func__);
        ALOGV("++%s() bytes %d, out->port_index %d", __func__, bytes, out->port_index);
        //ALOGD(" %lld us, %lld", new_tval.tv_sec, tval.tv_sec);

        us_since_last_write = (new_tval.tv_sec - out->timestamp.tv_sec) * 1000000 +
                (new_tval.tv_nsec - out->timestamp.tv_nsec) / 1000;
        //out->timestamp = new_tval;

        int used_this_write = (new_tval.tv_sec - tval.tv_sec) * 1000000 +
                (new_tval.tv_nsec - tval.tv_nsec) / 1000;
        int target_us = bytes * 1000 / frame_size / 48;

        ALOGV("time spent on write %lld us, written %d", us_since_last_write, written);
        ALOGV("used_this_write %d us, target %d us", used_this_write, target_us);
        throttle_timeus = target_us - us_since_last_write;
        if (throttle_timeus > 0 && throttle_timeus < 200000) {
            ALOGV("throttle time %lld us", throttle_timeus);
            if (throttle_timeus > 1800) {
                usleep(throttle_timeus - 1800);
                ALOGV("actual throttle %lld us, since last %lld us",
                        throttle_timeus, us_since_last_write);
            } else {
                ALOGV("%lld us, but un-throttle", throttle_timeus);
            }
        } else if (throttle_timeus != 0) {
            // first time write, sleep
            //usleep(target_us - 100);
            ALOGV("invalid throttle time %lld us, us since last %lld us", throttle_timeus, us_since_last_write);
            ALOGV("\n\n");
        }
    } else {
        ALOGE("%s(), write fail, err = %d", __func__, written);
    }

    // TODO: means first write, need check this by method
    if (us_since_last_write > 500000) {
        usleep(bytes * 1000 / 48 / frame_size);
        ALOGV("%s(), invalid duration %llu us", __func__, us_since_last_write);
        //ALOGE("last   write %ld s,  %ld ms", out->timestamp.tv_sec, out->timestamp.tv_nsec/1000000);
        //ALOGE("before write %ld s,  %ld ms", tval.tv_sec, tval.tv_nsec/1000000);
        //ALOGE("after  write %ld s,  %ld ms", new_tval.tv_sec, new_tval.tv_nsec/1000000);
    }

exit:
    // update new timestamp
    clock_gettime(CLOCK_MONOTONIC, &out->timestamp);
    out->lasttimestamp.tv_sec = out->timestamp.tv_sec;
    out->lasttimestamp.tv_nsec = out->timestamp.tv_nsec;
    if (written >= 0) {
        uint32_t latency_frames = mixer_get_inport_latency_frames(audio_mixer, out->port_index);
                //+ mixer_get_outport_latency_frames(audio_mixer);
        if (out->frame_write_sum > latency_frames)
            out->last_frames_postion = out->frame_write_sum - latency_frames;
        else
            out->last_frames_postion = out->frame_write_sum;

        if (0) {
            ALOGI("last position %lld, latency_frames %d", out->last_frames_postion, latency_frames);
        }
    }

    return written;
}

static int on_notify_cbk(void *data)
{
    struct aml_stream_out *out = data;
    ALOGD("%s(), line %d", __func__, __LINE__);
    pthread_cond_broadcast(&out->cond);
    return 0;
}

static int on_input_avail_cbk(void *data)
{
    struct aml_stream_out *out = data;
    ALOGV("%s(), line %d", __func__, __LINE__);
    //if (out->pause_status) {
    //    ALOGI("%s(), in pause status, exit", __func__);
    //    return 0;
    //}
    pthread_cond_broadcast(&out->cond);
    return 0;
}

static int out_get_presentation_position_port(
        const struct audio_stream_out *stream,
        uint64_t *frames,
        struct timespec *timestamp)
{
    struct aml_stream_out *out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = out->dev;
    struct subMixing *sm = adev->sm;
    struct amlAudioMixer *audio_mixer = sm->mixerData;
    uint64_t frames_written_hw = out->last_frames_postion;
    int ret = 0;

    if (!frames || !timestamp) {
        return -EINVAL;
    }

    if (!adev->audio_patching) {
        ret = mixer_get_presentation_position(audio_mixer,
                out->port_index, frames, timestamp);
        if (ret == 0) {
            out->last_frames_postion = *frames;
        } else {
            ALOGV("%s(), pts not valid yet", __func__);
        }
    } else {
        *frames = frames_written_hw;
        *timestamp = out->timestamp;
    }
    ALOGV("%s() out:%p frames:%"PRIu64", sec:%ld, nanosec:%ld, ret:%d\n",
            __func__, out, *frames, timestamp->tv_sec, timestamp->tv_nsec, ret);
    return ret;
}

static int initSubMixingInputPcm(
        struct audio_config *config,
        struct aml_stream_out *out)
{
    struct aml_audio_device *adev = out->dev;
    struct subMixing *sm = adev->sm;
    bool hwsync_lpcm = false;
    int flags = out->flags;
    int channel_count = popcount(config->channel_mask);

    hwsync_lpcm = (flags & AUDIO_OUTPUT_FLAG_HW_AV_SYNC && config->sample_rate <= 48000 &&
               audio_is_linear_pcm(config->format) && channel_count <= 2);
    ALOGI("++%s(), out %p, flags %#x, hwsync lpcm %d, out format %#x",
            __func__, out, flags, hwsync_lpcm, sm->outputCfg.format);
    out->audioCfg = *config;
    out->stream.write = out_write_subMixingPCM;
    out->stream.pause = out_pause_subMixingPCM;
    out->stream.resume = out_resume_subMixingPCM;
    out->stream.flush = out_flush_subMixingPCM;
    out->stream.common.standby = out_standby_subMixingPCM;
    if (flags & AUDIO_OUTPUT_FLAG_PRIMARY) {
        /* using subMixing clac function for system sound */
        ALOGI("%s(), primary presentation", __func__);
        out->stream.get_presentation_position = out_get_presentation_position_port;
    }
    list_init(&out->mdata_list);
    if (hwsync_lpcm) {
        ALOGI("%s(), lpcm case", __func__);
        mixer_set_continuous_output(sm->mixerData, true);
    }
    return 0;
}

static int deleteSubMixingInputPcm(struct aml_stream_out *out)
{
    struct aml_audio_device *adev = out->dev;
    struct subMixing *sm = adev->sm;
    struct amlAudioMixer *audio_mixer = sm->mixerData;
    struct audio_config *config = &out->audioCfg;
    bool hwsync_lpcm = false;
    int flags = out->flags;
    int channel_count = popcount(config->channel_mask);

    hwsync_lpcm = (flags & AUDIO_OUTPUT_FLAG_HW_AV_SYNC && config->sample_rate <= 48000 &&
               audio_is_linear_pcm(config->format) && channel_count <= 2);

    ALOGI("%s(), cnt_stream_using_mixer %d",
            __func__, sm->cnt_stream_using_mixer);
    //delete_mixer_input_port(audio_mixer, out->port_index);
    if (hwsync_lpcm) {
        ALOGI("%s(), lpcm case", __func__);
        mixer_set_continuous_output(sm->mixerData, false);
    }
    return 0;
}

int initSubMixingInput(struct aml_stream_out *out,
        struct audio_config *config)
{
    struct aml_audio_device *adev = out->dev;
    struct subMixing *sm = adev->sm;
    int ret = 0;

    if (out == NULL || config == NULL) {
        ALOGE("%s(), null pointer", __func__);
        return -EINVAL;
    }
    ALOGI("++%s()", __func__);

    if (sm->type == MIXER_LPCM) {
        ret = initSubMixingInputPcm(config, out);
    } else if (sm->type == MIXER_MS12) {
        //ret = initSubMixingInputMS12(sm, config, out);
        ALOGE("%s(), MS12 not supported yet", __func__);
        ret = -1;
    }

    return ret;
};

int deleteSubMixingInput(struct aml_stream_out *out)
{
    struct aml_audio_device *adev = out->dev;
    struct subMixing *sm = adev->sm;
    int ret = 0;

    ALOGI("++%s()", __func__);
    if (out == NULL) {
        ALOGE("%s(), null pointer", __func__);
        return -EINVAL;
    }

    if (sm->type == MIXER_LPCM) {
        ret = deleteSubMixingInputPcm(out);
    } else if (sm->type == MIXER_MS12) {
        //ret = deleteSubMixingInputMS12(out);
        ALOGE("%s(), MS12 not supported yet", __func__);
        ret = -1;
    }

    return ret;
}

#if 0
int subMixerWriteInport(
        struct subMixing *sm,
        void *buf,
        size_t bytes,
        enum MIXER_INPUT_PORT port_index)
{
    int ret = 0;
    if (sm->type == MIXER_LPCM) {
        ret = mixer_write_inport(sm->mixerData, port_index, buf, bytes);
    } else {
        ret = -EINVAL;
        ALOGE("not support");
    }

    return 0;
};

int mainWriteMS12(
            struct subMixing *sm,
            void *buf,
            size_t bytes)
{
    (void *)buf;
    (void *)bytes;
    struct dolby_ms12_desc *ms12 = (struct dolby_ms12_desc *)sm->mixerData;
    return 0;
}

int sysWriteMS12(
            struct subMixing *sm,
            void *buf,
            size_t bytes)
{
    (void *)buf;
    (void *)bytes;
    struct dolby_ms12_desc *ms12 = (struct dolby_ms12_desc *)sm->mixerData;
    return 0;
}
#endif
/* mixing only support pcm 16&32 format now */
static int newSubMixingFactory(
            struct subMixing **smixer,
            enum MIXER_TYPE type,
            struct audioCfg cfg,
            void *data)
{
    (void *)data;
    struct subMixing *sm = NULL;
    int res = 0;

    ALOGI("%s(), type %d", __func__, type);
    /* check output config */
    if ((cfg.channelCnt != 2 && cfg.channelCnt != 8) || cfg.sampleRate != 48000
        || ((cfg.format != AUDIO_FORMAT_PCM_16_BIT) && (cfg.format != AUDIO_FORMAT_PCM_32_BIT))) {
        ALOGE("%s(), unsupport config, chCnt %d, rate %d, fmt %#x",
                __func__, cfg.channelCnt, cfg.sampleRate, cfg.format);
        res = -EINVAL;
        goto exit;
    }
    sm = calloc(1, sizeof(struct subMixing));
    if (sm == NULL) {
        ALOGE("%s(), No mem!", __func__);
        res = -ENOMEM;
        goto exit;
    }

    switch (type) {
    case MIXER_LPCM:
        sm->type = MIXER_LPCM;
        strncpy(sm->name, "LPCM", 16);
        sm->outputCfg = cfg;
        //sm->writeMain = mainWritePCM;
        //sm->writeSys = sysWritePCM;
        //sm->mixerData = data;
        break;
    case MIXER_MS12:
        sm->type = MIXER_MS12;
        strncpy(sm->name, "MS12", 16);
        sm->outputCfg = cfg;
        //sm->writeMain = mainWriteMS12;
        //sm->writeSys = sysWriteMS12;
        //sm->mixerData = data;
        break;
    default:
        ALOGE("%s(), type %d not support!", __func__, type);
        break;
    };

    *smixer = sm;
exit:
    return res;
};

static void deleteSubMixing(struct subMixing *sm)
{
    ALOGI("++%s()", __func__);
    if (sm != NULL) {
        free(sm);
    }
}

static int initAudioConfig(struct audioCfg *cfg, bool isTV)
{
    if (cfg == NULL) {
        ALOGE("%s(), NULL pointer", __func__);
        return -EINVAL;
    }

    cfg->sampleRate = 48000;
    if (isTV) {
        cfg->format = AUDIO_FORMAT_PCM_32_BIT;
        cfg->channelCnt = 8;
    } else {
        cfg->format = AUDIO_FORMAT_PCM_16_BIT;
        cfg->channelCnt = 2;
    }
    cfg->frame_size = cfg->channelCnt * audio_bytes_per_sample(cfg->format);
    return 0;
}

int initHalSubMixing(struct subMixing **smixer,
        enum MIXER_TYPE type,
        struct aml_audio_device *adev,
        bool isTV)
{
    struct audioCfg outCfg;
    int ret = 0;

    ALOGI("%s(), type %d, isTV %d", __func__, type, isTV);
    if (smixer == NULL) {
        ALOGE("%s(), NULL pointer", __func__);
    }
    initAudioConfig(&outCfg, isTV);
    ret = newSubMixingFactory(smixer, type, outCfg, NULL);
    if (ret < 0) {
        ALOGE("%s(), fail to new mixer", __func__);
        goto err;
    }
    ret = initSubMixngOutput(*smixer, outCfg, adev);
    if (ret < 0) {
        ALOGE("%s(), fail to init mixer", __func__);
        goto err1;
    }
    return 0;
err1:
    deleteSubMixing(*smixer);
err:
    return ret;
}

int deleteHalSubMixing(struct subMixing *smixer)
{
    releaseSubMixingOutput(smixer);
    deleteSubMixing(smixer);
    return 0;
}
#if 0
int mainWritePCM(
        struct subMixing *mixer,
        void *buf,
        size_t bytes)
{
    (void *)buf;
    (void *)bytes;
    struct amlAudioMixer *am = (struct amlAudioMixer *)mixer->mixerData;
    return 0;
}

int sysWritePCM(
        struct subMixing *mixer,
        void *buf,
        size_t bytes)
{
    (void *)buf;
    (void *)bytes;
    struct amlAudioMixer *am = (struct amlAudioMixer *)mixer->mixerData;
    return 0;
}

int subWrite(
        struct subMixing *mixer,
        void *buf,
        size_t bytes)
{
    (void *)buf;
    (void *)bytes;
    struct amlAudioMixer *am = (struct amlAudioMixer *)mixer->mixerData;
    return 0;
}
#endif

int outSubMixingWrite(
            struct audio_stream_out *stream,
            const void *buf,
            size_t bytes)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = aml_out->dev;
    struct subMixing *sm = adev->sm;
    struct amlAudioMixer *audio_mixer = sm->mixerData;
    char *buffer = (char *)buf;

    sm->write(sm, buffer, bytes);
    return 0;
}
ssize_t mixer_main_buffer_write_sm (struct audio_stream_out *stream, const void *buffer,
                                 size_t bytes)
{
    ALOGV ("%s write in %zu!\n", __FUNCTION__, bytes);
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = aml_out->dev;
    struct aml_stream_out *ms12_out = (struct aml_stream_out *)adev->ms12_out;
    struct dolby_ms12_desc *ms12 = &(adev->ms12);
    //struct aml_audio_patch *patch = adev->audio_patch;
    int case_cnt;
    int ret = -1;
    void *output_buffer = NULL;
    size_t output_buffer_bytes = 0;
    bool need_reconfig_output = false;
    void   *write_buf = NULL;
    ssize_t  write_bytes = 0;
    size_t  hwsync_cost_bytes = 0;
    int total_write = 0;
    size_t used_size = 0;
    int write_retry = 0;
    audio_hwsync_t *hw_sync = aml_out->hwsync;

    if (adev->debug_flag) {
        ALOGI("%s write in %zu!, format = 0x%x\n", __FUNCTION__, bytes, aml_out->hal_internal_format);
    }
    int return_bytes = bytes;

    if (buffer == NULL) {
        ALOGE ("%s() invalid buffer %p\n", __FUNCTION__, buffer);
        return -1;
    }

    case_cnt = popcount(adev->usecase_masks & 0xfffffffe);
    if (case_cnt > 1) {
        ALOGE ("%s usemask %x,we do not support two direct stream output at the same time.TO CHECK CODE FLOW!!!!!!",__func__,adev->usecase_masks);
        return return_bytes;
    }

    /* handle HWSYNC audio data*/
    if (aml_out->hw_sync_mode) {
        ALOGV("hwsync mode %s", __func__);
        write_bytes = out_write_hwsync_lpcm(stream, buffer, bytes);
    } else {
        ALOGV("direct %s", __func__);
        write_bytes = out_write_direct_pcm(stream, buffer, bytes);
        if (write_bytes < 0) {
            ALOGE("%s(), write failed, err = %d", __func__, write_bytes);
        }
        //write_buf = (void *)buffer;
        //write_bytes = bytes;
    }

    aml_out->input_bytes_size += write_bytes;

    if (adev->debug_flag) {
        ALOGI("%s return %zu!\n", __FUNCTION__, bytes);
    }
    return return_bytes;
}

static const struct pcm_config config_bt = {
    .channels = 1,
    .rate = VX_NB_SAMPLING_RATE,
    .period_size = 256,
    .period_count = PLAYBACK_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

static int open_btSCO_device(struct aml_audio_device *adev, size_t frames)
{
    struct aml_bt_output *bt = &adev->bt_output;
    unsigned int card = adev->card;
    unsigned int port = PORT_PCM;
    struct pcm *pcm = NULL;
    struct pcm_config cfg;
    size_t resample_in_frames = 0;
    size_t output_frames = 0;
    int ret = 0;

    /* check to update port */
    port = alsa_device_update_pcm_index(port, PLAYBACK);
    ALOGD("%s(), open card(%d) port(%d)", __func__, card, port);
    cfg = config_bt;
    pcm = pcm_open(card, port, PCM_OUT, &cfg);
    if (!pcm_is_ready(pcm)) {
        ALOGE("%s() cannot open pcm_out: %s, card %d, device %d",
                __func__, pcm_get_error(pcm), card, port);
        pcm_close (pcm);
        ret = -ENOENT;
        goto err;
    }

    bt->pcm_bt = pcm;
    ret = create_resampler(MM_FULL_POWER_SAMPLING_RATE,
                            VX_NB_SAMPLING_RATE,
                            config_bt.channels,
                            RESAMPLER_QUALITY_DEFAULT,
                            NULL,
                            &bt->resampler);
    if (ret != 0) {
        ALOGE("cannot create resampler for bt");
        goto err_res;
    }

    output_frames = frames * VX_NB_SAMPLING_RATE / MM_FULL_POWER_SAMPLING_RATE + 1;
    bt->bt_out_buffer = calloc(1, output_frames * 2);
    if (bt->bt_out_buffer == NULL) {
        ALOGE ("cannot malloc memory for bt_out_buffer");
        ret = -ENOMEM;
        goto err_out_buf;
    }
    bt->bt_out_frames = 0;

    bt->resampler_buffer = calloc(1, frames * 2);
    if (bt->resampler_buffer == NULL) {
        ALOGE ("cannot malloc memory for resampler_buffer");
        ret = -ENOMEM;
        goto err_resampler_buf;
    }
    bt->resampler_in_frames = 0;
    bt->resampler_buffer_size_in_frames = frames;

    return 0;

err_resampler_buf:
    free(bt->bt_out_buffer);
err_out_buf:
    release_resampler(bt->resampler);
    bt->resampler = NULL;
err_res:
    pcm_close(bt->pcm_bt);
    bt->pcm_bt = NULL;
err:
    return ret;
}

static void close_btSCO_device(struct aml_audio_device *adev)
{
    struct aml_bt_output *bt = &adev->bt_output;
    struct pcm *pcm = bt->pcm_bt;

    ALOGD("%s() ", __func__);
    if (pcm) {
        pcm_close(pcm);
        pcm = NULL;
    }
    if (bt->resampler) {
        release_resampler(bt->resampler);
        bt->resampler = NULL;
    }
    if (bt->bt_out_buffer)
        free(bt->bt_out_buffer);
    if (bt->resampler_buffer)
        free(bt->resampler_buffer);
}

ssize_t write_to_sco(struct audio_stream_out *stream,
        const void *buffer, size_t bytes)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = aml_out->dev;
    struct aml_bt_output *bt = &adev->bt_output;
    size_t frame_size = audio_stream_out_frame_size(stream);
    size_t in_frames = bytes / frame_size;
    size_t out_frames = in_frames * VX_NB_SAMPLING_RATE / MM_FULL_POWER_SAMPLING_RATE + 1;;
    int16_t *in_buffer = (int16_t *)buffer;
    int16_t *out_buffer = (int16_t *)bt->bt_out_buffer;
    unsigned int i;
    int ret = 0;

    /* Discard right channel */
    for (i = 1; i < in_frames; i++) {
        in_buffer[i] = in_buffer[i * 2];
    }
    /* The frame size is now half */
    frame_size /= 2;

    //prepare input buffer
    if (bt->resampler) {
        size_t frames_needed = bt->resampler_in_frames + in_frames;
        if (bt->resampler_buffer_size_in_frames < frames_needed) {
            bt->resampler_buffer_size_in_frames = frames_needed;
            bt->resampler_buffer = (int16_t *)realloc(bt->resampler_buffer,
                    bt->resampler_buffer_size_in_frames * frame_size);
        }

        memcpy(bt->resampler_buffer + bt->resampler_in_frames,
                buffer, in_frames * frame_size);
        bt->resampler_in_frames += in_frames;

        size_t res_in_frames = bt->resampler_in_frames;
        bt->resampler->resample_from_input(bt->resampler,
                     bt->resampler_buffer, &res_in_frames,
                     (int16_t*)bt->bt_out_buffer, &out_frames);
        //prepare output buffer
        bt->resampler_in_frames -= res_in_frames;
        if (bt->resampler_in_frames) {
            memmove(bt->resampler_buffer,
                bt->resampler_buffer + bt->resampler_in_frames,
                bt->resampler_in_frames * frame_size);
        }
    }

    if (bt->pcm_bt) {
        pcm_write(bt->pcm_bt, bt->bt_out_buffer, out_frames * frame_size);
        if (getprop_bool("media.audiohal.btpcm"))
            aml_audio_dump_audio_bitstreams("/data/audio/sco_8.raw", bt->bt_out_buffer, out_frames * frame_size);
    }
    return bytes;
}

bool is_sco_port(enum OUT_PORT outport)
{
    return (outport == OUTPORT_BT_SCO_HEADSET) ||
            (outport == OUTPORT_BT_SCO);
}

ssize_t mixer_aux_buffer_write_sm(struct audio_stream_out *stream, const void *buffer,
                               size_t bytes)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = aml_out->dev;
    struct subMixing *sm = adev->sm;
    struct aml_bt_output *bt = &adev->bt_output;
    size_t frame_size = audio_stream_out_frame_size(stream);
    size_t in_frames = bytes / frame_size;
    size_t bytes_remaining = bytes;
    ssize_t bytes_written = 0;
#ifdef DEBUG_TIME
    uint64_t us_since_last_write = 0;
    struct timespec tval_begin, tval_end;
    int64_t throttle_timeus = 0;
    clock_gettime(CLOCK_MONOTONIC, &tval_begin);
#endif

    ALOGV("++%s", __func__);
    if (is_sco_port(adev->active_outport)) {
        int ret = 0;
        if (!bt->active) {
            open_btSCO_device(adev, in_frames);
            bt->active = true;
        }

        return write_to_sco(stream, buffer, bytes);
    } else if (bt->active) {
        close_btSCO_device(adev);
        bt->active = false;
    }

    if (aml_out->standby) {
        char *padding_buf = NULL;
        int padding_bytes = 512 * 4 * 8;

        //set_thread_affinity();
        init_mixer_input_port(sm->mixerData, &aml_out->audioCfg, aml_out->flags,
            on_notify_cbk, aml_out, on_input_avail_cbk, aml_out,
            NULL, NULL, 1.0);

        aml_out->port_index = get_input_port_index(&aml_out->audioCfg, aml_out->flags);
        ALOGI("%s(), primary %p port index = %d",
            __func__, aml_out, aml_out->port_index);
        aml_out->standby = false;
        /* start padding zero to fill buffer */
        padding_buf = calloc(1, 512 * 4);
        if (padding_buf == NULL) {
            ALOGE("%s(), no memory", __func__);
            return -ENOMEM;
        }
        mixer_set_padding_size(sm->mixerData, aml_out->port_index, padding_bytes);
        while (padding_bytes > 0) {
            ALOGD("padding_bytes %d", padding_bytes);
            aml_out_write_to_mixer(stream, padding_buf, 512 * 4);
            padding_bytes -= 512 * 4;
        }
        free(padding_buf);
    }

    if (bytes == 0) {
        ALOGW("%s(), write bytes 0, quickly feed ringbuf", __func__);
        bytes_written = 0;
        goto exit;
    }

    bytes_written = aml_out_write_to_mixer(stream, buffer, bytes);
    if (bytes_written < 0) {
        ALOGE("%s(), write failed, err = %d", __func__, bytes_written);
    }
#ifdef DEBUG_TIME
    clock_gettime(CLOCK_MONOTONIC, &tval_end);
    us_since_last_write = (tval_end.tv_sec - aml_out->timestamp.tv_sec) * 1000000 +
            (tval_end.tv_nsec - aml_out->timestamp.tv_nsec) / 1000;
    int used_this_write = (tval_end.tv_sec - tval_begin.tv_sec) * 1000000 +
            (tval_end.tv_nsec - tval_begin.tv_nsec) / 1000;
    int target_us = bytes * 1000 / frame_size / 48;

    ALOGV("time spent on write %lld us, written %d", us_since_last_write, bytes_written);
    ALOGV("used_this_write %d us, target %d us", used_this_write, target_us);
    throttle_timeus = target_us - us_since_last_write;

    if (throttle_timeus > 0 && throttle_timeus < 200000) {
        ALOGV("throttle time %lld us", throttle_timeus);
        if (throttle_timeus > 1800 && aml_out->us_used_last_write < (uint64_t)target_us/2) {
            usleep(throttle_timeus - 1800);
            ALOGV("actual throttle %lld us3, since last %lld us",
                    throttle_timeus, us_since_last_write);
        } else {
            ALOGV("%lld us, but un-throttle", throttle_timeus);
        }
    } else if (throttle_timeus != 0) {
        ALOGV("invalid throttle time %lld us, us since last %lld us", throttle_timeus, us_since_last_write);
        ALOGV("\n\n");
    }
    aml_out->us_used_last_write = us_since_last_write;
#endif
exit:
    aml_out->frame_write_sum += in_frames;
    clock_gettime(CLOCK_MONOTONIC, &aml_out->timestamp);
    aml_out->lasttimestamp.tv_sec = aml_out->timestamp.tv_sec;
    aml_out->lasttimestamp.tv_nsec = aml_out->timestamp.tv_nsec;

    aml_out->last_frames_postion = aml_out->frame_write_sum;
    ALOGV("%s(), frame write sum %lld", __func__, aml_out->frame_write_sum);
    return bytes;
}

/* must be called with hw device mutexes locked */
int usecase_change_validate_l_sm(struct aml_stream_out *aml_out, bool is_standby)
{
    struct aml_audio_device *aml_dev = aml_out->dev;
    struct subMixing *sm = aml_dev->sm;
    struct amlAudioMixer *audio_mixer = sm->mixerData;
    bool hw_mix;

    if (is_standby) {
        ALOGI("++%s(), dev usecase masks = %#x, is_standby = %d, out usecase %s",
              __func__, aml_dev->usecase_masks, is_standby,
              aml_out->usecase < STREAM_USECASE_MAX && aml_out->usecase >= STREAM_PCM_NORMAL ? usecase_to_str(aml_out->usecase) : "STREAM_USECASE_INVAL");
        /**
         * If called by standby, reset out stream's usecase masks and clear the aml_dev usecase masks.
         * So other active streams could know that usecase have been changed.
         * But keep it's own usecase if out_write is called in the future to exit standby mode.
         */
        aml_out->dev_usecase_masks = 0;
        aml_out->write = NULL;
        aml_dev->usecase_masks &= ~(1 << aml_out->usecase);
        ALOGI("--%s(), dev usecase masks = %#x, is_standby = %d, out usecase %s",
              __func__, aml_dev->usecase_masks, is_standby,
              aml_out->usecase < STREAM_USECASE_MAX && aml_out->usecase >= STREAM_PCM_NORMAL ? usecase_to_str(aml_out->usecase) : "STREAM_USECASE_INVAL");
        return 0;
    }

    /* No usecase changes, do nothing */
    if (((aml_dev->usecase_masks == aml_out->dev_usecase_masks) && aml_dev->usecase_masks) && (aml_dev->continuous_audio_mode == 0)) {
        return 0;
    }

    ALOGI("++%s: dev usecase masks = %#x, out usecase_masks = %#x, out usecase %s",
           __func__, aml_dev->usecase_masks, aml_out->dev_usecase_masks, usecase_to_str(aml_out->usecase));

    /* check the usecase validation */
    if (popcount(aml_dev->usecase_masks & 0xfffffffe) > 1) {
        ALOGE("%s(), invalid usecase masks = %#x, out usecase %s!",
              __func__, aml_dev->usecase_masks, usecase_to_str(aml_out->usecase));
        return -EINVAL;
    }

    if (((aml_dev->continuous_audio_mode == 1) && (aml_dev->debug_flag > 1)) || \
        (aml_dev->continuous_audio_mode == 0))
        ALOGI("++++%s(),continuous_audio_mode %d dev usecase masks = %#x, out usecase_masks = %#x, out usecase %s",
              __func__, aml_dev->continuous_audio_mode, aml_dev->usecase_masks, aml_out->dev_usecase_masks,
              aml_out->usecase < STREAM_USECASE_MAX && aml_out->usecase >= STREAM_PCM_NORMAL ? usecase_to_str(aml_out->usecase) : "STREAM_USECASE_INVAL");

    /* new output case entered, so no masks has been set to the out stream */
    if (!aml_out->dev_usecase_masks) {
        if ((1 << aml_out->usecase) & aml_dev->usecase_masks) {
            ALOGE("%s(), usecase: %s already exists!!", __func__,
                aml_out->usecase < STREAM_USECASE_MAX && aml_out->usecase >= STREAM_PCM_NORMAL ? usecase_to_str(aml_out->usecase) : "STREAM_USECASE_INVAL");
            return -EINVAL;
        }

        if (popcount((aml_dev->usecase_masks | (1 << aml_out->usecase)) & 0xfffffffe) > 1) {
            ALOGE("%s(), usecase masks = %#x, couldn't add new out usecase %s!",
                  __func__, aml_dev->usecase_masks, usecase_to_str(aml_out->usecase));
            return -EINVAL;
        }

        /* add the new output usecase to aml_dev usecase masks */
        aml_dev->usecase_masks |= 1 << aml_out->usecase;
    }

    if (aml_out->is_normal_pcm) {
        if (aml_dev->audio_patching) {
            ALOGI("%s(), tv patching, mixer_aux_buffer_write!", __FUNCTION__);
            aml_out->write = mixer_aux_buffer_write;
        } else {
            aml_out->write = mixer_aux_buffer_write_sm;
            ALOGI("%s(), mixer_aux_buffer_write_sm !", __FUNCTION__);
        }
    } else {
        aml_out->write = mixer_main_buffer_write_sm;
        ALOGI("%s(), mixer_main_buffer_write_sm !", __FUNCTION__);
    }

    /* store the new usecase masks in the out stream */
    aml_out->dev_usecase_masks = aml_dev->usecase_masks;
    if (((aml_dev->continuous_audio_mode == 1) && (aml_dev->debug_flag > 1)) || \
        (aml_dev->continuous_audio_mode == 0))
        ALOGI("----%s(), continuous_audio_mode %d dev usecase masks = %#x, out usecase_masks = %#x, out usecase %s",
              __func__, aml_dev->continuous_audio_mode, aml_dev->usecase_masks, aml_out->dev_usecase_masks,
              aml_out->usecase < STREAM_USECASE_MAX && aml_out->usecase >= STREAM_PCM_NORMAL ? usecase_to_str(aml_out->usecase) : "STREAM_USECASE_INVAL");
    return 0;
}

/* out_write_submixing entrance: every write to submixing goes in here. */
static ssize_t out_write_subMixingPCM(struct audio_stream_out *stream,
                      const void *buffer,
                      size_t bytes)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = aml_out->dev;
    struct subMixing *sm = adev->sm;
    struct amlAudioMixer *audio_mixer = sm->mixerData;
    ssize_t ret = 0;
    //write_func  write_func_p = NULL;

    ALOGV("%s: out_stream(%p) position(%zu)", __func__, stream, bytes);

    /**
     * deal with the device output changes
     * pthread_mutex_lock(&aml_out->lock);
     * out_device_change_validate_l(aml_out);
     * pthread_mutex_unlock(&aml_out->lock);
     */
    pthread_mutex_lock(&adev->lock);
    ret = usecase_change_validate_l_sm(aml_out, false);
    if (ret < 0) {
        ALOGE("%s() failed", __func__);
        pthread_mutex_unlock(&adev->lock);
        return ret;
    }
    //if (aml_out->write) {
    //    write_func_p = aml_out->write;
    //}
    if (adev->rawtopcm_flag) {
        mixer_stop_outport_pcm(audio_mixer);
        adev->rawtopcm_flag = false;
        ALOGI("rawtopcm_flag disable !!!");
    }
    pthread_mutex_unlock(&adev->lock);
    if (aml_out->write) {
        ret = aml_out->write(stream, buffer, bytes);
    } else {
        ALOGE("%s(), NULL write function", __func__);
    }
    if (ret > 0) {
        aml_out->total_write_size += ret;
    }
    if (adev->debug_flag > 1) {
        ALOGI("-%s() ret %zd,%p %"PRIu64"\n", __func__, ret, stream, aml_out->total_write_size);
    }
    return ret;
}

int out_standby_subMixingPCM(struct audio_stream *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = aml_out->dev;
    struct subMixing *sm = adev->sm;
    struct amlAudioMixer *audio_mixer = sm->mixerData;
    ssize_t ret = 0;

    ALOGD("%s: out_stream(%p) usecase: %s", __func__, stream, usecase_to_str(aml_out->usecase));
    /**
     * deal with the device output changes
     * pthread_mutex_lock(&aml_out->lock);
     * out_device_change_validate_l(aml_out);
     * pthread_mutex_unlock(&aml_out->lock);
     */
    pthread_mutex_lock(&adev->lock);
    if (aml_out->standby) {
        goto exit;
    }

    ret = usecase_change_validate_l_sm(aml_out, true);
    if (ret < 0) {
        ALOGE("%s() failed", __func__);
        goto exit;
    }
    aml_out->status = STREAM_STANDBY;
    aml_out->standby = true;
    aml_out->exiting = true;
    delete_mixer_input_port(audio_mixer, aml_out->port_index);

    if (adev->debug_flag > 1) {
        ALOGI("-%s() ret %zd,%p %"PRIu64"\n", __func__, ret, stream, aml_out->total_write_size);
    }
exit:
    pthread_mutex_unlock(&adev->lock);
    return ret;
}

static int out_pause_subMixingPCM(struct audio_stream_out *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream;
    struct aml_audio_device *aml_dev = aml_out->dev;
    struct subMixing *sm = aml_dev->sm;
    struct amlAudioMixer *audio_mixer = NULL;

    ALOGI("+%s(), stream %p, standby %d, pause status %d, usecase: %s",
            __func__,
            aml_out,
            aml_out->standby,
            aml_out->pause_status,
            usecase_to_str(aml_out->usecase));
    if (aml_out->standby || aml_out->pause_status) {
        ALOGW("%s(), stream already paused", __func__);
        return INVALID_STATE;
    }

    if (sm->type != MIXER_LPCM) {
        ALOGW("%s(), sub mixing type not pcm, type is %d", __func__, sm->type);
        return 0;
    }

    audio_mixer = sm->mixerData;
    send_mixer_inport_message(audio_mixer, aml_out->port_index, MSG_PAUSE);

    aml_out->pause_status = true;
    ALOGI("-%s()", __func__);
    return 0;
}

static int out_resume_subMixingPCM(struct audio_stream_out *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *aml_dev = aml_out->dev;
    struct subMixing *sm = aml_dev->sm;
    struct amlAudioMixer *audio_mixer = NULL;
    int ret = 0;

    ALOGD("+%s(), stream %p, standby %d, pause status %d, usecase: %s",
            __func__,
            aml_out,
            aml_out->standby,
            aml_out->pause_status,
            usecase_to_str(aml_out->usecase));

    if (!aml_out->pause_status) {
        ALOGW("%s(), steam not in pause status", __func__);
        return INVALID_STATE;
    }

    if (sm->type != MIXER_LPCM) {
        ALOGW("%s(), sub mixing type not pcm, type is %d", __func__, sm->type);
        return 0;
    }

    audio_mixer = sm->mixerData;
    send_mixer_inport_message(audio_mixer, aml_out->port_index, MSG_RESUME);

    aml_out->pause_status = false;
    ALOGD("-%s()", __func__);
    return 0;
}

/* If supported, a stream should always succeed to flush */
static int out_flush_subMixingPCM(struct audio_stream_out *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *aml_dev = aml_out->dev;
    struct subMixing *sm = aml_dev->sm;
    struct amlAudioMixer *audio_mixer = NULL;
    int ret = 0;

    ALOGI("+%s(), stream %p, standby %d, pause status %d, usecase: %s",
            __func__,
            aml_out,
            aml_out->standby,
            aml_out->pause_status,
            usecase_to_str(aml_out->usecase));

    if (sm->type != MIXER_LPCM) {
        ALOGW("%s(), sub mixing type not pcm, type is %d", __func__, sm->type);
        return 0;
    }
    aml_out->frame_write_sum  = 0;
    aml_out->last_frames_postion = 0;
    aml_out->spdif_enc_init_frame_write_sum =  0;
    aml_out->frame_skip_sum = 0;
    aml_out->skip_frame = 0;
    aml_out->input_bytes_size = 0;
    //aml_out->pause_status = false;
    if (aml_out->pause_status) {
        struct meta_data_list *mdata_list;
        struct listnode *item;

        //mixer_flush_inport(audio_mixer, out->port_index);
        if (aml_out->hw_sync_mode) {
            pthread_mutex_lock(&aml_out->mdata_lock);
            while (!list_empty(&aml_out->mdata_list)) {
                item = list_head(&aml_out->mdata_list);
                mdata_list = node_to_item(item, struct meta_data_list, list);
                list_remove(item);
                free(mdata_list);
            }
            pthread_mutex_unlock(&aml_out->mdata_lock);
        }
        audio_mixer = sm->mixerData;
        send_mixer_inport_message(audio_mixer, aml_out->port_index, MSG_FLUSH);
        if (!aml_out->standby)
            flush_hw_avsync_header_extractor(aml_out->hwsync_extractor);
        //mixer_set_inport_state(audio_mixer, out->port_index, FLUSHING);
        aml_out->last_frames_postion = 0;
        aml_out->first_pts_set = false;
        //aml_out->pause_status = false;
        //aml_out->standby = true;
    } else {
        ALOGW("%s(), line %d. Need check this case!", __func__, __LINE__);
        return 0;
    }

    ALOGI("-%s()", __func__);
    return 0;
}

int switchNormalStream(struct aml_stream_out *aml_out, bool on)
{
    ALOGI("+%s() stream %p, on = %d", __func__, aml_out, on);
    if (!aml_out) {
        ALOGE("%s(), no stream", __func__);
        return -EINVAL;

    }
    if (!aml_out->is_normal_pcm) {
        ALOGE("%s(), not normal pcm stream", __func__);
        return -EINVAL;
    }
    if (on) {
        initSubMixingInputPcm(&aml_out->out_cfg, aml_out);
        aml_out->stream.write = mixer_aux_buffer_write_sm;
        aml_out->stream.common.standby = out_standby_subMixingPCM;
        out_standby_subMixingPCM((struct audio_stream *)aml_out);
    } else {
        aml_out->stream.write = mixer_aux_buffer_write;
        aml_out->stream.common.standby = out_standby_new;
        deleteSubMixingInputPcm(aml_out);
        out_standby_new((struct audio_stream *)aml_out);
    }

    return 0;
}

