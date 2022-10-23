/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <ion/IONmem.h>
#include <ion/ion.h>
#include <linux/ion.h>
#include "ion_4.12.h"

#ifdef __DEBUG
#define __D(fmt, args...) printf("ionmem debug: " fmt, ## args)
#else
#define __D(fmt, args...)
#endif

#define __E(fmt, args...) printf("ionmem error: " fmt, ## args)

int ion_mem_init(void)
{
    int ion_fd = -1;
    ion_fd = ion_open();

    if (ion_fd < 0) {
        __E("%s failed: '%s'\n", strerror(errno));
        return -1;
    }
    __D("%s, ion_fd=%d\n", __func__, ion_fd);

    return ion_fd;
}

int ion_mem_alloc_fd(int ion_fd, size_t size, IONMEM_AllocParams *params, unsigned int flag, unsigned int alloc_hmask)
{
    int ret = -1;
    int i = 0, num_heaps = 0;
    unsigned int heap_mask = 0;

    if (ion_query_heap_cnt(ion_fd, &num_heaps) >= 0) {
        __D("num_heaps=%d\n", num_heaps);
        struct ion_heap_data *const heaps = (struct ion_heap_data *)malloc(num_heaps * sizeof(struct ion_heap_data));
        if (heaps != NULL && num_heaps) {
            if (ion_query_get_heaps(ion_fd, num_heaps, heaps) >= 0) {
                for (int i = 0; i != num_heaps; ++i) {
                    __D("heaps[%d].type=%d, heap_id=%d\n", i, heaps[i].type, heaps[i].heap_id);
                    if ((1 << heaps[i].type) == alloc_hmask) {
                        heap_mask = 1 << heaps[i].heap_id;
                        __D("%d, m=%x, 1<<heap_id=%x, heap_mask=%x, name=%s, alloc_hmask=%x\n",
                            heaps[i].type, 1<<heaps[i].type, heaps[i].heap_id, heap_mask, heaps[i].name, alloc_hmask);
                        break;
                    }
                }
            }
            free(heaps);
            if (heap_mask)
                ret = ion_alloc_fd(ion_fd, size, 0, heap_mask, flag, &params->mImageFd);
            else
                __E("don't find match heap!!\n");
        } else {
            __E("heaps is NULL or no heaps,num_heaps=%d\n", num_heaps);
        }
    } else {
        __E("query_heap_cnt fail! no ion heaps for alloc!!!\n");
    }
    if (ret < 0) {
        __E("ion_alloc failed, errno=%d\n", errno);
        return -ENOMEM;
    }
    return ret;
}

unsigned long ion_mem_alloc(int ion_fd, size_t size, IONMEM_AllocParams *params, bool cache_flag)
{
    int ret = -1;
    int legacy_ion = 0;
    unsigned flag = 0;

    legacy_ion = ion_is_legacy(ion_fd);

    if (cache_flag) {
        flag = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
    }
    __D("%s, cache=%d, bytes=%d, legacy_ion=%d, flag=0x%x\n",
        __func__, cache_flag, size, legacy_ion, flag);

    if (legacy_ion == 1) {
        if (!cache_flag) {
            ret = ion_alloc(ion_fd, size, 0, ION_HEAP_TYPE_DMA_MASK, flag,
                                &params->mIonHnd);
            if (ret < 0)
                ret = ion_alloc(ion_fd, size, 0, ION_HEAP_CARVEOUT_MASK, flag,
                                &params->mIonHnd);
        }
        if (ret < 0) {
            ret = ion_alloc(ion_fd, size, 0, 1 << ION_HEAP_TYPE_CUSTOM, flag,
                                &params->mIonHnd);
            if (ret < 0) {
                __E("%s failed, errno=%d\n", __func__, errno);
                return -ENOMEM;
            }
        }

        ret = ion_share(ion_fd, params->mIonHnd, &params->mImageFd);
        if (ret < 0) {
            __E("ion_share failed, errno=%d\n", errno);
            ion_free(ion_fd, params->mIonHnd);
            return -EINVAL;
        }
        ion_free(ion_fd, params->mIonHnd);
    } else {
        ret = ion_mem_alloc_fd(ion_fd, size, params, flag,
                                    ION_HEAP_TYPE_DMA_MASK);
        if (ret < 0)
            ret = ion_mem_alloc_fd(ion_fd, size, params, flag,
                                        ION_HEAP_CARVEOUT_MASK);
        if (ret < 0)
                ret = ion_mem_alloc_fd(ion_fd, size, params, flag,
                                            ION_HEAP_TYPE_CUSTOM);
        if (ret < 0) {
            __E("%s failed, errno=%d\n", __func__, errno);
            return -ENOMEM;
        }
    }

    __D("%s okay done!, legacy_ion=%d\n", __func__, legacy_ion);
    __D("%s, shared_fd=%d\n", __func__, params->mImageFd);

    return ret;
}

int ion_mem_invalid_cache(int ion_fd, int shared_fd)
{
    int legacy_ion = 0;

    legacy_ion = ion_is_legacy(ion_fd);
    if (!legacy_ion)
        return 0;
    if (ion_fd >= 0 && shared_fd >= 0) {
        if (ion_cache_invalid(ion_fd, shared_fd)) {
            __E("ion_mem_invalid_cache err!\n");
            return -1;
        }
    } else {
        __E("ion_mem_invalid_cache err!\n");
        return -1;
    }
    return 0;
}

void ion_mem_exit(int ion_fd)
{
    int ret = -1;

    __D("exit: %s, ion_fd=%d\n", __func__, ion_fd);

    ret = ion_close(ion_fd);
    if (ret < 0) {
        __E("%s err (%s)! ion_fd=%d\n", strerror(errno), ion_fd);
        return;
    }
}

