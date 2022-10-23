/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "secmem.h"
#include "secmem_ca.h"

static void* sec_sess;

int secmem_init()
{
    unsigned int ret;
    ret = Secure_V2_SessionCreate(&sec_sess);
    if (ret) {
        printf("Secure_V2_SessionCreate fail %x\n", ret);
        return -1;
    }
    ret = Secure_V2_Init(sec_sess, 1, 1, 0, 0);
    if (ret) {
        Secure_V2_SessionDestroy(sec_sess);
        printf("Secure_V2_Init fail %x\n", ret);
        return -1;
    }
    return 0;
}

void secmem_destroy()
{
    if (sec_sess)
        Secure_V2_SessionDestroy(&sec_sess);
    sec_sess = NULL;
}

struct secmem* secmem_alloc(uint32_t size)
{
    struct secmem* mem = calloc(1, sizeof(*mem));
    if (mem) {
        unsigned int ret;
        uint32_t maxsize;

        ret = Secure_V2_MemCreate(sec_sess, &mem->handle);
        if (ret) {
            printf("Secure_V2_MemCreate fail %x\n", ret);
            goto error;
        }
        ret = Secure_V2_MemAlloc(sec_sess, mem->handle, size, &mem->phyaddr);
        if (ret) {
            printf("Secure_V2_MemAlloc failed %x\n",ret);
            goto error2;
        }
        ret = Secure_V2_MemExport(sec_sess, mem->handle, &mem->fd, &maxsize);
        if (ret) {
            printf("Secure_V2_MemExport failed %x\n",ret);
            goto error3;
        }
    }
    return mem;
error3:
    Secure_V2_MemFree(sec_sess, mem->handle);
error2:
    Secure_V2_MemRelease(sec_sess, mem->handle);
error:
    free(mem);
    return NULL;
}

void secmem_free(struct secmem * mem)
{
    unsigned int ret;
    close(mem->fd);
    ret = Secure_V2_MemFree(sec_sess, mem->handle);
    if (ret)
        printf("Secure_V2_MemFree fail %x\n", ret);
    ret = Secure_V2_MemRelease(sec_sess, mem->handle);
    if (ret)
        printf("Secure_V2_MemRelease fail %x\n", ret);
    free(mem);
}

int secmem_fill(struct secmem* mem, uint8_t* buf, uint32_t offset, uint32_t size)
{
    unsigned int ret = 0;
    ret = Secure_V2_MemFill(sec_sess, mem->handle, offset, buf, size);
    if (ret) {
        printf("Secure_V2_MemFill fail %x\n", ret);
        return -1;
    }
    return ret;
}
