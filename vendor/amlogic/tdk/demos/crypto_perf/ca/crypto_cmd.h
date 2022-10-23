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
#ifndef CRYPTO_CMD_H
#define CRYPTO_CMD_H

#include <ta_crypt.h>
#include <utee_defines.h>
#include <assert.h>
#include <tee_client_api.h>
#include <tee_api_types.h>

TEEC_Result ta_crypt_cmd_allocate_operation(TEEC_Session *s,
						   TEE_OperationHandle *oph,
						   uint32_t algo, uint32_t mode,
						   uint32_t max_key_size);
TEEC_Result ta_crypt_cmd_free_operation(TEEC_Session *s,
					 TEE_OperationHandle oph);
TEEC_Result ta_crypt_cmd_copy_operation(TEEC_Session *s,
					 TEE_OperationHandle dst_oph,
					 TEE_OperationHandle src_oph);
TEE_Result ta_crypt_cmd_set_operation_key(TEEC_Session *s,
					   TEE_OperationHandle oph,
					   TEE_ObjectHandle key);
TEE_Result ta_crypt_cmd_set_operation_key2(TEEC_Session *s,
					    TEE_OperationHandle oph,
					    TEE_ObjectHandle key1,
					    TEE_ObjectHandle key2);
TEEC_Result ta_crypt_cmd_allocate_transient_object(TEEC_Session *s,
						    TEE_ObjectType obj_type,
						    uint32_t max_obj_size,
						    TEE_ObjectHandle *o);
TEEC_Result ta_crypt_cmd_populate_transient_object(TEEC_Session *s,
						    TEE_ObjectHandle o,
						    const TEE_Attribute *attrs,
							uint32_t attr_count);
TEEC_Result ta_crypt_cmd_free_transient_object(TEEC_Session *s,
						TEE_ObjectHandle o);
TEEC_Result ta_crypt_cmd_cipher_init(TEEC_Session *s,
				      TEE_OperationHandle oph, const void *iv,
				      size_t iv_len);
TEEC_Result ta_crypt_cmd_cipher_update(TEEC_Session *s,
					TEE_OperationHandle oph,
					const void *src, size_t src_len,
					void *dst, size_t *dst_len);
TEEC_Result ta_crypt_cmd_cipher_final(TEEC_Session *s,
					 TEE_OperationHandle oph,
					 const void *src, size_t src_len,
					 void *dst, size_t *dst_len);
TEEC_Result ta_crypt_cmd_digest_update(TEEC_Session *s,
					  TEE_OperationHandle oph,
					  const void *chunk, size_t chunk_size);
TEEC_Result ta_crypt_cmd_digest_do_final(TEEC_Session *s,
					  TEE_OperationHandle oph,
					  const void *chunk, size_t chunk_len,
					  void *hash, size_t *hash_len);
#endif
