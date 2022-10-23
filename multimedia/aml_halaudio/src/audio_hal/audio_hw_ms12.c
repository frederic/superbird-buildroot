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

//#ifdef DOLBY_MS12_ENABLE
#define LOG_TAG "audio_hw_primary"
//#define LOG_NDEBUG 0

#include "log.h"
#include <dolby_ms12.h>
#include <dolby_ms12_config_params.h>
#include <dolby_ms12_status.h>
#include <aml_android_utils.h>
#include <sys/prctl.h>
#include "properties.h"

#include "audio_hw_ms12.h"
#include "alsa_config_parameters.h"
#include "aml_ac3_parser.h"
#include <sound/asound.h>
#include <tinyalsa/asoundlib.h>
#include "audio_hw.h"
#include "alsa_manager.h"
#include "aml_audio_stream.h"

#define DOLBY_MS12_OUTPUT_FORMAT_TEST

#define DOLBY_DRC_LINE_MODE 0
#define DOLBY_DRC_RF_MODE   1

#define DDP_MAX_BUFFER_SIZE 2560//dolby ms12 input buffer threshold

#define MS12_OUTPUT_PCM_FILE "/data/audio_out/ms12_pcm.raw"
#define MS12_OUTPUT_BITSTREAM_FILE "/data/audio_out/ms12_bitstream.raw"

/*
 *@brief dump ms12 output data
 */
static void dump_ms12_output_data(void *buffer, int size, char *file_name)
{
    if (aml_getprop_bool("media.audiohal.outdump")) {
        FILE *fp1 = fopen(file_name, "a+");
        if (fp1) {
            int flen = fwrite((char *)buffer, 1, size, fp1);
            ALOGV("%s buffer %p size %d\n", __FUNCTION__, buffer, size);
            fclose(fp1);
        }
    }
}

static void *dolby_ms12_threadloop(void *data);

int dolby_ms12_register_callback(struct aml_stream_out *aml_out)
{
    ALOGI("\n+%s()", __FUNCTION__);
    struct aml_audio_device *adev = aml_out->dev;
    int ret = 0;
#ifdef REPLACE_OUTPUT_BUFFER_WITH_CALLBACK
    if (aml_out->dual_output_flag) {
        /*dual output, output format contains both AUDIO_FORMAT_PCM_16_BIT and AUDIO_FORMAT_AC3*/
        if (adev->sink_format == AUDIO_FORMAT_PCM_16_BIT) {
            ret = dolby_ms12_register_pcm_callback(pcm_output, (void *)aml_out);
            ALOGI("%s() dolby_ms12_register_pcm_callback return %d", __FUNCTION__, ret);
        }
        if (adev->optical_format == AUDIO_FORMAT_AC3) {
            ret = dolby_ms12_register_bitstream_callback(bitstream_output, (void *)aml_out);
            ALOGI("%s() dolby_ms12_register_bitstream_callback return %d", __FUNCTION__, ret);
        }
    } else {
        /*Single output, output format is AUDIO_FORMAT_PCM_16_BIT or AUDIO_FORMAT_AC3 or AUDIO_Format_E_AC3*/
        if (adev->sink_format == AUDIO_FORMAT_PCM_16_BIT) {
            ret = dolby_ms12_register_pcm_callback(pcm_output, (void *)aml_out);
            ALOGI("%s() dolby_ms12_register_pcm_callback return %d", __FUNCTION__, ret);
        } else {
            ret = dolby_ms12_register_bitstream_callback(bitstream_output, (void *)aml_out);
            ALOGI("%s() dolby_ms12_register_bitstream_callback return %d", __FUNCTION__, ret);
        }
    }
    ALOGI("-%s() ret %d\n\n", __FUNCTION__, ret);
    return ret;
#endif
}

/*
 *@brief get dolby ms12 prepared
 */
