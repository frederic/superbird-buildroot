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

#define LOG_TAG "libms12"
//#define LOG_NDEBUG 0
//#define LOG_NALOGV 0

#include "log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dlfcn.h>

#include "DolbyMS12.h"
#include "DolbyMS12ConfigParams.h"

#define DOLBY_MS12_LIB_PATH_A "/vendor/lib/libdolbyms12.so"
#define DOLBY_MS12_LIB_PATH_B "/system/vendor/lib/libdolbyms12.so"

namespace android
{

//static function pointer
int (*FuncGetMS12OutputMaxSize)(void);
void * (*FuncDolbyMS12Init)(int argc, char **pp_argv);
void (*FuncDolbyMS12Release)(void *);
int (*FuncDolbyMS12InputMain)(void *, const void *, size_t, int, int, int);
int (*FuncDolbyMS12InputAssociate)(void *, const void *, size_t, int, int, int);
int (*FuncDolbyMS12InputSystem)(void *, const void *, size_t, int, int, int);

#ifdef REPLACE_OUTPUT_BUFFER_WITH_CALLBACK
int (*FuncDolbyMS12RegisterPCMCallback)(output_callback , void *);
int (*FuncDolbyMS12RegisterBitstreamCallback)(output_callback , void *);
#else
int (*FuncDolbyMS12Output)(void *, const void *, size_t);
#endif

int (*FuncDolbyMS12UpdateRuntimeParams)(void *, int , char **);
int (*FuncDolbyMS12SchedulerRun)(void *);
void (*FuncDolbyMS12SetQuitFlag)(int);
void (*FuncDolbyMS12FlushInputBuffer)(void);
void (*FuncDolbyMS12FlushMainInputBuffer)(void);
void (*FuncDolbyMS12SetMainDummy)(int, int);
unsigned long long (*FuncDolbyMS12GetNBytesConsumedOfUDC)(void);
void (*FuncDolbyMS12GetPCMOutputSize)(unsigned long long *, unsigned long long *);
void (*FuncDolbyMS12GetBitstreamOutputSize)(unsigned long long *, unsigned long long *);

int (*FuncDolbyMS12GetMainBufferAvail)(void);
int (*FuncDolbyMS12GetAssociateBufferAvail)(void);
int (*FuncDolbyMS12GetSystemBufferAvail)(void);


DolbyMS12::DolbyMS12() :
    mDolbyMS12LibHanle(NULL)
{
    ALOGD("%s()", __FUNCTION__);
}


DolbyMS12::~DolbyMS12()
{
    ALOGD("%s()", __FUNCTION__);
}


int DolbyMS12::GetLibHandle(void)
{
    ALOGD("+%s()", __FUNCTION__);
    ReleaseLibHandle();

    //here there are two paths, "the DOLBY_MS12_LIB_PATH_A/B", where could exit that dolby ms12 libary.
    mDolbyMS12LibHanle = dlopen(DOLBY_MS12_LIB_PATH_A, RTLD_NOW);
    if (!mDolbyMS12LibHanle) {
        mDolbyMS12LibHanle = dlopen(DOLBY_MS12_LIB_PATH_B, RTLD_NOW);
        if (!mDolbyMS12LibHanle) {
            ALOGE("%s, failed to load libdolbyms12 lib %s\n", __FUNCTION__, dlerror());
            goto ERROR;
        }
    }

    FuncGetMS12OutputMaxSize = (int (*)(void)) dlsym(mDolbyMS12LibHanle, "get_ms12_output_max_size");
    if (!FuncGetMS12OutputMaxSize) {
        ALOGE("%s, dlsym get_ms12_output_max_size fail\n", __FUNCTION__);
        goto ERROR;
    }

    FuncDolbyMS12Init = (void* (*)(int, char **)) dlsym(mDolbyMS12LibHanle, "ms12_init");
    if (!FuncDolbyMS12Init) {
        ALOGE("%s, dlsym ms12_init fail\n", __FUNCTION__);
        goto ERROR;
    }

    FuncDolbyMS12Release = (void (*)(void *)) dlsym(mDolbyMS12LibHanle, "ms12_release");
    if (!FuncDolbyMS12Release) {
        ALOGE("%s, dlsym ms12_release fail\n", __FUNCTION__);
        goto ERROR;
    }

    FuncDolbyMS12InputMain = (int (*)(void *, const void *, size_t, int, int, int)) dlsym(mDolbyMS12LibHanle, "ms12_input_main");
    if (!FuncDolbyMS12InputMain) {
        ALOGE("%s, dlsym ms12_input_main fail\n", __FUNCTION__);
        goto ERROR;
    }

    FuncDolbyMS12InputAssociate = (int (*)(void *, const void *, size_t, int, int, int)) dlsym(mDolbyMS12LibHanle, "ms12_input_associate");
    if (!FuncDolbyMS12InputAssociate) {
        ALOGE("%s, dlsym ms12_input_associate fail\n", __FUNCTION__);
        goto ERROR;
    }

    FuncDolbyMS12InputSystem = (int (*)(void *, const void *, size_t, int, int, int)) dlsym(mDolbyMS12LibHanle, "ms12_input_system");
    if (!FuncDolbyMS12InputSystem) {
        ALOGE("%s, dlsym ms12_input_system fail\n", __FUNCTION__);
        goto ERROR;
    }

#ifdef REPLACE_OUTPUT_BUFFER_WITH_CALLBACK
    FuncDolbyMS12RegisterPCMCallback = (int (*)(output_callback , void *)) dlsym(mDolbyMS12LibHanle, "ms12_output_register_pcm_callback");
    if (!FuncDolbyMS12RegisterPCMCallback) {
        ALOGE("%s, dlsym ms12_output_register_pcm_callback fail\n", __FUNCTION__);
        goto ERROR;
    }
    FuncDolbyMS12RegisterBitstreamCallback = (int (*)(output_callback , void *)) dlsym(mDolbyMS12LibHanle, "ms12_output_register_bitstream_callback");
    if (!FuncDolbyMS12RegisterBitstreamCallback) {
        ALOGE("%s, dlsym ms12_output_register_bitstream_callback fail\n", __FUNCTION__);
        goto ERROR;
    }
#else
    FuncDolbyMS12Output = (int (*)(void *, const void *, size_t))  dlsym(mDolbyMS12LibHanle, "ms12_output");
    if (!FuncDolbyMS12Output) {
        ALOGE("%s, dlsym ms12_output fail\n", __FUNCTION__);
        goto ERROR;
    }
#endif

    FuncDolbyMS12UpdateRuntimeParams = (int (*)(void *, int , char **))  dlsym(mDolbyMS12LibHanle, "ms12_update_runtime_params");
    if (!FuncDolbyMS12UpdateRuntimeParams) {
        ALOGE("%s, dlsym ms12_update_runtime_params fail\n", __FUNCTION__);
        goto ERROR;
    }

    FuncDolbyMS12SchedulerRun = (int (*)(void *))  dlsym(mDolbyMS12LibHanle, "ms12_scheduler_run");
    if (!FuncDolbyMS12SchedulerRun) {
        ALOGE("%s, dlsym ms12_scheduler_run fail\n", __FUNCTION__);
        goto ERROR;
    }

    FuncDolbyMS12SetQuitFlag = (void (*)(int))  dlsym(mDolbyMS12LibHanle, "ms12_set_quit_flag");
    if (!FuncDolbyMS12SetQuitFlag) {
        ALOGE("%s, dlsym ms12_set_quit_flag fail\n", __FUNCTION__);
        goto ERROR;
    }

    FuncDolbyMS12FlushInputBuffer = (void (*)(void))  dlsym(mDolbyMS12LibHanle, "ms12_flush_input_buffer");
    if (!FuncDolbyMS12FlushInputBuffer) {
        ALOGE("%s, dlsym ms12_flush_input_buffer fail\n", __FUNCTION__);
        goto ERROR;
    }
    FuncDolbyMS12FlushMainInputBuffer = (void (*)(void))  dlsym(mDolbyMS12LibHanle, "ms12_flush_main_input_buffer");
    if (!FuncDolbyMS12FlushInputBuffer) {
        ALOGE("%s, dlsym ms12_flush_main_input_buffer fail\n", __FUNCTION__);
        goto ERROR;
    }
    FuncDolbyMS12SetMainDummy = (void (*)(int, int))  dlsym(mDolbyMS12LibHanle, "ms12_set_main_dummy");
    if (!FuncDolbyMS12SetMainDummy) {
        ALOGE("%s, dlsym ms12_set_main_dummy fail\n", __FUNCTION__);
        goto ERROR;
    }
    FuncDolbyMS12GetNBytesConsumedOfUDC = (unsigned long long (*)(void))  dlsym(mDolbyMS12LibHanle, "get_n_bytes_consumed_of_udc");
    if (!FuncDolbyMS12GetNBytesConsumedOfUDC) {
        ALOGE("%s, dlsym get_n_bytes_consumed_of_udc fail\n", __FUNCTION__);
        goto ERROR;
    }

    FuncDolbyMS12GetPCMOutputSize = (void (*)(unsigned long long *, unsigned long long *))  dlsym(mDolbyMS12LibHanle, "get_pcm_output_size");
    if (!FuncDolbyMS12GetPCMOutputSize) {
        ALOGE("%s, dlsym get_pcm_output_size fail\n", __FUNCTION__);
        goto ERROR;
    }

    FuncDolbyMS12GetBitstreamOutputSize = (void (*)(unsigned long long *, unsigned long long *))  dlsym(mDolbyMS12LibHanle, "get_bitstream_output_size");
    if (!FuncDolbyMS12GetBitstreamOutputSize) {
        ALOGE("%s, dlsym get_bitstream_output_size fail\n", __FUNCTION__);
        goto ERROR;
    }

    FuncDolbyMS12GetMainBufferAvail = (int (*)(void))  dlsym(mDolbyMS12LibHanle, "get_main_buffer_avail");
    if (!FuncDolbyMS12GetMainBufferAvail) {
        ALOGE("%s, dlsym get_main_buffer_avail fail\n", __FUNCTION__);
        goto ERROR;
    }

    FuncDolbyMS12GetAssociateBufferAvail = (int (*)(void))  dlsym(mDolbyMS12LibHanle, "get_associate_buffer_avail");
    if (!FuncDolbyMS12GetAssociateBufferAvail) {
        ALOGE("%s, dlsym get_associate_buffer_avail fail\n", __FUNCTION__);
        goto ERROR;
    }

    FuncDolbyMS12GetSystemBufferAvail = (int (*)(void))  dlsym(mDolbyMS12LibHanle, "get_system_buffer_avail");
    if (!FuncDolbyMS12GetSystemBufferAvail) {
        ALOGE("%s, dlsym get_system_buffer_avail fail\n", __FUNCTION__);
        goto ERROR;
    }

    ALOGD("-%s() line %d get libdolbyms12 success!", __FUNCTION__, __LINE__);
    return 0;

ERROR:
    ALOGD("-%s() line %d", __FUNCTION__, __LINE__);
    return -1;
}

void DolbyMS12::ReleaseLibHandle(void)
{
    ALOGD("+%s()", __FUNCTION__);

    //re-value the api as NULL
    FuncGetMS12OutputMaxSize = NULL;
    FuncDolbyMS12Init = NULL;
    FuncDolbyMS12Release = NULL;
    FuncDolbyMS12InputMain = NULL;
    FuncDolbyMS12InputAssociate = NULL;
    FuncDolbyMS12InputSystem = NULL;
#ifdef REPLACE_OUTPUT_BUFFER_WITH_CALLBACK
    FuncDolbyMS12RegisterPCMCallback = NULL;
    FuncDolbyMS12RegisterBitstreamCallback = NULL;
#else
    FuncDolbyMS12Output = NULL;
#endif
    FuncDolbyMS12UpdateRuntimeParams = NULL;
    FuncDolbyMS12SchedulerRun = NULL;
    FuncDolbyMS12SetQuitFlag = NULL;
    FuncDolbyMS12FlushInputBuffer = NULL;
    FuncDolbyMS12GetNBytesConsumedOfUDC = NULL;
    FuncDolbyMS12GetPCMOutputSize = NULL;
    FuncDolbyMS12GetBitstreamOutputSize = NULL;
    FuncDolbyMS12GetMainBufferAvail = NULL;
    FuncDolbyMS12GetAssociateBufferAvail = NULL;
    FuncDolbyMS12GetSystemBufferAvail = NULL;
    FuncDolbyMS12SetMainDummy = NULL;

    if (mDolbyMS12LibHanle != NULL) {
        dlclose(mDolbyMS12LibHanle);
        mDolbyMS12LibHanle = NULL;
    }

    ALOGD("-%s()", __FUNCTION__);
    return ;
}

int DolbyMS12::GetMS12OutputMaxSize(void)
{
    ALOGD("+%s()", __FUNCTION__);
    int ret = 0;
    if (!FuncGetMS12OutputMaxSize) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return -1;
    }

