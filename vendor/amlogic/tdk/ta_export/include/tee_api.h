/*
 * Copyright (c) 2014, STMicroelectronics International N.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Based on GP TEE Internal API Specification Version 1.1 */
#ifndef TEE_API_H
#define TEE_API_H

#include <stddef.h>
#include <compiler.h>
#include <types_ext.h>
#include <tee_api_defines.h>
#include <tee_api_types.h>
#if defined(CFG_TEE_PANIC_DEBUG)
#include <trace.h>
#endif

/* Property access functions */

TEE_Result TEE_GetPropertyAsString(TEE_PropSetHandle propsetOrEnumerator,
				   const char *name, char *valueBuffer,
				   uint32_t *valueBufferLen);

TEE_Result TEE_GetPropertyAsBool(TEE_PropSetHandle propsetOrEnumerator,
				 const char *name, bool *value);

TEE_Result TEE_GetPropertyAsU32(TEE_PropSetHandle propsetOrEnumerator,
				const char *name, uint32_t *value);

TEE_Result TEE_GetPropertyAsBinaryBlock(TEE_PropSetHandle propsetOrEnumerator,
					const char *name, void *valueBuffer,
					uint32_t *valueBufferLen);

TEE_Result TEE_GetPropertyAsUUID(TEE_PropSetHandle propsetOrEnumerator,
				 const char *name, TEE_UUID *value);

TEE_Result TEE_GetPropertyAsIdentity(TEE_PropSetHandle propsetOrEnumerator,
				     const char *name, TEE_Identity *value);

TEE_Result TEE_AllocatePropertyEnumerator(TEE_PropSetHandle *enumerator);

void TEE_FreePropertyEnumerator(TEE_PropSetHandle enumerator);

void TEE_StartPropertyEnumerator(TEE_PropSetHandle enumerator,
				 TEE_PropSetHandle propSet);

void TEE_ResetPropertyEnumerator(TEE_PropSetHandle enumerator);

TEE_Result TEE_GetPropertyName(TEE_PropSetHandle enumerator,
			       void *nameBuffer, uint32_t *nameBufferLen);

TEE_Result TEE_GetNextProperty(TEE_PropSetHandle enumerator);

/* System API - Misc */

void __TEE_Panic(TEE_Result panicCode);
void TEE_Panic(TEE_Result panicCode);
#if defined(CFG_TEE_PANIC_DEBUG)
#define TEE_Panic(c) do { \
		EMSG("Panic 0x%x", (c)); \
		__TEE_Panic(c); \
	} while (0)
#endif

/* System API - Internal Client API */

TEE_Result TEE_OpenTASession(const TEE_UUID *destination,
				uint32_t cancellationRequestTimeout,
				uint32_t paramTypes,
				TEE_Param params[TEE_NUM_PARAMS],
				TEE_TASessionHandle *session,
				uint32_t *returnOrigin);

void TEE_CloseTASession(TEE_TASessionHandle session);

TEE_Result TEE_InvokeTACommand(TEE_TASessionHandle session,
				uint32_t cancellationRequestTimeout,
				uint32_t commandID, uint32_t paramTypes,
				TEE_Param params[TEE_NUM_PARAMS],
				uint32_t *returnOrigin);

/* System API - Cancellations */

bool TEE_GetCancellationFlag(void);

bool TEE_UnmaskCancellation(void);

bool TEE_MaskCancellation(void);

/* System API - Memory Management */

TEE_Result TEE_CheckMemoryAccessRights(uint32_t accessFlags, void *buffer,
				       uint32_t size);

void TEE_SetInstanceData(const void *instanceData);

const void *TEE_GetInstanceData(void);

void *TEE_Malloc(uint32_t size, uint32_t hint);

void *TEE_Realloc(void *buffer, uint32_t newSize);

void TEE_Free(void *buffer);

void *TEE_MemMove(void *dest, const void *src, uint32_t size);

/*
 * Note: TEE_MemCompare() has a constant-time implementation (execution time
 * does not depend on buffer content but only on buffer size). It is the main
 * difference with memcmp().
 */
