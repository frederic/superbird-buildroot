/*
 * Copyright (C) 2020 Amlogic Corporation.
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


#ifndef AML_AUDIO_EFFECT_H
#define AML_AUDIO_EFFECT_H

#include "stdint.h"

typedef struct data_format {
    int sr ;   /**  samplerate*/
    int ch ;   /**  channels*/
    int bitwidth; /**the sample bit width, refer to sample_bitwidth*/
    int endian;   /*refer to sample_endian*/

} data_format_t;



typedef struct aml_effect_interface_s **aml_effect_handle_t;

// Effect control interface definition
struct aml_effect_interface_s {
    ////////////////////////////////////////////////////////////////////////////////
    //
    //    Function:       start
    //
    //    Description:    the post process will be started
    //
    //    Input:
    //          self:     handle to the effect interface this function is called on.
    //    Output:
    //          returned value:    0 successful operation
    //                          other value: wrong operation
    ////////////////////////////////////////////////////////////////////////////////
    int (*start)(aml_effect_handle_t self);

    ////////////////////////////////////////////////////////////////////////////////
    //
    //    Function:       stop
    //
    //    Description:    the post process will be stoped
    //
    //    Input:
    //          self:     handle to the effect interface this function is called on.
    //    Output:
    //          returned value:    0 successful operation
    //                          other value: wrong operation
    ////////////////////////////////////////////////////////////////////////////////
    int (*stop)(aml_effect_handle_t self);

    ////////////////////////////////////////////////////////////////////////////////
    //
    //    Function:       process
    //
    //    Description:    Effect process function.
    //
    //    Input:
    //          self:         handle to the effect interface this function is called on.
    //          inBuffer:     buffer indicating where to read samples to process.
    //          input_size:   the input data size
    //          outBuffer:    buffer indicating where to write processed samples.
    //          output_size:  the buffer size of output buf
    //          info:         indicate the dat info, include samplerate ch etc.
    //
    //    Output:
    //          used_size:    how many data is used in the process
    //          output_size:  how many data is processed in the output buf
    //          returned value:    0 successful operation
    //                          other value: wrong operation
    ////////////////////////////////////////////////////////////////////////////////
    int (*process)(aml_effect_handle_t self,
                   void *inBuffer,
                   int input_size,
                   int *used_size,
                   void *outBuffer,
                   int *output_size,
                   data_format_t * info);

    ////////////////////////////////////////////////////////////////////////////////
    //
    //    Function:       config
    //
    //    Description:    Effect config function.
    //
    //    Input:
    //          self:         handle to the effect interface this function is called on.
    //          keys:         used for config some info for post process, the
    //                        config string will be "XXX=XXX"
    //    Output:
    //          returned value:    0 successful operation
    //                          other value: wrong operation
    ////////////////////////////////////////////////////////////////////////////////
    int (*config)(aml_effect_handle_t self,
                  const char *kvpairs);

    /*used for get some info for post process, the return string will be "XXX=XXX"*/
    ////////////////////////////////////////////////////////////////////////////////
    //
    //    Function:       getinfo
    //
    //    Description:    Effect getinfo function.
    //
    //    Input:
    //          self:         handle to the effect interface this function is called on.
    //          keys:         the info key to query
    //    Output:
    //          returned value:    the query info will be "XXX=XXX", caller should free
    //                             the string buffer
    /////////////////////////////////////////////////////////////////////////////////
    char * (*getinfo)(aml_effect_handle_t self,
                      const char *keys);

    ////////////////////////////////////////////////////////////////////////////////
    //
    //    Function:       assist process
    //
    //    Description:    Effect process assist function.
    //
    //    Input:
    //          self:         handle to the effect interface this function is called on.
    //          inBuffer:     buffer indicating where to read samples to process.
    //          input_size:   the input data size
    //          outBuffer:    buffer indicating where to write processed samples.
    //          output_size:  the buffer size of output buf
    //          info:         indicate the dat info, include samplerate ch etc.
    //
    //    Output:
    //          used_size:    how many data is used in the process
    //          output_size:  how many data is processed in the output buf
    //          returned value:    0 successful operation
    //                          other value: wrong operation
    ////////////////////////////////////////////////////////////////////////////////
    int (*process_assist)(aml_effect_handle_t self,
                          void *inBuffer,
                          int input_size,
                          int *used_size,
                          void *outBuffer,
                          int *output_size,
                          data_format_t * info);

};



#define AML_AUDIO_EFFECT_LIBRARY_TAG ((('A') << 24) | (('E') << 16) | (('L') << 8) | ('T'))

typedef struct aml_audio_effect_library_s {
    // tag must be initialized to AUDIO_EFFECT_LIBRARY_TAG
    uint32_t tag;
    /*library name*/
    const char *name;
    /*library version*/
    const char * version;
    /*the effect name*/
    const char *implementor;
    /*create a effect handle*/
    int32_t (*create_effect)(aml_effect_handle_t *pHandle);
    /*release the effect handle*/
    int32_t (*release_effect)(aml_effect_handle_t handle);
} aml_audio_effect_library_t;


// Name of the hal_module_info
#define AML_AUDIO_EFFECT_LIBRARY_INFO_SYM         AML_AELI

// Name of the hal_module_info as a string
#define AML_AUDIO_EFFECT_LIBRARY_INFO_SYM_AS_STR  "AML_AELI"

#endif

