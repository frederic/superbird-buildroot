#include <stdio.h>
#include <string.h>
#include <ta_crypt.h>
#include <utee_defines.h>
#include <malloc.h>
#include <assert.h>
#include <tee_client_api.h>
#include <tee_api_types.h>

#define MAX_SHARED_MEM_SIZE (256*1024)

static const uint8_t ciph_data_aes_key1[] = {
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, /* 01234567 */
	0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, /* 89ABCDEF */
};

static const uint8_t ciph_data_128_iv1[] = {
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, /* 12345678 */
	0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x30, /* 9ABCDEF0 */
};

static const uint8_t ciph_data_des3_key1[] = {
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, /* 01234567 */
	0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, /* 89ABCDEF */
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, /* 12345678 */
};

static const uint8_t ciph_data_64_iv1[] = {
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, /* 12345678 */
};

/* Round up the even multiple of size, size has to be a multiple of 2 */
#define ROUNDUP(v, size) (((v) + (size - 1)) & ~(size - 1))

char *_device = NULL;
const TEEC_UUID crypt_user_ta_uuid = TA_CRYPT_UUID;
#define TEEC_OPERATION_INITIALIZER {0}

TEEC_Context xtest_teec_ctx;

TEEC_Result teec_ctx_init(void)
{
	return TEEC_InitializeContext(_device, &xtest_teec_ctx);
}

TEEC_Result teec_open_session(TEEC_Session *session,
				    const TEEC_UUID *uuid, TEEC_Operation *op,
				    uint32_t *ret_orig)
{
	return TEEC_OpenSession(&xtest_teec_ctx, session, uuid,
				TEEC_LOGIN_PUBLIC, NULL, op, ret_orig);
}

void teec_ctx_deinit(void)
{
	TEEC_FinalizeContext(&xtest_teec_ctx);
}

TEEC_Result allocate_operation(TEEC_Session *s,
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

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_VALUE_INPUT,
					 TEEC_NONE, TEEC_NONE);

	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_ALLOCATE_OPERATION, &op,
				 &ret_orig);

	if (res == TEEC_SUCCESS)
		*oph = (TEE_OperationHandle)(uintptr_t)op.params[0].value.a;

	return res;
}

TEEC_Result allocate_transient_object(TEEC_Session *s,
		TEE_ObjectType obj_type,
		uint32_t max_obj_size,
		TEE_ObjectHandle *o)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	op.params[0].value.a = obj_type;
	op.params[0].value.b = max_obj_size;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_OUTPUT,
					 TEEC_NONE, TEEC_NONE);

	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_ALLOCATE_TRANSIENT_OBJECT, &op,
				 &ret_orig);

	if (res == TEEC_SUCCESS)
		*o = (TEE_ObjectHandle)(uintptr_t)op.params[1].value.a;

	return res;
}

struct tee_attr_packed {
	uint32_t attr_id;
	uint32_t a;
	uint32_t b;
};

TEE_Result pack_attrs(const TEE_Attribute *attrs, uint32_t attr_count,
		      uint8_t **buf, size_t *blen)
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
			continue; /* Only memrefs need to be updated */

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

	*(uint32_t *)(void *)b = attr_count;
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
		a[n].a = (uint32_t)(uintptr_t)(b - *buf);

		/* Round up to good alignment */
		b += ROUNDUP(attrs[n].content.ref.length, 4);
	}

	return TEE_SUCCESS;
}

TEEC_Result populate_transient_object(TEEC_Session *s,
		TEE_ObjectHandle o,
		const TEE_Attribute *attrs,
		uint32_t attr_count)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;
	uint8_t *buf;
	size_t blen;

	res = pack_attrs(attrs, attr_count, &buf, &blen);
	if (res != TEEC_SUCCESS)
		return res;

	assert((uintptr_t)o <= UINT32_MAX);
	op.params[0].value.a = (uint32_t)(uintptr_t)o;

	op.params[1].tmpref.buffer = buf;
	op.params[1].tmpref.size = blen;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_MEMREF_TEMP_INPUT, TEEC_NONE,
					 TEEC_NONE);

	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_POPULATE_TRANSIENT_OBJECT, &op,
				 &ret_orig);

	free(buf);
	return res;
}