int get_the_dolby_ms12_prepared(
    struct aml_stream_out *aml_out
    , audio_format_t input_format
    , audio_channel_mask_t input_channel_mask
    , int input_sample_rate)
{
    ALOGI("+%s()", __FUNCTION__);
    struct aml_audio_device *adev = aml_out->dev;
    struct dolby_ms12_desc *ms12 = &(adev->ms12);
    int dolby_ms12_drc_mode = DOLBY_DRC_RF_MODE;
    struct aml_stream_out *out;
    ALOGI("\n+%s()", __FUNCTION__);
    pthread_mutex_lock(&ms12->lock);
    ALOGI("++%s(), locked", __FUNCTION__);
    set_audio_system_format(AUDIO_FORMAT_PCM_16_BIT);
    set_audio_main_format(input_format);
    ALOGI("+%s() dual_decoder_support %d\n", __FUNCTION__, adev->dual_decoder_support);

#ifdef DOLBY_MS12_OUTPUT_FORMAT_TEST
    {
        char buf[PROPERTY_VALUE_MAX];
        int prop_ret = -1;
        int out_format = 0;
        prop_ret = property_get("dolby.ms12.output.format", buf, NULL);
        if (prop_ret > 0) {
            out_format = atoi(buf);
            if (out_format == 0) {
                adev->sink_format = AUDIO_FORMAT_PCM_16_BIT;
                ALOGI("DOLBY_MS12_OUTPUT_FORMAT_TEST %d\n", (unsigned int)adev->sink_format);
            } else if (out_format == 1) {
                adev->sink_format = AUDIO_FORMAT_AC3;
                ALOGI("DOLBY_MS12_OUTPUT_FORMAT_TEST %d\n", (unsigned int)adev->sink_format);
            } else if (out_format == 2) {
                adev->sink_format = AUDIO_FORMAT_E_AC3;
                ALOGI("DOLBY_MS12_OUTPUT_FORMAT_TEST %d\n", (unsigned int)adev->sink_format);
            }
        }
    }
#endif

    if ((input_format == AUDIO_FORMAT_AC3) || (input_format == AUDIO_FORMAT_E_AC3)) {
        dolby_ms12_drc_mode = DOLBY_DRC_RF_MODE;
    } else {
        dolby_ms12_drc_mode = DOLBY_DRC_LINE_MODE;
    }

    /*set the associate audio format*/
    if (adev->dual_decoder_support == true) {
        set_audio_associate_format(input_format);
        ALOGI("%s set_audio_associate_format %#x", __FUNCTION__, input_format);
        dolby_ms12_set_asscociated_audio_mixing(adev->associate_audio_mixing_enable);
        dolby_ms12_set_user_control_value_for_mixing_main_and_associated_audio(adev->mixing_level);
        ALOGI("%s associate_audio_mixing_enable %d mixing_level set to %d\n",
              __FUNCTION__, adev->associate_audio_mixing_enable, adev->mixing_level);
        // fix for -xs configration
        //int ms12_runtime_update_ret = aml_ms12_update_runtime_params(&(adev->ms12));
        //ALOGI("aml_ms12_update_runtime_params return %d\n", ms12_runtime_update_ret);
    }
    dolby_ms12_set_drc_mode(dolby_ms12_drc_mode);
    ALOGI("%s dolby_ms12_set_drc_mode %s", __FUNCTION__, (dolby_ms12_drc_mode == DOLBY_DRC_RF_MODE) ? "RF MODE" : "LINE MODE");
    int ret = 0;

    /*set the continous output flag*/
    set_dolby_ms12_continuous_mode((bool)adev->continuous_audio_mode);
    /* create  the ms12 output stream here */
    /*************************************/
    if (continous_mode(adev)) {
        // TODO: zz: Might have memory leak, not clear route to release this pointer
        out = (struct aml_stream_out *)calloc(1, sizeof(struct aml_stream_out));
        if (!out) {
            ALOGE("%s malloc  stream failed failed", __func__);
            return -ENOMEM;
        }
        /* copy stream information */
        memcpy(out, aml_out, sizeof(struct aml_stream_out));
        if (out->is_tv_platform) {
            out->config.channels = 8;
            out->config.format = PCM_FORMAT_S32_LE;
            out->tmp_buffer_8ch = malloc(out->config.period_size * 4 * 8);
            if (out->tmp_buffer_8ch == NULL) {
                free(out);
                ALOGE("%s cannot malloc memory for out->tmp_buffer_8ch", __func__);
                return -ENOMEM;
            }
            out->tmp_buffer_8ch_size = out->config.period_size * 4 * 8;
            out->audioeffect_tmp_buffer = malloc(out->config.period_size * 6);
            if (out->audioeffect_tmp_buffer == NULL) {
                free(out->tmp_buffer_8ch);
                free(out);
                ALOGE("%s cannot malloc memory for audioeffect_tmp_buffer", __func__);
                return -ENOMEM;
            }
        }
        ALOGI("%s create ms12 stream %p,original stream %p", __func__, out, aml_out);
    } else {
        out = aml_out;
    }
    adev->ms12_out = out;
    /************end**************/
    /*set the system app sound mixing enable*/
    if (adev->continuous_audio_mode) {
        adev->system_app_mixing_status = SYSTEM_APP_SOUND_MIXING_ON;
    }
    dolby_ms12_set_system_app_audio_mixing(adev->system_app_mixing_status);

    //init the dolby ms12
    if (out->dual_output_flag) {
        dolby_ms12_set_dual_output_flag(out->dual_output_flag);
        aml_ms12_config(ms12, input_format, input_channel_mask, input_sample_rate, adev->optical_format);
    } else {
        dolby_ms12_set_dual_output_flag(out->dual_output_flag);
        aml_ms12_config(ms12, input_format, input_channel_mask, input_sample_rate, adev->sink_format);
    }
    if (ms12->dolby_ms12_enable) {
        //register Dolby MS12 callback
        dolby_ms12_register_callback(out);
        ms12->device = usecase_device_adapter_with_ms12(out->device, adev->sink_format);
        ALOGI("%s out [dual_output_flag %d] adev [format sink %#x optical %#x] ms12 [output-format %#x device %d]",
              __FUNCTION__, out->dual_output_flag, adev->sink_format, adev->optical_format, ms12->output_format, ms12->device);
        memcpy((void *) & (adev->ms12_config), (const void *) & (out->config), sizeof(struct pcm_config));
        get_hardware_config_parameters(
            &(adev->ms12_config)
            , adev->sink_format
            , audio_channel_count_from_out_mask(ms12->output_channelmask)
            , ms12->output_samplerate
            , out->is_tv_platform);

        if (continous_mode(adev)) {
            ms12->dolby_ms12_thread_exit = false;
            ret = pthread_create(&(ms12->dolby_ms12_threadID), NULL, &dolby_ms12_threadloop, out);
            if (ret != 0) {
                ALOGE("%s, Create dolby_ms12_thread fail!\n", __FUNCTION__);
                goto Err_dolby_ms12_thread;
            }
            ALOGI("%s() thread is builded, get dolby_ms12_threadID %ld\n", __FUNCTION__, ms12->dolby_ms12_threadID);
        }
    }
    ALOGI("--%s(), locked", __FUNCTION__);
    pthread_mutex_unlock(&ms12->lock);
    ALOGI("-%s()\n\n", __FUNCTION__);
    return ret;

Err_dolby_ms12_thread:
    if (continous_mode(adev)) {
        ALOGE("%s() %d exit dolby_ms12_thread\n", __FUNCTION__, __LINE__);
        ms12->dolby_ms12_thread_exit = true;
        ms12->dolby_ms12_threadID = 0;
        free(out->tmp_buffer_8ch);
        free(out->audioeffect_tmp_buffer);
        free(out);
    }

    pthread_mutex_unlock(&ms12->lock);
    return ret;
}

