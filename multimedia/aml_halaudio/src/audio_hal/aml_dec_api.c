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

#define LOG_TAG "aml_dec_api"

#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <sys/time.h>


#include "log.h"
#include "audio_hw_utils.h"
#include "aml_dec_api.h"
#include "aml_dca_dec_api.h"
#include "aml_dcv_dec_api.h"
#include "aml_pcm_dec_api.h"
#include "aml_datmos_api.h"
#include "aml_mat_parser.h"
#include "dolby_lib_api.h"


static int gdolby_strategy = 0;



static aml_dec_func_t * get_decoder_function(audio_format_t format, int dolby_strategy)
{
    switch (format) {
    case AUDIO_FORMAT_AC3:
    case AUDIO_FORMAT_E_AC3: {
        if (dolby_strategy == eDolbyDcvLib) {
            return &aml_dcv_func;
        } else if (dolby_strategy == eDolbyAtmosLib) {
            return &aml_datmos_func;
        } else {
            return NULL;
        }
    }
    case AUDIO_FORMAT_DOLBY_TRUEHD:
        return &aml_datmos_func;
    case AUDIO_FORMAT_DTS:
    case AUDIO_FORMAT_DTS_HD: {
        return &aml_dca_func;
    }
    case AUDIO_FORMAT_PCM_16_BIT:
    case AUDIO_FORMAT_PCM_32_BIT:
    case AUDIO_FORMAT_PCM_8_BIT:
    case AUDIO_FORMAT_PCM_8_24_BIT: {
        if (dolby_strategy == eDolbyAtmosLib)
            return &aml_datmos_func;
        else
            return &aml_pcm_func;
    }
    default:
        return NULL;
    }

    return NULL;
}


int aml_decoder_init(aml_dec_t **ppaml_dec, audio_format_t format, aml_dec_config_t * dec_config)
{
    int ret = -1;
    aml_dec_func_t *dec_fun = NULL;
    gdolby_strategy = detect_dolby_lib_type();
    ALOGI("gdolby_strategy %d\n",gdolby_strategy);
    dec_fun = get_decoder_function(format, gdolby_strategy);
    aml_dec_t *aml_dec_handel = NULL;
    if (dec_fun == NULL) {
        ALOGE("%s got dec_fun as NULL!\n", __func__);
        return -1;
    }

    ALOGD("dec_fun->f_init=%p dolby lib=%d\n", dec_fun->f_init,gdolby_strategy);
    if (dec_fun->f_init) {
        ret = dec_fun->f_init(ppaml_dec, format, dec_config);
        if (ret < 0) {
            return -1;
        }
    } else {
        return -1;
    }

    aml_dec_handel = *ppaml_dec;
    if (aml_dec_handel) {
        aml_dec_handel->dec_info.output_sr = 48000;
        aml_dec_handel->dec_info.output_ch = 2;
        aml_dec_handel->dec_info.output_bitwidth = SAMPLE_16BITS;
        aml_dec_handel->raw_deficiency = 0;
        aml_dec_handel->inbuf_wt = 0;
        aml_dec_handel->burst_payload_size = 0;
        aml_dec_handel->first_PAPB = 0;
        aml_dec_handel->next_PAPB = 0;
        aml_dec_handel->last_consumed_bytes = 0;
        aml_dec_handel->is_truehd_within_mat = 0;
        aml_dec_handel->is_dolby_atmos = 0;

        aml_dec_handel->min_time = 10.0;
        aml_dec_handel->max_time = 0.0;
        aml_dec_handel->average_time = 0.0;
        aml_dec_handel->frame_total_ms = 0;
        aml_dec_handel->dec_total_ms = 0;
        aml_dec_handel->frame_cnt = 0;

    }
    aml_dec_handel->format = format;

    return ret;


}
int aml_decoder_release(aml_dec_t *aml_dec)
{
    int ret = -1;
    aml_dec_func_t *dec_fun = NULL;
    if (aml_dec == NULL) {
        ALOGE("%s aml_dec is NULL\n", __func__);
        return -1;
    }

    dec_fun = get_decoder_function(aml_dec->format, gdolby_strategy);
    if (dec_fun == NULL) {
        return -1;
    }

    if (dec_fun->f_release) {
        dec_fun->f_release(aml_dec);
    } else {
        return -1;
    }

    return ret;


}
int aml_decoder_config(aml_dec_t *aml_dec, aml_dec_config_t * config)
{
    int ret = -1;
    aml_dec_func_t *dec_fun = NULL;
    if (aml_dec == NULL) {
        ALOGE("%s aml_dec is NULL\n", __func__);
        return -1;
    }
    dec_fun = get_decoder_function(aml_dec->format, gdolby_strategy);
    if (dec_fun == NULL) {
        return -1;
    }

    return ret;
}

