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

#define LOG_TAG "aml_spdif_in"
#include "aml_alsa_mixer.h"
#include "aml_spdif_in.h"
#include "log.h"

#define SPDIFIN_DETECTING_DURATION  30*1000 // us

enum {
    SPDIFIN_UNSTABLE,
    SPDIFIN_UNSTABLE_TO_STABLE,
    SPDIFIN_STABLE,
    SPDIFIN_STABLE_TO_UNSTABLE,
};

typedef struct aml_spdif_in {
    int status;
    int samplerate;
    int64_t  begin_time;
} aml_spdif_in_t;

static const char * const spdifin_sample_rate_text[] = {
    "N/A",
    "32000",
    "44100",
    "48000",
    "88200",
    "96000",
    "176400",
    "192000"

};

/*We need remove these define some where*/
static const char *const resample_module_texts[] = {
    "TDMIN_A",
    "TDMIN_B",
    "TDMIN_C",
    "SPDIFIN",
    "PDMIN",
    "NONE",
    "TDMIN_LB",
    "LOOPBACK",
};

static const char *const resample_texts[] = {
    "Disable",
    "Enable:32K",
    "Enable:44K",
    "Enable:48K",
    "Enable:88K",
    "Enable:96K",
    "Enable:176K",
    "Enable:192K",
};


int get_spdifin_resample_enabled(void)
{
    int index = 0;

    index = aml_mixer_ctrl_get_int(AML_MIXER_ID_HW_RESAMPLE_MODULE);
    if (index <= 0 || index > 7) {
        return 0;
    }

    if (strncmp(resample_module_texts[index], "SPDIFIN", 6)) {
        //ALOGD("Resample module is not SPDIF IN\n");
        return 0;
    } else {
        index = aml_mixer_ctrl_get_int(AML_MIXER_ID_HW_RESAMPLE_ENABLE);
        //ALOGD("SPDIF In Resample =%d\n", index);
        if (index == 0) {
            return 0;
        } else {
            return 1;
        }
    }
}


int get_spdifin_samplerate(void)
{
    int index = 0;

    index = aml_mixer_ctrl_get_int(AML_MIXER_ID_SPDIFIN_SAMPLE_RATE);
    if (index <= 0 || index > 7) {
        return 0;
    }

    return atoi(spdifin_sample_rate_text[index]);
}


int aml_spdifin_init(aml_spdif_in_t **aml_spdifin)
{
    aml_spdif_in_t * spdifin_handle = NULL;

    spdifin_handle = (aml_spdif_in_t *)calloc(1, sizeof(aml_spdif_in_t));

    if (spdifin_handle == NULL) {
        ALOGE("malloc spdifin_handle failed\n");
        *aml_spdifin = NULL;
        return -1;
    }
    spdifin_handle->status = SPDIFIN_UNSTABLE;

    *aml_spdifin = spdifin_handle;

    return 0;
}

int aml_spdifin_release(aml_spdif_in_t *aml_spdifin)
{

    if (aml_spdifin == NULL) {
        return -1;
    }

    free(aml_spdifin);
    return 0;
}

