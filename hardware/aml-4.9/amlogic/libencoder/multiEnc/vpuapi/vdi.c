
/*
* Copyright (c) 2019 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in below
* which is part of this source code package.
*
* Description:
*/

// Copyright (C) 2019 Amlogic, Inc. All rights reserved.
//
// All information contained herein is Amlogic confidential.
//
// This software is provided to you pursuant to Software License
// Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
// used only in accordance with the terms of this agreement.
//
// Redistribution and use in source and binary forms, with or without
// modification is strictly prohibited without prior written permission
// from Amlogic.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>		/* SIGIO */
#include <fcntl.h>		/* fcntl */
#include <pthread.h>
#include <sys/mman.h>		/* mmap */
#include <sys/ioctl.h>		/* fopen/fread */
#include <sys/errno.h>		/* fopen/fread */
#include <sys/types.h>
#include <sys/time.h>

#include "vdi.h"
#include "vdi_osal.h"
#include "enc_regdefine.h"
#include "vpuapi.h"
#include "debug.h"

#define VPU_DEVICE_NAME "/dev/amvenc_multi"

typedef pthread_mutex_t	MUTEX_HANDLE;

#define VDI_SRAM_BASE_ADDR          0x00000000    // if we can know the sram address in SOC directly for vdi layer. it is possible to set in vdi layer without allocation from driver
#define INIT_RETRY      100

#define VDI_SYSTEM_ENDIAN                   VDI_LITTLE_ENDIAN
#define VDI_128BIT_BUS_SYSTEM_ENDIAN        VDI_128BIT_LITTLE_ENDIAN

#define VPU_BIT_REG_SIZE                    (0x4000*MAX_NUM_VPU_CORE)

#define VDI_VP521C_SRAM_SIZE              0x23000     /* 10bit profile : 8Kx8K -> 143360, 4Kx2K -> 71680
                                                         *  8bit profile : 8Kx8K -> 114688, 4Kx2K -> 57344
                                                         * NOTE: Decoder > Encoder
                                                         */

#define VDI_NUM_LOCK_HANDLES                4

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
#define VPU_CORE_BASE_OFFSET 0x4000
#endif
enum
{
    INIT_VDI_STAT_NULL = 0,
    INIT_VDI_STAT_INIT = 1,
    INIT_VDI_STAT_OPENING = 2,
    INIT_VDI_STAT_ALLOC = 3,
    INIT_VDI_STAT_DONE = 4,
};

typedef struct vpu_buffer_t vpudrv_buffer_t;

typedef struct vpudrv_buffer_pool_t
{
    vpudrv_buffer_t vdb;
    int inuse;
} vpudrv_buffer_pool_t;

typedef struct  {
    unsigned long core_idx;
    unsigned int product_code;
    int vpu_fd;
    vpu_instance_pool_t *pvip;
    int task_num;
    int clock_state;
    vpudrv_buffer_t vdb_register;
    vpu_buffer_t vpu_common_memory;
    vpudrv_buffer_pool_t vpu_buffer_pool[MAX_VPU_BUFFER_POOL];
    int vpu_buffer_pool_count;

    void* vpu_mutex;
    void* vpu_omx_mutex;
    void* vpu_disp_mutex;
    void* vmem_mutex;
} vdi_info_t;

static vdi_info_t s_vdi_info[MAX_NUM_VPU_CORE];
static int vdi_init_flag[MAX_NUM_VPU_CORE] = {0};
static pthread_mutex_t vid_mutex = PTHREAD_MUTEX_INITIALIZER;

static int swap_endian(u32 core_idx, unsigned char *data, int len, int endian);

static void restore_mutex_in_dead(MUTEX_HANDLE *mutex)
{
    int mutex_value;

    if (!mutex)
        return;
#if 0 // defined(ANDROID)
    mutex_value = mutex->value;
#else
    memcpy(&mutex_value, mutex, sizeof(mutex_value));
#endif
    if (mutex_value == (int)0xdead10cc) // destroy by device driver
    {
        pthread_mutexattr_t mutexattr;
        pthread_mutexattr_init(&mutexattr);
        pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(mutex, &mutexattr);
    }
}

static void vmem_lock(vdi_info_t* vdi)
{
#if defined(ANDROID) || !defined(PTHREAD_MUTEX_ROBUST_NP)
#else
    const int MUTEX_TIMEOUT = 0x7fffffff;
#endif

#if defined(ANDROID) || !defined(PTHREAD_MUTEX_ROBUST_NP)
    restore_mutex_in_dead((MUTEX_HANDLE *)vdi->vmem_mutex);
    pthread_mutex_lock((MUTEX_HANDLE*)vdi->vmem_mutex);
#else
    if (pthread_mutex_lock((MUTEX_HANDLE *)vdi->vmem_mutex) != 0) {
        VLOG(ERR, "%s:%d failed to pthread_mutex_locK\n", __FUNCTION__, __LINE__);
    }
#endif
    return;
}

static void vmem_unlock(vdi_info_t* vdi)
{
    pthread_mutex_unlock((MUTEX_HANDLE *)vdi->vmem_mutex);
}


int vdi_probe(u32 core_idx)
{
    int ret;

    ret = vdi_init(core_idx);
    vdi_release(core_idx);
    return ret;
}