    ret = (*FuncGetMS12OutputMaxSize)();
    ALOGD("-%s() ret %d", __FUNCTION__, ret);
    return ret;
}


void * DolbyMS12::DolbyMS12Init(int configNum, char **configParams)
{
    void * dolby_ms12_init_ret = NULL;
    ALOGD("+%s()", __FUNCTION__);
    if (!FuncDolbyMS12Init) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return NULL;
    }

    dolby_ms12_init_ret = (*FuncDolbyMS12Init)(configNum, configParams);
    ALOGD("-%s() dolby_ms12_init_ret %p", __FUNCTION__, dolby_ms12_init_ret);
    return dolby_ms12_init_ret;
}

void DolbyMS12::DolbyMS12Release(void *DolbyMS12Pointer)
{
    ALOGD("+%s()", __FUNCTION__);
    if (!FuncDolbyMS12Release) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return ;
    }

    (*FuncDolbyMS12Release)(DolbyMS12Pointer);
    ALOGD("-%s()", __FUNCTION__);
    return ;
}

int DolbyMS12::DolbyMS12InputMain(
    void *DolbyMS12Pointer
    , const void *audio_stream_out_buffer //ms12 input buffer
    , size_t audio_stream_out_buffer_size //ms12 input buffer size
    , int audio_stream_out_format
    , int audio_stream_out_channel_num
    , int audio_stream_out_sample_rate
)
{
    ALOGV("+%s()", __FUNCTION__);
    int ret = 0;

    if (!FuncDolbyMS12InputMain) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return -1;
    }

    ret = (*FuncDolbyMS12InputMain)(DolbyMS12Pointer
                                    , audio_stream_out_buffer //ms12 input buffer
                                    , audio_stream_out_buffer_size //ms12 input buffer size
                                    , audio_stream_out_format
                                    , audio_stream_out_channel_num
                                    , audio_stream_out_sample_rate);
    ALOGV("-%s() ret %d", __FUNCTION__, ret);
    return ret;
}

