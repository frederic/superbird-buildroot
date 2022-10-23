/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef IONMEM_H
#define IONMEM_H
#include "ion.h"
#if defined (__cplusplus)
extern "C" {
#endif

#define ION_IOC_MESON_PHYS_ADDR 8


struct meson_phys_data{
    int handle;
    unsigned int phys_addr;
    unsigned int size;
};

typedef struct IONMEM_AllocParams {
    ion_user_handle_t   mIonHnd;
    int                 mImageFd;
    size_t size;
    unsigned char *usr_ptr;
} IONMEM_AllocParams;


#define ION_IOC_MAGIC       'I'

#define ION_IOC_CUSTOM      _IOWR(ION_IOC_MAGIC, 6, struct ion_custom_data)

#ifdef __DEBUG
#define __D(fmt, args...) fprintf(stderr, "CMEM Debug: " fmt, ## args)
#else
#define __D(fmt, args...)
#endif

#define __E(fmt, args...) fprintf(stderr, "CMEM Error: " fmt, ## args)


int CMEM_init(void);
unsigned long CMEM_alloc(size_t size, IONMEM_AllocParams *params, bool cache_flag);
/*void* CMEM_getUsrPtr(unsigned long PhyAdr, int size);*/
int CMEM_free(IONMEM_AllocParams *params);
int CMEM_exit(void);
int CMEM_invalid_cache(int shared_fd);

#if defined (__cplusplus)
}
#endif

#endif

