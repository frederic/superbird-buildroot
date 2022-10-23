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

#ifndef _DOLBY_MS12_H_
#define _DOLBY_MS12_H_

#include <utils/Mutex.h>

#include "DolbyMS12ConfigParams.h"

#ifdef __cplusplus

namespace android
{
typedef int (*output_callback)(void *buffer, void *priv, size_t size);

class DolbyMS12
{

public:
    // static DolbyMS12* getInstance();

    DolbyMS12();
    virtual ~DolbyMS12();
    virtual int     GetLibHandle(void);
    virtual void    ReleaseLibHandle(void);
    virtual int     GetMS12OutputMaxSize(void);
    virtual void *  DolbyMS12Init(int configNum, char **configParams);
    virtual void    DolbyMS12Release(void *dolbyMS12_pointer);
    virtual int     DolbyMS12InputMain(
        void *dolbyMS12_pointer
        , const void *audio_stream_out_buffer //ms12 input buffer
        , size_t audio_stream_out_buffer_size //ms12 input buffer size
        , int audio_stream_out_format
        , int audio_stream_out_channel_num
        , int audio_stream_out_sample_rate
    );
    virtual int     DolbyMS12InputAssociate(
        void *dolbyMS12_pointer
        , const void *audio_stream_out_buffer //ms12 input buffer
        , size_t audio_stream_out_buffer_size //ms12 input buffer size
        , int audio_stream_out_format
        , int audio_stream_out_channel_num
        , int audio_stream_out_sample_rate
    );
    virtual int     DolbyMS12InputSystem(
        void *dolbyMS12_pointer
        , const void *audio_stream_out_buffer //ms12 input buffer
        , size_t audio_stream_out_buffer_size //ms12 input buffer size
        , int audio_stream_out_format
        , int audio_stream_out_channel_num
        , int audio_stream_out_sample_rate
    );

#ifdef REPLACE_OUTPUT_BUFFER_WITH_CALLBACK

    virtual int     DolbyMS12RegisterPCMCallback(output_callback callback, void *priv_data);

    virtual int     DolbyMS12RegisterBitstreamCallback(output_callback callback, void *priv_data);

#else

    virtual int     DolbyMS12Output(
        void *dolbyMS12_pointer
        , const void *ms12_out_buffer //ms12 output buffer
        , size_t request_out_buffer_size //ms12 output buffer size
    );

#endif

    virtual int     DolbyMS12UpdateRuntimeParams(
        void *DolbyMS12Pointer
        , int configNum
        , char **configParams);

    virtual int     DolbyMS12SchedulerRun(void *DolbyMS12Pointer);

    virtual void    DolbyMS12SetQuitFlag(int is_quit);

    virtual void    DolbyMS12FlushInputBuffer(void);

    virtual void    DolbyMS12FlushMainInputBuffer(void);

    virtual void    DolbyMS12SetMainDummy(int type, int dummy);

    virtual unsigned long long DolbyMS12GetNBytesConsumedOfUDC(void);

    virtual void    DolbyMS12GetPCMOutputSize(unsigned long long *all_output_size, unsigned long long *ms12_generate_zero_size);

    virtual void    DolbyMS12GetBitstreamOutputSize(unsigned long long *all_output_size, unsigned long long *ms12_generate_zero_size);

    virtual int     DolbyMS12GetMainBufferAvail(void);

    virtual int     DolbyMS12GetAssociateBufferAvail(void);

    virtual int     DolbyMS12GetSystemBufferAvail(void);


    // protected:


private:
    // DolbyMS12(const DolbyMS12&);
    // DolbyMS12& operator = (const DolbyMS12&);
    // static android::Mutex mLock;
    // static DolbyMS12 *gInstance;
    void *mDolbyMS12LibHanle;
};  // class DolbyMS12

}   // android
#endif


#endif  // _DOLBY_MS12_H_