static bool is_iec61937_format(struct audio_stream_out *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = aml_out->dev;

    /*
     *Attation, the DTV input frame(format/size) is IEC61937.
     *but the dd/ddp of HDMI-IN, has same format as IEC61937 but size do not match.
     *Fixme: in Kodi APK, audio passthrough choose AUDIO_FORMAT_IEC61937.
    */
    return ((adev->patch_src == SRC_DTV) && \
            ((aml_out->flags & AUDIO_OUTPUT_FLAG_IEC958_NONAUDIO) || (aml_out->hal_format == AUDIO_FORMAT_IEC61937)));
}
/*
 *@brief dolby ms12 main process
 *
 * input parameters
 *     stream: audio_stream_out handle
 *     buffer: data buffer address
 *     bytes: data size
 * output parameters
 *     use_size: buffer used size
 */

int dolby_ms12_main_process(
    struct audio_stream_out *stream
    , const void *buffer
    , size_t bytes
    , size_t *use_size)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = aml_out->dev;
    struct dolby_ms12_desc *ms12 = &(adev->ms12);
    int ms12_output_size = 0;
    int dolby_ms12_input_bytes = 0;
    void *output_buffer = NULL;
    size_t output_buffer_bytes = 0;

    void *input_buffer = (void *)buffer;
    size_t input_bytes = bytes;
    int dual_decoder_used_bytes = 0;
    int single_decoder_used_bytes = 0;
    void *main_frame_buffer = input_buffer;/*input_buffer as default*/
    int main_frame_size = input_bytes;/*input_bytes as default*/
    void *associate_frame_buffer = NULL;
    int associate_frame_size = 0;
    size_t main_frame_deficiency = 0;

    if (adev->debug_flag >= 2) {
        ALOGI("\n%s() in continuous %d input ms12 bytes %d input bytes %zu\n",
              __FUNCTION__, adev->continuous_audio_mode, dolby_ms12_input_bytes, input_bytes);
    }

    //dtv single decoder, if input data is less than one iec61937 size, and do not contain one complete frame
    //after adding the frame_deficiency, got a complete frame without scan the frame
    //keyword: frame_deficiency
    if (!adev->dual_decoder_support
        && (is_iec61937_format(stream) || aml_out->hal_format == AUDIO_FORMAT_IEC61937)
        && (aml_out->frame_deficiency > 0)) {
        //ALOGI("\n%s() frame_deficiency = %d , input bytes = %d\n",__FUNCTION__, aml_out->frame_deficiency , input_bytes);
        if (aml_out->frame_deficiency <= input_bytes) {
            main_frame_size = aml_out->frame_deficiency;
            single_decoder_used_bytes = aml_out->frame_deficiency;
            aml_out->frame_deficiency = 0;
        } else {
            main_frame_size = input_bytes;
            single_decoder_used_bytes = input_bytes;
            aml_out->frame_deficiency -= input_bytes;
        }
        goto MAIN_INPUT;
    }

    if (ms12->dolby_ms12_enable) {
        //ms12 input main
        int dual_input_ret = 0;
        pthread_mutex_lock(&ms12->main_lock);
        if (adev->dual_decoder_support == true) {
            dual_input_ret = scan_dolby_main_associate_frame(input_buffer
                             , input_bytes
                             , &dual_decoder_used_bytes
                             , &main_frame_buffer
                             , &main_frame_size
                             , &associate_frame_buffer
                             , &associate_frame_size);
            if (dual_input_ret) {
                ALOGE("%s used size %zu dont find the iec61937 format header, rescan next time!\n", __FUNCTION__, *use_size);
                goto  exit;
            }
        }
        /*
        As the audio payload may cross two write process,we can not skip the
        data when we do not get a complete payload.for ATSC,as we have a
        complete burst align for 6144/24576,so we always can find a valid
        payload in one write process.
        */
        else if (is_iec61937_format(stream)) {
            //keyword: frame_deficiency
            int single_input_ret = scan_dolby_main_frame_ext(input_buffer
                                   , input_bytes
                                   , &single_decoder_used_bytes
                                   , &main_frame_buffer
                                   , &main_frame_size
                                   , &main_frame_deficiency);
            if (single_input_ret) {
                ALOGE("%s used size %zu dont find the iec61937 format header, rescan next time!\n", __FUNCTION__, *use_size);
                goto  exit;
            }
            if (main_frame_deficiency > 0) {
                main_frame_size = main_frame_size - main_frame_deficiency;
            }
            aml_out->frame_deficiency = main_frame_deficiency;
        }

        if (adev->dual_decoder_support == true) {
            /*if there is associate frame, send it to dolby ms12.*/
            char tmp_array[4096] = {0};
            if (!associate_frame_buffer || (associate_frame_size == 0)) {
                associate_frame_buffer = (void *)&tmp_array[0];
                associate_frame_size = sizeof(tmp_array);
            }
            if (associate_frame_size < main_frame_size) {
                ALOGV("%s() main frame addr %p size %d associate frame addr %p size %d, need a larger ad input size!\n",
                      __FUNCTION__, main_frame_buffer, main_frame_size, associate_frame_buffer, associate_frame_size);
                memcpy(&tmp_array[0], associate_frame_buffer, associate_frame_size);
                associate_frame_size = sizeof(tmp_array);
            }
            dolby_ms12_input_associate(ms12->dolby_ms12_ptr
                                       , (const void *)associate_frame_buffer
                                       , (size_t)associate_frame_size
                                       , ms12->input_config_format
                                       , audio_channel_count_from_out_mask(ms12->config_channel_mask)
                                       , ms12->config_sample_rate
                                      );
        }

MAIN_INPUT:
        if (main_frame_buffer && (main_frame_size > 0)) {
            /*input main frame*/
            int main_format = ms12->input_config_format;
            int main_channel_num = audio_channel_count_from_out_mask(ms12->config_channel_mask);
            int main_sample_rate = ms12->config_sample_rate;
            if ((dolby_ms12_get_dolby_main1_file_is_dummy() == true) && \
                (dolby_ms12_get_ott_sound_input_enable() == true) && \
                (adev->continuous_audio_mode == 1)) {
                //hwsync pcm, 16bits-stereo
                main_format = AUDIO_FORMAT_PCM_16_BIT;
                main_channel_num = 2;
                main_sample_rate = 48000;
            }
            dolby_ms12_input_bytes =
                dolby_ms12_input_main(
                    ms12->dolby_ms12_ptr
                    , main_frame_buffer
                    , main_frame_size
                    , main_format
                    , main_channel_num
                    , main_sample_rate);

            if (adev->continuous_audio_mode == 0) {
                dolby_ms12_scheduler_run(ms12->dolby_ms12_ptr);
            }

            if (dolby_ms12_input_bytes > 0) {
                if (adev->dual_decoder_support == true) {
                    *use_size = dual_decoder_used_bytes;
                } else {
                    if (adev->debug_flag >= 2) {
                        ALOGI("%s() continuous %d input ms12 bytes %d input bytes %zu sr %d main size %d parser size %d\n\n",
                              __FUNCTION__, adev->continuous_audio_mode, dolby_ms12_input_bytes, input_bytes, ms12->config_sample_rate, main_frame_size, single_decoder_used_bytes);
                    }
                    if (adev->continuous_audio_mode == 1) {
                        //FIXME, if ddp input, the size suppose as CONTINUOUS_OUTPUT_FRAME_SIZE
                        //if pcm input, suppose 2ch/16bits/48kHz
                        int need_sleep_us = 0;
                        if ((aml_out->hal_format == AUDIO_FORMAT_AC3) || \
                            (aml_out->hal_format == AUDIO_FORMAT_E_AC3)) {
                            /*
                             hwsync mode,normally we write a frame payload one time.the duration is 32 ms.
                             as we write the data into the ringbuf,we need sleep for a while to avoid
                             audioflinger write too often and cost high cpu loading.
                             */
                            if (aml_out->hw_sync_mode) {
                                need_sleep_us = 32 * 1000 / 2;
                            } else {
                                need_sleep_us = ((input_bytes * 32 * 1000) / aml_out->ddp_frame_size);
                            }
                            need_sleep_us = need_sleep_us / 2;
                        } else {
                            /*
                            for LPCM audio,we support it is 2 ch 48K audio.
                            */
                            if (dolby_ms12_input_bytes > 0) {
                                need_sleep_us = (dolby_ms12_input_bytes * 1000000 / 4 / (ms12->config_sample_rate) * 5 / 6);
                            }
                        }
                        if (need_sleep_us > 0) {
                            usleep(need_sleep_us);
                            if (adev->debug_flag >= 2) {
                                ALOGI("%s sleep %d ms\n", __FUNCTION__, need_sleep_us / 1000);
                            }
                        }
                    }

                    if (is_iec61937_format(stream)) {
                        *use_size = single_decoder_used_bytes;
                    } else {
                        *use_size = dolby_ms12_input_bytes;
                    }

                }
            }
        } else {
            if (adev->dual_decoder_support == true) {
                *use_size = dual_decoder_used_bytes;
            } else {
                *use_size = input_bytes;
            }
        }
exit:
        pthread_mutex_unlock(&ms12->main_lock);
        return 0;
    } else {
        return -1;
    }
}