int vdi_init(u32 core_idx)
{
    vdi_info_t *vdi;
    int i, retry_cnt = 0;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return 0;
    pthread_mutex_lock(&vid_mutex);
    if (vdi_init_flag[core_idx] == INIT_VDI_STAT_NULL) {
        vdi_init_flag[core_idx] = INIT_VDI_STAT_INIT;
        memset(&s_vdi_info[core_idx], 0x00, sizeof(s_vdi_info));

    }

    vdi = &s_vdi_info[core_idx];

    if (vdi->vpu_fd != -1 && vdi->vpu_fd != 0x00
        && vdi_init_flag[core_idx] == INIT_VDI_STAT_DONE)
    {
        VLOG(ERR, "[VDI] opend already.\n");
        vdi->task_num++;
        pthread_mutex_unlock(&vid_mutex);
        return 0;
    }

    vdi_init_flag[core_idx] = INIT_VDI_STAT_OPENING;
retry:
    vdi->vpu_fd = open(VPU_DEVICE_NAME, O_RDWR);
    if (vdi->vpu_fd < 0) {
        if (retry_cnt >= INIT_RETRY) {
                VLOG(ERR, "[VDI] Can't open vpu driver. [error=%s]\n", strerror(errno));
                vdi_init_flag[core_idx] = INIT_VDI_STAT_DONE;
                pthread_mutex_unlock(&vid_mutex);
                return -1;
        } else {
                VLOG(ERR,"[VDI] Init open vpu driver fail retrying \n");
                retry_cnt ++;
                osal_msleep(100);
                goto retry;
        }
    }

    memset(vdi->vpu_buffer_pool, 0x00, sizeof(vpudrv_buffer_pool_t)*MAX_VPU_BUFFER_POOL);
    vdi_init_flag[core_idx] = INIT_VDI_STAT_ALLOC;
    if (!vdi_get_instance_pool(core_idx))
    {
        VLOG(INFO, "[VDI] fail to create shared info for saving context \n");
        goto ERR_VDI_INIT;
    }

    if (vdi->pvip->instance_pool_inited == FALSE)
    {
        int* pCodecInst;
        pthread_mutexattr_t mutexattr;
        pthread_mutexattr_init(&mutexattr);
        pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
#if defined(ANDROID) || !defined(PTHREAD_MUTEX_ROBUST_NP)
#else
        /* If a process or a thread is terminated abnormally,
        * pthread_mutexattr_setrobust_np(attr, PTHREAD_MUTEX_ROBUST_NP) makes
        * next onwer call pthread_mutex_lock() without deadlock.
        */
        pthread_mutexattr_setrobust_np(&mutexattr, PTHREAD_MUTEX_ROBUST_NP);
#endif
        pthread_mutex_init((MUTEX_HANDLE *)vdi->vpu_mutex, &mutexattr);
        pthread_mutex_init((MUTEX_HANDLE *)vdi->vpu_disp_mutex, &mutexattr);
        pthread_mutex_init((MUTEX_HANDLE *)vdi->vmem_mutex, &mutexattr);

        for ( i = 0; i < MAX_NUM_INSTANCE; i++) {
            pCodecInst = (int *)vdi->pvip->codecInstPool[i];
            pCodecInst[1] = i;	// indicate instIndex of CodecInst
            pCodecInst[0] = 0;	// indicate inUse of CodecInst
        }
        vdi->pvip->instance_pool_inited = TRUE;
    }
    if (ioctl(vdi->vpu_fd, VDI_IOCTL_GET_REGISTER_INFO, &vdi->vdb_register) < 0)
    {
        VLOG(ERR, "[VDI] fail to get host interface register\n");
        goto ERR_VDI_INIT;
    }

    vdi->vdb_register.virt_addr = (unsigned long)mmap(NULL, vdi->vdb_register.size, PROT_READ | PROT_WRITE, MAP_SHARED, vdi->vpu_fd, vdi->vdb_register.phys_addr);

    if ((void *)vdi->vdb_register.virt_addr == MAP_FAILED)
    {
        VLOG(ERR, "[VDI] fail to map vpu registers \n");
        goto ERR_VDI_INIT;
    }
    VLOG(INFO, "[VDI] map vdb_register core_idx=%d, virtaddr=0x%lx, size=%d\n", core_idx, vdi->vdb_register.virt_addr, vdi->vdb_register.size);

    vdi_set_clock_gate(core_idx, 1);

    vdi->product_code = vdi_read_register(core_idx, VPU_PRODUCT_CODE_REGISTER);

    vdi_set_clock_gate(core_idx, 0);

    if (vdi_lock(core_idx) < 0)
    {
        VLOG(ERR, "[VDI] fail to handle lock function\n");
        goto ERR_VDI_INIT;
    }

    if (vdi_allocate_common_memory(core_idx) < 0)
    {
        VLOG(ERR, "[VDI] fail to get vpu common buffer from driver\n");
        goto ERR_VDI_INIT;
    }

    vdi->core_idx = core_idx;
    vdi->task_num++;
    vdi_unlock(core_idx);
    vdi_init_flag[core_idx] = INIT_VDI_STAT_DONE;
    pthread_mutex_unlock(&vid_mutex);
    VLOG(INFO, "[VDI] success to init driver \n");

    return 0;

ERR_VDI_INIT:
    vdi_unlock(core_idx);
    vdi_init_flag[core_idx] = INIT_VDI_STAT_DONE;
    pthread_mutex_unlock(&vid_mutex);
    vdi_release(core_idx);
    return -1;
}

