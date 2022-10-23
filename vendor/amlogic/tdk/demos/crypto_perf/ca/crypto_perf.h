/*
 * Copyright (C) 2016 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 */
#ifndef CRYPTO_PERF_H
#define CRYPTO_PERF_H

#include "crypto_perf.h"

#define CA_FALSE  0
#define CA_TRUE   1

typedef struct _cipher_param_st {
	uint32_t algo;
	uint32_t mode;
	uint32_t key_type;
	uint8_t key[32];
	uint32_t key_len;
	uint8_t iv[16];
	uint32_t iv_len;
} st_cipher_param_type;

const uint32_t hash_digest_algo[] = {
	TEE_ALG_MD5,
	TEE_ALG_SHA1,
	TEE_ALG_SHA224,
	TEE_ALG_SHA256,
	TEE_ALG_SHA384,
	TEE_ALG_SHA512,
	0
};

const uint32_t cipher_aes_algo[] = {
	TEE_ALG_AES_ECB_NOPAD,
	TEE_ALG_AES_CBC_NOPAD,
	TEE_ALG_AES_CTR,
	TEE_ALG_AES_CTS,
	TEE_ALG_AES_XTS,
	TEE_ALG_AES_CBC_MAC_NOPAD,
	TEE_ALG_AES_CBC_MAC_PKCS5,
	TEE_ALG_AES_CMAC,
	TEE_ALG_AES_CCM,
	TEE_ALG_AES_GCM,
	TEE_ALG_DES_ECB_NOPAD,
	TEE_ALG_DES_CBC_NOPAD,
	TEE_ALG_DES_CBC_MAC_NOPAD,
	TEE_ALG_DES_CBC_MAC_PKCS5,
	TEE_ALG_DES3_ECB_NOPAD,
	TEE_ALG_DES3_CBC_NOPAD,
	TEE_ALG_DES3_CBC_MAC_NOPAD,
	TEE_ALG_DES3_CBC_MAC_PKCS5,
	0
};

const uint32_t crypto_test_data_len[] = {
	1 * 1024,
	4 * 1024,
	8 * 1024,
	16 * 1024,
	64 * 1024,
	1 * 1024 * 1024,
	4 * 1024 * 1024,
	8 * 1024 * 1024,
	16 * 1024 * 1024,
	32 * 1024 * 1024,
/*	64 * 1024 * 1024, */
	0
};
#define UINT32_SIZE (sizeof(uint32_t))
#define DATA_LEN_ARR_SIZE (sizeof(crypto_test_data_len))
#define ALGO_HASH_ARR_SIZE (sizeof(hash_digest_algo))
#define ALGO_CIPHER_ARR_SIZE (sizeof(cipher_aes_algo))
#define SPEED_DATA_BUFFER_LEN_NUM (DATA_LEN_ARR_SIZE/UINT32_SIZE - 1)
#define SPEED_DATA_HASH_DIGEST_NUM (ALGO_HASH_ARR_SIZE/UINT32_SIZE - 1)
#define SPPED_DATA_ALGO_CIPHER_NUM (ALGO_CIPHER_ARR_SIZE/UINT32_SIZE - 1)

struct _alg {
	uint32_t algo;
	struct timeval time_value;
	uint32_t data_len;
};
struct _hash_speed_tbl {
	uint8_t name[16];
	struct _alg data[SPEED_DATA_HASH_DIGEST_NUM][SPEED_DATA_BUFFER_LEN_NUM];
};
struct _cipher_speed_tbl {
	uint8_t name[16];
	uint32_t key_len;
	struct _alg data[SPPED_DATA_ALGO_CIPHER_NUM][SPEED_DATA_BUFFER_LEN_NUM];
};

#endif