/*
 *@brief dolby ms12 system process
 *
 * input parameters
 *     stream: audio_stream_out handle
 *     buffer: data buffer address
 *     bytes: data size
 * output parameters
 *     use_size: buffer used size
 */
int dolby_ms12_system_process(
    struct audio_stream_out *stream
    , const void *buffer
    , size_t bytes
    , size_t *use_size)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = aml_out->dev;
    struct dolby_ms12_desc *ms12 = &(adev->ms12);
    audio_channel_mask_t mixer_default_channelmask = AUDIO_CHANNEL_OUT_STEREO;
    int mixer_default_samplerate = 48000;
    int dolby_ms12_input_bytes = 0;
    int ms12_output_size = 0;
    pthread_mutex_lock(&ms12->lock);
    if (ms12->dolby_ms12_enable) {
        //Dual input, here get the system data
        dolby_ms12_input_bytes =
            dolby_ms12_input_system(
                ms12->dolby_ms12_ptr
                , buffer
                , bytes
                , AUDIO_FORMAT_PCM_16_BIT
                , audio_channel_count_from_out_mask(mixer_default_channelmask)
                , mixer_default_samplerate);
        if (dolby_ms12_input_bytes > 0) {
            *use_size = dolby_ms12_input_bytes;
        }
    }
    pthread_mutex_unlock(&ms12->lock);
    return 0;
}

