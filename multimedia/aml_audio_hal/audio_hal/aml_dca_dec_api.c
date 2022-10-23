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

#define LOG_TAG "aml_audio_dca_dec"
//#define LOG_NDEBUG 0

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
#include <sound/asound.h>
#include <cutils/log.h>
#include <tinyalsa/asoundlib.h>

#include "audio_hw_utils.h"
#include "aml_dca_dec_api.h"
#include "aml_audio_resample_manager.h"

enum {
    EXITING_STATUS = -1001,
    NO_ENOUGH_DATA = -1002,
};
#define MAX_DECODER_FRAME_LENGTH 32768
#define READ_PERIOD_LENGTH 2048 * 4
#define DTS_TYPE_I     0xB
#define DTS_TYPE_II    0xC
#define DTS_TYPE_III   0xD
#define DTS_TYPE_IV    0x11
#define IEC61937_HEADER_LENGTH  8
#define IEC_DTS_HD_APPEND_LNGTH 12
#define IEC61937_PA_OFFSET  0
#define IEC61937_PA_SIZE    2
#define IEC61937_PB_OFFSET  2
#define IEC61937_PB_SIZE    2
#define IEC61937_PC_OFFSET  4
#define IEC61937_PC_SIZE    2
#define IEC61937_PD_OFFSET  6
#define IEC61937_PD_SIZE    2

//#define MAX_DDP_FRAME_LENGTH 2048

///static struct pcm_info pcm_out_info;
/*dts decoder lib function*/
int (*dts_decoder_init)(int, int);
int (*dts_decoder_cleanup)();
int (*dts_decoder_process)(char * , int , int *, char *, int *, struct pcm_info *, char *, int *);
void *gDtsDecoderLibHandler = NULL;


static  int unload_dts_decoder_lib()
{
    if (dts_decoder_cleanup != NULL) {
        (*dts_decoder_cleanup)();
    }
    dts_decoder_init = NULL;
    dts_decoder_process = NULL;
    dts_decoder_cleanup = NULL;
    if (gDtsDecoderLibHandler != NULL) {
        dlclose(gDtsDecoderLibHandler);
        gDtsDecoderLibHandler = NULL;
    }
    return 0;
}

static int dca_decoder_init(int digital_raw)
{
    //int digital_raw = 1;
    gDtsDecoderLibHandler = dlopen("/vendor/lib/libHwAudio_dtshd.so", RTLD_NOW);
    if (!gDtsDecoderLibHandler) {
        ALOGE("%s, failed to open (libstagefright_soft_dtshd.so), %s\n", __FUNCTION__, dlerror());
        goto Error;
    } else {
        ALOGV("<%s::%d>--[gDtsDecoderLibHandler]", __FUNCTION__, __LINE__);
    }

    dts_decoder_init = (int (*)(int, int)) dlsym(gDtsDecoderLibHandler, "dca_decoder_init");
    if (dts_decoder_init == NULL) {
        ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
        goto Error;
    } else {
        ALOGV("<%s::%d>--[dts_decoder_init:]", __FUNCTION__, __LINE__);
    }

    dts_decoder_process = (int (*)(char * , int , int *, char *, int *, struct pcm_info *, char *, int *))
                          dlsym(gDtsDecoderLibHandler, "dca_decoder_process");
    if (dts_decoder_process == NULL) {
        ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
        goto Error;
    } else {
        ALOGV("<%s::%d>--[dts_decoder_process:]", __FUNCTION__, __LINE__);
    }

    dts_decoder_cleanup = (int (*)()) dlsym(gDtsDecoderLibHandler, "dca_decoder_deinit");
    if (dts_decoder_cleanup == NULL) {
        ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
        goto Error;
    } else {
        ALOGV("<%s::%d>--[dts_decoder_cleanup:]", __FUNCTION__, __LINE__);
    }

    /*TODO: always decode*/
    (*dts_decoder_init)(1, digital_raw);
    return 0;
Error:
    unload_dts_decoder_lib();
    return -1;
}

