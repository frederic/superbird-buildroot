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

enum {
    NV12 = 1,
    YV12,
    Y_GREY,
    YUV444_P,
    RGB444_P,
    FMT_MAX
};


typedef unsigned int uint32_t;

// each configuration addresses and size
typedef struct gdc_config {
    uint32_t format;
    uint32_t config_addr;   //gdc config address
    uint32_t config_size;   //gdc config size in 32bit
    uint32_t input_width;  //gdc input width resolution
    uint32_t input_height; //gdc input height resolution
    uint32_t input_y_stride; //gdc input y stride resolution
    uint32_t input_c_stride; //gdc input uv stride
    uint32_t output_width;  //gdc output width resolution
    uint32_t output_height; //gdc output height resolution
    uint32_t output_y_stride; //gdc output y stride
    uint32_t output_c_stride; //gdc output uv stride
} gdc_config_t;

// overall gdc settings and state
typedef struct gdc_settings {
    uint32_t magic;
    uint32_t base_gdc;        //writing/reading to gdc base address, currently not read by api
    gdc_config_t gdc_config; //array of gdc configuration and sizes
    //int gdc_config_total;     //update this index for new config
    uint32_t buffer_addr;     //start memory to write gdc output framse
    uint32_t buffer_size;     //size of memory output frames to determine if it is enough and can do multiple write points
    uint32_t current_addr;    //current output address of gdc
    int32_t is_waiting_gdc;       //set when expecting an interrupt from gdc

    int32_t in_fd;  //input buffer's share fd
    int32_t out_fd; //output buffer's share fd

    //input address for y and u, v planes
    uint32_t y_base_addr;
    union {
        uint32_t uv_base_addr;
        uint32_t u_base_addr;
    };
    uint32_t v_base_addr;
    void *ddr_mem;			//opaque address in ddr added with offset to write the gdc config sequence
    //when inititialised this callback will be called to update frame buffer addresses and offsets
    void (*get_frame_buffer)(uint32_t y_base_addr, uint32_t uv_base_addr, uint32_t y_line_offset, uint32_t uv_line_offset);
    void *fh;
    int32_t y_base_fd;
    union {
        int32_t uv_base_fd;
        int32_t u_base_fd;
    };
    int32_t v_base_fd;
} gdc_settings_t;

#define GDC_IOC_MAGIC  'G'
#define GDC_PROCESS		            _IOW(GDC_IOC_MAGIC, 0x00, struct gdc_settings)
#define GDC_PROCESS_NO_BLOCK		_IOW(GDC_IOC_MAGIC, 0x01, struct gdc_settings)
#define GDC_RUN	_IOW(GDC_IOC_MAGIC, 0x02, struct gdc_settings)
#define GDC_REQUEST_BUFF _IOW(GDC_IOC_MAGIC, 0x03, struct gdc_settings)
#define GDC_HANDLE	_IOW(GDC_IOC_MAGIC, 0x04, struct gdc_settings)

enum {
	INPUT_BUFF_TYPE = 0x1000,
	OUTPUT_BUFF_TYPE,
	CONFIG_BUFF_TYPE,
	GDC_BUFF_TYPE_MAX
};

struct gdc_buf_cfg {
	uint32_t type;
	unsigned long len;
};

struct gdc_usr_ctx_s {
    //TODO  currently donot support multi-context
    int gdc_client;
    int ion_client;
    struct gdc_settings gs;
	char *i_buff;
	char *o_buff;
	char *c_buff;
	unsigned long i_len;
	unsigned long o_len;
	unsigned long c_len;
};

/**
 *   This function create a contex for gdc work.
 *
 *   @param  ctx - contains client of gdc and ion.
 *
 *   @return 0 - success
 *           -1 - fail
 */
int gdc_create_ctx(struct gdc_usr_ctx_s *ctx);

/**
 *   This function points gdc to its input resolution and yuv address and offsets
 *
 *   @param  ctx - contains client of gdc and ion.
 *   @param  gs  - overall gdc settings init except input and output buf
 *
 *   @return 0 - success
 *           -1 - no interrupt from GDC.
 */
int gdc_init(struct gdc_usr_ctx_s *ctx, struct gdc_settings *gs);

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
int gdc_process(struct gdc_usr_ctx_s *ctx);

/**
 *   This function points gdc to its input resolution and yuv address and offsets
 *
 *   @param  ctx - contains client of gdc and ion.
 *
 *   @return 0 - success
 *           -1 - no interrupt from GDC.
 */
int gdc_destroy_ctx(struct gdc_usr_ctx_s *ctx);

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
int gdc_alloc_dma_buffer (struct gdc_usr_ctx_s *ctx,
			uint32_t type, size_t len);

#endif