/*
 *@brief get dolby ms12 cleanup
 */
int get_dolby_ms12_cleanup(struct dolby_ms12_desc *ms12)
{
    int is_quit = 1;

    ALOGI("+%s()", __FUNCTION__);
    if (!ms12) {
        return -EINVAL;
    }

    pthread_mutex_lock(&ms12->lock);
    pthread_mutex_lock(&ms12->main_lock);
    ALOGI("++%s(), locked", __FUNCTION__);
    ALOGI("%s() dolby_ms12_set_quit_flag %d", __FUNCTION__, is_quit);
    dolby_ms12_set_quit_flag(is_quit);

    if (ms12->dolby_ms12_threadID != 0) {
        ms12->dolby_ms12_thread_exit = true;
        int ms12_runtime_update_ret = aml_ms12_update_runtime_params(ms12);
        ALOGI("aml_ms12_update_runtime_params return %d\n", ms12_runtime_update_ret);
        pthread_join(ms12->dolby_ms12_threadID, NULL);
        ms12->dolby_ms12_threadID = 0;
        ALOGI("%s() dolby_ms12_threadID reset to %ld\n", __FUNCTION__, ms12->dolby_ms12_threadID);
    }
    set_audio_system_format(AUDIO_FORMAT_INVALID);
    set_audio_main_format(AUDIO_FORMAT_INVALID);
    dolby_ms12_flush_main_input_buffer();
    dolby_ms12_config_params_set_system_flag(false);
    aml_ms12_cleanup(ms12);
    ms12->output_format = AUDIO_FORMAT_INVALID;
    ms12->dolby_ms12_enable = false;
    ALOGI("--%s(), locked", __FUNCTION__);
    pthread_mutex_unlock(&ms12->main_lock);
    pthread_mutex_unlock(&ms12->lock);
    ALOGI("-%s()", __FUNCTION__);
    return 0;
}

