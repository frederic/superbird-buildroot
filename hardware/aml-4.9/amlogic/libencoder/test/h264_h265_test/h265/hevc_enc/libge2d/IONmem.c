/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "ion.h"
#include "IONmem.h"

static int cmem_fd = -2;
static int ref_count = 0;

static int validate_init()
{
    switch (cmem_fd) {
      case -3:
        __E("CMEM_exit() already called, check stderr output for earlier "
            "CMEM failure messages (possibly version mismatch).\n");

        return 0;

      case -2:
        __E("CMEM_init() not called, you must initialize CMEM before "
            "making API calls.\n");

        return 0;

      case -1:
        __E("CMEM file descriptor -1 (failed 'open()'), ensure CMEMK "
            "kernel module cmemk.ko has been installed with 'insmod'");

        return 0;

      default:
        return 1;
    }
}

int CMEM_init(void)
{
    int flags;

    __D("init: entered - ref_count %d, cmem_fd %d\n", ref_count, cmem_fd);

    if (cmem_fd >= 0) {
        ref_count++;
        __D("init: /dev/ion already opened, incremented ref_count %d\n",
            ref_count);
        return 0;
    }

    cmem_fd = ion_open();

    if (cmem_fd < 0) {
        __E("init: Failed to open /dev/ion: '%s'\n", strerror(errno));
        return -1;
    }

    ref_count++;

    __D("init: successfully opened /dev/ion...\n");

    __D("init: exiting, returning success\n");

    return 0;
}


unsigned long CMEM_alloc(size_t size, IONMEM_AllocParams *params, bool cache_flag)
{
    int ret = 0;
    unsigned long PhyAdrr = 0;
    struct meson_phys_data phy_data;
    struct ion_custom_data custom_data;

    if (!validate_init()) {
        return 0;
    }
    unsigned flag = 0;
    if (cache_flag) {
        flag = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
    }
    __D("CMEM_alloc, cache=%d, bytes=%d\n", cache_flag, size);
    ret = ion_alloc(cmem_fd, size, 0, 1<<ION_HEAP_TYPE_CUSTOM, flag, &params->mIonHnd);
    if (ret < 0) {
        ion_close(cmem_fd);
        __E("ion_alloc failed, errno=%d", errno);
        cmem_fd = -1;
        return -ENOMEM;
    }
    ret = ion_share(cmem_fd, params->mIonHnd, &params->mImageFd);
    if (ret < 0) {
        __E("ion_share failed, errno=%d", errno);
        ion_free(cmem_fd, params->mIonHnd);
        ion_close(cmem_fd);
        cmem_fd = -1;
        return -EINVAL;
    }

    #if 0
    phy_data.handle = params->mImageFd;
    phy_data.phys_addr = 0;
    phy_data.size = 0;

    custom_data.cmd = ION_IOC_MESON_PHYS_ADDR;
    custom_data.arg = (unsigned long)&phy_data;

    ret = ioctl(cmem_fd, ION_IOC_CUSTOM, (unsigned long)&custom_data);
    if (ret < 0) {
        __D("ion custom ioctl %x failed with code %d: %s\n",
            ION_IOC_MESON_PHYS_ADDR, ret, strerror(errno));
    } else {
        PhyAdrr = phy_data.phys_addr;
    }
    __D("allocate ion buffer for capture, ret=%d, bytes=%d, PysAdrr=%ld .\n",
        ret, size, PhyAdrr);
    return PhyAdrr;
    #endif
    return ret;
}

#if 0
void* CMEM_getUsrPtr(unsigned long PhyAdr, int size)
{
    void *userp = NULL;
    /* Map the physical address to user space */
    userp = mmap(0,                       // Preferred start address
                 size,                    // Length to be mapped
                 PROT_WRITE | PROT_READ,  // Read and write access
                 MAP_SHARED,              // Shared memory
                 cmem_fd,                 // File descriptor
                 PhyAdr);               // The byte offset from fd

    if (userp == MAP_FAILED) {
        __E("registerAlloc: Failed to mmap buffer at physical address %#lx\n",
            PhyAdr);
        return NULL;
    }
    __D("mmap succeeded, returning virt buffer %p\n", userp);

    return userp;
}
#endif

int CMEM_invalid_cache(int shared_fd)
{
    if (cmem_fd !=-1 && shared_fd != -1) {
        if (ion_cache_invalid(cmem_fd, shared_fd)) {
            __E("CMEM_invalid_cache err!\n");
            return -1;
        }
    } else {
        __E("CMEM_invalid_cache err!\n");
        return -1;
    }
    return 0;
}

int CMEM_free(IONMEM_AllocParams *params)
{
    if (!validate_init()) {
        return -1;
    }
    __D("CMEM_free,mIonHnd=%x free\n", params->mIonHnd);

    ion_free(cmem_fd, params->mIonHnd);

    return 0;
}


int CMEM_exit(void)
{
    int result = 0;

    __D("exit: entered - ref_count %d, cmem_fd %d\n", ref_count, cmem_fd);

    if (!validate_init()) {
        return -1;
    }

    __D("exit: decrementing ref_count\n");

    ref_count--;
    if (ref_count == 0) {
        result = ion_close(cmem_fd);

        __D("exit: ref_count == 0, closed /dev/ion (%s)\n",
            result == -1 ? strerror(errno) : "succeeded");

        /* setting -3 allows to distinguish CMEM exit from CMEM failed */
        cmem_fd = -3;
    }

    __D("exit: exiting, returning %d\n", result);

    return result;
}