int32_t TEE_MemCompare(const void *buffer1, const void *buffer2, uint32_t size);

void *TEE_MemFill(void *buff, uint32_t x, uint32_t size);

/* Data and Key Storage API  - Generic Object Functions */

void TEE_GetObjectInfo(TEE_ObjectHandle object, TEE_ObjectInfo *objectInfo);
TEE_Result TEE_GetObjectInfo1(TEE_ObjectHandle object, TEE_ObjectInfo *objectInfo);

void TEE_RestrictObjectUsage(TEE_ObjectHandle object, uint32_t objectUsage);
TEE_Result TEE_RestrictObjectUsage1(TEE_ObjectHandle object, uint32_t objectUsage);

TEE_Result TEE_GetObjectBufferAttribute(TEE_ObjectHandle object,
					uint32_t attributeID, void *buffer,
					uint32_t *size);

TEE_Result TEE_GetObjectValueAttribute(TEE_ObjectHandle object,
				       uint32_t attributeID, uint32_t *a,
				       uint32_t *b);

void TEE_CloseObject(TEE_ObjectHandle object);

/* Data and Key Storage API  - Transient Object Functions */

TEE_Result TEE_AllocateTransientObject(TEE_ObjectType objectType,
				       uint32_t maxKeySize,
				       TEE_ObjectHandle *object);

void TEE_FreeTransientObject(TEE_ObjectHandle object);

void TEE_ResetTransientObject(TEE_ObjectHandle object);

TEE_Result TEE_PopulateTransientObject(TEE_ObjectHandle object,
				       const TEE_Attribute *attrs,
				       uint32_t attrCount);

void TEE_InitRefAttribute(TEE_Attribute *attr, uint32_t attributeID,
			  const void *buffer, uint32_t length);

void TEE_InitValueAttribute(TEE_Attribute *attr, uint32_t attributeID,
			    uint32_t a, uint32_t b);

void TEE_CopyObjectAttributes(TEE_ObjectHandle destObject,
			      TEE_ObjectHandle srcObject);

TEE_Result TEE_CopyObjectAttributes1(TEE_ObjectHandle destObject,
			      TEE_ObjectHandle srcObject);

TEE_Result TEE_GenerateKey(TEE_ObjectHandle object, uint32_t keySize,
			   const TEE_Attribute *params, uint32_t paramCount);

/* Data and Key Storage API  - Persistent Object Functions */

TEE_Result TEE_OpenPersistentObject(uint32_t storageID, const void *objectID,
				    uint32_t objectIDLen, uint32_t flags,
				    TEE_ObjectHandle *object);

TEE_Result TEE_CreatePersistentObject(uint32_t storageID, const void *objectID,
				      uint32_t objectIDLen, uint32_t flags,
				      TEE_ObjectHandle attributes,
				      const void *initialData,
				      uint32_t initialDataLen,
				      TEE_ObjectHandle *object);

void TEE_CloseAndDeletePersistentObject(TEE_ObjectHandle object);

TEE_Result TEE_CloseAndDeletePersistentObject1(TEE_ObjectHandle object);

TEE_Result TEE_RenamePersistentObject(TEE_ObjectHandle object,
				      const void *newObjectID,
				      uint32_t newObjectIDLen);

TEE_Result TEE_AllocatePersistentObjectEnumerator(TEE_ObjectEnumHandle *
						  objectEnumerator);

void TEE_FreePersistentObjectEnumerator(TEE_ObjectEnumHandle objectEnumerator);

void TEE_ResetPersistentObjectEnumerator(TEE_ObjectEnumHandle objectEnumerator);

TEE_Result TEE_StartPersistentObjectEnumerator(TEE_ObjectEnumHandle
					       objectEnumerator,
					       uint32_t storageID);

TEE_Result TEE_GetNextPersistentObject(TEE_ObjectEnumHandle objectEnumerator,
				       TEE_ObjectInfo *objectInfo,
				       void *objectID, uint32_t *objectIDLen);

/* Data and Key Storage API  - Data Stream Access Functions */

