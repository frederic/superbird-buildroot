/*
 * Copyright (C) 2016 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 */
#include <malloc.h>
#include <string.h>
#include <sys/time.h>
#include "crypto_cmd.h"
#include "crypto_perf.h"
#include <ta_crypt.h>

#define MAX_SHARED_MEM_SIZE (256*1024)
#define TEEC_OPERATION_INITIALIZER { 0 }

/* #define _DEBUG_SYM */
/* #define __DEBUG_HASH */

static const uint8_t cipher_data_aes_key1[] = {
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,	/* 01234567 */
	0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,	/* 89ABCDEF */
};

static const uint8_t cipher_data_128_iv1[] = {
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,	/* 12345678 */
	0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x30,	/* 9ABCDEF0 */
};

static const uint8_t cipher_data_des3_key1[] = {
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,	/* 01234567 */
	0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,	/* 89ABCDEF */
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,	/* 12345678 */
};

static const uint8_t cipher_data_64_iv1[] = {
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,	/* 12345678 */
};

static TEEC_Context crypto_speed_ctx;

const TEEC_UUID crypt_time_ta_uuid = TA_CRYPT_UUID;
char *_device = NULL;

uint8_t digest_out_buf[68];
size_t digest_out_size = 68;
static uint8_t *pbuf_in, *pbuf_out;
static uint32_t pin_len, pout_len;
static struct _cipher_speed_tbl *psym_tbl;

static TEEC_Result teec_open_session(TEEC_Session *session,
				     const TEEC_UUID *uuid,
				     TEEC_Operation *op, uint32_t *ret_orig)
{
	return TEEC_OpenSession(&crypto_speed_ctx, session, uuid,
				TEEC_LOGIN_PUBLIC, NULL, op, ret_orig);
}

static int32_t crypto_speed_message_digest(uint32_t algo, uint8_t *buf_in,
					   uint32_t in_len,
					   struct timeval *time_value,
					   TEEC_Session *session)
{
	uint32_t i = 0;
	TEE_OperationHandle op;
	uint8_t digest[64];
	size_t out_size = 64;
	uint8_t *plaintext = NULL;
	uint32_t update_cnt;
	uint32_t final_cnt;
	struct timeval start = { 0 };
	struct timeval end = { 0 };
	size_t n = 0;

	if ((NULL == buf_in) || (in_len == 0) || time_value == NULL)
		return TEEC_ERROR_BAD_PARAMETERS;

	if (ta_crypt_cmd_allocate_operation(session, &op,
					    algo, TEE_MODE_DIGEST,
					    0) != TEEC_SUCCESS) {
		printf("ta_crypt_cmd_allocate_operation Failed\n");
		return -1;
	}

	plaintext = (uint8_t *) malloc(MAX_SHARED_MEM_SIZE);
	if (plaintext == NULL) {
		printf("Unable to allocate memory for plaintext\n");
		ta_crypt_cmd_free_operation(session, op);
		return -1;
	}

	update_cnt = in_len / MAX_SHARED_MEM_SIZE;
	final_cnt = in_len % MAX_SHARED_MEM_SIZE;

	gettimeofday(&start, NULL);

	i = 0;
	while (1) {
		if (update_cnt == 0) {
			memcpy(plaintext,
			       buf_in + update_cnt * MAX_SHARED_MEM_SIZE,
			       final_cnt);
			n = final_cnt;
		} else {
			if (i < update_cnt) {
				memcpy(plaintext,
				       buf_in + i * MAX_SHARED_MEM_SIZE,
				       MAX_SHARED_MEM_SIZE);
				n = MAX_SHARED_MEM_SIZE;
				i++;
			} else {
				plaintext = NULL;
				n = 0;
			}
		}

		if (n == MAX_SHARED_MEM_SIZE) {
			if (ta_crypt_cmd_digest_update(session, op,
						       plaintext,
						       n) != TEEC_SUCCESS)
				goto out;
		} else {
			if (ta_crypt_cmd_digest_do_final(session, op,
							 plaintext,
							 n, digest,
							 &out_size) !=
			    TEEC_SUCCESS)
				goto out;
			break;
		}
	};

	gettimeofday(&end, NULL);

	time_value->tv_sec = (end.tv_sec - start.tv_sec);
	time_value->tv_usec = (end.tv_usec - start.tv_usec);

	if (ta_crypt_cmd_free_operation(session, op) != TEEC_SUCCESS)
		goto out;

	if (plaintext)
		free(plaintext);

	return TEEC_SUCCESS;
out:
	if (plaintext)
		free(plaintext);

	return -1;
}

