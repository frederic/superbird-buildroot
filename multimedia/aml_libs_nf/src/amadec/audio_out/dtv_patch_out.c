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
 *
 *  DESCRIPTION:
 *      brief  Functions of Auduo output control for Linux Platform.
 *
 */

#define LOG_TAG "amadec"
#include <fcntl.h>
#include <linux/soundcard.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
//#include <config.h>
#include <adec-pts-mgt.h>
#include <audio-dec.h>
#include <cutils/properties.h>
#include <log-print.h>
#include <pthread.h>
#include <adec-external-ctrl.h>
#include <amthreadpool.h>
#include <dtv_patch_out.h>

#define AUD_ASSO_PROP "media.audio.enable_asso"
#define AUD_ASSO_MIX_PROP "media.audio.mix_asso"
#define VID_DISABLED_PROP "media.dvb.video.disabled"
#define AUD_DISABLED_PROP "media.dvb.audio.disabled"

typedef struct _dtv_patch_out {
    aml_audio_dec_t *audec;
    out_pcm_write pcmout_cb;
    out_get_wirte_space space_cb;
    out_audio_info   info_cb;
    int device_opened;
    int state;
    pthread_t tid;
    void *pargs;
} dtv_patch_out;

enum {
    DTV_PATCH_STATE_CLODED = 0x01,
    DTV_PATCH_STATE_OPENED,
    DTV_PATCH_STATE_RUNNING,
    DTV_PATCH_STATE_PAUSE,
    DTV_PATCH_STATE_RESUME,
    DTV_PATCH_STATE_STOPED,
};

static dtv_patch_out outparam = {
    .audec = NULL,
    .pcmout_cb = NULL,
    .space_cb = NULL,
    .device_opened = 0,
    .state = DTV_PATCH_STATE_CLODED,
    .tid = -1,
    .pargs = NULL,
};

static pthread_mutex_t patch_out_mutex = PTHREAD_MUTEX_INITIALIZER;

//static char output_buffer[64 * 1024];
static unsigned char decode_buffer[OUTPUT_BUFFER_SIZE + 64];
#define PERIOD_SIZE 1024
#define PERIOD_NUM 4
#define AV_SYNC_THRESHOLD 60

static int _get_asso_enable()
{
    return property_get_int32(AUD_ASSO_PROP, 0);
}
static int _get_asso_mix()
{
    return property_get_int32(AUD_ASSO_MIX_PROP, 50);
}
/*
static int _get_vid_disabled()
{
    return property_get_int32(VID_DISABLED_PROP, 0);
}
static int _get_aud_disabled()
{
    return property_get_int32(AUD_DISABLED_PROP, 0);
}
*/
static dtv_patch_out *get_patchout(void)
{
    return &outparam;
}

static int out_patch_initd = 0;

unsigned long dtv_patch_get_pts(void)
{
    unsigned long pts;
    dtv_patch_out *param = get_patchout();

    pthread_mutex_lock(&patch_out_mutex);
    aml_audio_dec_t *audec = (aml_audio_dec_t *)param->audec;
    if (param->state != DTV_PATCH_STATE_STOPED && audec != NULL
        && audec->adsp_ops.get_cur_pts != NULL) {
        pts = audec->adsp_ops.get_cur_pts(&audec->adsp_ops);
        pthread_mutex_unlock(&patch_out_mutex);
        return pts;
    } else {
        pthread_mutex_unlock(&patch_out_mutex);
        return -1;
    }
}