int DolbyMS12::DolbyMS12InputAssociate(
    void *DolbyMS12Pointer
    , const void *audio_stream_out_buffer //ms12 input buffer
    , size_t audio_stream_out_buffer_size //ms12 input buffer size
    , int audio_stream_out_format
    , int audio_stream_out_channel_num
    , int audio_stream_out_sample_rate
)
{
    ALOGV("+%s()", __FUNCTION__);
    int ret = 0;

    if (!FuncDolbyMS12InputAssociate) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return -1;
    }

    ret = (*FuncDolbyMS12InputAssociate)(DolbyMS12Pointer
                                         , audio_stream_out_buffer //ms12 input buffer
                                         , audio_stream_out_buffer_size //ms12 input buffer size
                                         , audio_stream_out_format
                                         , audio_stream_out_channel_num
                                         , audio_stream_out_sample_rate);
    ALOGV("-%s() ret %d", __FUNCTION__, ret);
    return ret;
}

int DolbyMS12::DolbyMS12InputSystem(
    void *DolbyMS12Pointer
    , const void *audio_stream_out_buffer //ms12 input buffer
    , size_t audio_stream_out_buffer_size //ms12 input buffer size
    , int audio_stream_out_format
    , int audio_stream_out_channel_num
    , int audio_stream_out_sample_rate
)
{
    ALOGV("+%s()", __FUNCTION__);
    int ret = 0;

    if (!FuncDolbyMS12InputSystem) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return -1;
    }

    ret = (*FuncDolbyMS12InputSystem)(DolbyMS12Pointer
                                      , audio_stream_out_buffer //ms12 input buffer
                                      , audio_stream_out_buffer_size //ms12 input buffer size
                                      , audio_stream_out_format
                                      , audio_stream_out_channel_num
                                      , audio_stream_out_sample_rate);
    ALOGV("-%s() ret %d", __FUNCTION__, ret);
    return ret;
}

