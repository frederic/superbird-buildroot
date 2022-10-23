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


#include <pulse/simple.h>
#include <pulse/error.h>
#include <unistd.h>


#include "log.h"
#include "audio_core.h"
#include "audio_hal.h"
#include "audio_hw.h"





typedef struct pulse_audio_handle {
    pa_sample_spec sample_spec;
	pa_simple * pa_simple;

} pa_handle_t;

/* The Sample format to use */
static const pa_sample_spec  sample_spec= {
	.format = PA_SAMPLE_S16LE,
	.rate = 48000,
	.channels = 2
};

int aml_pa_output_open(void **handle, aml_stream_config_t * stream_config, aml_device_config_t *device_config) {
    int ret = -1;
	int error;
	struct pcm *pcm = NULL;
	pa_handle_t * pa_handle = NULL;
	int audio_latency = 0;
	
	pa_simple *s = NULL;
	pa_buffer_attr buf_attr;

    
	pa_handle = (pa_handle_t *)malloc(sizeof(pa_handle_t));
    if(pa_handle == NULL) {
		ALOGE("malloc pa_handle failed\n");
		return -1;
	}

	memcpy(&pa_handle->sample_spec, &sample_spec,sizeof(pa_sample_spec));

	if (stream_config->format == AUDIO_FORMAT_PCM_16_BIT) {
		pa_handle->sample_spec.format = PA_SAMPLE_S16LE;
	} else if (stream_config->format == AUDIO_FORMAT_PCM_32_BIT) {
		pa_handle->sample_spec.format = PA_SAMPLE_S32LE;
	} else {
		pa_handle->sample_spec.format = PA_SAMPLE_S16LE;

	}

	pa_handle->sample_spec.rate     = stream_config->rate;
	pa_handle->sample_spec.channels = stream_config->channels;
	ALOGD("%s rate=%d format=%d ch=%d\n",__func__,pa_handle->sample_spec.rate,pa_handle->sample_spec.format,pa_handle->sample_spec.channels);

    // currently we accetp a small value
	if(stream_config->latency > 0) {
		audio_latency = stream_config->latency*1000;
	}else {
		audio_latency = 50*1000;
	}
	buf_attr.maxlength = -1;
	buf_attr.tlength = pa_usec_to_bytes(audio_latency, &pa_handle->sample_spec);
	buf_attr.prebuf = -1;
	buf_attr.minreq = -1;
	buf_attr.fragsize = -1;

	ALOGD("buf_attr.tlength=%d ms=%d\n",buf_attr.tlength,audio_latency);
	if (!(s = pa_simple_new(NULL, "aml_halaudio", PA_STREAM_PLAYBACK, NULL, "playback", &pa_handle->sample_spec, NULL, &buf_attr, &error))) {
        ALOGE(": pa_simple_new() failed: %s\n", pa_strerror(error));
        goto exit;
    }
	
	pa_handle->pa_simple = s;

	*handle = (void*)pa_handle;

	return 0;

exit:
	if(pa_handle) {
		free(pa_handle);
	}
	return -1;

}


void aml_pa_output_close(void *handle)
{
    ALOGI("\n+%s() hanlde %p\n", __func__, handle);
	pa_handle_t * pa_handle = NULL;
	
    pa_handle = (pa_handle_t *)handle;

	if(pa_handle == NULL) {
		ALOGE("handle is NULL\n");
		return ;
	}

    if (pa_handle->pa_simple) {
        pa_simple_free(pa_handle->pa_simple);
    }	

	free(pa_handle);
	
    ALOGI("-%s()\n\n", __func__);
}

size_t aml_pa_output_write(void *handle, const void *buffer, size_t bytes) {
	int ret = -1;
	int error;
	pa_handle_t * pa_handle = NULL;


    pa_handle = (pa_handle_t *)handle;

	if(pa_handle == NULL) {
		ALOGE("handle is NULL\n");
		return -1;
	}

	if(pa_handle->pa_simple == NULL) {
		ALOGE("pa_simple is NULL\n");
		return -1;
	}

	//ALOGE("buff=%p bytes=%d\n",buffer,bytes);
	if (pa_simple_write(pa_handle->pa_simple, buffer, (size_t) bytes, &error) < 0) {
		ALOGE(": pa_simple_write() failed: %s\n", pa_strerror(error));
		return -1;
	}



	return 0;
}