static inline uint8_t sym_algo_is_notsupport(uint32_t algo, uint32_t key_len)
{
	uint8_t res = CA_FALSE;

	if ((algo == TEE_ALG_DES_CBC_NOPAD) && key_len == (128 / 8))
		res = CA_TRUE;
	else if ((algo == TEE_ALG_DES_CBC_NOPAD) && key_len == (192 / 8))
		res = CA_TRUE;
	else if ((algo == TEE_ALG_DES_CBC_NOPAD) && key_len == (224 / 8))
		res = CA_TRUE;
	else if ((algo == TEE_ALG_DES_CBC_NOPAD) && key_len == (256 / 8))
		res = CA_TRUE;
	else if ((algo == TEE_ALG_DES3_CBC_NOPAD) && key_len == (224 / 8))
		res = CA_TRUE;
	else if ((algo == TEE_ALG_DES3_CBC_NOPAD) && key_len == (256 / 8))
		res = CA_TRUE;

	return (uint8_t) res;
}

static inline void algo_printf(uint32_t algo)
{
	switch (algo) {
	case TEE_ALG_MD5:
		printf("TEE_ALG_MD5");
		break;
	case TEE_ALG_SHA1:
		printf("TEE_ALG_SHA1");
		break;
	case TEE_ALG_SHA224:
		printf("TEE_ALG_SHA224");
		break;
	case TEE_ALG_SHA256:
		printf("TEE_ALG_SHA256");
		break;
	case TEE_ALG_SHA384:
		printf("TEE_ALG_SHA384");
		break;
	case TEE_ALG_SHA512:
		printf("TEE_ALG_SHA512");
		break;
	case TEE_ALG_AES_ECB_NOPAD:
		printf("TEE_ALG_AES_ECB_NOPAD");
		break;
	case TEE_ALG_AES_CBC_NOPAD:
		printf("TEE_ALG_AES_CBC_NOPAD");
		break;
	case TEE_ALG_AES_CTR:
		 printf("TEE_ALG_AES_CTR");
		 break;
	case TEE_ALG_AES_CTS:
		 printf("TEE_ALG_AES_CTS");
		 break;
	case TEE_ALG_AES_XTS:
		 printf("TEE_ALG_AES_XTS");
		 break;
	case TEE_ALG_AES_CBC_MAC_NOPAD:
		 printf("TEE_ALG_AES_CBC_MAC_NOPAD");
		 break;
	case TEE_ALG_AES_CBC_MAC_PKCS5:
		 printf("TEE_ALG_AES_CBC_MAC_PKCS5");
		 break;
	case TEE_ALG_AES_CMAC:
		 printf("TEE_ALG_AES_CMAC");
		 break;
	case TEE_ALG_AES_CCM:
		 printf("TEE_ALG_AES_CCM");
		 break;
	case TEE_ALG_DES_ECB_NOPAD:
		 printf("TEE_ALG_DES_ECB_NOPAD");
		 break;
	case TEE_ALG_DES_CBC_NOPAD:
		 printf("TEE_ALG_DES_CBC_NOPAD");
		 break;
	case TEE_ALG_DES_CBC_MAC_NOPAD:
		 printf("TEE_ALG_DES_CBC_MAC_NOPAD");
		 break;
	case TEE_ALG_DES_CBC_MAC_PKCS5:
		 printf("TEE_ALG_DES_CBC_MAC_PKCS5");
		 break;
	case TEE_ALG_DES3_ECB_NOPAD:
		 printf("TEE_ALG_DES3_ECB_NOPAD");
		 break;
	case TEE_ALG_DES3_CBC_NOPAD:
		 printf("TEE_ALG_DES3_CBC_NOPAD");
		 break;
	case TEE_ALG_DES3_CBC_MAC_NOPAD:
		 printf("TEE_ALG_DES3_CBC_MAC_NOPAD");
		 break;
	case TEE_ALG_DES3_CBC_MAC_PKCS5:
		 printf("TEE_ALG_DES3_CBC_MAC_PKCS5");
		 break;
	default:
		 printf("The ALG %d is not support !\n", algo);
		 break;
	}
}