int vdi_set_bit_firmware_to_pm(u32 core_idx, const u16 *code)
{
    int i;
    vpu_bit_firmware_info_t bit_firmware_info;
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return 0;

    vdi = &s_vdi_info[core_idx];

    if (!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return 0;

    bit_firmware_info.size = sizeof(vpu_bit_firmware_info_t);
    bit_firmware_info.core_idx = core_idx;
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    bit_firmware_info.reg_base_offset = (core_idx*VPU_CORE_BASE_OFFSET);
#else
    bit_firmware_info.reg_base_offset = 0;
#endif
    for (i=0; i<512; i++)
        bit_firmware_info.bit_code[i] = code[i];

    if (write(vdi->vpu_fd, &bit_firmware_info, bit_firmware_info.size) < 0)
    {
        VLOG(ERR, "[VDI] fail to vdi_set_bit_firmware core=%d\n", bit_firmware_info.core_idx);
        return -1;
    }
    return 0;
}



#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
int vdi_get_task_num(unsigned long core_idx)
{
	vdi_info_t *vdi;
	vdi = &s_vdi_info[core_idx];

	if (!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
		return -1;

	return vdi->task_num;
}
#endif

int vdi_release(u32 core_idx)
{
    int i;
    vpudrv_buffer_t vdb;
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return 0;

    vdi = &s_vdi_info[core_idx];

    if (!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return 0;

    if (vdi_lock(core_idx) < 0)
    {
        VLOG(ERR, "[VDI] fail to handle lock function\n");
        return -1;
    }

    if (vdi->task_num > 1) // means that the opened instance remains
    {
        vdi->task_num--;
        vdi_unlock(core_idx);
        return 0;
    }



    if (vdi->vdb_register.virt_addr)
        munmap((void *)vdi->vdb_register.virt_addr, vdi->vdb_register.size);

    osal_memset(&vdi->vdb_register, 0x00, sizeof(vpudrv_buffer_t));
    vdb.size = 0;
    // get common memory information to free virtual address
    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_common_memory.phys_addr >= vdi->vpu_buffer_pool[i].vdb.phys_addr &&
            vdi->vpu_common_memory.phys_addr < (vdi->vpu_buffer_pool[i].vdb.phys_addr + vdi->vpu_buffer_pool[i].vdb.size))
        {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            vdb = vdi->vpu_buffer_pool[i].vdb;
            break;
        }
    }

    vdi_unlock(core_idx);

    pthread_mutex_lock(&vid_mutex);
    if (vdb.size > 0)
    {
        munmap((void *)vdb.virt_addr, vdb.size);
        memset(&vdi->vpu_common_memory, 0x00, sizeof(vpu_buffer_t));
    }

    vdi->task_num--;

    if (vdi->vpu_fd != -1 && vdi->vpu_fd != 0x00)
    {
        close(vdi->vpu_fd);
        vdi->vpu_fd = -1;

    }

    memset(vdi, 0x00, sizeof(vdi_info_t));
    pthread_mutex_unlock(&vid_mutex);

    return 0;
}

int vdi_get_common_memory(u32 core_idx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd==0x00)
        return -1;

    osal_memcpy(vb, &vdi->vpu_common_memory, sizeof(vpu_buffer_t));

    return 0;
}

int vdi_allocate_common_memory(u32 core_idx)
{
    vdi_info_t *vdi = &s_vdi_info[core_idx];
    vpudrv_buffer_t vdb;
    int i;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd==0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    vdb.size = SIZE_COMMON*MAX_NUM_VPU_CORE;

    if (ioctl(vdi->vpu_fd, VDI_IOCTL_GET_COMMON_MEMORY, &vdb) < 0)
    {
        VLOG(ERR, "[VDI] fail to vdi_allocate_dma_memory size=%d\n", vdb.size);
        return -1;
    }

    vdb.virt_addr = (unsigned long)mmap(NULL, vdb.size, PROT_READ | PROT_WRITE, MAP_SHARED, vdi->vpu_fd, vdb.phys_addr);
    if ((void *)vdb.virt_addr == MAP_FAILED)
    {
        VLOG(ERR, "[VDI] fail to map common memory phyaddr=0x%lx, size = %d\n", (ulong)vdb.phys_addr, (int)vdb.size);
        return -1;
    }

    VLOG(INFO, "[VDI] vdi_allocate_common_memory, physaddr=0x%lx, virtaddr=0x%lx\n", (ulong)vdb.phys_addr, (ulong)vdb.virt_addr);

    // convert os driver buffer type to vpu buffer type
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    vdi->pvip->vpu_common_buffer.size = SIZE_COMMON;
    vdi->pvip->vpu_common_buffer.phys_addr = (ulong)(vdb.phys_addr + (core_idx*SIZE_COMMON));
    vdi->pvip->vpu_common_buffer.base = (ulong)(vdb.base + (core_idx*SIZE_COMMON));
    vdi->pvip->vpu_common_buffer.virt_addr = (ulong)(vdb.virt_addr + (core_idx*SIZE_COMMON));
#else
    vdi->pvip->vpu_common_buffer.size = SIZE_COMMON;
    vdi->pvip->vpu_common_buffer.phys_addr = (ulong)(vdb.phys_addr);
    vdi->pvip->vpu_common_buffer.base = (ulong)(vdb.base);
    vdi->pvip->vpu_common_buffer.virt_addr = (ulong)(vdb.virt_addr);
#endif
    osal_memcpy(&vdi->vpu_common_memory, &vdi->pvip->vpu_common_buffer, sizeof(vpudrv_buffer_t));

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 0)
        {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool_count++;
            vdi->vpu_buffer_pool[i].inuse = 1;
            break;
        }
    }

    VLOG(INFO, "[VDI] vdi_get_common_memory physaddr=0x%lx, size=%d, virtaddr=0x%lx\n", (ulong)vdi->vpu_common_memory.phys_addr, (int)vdi->vpu_common_memory.size, (ulong)vdi->vpu_common_memory.virt_addr);

    return 0;
}

