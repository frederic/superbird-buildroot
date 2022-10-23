/*
 * Copyright (C) 2014 The Android Open Source Project
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
 *  @author   Hugo Hong
 *  @version  1.0
 *  @date     2018/04/01
 *  @par function description:
 *  - 1 bluetooth rc audio hal
 */

#define LOG_TAG "AudioHAL:audio_hal_thunks"
#include <utils/log.h>

#include <errno.h>
#include <stdlib.h>

#include <hardware/hardware.h>
#include <hardware/audio.h>

#include "AudioHardwareInput.h"
#include "AudioStreamIn.h"
#include "audio_hal_thunks.h"

using namespace android;

AudioHardwareInput gAudioHardwareInput;

extern "C" {

struct atv_stream_in {
    struct aml_stream_in stream;
    AudioStreamIn* impl;
};

struct atv_in_device {
    AudioHardwareInput* input;
    Mutex lock;
};

static struct atv_in_device g_indevice = {
    .input = &gAudioHardwareInput,
};

/*******************************************************************************
 *
 * Bluetooth RC Audio input stream implementation
 *
 ******************************************************************************/

static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    Mutex::Autolock _l(g_indevice.lock);
    const struct atv_stream_in* tstream =
        reinterpret_cast<const struct atv_stream_in*>(stream);

    return tstream->impl->getSampleRate();
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    Mutex::Autolock _l(g_indevice.lock);
    const struct atv_stream_in* tstream =
        reinterpret_cast<const struct atv_stream_in*>(stream);

    return tstream->impl->setSampleRate(rate);
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    Mutex::Autolock _l(g_indevice.lock);
    const struct atv_stream_in* tstream =
        reinterpret_cast<const struct atv_stream_in*>(stream);

    return tstream->impl->getBufferSize();
}

static uint32_t in_get_channels(const struct audio_stream *stream)
{
    Mutex::Autolock _l(g_indevice.lock);
    const struct atv_stream_in* tstream =
        reinterpret_cast<const struct atv_stream_in*>(stream);

    return tstream->impl->getChannelMask();
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
    Mutex::Autolock _l(g_indevice.lock);
    const struct atv_stream_in* tstream =
        reinterpret_cast<const struct atv_stream_in*>(stream);

    return tstream->impl->getFormat();
}

static int in_set_format(struct audio_stream *stream, audio_format_t format)
{
    Mutex::Autolock _l(g_indevice.lock);
    const struct atv_stream_in* tstream =
        reinterpret_cast<const struct atv_stream_in*>(stream);

    return tstream->impl->setFormat(format);
}

static int in_standby(struct audio_stream *stream)
{
    Mutex::Autolock _l(g_indevice.lock);
    const struct atv_stream_in* tstream =
        reinterpret_cast<const struct atv_stream_in*>(stream);

    return tstream->impl->standby();
}

static int in_dump(const struct audio_stream *stream, int fd)
{
    Mutex::Autolock _l(g_indevice.lock);
    const struct atv_stream_in* tstream =
        reinterpret_cast<const struct atv_stream_in*>(stream);

    return tstream->impl->dump(fd);
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    Mutex::Autolock _l(g_indevice.lock);
    const struct atv_stream_in* tstream =
        reinterpret_cast<const struct atv_stream_in*>(stream);

    return tstream->impl->setParameters(stream, kvpairs);
}

static char* in_get_parameters(const struct audio_stream *stream,
                               const char *keys)
{
    Mutex::Autolock _l(g_indevice.lock);
    const struct atv_stream_in* tstream =
        reinterpret_cast<const struct atv_stream_in*>(stream);

    return tstream->impl->getParameters(keys);
}

static int in_set_gain(struct audio_stream_in *stream, float gain)
{
    Mutex::Autolock _l(g_indevice.lock);
    const struct atv_stream_in* tstream =
        reinterpret_cast<const struct atv_stream_in*>(stream);

    return tstream->impl->setGain(gain);
}

static ssize_t in_read(struct audio_stream_in *stream, void* buffer,
                       size_t bytes)
{
    Mutex::Autolock _l(g_indevice.lock);
    const struct atv_stream_in* tstream =
        reinterpret_cast<const struct atv_stream_in*>(stream);

    return tstream->impl->read(buffer, bytes);
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    Mutex::Autolock _l(g_indevice.lock);
    const struct atv_stream_in* tstream =
        reinterpret_cast<const struct atv_stream_in*>(stream);

    return tstream->impl->getInputFramesLost();
}

static int in_add_audio_effect(const struct audio_stream *stream,
                               effect_handle_t effect)
{
    Mutex::Autolock _l(g_indevice.lock);
    const struct atv_stream_in* tstream =
        reinterpret_cast<const struct atv_stream_in*>(stream);

    return tstream->impl->addAudioEffect(effect);
}

static int in_remove_audio_effect(const struct audio_stream *stream,
                                  effect_handle_t effect)
{
    Mutex::Autolock _l(g_indevice.lock);
    const struct atv_stream_in* tstream =
        reinterpret_cast<const struct atv_stream_in*>(stream);

    return tstream->impl->removeAudioEffect(effect);
}

/*******************************************************************************
 *
 * Audio device stubs
 *
 ******************************************************************************/

int rc_open_input_stream(struct aml_stream_in **stream,
                        struct audio_config *config) {
    ALOGD("%s++:build %s, %s", __FUNCTION__, __DATE__, __TIME__);
    assert(stream != NULL);

    Mutex::Autolock _l(g_indevice.lock);
    struct atv_stream_in* in = NULL;
    int ret = 0;

    in = (struct atv_stream_in*)realloc(*stream, sizeof(struct atv_stream_in));
    if (in == NULL) return -ENOMEM;

    *stream = &(in->stream);
    (*stream)->stream.common.get_sample_rate = in_get_sample_rate;
    (*stream)->stream.common.set_sample_rate = in_set_sample_rate;
    (*stream)->stream.common.get_buffer_size = in_get_buffer_size;
    (*stream)->stream.common.get_channels = in_get_channels;
    (*stream)->stream.common.get_format = in_get_format;
    (*stream)->stream.common.set_format = in_set_format;
    (*stream)->stream.common.standby = in_standby;
    (*stream)->stream.common.dump = in_dump;
    (*stream)->stream.common.set_parameters = in_set_parameters;
    (*stream)->stream.common.get_parameters = in_get_parameters;
    (*stream)->stream.common.add_audio_effect = in_add_audio_effect;
    (*stream)->stream.common.remove_audio_effect = in_remove_audio_effect;
    (*stream)->stream.set_gain = in_set_gain;
    (*stream)->stream.read = in_read;
    (*stream)->stream.get_input_frames_lost = in_get_input_frames_lost;

    //setup in stream
    in->impl = g_indevice.input->openInputStream((struct audio_stream_in*)(*stream),
                                            &config->format,
                                            &config->channel_mask,
                                            &config->sample_rate,
                                            reinterpret_cast<status_t*>(&ret));

    ALOGD("%s--, ret=%d",__FUNCTION__, ret);
    return ret;
}

void rc_close_input_stream(struct aml_stream_in *stream) {
    ALOGD("%s", __FUNCTION__);
    assert(stream != NULL);

    Mutex::Autolock _l(g_indevice.lock);
    struct atv_stream_in* in = reinterpret_cast<struct atv_stream_in*>(stream);

    if (in->impl != NULL) {
        g_indevice.input->closeInputStream(in->impl);
        in->impl = NULL;
    }

}


}  // extern "C"