void crypto_speed_msgtbl_print(struct _hash_speed_tbl *ptbl)
{
	uint32_t i, j;
	float speed_in_kb, spent_time;
	float speed;

	for (i = 0; i < SPEED_DATA_HASH_DIGEST_NUM; i++) {
		algo_printf(hash_digest_algo[i]);
		printf("\n");

#ifdef __DEBUG_HASH
		if (hash_digest_algo[i] == TEE_ALG_MD5) {
			printf("tv_usec = %ld,tv_sec = %ld\n",
			       ptbl->data[i][0].time_value.tv_usec,
			       ptbl->data[i][0].time_value.tv_sec);
			spent_time =
			    (ptbl->data[i][0].time_value.tv_sec) +
			    (ptbl->data[i][0].time_value.tv_usec / 1000000.0);
			printf("spent_time = %8.3f\n", spent_time);
			speed = crypto_test_data_len[0] / spent_time;
			printf("speed = %8.3f\n", speed);
			speed_in_kb = speed / 1024.0;
			printf("speed_in_kb = %8.3f", speed_in_kb);
		}
#endif

		printf("-----------------+---------------+----------------\n");
		printf(" Data Size (KB)  | Time (s)      | Speed (KB/s)\n");
		printf("-----------------+---------------+----------------\n");

		for (j = 0; j < SPEED_DATA_BUFFER_LEN_NUM; j++) {
			spent_time = (ptbl->data[i][j].time_value.tv_sec) +
			    (ptbl->data[i][j].time_value.tv_usec / 1000000.0);
			speed = crypto_test_data_len[j] / spent_time;
			speed_in_kb = speed / 1024.0;

			printf(" %8zd        | %8.5f      | %8.3f\n",
			       crypto_test_data_len[j] / 1024, spent_time,
			       speed_in_kb);

		}
		printf("-----------------+---------------+----------------\n");

	}

}

void crypto_speed_symtbl_print(struct _cipher_speed_tbl *ptbl,
			       st_cipher_param_type *sym_param)
{
	uint32_t i, j;
	float speed_in_kb, spent_time;
	float speed;

	for (i = 0; i < SPPED_DATA_ALGO_CIPHER_NUM; i++) {
		if ((cipher_aes_algo[i] != TEE_ALG_AES_CBC_NOPAD) &&
		    (cipher_aes_algo[i] != TEE_ALG_AES_CTR) &&
		    (cipher_aes_algo[i] != TEE_ALG_DES_CBC_NOPAD) &&
		    (cipher_aes_algo[i] != TEE_ALG_DES3_CBC_NOPAD)
		    ) {
			continue;
		}
		if (sym_algo_is_notsupport
		    (cipher_aes_algo[i], sym_param->key_len))
			continue;

		algo_printf(cipher_aes_algo[i]);
		if (sym_param->mode == TEE_MODE_ENCRYPT)
			printf(",TEE_MODE_ENCRYPT,");
		else if (sym_param->mode == TEE_MODE_DECRYPT)
			printf(",TEE_MODE_DECRYPT,");
		printf("key_len=%d,iv_len=%d",
		       sym_param->key_len * 8, 8 * sym_param->iv_len);
		printf("\n");

		printf("-----------------+---------------+----------------\n");
		printf(" Data Size (KB)  | Time (s)      | Speed (KB/s)\n");
		printf("-----------------+---------------+----------------\n");

		for (j = 0; j < SPEED_DATA_BUFFER_LEN_NUM; j++) {
			spent_time = (ptbl->data[i][j].time_value.tv_sec) +
			    (ptbl->data[i][j].time_value.tv_usec / 1000000.0);
			speed = crypto_test_data_len[j] / spent_time;
			speed_in_kb = speed / 1024.0;

			printf(" %8zd        | %8.5f      | %8.3f\n",
			       crypto_test_data_len[j] / 1024, spent_time,
			       speed_in_kb);

		}
		printf("-----------------+---------------+----------------\n");

	}
}