vpu_instance_pool_t *vdi_get_instance_pool(u32 core_idx)
{
    vdi_info_t *vdi;
    vpudrv_buffer_t vdb;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return NULL;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00 )
        return NULL;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));
    if (!vdi->pvip)
    {
        vdb.size = sizeof(vpu_instance_pool_t) + sizeof(MUTEX_HANDLE)*VDI_NUM_LOCK_HANDLES;
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
        vdb.size *= MAX_NUM_VPU_CORE;
#endif

        if (ioctl(vdi->vpu_fd, VDI_IOCTL_GET_INSTANCE_POOL, &vdb) < 0)
        {
            VLOG(ERR, "[VDI] fail to allocate get instance pool physical space=%d\n", (int)vdb.size);
            return NULL;
        }

        vdb.virt_addr = (ulong)mmap(NULL, vdb.size, PROT_READ | PROT_WRITE, MAP_SHARED, vdi->vpu_fd, 0);

        if ((void *)vdb.virt_addr == MAP_FAILED)
        {
            VLOG(ERR, "[VDI] fail to map instance pool phyaddr=0x%x, size = %d\n", (int)vdb.phys_addr, (int)vdb.size);
            return NULL;
        }

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
        vdi->pvip = (vpu_instance_pool_t *)(vdb.virt_addr + (core_idx*(sizeof(vpu_instance_pool_t) + sizeof(MUTEX_HANDLE)*VDI_NUM_LOCK_HANDLES)));
#else
        vdi->pvip = (vpu_instance_pool_t *)(vdb.virt_addr);
#endif
        vdi->vpu_mutex = (void *)((ulong)vdi->pvip + sizeof(vpu_instance_pool_t));	//change the pointer of vpu_mutex to at end pointer of vpu_instance_pool_t to assign at allocated position.
        vdi->vpu_disp_mutex = (void *)((ulong)vdi->pvip + sizeof(vpu_instance_pool_t) + sizeof(MUTEX_HANDLE));
        vdi->vmem_mutex = (void *)((unsigned long)vdi->pvip + sizeof(vpu_instance_pool_t) + 2*sizeof(MUTEX_HANDLE));

        VLOG(INFO, "[VDI] instance pool physaddr=0x%x, virtaddr=0x%x, base=0x%x, size=%d\n", (int)vdb.phys_addr, (int)vdb.virt_addr, (int)vdb.base, (int)vdb.size);
    }

    return (vpu_instance_pool_t *)vdi->pvip;
}

int vdi_open_instance(u32 core_idx, u32 inst_idx)
{
    vdi_info_t *vdi;
    vpudrv_inst_info_t inst_info;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    inst_info.core_idx = core_idx;
    inst_info.inst_idx = inst_idx;
    if (ioctl(vdi->vpu_fd, VDI_IOCTL_OPEN_INSTANCE, &inst_info) < 0)
    {
        VLOG(ERR, "[VDI] fail to deliver open instance num inst_idx=%d\n", (int)inst_idx);
        return -1;
    }
    vdi->pvip->vpu_instance_num = inst_info.inst_open_count;

    return 0;
}

int vdi_close_instance(u32 core_idx, u32 inst_idx)
{
    vdi_info_t *vdi;
    vpudrv_inst_info_t inst_info = {0, };

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    inst_info.core_idx = core_idx;
    inst_info.inst_idx = inst_idx;
    if (ioctl(vdi->vpu_fd, VDI_IOCTL_CLOSE_INSTANCE, &inst_info) < 0)
    {
        VLOG(ERR, "[VDI] fail to deliver open instance num inst_idx=%d\n", (int)inst_idx);
        return -1;
    }
    vdi->pvip->vpu_instance_num = inst_info.inst_open_count;

    return 0;
}

u32 vdi_get_instance_num(u32 core_idx)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    return vdi->pvip->vpu_instance_num;
}

int vdi_hw_reset(u32 core_idx) // DEVICE_ADDR_SW_RESET
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    return ioctl(vdi->vpu_fd, VDI_IOCTL_RESET, 0);

}

int vdi_lock(u32 core_idx)
{
    vdi_info_t *vdi;
#if defined(ANDROID) || !defined(PTHREAD_MUTEX_ROBUST_NP)
#else
    const int MUTEX_TIMEOUT = 0x7fffffff;
#endif

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;
#if defined(ANDROID) || !defined(PTHREAD_MUTEX_ROBUST_NP)
	restore_mutex_in_dead((MUTEX_HANDLE *)vdi->vpu_mutex);
	pthread_mutex_lock((MUTEX_HANDLE*)vdi->vpu_mutex);
#else
    if (pthread_mutex_lock((MUTEX_HANDLE *)vdi->vpu_mutex) != 0) {
        VLOG(ERR, "%s:%d failed to pthread_mutex_locK\n", __FUNCTION__, __LINE__);
        return -1;
    }
#endif

    return 0;
}

void vdi_unlock(u32 core_idx)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return;

    pthread_mutex_unlock((MUTEX_HANDLE *)vdi->vpu_mutex);//lint !e455
}

int vdi_disp_lock(u32 core_idx)
{
    vdi_info_t *vdi;
#if defined(ANDROID) || !defined(PTHREAD_MUTEX_ROBUST_NP)
#else
    const int MUTEX_TIMEOUT = 5000;  // ms
#endif

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;
#if defined(ANDROID) || !defined(PTHREAD_MUTEX_ROBUST_NP)
    restore_mutex_in_dead((MUTEX_HANDLE *)vdi->vpu_disp_mutex);
    pthread_mutex_lock((MUTEX_HANDLE*)vdi->vpu_disp_mutex);
#else
    if (pthread_mutex_lock((MUTEX_HANDLE *)vdi->vpu_disp_mutex) != 0)
        VLOG(ERR, "%s:%d failed to pthread_mutex_lock\n", __FUNCTION__, __LINE__);
        return -1;
    }

#endif /* ANDROID */

    return 0;//lint !e454
}

