/*
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License
 * Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
 * used only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission
 * from Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UTEE_TYPE_EXTENSIONS_H
#define UTEE_TYPE_EXTENSIONS_H

#define TEE_EXTEND_VDEC_GET_INFO                        0
#define TEE_EXTEND_VDEC_MMAP                            2
#define TEE_EXTEND_VDEC_MUNMAP                          3
#define TEE_EXTEND_PROTECT_MEM                          4
#define TEE_EXTEND_UNIFY_READ                           5
#define TEE_EXTEND_EFUSE_READ_TEE                       7
#define TEE_EXTEND_EFUSE_READ_REE                       8
#define TEE_EXTEND_HDCP_GET_STATE                       9
#define TEE_EXTEND_ASYMM_SIGN_PADDING                   10
#define TEE_EXTEND_ASYMM_VERIFY_PADDING                 11
#define TEE_EXTEND_CRYP_EXPORT_KEY                      12
#define TEE_EXTEND_CRYP_IMPORT_KEY                      13
#define TEE_EXTEND_MEMSET                               14
#define TEE_EXTEND_VXWM_SET_PARA_REND                   15
#define TEE_EXTEND_EFUSE_READ                           16
#define TEE_EXTEND_EFUSE_WRITE_BLOCK                    17
#define TEE_EXTEND_VIDEO_LOAD_FW                        18
#define TEE_EXTEND_KL_RUN                               19
#define TEE_EXTEND_KL_CR                                20
#define TEE_EXTEND_DESC_ALLOC_CHANNEL                   21
#define TEE_EXTEND_DESC_FREE_CHANNEL                    22
#define TEE_EXTEND_DESC_RESET                           23
#define TEE_EXTEND_DESC_SET_PID                         24
#define TEE_EXTEND_DESC_SET_KEY                         25
#define TEE_EXTEND_DESC_EXIT                            26
#define TEE_EXTEND_DESC_INIT                            27
#define TEE_EXTEND_DESC_SET_OUTPUT                      28
#define TEE_EXTEND_PROTECT_MEM2                         29
#define TEE_EXTEND_HDCP_LOAD_KEY                        30
#define TEE_EXTEND_STORAGE_OBJ_ACCESS_COMP              31
#define TEE_EXTEND_STORAGE_OBJ_OPEN                     32
#define TEE_EXTEND_STORAGE_OBJ_CREATE                   33
#define TEE_EXTEND_STORAGE_OBJ_READ                     34
#define TEE_EXTEND_STORAGE_OBJ_WRITE                    35
#define TEE_EXTEND_STORAGE_OBJ_TRUNC                    36
#define TEE_EXTEND_STORAGE_OBJ_SEEK                     37
#define TEE_EXTEND_STORAGE_OBJ_RENAME                   38
#define TEE_EXTEND_STORAGE_OBJ_DEL                      39
#define TEE_EXTEND_STORAGE_OBJ_GET_INFO                 40
#define TEE_EXTEND_STORAGE_OBJ_CLOSE                    41
#define TEE_EXTEND_NGWM_SET_SEED                        42
#define TEE_EXTEND_NGWM_SET_OPERATOR_ID                 43
#define TEE_EXTEND_NGWM_SET_SETTINGS                    44
#define TEE_EXTEND_NGWM_SET_DEVICE_ID                   45
#define TEE_EXTEND_NGWM_SET_TIME_CODE                   46
#define TEE_EXTEND_NGWM_ENABLE_SERVICE                  47
#define TEE_EXTEND_NGWM_SET_STUB_EMBEDDING              48
#define TEE_EXTEND_NGWM_SET_24BIT_MODE                  49
#define TEE_EXTEND_HDCP_SET_STREAMID                    50
#define TEE_EXTEND_HDCP_GET_STREAMID                    51
#define TEE_EXTEND_TVP_ENTER                            52
#define TEE_EXTEND_TVP_EXIT                             53
#define TEE_EXTEND_TVP_GET_VIDEO_SIZE                   54
#define TEE_EXTEND_TVP_GET_DISPLAY_SIZE                 55
#define TEE_EXTEND_TVP_SET_VIDEO_LAYER                  56
#define TEE_EXTEND_TVP_GET_VIDEO_LAYER                  57
#define TEE_EXTEND_TVP_INIT                             58

struct tee_vdec_info_param {
	paddr_t pa;
	size_t size;
};

#define TEE_TVP_POOL_MAX_COUNT	4
struct tee_tvp_init_param {
	size_t count;
	struct tee_vdec_info_param p[TEE_TVP_POOL_MAX_COUNT];
};

struct tee_vdec_mmap_param {
	paddr_t pa;
	size_t size;
	vaddr_t va;
};

struct tee_vdec_munmap_param {
	paddr_t pa;
	size_t size;
};

struct tee_protect_mem_param {
	unsigned int start;
	unsigned int size;
	int enable;
};

struct tee_unify_read_param {
	uint8_t *name;
	uint32_t namelen;
	uint8_t *buf;
	uint32_t buflen;
	uint32_t readlen;
};

struct tee_efuse_read_tee_param {
	uint8_t *buf;
	uint32_t offset;
	size_t size;
};

struct tee_efuse_read_ree_param {
	uint8_t *buf;
	uint32_t offset;
	size_t size;
};

struct tee_efuse_read_param {
	uint8_t *buf;
	uint32_t offset;
	size_t size;
};

struct tee_efuse_write_block_param {
	uint8_t *buf;
	uint32_t block;
};

struct tee_hdcp_get_state_param {
	uint32_t mode;
	uint32_t auth;
};

struct tee_hdcp_load_key_param {
	uint32_t mode;
	uint8_t *keybuf;
	uint32_t keylen;
};

struct tee_hdcp_streamid_param {
	uint32_t type;
};

struct tee_tvp_resolution_param {
	uint32_t width;
	uint32_t height;
};

struct tee_tvp_video_layer_param {
	uint32_t video_layer;
	uint32_t enable;
	uint32_t flags;
};

struct tee_asymm_sign_padding_param {
	unsigned long state;
	struct utee_attribute *params;
	size_t num_params;
	void *src_data;
	size_t src_len;
	void *dest_data;
	uint32_t dest_len;
};

struct tee_asymm_verify_padding_param {
	unsigned long state;
	struct utee_attribute *params;
	size_t num_params;
	void *data;
	size_t data_len;
	void *sig;
	size_t sig_len;
};

struct tee_cryp_export_key {
	unsigned long obj;
	void *buf;
	size_t *blen;
	bool pub;
};

struct tee_cryp_import_key {
	unsigned long obj;
	void *buf;
	size_t blen;
};

struct tee_memset_param {
	void *buf;
	uint32_t x;
	uint32_t size;
};

#if defined(CFG_WATERMARK_VERIMATRIX)
typedef struct {
	void *para;
	uint32_t para_len;
} tee_vxwm_param;
#endif

struct tee_video_fw_param {
	void *firmware;
	uint32_t fw_size;
	void *info;
	uint32_t info_size;
};

struct tee_kl_cr_param {
	unsigned char  kl_num;
	unsigned char  __padding[7];
	unsigned char  cr[16];/* in: challenge-nonce, out:response-dnonce */
	unsigned char  ekn1[16];/* ekn-1 (e.g. ek2 for 3-key ladder) */
};