void crypto_speed_hash_digest(TEEC_Session *session)
{
	uint32_t i, j;
	struct timeval tval = { 0 };
	uint8_t *buf_in;
	TEEC_Result res;
	struct _hash_speed_tbl *phash_tbl;

	phash_tbl = malloc(sizeof(struct _hash_speed_tbl));

	printf("**********************************************************\n");
	printf("      Test message digest algorithm time and speed :\n");
	printf("**********************************************************\n");
	printf("\n");

	for (i = 0; i < SPEED_DATA_HASH_DIGEST_NUM; i++) {
		for (j = 0; j < SPEED_DATA_BUFFER_LEN_NUM; j++) {
			buf_in = (uint8_t *) malloc(crypto_test_data_len[j]);
			if (buf_in == NULL) {
				printf(" malloc memory failed\n");
				free(phash_tbl);
				return;
			}
			res = crypto_speed_message_digest(hash_digest_algo[i],
							  buf_in,
							  crypto_test_data_len
							  [j], &tval, session);
			if (res != TEEC_SUCCESS) {
				printf("speed message digest failed !\n ");
				free(buf_in);
				free(phash_tbl);
				return;
			}
			free(buf_in);
			/*
			memcpy(phash_tbl->name,"HASH",sizeof("HASH"));
			*/
#ifdef __DEBUG_HASH
			if ((hash_digest_algo[i] == TEE_ALG_MD5)
			    && (crypto_test_data_len[j] == 1024)) {
				printf("tv_sec = %ld,tv_usec = %ld\n",
				       tval.tv_sec, tval.tv_usec);
			}
			if ((hash_digest_algo[i] == TEE_ALG_MD5)
			    && (crypto_test_data_len[j] == 4096)) {
				printf("tv_sec = %ld,tv_usec = %ld\n",
				       tval.tv_sec, tval.tv_usec);
			}
#endif

			phash_tbl->data[i][j].algo = hash_digest_algo[i];
			phash_tbl->data[i][j].data_len =
			    crypto_test_data_len[j];
			phash_tbl->data[i][j].time_value.tv_usec = tval.tv_usec;
			phash_tbl->data[i][j].time_value.tv_sec = tval.tv_sec;

		}
	}
	crypto_speed_msgtbl_print(phash_tbl);
	free(phash_tbl);

}

/* the buf should be large enough to contain all the padded data */
unsigned int do_pkcs5_padding(unsigned char *buf, unsigned int len,
			      unsigned int block_size)
{
	unsigned int bytes_to_pad = 0;
	unsigned int i = 0;

	bytes_to_pad = ((len + block_size) / block_size) * block_size - len;

	for (i = len; i < len + bytes_to_pad; i++)
		buf[i] = bytes_to_pad;

	return bytes_to_pad;
}

int32_t buffer_read_data(uint8_t *pbuf, uint8_t *buf, uint32_t len,
			 uint32_t *bytes_padded, uint32_t block_size)
{
	uint32_t n = 0;
	uint32_t pad = 0;

	if (!buf)
		return 0;
	memcpy(buf, pbuf, len);
	n = len;

	if (bytes_padded) {
		if (n < len)
			*bytes_padded = do_pkcs5_padding(buf, n, block_size);
		else
			*bytes_padded = 0;
		pad = *bytes_padded;
	}
	pad = pad + n;

	return pad;
}

void buffer_write_data(uint8_t *pbuf, const uint8_t *buf, uint32_t len)
{
	memcpy(pbuf, buf, len);
}

