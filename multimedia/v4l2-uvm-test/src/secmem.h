/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _SECMEMC_H_
#define _SECMEMC_H_

#include <stdint.h>

struct secmem {
    uint32_t handle;
    uint32_t phyaddr;
    int fd;
};

int secmem_init();
void secmem_destroy();
struct secmem* secmem_alloc(uint32_t size);
void secmem_free(struct secmem * mem);
int secmem_fill(struct secmem* mem, uint8_t* buf, uint32_t offset, uint32_t size);
#endif