TEE_Result set_operation_key(TEEC_Session *s,
		TEE_OperationHandle oph,
		TEE_ObjectHandle key)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	assert((uintptr_t)oph <= UINT32_MAX);
	op.params[0].value.a = (uint32_t)(uintptr_t)oph;

	assert((uintptr_t)key <= UINT32_MAX);
	op.params[0].value.b = (uint32_t)(uintptr_t)key;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE,
					 TEEC_NONE);

	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_SET_OPERATION_KEY, &op,
				 &ret_orig);

	return res;
}

TEEC_Result free_transient_object(TEEC_Session *s,
		TEE_ObjectHandle o)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	assert((uintptr_t)o <= UINT32_MAX);
	op.params[0].value.a = (uint32_t)(uintptr_t)o;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE,
					 TEEC_NONE);

	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_FREE_TRANSIENT_OBJECT, &op,
				 &ret_orig);

	return res;
}

static TEEC_Result cipher_init(TEEC_Session *s,
		TEE_OperationHandle oph,
		const void *iv, size_t iv_len)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	assert((uintptr_t)oph <= UINT32_MAX);
	op.params[0].value.a = (uint32_t)(uintptr_t)oph;

	if (iv != NULL) {
		op.params[1].tmpref.buffer = (void *)iv;
		op.params[1].tmpref.size = iv_len;

		op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
						 TEEC_MEMREF_TEMP_INPUT,
						 TEEC_NONE, TEEC_NONE);
	} else {
		op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE,
						 TEEC_NONE, TEEC_NONE);
	}

	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_CIPHER_INIT, &op, &ret_orig);

	return res;
}

static TEEC_Result cipher_update(TEEC_Session *s,
		TEE_OperationHandle oph,
		const void *src, size_t src_len,
		void *dst, size_t *dst_len)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	assert((uintptr_t)oph <= UINT32_MAX);
	op.params[0].value.a = (uint32_t)(uintptr_t)oph;

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

static TEEC_Result cipher_final(TEEC_Session *s,
		TEE_OperationHandle oph,
		const void *src,
		size_t src_len,
		void *dst,
		size_t *dst_len)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	assert((uintptr_t)oph <= UINT32_MAX);
	op.params[0].value.a = (uint32_t)(uintptr_t)oph;

	op.params[1].tmpref.buffer = (void *)src;
	op.params[1].tmpref.size = src_len;

	op.params[2].tmpref.buffer = (void *)dst;
	op.params[2].tmpref.size = *dst_len;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE);

	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_CIPHER_DO_FINAL, &op,
				 &ret_orig);

	if (res == TEEC_SUCCESS)
		*dst_len = op.params[2].tmpref.size;

	return res;
}

static TEEC_Result digest_update(TEEC_Session *s,
		TEE_OperationHandle oph,
		const void *chunk,
		size_t chunk_size)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	assert((uintptr_t)oph <= UINT32_MAX);
	op.params[0].value.a = (uint32_t)(uintptr_t)oph;
	op.params[1].tmpref.buffer = (void *)chunk;
	op.params[1].tmpref.size = chunk_size;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_MEMREF_TEMP_INPUT, TEEC_NONE,
					 TEEC_NONE);

	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_DIGEST_UPDATE, &op, &ret_orig);

	return res;
}

static TEEC_Result digest_do_final(TEEC_Session *s,
		TEE_OperationHandle oph,
		const void *chunk,
		size_t chunk_len, void *hash,
		size_t *hash_len)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	assert((uintptr_t)oph <= UINT32_MAX);
	op.params[0].value.a = (uint32_t)(uintptr_t)oph;

	op.params[1].tmpref.buffer = (void *)chunk;
	op.params[1].tmpref.size = chunk_len;

	op.params[2].tmpref.buffer = (void *)hash;
	op.params[2].tmpref.size = *hash_len;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE);

	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_DIGEST_DO_FINAL, &op,
				 &ret_orig);

	if (res == TEEC_SUCCESS)
		*hash_len = op.params[2].tmpref.size;

	return res;
}

TEEC_Result free_operation(TEEC_Session *s,
		TEE_OperationHandle oph)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t ret_orig;

	op.params[0].value.a = (uint32_t)(uintptr_t)oph;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE,
					 TEEC_NONE);

	res = TEEC_InvokeCommand(s, TA_CRYPT_CMD_FREE_OPERATION, &op,
				 &ret_orig);

	return res;
}

/* the buf should be large enough to contain all the padded data */
unsigned int do_pkcs5_padding(unsigned char* buf, unsigned int len, unsigned int block_size)
{
	unsigned int bytes_to_pad = 0; // padding size
	unsigned int i = 0;	

	bytes_to_pad = ((len + block_size)/ block_size) * block_size - len;

	for (i = len; i < len + bytes_to_pad; i++)
		buf[i] = bytes_to_pad;

	return bytes_to_pad;
}

