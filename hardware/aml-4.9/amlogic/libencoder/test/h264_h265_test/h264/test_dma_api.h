/*
 * Copyright (C) 2018 The Android Open Source Project
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
#ifndef __GDC_API_H__
#define __GDC_API_H__

#define WORD_SIZE 16
#define WORD_MASK (~(WORD_SIZE - 1))
#define AXI_WORD_ALIGN(size) ((size + WORD_SIZE - 1) & WORD_MASK)

enum dma_data_direction {
    DMA_BIDIRECTIONAL = 0,
    DMA_TO_DEVICE = 1,
    DMA_FROM_DEVICE = 2,
    DMA_NONE = 3,
};

typedef unsigned int uint32_t;

/* for gdc dma buf define */
struct dmabuf_req_s {
    int index;
    unsigned int len;
    unsigned int dma_dir;
};

struct dmabuf_exp_s {
    int index;
    unsigned int flags;
    int fd;
};
/* end of gdc dma buffer define */

#define GDC_IOC_MAGIC  'G'

#define GDC_REQUEST_DMA_BUFF _IOW(GDC_IOC_MAGIC, 0x06, struct dmabuf_req_s)
#define GDC_EXP_DMA_BUFF _IOW(GDC_IOC_MAGIC, 0x07, struct dmabuf_exp_s)
#define GDC_FREE_DMA_BUFF _IOW(GDC_IOC_MAGIC, 0x08, int)
#define GDC_SYNC_DEVICE _IOW(GDC_IOC_MAGIC, 0x09, int)
#define GDC_SYNC_CPU _IOW(GDC_IOC_MAGIC, 0x0a, int)

enum {
    INPUT_BUFF_TYPE = 0x1000,
    OUTPUT_BUFF_TYPE,
    CONFIG_BUFF_TYPE,
    GDC_BUFF_TYPE_MAX
};

struct usr_ctx_s {
    //TODO  currently donot support multi-context
    int gdc_client;
    char *i_buff[3];
    char *o_buff;
    char *c_buff;
    unsigned long i_len[3];
    unsigned long o_len;
    unsigned long c_len;
    int fd[3];
    int inbuf_num;
};

/**
 *   This function create a contex for gdc work.
 *
 *   @param  ctx - contains client of gdc and ion.
 *
 *   @return 0 - success
 *           -1 - fail
 */
int create_ctx(struct usr_ctx_s *ctx);

/**
 *   This function points gdc to its input resolution and yuv address and offsets
 *
 *   @param  gdc_settings - overall gdc settings
 *   @param  in_buf_fd    - input buffer's fd
 *   @param  out_buf_fd   - output buffer's fd
 *
 *   @return 0 - success
 *           -1 - no interrupt from GDC.
 */
int destroy_ctx(struct usr_ctx_s *ctx);

/**
 *   This function points gdc to its input resolution and yuv address and offsets
 *
 *   @param  ctx - contains client of gdc and ion.
 *   @param  share_fd - buffer's fd
 *   @param  len - buffer's size
 *
 *   @return 0 - success
 *           -1 - no interrupt from GDC.
 */
int alloc_dma_buffer (struct usr_ctx_s *ctx,
    uint32_t type, size_t len);

int sync_cpu(struct usr_ctx_s *ctx);
#endif