void vdi_disp_unlock(u32 core_idx)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return;

    pthread_mutex_unlock((MUTEX_HANDLE *)vdi->vpu_disp_mutex);//lint !e455
}

void vdi_write_register(u32 core_idx, unsigned int addr, unsigned int data)
{
    vdi_info_t *vdi;
    unsigned long *reg_addr;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return;


#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    reg_addr = (unsigned long *)(addr + (unsigned long)vdi->vdb_register.virt_addr + (core_idx*VPU_CORE_BASE_OFFSET));
#else
    reg_addr = (unsigned long *)(addr + (unsigned long)vdi->vdb_register.virt_addr);
#endif
    *(volatile unsigned int *)reg_addr = data;
}

unsigned int vdi_read_register(u32 core_idx, unsigned int addr)
{
    vdi_info_t *vdi;
	unsigned long *reg_addr;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return (unsigned int)-1;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return (unsigned int)-1;


#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    reg_addr = (unsigned long *)(addr + (unsigned long)vdi->vdb_register.virt_addr + (core_idx*VPU_CORE_BASE_OFFSET));
#else
    reg_addr = (unsigned long *)(addr + (unsigned long)vdi->vdb_register.virt_addr);
#endif
    return *(volatile unsigned int *)reg_addr;
}

#define FIO_TIMEOUT         100

unsigned int vdi_fio_read_register(u32 core_idx, unsigned int addr)
{
    unsigned int ctrl;
    unsigned int count = 0;
    unsigned int data  = 0xffffffff;

    ctrl  = (addr&0xffff);
    ctrl |= (0<<16);    /* read operation */
    vdi_write_register(core_idx, VP5_VPU_FIO_CTRL_ADDR, ctrl);
    count = FIO_TIMEOUT;
    while (count--) {
        ctrl = vdi_read_register(core_idx, VP5_VPU_FIO_CTRL_ADDR);
        if (ctrl & 0x80000000) {
            data = vdi_read_register(core_idx, VP5_VPU_FIO_DATA);
            break;
        }
    }

    return data;
}

void vdi_fio_write_register(u32 core_idx, unsigned int addr, unsigned int data)
{
    unsigned int ctrl;
    unsigned int count = 0;

    vdi_write_register(core_idx, VP5_VPU_FIO_DATA, data);
    ctrl  = (addr&0xffff);
    ctrl |= (1<<16);    /* write operation */
    vdi_write_register(core_idx, VP5_VPU_FIO_CTRL_ADDR, ctrl);

    count = FIO_TIMEOUT;
    while (count--) {
        ctrl = vdi_read_register(core_idx, VP5_VPU_FIO_CTRL_ADDR);
        if (ctrl & 0x80000000) {
            break;
        }
    }
}


int vdi_clear_memory(u32 core_idx, PhysicalAddress addr, int len, int endian)
{
    vdi_info_t *vdi;
    vpudrv_buffer_t vdb;
    unsigned long offset;

    int i;
    Uint8*  zero;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    core_idx = 0;
#endif

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;
    if(endian > VDI_ENDIAN_MAX) return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 1)
        {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
        }
    }

    if (!vdb.size) {
        VLOG(ERR, "address 0x%08x is not mapped address!!!\n", (int)addr);
        return -1;
    }

    zero = (Uint8*)osal_malloc(len);
    osal_memset((void*)zero, 0x00, len);

    offset = addr - (unsigned long)vdb.phys_addr;
    osal_memcpy((void *)((unsigned long)vdb.virt_addr+offset), zero, len);

    osal_free(zero);
    vdb.phys_addr = addr;
    vdb.size = len;
    vdi_flush_memory(core_idx, &vdb);
    return len;
}

int vdi_set_memory(u32 core_idx, PhysicalAddress addr, int len, int endian, Uint32 data)
{
    vdi_info_t *vdi;
    vpudrv_buffer_t vdb;
    unsigned long offset;

    int i;
    Uint8*  zero;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    core_idx = 0;
#endif

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;
    if (endian > VDI_ENDIAN_MAX) return -1;

    vdi = &s_vdi_info[core_idx];

    if (!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 1)
        {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
        }
    }

    if (!vdb.size) {
        VLOG(ERR, "address 0x%08x is not mapped address!!!\n", (int)addr);
        return -1;
    }

    zero = (Uint8*)osal_malloc(len);
    osal_memset((void*)zero, data, len);

    offset = addr - (unsigned long)vdb.phys_addr;
    osal_memcpy((void *)((unsigned long)vdb.virt_addr+offset), zero, len);

    osal_free(zero);

    vdb.phys_addr = addr;
    vdb.size = len;
    vdi_flush_memory(core_idx, &vdb);

    return len;
}

int vdi_write_memory(u32 core_idx, PhysicalAddress addr, unsigned char *data, int len, int endian)
{
    vdi_info_t *vdi;
    vpudrv_buffer_t vdb;
    unsigned long offset;
    int i;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    core_idx = 0;
#endif

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;
    if (!data)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 1)
        {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size)) {
                break;
            }
        }
    }

    if (!vdb.size) {
        VLOG(ERR, "address 0x%08x is not mapped address!!!\n", (int)addr);
        return -1;
    }


    offset = addr - (unsigned long)vdb.phys_addr;
    swap_endian(core_idx, data, len, endian);
    osal_memcpy((void *)((unsigned long)vdb.virt_addr+offset), data, len);
    vdb.phys_addr = addr;
    vdb.size = len;
    vdi_flush_memory(core_idx, &vdb);
    return len;
}

