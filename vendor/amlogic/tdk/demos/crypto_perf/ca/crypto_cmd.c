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
#include "crypto_cmd.h"
#include <malloc.h>
#include <string.h>
#include <util.h>

#define TEEC_OPERATION_INITIALIZER { 0 }
struct tee_attr_packed {
	uint32_t attr_id;
	uint32_t a;
	uint32_t b;
};
TEEC_Result ta_crypt_cmd_allocate_operation(TEEC_Session *s,
						TEE_OperationHandle *oph,
						uint32_t algo, uint32_t mode,
						uint32_t max_key_size)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	op.params[0].value.a = 0;
	op.params[0].value.b = algo;
	op.params[1].value.a = mode;
	op.params[1].value.b = max_key_size;
	op.paramTypes =  TEEC_PARAM_TYPES(TEEC_VALUE_INOUT,
					  TEEC_VALUE_INPUT,
					  TEEC_NONE, TEEC_NONE);
	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_ALLOCATE_OPERATION,
				&op, &ret_orig);
	if (res == TEEC_SUCCESS)
		*oph = (TEE_OperationHandle) (uintptr_t) op.params[0].value.a;

	return res;
}

TEEC_Result ta_crypt_cmd_free_operation(TEEC_Session *s,
					TEE_OperationHandle oph)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	op.params[0].value.a = (uint32_t) (uintptr_t) oph;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_NONE, TEEC_NONE, TEEC_NONE);
	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_FREE_OPERATION,
				 &op, &ret_orig);

	return res;
}

TEEC_Result ta_crypt_cmd_reset_operation(TEEC_Session *s,
					 TEE_OperationHandle oph)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	assert((uintptr_t) oph <= UINT32_MAX);
	op.params[0].value.a = (uint32_t) (uintptr_t) oph;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_NONE, TEEC_NONE, TEEC_NONE);
	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_RESET_OPERATION,
				 &op, &ret_orig);
	if (res != TEEC_SUCCESS) {
		printf("ta_crypt_cmd_reset_operation Failed with ret code %d\n",
			 ret_orig);
	}

	return res;
}

TEEC_Result ta_crypt_cmd_copy_operation(TEEC_Session *s,
					TEE_OperationHandle dst_oph,
					TEE_OperationHandle src_oph)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	assert((uintptr_t) dst_oph <= UINT32_MAX);
	op.params[0].value.a = (uint32_t) (uintptr_t) dst_oph;
	assert((uintptr_t) src_oph <= UINT32_MAX);
	op.params[0].value.b = (uint32_t) (uintptr_t) src_oph;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_NONE, TEEC_NONE, TEEC_NONE);
	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_COPY_OPERATION,
				 &op, &ret_orig);
	if (res != TEEC_SUCCESS) {
		printf("ta_crypt_cmd_copy_operation Failed with ret code %d\n",
			 ret_orig);
	}

	return res;
}

TEE_Result ta_crypt_cmd_set_operation_key(TEEC_Session *s,
					  TEE_OperationHandle oph,
					  TEE_ObjectHandle key)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	assert((uintptr_t) oph <= UINT32_MAX);
	op.params[0].value.a = (uint32_t) (uintptr_t) oph;
	assert((uintptr_t) key <= UINT32_MAX);
	op.params[0].value.b = (uint32_t) (uintptr_t) key;
	op.paramTypes =  TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					  TEEC_NONE, TEEC_NONE, TEEC_NONE);
	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_SET_OPERATION_KEY,
				 &op, &ret_orig);

	return res;
}

TEE_Result ta_crypt_cmd_set_operation_key2(TEEC_Session *s,
					   TEE_OperationHandle oph,
					   TEE_ObjectHandle key1,
					   TEE_ObjectHandle key2)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	assert((uintptr_t) oph <= UINT32_MAX);
	op.params[0].value.a = (uint32_t) (uintptr_t) oph;
	assert((uintptr_t) key1 <= UINT32_MAX);
	op.params[0].value.b = (uint32_t) (uintptr_t) key1;
	assert((uintptr_t) key2 <= UINT32_MAX);
	op.params[1].value.a = (uint32_t) (uintptr_t) key2;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_VALUE_INPUT,
					 TEEC_NONE, TEEC_NONE);
	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_SET_OPERATION_KEY2, &op,
				   &ret_orig);
	if (res != TEEC_SUCCESS) {
		printf("ta_crypt_cmd_set_operation_key2 Failed ret code %d\n",
			 ret_orig);
	}

	return res;
}