struct tee_kl_run_param {
	unsigned int   dest;
	unsigned char  kl_num;
	unsigned char  kl_levels;
	unsigned char  __padding[6];
	unsigned char  keys[7][16];
};

struct tee_desc_alloc_channel_param {
	int dsc_no;
	int fd;
};

struct tee_desc_free_channel_param {
	int dsc_no;
	int fd;
};

struct tee_desc_reset_param {
	int dsc_no;
	int all;
};

struct tee_desc_set_pid_param {
	int dsc_no;
	int fd;
	int pid;
};

struct tee_desc_set_key_param {
	int dsc_no;
	int fd;
	int parity;
	unsigned char *key;
	uint32_t key_type;
};

struct tee_desc_set_output_param {
	int module;
	int output;
};

struct tee_storage_obj_access_param {
	void *object_id;
	size_t object_id_len;
	uint32_t storage_id;
};

struct tee_storage_obj_open_param {
	unsigned long storage_id;
	void *object_id;
	size_t object_id_len;
	unsigned long flags;
	uint32_t obj;
};

struct tee_storage_obj_create_param {
	unsigned long storage_id;
	void *object_id;
	size_t object_id_len;
	unsigned long flags;
	unsigned long attributes;
	const void *initial_data;
	uint32_t initial_data_len;
	uint32_t obj;
};

struct tee_storage_obj_read_param {
	unsigned long obj;
	void *data;
	size_t len;
	uint64_t count;
};

struct tee_storage_obj_write_param {
	unsigned long obj;
	const void *data;
	size_t len;
};

struct tee_storage_obj_trunc_param {
	unsigned long obj;
	size_t len;
};

struct tee_storage_obj_seek_param {
	unsigned long obj;
	int32_t offset;
	unsigned long whence;
};

struct tee_storage_obj_rename_param {
	unsigned long obj;
	void *object_id;
	size_t object_id_len;
};

struct tee_storage_obj_del_param {
	unsigned long obj;
};

struct tee_storage_obj_get_info_param {
	unsigned long obj;
	unsigned long info;
};

struct tee_storage_obj_close_param {
	unsigned long obj;
};

#if defined(CFG_WATERMARK_NEXGUARD)
typedef struct {
	void *pxEmbedder;
	uint32_t xSeed;
} ngwm_set_seed_param;

typedef struct {
	void *pxEmbedder;
	uint8_t xOperatorId;
} ngwm_set_operatorid_param;

typedef struct {
	void *pxEmbedder;
	const uint8_t *pxSettings;
	uint32_t xSize;
} ngwm_set_settings_param;

typedef struct {
	void *pxEmbedder;
	const uint8_t *pxDeviceId;
	uint8_t xSizeInBits;
} ngwm_set_deviceid_param;

typedef struct {
	void *pxEmbedder;
	uint16_t xTimeCode;
} ngwm_set_time_code_param;

typedef struct {
	void *pxEmbedder;
	bool xIsEnabled;
} ngwm_enable_service_param;

typedef struct {
	void *pxEmbedder;
	bool xIsEnabled;
} ngwm_set_stub_embedding_param;

typedef struct {
	void *pxEmbedder;
	bool xIsEnabled;
} ngwm_set_24bit_mode_param;
#endif

#endif /* UTEE_TYPE_EXTENSIONS_H */