int vdi_read_memory(u32 core_idx, PhysicalAddress addr, unsigned char *data, int len, int endian)
{
    vdi_info_t *vdi;
    vpudrv_buffer_t vdb;
    ulong offset;
    int i;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    core_idx = 0;
#endif
    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 1)
        {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
        }
    }

    if (!vdb.size)
        return -1;

    offset = addr - (ulong)vdb.phys_addr;
        VLOG(DEBUG, "mid13 of vdi_read_memory");
    vdb.phys_addr = addr;
    vdb.size = len;
    vdi_invalidate_memory(core_idx, &vdb);
    osal_memcpy(data, (const void *)((ulong)vdb.virt_addr+offset), len);
    swap_endian(core_idx, data, len,  endian);

    return len;
}

int vdi_allocate_dma_memory(u32 core_idx, vpu_buffer_t *vb, int memTypes, int instIndex)
{
    vdi_info_t *vdi;
    int i;
    vpudrv_buffer_t vdb;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    core_idx = 0;
#endif
    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    vdb.size = vb->size;
    vdb.cached = 1;
    if (ioctl(vdi->vpu_fd, VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY, &vdb) < 0)
    {
        VLOG(ERR, "[VDI] fail to vdi_allocate_dma_memory size=%d\n", vb->size);
        return -1;
    }
    vb->phys_addr = (ulong)vdb.phys_addr;
    vb->base = (ulong)vdb.base;

    //map to virtual address
    vdb.virt_addr = (ulong)mmap(NULL, vdb.size, PROT_READ | PROT_WRITE,
        MAP_SHARED, vdi->vpu_fd, vdb.phys_addr);
    if ((void *)vdb.virt_addr == MAP_FAILED)
    {
        memset(vb, 0x00, sizeof(vpu_buffer_t));
        return -1;
    }
    vb->virt_addr = vdb.virt_addr;

    vmem_lock(vdi);
    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 0)
        {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool_count++;
            vdi->vpu_buffer_pool[i].inuse = 1;
            break;
        }
    }
    vmem_unlock(vdi);
    VLOG(INFO, "[VDI] vdi_allocate_dma_memory, physaddr=%lx, virtaddr=%p~%p, size=%d, memType=%d\n",
        vb->phys_addr, (void *)vb->virt_addr, (void *)(vb->virt_addr + vb->size), vb->size, memTypes);
    return 0;
}

unsigned long vdi_get_dma_memory_free_size(u32 coreIdx)
{
    vdi_info_t *vdi;
    int size;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return (unsigned long)-1;
    if (ioctl(vdi->vpu_fd, VDI_IOCTL_GET_FREE_MEM_SIZE, &size) < 0) {
        VLOG(ERR, "[VDI] fail VDI_IOCTL_GET_FREE_MEM_SIZE size=%d\n", size);
        return 0;
    }
    return size;
}

int vdi_attach_dma_memory(u32 core_idx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi;
    int i;
    vpudrv_buffer_t vdb;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    core_idx = 0;
#endif

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    vdb.size = vb->size;
    vdb.phys_addr = vb->phys_addr;
    vdb.base = vb->base;

    vdb.virt_addr = vb->virt_addr;

    vmem_lock(vdi);
    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
        {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool[i].inuse = 1;
            break;
        }
        else
        {
            if (vdi->vpu_buffer_pool[i].inuse == 0)
            {
                vdi->vpu_buffer_pool[i].vdb = vdb;
                vdi->vpu_buffer_pool_count++;
                vdi->vpu_buffer_pool[i].inuse = 1;
                break;
            }
        }
    }
    vmem_unlock(vdi);

    //VLOG(INFO, "[VDI] vdi_attach_dma_memory, physaddr=0x%lx, virtaddr=0x%lx, size=%d, index=%d\n", vb->phys_addr, vb->virt_addr, vb->size, i);

    return 0;
}

int vdi_dettach_dma_memory(u32 core_idx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi;
    int i;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    core_idx = 0;
#endif

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vb || !vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    if (vb->size == 0)
        return -1;

    vmem_lock(vdi);
    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
        {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            break;
        }
    }
    vmem_unlock(vdi);

    return 0;
}

void vdi_free_dma_memory(u32 core_idx, vpu_buffer_t *vb, int memTypes, int instIndex)
{
    vdi_info_t *vdi;
    int i;
    vpudrv_buffer_t vdb;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    core_idx = 0;
#endif

    if (core_idx >= MAX_NUM_VPU_CORE)
        return;

    vdi = &s_vdi_info[core_idx];

    if(!vb || !vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return;

    if (vb->size == 0)
        return ;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    vmem_lock(vdi);
    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
        {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            vdb = vdi->vpu_buffer_pool[i].vdb;
            break;
        }
    }
    vmem_unlock(vdi);

    if (!vdb.size)
    {
        VLOG(ERR, "[VDI] invalid buffer to free address = 0x%lx vb address 0x%lx size %d\n",
                (ulong)vdb.virt_addr, vb->phys_addr, vb->size);
        return ;
    }
    ioctl(vdi->vpu_fd, VDI_IOCTL_FREE_PHYSICALMEMORY, &vdb);

    if (munmap((void *)vdb.virt_addr, vdb.size) != 0)
    {
        VLOG(ERR, "[VDI] fail to vdi_free_dma_memory virtial address = 0x%lx\n", (ulong)vdb.virt_addr);
    }
    osal_memset(vb, 0, sizeof(vpu_buffer_t));
}

