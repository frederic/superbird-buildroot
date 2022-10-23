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

#include <stdbool.h>

#define WORD_SIZE 16
#define WORD_MASK (~(WORD_SIZE - 1))
#define AXI_WORD_ALIGN(size) ((size + WORD_SIZE - 1) & WORD_MASK)

typedef unsigned long phys_addr_t;

#define __iomem

// #define __DEBUG

#ifdef __DEBUG
#define D_GDC(fmt, args...) printf(fmt, ## args)
#else
#define D_GDC(fmt, args...)
#endif
#define E_GDC(fmt, args...) printf(fmt, ## args)


enum gdc_memtype_s {
	AML_GDC_MEM_ION,
	AML_GDC_MEM_DMABUF,
	AML_GDC_MEM_INVALID,
};

enum dma_data_direction {
	DMA_BIDIRECTIONAL = 0,
	DMA_TO_DEVICE = 1,
	DMA_FROM_DEVICE = 2,
	DMA_NONE = 3,
};

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
	uint32_t config_addr;     //gdc config address
	uint32_t config_size;     //gdc config size in 32bit
	uint32_t input_width;     //gdc input width resolution
	uint32_t input_height;    //gdc input height resolution
	uint32_t input_y_stride;  //gdc input y stride resolution
	uint32_t input_c_stride;  //gdc input uv stride
	uint32_t output_width;    //gdc output width resolution
	uint32_t output_height;   //gdc output height resolution
	uint32_t output_y_stride; //gdc output y stride
	uint32_t output_c_stride; //gdc output uv stride
} gdc_config_t;

typedef struct gdc_buffer_info {
	unsigned int mem_alloc_type;
	unsigned int plane_number;
	union {
		unsigned int y_base_fd;
		unsigned int shared_fd;
	};
	union {
		unsigned int uv_base_fd;
		unsigned int u_base_fd;
	};
	unsigned int v_base_fd;
} gdc_buffer_info_t;

// overall gdc settings and state
typedef struct gdc_settings {
	uint32_t magic;
	uint32_t base_gdc;       //writing/reading to gdc base address, currently not read by api
	gdc_config_t gdc_config; //array of gdc configuration and sizes
	//int gdc_config_total;  //update this index for new config
	uint32_t buffer_addr;    //start memory to write gdc output framse
	uint32_t buffer_size;    //size of memory output frames to determine if it is enough and can do multiple write points
	uint32_t current_addr;   //current output address of gdc
	int32_t is_waiting_gdc;  //set when expecting an interrupt from gdc

	int32_t in_fd;  //input buffer's share fd
	int32_t out_fd; //output buffer's share fd

	//input address for y and u, v planes
	uint32_t y_base_addr;
	union {
		uint32_t uv_base_addr;
		uint32_t u_base_addr;
	};
	uint32_t v_base_addr;
	void *ddr_mem; //opaque address in ddr added with offset to write the gdc config sequence
	void (*get_frame_buffer)(uint32_t y_base_addr,
		uint32_t uv_base_addr,
		uint32_t y_line_offset,
		uint32_t uv_line_offset);
	void *fh;
	int32_t y_base_fd;
	union {
		int32_t uv_base_fd;
		int32_t u_base_fd;
	};
	int32_t v_base_fd;
}gdc_settings_t;

typedef struct gdc_settings_ex {
	uint32_t magic;
	gdc_config_t gdc_config; //array of gdc configuration and sizes
	gdc_buffer_info_t input_buffer;
	gdc_buffer_info_t config_buffer;
	gdc_buffer_info_t output_buffer;
} gdc_settings_ex_t;

/* for gdc dma buf define */
struct gdc_dmabuf_req_s {
	int index;
	unsigned int len;
	unsigned int dma_dir;
};

struct gdc_dmabuf_exp_s {
	int index;
	unsigned int flags;
	int fd;
};
/* end of gdc dma buffer define */

#define GDC_IOC_MAGIC 'G'
#define GDC_PROCESS _IOW(GDC_IOC_MAGIC, 0x00, struct gdc_settings)
#define GDC_PROCESS_NO_BLOCK _IOW(GDC_IOC_MAGIC, 0x01, struct gdc_settings)
#define GDC_RUN	_IOW(GDC_IOC_MAGIC, 0x02, struct gdc_settings)
#define GDC_REQUEST_BUFF _IOW(GDC_IOC_MAGIC, 0x03, struct gdc_settings)