TEE_Result TEE_ReadObjectData(TEE_ObjectHandle object, void *buffer,
			      uint32_t size, uint32_t *count);

TEE_Result TEE_WriteObjectData(TEE_ObjectHandle object, const void *buffer,
			       uint32_t size);

TEE_Result TEE_TruncateObjectData(TEE_ObjectHandle object, uint32_t size);

TEE_Result TEE_SeekObjectData(TEE_ObjectHandle object, int32_t offset,
			      TEE_Whence whence);

/* Cryptographic Operations API - Generic Operation Functions */

TEE_Result TEE_AllocateOperation(TEE_OperationHandle *operation,
				 uint32_t algorithm, uint32_t mode,
				 uint32_t maxKeySize);

TEE_Result TEE_FreeOperation(TEE_OperationHandle operation);

void TEE_GetOperationInfo(TEE_OperationHandle operation,
			  TEE_OperationInfo *operationInfo);

TEE_Result TEE_GetOperationInfoMultiple(TEE_OperationHandle operation,
			  TEE_OperationInfoMultiple *operationInfoMultiple,
			  uint32_t *operationSize);

void TEE_ResetOperation(TEE_OperationHandle operation);

TEE_Result TEE_SetOperationKey(TEE_OperationHandle operation,
			       TEE_ObjectHandle key);

TEE_Result TEE_SetOperationKey2(TEE_OperationHandle operation,
				TEE_ObjectHandle key1, TEE_ObjectHandle key2);

void TEE_CopyOperation(TEE_OperationHandle dstOperation,
		       TEE_OperationHandle srcOperation);

/* Cryptographic Operations API - Message Digest Functions */

void TEE_DigestUpdate(TEE_OperationHandle operation,
		      const void *chunk, uint32_t chunkSize);

TEE_Result TEE_DigestDoFinal(TEE_OperationHandle operation, const void *chunk,
			     uint32_t chunkLen, void *hash, uint32_t *hashLen);

/* Cryptographic Operations API - Symmetric Cipher Functions */

void TEE_CipherInit(TEE_OperationHandle operation, const void *IV,
		    uint32_t IVLen);

TEE_Result TEE_CipherUpdate(TEE_OperationHandle operation, const void *srcData,
			    uint32_t srcLen, void *destData, uint32_t *destLen);

TEE_Result TEE_CipherUpdateWithoutBuffering(TEE_OperationHandle operation, void *srcData,
			    uint32_t srcLen, void *destData, uint32_t *destLen);

TEE_Result TEE_CipherDoFinal(TEE_OperationHandle operation,
			     const void *srcData, uint32_t srcLen,
			     void *destData, uint32_t *destLen);

/* Cryptographic Operations API - MAC Functions */

void TEE_MACInit(TEE_OperationHandle operation, const void *IV,
		 uint32_t IVLen);

void TEE_MACUpdate(TEE_OperationHandle operation, const void *chunk,
		   uint32_t chunkSize);

TEE_Result TEE_MACComputeFinal(TEE_OperationHandle operation,
			       const void *message, uint32_t messageLen,
			       void *mac, uint32_t *macLen);

TEE_Result TEE_MACCompareFinal(TEE_OperationHandle operation,
			       const void *message, uint32_t messageLen,
			       const void *mac, uint32_t macLen);

/* Cryptographic Operations API - Authenticated Encryption Functions */

TEE_Result TEE_AEInit(TEE_OperationHandle operation, const void *nonce,
		      uint32_t nonceLen, uint32_t tagLen, uint32_t AADLen,
		      uint32_t payloadLen);

void TEE_AEUpdateAAD(TEE_OperationHandle operation, const void *AADdata,
		     uint32_t AADdataLen);

TEE_Result TEE_AEUpdate(TEE_OperationHandle operation, const void *srcData,
			uint32_t srcLen, void *destData, uint32_t *destLen);

TEE_Result TEE_AEUpdateWithoutBuffering(TEE_OperationHandle operation, void *srcData,
			uint32_t srcLen, void *destData, uint32_t *destLen);