unsigned int detect_pkcs5_padding_len(unsigned char *buf, unsigned int len)
{
	unsigned int bytes_to_pad = 0;
	unsigned int tmp = 0;

	tmp = len - 1 - bytes_to_pad;
	for (bytes_to_pad = 1; buf[tmp] == buf[len - 1]; bytes_to_pad++)
		;

	return bytes_to_pad;
}

static void sym_ciphers(uint32_t algo,
			uint32_t mode, uint32_t key_type, const uint8_t *key,
			uint32_t key_len, const uint8_t *iv, uint32_t iv_len,
			uint8_t *pbuf_in, uint32_t pin_len, uint8_t *pbuf_out,
			uint32_t pout_len, struct timeval *time_value,
			TEEC_Session *session)
{
	TEE_OperationHandle op;
	TEE_ObjectHandle key1_handle = TEE_HANDLE_NULL;
	size_t out_size = MAX_SHARED_MEM_SIZE;
	TEE_Attribute key_attr;
	size_t key_size;
	uint8_t *plaintext = NULL;
	uint8_t *ciphertext = NULL;
	uint8_t *buf_in = NULL;
	uint8_t *buf_out = NULL;
	uint32_t bytes_padded = 0;
	uint32_t data_len = 0;
	uint32_t block_size = 0;
	uint32_t buf_cnt = 0;
	struct timeval start, end;
	TEEC_Result res;
	uint32_t mem_cnt;

	if ((pbuf_in == NULL) || (pin_len == 0) || (pbuf_out == NULL)
	    || (pout_len == 0)) {
		printf("function:%s params is NULL !\n", __func__);
		return;
	}

	if (key_type == TEE_TYPE_AES)
		block_size = 16;
	else if (key_type == TEE_TYPE_DES || key_type == TEE_TYPE_DES3)
		block_size = 8;

	plaintext =
	    (unsigned char *)calloc(MAX_SHARED_MEM_SIZE, sizeof(unsigned char));
	if (plaintext == NULL) {
		printf("Unable to allocate memory for plaintext\n");
		goto out;
	}
	ciphertext =
	    (unsigned char *)calloc(MAX_SHARED_MEM_SIZE, sizeof(unsigned char));
	if (ciphertext == NULL) {
		printf("Unable to allocate memory for ciphertext\n");
		goto out;
	}

	key_attr.attributeID = TEE_ATTR_SECRET_VALUE;
	key_attr.content.ref.buffer = (void *)key;
	key_attr.content.ref.length = key_len;

	key_size = key_attr.content.ref.length * 8;

	if (key_type == TEE_TYPE_DES || key_type == TEE_TYPE_DES3)
		/* Exclude parity in bit size of key */
		key_size -= key_size / 8;

	res = ta_crypt_cmd_allocate_operation(session, &op, algo, mode,
					      key_size);
	if (res != TEEC_SUCCESS) {
		printf("ta_crypt_cmd_allocate_operation failed ! res = 0x%x\n",
		       res);
		algo_printf(algo);
		printf(",key_size = %d\n", key_size);
		goto out;
	}

	if (ta_crypt_cmd_allocate_transient_object
	    (session, key_type, key_size, &key1_handle) != TEEC_SUCCESS) {
		printf("ta_crypt_cmd_allocate_transient_object failed !\n");
		goto out;
	}

	if (ta_crypt_cmd_populate_transient_object
	    (session, key1_handle, &key_attr, 1) != TEEC_SUCCESS) {
		printf("ta_crypt_cmd_populate_transient_object failed !\n");
		goto out;
	}

	if (ta_crypt_cmd_set_operation_key(session, op, key1_handle) !=
	    TEEC_SUCCESS) {
		printf("ta_crypt_cmd_set_operation_key failed !\n");
		goto out;
	}

	if (ta_crypt_cmd_free_transient_object(session, key1_handle) !=
	    TEEC_SUCCESS) {
		printf("ta_crypt_cmd_free_transient_object failed !\n");
		goto out;
	}
	if (ta_crypt_cmd_cipher_init(session, op, iv, iv_len) != TEEC_SUCCESS) {
		printf("ta_crypt_cmd_cipher_init failed !\n");
		goto out;
	}

	if (mode == TEE_MODE_ENCRYPT) {
		buf_in = plaintext;
		buf_out = ciphertext;
	} else {
		buf_in = ciphertext;
		buf_out = plaintext;
	}
#ifdef _DEBUG_SYM
	printf("cipher start here ......\n");
	printf("block_size = %d\n", block_size);
	printf("key_size = %d\n", key_size);
#endif

	mem_cnt = pin_len / MAX_SHARED_MEM_SIZE;

	gettimeofday(&start, NULL);

	if (mode == TEE_MODE_ENCRYPT) {
		buf_cnt = 0;
		while (bytes_padded == 0) {
#ifdef _DEBUG_SYM
			printf("-------------------------------------\n");
			printf("buf_cnt = %d\n", buf_cnt);
			printf("mem_cnt = %d\n", mem_cnt);
			printf("block_size = %d\n", block_size);
			printf("-------------------------------------\n");
#endif
			if (buf_cnt < mem_cnt) {
				data_len =
				    buffer_read_data(pbuf_in +
						     buf_cnt *
						     MAX_SHARED_MEM_SIZE,
						     buf_in,
						     MAX_SHARED_MEM_SIZE,
						     &bytes_padded, block_size);
#ifdef _DEBUG_SYM
				printf("bytes_padded = %d\n", bytes_padded);
#endif
			} else if ((buf_cnt == mem_cnt)
				   && ((mem_cnt * MAX_SHARED_MEM_SIZE) <
				       pin_len)) {
				data_len =
				    buffer_read_data(pbuf_in +
						     buf_cnt *
						     MAX_SHARED_MEM_SIZE,
						     buf_in,
						     pin_len -
						     (buf_cnt *
						      MAX_SHARED_MEM_SIZE),
						     &bytes_padded, block_size);
			} else {
				data_len = 0;
				bytes_padded = 0;
				break;
			}
			if (!bytes_padded) {
#ifdef _DEBUG_SYM
				printf("update !\n");
				printf("data_len = %d\n", data_len);
#endif
				res = ta_crypt_cmd_cipher_update(session, op,
								 buf_in,
								 data_len,
								 buf_out,
								 &out_size);
				if (res != TEEC_SUCCESS) {
					printf("cipher update failed !\n");
					algo_printf(algo);
					printf("\n");
					goto out;
				}
			} else {
				if (algo == TEE_ALG_AES_CTR)
					data_len -= bytes_padded;
				res = ta_crypt_cmd_cipher_final(session, op,
								buf_in,
								data_len,
								buf_out,
								(size_t *) &
								out_size);
				if (res != TEEC_SUCCESS) {
					printf("cipher do final failed!\n");
					goto out;
				}
			}
			buffer_write_data(pbuf_out +
					  buf_cnt * MAX_SHARED_MEM_SIZE,
					  buf_out, out_size);
			buf_cnt++;
		}
	} else {
		buf_cnt = 0;
		do {
			if (buf_cnt < mem_cnt) {
				data_len =
				    buffer_read_data(pbuf_in +
						     buf_cnt *
						     MAX_SHARED_MEM_SIZE,
						     buf_in,
						     MAX_SHARED_MEM_SIZE,
						     &bytes_padded, block_size);
			} else if ((buf_cnt == mem_cnt)
				   && ((mem_cnt * MAX_SHARED_MEM_SIZE) <
				       pin_len)) {
				data_len =
				    buffer_read_data(pbuf_in +
						     buf_cnt *
						     MAX_SHARED_MEM_SIZE,
						     buf_in,
						     pin_len -
						     (buf_cnt *
						      MAX_SHARED_MEM_SIZE),
						     &bytes_padded, block_size);
			} else {
				break;
			}
			if (data_len == MAX_SHARED_MEM_SIZE) {
				res =
				    ta_crypt_cmd_cipher_update(session, op,
							       buf_in, data_len,
							       buf_out,
							       &out_size);
				if (res != TEEC_SUCCESS) {
					printf("cipher update failed!\n");
					goto out;
				}
			} else {
				res =
				    ta_crypt_cmd_cipher_final(session, op,
							      buf_in, data_len,
							      buf_out,
							      &out_size);
				if (res != TEEC_SUCCESS) {
					printf("cipher do final failed!\n");
					goto out;
				}
			}
			if (data_len < MAX_SHARED_MEM_SIZE &&
			    (algo == TEE_ALG_AES_CBC_NOPAD ||
			     algo == TEE_ALG_DES_CBC_NOPAD ||
			     algo == TEE_ALG_DES3_CBC_NOPAD)) {
				bytes_padded =
				    detect_pkcs5_padding_len(buf_out, out_size);
				out_size -= bytes_padded;
			}
			if (buf_cnt == pout_len)
				printf("out buffer is not enough!\n");
			buffer_write_data(pbuf_out +
					  buf_cnt * MAX_SHARED_MEM_SIZE,
					  buf_out, out_size);
			buf_cnt++;
		} while (data_len == MAX_SHARED_MEM_SIZE);
	}

	gettimeofday(&end, NULL);

	time_value->tv_sec = (end.tv_sec - start.tv_sec);
	time_value->tv_usec = (end.tv_usec - start.tv_usec);

	if (ta_crypt_cmd_free_operation(session, op) != TEEC_SUCCESS) {
		printf("ta_crypt_cmd_free_operation failed !\n");
		goto out;
	}

out:
	if (plaintext)
		free(plaintext);
	if (ciphertext)
		free(ciphertext);
}