#define GDC_PROCESS_EX _IOW(GDC_IOC_MAGIC, 0x05, struct gdc_settings_ex)
#define GDC_REQUEST_DMA_BUFF _IOW(GDC_IOC_MAGIC, 0x06, struct gdc_dmabuf_req_s)
#define GDC_EXP_DMA_BUFF _IOW(GDC_IOC_MAGIC, 0x07, struct gdc_dmabuf_exp_s)
#define GDC_FREE_DMA_BUFF _IOW(GDC_IOC_MAGIC, 0x08, int)
#define GDC_SYNC_DEVICE _IOW(GDC_IOC_MAGIC, 0x09, int)
#define GDC_SYNC_CPU _IOW(GDC_IOC_MAGIC, 0x0a, int)
#define GDC_PROCESS_WITH_FW _IOW(GDC_IOC_MAGIC, 0x0b, \
					struct gdc_settings_with_fw)

#define MAX_PLANE 3

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

enum {
	EQUISOLID = 1,
	CYLINDER,
	EQUIDISTANT,
	CUSTOM,
	AFFINE,
	FW_TYPE_MAX
};

struct fw_equisolid_s {
	/* float */
	char strengthX[8];
	/* float */
	char strengthY[8];
	int rotation;
};

struct fw_cylinder_s {
	/* float */
	char strength[8];
	int rotation;
};

struct fw_equidistant_s {
	/* float */
	char azimuth[8];
	int elevation;
	int rotation;
	int fov_width;
	int fov_height;
	bool keep_ratio;
	int cylindricityX;
	int cylindricityY;
};

struct fw_custom_s {
	char *fw_name;
};

struct fw_affine_s {
	int rotation;
};

struct fw_input_info_s {
	int with;
	int height;
	int fov;
	int diameter;
	int offsetX;
	int offsetY;
};

union transform_u {
	struct fw_equisolid_s fw_equisolid;
	struct fw_cylinder_s fw_cylinder;
	struct fw_equidistant_s fw_equidistant;
	struct fw_custom_s fw_custom;
	struct fw_affine_s fw_affine;
};

struct fw_output_info_s {
	int offsetX;
	int offsetY;
	int width;
	int height;
	union transform_u trans;
	int pan;
	int tilt;
	/* float*/
	char zoom[8];
};

struct firmware_info {
	unsigned int format;
	unsigned int trans_size_type;
	char *file_name;
	phys_addr_t phys_addr;
	void __iomem *virt_addr;
	unsigned int size;
	struct page *cma_pages;
	unsigned int loaded;
};

struct fw_info_s {
	char *fw_name;
	int fw_type;
	struct page *cma_pages;
	phys_addr_t phys_addr;
	void __iomem *virt_addr;
	struct fw_input_info_s fw_input_info;
	struct fw_output_info_s fw_output_info;
};

struct gdc_settings_with_fw {
	uint32_t magic;
	gdc_config_t gdc_config;
	struct gdc_buffer_info input_buffer;
	struct gdc_buffer_info reserved;
	struct gdc_buffer_info output_buffer;
	struct fw_info_s fw_info;
};

struct gdc_usr_ctx_s {
	int gdc_client;
	int ion_fd;
	int custom_fw;
	struct gdc_settings gs;
	struct gdc_settings_ex gs_ex;
	struct gdc_settings_with_fw gs_with_fw;
	int mem_type;
	int plane_number;
	char *i_buff[MAX_PLANE];
	char *o_buff[MAX_PLANE];
	char *c_buff;
	unsigned long i_len[MAX_PLANE];
	unsigned long o_len[MAX_PLANE];
	unsigned long c_len;
};

typedef struct gdc_alloc_buffer_s {
	unsigned int format;
	unsigned int plane_number;
	unsigned int len[MAX_PLANE];
} gdc_alloc_buffer_t;

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
 *
 *   @return 0 - success
 *           -1 - no interrupt from GDC.
 */
int gdc_destroy_ctx(struct gdc_usr_ctx_s *ctx);

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

int gdc_process_with_builtin_fw(struct gdc_usr_ctx_s *ctx);

int gdc_alloc_buffer (struct gdc_usr_ctx_s *ctx, uint32_t type,
			struct gdc_alloc_buffer_s *buf, bool cache_flag);

int gdc_sync_for_device(struct gdc_usr_ctx_s *ctx);
int gdc_sync_for_cpu(struct gdc_usr_ctx_s *ctx);
#endif
