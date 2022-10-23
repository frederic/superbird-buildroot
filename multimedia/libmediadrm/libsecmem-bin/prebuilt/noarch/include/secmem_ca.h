/*
 * secmem_ca.h
 *
 *  Created on: Feb 2, 2020
 *      Author: tao
 */

#ifndef _SECMEM_CA_H_
#define _SECMEM_CA_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Common API
 */
unsigned int Secure_GetSecmemSize(void);
unsigned int Secure_GetVersion(void);
unsigned int Secure_NegotiateVersion(unsigned int expected);
unsigned int Secure_GetBufferConfig(uint32_t *count, uint32_t *size);

/**
 * V1 API
 */
unsigned int Secure_AllocSecureMem(unsigned int length,
                            unsigned int tvp_set);
unsigned int Secure_ReleaseResource(void);
unsigned int Secure_GetCsdDataDrmInfo(unsigned int srccsdaddr,
                            unsigned int csd_len,
                            unsigned int* store_csd_phyaddr,
                            unsigned int* store_csd_size,
                            unsigned int overwrite);
unsigned int Secure_GetPadding(unsigned int* pad_addr,
                            unsigned int* pad_size,
                            unsigned int pad_type);
unsigned int Secure_GetVp9HeaderSize(void *src,
                            unsigned int size,
                            unsigned int *header_size);

/**
 * V2 API
 */
unsigned int Secure_V2_SessionCreate(void **sess);
unsigned int Secure_V2_SessionDestroy(void **sess);
unsigned int Secure_V2_Init(void *sess,
                            uint32_t source,
                            uint32_t flags,
                            uint32_t paddr,
                            uint32_t msize);
unsigned int Secure_V2_MemCreate(void *sess,
                            uint32_t *handle);
unsigned int Secure_V2_MemAlloc(void *sess,
                            uint32_t handle,
                            uint32_t size,
                            uint32_t *phyaddr);
unsigned int Secure_V2_MemToPhy(void *sess,
                            uint32_t handle,
                            uint32_t *phyaddr);
unsigned int Secure_V2_MemFill(void *sess,
                            uint32_t handle,
                            uint8_t *buffer,
                            uint32_t offset,
                            uint32_t size);
unsigned int Secure_V2_MemCheck(void *sess,
                            uint32_t handle,
                            uint8_t *buffer,
                            uint32_t len);
unsigned int Secure_V2_MemExport(void *sess,
                            uint32_t handle,
                            int *fd);
unsigned int Secure_V2_FdToHandle(void *sess,
                            int fd);
unsigned int Secure_V2_FdToPaddr(void *sess,
                            int fd);
unsigned int Secure_V2_MemFree(void *sess,
                            uint32_t handle);
unsigned int Secure_V2_MemRelease(void *sess,
                            uint32_t handle);
unsigned int Secure_V2_MemFlush(void *sess);
unsigned int Secure_V2_MemClear(void *sess);
unsigned int Secure_V2_SetCsdData(void*sess,
                            unsigned char *csd,
                            unsigned int csd_len);
unsigned int Secure_V2_GetCsdDataDrmInfo(void *sess,
                            unsigned int srccsdaddr,
                            unsigned int csd_len,
                            unsigned int *store_csd_phyaddr,
                            unsigned int *store_csd_size,
                            unsigned int overwrite);
unsigned int Secure_V2_GetPadding(void *sess,
                            unsigned int* pad_addr,
                            unsigned int *pad_size,
                            unsigned int pad_type);
unsigned int Secure_V2_GetVp9HeaderSize(void *sess,
                            void *src,
                            unsigned int size,
                            unsigned int *header_size,
                            uint32_t *frames);
unsigned int Secure_V2_MergeCsdDataDrmInfo(void *sess,
                            uint32_t *phyaddr,
                            uint32_t *csdlen);
unsigned int Secure_V2_MergeCsdData(void *sess,
                            uint32_t handle,
                            uint32_t *csdlen);
unsigned int Secure_V2_ResourceAlloc(void *sess,
                            uint32_t* phyaddr,
                            uint32_t *size);
unsigned int Secure_V2_ResourceFree(void *sess);


#ifdef __cplusplus
}
#endif


#endif /* _SECMEM_CA_H_ */