static void *dtv_patch_out_loop(void *args)
{

    int len = 0;
    int len2 = 0;
    int offset = 0;
    int readcount = 0;
    //unsigned space_size = 0;
    //unsigned long pts;
    char *buffer =
        (char *)(((unsigned long)decode_buffer + 32) & (~0x1f));
    dtv_patch_out *patchparm = (dtv_patch_out *)args;
    int  channels, samplerate;
    aml_audio_dec_t *audec = (aml_audio_dec_t *)patchparm->audec;

    while (!(patchparm->state == DTV_PATCH_STATE_STOPED)) {
        // pthread_mutex_lock(&patch_out_mutex);
        {

            if (patchparm->state == DTV_PATCH_STATE_PAUSE) {
                // pthread_mutex_unlock(&patch_out_mutex);
                usleep(10000);
                continue;
            }
            if (offset > 0) {
                memmove(buffer, buffer + offset, len);
            }
            if (audec == NULL) {
                adec_print("the audec is NULL\n");
                // pthread_mutex_unlock(&patch_out_mutex);
                usleep(10000);
                if (patchparm->state == DTV_PATCH_STATE_STOPED) {
                    goto exit;
                }
                continue;
            }
            if (audec->adsp_ops.dsp_read == NULL) {
                adec_print("the audec dsp_read is NULL\n");
                // pthread_mutex_unlock(&patch_out_mutex);
                usleep(10000);
                if (patchparm->state == DTV_PATCH_STATE_STOPED) {
                    goto exit;
                }
                continue;
            }

            if (audec->g_bst->buf_level < 1024) {
                usleep(1000);
                if (patchparm->state == DTV_PATCH_STATE_STOPED) {
                    goto exit;
                }
                continue;
            }

            if (audec->adsp_ops.get_cur_pts == NULL) {
                adec_print("the audec get_cur_pts is NULL\n");
                // pthread_mutex_unlock(&patch_out_mutex);
                usleep(10000);
                if (patchparm->state == DTV_PATCH_STATE_STOPED) {
                    goto exit;
                }

                continue;
            }

            if (patchparm->space_cb(patchparm->pargs) < 4096) {
                if (patchparm->state == DTV_PATCH_STATE_STOPED) {
                    goto exit;
                }
                usleep(1000);
                if (patchparm->state == DTV_PATCH_STATE_STOPED) {
                    goto exit;
                }

                continue;
            }
            // pts = audec->adsp_ops.get_cur_pts(&audec->adsp_ops);
            if (audec->g_bst->buf_level < (OUTPUT_BUFFER_SIZE - len) &&
                readcount < 25 &&
                (audec->format == ACODEC_FMT_AC3 ||
                 audec->format == ACODEC_FMT_EAC3 ||
                 audec->format == ACODEC_FMT_DTS)) {
                len2 = 0;
                readcount++;
            } else {
                len2 = audec->adsp_ops.dsp_read(&audec->adsp_ops, (buffer + len),
                                                (OUTPUT_BUFFER_SIZE - len));
                readcount = 0;
            }
            //adec_print("len2 %d", len2);
            len = len + len2;
            offset = 0;
        }

        if (len == 0) {
            usleep(1000);
            if (patchparm->state == DTV_PATCH_STATE_STOPED) {
                goto exit;
            }
            continue;
        }
        audec->pcm_bytes_readed += len;
        {
           if (audec->format == ACODEC_FMT_DRA) {
                patchparm->info_cb(patchparm->pargs,audec->adec_ops->NchOriginal,
                 audec->adec_ops->lfepresent);
           }
            channels = audec->g_bst->channels;
            samplerate = audec->g_bst->samplerate;
            len2 = patchparm->pcmout_cb((unsigned char *)(buffer + offset), len, samplerate,
                                        channels, patchparm->pargs);
            if (len2 == 0) {
                if (patchparm->state == DTV_PATCH_STATE_STOPED) {
                    goto exit;
                }
            }
        }

        if (len2 >= 0) {
            len -= len2;
            offset += len2;
        } else {
            len = 0;
            offset = 0;
        }
        // pthread_mutex_unlock(&patch_out_mutex);
    }
exit:
    adec_print("Exit alsa playback loop !\n");
    pthread_exit(NULL);
    return NULL;
}