#ifdef REPLACE_OUTPUT_BUFFER_WITH_CALLBACK
int DolbyMS12::DolbyMS12RegisterPCMCallback(output_callback callback, void *priv_data)
{
    int ret = 0;
    ALOGV("+%s()", __FUNCTION__);
    if (!FuncDolbyMS12RegisterPCMCallback) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return -1;
    }

    ret = (*FuncDolbyMS12RegisterPCMCallback)(callback, priv_data);
    ALOGV("-%s() ret %d", __FUNCTION__, ret);
    return ret;
}

int DolbyMS12::DolbyMS12RegisterBitstreamCallback(output_callback callback, void *priv_data)
{
    int ret = 0;
    ALOGV("+%s()", __FUNCTION__);
    if (!FuncDolbyMS12RegisterBitstreamCallback) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return -1;
    }

    ret = (*FuncDolbyMS12RegisterBitstreamCallback)(callback , priv_data);
    ALOGV("-%s() ret %d", __FUNCTION__, ret);
    return ret;
}
#else
int DolbyMS12::DolbyMS12Output(
    void *DolbyMS12Pointer
    , const void *ms12_out_buffer //ms12 output buffer
    , size_t request_out_buffer_size //ms12 output buffer size
)
{
    int ret = 0;
    ALOGV("+%s()", __FUNCTION__);
    if (!FuncDolbyMS12Output) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return -1;
    }

    ret = (*FuncDolbyMS12Output)(DolbyMS12Pointer
                                 , ms12_out_buffer //ms12 output buffer
                                 , request_out_buffer_size //ms12 output buffer size
                                );
    ALOGV("-%s() ret %d", __FUNCTION__, ret);
    return ret;
}
#endif