unsigned int detect_pkcs5_padding_len(unsigned char* buf, unsigned int len)
{
	unsigned int bytes_to_pad = 0; 

	for (bytes_to_pad = 1; buf[len - 1 - bytes_to_pad] == buf[len - 1]; bytes_to_pad++);

	return bytes_to_pad;
}

int32_t file_read_data(FILE *fp, uint8_t *buf, uint32_t len, uint32_t *bytes_padded, uint32_t block_size)
{
	uint32_t n = 0;
	uint32_t pad = 0;
	if (!buf)
		return 0;
	n = fread(buf, sizeof(uint8_t), len, fp);
	// Successfully read. Push into cipher
	if (bytes_padded) {
		if (n < len){
			*bytes_padded = do_pkcs5_padding(buf, n, block_size);
		} else {
			*bytes_padded = 0;
		}
		pad = *bytes_padded;
	}
	return (n + pad);
}

void file_write_data(FILE *fp, const uint8_t *buf, uint32_t len)
{
	if (fwrite(buf, sizeof(uint8_t), len, fp) != len)
		printf("Error: output file failed.\n");
}

static void sym_ciphers(uint32_t algo,
		uint32_t mode, uint32_t key_type,
		const uint8_t *key, uint32_t key_len,
		const uint8_t *iv, uint32_t iv_len,
		FILE* fp_in,
		FILE* fp_out)
{
	TEEC_Session session = { 0 };
	TEE_OperationHandle op;
	TEE_ObjectHandle key1_handle = TEE_HANDLE_NULL;
	size_t out_size = MAX_SHARED_MEM_SIZE;
	uint32_t ret_orig;
	TEE_Attribute key_attr;
	size_t key_size;
	uint8_t *plaintext = NULL;
	uint8_t *ciphertext = NULL;
	uint8_t *buf_in = NULL;
	uint8_t *buf_out = NULL;
	uint32_t bytes_padded = 0;
	uint32_t data_len = 0;
	uint32_t block_size = 0;

	if (teec_open_session(
				&session,
				&crypt_user_ta_uuid,
				NULL,
				&ret_orig) != TEEC_SUCCESS)
		return;

	if (key_type == TEE_TYPE_AES)
		block_size = 16;
	else if  (key_type == TEE_TYPE_DES || key_type == TEE_TYPE_DES3)
		block_size = 8;

	plaintext = (unsigned char*)calloc(MAX_SHARED_MEM_SIZE, sizeof(unsigned char));   
	if (plaintext == NULL) {
		printf("Unable to allocate memory for plaintext\n");
		goto out;
	}
	ciphertext = (unsigned char*)calloc(MAX_SHARED_MEM_SIZE, sizeof(unsigned char));
	if (ciphertext == NULL) {
		printf("Unable to allocate memory for ciphertext\n");
		goto out;
	}

	key_attr.attributeID = TEE_ATTR_SECRET_VALUE;
	key_attr.content.ref.buffer = (void *)key;
	key_attr.content.ref.length = key_len;

	key_size = key_attr.content.ref.length * 8;
	if (key_type == TEE_TYPE_DES ||
			key_type == TEE_TYPE_DES3)
		/* Exclude parity in bit size of key */
		key_size -= key_size / 8;

	if (allocate_operation(&session, &op,
				algo, mode,
				key_size) != TEEC_SUCCESS)
		goto out;

	if (allocate_transient_object(&session,
				key_type, key_size,
				&key1_handle) != TEEC_SUCCESS)
		goto out;

	if (populate_transient_object(&session,
				key1_handle, &key_attr, 1) != TEEC_SUCCESS)
		goto out;

	if (set_operation_key(&session, op,
				key1_handle) != TEEC_SUCCESS)
		goto out;

	if (free_transient_object(&session,
				key1_handle) != TEEC_SUCCESS)
		goto out;

	if (cipher_init(&session, op,
				iv,
				iv_len) != TEEC_SUCCESS)
		goto out;

	if (mode == TEE_MODE_ENCRYPT) {
		buf_in = plaintext;
		buf_out = ciphertext;
	} else {
		buf_in = ciphertext;
		buf_out = plaintext;
	}

	if (mode == TEE_MODE_ENCRYPT) {
		while (bytes_padded == 0) {
			data_len = file_read_data(fp_in, buf_in, MAX_SHARED_MEM_SIZE,
					&bytes_padded, block_size);
			if (!bytes_padded) {
				if (cipher_update(&session, op,
							buf_in, data_len, buf_out,
							&out_size) != TEEC_SUCCESS)
					goto out;


			} else {
				if (algo == TEE_ALG_AES_CTR)
					data_len -= bytes_padded;
				if (cipher_final(&session, op,
							buf_in,
							data_len,
							buf_out,
							&out_size) != TEEC_SUCCESS)
					goto out;

			}
			file_write_data(fp_out, buf_out, out_size);
		}
	} else {
		do {
			data_len = file_read_data(fp_in, buf_in, MAX_SHARED_MEM_SIZE, NULL, block_size);
			if (data_len == MAX_SHARED_MEM_SIZE) {
				if (cipher_update(&session, op,
							buf_in, data_len, buf_out,
							&out_size) != TEEC_SUCCESS)
					goto out;
			} else {
				if (cipher_final(&session, op,
							buf_in,
							data_len,
							buf_out,
							&out_size) != TEEC_SUCCESS)
					goto out;
			}
			if (data_len < MAX_SHARED_MEM_SIZE &&
					(algo == TEE_ALG_AES_CBC_NOPAD ||
					 algo == TEE_ALG_DES_CBC_NOPAD ||
					 algo == TEE_ALG_DES3_CBC_NOPAD)) {
				bytes_padded = detect_pkcs5_padding_len(buf_out, out_size);
				out_size -= bytes_padded;
			}
			file_write_data(fp_out, buf_out, out_size);
		} while (data_len == MAX_SHARED_MEM_SIZE);
	}

	if (free_operation(&session, op) != TEEC_SUCCESS)
		goto out;

out:
	TEEC_CloseSession(&session);
	if (plaintext)
		free(plaintext);
	if (ciphertext)
		free(ciphertext);
}