int dtv_patch_input_open(unsigned int *handle, out_pcm_write pcmcb,
                         out_get_wirte_space spacecb, out_audio_info info_cb,void *args)
{
    //int ret;

    if (handle == NULL || pcmcb == NULL || spacecb == NULL) {
        return -1;
    }

    dtv_patch_out *param = get_patchout();

    pthread_mutex_lock(&patch_out_mutex);
    param->pcmout_cb = pcmcb;
    param->space_cb = spacecb;
    param->info_cb = info_cb;
    param->pargs = args;
    param->device_opened = 1;
    amthreadpool_system_init();
    pthread_mutex_unlock(&patch_out_mutex);
    adec_print("now the audio decoder open now!\n");
    *handle = (int)param;

    return 0;
}

int dtv_patch_input_start(unsigned int handle, int aformat, int has_video)
{
    int ret;
    adec_print("now enter the dtv_patch_input_start function handle %d "
               "out_patch_initd %d  \n ",
               handle, out_patch_initd);

    if (handle == 0) {
        return -1;
    }

    pthread_mutex_lock(&patch_out_mutex);
    if (out_patch_initd == 1) {
        pthread_mutex_unlock(&patch_out_mutex);
        return -1;
    }

    arm_audio_info param;
    dtv_patch_out *paramout = get_patchout();

    if (paramout->state == DTV_PATCH_STATE_RUNNING) {
        pthread_mutex_unlock(&patch_out_mutex);
        adec_print("11111111111111");
        return -1;
    }
    adec_print("22222222222222222");
    adec_print("now the audio decoder start  now, aformat %d  has_video %d!\n",
               aformat, has_video);
    memset(&param, 0, sizeof(param));
    param.handle = -1;
    param.format = aformat;
    param.has_video = has_video;
    paramout->audec = NULL;
    param.associate_dec_supported = _get_asso_enable();
    param.mixing_level = _get_asso_mix();
    paramout->state = DTV_PATCH_STATE_RUNNING;
    audio_decode_init((void **)(&(paramout->audec)), &param);
    ret = pthread_create(&(paramout->tid), NULL, &dtv_patch_out_loop, paramout);
    out_patch_initd = 1;
    pthread_mutex_unlock(&patch_out_mutex);
    adec_print("now leave the dtv_patch_input_start function \n ");

    return 0;
}

int dtv_patch_input_stop(unsigned int handle)
{

    if (handle == 0) {
        return -1;
    }

    dtv_patch_out *paramout = get_patchout();
    pthread_mutex_lock(&patch_out_mutex);
    if (out_patch_initd == 0) {
        pthread_mutex_unlock(&patch_out_mutex);
        return -1;
    }

    adec_print("now enter the audio decoder stop now!\n");

    paramout->state = DTV_PATCH_STATE_STOPED;
    pthread_join(paramout->tid, NULL);
    adec_print("now enter the audio decoder stop now111111!\n");
    audio_decode_stop(paramout->audec);
    audio_decode_release((void **) & (paramout->audec));

    adec_print("now enter the audio decoder stop now222222!\n");
    paramout->audec = NULL;
    adec_print("now enter the audio decoder stop now!\n");
    paramout->tid = -1;
    out_patch_initd = 0;
    pthread_mutex_unlock(&patch_out_mutex);

    adec_print("now leave the audio decoder stop now!\n");
    return 0;
}

int dtv_patch_input_pause(unsigned int handle)
{
    if (handle == 0) {
        return -1;
    }
    dtv_patch_out *paramout = get_patchout();
    pthread_mutex_lock(&patch_out_mutex);
    audio_decode_pause(paramout->audec);
    paramout->state = DTV_PATCH_STATE_PAUSE;
    pthread_mutex_unlock(&patch_out_mutex);
    return 0;
}

int dtv_patch_input_resume(unsigned int handle)
{
    if (handle == 0) {
        return -1;
    }
    dtv_patch_out *paramout = get_patchout();
    pthread_mutex_lock(&patch_out_mutex);
    audio_decode_resume(paramout->audec);
    paramout->state = DTV_PATCH_STATE_RUNNING;
    pthread_mutex_unlock(&patch_out_mutex);
    return 0;
}