TEEC_Result ta_crypt_cmd_allocate_transient_object(TEEC_Session *s,
						   TEE_ObjectType obj_type,
						   uint32_t max_obj_size,
						   TEE_ObjectHandle *o)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	op.params[0].value.a = obj_type;
	op.params[0].value.b = max_obj_size;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_VALUE_OUTPUT,
					 TEEC_NONE, TEEC_NONE);
	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_ALLOCATE_TRANSIENT_OBJECT, &op,
				   &ret_orig);
	if (res == TEEC_SUCCESS)
		*o = (TEE_ObjectHandle) (uintptr_t) op.params[1].value.a;

	return res;
}

TEE_Result ta_crypt_pack_attrs(const TEE_Attribute *attrs,
				   uint32_t attr_count, uint8_t **buf,
				   size_t *blen)
{
	struct tee_attr_packed *a;
	uint8_t *b;
	size_t bl;
	size_t n;

	*buf = NULL;
	*blen = 0;
	if (attr_count == 0)
		return TEE_SUCCESS;
	bl = sizeof(uint32_t) + sizeof(struct tee_attr_packed) * attr_count;
	for (n = 0; n < attr_count; n++) {
		if ((attrs[n].attributeID & TEE_ATTR_BIT_VALUE) != 0)
			continue;	/* Only memrefs need to be updated */
		if (!attrs[n].content.ref.buffer)
			continue;

		/* Make room for padding */
		bl += ROUNDUP(attrs[n].content.ref.length, 4);
	}
	b = calloc(1, bl);
	if (!b)
		return TEE_ERROR_OUT_OF_MEMORY;
	*buf = b;
	*blen = bl;
	*(uint32_t *) (void *)b = attr_count;
	b += sizeof(uint32_t);
	a = (struct tee_attr_packed *)(void *)b;
	b += sizeof(struct tee_attr_packed) * attr_count;
	for (n = 0; n < attr_count; n++) {
		a[n].attr_id = attrs[n].attributeID;
		if (attrs[n].attributeID & TEE_ATTR_BIT_VALUE) {
			a[n].a = attrs[n].content.value.a;
			a[n].b = attrs[n].content.value.b;
			continue;
		}
		a[n].b = attrs[n].content.ref.length;
		if (!attrs[n].content.ref.buffer) {
			a[n].a = 0;
			continue;
		}
		memcpy(b, attrs[n].content.ref.buffer,
			   attrs[n].content.ref.length);

		/* Make buffer pointer relative to *buf */
		a[n].a = (uint32_t) (uintptr_t) (b - *buf);

		/* Round up to good alignment */
		b += ROUNDUP(attrs[n].content.ref.length, 4);
	}
	return TEE_SUCCESS;
}

TEEC_Result ta_crypt_cmd_populate_transient_object(TEEC_Session *s,
						TEE_ObjectHandle o,
						const TEE_Attribute *attrs,
						uint32_t attr_count)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;
	uint8_t *buf;
	size_t blen;

	res = ta_crypt_pack_attrs(attrs, attr_count, &buf, &blen);
	if (res != TEEC_SUCCESS)
		return res;
	assert((uintptr_t) o <= UINT32_MAX);
	op.params[0].value.a = (uint32_t) (uintptr_t) o;
	op.params[1].tmpref.buffer = buf;
	op.params[1].tmpref.size = blen;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_NONE, TEEC_NONE);
	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_POPULATE_TRANSIENT_OBJECT, &op,
				   &ret_orig);
	free(buf);

	return res;
}