static void sym_ciphers_data_test(int i, st_cipher_param_type *sym_param,
				  struct timeval *tval, TEEC_Session *session)
{
	uint32_t j;

	if ((cipher_aes_algo[i] == TEE_ALG_AES_CBC_NOPAD)
	    || (cipher_aes_algo[i] == TEE_ALG_AES_CTR)) {
		memcpy(sym_param->key, cipher_data_aes_key1,
		       sym_param->key_len);
		memcpy(sym_param->iv, cipher_data_128_iv1, sym_param->iv_len);
	} else {
		memcpy(sym_param->key, cipher_data_des3_key1,
		       sym_param->key_len);
		memcpy(sym_param->iv, cipher_data_64_iv1, sym_param->iv_len);
	}

	for (j = 0; j < SPEED_DATA_BUFFER_LEN_NUM; j++) {
		pin_len = crypto_test_data_len[j];
		pbuf_in = (uint8_t *) malloc(pin_len);
		if (pbuf_in == NULL) {
			printf("function: %s,line %d,allocate memory failed!\n",
			     __func__, __LINE__);
			return;
		}

		pout_len = crypto_test_data_len[j];
		pbuf_out = (uint8_t *) malloc(pout_len);
		if (pbuf_out == NULL) {
			printf("function: %s,line %d,allocate memory failed!\n",
			     __func__, __LINE__);
			return;
		}
#ifdef _DEBUG_SYM
		printf("sym_ciphers start .........\n");

		printf("algo = 0x%x\n", sym_param->algo);
		printf("mode = 0x%x\n", sym_param->mode);
		printf("key_type = 0x%x\n", sym_param->key_type);
		printf("key_len = %d\n", sym_param->key_len);
		printf("iv_len = %d\n", sym_param->iv_len);
		printf("pin_len = %d\n", pin_len);
		printf("pout_len = %d\n", pout_len);
#endif

		sym_ciphers(sym_param->algo, sym_param->mode,
			    sym_param->key_type, sym_param->key,
			    sym_param->key_len, sym_param->iv,
			    sym_param->iv_len, pbuf_in, pin_len, pbuf_out,
			    pout_len, tval, session);
#ifdef _DEBUG_SYM
		printf("tval.usec = %ld,tval.sec = %ld\n", tval->tv_usec,
		       tval->tv_sec);
		printf("sym_ciphers end .........\n");
#endif
		free(pbuf_in);
		free(pbuf_out);
		pin_len = 0;
		pout_len = 0;

		psym_tbl->data[i][j].algo = cipher_aes_algo[i];
		psym_tbl->data[i][j].data_len = crypto_test_data_len[j];
		psym_tbl->data[i][j].time_value.tv_usec = tval->tv_usec;
		psym_tbl->data[i][j].time_value.tv_sec = tval->tv_sec;
		psym_tbl->key_len = sym_param->key_len;
	}

}

