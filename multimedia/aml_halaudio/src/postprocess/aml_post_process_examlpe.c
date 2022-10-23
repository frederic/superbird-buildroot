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

#define LOG_TAG "aml_post_process"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "aml_audio_effect.h"

enum sample_bitwidth {
    SAMPLE_8BITS =  8,
    SAMPLE_16BITS = 16,
    SAMPLE_24BITS = 24,
    SAMPLE_32BITS = 32,
};

enum sample_endian {
    SAMPLE_LITTLE_ENDIAN,
    SAMPLE_BIG_ENDIAN,
};

#define   Clip(acc,min,max) ((acc) > max ? max : ((acc) < min ? min : (acc)))

typedef struct postprocess_example {
    const struct aml_effect_interface_s *itfe;
} postprocess_example_t;


/*used for the start*/
int aml_post_example_start(aml_effect_handle_t self)
{

    postprocess_example_t *postprocess_handle = (postprocess_example_t *)self;
    if (postprocess_handle == NULL) {
        return -1;
    }
    /*do some start preparation*/
    printf("%s \n", __FUNCTION__);
    return 0;
}

/*used for the start*/
int aml_post_example_stop(aml_effect_handle_t self)
{

    postprocess_example_t *postprocess_handle = (postprocess_example_t *)self;
    if (postprocess_handle == NULL) {
        return -1;
    }
    /*do some stop preparation*/
    printf("%s \n", __FUNCTION__);
    return 0;
}


/*used for the post process*/
int aml_post_example_process(aml_effect_handle_t self,
                             void *inBuffer,
                             int input_size,
                             int *used_size,
                             void *outBuffer,
                             int *output_size,
                             data_format_t * info)
{
    if (info == NULL || inBuffer == NULL || outBuffer == NULL) {
        return -1;
    }
    postprocess_example_t *postprocess_handle = (postprocess_example_t *)self;
    if (postprocess_handle == NULL) {
        return -1;
    }
    *used_size   = input_size;
    *output_size = input_size;

    return 0;
}

/*used for config some info for post process, the config string will be "XXX=XXX"*/
int aml_post_example_config(aml_effect_handle_t self, const char *kvpairs)
{
    int ret = 0;
    printf("handle =%p post process config: %s \n", self, kvpairs);
    return ret;
}

/*used for get some info for post process, the return string will be "XXX=XXX"*/
char * aml_post_example_getinfo(aml_effect_handle_t self, const char *keys)
{
    printf("%s %s\n", __FUNCTION__, keys);
    return strdup("info=example");
}

/*used for other post process*/
int aml_post_example_process_assist(aml_effect_handle_t self,
                                    void *inBuffer,
                                    int input_size,
                                    int *used_size,
                                    void *outBuffer,
                                    int *output_size,
                                    data_format_t * info)
{

    int ret = 0;

    return ret;
}


const struct aml_effect_interface_s gPostExampleInterface = {
    aml_post_example_start,
    aml_post_example_stop,
    aml_post_example_process,
    aml_post_example_config,
    aml_post_example_getinfo,
    aml_post_example_process_assist,
};



/*create a effect handle*/
int32_t aml_postprocess_create(aml_effect_handle_t *pHandle)
{
    int32_t ret = 0;
    postprocess_example_t *postprocess_handle = NULL;
    if (pHandle == NULL) {
        return -1;
    }
    postprocess_handle = calloc(1, sizeof(postprocess_example_t));
    if (postprocess_handle == NULL) {
        return -1;
    }
    postprocess_handle->itfe   = &gPostExampleInterface;
    *pHandle = (aml_effect_handle_t)postprocess_handle;
    printf("%s handle=%p success\n", __FUNCTION__, postprocess_handle);
    return ret;
}

/*release the effect handle*/
int32_t aml_postprocess_release(aml_effect_handle_t handle)
{
    int32_t ret = 0;
    postprocess_example_t *postprocess_handle = (postprocess_example_t *)handle;
    if (postprocess_handle == NULL) {
        return -1;
    }
    free(postprocess_handle);
    printf("%s success\n", __FUNCTION__);
    return ret;
}


// This is the only symbol that needs to be exported
__attribute__((visibility("default")))
aml_audio_effect_library_t AML_AUDIO_EFFECT_LIBRARY_INFO_SYM = {
    tag : AML_AUDIO_EFFECT_LIBRARY_TAG,
    name : "PostProcess Example",
    version : "v1.0",
    implementor : "Amlogic",
    create_effect : aml_postprocess_create,
    release_effect : aml_postprocess_release,
};