/*
 *@brief set dolby ms12 primary gain
 */
int set_dolby_ms12_primary_input_db_gain(struct dolby_ms12_desc *ms12, int db_gain)
{
    MixGain gain;
    int ret = 0;

    ALOGI("+%s(): gain %ddb, ms12 enable(%d)",
          __FUNCTION__, db_gain, ms12->dolby_ms12_enable);
    if (!ms12) {
        return -EINVAL;
    }

    pthread_mutex_lock(&ms12->lock);
    if (!ms12->dolby_ms12_enable) {
        ret = -EINVAL;
        goto exit;
    }

    gain.target = db_gain;
    gain.duration = 10;
    gain.shape = 0;
    dolby_ms12_set_system_sound_mixer_gain_values_for_primary_input(&gain);
    dolby_ms12_set_input_mixer_gain_values_for_main_program_input(&gain);
    //Fixme when tunnel mode is working, the Alexa start and mute the main input!
    //dolby_ms12_set_input_mixer_gain_values_for_ott_sounds_input(&gain);
    ret = aml_ms12_update_runtime_params(ms12);

exit:
    pthread_mutex_unlock(&ms12->lock);
    return ret;
}

#ifdef REPLACE_OUTPUT_BUFFER_WITH_CALLBACK
int pcm_output(void *buffer, void *priv_data, size_t size)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)priv_data;
    struct aml_audio_device *adev = aml_out->dev;
    void *output_buffer = NULL;
    size_t output_buffer_bytes = 0;
    audio_format_t output_format = AUDIO_FORMAT_PCM_16_BIT;
    aml_data_format_t data_format;
    int ret = 0;

    if (adev->debug_flag > 1) {
        ALOGI("+%s() size %zu", __FUNCTION__, size);
    }

    /*dump ms12 pcm output*/
    dump_ms12_output_data(buffer, size, MS12_OUTPUT_PCM_FILE);

    data_format.format = output_format;
    data_format.sr     = 48000;
    data_format.ch     = 2;
    data_format.bitwidth = SAMPLE_16BITS;
    if (0 == audio_hal_data_processing((struct audio_stream_out *)aml_out, buffer, size
        , &output_buffer, &output_buffer_bytes, &data_format) == 0) {
        ret = hw_write((struct audio_stream_out *)aml_out, output_buffer, output_buffer_bytes, output_format);
    }

    if (adev->debug_flag > 1) {
        ALOGI("-%s() ret %d", __FUNCTION__, ret);
    }

    return ret;
}