void sym_ciphers_print(uint32_t mode, uint32_t key_len, TEEC_Session *session)
{
	uint32_t i;
	st_cipher_param_type sym_param = { 0 };
	struct timeval tval = { 0 };

	psym_tbl =
	    (struct _cipher_speed_tbl *)
	    malloc(sizeof(struct _cipher_speed_tbl));

	for (i = 0; i < SPPED_DATA_ALGO_CIPHER_NUM; i++) {
		/*
		   Only test the following algorithm for the time being :
		   TEE_ALG_AES_CBC_NOPAD
		   TEE_ALG_AES_CTR
		   TEE_ALG_DES_CBC_NOPAD
		   TEE_ALG_DES3_CBC_NOPAD
		 */
		if ((cipher_aes_algo[i] != TEE_ALG_AES_CBC_NOPAD) &&
		    (cipher_aes_algo[i] != TEE_ALG_AES_CTR) &&
		    (cipher_aes_algo[i] != TEE_ALG_DES_CBC_NOPAD) &&
		    (cipher_aes_algo[i] != TEE_ALG_DES3_CBC_NOPAD)
		    ) {
			continue;
		}
		if ((cipher_aes_algo[i] & 0x000000ff) == 0x00000010) {
			sym_param.key_type = TEE_TYPE_AES;
			sym_param.iv_len = 16;	/* 128 */
		} else if ((cipher_aes_algo[i] & 0x000000ff) == 0x00000011) {
			sym_param.key_type = TEE_TYPE_DES;
			sym_param.iv_len = 8;
		} else if ((cipher_aes_algo[i] & 0x000000ff) == 0x00000013) {
			sym_param.key_type = TEE_TYPE_DES3;
			sym_param.iv_len = 8;
		} else {
			printf("This is not cipher algo : %d\n",
			       cipher_aes_algo[i]);
		}

		sym_param.mode = mode;
		sym_param.algo = cipher_aes_algo[i];
		sym_param.key_len = key_len;

		if (sym_algo_is_notsupport(cipher_aes_algo[i], key_len))
			continue;
		sym_ciphers_data_test(i, &sym_param, &tval, session);
	}
	crypto_speed_symtbl_print(psym_tbl, &sym_param);
	free(psym_tbl);

}