int DolbyMS12::DolbyMS12UpdateRuntimeParams(void *DolbyMS12Pointer, int configNum, char **configParams)
{
    int ret = 0;
    ALOGV("+%s()", __FUNCTION__);
    if (!FuncDolbyMS12UpdateRuntimeParams) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return -1;
    }

    ret = (*FuncDolbyMS12UpdateRuntimeParams)(DolbyMS12Pointer, configNum, configParams);
    ALOGV("-%s() ret %d", __FUNCTION__, ret);
    return ret;
}

int DolbyMS12::DolbyMS12SchedulerRun(void *DolbyMS12Pointer)
{
    int ret = 0;
    ALOGV("+%s()", __FUNCTION__);
    if (!FuncDolbyMS12SchedulerRun) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return -1;
    }

    ret = (*FuncDolbyMS12SchedulerRun)(DolbyMS12Pointer);
    ALOGV("-%s() ret %d", __FUNCTION__, ret);
    return ret;
}

void DolbyMS12::DolbyMS12SetQuitFlag(int is_quit)
{
    int ret = 0;
    ALOGV("+%s() is_quit %d", __FUNCTION__, is_quit);
    if (!FuncDolbyMS12SetQuitFlag) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return ;
    }

    (*FuncDolbyMS12SetQuitFlag)(is_quit);
    ALOGV("-%s() ret %d", __FUNCTION__, ret);
    return ;
}