static int dca_decode_process(unsigned char*input, int input_size, unsigned char *outbuf,
                              int *out_size, char *spdif_buf, int *raw_size, struct pcm_info * pcm_out_info)
{
    int outputFrameSize = 0;
    int used_size = 0;
    int decoded_pcm_size = 0;
    int ret = -1;

    if (dts_decoder_process == NULL) {
        return ret;
    }

    ret = (*dts_decoder_process)((char *) input
                                 , input_size
                                 , &used_size
                                 , (char *) outbuf
                                 , out_size
                                 , pcm_out_info
                                 , (char *) spdif_buf
                                 , (int *) raw_size);
    if (ret == 0) {
        ALOGI("decode ok");
    }
    ALOGV("used_size %d,lpcm out_size %d,raw out size %d", used_size, *out_size, *raw_size);

    return used_size;
}


int dca_decoder_init_patch(struct dca_dts_dec *dts_dec)
{
    ALOGD("%s:%d enter", __func__, __LINE__);
    dts_dec->status = dca_decoder_init(dts_dec->digital_raw);
    if (dts_dec->status < 0) {
        return -1;
    }
    dts_dec->status = 1;
    dts_dec->remain_size = 0;
    dts_dec->outlen_pcm = 0;
    dts_dec->outlen_raw = 0;
    dts_dec->is_dtscd = -1;
    dts_dec->inbuf = NULL;
    dts_dec->outbuf = NULL;
    dts_dec->outbuf_raw = NULL;
    dts_dec->inbuf = (unsigned char*) malloc(MAX_DECODER_FRAME_LENGTH * 4 * 2);
    if (!dts_dec->inbuf) {
        ALOGE("%s:%d malloc input buffer failed", __func__, __LINE__);
        return -1;
    }
    dts_dec->outbuf = (unsigned char*) malloc(MAX_DECODER_FRAME_LENGTH * 3 + MAX_DECODER_FRAME_LENGTH + 8);
    if (!dts_dec->outbuf) {
        ALOGE("%s:%d malloc output buffer failed", __func__, __LINE__);
        return -1;
    }
    dts_dec->outbuf_raw = dts_dec->outbuf + MAX_DECODER_FRAME_LENGTH * 3;
    dts_dec->decoder_process = dca_decode_process;
    ring_buffer_init(&dts_dec->output_ring_buf, 6 * MAX_DECODER_FRAME_LENGTH);
    return 1;
}

int dca_decoder_release_patch(struct dca_dts_dec *dts_dec)
{
    ALOGD("%s:%d enter", __func__, __LINE__);
    if (dts_decoder_cleanup != NULL) {
        (*dts_decoder_cleanup)();
    }
    if (dts_dec->status == 1) {
        dts_dec->status = 0;
        dts_dec->remain_size = 0;
        dts_dec->outlen_pcm = 0;
        dts_dec->outlen_raw = 0;
        free(dts_dec->inbuf);
        free(dts_dec->outbuf);
        dts_dec->inbuf = NULL;
        dts_dec->outbuf = NULL;
        dts_dec->outbuf_raw = NULL;
        dts_dec->decoder_process = NULL;
        memset(&dts_dec->pcm_out_info, 0, sizeof(struct pcm_info));
        if (dts_dec->resample_handle) {
            aml_audio_resample_close(dts_dec->resample_handle);
            dts_dec->resample_handle = NULL;
        }
        ring_buffer_release(&dts_dec->output_ring_buf);
    }
    return 1;
}