#define IEC61937_IN_FILE "/tmp/iec61937_in.data"
#define IEC61937_HEADER_BYTES 8

#define STEREO 2
static int pcm_16bits_convert_to_32bits
(   unsigned char* out
    , int out_max_len
    , int *produced_bytes
    , unsigned char *in
    , int in_len
    , int channel_num)
{
    int i = 0;
    int c = 0;
    int16_t *in_buffer = NULL;
    int32_t *out_buffer = NULL;
    int frame_num = 0;


    if (!in || !out || !produced_bytes || (in_len <= 0)) {
        ALOGE("%s fatal error about in params\n", __func__);
        return -1;
    }

    in_buffer = (int16_t *)in;
    out_buffer = (int32_t *)out;
    frame_num = in_len / (sizeof(short) * channel_num);

    if (out_max_len < (frame_num * sizeof(int) *channel_num)) {
        ALOGE("%s fatal error buffer length is not enough\n", __func__);
        return -1;
    }

    for (i = 0; i < frame_num; i++) {
        for (c = 0; c < channel_num; c++)
            out_buffer[channel_num*i + c] = ( (int32_t) (in_buffer[channel_num*i + c]) ) << 16;
    }
    *produced_bytes = frame_num * sizeof(int) * channel_num;

    return 0;
}