TEEC_Result ta_crypt_cmd_free_transient_object(TEEC_Session *s,
						   TEE_ObjectHandle o)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	assert((uintptr_t) o <= UINT32_MAX);
	op.params[0].value.a = (uint32_t) (uintptr_t) o;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_NONE, TEEC_NONE, TEEC_NONE);
	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_FREE_TRANSIENT_OBJECT, &op,
				   &ret_orig);

	return res;
}

TEEC_Result ta_crypt_cmd_cipher_init(TEEC_Session *s,
				 TEE_OperationHandle oph, const void *iv,
				 size_t iv_len)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	assert((uintptr_t) oph <= UINT32_MAX);
	op.params[0].value.a = (uint32_t) (uintptr_t) oph;
	if (iv != NULL) {
		op.params[1].tmpref.buffer = (void *)iv;
		op.params[1].tmpref.size = iv_len;
		op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
						 TEEC_MEMREF_TEMP_INPUT,
						 TEEC_NONE, TEEC_NONE);
	} else {
		op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
						 TEEC_NONE, TEEC_NONE,
						 TEEC_NONE);
	}
	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_CIPHER_INIT, &op, &ret_orig);

	return res;
}

TEEC_Result ta_crypt_cmd_cipher_update(TEEC_Session *s,
					   TEE_OperationHandle oph,
					   const void *src, size_t src_len,
					   void *dst, size_t *dst_len)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	assert((uintptr_t) oph <= UINT32_MAX);
	op.params[0].value.a = (uint32_t) (uintptr_t) oph;
	op.params[1].tmpref.buffer = (void *)src;
	op.params[1].tmpref.size = src_len;
	op.params[2].tmpref.buffer = dst;
	op.params[2].tmpref.size = *dst_len;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE);
	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_CIPHER_UPDATE, &op, &ret_orig);
	if (res == TEEC_SUCCESS)
		*dst_len = op.params[2].tmpref.size;

	return res;
}

TEEC_Result ta_crypt_cmd_cipher_final(TEEC_Session *s,
					  TEE_OperationHandle oph,
					  const void *src, size_t src_len,
					  void *dst, size_t *dst_len)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	assert((uintptr_t) oph <= UINT32_MAX);
	op.params[0].value.a = (uint32_t) (uintptr_t) oph;
	op.params[1].tmpref.buffer = (void *)src;
	op.params[1].tmpref.size = src_len;
	op.params[2].tmpref.buffer = (void *)dst;
	op.params[2].tmpref.size = *dst_len;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE);
	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_CIPHER_DO_FINAL,
				&op, &ret_orig);
	if (res == TEEC_SUCCESS)
		*dst_len = op.params[2].tmpref.size;

	return res;
}

TEEC_Result ta_crypt_cmd_digest_update(TEEC_Session *s,
					   TEE_OperationHandle oph,
					   const void *chunk, size_t chunk_size)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	assert((uintptr_t) oph <= UINT32_MAX);
	op.params[0].value.a = (uint32_t) (uintptr_t) oph;
	op.params[1].tmpref.buffer = (void *)chunk;
	op.params[1].tmpref.size = chunk_size;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_NONE, TEEC_NONE);
	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_DIGEST_UPDATE, &op, &ret_orig);

	return res;
}

TEEC_Result ta_crypt_cmd_digest_do_final(TEEC_Session *s,
					 TEE_OperationHandle oph,
					 const void *chunk, size_t chunk_len,
					 void *hash, size_t *hash_len)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	assert((uintptr_t) oph <= UINT32_MAX);
	op.params[0].value.a = (uint32_t) (uintptr_t) oph;
	op.params[1].tmpref.buffer = (void *)chunk;
	op.params[1].tmpref.size = chunk_len;
	op.params[2].tmpref.buffer = (void *)hash;
	op.params[2].tmpref.size = *hash_len;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE);
	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_DIGEST_DO_FINAL,
				&op, &ret_orig);
	if (res == TEEC_SUCCESS)
		*hash_len = op.params[2].tmpref.size;

	return res;
}