int dca_decoder_process_patch(struct dca_dts_dec *dts_dec, unsigned char*buffer, int bytes)
{
    int mFrame_size = 0;
    unsigned char *read_pointer = NULL;
    int dts_type = 0;
    //ALOGD("remain=%d bytes=%d\n",dts_dec->remain_size,bytes);
    memcpy((char *)dts_dec->inbuf + dts_dec->remain_size, (char *)buffer, bytes);
    dts_dec->remain_size += bytes;
    read_pointer = dts_dec->inbuf;
    int decoder_used_bytes = 0;
    void *main_frame_buffer = NULL;
    int main_frame_size = 0;
    bool SyncFlag = false;
    bool  little_end = false;
    int data_offset = 0;
    int ret = -1;
    if (dts_dec->is_dtscd == 1) {
        main_frame_buffer = read_pointer;
        main_frame_size = mFrame_size = dts_dec->remain_size;

    } else if (dts_dec->remain_size >= MAX_DECODER_FRAME_LENGTH) {
        while (!SyncFlag && dts_dec->remain_size > IEC61937_HEADER_LENGTH) {
            //DTS_SYNCWORD_IEC61937 : 0xF8724E1F
            if (read_pointer[0] == 0x72 && read_pointer[ 1] == 0xf8
                && read_pointer[2] == 0x1f && read_pointer[3] == 0x4e) {
                SyncFlag = true;
                dts_type = read_pointer[4] & 0x1f;
            } else if (read_pointer[0] == 0xf8 && read_pointer[1] == 0x72
                       && read_pointer[2] == 0x4e && read_pointer[3] == 0x1f) {
                SyncFlag = true;
                little_end = true;
                dts_type = read_pointer[5] & 0x1f;
            }
            if (SyncFlag == 0) {
                read_pointer++;
                dts_dec->remain_size--;
            }
        }
        //ALOGD("DTS Sync=%d little endian=%d dts type=%d\n",SyncFlag,little_end,dts_type);
        if (SyncFlag) {
            // point to pd
            read_pointer = read_pointer + IEC61937_PD_OFFSET;
            //ALOGD("read_pointer[0]:0x%x read_pointer[1]:0x%x",read_pointer[0],read_pointer[1]);
            if (!little_end) {
                mFrame_size = (read_pointer[0] | read_pointer[1] << 8);
            } else {
                mFrame_size = (read_pointer[1] | read_pointer[0] << 8);
            }

            if (dts_type == DTS_TYPE_I ||
                dts_type == DTS_TYPE_II ||
                dts_type == DTS_TYPE_III) {
                // these DTS type use bits length for PD
                mFrame_size = mFrame_size >> 3;
                // point to the address after pd
                read_pointer = read_pointer + IEC61937_PD_SIZE;
                data_offset = IEC61937_HEADER_LENGTH;
            } else if (dts_type == DTS_TYPE_IV) {
                /*refer kodi how to add 12 bytes header for DTS HD
                01 00 00 00 00 00 00 00 fe fe ** **, last 2 bytes for data size
                */
                // point to the address after pd
                read_pointer = read_pointer + IEC61937_PD_SIZE;
                if (dts_dec->remain_size < (IEC_DTS_HD_APPEND_LNGTH + IEC61937_HEADER_LENGTH)) {
                    // point to pa
                    memcpy(dts_dec->inbuf, read_pointer - IEC61937_HEADER_LENGTH, dts_dec->remain_size);
                    ALOGD("Not enough data for DTS HD header parsing\n");
                    return -1;
                }

                if (read_pointer[0] == 0x00 && read_pointer[1] == 0x01 && read_pointer[8] == 0xfe && read_pointer[9] == 0xfe) {
                    mFrame_size = (read_pointer[10] | read_pointer[11] << 8);
                } else if ((read_pointer[0] == 0x01 && read_pointer[1] == 0x00 && read_pointer[8] == 0xfe && read_pointer[9] == 0xfe)) {
                    mFrame_size = (read_pointer[11] | read_pointer[10] << 8);
                } else {
                    ALOGE("DTS HD error data\n");
                    mFrame_size = 0;
                }
                //ALOGD("size data=0x%x 0x%x\n",read_pointer[10],read_pointer[11]);
                // point to the address after 12 bytes header
                read_pointer = read_pointer + IEC_DTS_HD_APPEND_LNGTH;
                data_offset = IEC_DTS_HD_APPEND_LNGTH + IEC61937_HEADER_LENGTH;

            } else {
                ALOGE("Unknow DTS type=0x%x\n", dts_type);
                mFrame_size = 0;
                data_offset = IEC61937_PD_OFFSET;
            }

            if (mFrame_size <= 0) {
                ALOGE("wrong data for DTS,skip the header remain=%d data offset=%d\n", dts_dec->remain_size, data_offset);
                dts_dec->remain_size = dts_dec->remain_size - data_offset;

                if (dts_dec->remain_size < 0) {
                    dts_dec->remain_size = 0;
                    ALOGE("Carsh issue happens\n");
                }
                memcpy(dts_dec->inbuf, read_pointer, dts_dec->remain_size);
                return -1;
            }

            //to do know why
            if (mFrame_size == 2013) {
                mFrame_size = 2012;
            }

            main_frame_buffer = read_pointer;
            main_frame_size = mFrame_size;
            ALOGV("mFrame_size:%d dts_dec->remain_size:%d little_end:%d", mFrame_size, dts_dec->remain_size, little_end);
            // the remain size contain the header and raw data size
            if (dts_dec->remain_size < (mFrame_size + data_offset)) {
                // point to pa and copy these bytes
                memcpy(dts_dec->inbuf, read_pointer - data_offset, dts_dec->remain_size);
                mFrame_size = 0;
            } else {
                // there is enough data, header has been used, update the remain size
                dts_dec->remain_size = dts_dec->remain_size - data_offset;
            }
        } else {
            mFrame_size = 0;
        }

    }
    if (mFrame_size <= 0) {
        return -1;
    }

#if 0
    if (getprop_bool("media.audiohal.todca")) {
        FILE *dump_fp = NULL;
        dump_fp = fopen("/data/audio_hal/audio2dca.dts", "a+");
        if (dump_fp != NULL) {
            fwrite(main_frame_buffer, main_frame_size, 1, dump_fp);
            fclose(dump_fp);
        } else {
            ALOGW("[Error] Can't write to /data/audio_hal/audio2dca.raw");
        }
    }
#endif


    int used_size = dts_dec->decoder_process((unsigned char*)main_frame_buffer,
                    main_frame_size,
                    (unsigned char *)dts_dec->outbuf,
                    &dts_dec->outlen_pcm,
                    (char *)dts_dec->outbuf_raw,
                    &dts_dec->outlen_raw,
                    &dts_dec->pcm_out_info);

#if 0
    if (getprop_bool("media.audiohal.dtspcm")) {
        FILE *dump_fp = NULL;
        dump_fp = fopen("/data/audio_hal/audio2dca.pcm", "a+");
        if (dump_fp != NULL) {
            fwrite(dts_dec->outbuf, dts_dec->outlen_pcm, 1, dump_fp);
            fclose(dump_fp);
        } else {
            ALOGW("[Error] Can't write to /data/audio_hal/audio2dca.pcm");
        }
    }
#endif


    //ALOGD("mFrame_size:%d, outlen_pcm:%d, used_size = %d raw=%d\n", main_frame_size, dts_dec->outlen_pcm, used_size, dts_dec->outlen_raw);
    //ALOGD("sample rate=%d channels=%d\n",dts_dec->pcm_out_info.sample_rate,dts_dec->pcm_out_info.channel_num);
    if (used_size > 0) {
        int temp = dts_dec->remain_size;
        dts_dec->remain_size -= used_size;
        if (dts_dec->remain_size < 0) {
            ALOGE("remain ori=%d new=%d used_size=%d main=%d\n", temp, dts_dec->remain_size, used_size, main_frame_size);
            dts_dec->remain_size = 0;
        }
        memcpy(dts_dec->inbuf, read_pointer + used_size, dts_dec->remain_size);
    }

    if (dts_dec->outlen_pcm > 0 && dts_dec->pcm_out_info.sample_rate > 0 && dts_dec->pcm_out_info.sample_rate != 48000) {

        if (dts_dec->resample_handle) {
            if (dts_dec->pcm_out_info.sample_rate != (int)dts_dec->resample_handle->resample_config.input_sr) {
                audio_resample_config_t resample_config;
                ALOGD("Sample rate is changed from %d to %d, reset the resample\n",dts_dec->resample_handle->resample_config.input_sr, dts_dec->pcm_out_info.sample_rate);
                aml_audio_resample_close(dts_dec->resample_handle);
                dts_dec->resample_handle = NULL;
            }
        }

        if (dts_dec->resample_handle == NULL) {
            audio_resample_config_t resample_config;
            ALOGI("init resampler from %d to 48000!\n", dts_dec->pcm_out_info.sample_rate);
            resample_config.aformat   = AUDIO_FORMAT_PCM_16_BIT;
            resample_config.channels  = 2;
            resample_config.input_sr  = dts_dec->pcm_out_info.sample_rate;
            resample_config.output_sr = 48000;
            ret = aml_audio_resample_init((aml_audio_resample_t **)&dts_dec->resample_handle, AML_AUDIO_ANDROID_RESAMPLE, &resample_config);
            if (ret < 0) {
                ALOGE("resample init error\n");
                return -1;
            }
        }



        ret = aml_audio_resample_process(dts_dec->resample_handle, dts_dec->outbuf, dts_dec->outlen_pcm);
        if (ret < 0) {
            ALOGE("resample process error\n");
            return -1;
        }
        //ALOGD("resample in size =%d out=%d\n",dts_dec->outlen_pcm, dts_dec->resample_handle->resample_size);
        dts_dec->outlen_pcm = dts_dec->resample_handle->resample_size;

        if (get_buffer_write_space(&dts_dec->output_ring_buf) > dts_dec->outlen_pcm) {
            ring_buffer_write(&dts_dec->output_ring_buf, dts_dec->resample_handle->resample_buffer,
                    dts_dec->outlen_pcm, UNCOVER_WRITE);
            ALOGV("mFrame_size:%d, outlen_pcm:%d, used = %d\n", mFrame_size , dts_dec->outlen_pcm, used_size);
        } else {
            ALOGI("Lost data, outlen_pcm:%d\n", dts_dec->outlen_pcm);
        }

    } else if (dts_dec->outlen_pcm > 0) {
        if (get_buffer_write_space(&dts_dec->output_ring_buf) > dts_dec->outlen_pcm) {
            ring_buffer_write(&dts_dec->output_ring_buf, dts_dec->outbuf,
                    dts_dec->outlen_pcm, UNCOVER_WRITE);
            ALOGV("mFrame_size:%d, outlen_pcm:%d, used = %d\n", mFrame_size, dts_dec->outlen_pcm, used_size);
        }
    }


    return 0;
EXIT:

    return -1;
}