static void digesting(uint32_t algo, FILE* fp_in)
{
	TEEC_Session session = { 0 };
	uint32_t ret_orig;
	size_t n = 0;
	TEE_OperationHandle op;
	uint8_t digest[64];
	size_t out_size = 64;;
	uint32_t i = 0;
	uint8_t *plaintext = NULL;

	if (teec_open_session(&session, &crypt_user_ta_uuid, NULL,
				&ret_orig) != TEEC_SUCCESS)
		return;

	plaintext = (unsigned char*)calloc(MAX_SHARED_MEM_SIZE, sizeof(unsigned char));   
	if (plaintext == NULL) {
		printf("Unable to allocate memory for plaintext\n");
		goto out;
	}

	if (allocate_operation(&session, &op,
				algo,
				TEE_MODE_DIGEST, 0) != TEEC_SUCCESS)
		goto out;

	while(1) {
		n = fread(plaintext, sizeof(uint8_t), MAX_SHARED_MEM_SIZE, fp_in);
		if (n == MAX_SHARED_MEM_SIZE) {
			if (digest_update(&session, op,
						plaintext,
						n) != TEEC_SUCCESS)
				goto out;
		} else {
			if (digest_do_final(&session, op,
						plaintext,
						n, digest,
						&out_size) != TEEC_SUCCESS)
				goto out;
			break;
		}
	};

	if (free_operation(&session, op) != TEEC_SUCCESS)
		goto out;
	if (algo == TEE_ALG_SHA1) {
		printf("SHA1= ");
		for (i = 0; i < 20; i++)
			printf("%x", digest[i]);
	} else if (algo == TEE_ALG_SHA224) {
		printf("SHA224= ");
		for (i = 0; i < 28; i++)
			printf("%x", digest[i]);
	} else if (algo == TEE_ALG_SHA256) {
		printf("SHA256= ");
		for (i = 0; i < 32; i++)
			printf("%x", digest[i]);
	}
	printf("\n");

out:
	TEEC_CloseSession(&session);
	if (plaintext)
		free(plaintext);
}
static void print_usage(){
	printf("Usage: tee_crypto_64 <e/d/sha1/sha224/sha256> <cipher> <input file> <output file>\n");
	printf("e/d/sha1/sha224/sha256:   encrypt or decrypt or sha digest\n");
	printf("cipher:      aes-128/192/256-cbc, aes-128/192/256-ctr, des-ede3-cbc\n");
	printf("input_file:  input file name\n");
}