TEE_Result TEE_AEEncryptFinal(TEE_OperationHandle operation,
			      const void *srcData, uint32_t srcLen,
			      void *destData, uint32_t *destLen, void *tag,
			      uint32_t *tagLen);

TEE_Result TEE_AEDecryptFinal(TEE_OperationHandle operation,
			      const void *srcData, uint32_t srcLen,
			      void *destData, uint32_t *destLen, void *tag,
			      uint32_t tagLen);

/* Cryptographic Operations API - Asymmetric Functions */

TEE_Result TEE_AsymmetricEncrypt(TEE_OperationHandle operation,
				 const TEE_Attribute *params,
				 uint32_t paramCount, const void *srcData,
				 uint32_t srcLen, void *destData,
				 uint32_t *destLen);

TEE_Result TEE_AsymmetricDecrypt(TEE_OperationHandle operation,
				 const TEE_Attribute *params,
				 uint32_t paramCount, const void *srcData,
				 uint32_t srcLen, void *destData,
				 uint32_t *destLen);

TEE_Result TEE_AsymmetricSign(TEE_OperationHandle operation,
				    TEE_Attribute *params,
				    uint32_t paramCount, void *message,
				    uint32_t messageLen, void *signature,
				    uint32_t *signatureLen);

TEE_Result TEE_AsymmetricVerify(TEE_OperationHandle operation,
				      TEE_Attribute *params,
				      uint32_t paramCount, void *message,
				      uint32_t messageLen, void *signature,
				      uint32_t signatureLen);

TEE_Result TEE_AsymmetricSignDigest(TEE_OperationHandle operation,
				    const TEE_Attribute *params,
				    uint32_t paramCount, const void *digest,
				    uint32_t digestLen, void *signature,
				    uint32_t *signatureLen);

TEE_Result TEE_AsymmetricVerifyDigest(TEE_OperationHandle operation,
				      const TEE_Attribute *params,
				      uint32_t paramCount, const void *digest,
				      uint32_t digestLen, const void *signature,
				      uint32_t signatureLen);

/* Cryptographic Operations API - Key Derivation Functions */

void TEE_DeriveKey(TEE_OperationHandle operation,
		   const TEE_Attribute *params, uint32_t paramCount,
		   TEE_ObjectHandle derivedKey);

/* Cryptographic Operations API - Random Number Generation Functions */

void TEE_GenerateRandom(void *randomBuffer, uint32_t randomBufferLen);

TEE_Result TEE_ExportKey(TEE_ObjectHandle key, void *publibKeyBuffer,
		uint32_t *publibKeyBufferLen, bool publicKey);

TEE_Result TEE_ImportKey(TEE_ObjectHandle key, void *keyBuffer,
		uint32_t keyBufferLen);

/* Date & Time API */

void TEE_GetSystemTime(TEE_Time *time);

TEE_Result TEE_Wait(uint32_t timeout);

TEE_Result TEE_GetTAPersistentTime(TEE_Time *time);

TEE_Result TEE_SetTAPersistentTime(const TEE_Time *time);

void TEE_GetREETime(TEE_Time *time);

/* TEE Arithmetical API - Memory allocation and size of objects */

uint32_t TEE_BigIntFMMSizeInU32(uint32_t modulusSizeInBits);

uint32_t TEE_BigIntFMMContextSizeInU32(uint32_t modulusSizeInBits);

/* TEE Arithmetical API - Initialization functions */

void TEE_BigIntInit(TEE_BigInt *bigInt, uint32_t len);

void TEE_BigIntInitFMMContext(TEE_BigIntFMMContext *context, uint32_t len,
			      const TEE_BigInt *modulus);

void TEE_BigIntInitFMM(TEE_BigIntFMM *bigIntFMM, uint32_t len);

/* TEE Arithmetical API - Converter functions */

TEE_Result TEE_BigIntConvertFromOctetString(TEE_BigInt *dest,
					    const uint8_t *buffer,
					    uint32_t bufferLen,
					    int32_t sign);

TEE_Result TEE_BigIntConvertToOctetString(uint8_t *buffer, uint32_t *bufferLen,
					  const TEE_BigInt *bigInt);

