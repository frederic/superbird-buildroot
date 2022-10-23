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

#define LOG_TAG "aml_sample_conv"


#include <unistd.h>
#include "aml_sample_conv.h"
#include "log.h"



typedef void (*aml_convert_func_t)(unsigned n, const void *a, void *b);
typedef struct aml_conv_handle {
    int src_bitwidth;
    int dst_bitwidth;
    aml_convert_func_t func;
} aml_conv_handle_t;


static inline void s16_to_s32(unsigned n, const void *a, void *b)
{
    int i = 0;
    int16_t * src = (int16_t *)a;
    int32_t * dst = (int32_t *)b;
    for (i = 0; i < n; i ++) {
        dst[i] = ((int32_t) src[i]) << 16;
    }
}

static aml_conv_handle_t aml_conv_table [] = {
    {SAMPLE_16BITS, SAMPLE_32BITS, s16_to_s32},
};
int aml_sampleconv_init(aml_sample_conv_t ** handle)
{
    int ret = -1;
    int size = 1024 * 4;
    void *tmp_convert_buffer;
    aml_sample_conv_t * sample_conv = NULL;

    sample_conv = malloc(sizeof(aml_sample_conv_t));

    if (sample_conv == NULL) {
        ALOGD("sample_conv malloc failed\n");
        goto exit;
    }

    tmp_convert_buffer = malloc(size);
    if (tmp_convert_buffer == NULL) {
        ALOGD("tmp_convert_buffer malloc failed\n");
        goto exit;

    }

    sample_conv->convert_buffer = tmp_convert_buffer;
    sample_conv->convert_size   = size;
    sample_conv->buf_size       = size;


    * handle = sample_conv;

    return 0;
exit:
    return -1;

}

int aml_sampleconv_close(aml_sample_conv_t * handle)
{
    if (handle == NULL) {
        return 0;
    }
    if (handle->convert_buffer) {
        free(handle->convert_buffer);
        handle->convert_buffer = NULL;
    }

    free(handle);

    return 0;
}


int aml_sampleconv_process(aml_sample_conv_t * handle, aml_data_format_t *src, void * in_data, aml_data_format_t *dst, size_t nsamples)
{
    int i = 0;
    aml_conv_handle_t *conv_item;
    aml_convert_func_t conv_func = NULL;
    size_t need_bytes = 0;

    need_bytes = nsamples * (dst->bitwidth >> 3);

    if (handle->buf_size < need_bytes) {
        ALOGI("realloc tmp_convert_buffer size from %zu to %zu\n", handle->convert_size, need_bytes);
        handle->convert_buffer = realloc(handle->convert_buffer, need_bytes);
        if (handle->convert_buffer == NULL) {
            ALOGE("realloc convert_buffer buf failed size %zu\n", need_bytes);
            return -1;
        }
         handle->buf_size = need_bytes;
    }

    handle->convert_size = need_bytes;

    for (i = 0; i < sizeof(aml_conv_table) / sizeof(aml_conv_handle_t); i ++) {
        conv_item = &aml_conv_table[i];
        if (conv_item->src_bitwidth == src->bitwidth && conv_item->dst_bitwidth == dst->bitwidth) {
            conv_func = conv_item->func;
            conv_func(nsamples, in_data, handle->convert_buffer);
            return 0;
        }

    }

    return -1 ;
}