void crypto_speed_sym_ciphers(TEEC_Session *session)
{
	printf("******************************************************\n");
	printf("              Test encrypt ciphers\n\n");
	printf("******************************************************\n\n");

	sym_ciphers_print(TEE_MODE_ENCRYPT, 128 / 8, session);
	sym_ciphers_print(TEE_MODE_ENCRYPT, 192 / 8, session);
	sym_ciphers_print(TEE_MODE_ENCRYPT, 256 / 8, session);

	printf("******************************************************\n");
	printf("              Test decrypt ciphers\n\n");
	printf("******************************************************\n\n");

	sym_ciphers_print(TEE_MODE_ENCRYPT, 128 / 8, session);
	sym_ciphers_print(TEE_MODE_ENCRYPT, 192 / 8, session);
	sym_ciphers_print(TEE_MODE_ENCRYPT, 256 / 8, session);
}

int main(int argc, char *argv[])
{
	TEEC_Session session = { 0 };
	uint32_t ret_orig;

	TEEC_InitializeContext(_device, &crypto_speed_ctx);

	if (teec_open_session(&session, &crypt_time_ta_uuid, NULL,
			      &ret_orig) != TEEC_SUCCESS)
		return -1;

	/* MD5,SHA1,SHA224,SHA256,SHA384,SHA512 */
	crypto_speed_hash_digest(&session);

	/* AES,DES,DES3 */
	crypto_speed_sym_ciphers(&session);

	TEEC_CloseSession(&session);

	TEEC_FinalizeContext(&crypto_speed_ctx);

	return 0;
}