int bitstream_output(void *buffer, void *priv_data, size_t size)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)priv_data;
    struct aml_audio_device *adev = aml_out->dev;
    void *output_buffer = NULL;
    size_t output_buffer_bytes = 0;
    audio_format_t output_format = AUDIO_FORMAT_AC3;
    int ret = 0;

    if (adev->debug_flag > 1) {
        ALOGI("+%s() size %zu", __FUNCTION__, size);
    }

    /*dump ms12 bitstream output*/
    dump_ms12_output_data(buffer, size, MS12_OUTPUT_BITSTREAM_FILE);

    if (aml_out->dual_output_flag) {
        output_format = adev->optical_format;
        struct audio_stream_out *stream_out = (struct audio_stream_out *)aml_out;
        ret = aml_audio_spdif_output(stream_out, buffer, size);
    } else {
        output_format = adev->sink_format;
        aml_data_format_t data_format;
        data_format.format = output_format;
        data_format.sr     = 48000;
        data_format.ch     = 2;
        data_format.bitwidth = SAMPLE_16BITS;
        if (0 == audio_hal_data_processing((struct audio_stream_out *)aml_out, buffer
            , size, &output_buffer, &output_buffer_bytes, &data_format) == 0) {
            ret = hw_write((struct audio_stream_out *)aml_out, output_buffer, output_buffer_bytes, output_format);
        }
    }

    if (adev->debug_flag > 1) {
        ALOGI("-%s() ret %d", __FUNCTION__, ret);
    }

    return ret;
}
#endif

