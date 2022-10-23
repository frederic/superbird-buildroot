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

#include <tee_internal_api.h>

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
#define TEE_EXTEND_MUTEX                                59
#define TEE_EXTEND_CRYPTO_RUN                           60
#define TEE_EXTEND_CRYPTO_SET_KEY_IV                    61
#define TEE_EXTEND_SHM_MMAP                             62
#define TEE_EXTEND_SHM_MUNMAP                           63
#define TEE_EXTEND_READ_REG                             64
#define TEE_EXTEND_WRITE_REG                            65
#define TEE_EXTEND_UPDATE_MVN                           66
#define TEE_EXTEND_READ_RNG_POOL                        67
#define TEE_EXTEND_TVP_SET_AUDIO_MUTE                   68
#define TEE_EXTEND_DESC_SET_ALGO                        69
#define TEE_EXTEND_DESC_SET_MODE                        70
#define TEE_EXTEND_WM_SET_PARA_LAST                     71
#define TEE_EXTEND_HDMI_GET_STATE                       72
#define TEE_EXTEND_CALLBACK                             73
#define TEE_EXTEND_CRYPTO_AE_DECRYPT_WITH_DERIVED_KWRAP 74
#define TEE_EXTEND_CRYPTO_AE_DECRYPT_WITH_DERIVED_KSECRET 75
#define TEE_EXTEND_MAILBOX_SEND_CMD                     76
#define TEE_EXTEND_KL_MSR_LUT                           77
#define TEE_EXTEND_KL_MSR_ECW_TWO_LAYER                 78
#define TEE_EXTEND_KL_MSR_ECW_TWO_LAYER_WITH_TF         79
#define TEE_EXTEND_KL_MSR_PVR_THREE_LAYER               80
#define TEE_EXTEND_KL_MSR_LOAD_CCCK                     81
#define TEE_EXTEND_CIPHER_DECRYPT_WITH_KWRAP            82
#define TEE_EXTEND_TVP_OPEN_CHAN                        83
#define TEE_EXTEND_TVP_CLOSE_CHAN                       84
#define TEE_EXTEND_TVP_BIND_CHAN                        85
#define TEE_EXTEND_KM_SET_BOOT_PARAMS                   86
#define TEE_EXTEND_KM_GET_BOOT_PARAMS                   87
#define TEE_EXTEND_CRYPTO_RUN_EXT                       88
#define TEE_EXTEND_TIMER_CREATE                         89
#define TEE_EXTEND_TIMER_DESTROY                        90
#define TEE_EXTEND_ASYMM_PUBKEY_DECRYPT                 91
#define TEE_EXTEND_VDEC_MMAP_CACHED                     92
#define TEE_EXTEND_CIPHER_ENCRYPT_WITH_KWRAP            93


// For CFG_VMX_240_COMPAT:
#define TEE_EXTEND_TVP_ENTER_VMX_2017_240               33
#define TEE_EXTEND_TVP_GET_VIDEO_SIZE_VMX_2017_240      35
#define TEE_EXTEND_TVP_SET_VIDEO_LAYER_VMX_2017_240     37
#define TEE_EXTEND_CALLBACK_VMX_2017_240                38
#define TEE_EXTEND_CRYPTO_RUN_VMX_2017_240              40
#define TEE_EXTEND_CRYPTO_SET_KEY_IV_VMX_2017_240       41
#define TEE_EXTEND_SHM_MMAP_VMX_2017_240                42
#define TEE_EXTEND_SHM_MUNMAP_VMX_2017_240              43
#define TEE_EXTEND_READ_REG_VMX_2017_240                44
#define TEE_EXTEND_WRITE_REG_VMX_2017_240               45
#define TEE_EXTEND_UPDATE_MVN_VMX_2017_240              46
#define TEE_EXTEND_READ_RNG_POOL_VMX_2017_240           47
#define TEE_EXTEND_TVP_SET_AUDIO_MUTE_VMX_2017_240      48
#define TEE_EXTEND_DESC_SET_DVR_INFO_VMX_2017_240       49
#define TEE_EXTEND_DESC_IS_DVR_VMX_2017_240             50
#define TEE_EXTEND_CALLBACK_V1                          58


struct tee_vdec_info_param {
	paddr_t pa;
	size_t size;
};

#define TEE_TVP_POOL_MAX_COUNT	4
struct tee_tvp_open_chan_param {
	uint32_t cfg;
	struct tee_vdec_info_param input;
	struct tee_vdec_info_param output[TEE_TVP_POOL_MAX_COUNT];
	TEE_Tvp_Handle handle;
};