int aml_decoder_process(aml_dec_t *aml_dec, unsigned char*buffer, int bytes, int * used_bytes)
{
    int ret = -1;
    aml_dec_func_t *dec_fun = NULL;
    int fill_bytes = 0;
    int parser_raw = 0;
    int offset = 0;
    int dump_iec = 0;
    int check_data = 0;

    if (aml_dec == NULL) {
        ALOGE("%s aml_dec is NULL\n", __func__);
        return -1;
    }
    aml_dec->outlen_pcm = 0;

    if (dump_iec) {
        FILE *fpa = fopen(IEC61937_IN_FILE, "a+");
        // convert_format(aml_dec->inbuf, aml_dec->burst_payload_size);
        fwrite((char *)buffer, 1, bytes, fpa);
        fclose(fpa);
    }

    dec_fun = get_decoder_function(aml_dec->format, gdolby_strategy);
    if (dec_fun == NULL) {
        return -1;
    }

    if (gdolby_strategy == eDolbyDcvLib) {
        if (dec_fun->f_process) {
            ret = dec_fun->f_process(aml_dec, buffer, bytes);
        } else {
            return -1;
        }
        *used_bytes = bytes;
        return ret;


    }


    if (aml_dec->is_iec61937 == 0) {
        if (dec_fun->f_process) {
            // below code is temparily for Dolby
            if ((gdolby_strategy == eDolbyAtmosLib) && audio_is_linear_pcm(aml_dec->format)) {
                if (aml_dec->inbuf_wt + bytes > aml_dec->inbuf_max_len) {
                    ALOGE("%s line %d Fatal error, buffer is overwrite!\n", __func__, __LINE__);
                    aml_dec->inbuf_wt = 0;
                }
                /* here we use the stereo/16bits pcm*/
                if (aml_dec->inbuf) {
                    memcpy(aml_dec->inbuf + aml_dec->inbuf_wt, buffer, bytes);
                    aml_dec->inbuf_wt += bytes;
                    aml_dec->burst_payload_size += bytes;
                }

                int bytes_per_frame = aml_dec->dec_info.stream_bitwidth/8;
                if (aml_dec->inbuf_wt / bytes_per_frame < 1536) {
                    *used_bytes = bytes;
                    return 0;
                }

            }
            else if (aml_dec->inbuf) {
                memcpy(aml_dec->inbuf, buffer, bytes);
                aml_dec->burst_payload_size = bytes;
                aml_dec->inbuf_wt = bytes;
            }
            ret = dec_fun->f_process(aml_dec, buffer, bytes);
        } else {
            return -1;
        }
        *used_bytes = bytes;
        return ret;
    }


    if (aml_dec->format == AUDIO_FORMAT_DOLBY_TRUEHD) {
        aml_dec->next_PAPB = aml_dec->first_PAPB;
    }

    fill_bytes = fill_in_the_remaining_data(buffer, bytes, &(aml_dec->raw_deficiency), aml_dec->inbuf, &aml_dec->inbuf_wt, aml_dec->inbuf_max_len);
    // ALOGE("bytes %#x fill_bytes %#x\n", bytes, fill_bytes);
    /*
     *if fill_bytes = -1, inbuf valid space is not enough, better to increase the length.
     *otherwise, use IEC61937 decoder to get the payload audio data.
     */
    if ((fill_bytes >= 0) && (bytes - fill_bytes > 0)) {
        int got_format = 0;
        aml_dec->last_consumed_bytes += fill_bytes;
        parser_raw = decode_IEC61937_to_raw_data(buffer + fill_bytes
                     , bytes - fill_bytes
                     , aml_dec->inbuf
                     , &aml_dec->inbuf_wt
                     , aml_dec->inbuf_max_len
                     , &(aml_dec->raw_deficiency)
                     , &(aml_dec->burst_payload_size)
                     , &offset
                     , &got_format);

        // ALOGE("parser_raw %#x offset %#x got_format %d\n", parser_raw, offset, got_format);
        if (got_format == TRUEHD) {
            int new_truehd_bytes = (parser_raw - (offset + IEC61937_HEADER_BYTES));
            if (check_data) {
                char *mat_header = aml_dec->inbuf + aml_dec->inbuf_wt - new_truehd_bytes;
                // ALOGE("inbuf_wt %#x bytes %#x fill_bytes %#x offset %#x mat_header(offset=%#x)",
                //     aml_dec->inbuf_wt, bytes, fill_bytes, offset, aml_dec->inbuf_wt - (bytes - fill_bytes - offset));
                ALOGE("mat_header %2x %2x", mat_header[0], mat_header[1]);
            }

            /*PA | PB | PC | PD | burst payload | burst spacing*/
            /*truehd/mat iec61937 len is 61440bytes*/
            aml_dec->last_consumed_bytes += offset;
            if ((aml_dec->last_consumed_bytes != IEC61937_MAT_BYTES)
                /* || \
                 *((aml_dec->inbuf_wt > aml_dec->burst_payload_size) && ((aml_dec->inbuf_wt - new_truehd_bytes) % aml_dec->burst_payload_size != 0))
                 */
               ) {
                ALOGE("@@end last_consumed_bytes %#x wt %#x new %#x burst %#x\n", aml_dec->last_consumed_bytes, aml_dec->inbuf_wt, new_truehd_bytes, aml_dec->burst_payload_size);
                /*
                 *new store data is right, so remove the early data
                 */
                memmove(aml_dec->inbuf
                        , aml_dec->inbuf + aml_dec->inbuf_wt - new_truehd_bytes
                        , new_truehd_bytes);
                aml_dec->inbuf_wt = new_truehd_bytes;
                // char *sync_header = aml_dec->inbuf;
                // ALOGE("sync_header %2x %2x", sync_header[0], sync_header[1]);
            }

            memcpy(&(aml_dec->first_PAPB), buffer + fill_bytes + offset, sizeof(aml_dec->first_PAPB));
            // ALOGE("##begin first_PAPB %#x fill_bytes %#x offset %#x\n", aml_dec->first_PAPB, fill_bytes, offset);
            aml_dec->last_consumed_bytes = bytes - fill_bytes - offset;
        } else {
            /*suppose it as the burst spacing*/
            aml_dec->last_consumed_bytes += parser_raw;
        }
        // ALOGE("last_consumed_bytes %#x\n", aml_dec->last_consumed_bytes);
    } else {
        aml_dec->last_consumed_bytes += fill_bytes;
        // ALOGE("last_consumed_bytes %#x\n", aml_dec->last_consumed_bytes);
    }

    if (dec_fun->f_process) {
        if (check_data && (aml_dec->inbuf_wt >= aml_dec->burst_payload_size) && (aml_dec->burst_payload_size > 0)) {
            char *dec_data = aml_dec->inbuf;
            ALOGE("header %2x %2x", dec_data[0], dec_data[1]);
        }
        ret = dec_fun->f_process(aml_dec, buffer, bytes);
    } else {
        return -1;
    }

    // change it later
    *used_bytes = bytes;
    return ret;

}

int aml_decoder_dynamic_param_set(aml_dec_t *aml_dec)
{
    int ret = -1;
    aml_dec_func_t *dec_fun = NULL;
    if (aml_dec == NULL) {
        ALOGE("%s aml_dec is NULL\n", __func__);
        return -1;
    }

    dec_fun = get_decoder_function(aml_dec->format, gdolby_strategy);
    if (dec_fun == NULL) {
        return -1;
    }

    if (dec_fun->f_dynamic_param_set) {
        ret = dec_fun->f_dynamic_param_set(aml_dec);
    } else {
        return -1;
    }

    return ret;


}