void DolbyMS12::DolbyMS12FlushInputBuffer(void)
{
    int ret = 0;
    ALOGV("+%s()", __FUNCTION__);
    if (!FuncDolbyMS12FlushInputBuffer) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return ;
    }

    (*FuncDolbyMS12FlushInputBuffer)();
    ALOGV("-%s() ret %d", __FUNCTION__, ret);
    return ;
}
void DolbyMS12::DolbyMS12FlushMainInputBuffer(void)
{
    int ret = 0;
    ALOGV("+%s()", __FUNCTION__);
    if (!FuncDolbyMS12FlushMainInputBuffer) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return ;
    }

    (*FuncDolbyMS12FlushMainInputBuffer)();
    ALOGV("-%s() ret %d", __FUNCTION__, ret);
    return ;
}

void DolbyMS12::DolbyMS12SetMainDummy(int type, int dummy)
{
    int ret = 0;
    ALOGV("+%s()", __FUNCTION__);
    if (!FuncDolbyMS12SetMainDummy) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return ;
    }

    (*FuncDolbyMS12SetMainDummy)(type, dummy);
    ALOGV("-%s() ret %d", __FUNCTION__, ret);
    return ;
}

unsigned long long DolbyMS12::DolbyMS12GetNBytesConsumedOfUDC(void)
{
    unsigned long long ret = 0;
    ALOGV("+%s()", __FUNCTION__);
    if (!FuncDolbyMS12GetNBytesConsumedOfUDC) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return ret;
    }

    ret = (*FuncDolbyMS12GetNBytesConsumedOfUDC)();
    ALOGV("-%s() ret %llu", __FUNCTION__, ret);
    return ret;
}

void DolbyMS12::DolbyMS12GetPCMOutputSize(unsigned long long *all_output_size, unsigned long long *ms12_generate_zero_size)
{
    ALOGV("+%s()", __FUNCTION__);
    if (!FuncDolbyMS12GetPCMOutputSize) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return ;
    }

    (*FuncDolbyMS12GetPCMOutputSize)(all_output_size, ms12_generate_zero_size);
    ALOGV("-%s() *all_output_size %llu *ms12_generate_zero_size %llu", __FUNCTION__, *all_output_size,  *ms12_generate_zero_size);
    return ;
}


void DolbyMS12::DolbyMS12GetBitstreamOutputSize(unsigned long long *all_output_size, unsigned long long *ms12_generate_zero_size)
{
    ALOGV("+%s()", __FUNCTION__);
    if (!FuncDolbyMS12GetBitstreamOutputSize) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return ;
    }

    (*FuncDolbyMS12GetBitstreamOutputSize)(all_output_size, ms12_generate_zero_size);
    ALOGV("-%s() *all_output_size %llu *ms12_generate_zero_size %llu", __FUNCTION__, *all_output_size,  *ms12_generate_zero_size);
    return ;
}

int DolbyMS12::DolbyMS12GetMainBufferAvail(void)
{
    int ret = 0;
    ALOGV("+%s()", __FUNCTION__);
    if (!FuncDolbyMS12GetMainBufferAvail) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return ret;
    }

    ret = (*FuncDolbyMS12GetMainBufferAvail)();
    ALOGV("-%s() ret %d", __FUNCTION__, ret);
    return ret;
}

int DolbyMS12::DolbyMS12GetAssociateBufferAvail(void)
{
    int ret = 0;
    ALOGV("+%s()", __FUNCTION__);
    if (!FuncDolbyMS12GetAssociateBufferAvail) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return ret;
    }

    ret = (*FuncDolbyMS12GetAssociateBufferAvail)();
    ALOGV("-%s() ret %d", __FUNCTION__, ret);
    return ret;
}

int DolbyMS12::DolbyMS12GetSystemBufferAvail(void)
{
    int ret = 0;
    ALOGV("+%s()", __FUNCTION__);
    if (!FuncDolbyMS12GetSystemBufferAvail) {
        ALOGE("%s(), pls load lib first.\n", __FUNCTION__);
        return ret;
    }

    ret = (*FuncDolbyMS12GetSystemBufferAvail)();
    ALOGV("-%s() ret %d", __FUNCTION__, ret);
    return ret;
}

/*--------------------------------------------------------------------------*/
}   // namespace android