struct tee_tvp_close_chan_param {
	TEE_Tvp_Handle handle;
};

struct tee_tvp_bind_chan_param {
	TEE_Tvp_Handle handle;
	TEE_UUID uuid;
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

struct tee_hdmi_get_state_param {
	uint32_t state;
	uint32_t reserved;
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

struct tee_tvp_audio_mute_param {
	uint32_t mute;
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

typedef struct {
	void *para;
	uint32_t para_len;
	uint8_t svc_idx;
} tee_vxwm_param;

struct tee_video_fw_param {
	void *firmware;
	uint32_t fw_size;
	void *info;
	uint32_t info_size;
};

struct tee_kl_cr_param {
	uint8_t rk_cfg_idx;
	uint8_t ek[16];
	uint8_t nonce[16];
	uint8_t dnonce[16];
};

struct tee_kl_run_param_v1 {
	unsigned int   dest;
	unsigned char  kl_num;
	unsigned char  kl_levels;
	unsigned char  __padding[6];
	unsigned char  keys[7][16];
};

struct tee_kl_run_param {
	uint8_t levels;
	uint8_t rk_cfg_idx;
	uint8_t eks[7][16];
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

struct tee_desc_set_algo_param {
	int dsc_no;
	int fd;
	int algo;
};

struct tee_desc_set_mode_param {
	int dsc_no;
	int fd;
	int mode;
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

struct tee_kl_req_ecw_param {
	struct req_ecw ecw;
};

struct tee_kl_etask_lut_param {
	struct req_etask_lut lut;
};


struct tee_desc_set_output_param {
	int module;
	int output;
};

struct tee_desc_dvr_info_param {
	uint8_t svc_idx;
	uint8_t pid_count;
	uint16_t pids[8];
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

struct tee_callback_param {
	uint32_t client_id;
	uint32_t context_id;
	uint32_t func_id;
	uint32_t cmd_id;
	uint32_t ret_size;
	uint8_t *in_buff;
	uint32_t in_size;
	uint8_t *out_buff;
	uint32_t out_size;
};

struct tee_mutex_param {
	uint32_t lock;
};

struct tee_crypto_run_param {
	char alg[64];  //CRYPTO_MAX_ALG_NAME
	uint8_t *srcaddr;
	uint32_t datalen;
	uint8_t *dstaddr;
	uint8_t *keyaddr;
	uint32_t keysize;
	uint8_t *ivaddr;
	uint8_t dir;
	uint8_t thread;
	uint8_t __padding[2];
};

struct tee_crypto_set_key_iv_param {
	uint32_t threadidx;
	uint32_t *in;
	uint32_t len;
	uint8_t swap;
	uint8_t __padding[3];
	uint32_t from_kl;
	uint32_t dest_idx;
};

struct tee_shm_param {
	uint32_t pa;
	uint32_t va;
	uint32_t size;
};

struct tee_read_reg_param {
	uint32_t reg;
	uint32_t val;
};

struct tee_write_reg_param {
	uint32_t reg;
	uint32_t val;
};

struct tee_update_mvn_param {
	uint32_t type;
	uint32_t flag;
	uint32_t check;
};

struct tee_mailbox_param {
	uint32_t command;
	uint8_t *inbuf;
	uint32_t inlen;
	uint8_t *outbuf;
	uint32_t outlen;
	uint32_t response;
};

struct tee_cipher_encrypt_with_kwrap_param {
	const uint8_t *iv;
	uint32_t iv_len;
	const uint8_t *src;
	uint32_t src_len;
	uint8_t *dst;
	uint32_t *dst_len;
};

struct tee_cipher_decrypt_with_kwrap_param {
	const uint8_t *iv;
	uint32_t iv_len;
	const uint8_t *src;
	uint32_t src_len;
	uint8_t *dst;
	uint32_t *dst_len;
};

#define SHA256_DIGEST_SIZE 32
struct tee_km_boot_params{
	uint32_t device_locked;
	uint32_t verified_boot_state;
	uint8_t verified_boot_key[SHA256_DIGEST_SIZE];
	uint8_t verified_boot_hash[SHA256_DIGEST_SIZE];
};

struct tee_read_rng_pool_param {
	uint8_t *out;
	uint32_t size;
};

struct tee_timer_param {
	uint32_t handle;
	uint32_t timeout;
	uint32_t flags;
};

#endif /* UTEE_TYPE_EXTENSIONS_H */