int aml_spdifin_getinfo(aml_spdif_in_t *aml_spdifin, int * original_rate, int * output_rate)
{
    struct timespec now;
    int64_t time_diff;
    int current_samplerate = 0;
    int bspdif_resampled = 0;
    if (aml_spdifin == NULL) {
        *output_rate = 0;
        *original_rate = 0;
        return -1;
    }

    //bspdif_resampled = get_spdifin_resample_enabled();
#if 0
    if (bspdif_resampled == 1) {
        *output_rate = 48000;
        *original_rate = get_spdifin_samplerate();
        return 0;
    }
#endif
    switch (aml_spdifin->status) {
    case SPDIFIN_UNSTABLE:
        clock_gettime(CLOCK_MONOTONIC, &now);
        aml_spdifin->begin_time = (now.tv_sec * 1000000LL + now.tv_nsec / 1000LL);
        aml_spdifin->samplerate = get_spdifin_samplerate();
        *original_rate = aml_spdifin->samplerate;
        *output_rate = 0;
        if (aml_spdifin->samplerate == 0) {
            aml_spdifin->status = SPDIFIN_UNSTABLE;
        } else {
            aml_spdifin->status = SPDIFIN_UNSTABLE_TO_STABLE;
        }
        break;
    case SPDIFIN_UNSTABLE_TO_STABLE:
        clock_gettime(CLOCK_MONOTONIC, &now);
        time_diff = (now.tv_sec * 1000000LL + now.tv_nsec / 1000LL) - aml_spdifin->begin_time;
        //ALOGD("time diff=%lld\n",time_diff);
        current_samplerate = get_spdifin_samplerate();
        *original_rate = current_samplerate;
        /*during detecting stage, the samplerate is changed, we back to unstable*/
        if (aml_spdifin->samplerate != current_samplerate) {
            aml_spdifin->samplerate = current_samplerate;
            *output_rate = 0;
            aml_spdifin->status = SPDIFIN_UNSTABLE;
        } else {
            if (time_diff >= SPDIFIN_DETECTING_DURATION) {
                /* if the samplerate is not chnaged during period time, we treat it as stable*/
                aml_spdifin->samplerate = current_samplerate;
                *output_rate = current_samplerate;
                aml_spdifin->status = SPDIFIN_STABLE;
            } else {
                /*continue to check*/
                *output_rate = 0;
                aml_spdifin->status = SPDIFIN_UNSTABLE_TO_STABLE;
            }
        }
        ALOGV("spdif in status=%d rate=%d detecting=%d\n", aml_spdifin->status, *output_rate, current_samplerate);
        break;
    case SPDIFIN_STABLE:
        clock_gettime(CLOCK_MONOTONIC, &now);
        current_samplerate = get_spdifin_samplerate();
        *original_rate = current_samplerate;
        *output_rate = aml_spdifin->samplerate;
        if (aml_spdifin->samplerate == current_samplerate) {
            aml_spdifin->status = SPDIFIN_STABLE;

        } else {
            /*find some samplerate changed, we need some time to detect it*/
            aml_spdifin->begin_time = (now.tv_sec * 1000000LL + now.tv_nsec / 1000LL);
            aml_spdifin->status = SPDIFIN_STABLE_TO_UNSTABLE;
            ALOGV("spdif in status=%d rate=%d detect=%d\n", aml_spdifin->status, *output_rate, current_samplerate);

        }

        break;
    case SPDIFIN_STABLE_TO_UNSTABLE:
        clock_gettime(CLOCK_MONOTONIC, &now);
        current_samplerate = get_spdifin_samplerate();
        *original_rate = current_samplerate;
        time_diff = (now.tv_sec * 1000000LL + now.tv_nsec / 1000LL) - aml_spdifin->begin_time;

        if (current_samplerate != aml_spdifin->samplerate) {

            if (time_diff >= SPDIFIN_DETECTING_DURATION) {
                /*it is changed, we switch it*/
                aml_spdifin->status = SPDIFIN_UNSTABLE;
                *output_rate = 0;
            } else {
                /*treat these dither still stable*/
                aml_spdifin->status = SPDIFIN_STABLE_TO_UNSTABLE;
                *output_rate = aml_spdifin->samplerate;
            }

        } else {
            /*it is still stable*/
            aml_spdifin->status = SPDIFIN_STABLE;
            *output_rate = aml_spdifin->samplerate;

        }
        ALOGV("spdif in status=%d rate=%d detect=%d\n", aml_spdifin->status, *output_rate, current_samplerate);
        break;
    default:
        *output_rate = 0;
        break;
    }

    if (aml_spdifin->status == SPDIFIN_STABLE ||
        aml_spdifin->status == SPDIFIN_STABLE_TO_UNSTABLE) {
        if (bspdif_resampled == 1) {
            *output_rate = 48000;
        }
    }


    return 0;
}