void TEE_BigIntConvertFromS32(TEE_BigInt *dest, int32_t shortVal);

TEE_Result TEE_BigIntConvertToS32(int32_t *dest, const TEE_BigInt *src);

/* TEE Arithmetical API - Logical operations */

int32_t TEE_BigIntCmp(const TEE_BigInt *op1, const TEE_BigInt *op2);

int32_t TEE_BigIntCmpS32(const TEE_BigInt *op, int32_t shortVal);

void TEE_BigIntShiftRight(TEE_BigInt *dest, const TEE_BigInt *op,
			  size_t bits);

bool TEE_BigIntGetBit(const TEE_BigInt *src, uint32_t bitIndex);

uint32_t TEE_BigIntGetBitCount(const TEE_BigInt *src);

void TEE_BigIntAdd(TEE_BigInt *dest, const TEE_BigInt *op1,
		   const TEE_BigInt *op2);

void TEE_BigIntSub(TEE_BigInt *dest, const TEE_BigInt *op1,
		   const TEE_BigInt *op2);

void TEE_BigIntNeg(TEE_BigInt *dest, const TEE_BigInt *op);

void TEE_BigIntMul(TEE_BigInt *dest, const TEE_BigInt *op1,
		   const TEE_BigInt *op2);

void TEE_BigIntSquare(TEE_BigInt *dest, const TEE_BigInt *op);

void TEE_BigIntDiv(TEE_BigInt *dest_q, TEE_BigInt *dest_r,
		   const TEE_BigInt *op1, const TEE_BigInt *op2);

/* TEE Arithmetical API - Modular arithmetic operations */

void TEE_BigIntMod(TEE_BigInt *dest, const TEE_BigInt *op,
		   const TEE_BigInt *n);

void TEE_BigIntAddMod(TEE_BigInt *dest, const TEE_BigInt *op1,
		      const TEE_BigInt *op2, const TEE_BigInt *n);

void TEE_BigIntSubMod(TEE_BigInt *dest, const TEE_BigInt *op1,
		      const TEE_BigInt *op2, const TEE_BigInt *n);

void TEE_BigIntMulMod(TEE_BigInt *dest, const  TEE_BigInt *op1,
		      const TEE_BigInt *op2, const TEE_BigInt *n);

void TEE_BigIntSquareMod(TEE_BigInt *dest, const TEE_BigInt *op,
			 const TEE_BigInt *n);

void TEE_BigIntInvMod(TEE_BigInt *dest, const TEE_BigInt *op,
		      const TEE_BigInt *n);

/* TEE Arithmetical API - Other arithmetic operations */

bool TEE_BigIntRelativePrime(const TEE_BigInt *op1, const TEE_BigInt *op2);

void TEE_BigIntComputeExtendedGcd(TEE_BigInt *gcd, TEE_BigInt *u,
				  TEE_BigInt *v, const TEE_BigInt *op1,
				  const TEE_BigInt *op2);

int32_t TEE_BigIntIsProbablePrime(const TEE_BigInt *op,
				  uint32_t confidenceLevel);

/* TEE Arithmetical API - Fast modular multiplication operations */

void TEE_BigIntConvertToFMM(TEE_BigIntFMM *dest, const TEE_BigInt *src,
			    const TEE_BigInt *n,
			    const TEE_BigIntFMMContext *context);

void TEE_BigIntConvertFromFMM(TEE_BigInt *dest, const TEE_BigIntFMM *src,
			      const TEE_BigInt *n,
			      const TEE_BigIntFMMContext *context);

void TEE_BigIntFMMConvertToBigInt(TEE_BigInt *dest, const TEE_BigIntFMM *src,
				  const TEE_BigInt *n,
				  const TEE_BigIntFMMContext *context);

void TEE_BigIntComputeFMM(TEE_BigIntFMM *dest, const TEE_BigIntFMM *op1,
			  const TEE_BigIntFMM *op2, const TEE_BigInt *n,
			  const TEE_BigIntFMMContext *context);