int main(int argc, char *argv[])
{
	FILE* fp_in = NULL;
	FILE* fp_out = NULL;
	char cipher1_str[16] = "";
	char cipher2_str[16] = "";
	char bits[16] = "";
	uint32_t algo = 0;
	uint32_t key_len = 0;
	TEE_OperationMode mode = TEE_MODE_ENCRYPT;
	uint32_t key_type = TEE_TYPE_AES;

	argv++;
	argc--;

	printf("crypto client!\n");
	if (argc >= 1) {
		if (!strcmp(argv[0], "e")) {
			argv++;
			argc--;
			mode = TEE_MODE_ENCRYPT;
		} else if (!strcmp(argv[0], "d")) {
			argv++;
			argc--;
			mode = TEE_MODE_DECRYPT;
		} else if (!strcmp(argv[0], "sha1")) {
			argv++;
			argc--;
			algo = TEE_ALG_SHA1;
			mode = TEE_MODE_DIGEST;
		} else if (!strcmp(argv[0], "sha224")) {
			argv++;
			argc--;
			algo = TEE_ALG_SHA224;
			mode = TEE_MODE_DIGEST;
		} else if (!strcmp(argv[0], "sha256")) {
			argv++;
			argc--;
			algo = TEE_ALG_SHA256;
			mode = TEE_MODE_DIGEST;
		} else {
			print_usage();
		}
	} else {
		print_usage();
		return -1;
	}

	if (argc < 1) {
		print_usage();
		return -1;
	}
	if (algo == TEE_ALG_SHA1 ||
			algo == TEE_ALG_SHA224 ||
			algo == TEE_ALG_SHA256) {
		if ((fp_in = fopen(argv[0], "rb")) == NULL){
			printf("Error: Open file failed.\n");
			return -1;
		}
	} else {
		sscanf(argv[0], "%[^-]-%[^-]-%s", cipher1_str, bits, cipher2_str);
		argv++;
		argc--;
		if (!strcmp(cipher1_str, "aes")) {
			algo = TEE_ALG_AES_ECB_NOPAD;
			key_type = TEE_TYPE_AES;
		} else if (!strcmp(cipher1_str, "des")) {
			algo = TEE_ALG_DES3_ECB_NOPAD;
			key_type = TEE_TYPE_DES3;
		}

		if (!strcmp(cipher2_str, "cbc")) {
			algo |= 0x100;
		}
		else if (!strcmp(cipher2_str, "ctr")){
			algo |= 0x200;
		}

		if (!strcmp(bits, "128")) {
			key_len = 16;
		} else if (!strcmp(bits, "192")) {
			key_len = 24;
		} else if (!strcmp(bits, "256")) {
			key_len = 32;
		} else if (!strcmp(bits, "ede3")) {
			key_len = 24;
		}

		if (argc < 1) {
			print_usage();
			return -1;
		}

		if ((fp_in = fopen(argv[0], "rb")) == NULL){
			printf("Error: Open file failed.\n");
			return -1;
		}
		argv++;
		argc--;
		if (argc < 1) {
			print_usage();
			return -1;
		}
		if ((fp_out = fopen(argv[0], "wb")) == NULL){
			printf("Error: Open output file failed.\n");
			return -1;
		}
	}
	teec_ctx_init();

	if (algo == TEE_ALG_AES_CBC_NOPAD ||
			algo == TEE_ALG_AES_CTR ||
			algo == TEE_ALG_DES_CBC_NOPAD ||
			algo == TEE_ALG_DES3_CBC_NOPAD) {
		uint8_t key[32] = {0};
		uint8_t iv[16] = {0};
		uint32_t iv_len = 0;
		if (algo == TEE_ALG_AES_CBC_NOPAD ||
			algo == TEE_ALG_AES_CTR) {
			memcpy(key, ciph_data_aes_key1, key_len);
			iv_len = 16;
			memcpy(iv, ciph_data_128_iv1, iv_len);
		} else {
			memcpy(key, ciph_data_des3_key1, key_len);
			iv_len = 8;
			memcpy(iv, ciph_data_64_iv1, iv_len);
		}

		sym_ciphers(algo, mode, key_type,
				key, key_len,
				iv, iv_len,
				fp_in,
				fp_out);
	} else {
		digesting(algo, fp_in);
	}

	teec_ctx_deinit();
	if (fp_in) {
		fclose(fp_in);
		fp_in = NULL;
	}
	if (fp_out) {
		fclose(fp_out);
		fp_out = NULL;
	}

	printf("crypto done!\n");
	return 0;
}