static int Write_buffer(struct aml_audio_parser *parser, unsigned char *buffer, int size)
{
    int writecnt = -1;
    int sleep_time = 0;

    if (parser->decode_ThreadExitFlag == 1) {
        ALOGI("decoder exiting status %s\n", __func__);
        return EXITING_STATUS;
    }
    while (parser->decode_ThreadExitFlag == 0) {
        if (get_buffer_write_space(&parser->aml_ringbuffer) < size) {
            sleep_time++;
            usleep(3000);
        } else {
            writecnt = ring_buffer_write(&parser->aml_ringbuffer,
                                         (unsigned char*) buffer, size, UNCOVER_WRITE);
            break;
        }
        if (sleep_time > 1000) { //wait for max 1s to get audio data
            ALOGW("[%s] time out to write audio buffer data! wait for 1s\n", __func__);
            return (parser->decode_ThreadExitFlag == 1) ? EXITING_STATUS : NO_ENOUGH_DATA;
        }
    }
    return writecnt;
}

#define DTS_DECODER_ENABLE
static void *decode_threadloop(void *data)
{
    struct aml_audio_parser *parser = (struct aml_audio_parser *) data;
    unsigned char *outbuf = NULL;
    unsigned char *inbuf = NULL;
    unsigned char *outbuf_raw = NULL;
    unsigned char *read_pointer = NULL;
    int valid_lib = 1;
    int outlen = 0;
    int outlen_raw = 0;
    int outlen_pcm = 0;
    int remain_size = 0;
    bool get_frame_size_ok = 0;
    bool little_end = false;
    bool SyncFlag = false;
    int used_size = 0;
    unsigned int u32AlsaFrameSize = 0;
    int s32AlsaReadFrames = 0;
    int mSample_rate = 0;
    int s32DtsFramesize = 0;
    int mChNum = 0;
    unsigned char temp;
    int i, j;
    int digital_raw = 0;
    int mute_count = 5;
    struct pcm_info  pcm_out_info;

    if (NULL == parser) {
        ALOGE("%s:%d parser == NULL", __func__, __LINE__);
        return NULL;
    }

    ALOGI("++ %s:%d in_sr = %d, out_sr = %d\n", __func__, __LINE__, parser->in_sample_rate, parser->out_sample_rate);
    outbuf = (unsigned char*) malloc(MAX_DECODER_FRAME_LENGTH * 4 + MAX_DECODER_FRAME_LENGTH + 8);
    if (!outbuf) {
        ALOGE("%s:%d malloc output buffer failed", __func__, __LINE__);
        return NULL;
    }

    outbuf_raw = outbuf + MAX_DECODER_FRAME_LENGTH;
    inbuf = (unsigned char*) malloc(MAX_DECODER_FRAME_LENGTH * 4 * 2);

    if (!inbuf) {
        ALOGE("%s:%d malloc input buffer failed", __func__, __LINE__);
        free(outbuf);
        return NULL;
    }
#ifdef DTS_DECODER_ENABLE
    if (dts_decoder_init != NULL) {
        unload_dts_decoder_lib();
    }
    //use and bt output need only pcm output,need not raw output
    if (dca_decoder_init(0)) {
        ALOGW("%s:%d dts decoder init failed, maybe no lisensed dts decoder", __func__, __LINE__);
        valid_lib = 0;
    }
    //parser->decode_enabled = 1;
#endif
    if (parser->in_sample_rate != parser->out_sample_rate) {
        parser->aml_resample.input_sr = parser->in_sample_rate;
        parser->aml_resample.output_sr = parser->out_sample_rate;
        parser->aml_resample.channels = 2;
        resampler_init(&parser->aml_resample);
    }

    struct aml_stream_in *in = (struct aml_stream_in *)parser->stream;
    u32AlsaFrameSize = in->config.channels * pcm_format_to_bits(in->config.format) / 8;
    prctl(PR_SET_NAME, (unsigned long)"audio_dca_dec");
    while (parser->decode_ThreadExitFlag == 0) {
        outlen = 0;
        outlen_raw = 0;
        outlen_pcm = 0;
        SyncFlag = 0;
        s32DtsFramesize = 0;
        if (parser->decode_ThreadExitFlag == 1) {
            ALOGI("%s:%d exit threadloop!", __func__, __LINE__);
            break;
        }

        //here we call decode api to decode audio frame here
        if (remain_size + READ_PERIOD_LENGTH <= (MAX_DECODER_FRAME_LENGTH * 4 * 2)) { //input buffer size
            // read raw DTS data for alsa
            s32AlsaReadFrames = pcm_read(parser->aml_pcm, inbuf + remain_size, READ_PERIOD_LENGTH);
            if (s32AlsaReadFrames < 0) {
                usleep(1000);
                continue;
            } else {
#if 0
                FILE *dump_origin = NULL;
                dump_origin = fopen("/data/tmp/pcm_read.raw", "a+");
                if (dump_origin != NULL) {
                    fwrite(inbuf + remain_size, READ_PERIOD_LENGTH, 1, dump_origin);
                    fclose(dump_origin);
                } else {
                    ALOGW("[Error] Can't write to /data/tmp/pcm_read.raw");
                }
#endif
            }
            remain_size += s32AlsaReadFrames * u32AlsaFrameSize;
        }

#ifdef DTS_DECODER_ENABLE
        if (valid_lib == 0) {
            continue;
        }
#endif
        //find header and get paramters
        read_pointer = inbuf;
        while (parser->decode_ThreadExitFlag == 0 && remain_size > MAX_DECODER_FRAME_LENGTH) {
            //DTS_SYNCWORD_IEC61937 : 0xF8724E1F
            if (read_pointer[0] == 0x72 && read_pointer[ 1] == 0xf8
                && read_pointer[2] == 0x1f && read_pointer[3] == 0x4e) {
                SyncFlag = true;
                little_end = false;
                s32DtsFramesize = (read_pointer[6] | read_pointer[7] << 8) / 8;
                if (s32DtsFramesize == 2013) {
                    s32DtsFramesize = 2012;
                }
                //ALOGI("mFrame_size:%d dts_dec->remain_size:%d little_end:%d", mFrame_size, remain_size, little_end);
                break;
            }
            read_pointer++;
            remain_size--;
        }

        if (remain_size < (s32DtsFramesize + 8) || SyncFlag == 0) {
            ALOGV("%s:%d remain:%d, DtsFramesize:%d, SyncFlag:%d, read more...", __func__, __LINE__,
                remain_size, s32DtsFramesize, SyncFlag);
            memcpy(inbuf, read_pointer, remain_size);
            continue;
        }
        read_pointer += 8;   //pa pb pc pd
#ifdef DTS_DECODER_ENABLE
        used_size = dca_decode_process(read_pointer, s32DtsFramesize, outbuf,
                                       &outlen_pcm, (char *) outbuf_raw, &outlen_raw,&pcm_out_info);
#if 1
    if (getprop_bool("media.audiohal.dtsdump")) {
        FILE *dump_fp = NULL;
        dump_fp = fopen("/data/audio_hal/audio2dca.dts", "a+");
        if (dump_fp != NULL) {
            fwrite(read_pointer, s32DtsFramesize, 1, dump_fp);
            fclose(dump_fp);
        } else {
            ALOGW("[Error] Can't write to /data/audio_hal/audio2dca.raw");
        }
    }
#endif

#else
        used_size = s32DtsFramesize;
#endif
        if (used_size > 0) {
            remain_size -= 8;    //pa pb pc pd
            remain_size -= used_size;
            ALOGV("%s:%d decode success used_size:%d, outlen_pcm:%d", __func__, __LINE__, used_size, outlen_pcm);
            memcpy(inbuf, read_pointer + used_size, remain_size);
        } else {
            ALOGW("%s:%d decode failed, used_size:%d", __func__, __LINE__, used_size);
        }

#ifdef DTS_DECODER_ENABLE
        //only need pcm data
        if (outlen_pcm > 0) {

            // here only downresample, so no need to malloc more buffer
            if (parser->in_sample_rate != parser->out_sample_rate) {
                int out_frame = outlen_pcm >> 2;
                out_frame = resample_process(&parser->aml_resample, out_frame, (short*) outbuf, (short*) outbuf);
                outlen_pcm = out_frame << 2;
            }
            parser->data_ready = 1;
            //here when pcm -> dts,some error frames maybe received by deocder
            //and I tried to use referrence tools to decoding the frames and the output also has noise frames.
            //so here we try to mute count outbuf frame to wrok arount the issuse
            if (mute_count > 0) {
                memset(outbuf, 0, outlen_pcm);
                mute_count--;
            }
#if 1
            if (getprop_bool("media.audiohal.dtsdump")) {
                FILE *dump_fp = NULL;
                dump_fp = fopen("/data/audio_hal/dtsout.pcm", "a+");
                if (dump_fp != NULL) {
                    fwrite(outbuf, outlen_pcm, 1, dump_fp);
                    fclose(dump_fp);
                } else {
                    ALOGW("[Error] Can't write to /data/audio_hal/dtsout.pcm");
                }
            }
#endif
            Write_buffer(parser, outbuf, outlen_pcm);
        }
#endif
    }
    parser->decode_enabled = 0;
    if (inbuf) {
        free(inbuf);
    }
    if (outbuf) {
        free(outbuf);
    }
#ifdef DTS_DECODER_ENABLE
    unload_dts_decoder_lib();
#endif
    ALOGI("-- %s:%d", __func__, __LINE__);
    return NULL;
}


static int start_decode_thread(struct aml_audio_parser *parser)
{
    int ret = 0;

    ALOGI("%s:%d enter", __func__, __LINE__);
    parser->decode_enabled = 1;
    parser->decode_ThreadExitFlag = 0;
    ret = pthread_create(&parser->decode_ThreadID, NULL, &decode_threadloop, parser);
    if (ret != 0) {
        ALOGE("%s:%d, Create thread fail!", __FUNCTION__, __LINE__);
        return -1;
    }
    return 0;
}

static int stop_decode_thread(struct aml_audio_parser *parser)
{
    ALOGI("++%s:%d enter", __func__, __LINE__);
    parser->decode_ThreadExitFlag = 1;
    pthread_join(parser->decode_ThreadID, NULL);
    parser->decode_ThreadID = 0;
    ALOGI("--%s:%d exit", __func__, __LINE__);
    return 0;
}

int dca_decode_init(struct aml_audio_parser *parser)
{
    return start_decode_thread(parser);
}

int dca_decode_release(struct aml_audio_parser *parser)
{
    return stop_decode_thread(parser);
}