typedef struct {
	paddr_t pa;
	size_t size;
} vdec_info_t;

TEE_Result TEE_Vdec_Get_Info(vdec_info_t *info);

TEE_Result TEE_Tvp_Init(vdec_info_t *info, size_t count);

TEE_Result TEE_Tvp_Enter(void);

TEE_Result TEE_Tvp_Exit(void);

TEE_Result TEE_Vdec_Mmap(paddr_t pa, size_t size, vaddr_t *va);

TEE_Result TEE_Vdec_Munmap(paddr_t pa, size_t size);

TEE_Result TEE_Protect_Mem(unsigned int startaddr, unsigned int size, int enable);

TEE_Result TEE_Protect_Mem2(unsigned int startaddr, unsigned int size, int enable);

TEE_Result TEE_Setting_Device(int device_number, int port, int enable);

TEE_Result TEE_Unify_Read(uint8_t *keyname, uint32_t keynamelen,
			uint8_t *keybuf, uint32_t keylen, uint32_t *readlen);

TEE_Result TEE_Efuse_Read_Tee(uint8_t *outbuf, uint32_t offset,
			size_t size);

TEE_Result TEE_Efuse_Read_Ree(uint8_t *outbuf, uint32_t offset,
			size_t size);

TEE_Result TEE_Efuse_Read(uint8_t *outbuf, uint32_t offset,
			size_t size);

TEE_Result TEE_Efuse_Write_Block(uint8_t *inbuf, uint32_t block);

/*
 * Get HDCP authentication state
 *     auth = 1, mode = 1, HDCP 1.4 authenticated
 *     auth = 0, mode = 2, HDCP 2.2 authenticated
 *     auth = 0, mode = 0, HDCP authentication failed
 */
TEE_Result TEE_HDCP_Get_State(uint32_t *mode, uint32_t *auth);
/*
 * Get HDCP Streaming ID
 *     type = 0x00, Type 0 Content Stream
 *     type = 0x01, Type 1 Content Stream
 *     type = 0x02~0xFF, Reserved for future use only
 */
TEE_Result TEE_HDCP_Set_StreamID(uint32_t type);

TEE_Result TEE_HDCP_Get_StreamID(uint32_t *type);

TEE_Result TEE_Tvp_Get_Video_Size(uint32_t *width, uint32_t *height);

TEE_Result TEE_Tvp_Get_Display_Size(uint32_t *width, uint32_t *height);

/*
 * Set Video Layer
 *     video layer: 1: video layer1, 2: video layer2
 *     eanble: 0: Disable Video Layer, 1: Enable Video Layer
 *     flags: not used
 */
TEE_Result TEE_Tvp_Set_Video_Layer(uint32_t video_layer, uint32_t enable, uint32_t flags);
TEE_Result TEE_Tvp_Get_Video_Layer(uint32_t video_layer, uint32_t *enable);

TEE_Result TEE_Video_Load_FW(uint8_t *firmware, uint32_t fw_size,
		uint8_t *info, uint32_t info_size);
/*
 * Load HDCP key
 *     mode = 1, load HDCP1.4 key
 *     mode = 2, load HDCP2.2 key
 */
TEE_Result TEE_HDCP_Load_Key(uint32_t mode, uint8_t *keybuf, uint32_t keylen);

/* keyladder */
TEE_Result TEE_Desc_AllocChannel(int dsc_no, int *fd);

TEE_Result TEE_Desc_FreeChannel(int dsc_no, int fd);

TEE_Result TEE_Desc_Reset(int dsc_no, int all);

TEE_Result TEE_Desc_Set_Pid(int dsc_no, int fd, int pid);

TEE_Result TEE_Desc_Set_Key(int dsc_no, int fd, int parity, uint8_t *key,
		uint32_t key_type);

TEE_Result TEE_KL_Run(void *ra);

TEE_Result TEE_KL_GetResponseToChallenge(void *cra);

TEE_Result TEE_Desc_Exit(void);

TEE_Result TEE_Desc_Init(void);

TEE_Result TEE_Desc_Set_Output(int module, int output);

#endif /* TEE_API_H */