static void *dolby_ms12_threadloop(void *data)
{
    ALOGI("+%s() ", __FUNCTION__);
    struct aml_stream_out *aml_out = (struct aml_stream_out *)data;
    struct aml_audio_device *adev = aml_out->dev;
    struct dolby_ms12_desc *ms12 = &(adev->ms12);
    if (ms12 == NULL) {
        ALOGE("%s ms12 pointer invalid!", __FUNCTION__);
        goto Error;
    }

    if (ms12->dolby_ms12_enable) {
        dolby_ms12_set_quit_flag(ms12->dolby_ms12_thread_exit);
    }

    prctl(PR_SET_NAME, (unsigned long)"DOLBY_MS12");

    while ((ms12->dolby_ms12_thread_exit == false) && (ms12->dolby_ms12_enable)) {
        ALOGV("%s() goto dolby_ms12_scheduler_run", __FUNCTION__);
        if (ms12->dolby_ms12_ptr) {
            dolby_ms12_scheduler_run(ms12->dolby_ms12_ptr);
        } else {
            ALOGE("%s() ms12->dolby_ms12_ptr is NULL, fatal error!", __FUNCTION__);
            break;
        }
        ALOGV("%s() dolby_ms12_scheduler_run end", __FUNCTION__);
    }
    ALOGI("%s remove   ms12 stream %p", __func__, aml_out);
    if (continous_mode(adev)) {
        pthread_mutex_lock(&adev->alsa_pcm_lock);
        aml_alsa_output_close((struct audio_stream_out*)aml_out);
        struct pcm *pcm = adev->pcm_handle[DIGITAL_DEVICE];
        if (aml_out->dual_output_flag && pcm) {
            ALOGI("%s close dual output pcm handle %p", __func__, pcm);
            pcm_close(pcm);
            adev->pcm_handle[DIGITAL_DEVICE] = NULL;
            aml_out->dual_output_flag = 0;
        }
        pthread_mutex_unlock(&adev->alsa_pcm_lock);
        release_audio_stream((struct audio_stream_out *)aml_out);
    }
    adev->ms12_out = NULL;
    ALOGI("-%s(), exit dolby_ms12_thread\n", __FUNCTION__);
    return ((void *)0);

Error:
    ALOGI("-%s(), exit dolby_ms12_thread, because of erro input params\n", __FUNCTION__);
    return ((void *)0);
}

int set_system_app_mixing_status(struct aml_stream_out *aml_out, int stream_status)
{
    struct aml_audio_device *adev = aml_out->dev;
    struct dolby_ms12_desc *ms12 = &(adev->ms12);
    int system_app_mixing_status = SYSTEM_APP_SOUND_MIXING_OFF;
    int ret = 0;

    if (STREAM_STANDBY == stream_status) {
        system_app_mixing_status = SYSTEM_APP_SOUND_MIXING_OFF;
    } else {
        system_app_mixing_status = SYSTEM_APP_SOUND_MIXING_ON;
    }

    //when under continuous_audio_mode, system app sound mixing always on.
    if (adev->continuous_audio_mode) {
        system_app_mixing_status = SYSTEM_APP_SOUND_MIXING_ON;
    }

    adev->system_app_mixing_status = system_app_mixing_status;

    if (adev->debug_flag) {
        ALOGI("%s stream-status %d set system-app-audio-mixing %d current %d continuous_audio_mode %d\n", __func__,
              stream_status, system_app_mixing_status, dolby_ms12_get_system_app_audio_mixing(), adev->continuous_audio_mode);
    }

    dolby_ms12_set_system_app_audio_mixing(system_app_mixing_status);

    if (ms12->dolby_ms12_enable) {
        pthread_mutex_lock(&ms12->lock);
        ret = aml_ms12_update_runtime_params(&(adev->ms12));
        pthread_mutex_unlock(&ms12->lock);
        ALOGI("%s return %d stream-status %d set system-app-audio-mixing %d\n",
              __func__, ret, stream_status, system_app_mixing_status);
        return ret;
    }

    return 1;
}

//#endif