int vdi_get_sram_memory(u32 core_idx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi = NULL;
    vpudrv_buffer_t vdb;
    unsigned int sram_size = 0;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vb || !vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    switch (vdi->product_code) {
    case VP521C_CODE:
        sram_size = VDI_VP521C_SRAM_SIZE; break;
    default:
        VLOG(ERR, "[VDI] check product_code(%x)\n", vdi->product_code);
        break;
    }

    if (sram_size > 0)	// if we can know the sram address directly in vdi layer, we use it first for sdram address
    {
        vb->phys_addr = VDI_SRAM_BASE_ADDR+(core_idx*sram_size);    // HOST can set DRAM base addr to VDI_SRAM_BASE_ADDR.
        vb->size = sram_size;

        return 0;
    }

    return 0;
}

int vdi_flush_memory(u32 core_idx, vpu_buffer_t* vb)
{
    vdi_info_t* vdi;
    int i, ret = -1;
    vpu_buffer_t vdb;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if(!vb || !vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;
    memset(&vdb, 0x00, sizeof(vpu_buffer_t));

    for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
        if ((vdi->vpu_buffer_pool[i].vdb.phys_addr <= vb->phys_addr) &&
             (vdi->vpu_buffer_pool[i].vdb.phys_addr + vdi->vpu_buffer_pool[i].vdb.size) >= (vb->phys_addr + vb->size))
        { //validate buffer
            vdb = *vb;
            break;
        }
    }
    if (vdb.size > 0)
        ret = ioctl(vdi->vpu_fd, VDI_IOCTL_FLUSH_BUFFER, (void*)&vdb);
    return ret;
}

int vdi_invalidate_memory(u32 core_idx, vpu_buffer_t *vb)
{
    vdi_info_t* vdi;
    int i, ret = -1;
    vpu_buffer_t vdb;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if(!vb || !vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
         return -1;

    memset(&vdb, 0x00, sizeof(vpu_buffer_t));

    for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr <= vb->phys_addr &&
             (vdi->vpu_buffer_pool[i].vdb.phys_addr + vdi->vpu_buffer_pool[i].vdb.size) >= (vb->phys_addr + vb->size))
        { //validate buffer
            vdb = *vb;
            break;
        }
    }
    if (vdb.size > 0)
        ret = ioctl(vdi->vpu_fd, VDI_IOCTL_CACHE_INV_BUFFER, (void*)&vdb);
    return ret;
}

int vdi_config_dma(u32 core_idx, vpu_dma_buf_info_t *info)
{
    int ret;
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

   VLOG(INFO, "[VDI] CONFIG_DMA plane %d fd(%d, %d, %d)\n",
         info->num_planes, info->fd[0], info->fd[1], info->fd[2]);
    ret = ioctl(vdi->vpu_fd, VDI_IOCTL_CONFIG_DMA, (void*)info);
    VLOG(INFO, "[VDI] DMA physic %d phy(%lx, %lx, %lx)\n",
     info->num_planes, info->phys_addr[0], info->phys_addr[1], info->phys_addr[2]);
    return ret;
}

int vdi_unmap_dma(u32 core_idx, vpu_dma_buf_info_t *info)
{
    int ret;
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    ret = ioctl(vdi->vpu_fd, VDI_IOCTL_UNMAP_DMA, (void*)info);

    return ret;
}


int vdi_set_clock_gate(u32 core_idx, int enable)
{
    vdi_info_t *vdi = NULL;
    int ret;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;
    vdi = &s_vdi_info[core_idx];
    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    vdi->clock_state = enable;

    ret = ioctl(vdi->vpu_fd, VDI_IOCTL_SET_CLOCK_GATE, &enable);

    VLOG(INFO, "[VDI] clock enable %d\n", enable);
    return ret;
}

int vdi_get_clock_gate(u32 core_idx)
{
    vdi_info_t *vdi;
    int ret;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    ret = vdi->clock_state;
    return ret;
}

static int get_pc_addr(Uint32 product_code)
{
    if (PRODUCT_CODE_VP_SERIES(product_code)) {
        return VP5_VCPU_CUR_PC;
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", product_code);
        return -1;
    }
}

int vdi_wait_bus_busy(u32 core_idx, int timeout, unsigned int gdi_busy_flag)
{
    Uint64 elapse, cur;
    Uint32 pc;
    vdi_info_t *vdi;
    vdi = &s_vdi_info[core_idx];

    elapse = osal_gettime();
    pc = get_pc_addr(vdi->product_code);
    while(1)
    {
        if (vdi->product_code == VP521C_CODE) {
            if (vdi_fio_read_register(core_idx, gdi_busy_flag) == 0x3f) break;
        }
        else {
            VLOG(ERR, "Unknown product id : %08x\n", vdi->product_code);
            return -1;
        }

        if (timeout > 0) {
            cur = osal_gettime();

            if ((int)(cur - elapse) > timeout) {
                print_busy_timeout_status(core_idx, vdi->product_code, pc);
                return -1;
            }
        }
    }
    return 0;

}

int vdi_wait_vpu_busy(u32 core_idx, int timeout, unsigned int addr_bit_busy_flag)
{
    Uint64 elapse, cur;
    Uint32 pc;
    Uint32 normalReg = TRUE;
    vdi_info_t *vdi;
    vdi = &s_vdi_info[core_idx];

    elapse = osal_gettime();
    pc = get_pc_addr(vdi->product_code);

    if (PRODUCT_CODE_VP_SERIES(vdi->product_code)) {
        if (addr_bit_busy_flag&0x8000) normalReg = FALSE;
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", vdi->product_code);
        return -1;
    }

    while(1)
    {
        if (normalReg == TRUE) {
            if (vdi_read_register(core_idx, addr_bit_busy_flag) == 0) break;
        }
        else {
            if (vdi_fio_read_register(core_idx, addr_bit_busy_flag) == 0) break;
        }

        if (timeout > 0) {
            cur = osal_gettime();

            if ((int)(cur - elapse) > timeout) {
                print_busy_timeout_status(core_idx, vdi->product_code, pc);
                return -1;
            }
        }
    }
    return 0;

}

int vdi_wait_vcpu_bus_busy(u32 core_idx, int timeout, unsigned int addr_bit_busy_flag)
{
    Uint64 elapse, cur;
    Uint32 pc;
    Uint32 normalReg = TRUE;
    vdi_info_t *vdi;
    vdi = &s_vdi_info[core_idx];

    elapse = osal_gettime();

    pc = get_pc_addr(vdi->product_code);
    if (PRODUCT_CODE_VP_SERIES(vdi->product_code)) {
        if (addr_bit_busy_flag&0x8000) normalReg = FALSE;
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", vdi->product_code);
        return -1;
    }
    while(1)
    {
        if (normalReg == TRUE) {
            if (vdi_read_register(core_idx, addr_bit_busy_flag) == 0x40) break;
        }
        else {
            if (vdi_fio_read_register(core_idx, addr_bit_busy_flag) == 0x40) break;
        }

        if (timeout > 0) {
            cur = osal_gettime();

            if ((int)(cur - elapse) > timeout) {
                print_busy_timeout_status(core_idx, vdi->product_code, pc);
                return -1;
            }
        }
    }
    return 0;
}

int vdi_wait_interrupt(u32 coreIdx, unsigned int instIdx, int timeout)
{
	int intr_reason = 0;
	int ret;
	vdi_info_t *vdi;
	vpudrv_intr_info_t intr_info;

	if (coreIdx >= MAX_NUM_VPU_CORE)
		return -1;

	vdi = &s_vdi_info[coreIdx];

	if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
		return -1;
	intr_info.timeout     = timeout;
	intr_info.intr_reason = 0;
	intr_info.intr_inst_index = instIdx;
	ret = ioctl(vdi->vpu_fd, VDI_IOCTL_WAIT_INTERRUPT, (void*)&intr_info);
	if (ret != 0)
		return -1;
	intr_reason = intr_info.intr_reason;

	return intr_reason;
}

//------------------------------------------------------------------------------
// LOG & ENDIAN functions
//------------------------------------------------------------------------------

int vdi_get_system_endian(u32 core_idx)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    if (PRODUCT_CODE_VP_SERIES(vdi->product_code)) {
        return VDI_128BIT_BUS_SYSTEM_ENDIAN;
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", vdi->product_code);
        return -1;
    }
}

int vdi_convert_endian(u32 core_idx, unsigned int endian)
{
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || !vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    if (PRODUCT_CODE_VP_SERIES(vdi->product_code)) {
        switch (endian) {
        case VDI_LITTLE_ENDIAN:       endian = 0x00; break;
        case VDI_BIG_ENDIAN:          endian = 0x0f; break;
        case VDI_32BIT_LITTLE_ENDIAN: endian = 0x04; break;
        case VDI_32BIT_BIG_ENDIAN:    endian = 0x03; break;
        }
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", vdi->product_code);
        return -1;
    }

    return (endian&0x0f);
}

void byte_swap(unsigned char* data, int len)
{
    Uint8 temp;
    Int32 i;
    if(len & 0x1) len++;
    for (i=0; i<len; i+=2) {
        temp      = data[i];
        data[i]   = data[i+1];
        data[i+1] = temp;
    }
}

void word_swap(unsigned char* data, int len)
{
    Uint16  temp;
    Uint16* ptr = (Uint16*)data;
    Int32   i, size = (len + (int)sizeof(Uint16) -1)/(int)sizeof(Uint16);

    for (i=0; i<size; i+=2) {
        temp      = ptr[i];
        ptr[i]   = ptr[i+1];
        ptr[i+1] = temp;
    }
}

void dword_swap(unsigned char* data, int len)
{
    Uint32  temp;
    Uint32* ptr = (Uint32*)data;
    Int32   i, size = (len + (int)sizeof(Uint32) -1)/(int)sizeof(Uint32);

    for (i=0; i<size; i+=2) {
        temp      = ptr[i];
        ptr[i]   = ptr[i+1];
        ptr[i+1] = temp;
    }
}

void lword_swap(unsigned char* data, int len)
{
    Uint64  temp;
    Uint64* ptr = (Uint64*)data;
    Int32   i, size = (len + (int)sizeof(Uint64) -1)/(int)sizeof(Uint64);

    for (i=0; i<size; i+=2) {
        temp      = ptr[i];
        ptr[i]   = ptr[i+1];
        ptr[i+1] = temp;
    }
}

int swap_endian(u32 core_idx, unsigned char *data, int len, int endian)
{
    vdi_info_t *vdi;
    int changes;
    int sys_endian;
    BOOL byteChange, wordChange, dwordChange, lwordChange;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    if (PRODUCT_CODE_VP_SERIES(vdi->product_code)) {
        sys_endian = VDI_128BIT_BUS_SYSTEM_ENDIAN;
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", vdi->product_code);
        return -1;
    }

    endian     = vdi_convert_endian(core_idx, endian);
    sys_endian = vdi_convert_endian(core_idx, sys_endian);
    if (endian == sys_endian)
        return 0;

    if (PRODUCT_CODE_VP_SERIES(vdi->product_code)) {
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", vdi->product_code);
        return -1;
    }

    changes     = endian ^ sys_endian;
    byteChange  = changes&0x01;
    wordChange  = ((changes&0x02) == 0x02);
    dwordChange = ((changes&0x04) == 0x04);
    lwordChange = ((changes&0x08) == 0x08);

    if (byteChange)  byte_swap(data, len);
    if (wordChange)  word_swap(data, len);
    if (dwordChange) dword_swap(data, len);
    if (lwordChange) lword_swap(data, len);

    return 1;
}
