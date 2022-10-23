//#define LOG_NDEBUG 0
#define LOG_TAG "VDI"
#include "./include/vdi.h"
#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "./include/common_regdefine.h"
#include "./include/wave4_regdefine.h"
#ifdef MAKEANDROID
#include <utils/Log.h>
#endif

#define VPU_DEVICE_NAME "/dev/HevcEnc"

#define VCORE_DBG_ADDR(__vCoreIdx) 0x8000 + (0x1000 * __vCoreIdx) + 0x300
#define VCORE_DBG_DATA(__vCoreIdx) 0x8000 + (0x1000 * __vCoreIdx) + 0x304
#define VCORE_DBG_READY(__vCoreIdx) 0x8000 + (0x1000 * __vCoreIdx) + 0x308

//#define VPU_BIT_REG_SIZE  (0x4000 * MAX_NUM_VPU_CORE)
//#define VDI_WAVE420L_SRAM_SIZE 0x2E000 // 8Kx8X MAIN10 MAX size

typedef struct vpudrv_buffer_pool_t {
    vpu_buffer_t vdb;
    u32 inuse;
} vpudrv_buffer_pool_t;

typedef struct {
    u32 core_idx;
    s32 vpu_fd;
    u8 opened;
    vpu_instance_pool_t *pvip;
    u32 task_num;
    u32 clock_state;
    vpu_buffer_t vdb_register;
    vpu_buffer_t instance_pool;
    vpu_buffer_t vpu_common_memory;
    vpudrv_buffer_pool_t vpu_buffer_pool[MAX_VPU_BUFFER_POOL];
    u32 vpu_buffer_pool_count;
} vdi_info_t;

static vdi_info_t s_vdi_info[MAX_NUM_VPU_CORE];

static s32 allocate_common_memory(u32 core_idx);

s32 vdi_init(u32 core_idx) {
    vdi_info_t *vdi;
    s32 i;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return 0;

    vdi = &s_vdi_info[core_idx];
    if (vdi->opened == 0)
        vdi->vpu_fd = -1;

    if (vdi->vpu_fd >= 0) {
        vdi->task_num++;
        return 0;
    }

    vdi->vpu_fd = open(VPU_DEVICE_NAME, O_RDWR);
    if (vdi->vpu_fd < 0) {
        VLOG(ERR, "[VDI] Can't open vpu driver");
        return -1;
    }

    vdi->opened = 1;
    memset(&vdi->vpu_buffer_pool, 0x00,
        sizeof(vpudrv_buffer_pool_t) * MAX_VPU_BUFFER_POOL);

    if (!vdi_get_instance_pool(core_idx)) {
        VLOG(ERR, "[VDI] fail to create shared info for saving context");
        goto ERR_VDI_INIT;
    }

    if (vdi->pvip->instance_pool_inited == 0) {
        u32* pCodecInst;
        for (i = 0; i < MAX_NUM_INSTANCE; i++) {
            pCodecInst = (u32 *) vdi->pvip->codecInstPool[i];
            pCodecInst[1] = i; // indicate instIndex of CodecInst
            pCodecInst[0] = 0; // indicate inUse of CodecInst
        }
        vdi->pvip->instance_pool_inited = 1;
    }
    vdi->vdb_register.cached = 0;
    if (ioctl(vdi->vpu_fd, VDI_IOCTL_GET_REGISTER_INFO, &vdi->vdb_register) < 0) {
        VLOG(ERR, "[VDI] fail to get host interface register");
        goto ERR_VDI_INIT;
    }

    VLOG(DEBUG, "vdb_register.size:%u vdi->vdb_register.phys_addr:%lu errno:%d",
                                vdi->vdb_register.size, vdi->vdb_register.phys_addr, errno);
    vdi->vdb_register.virt_addr = (ulong) mmap(NULL,
        vdi->vdb_register.size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        vdi->vpu_fd,
        0/*vdi->vdb_register.phys_addr*/);

    if ((void *) vdi->vdb_register.virt_addr == MAP_FAILED) {
        VLOG(ERR, "[VDI] fail to map vpu registers. virt_addr:%lx errno:%d",
                                            vdi->vdb_register.virt_addr, errno);
        goto ERR_VDI_INIT;
    }

    VLOG(DEBUG, "[VDI] map vdb_register core_idx=%d, virtaddr=0x%lx, size=0x%x",
                            core_idx, vdi->vdb_register.virt_addr, vdi->vdb_register.size);

    vdi_set_clock_gate(core_idx, 1);

    if (allocate_common_memory(core_idx) < 0) {
        VLOG(ERR, "[VDI] fail to get vpu common buffer from driver");
        goto ERR_VDI_INIT;
    }

    vdi->core_idx = core_idx;
    vdi->task_num++;
    VLOG(DEBUG, "[VDI] success to init driver");
    return 0;

    ERR_VDI_INIT: vdi_release(core_idx);
    return -1;
}

s32 vdi_set_bit_firmware_to_pm(u32 core_idx, const u16 *code) {
    s32 i;
    vpu_bit_firmware_info_t bit_firmware_info;
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return 0;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return 0;

    bit_firmware_info.size = sizeof(vpu_bit_firmware_info_t);
    bit_firmware_info.core_idx = core_idx;
    bit_firmware_info.reg_base_offset = 0;
    for (i = 0; i < 512; i++)
        bit_firmware_info.bit_code[i] = code[i];

    if (write(vdi->vpu_fd, &bit_firmware_info, bit_firmware_info.size) < 0) {
        VLOG(ERR, "[VDI] fail to vdi_set_bit_firmware core=%d", bit_firmware_info.core_idx);
        return -1;
    }
    return 0;
}

s32 vdi_release(u32 core_idx) {
    s32 i;
    vpu_buffer_t vdb;
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return 0;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return 0;

    if (vdi->task_num > 1) {
        // means that the opened instance remains
        vdi->task_num--;
        return 0;
    }

    if (vdi->vdb_register.virt_addr)
        munmap((void *) vdi->vdb_register.virt_addr, vdi->vdb_register.size);

    memset(&vdi->vdb_register, 0x00, sizeof(vpu_buffer_t));
    vdb.size = 0;
    // get common memory information to free virtual address
    for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
        if (vdi->vpu_common_memory.phys_addr >= vdi->vpu_buffer_pool[i].vdb.phys_addr
           && vdi->vpu_common_memory.phys_addr < (vdi->vpu_buffer_pool[i].vdb.phys_addr + vdi->vpu_buffer_pool[i].vdb.size)) {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            vdb = vdi->vpu_buffer_pool[i].vdb;
            break;
        }
    }

    if (vdb.size > 0) {
        munmap((void *) vdb.virt_addr, vdb.size);
        memset(&vdi->vpu_common_memory, 0x00, sizeof(vpu_buffer_t));
    }

    if (vdi->instance_pool.virt_addr)
        munmap((void *) vdi->instance_pool.virt_addr, vdi->instance_pool.size);

    vdi->task_num--;

    if (vdi->vpu_fd >= 0) {
        close(vdi->vpu_fd);
        vdi->vpu_fd = -1;
    }
    memset(vdi, 0x00, sizeof(vdi_info_t));
    vdi->vpu_fd = -1;
    return 0;
}

s32 vdi_get_common_memory(u32 core_idx, vpu_buffer_t *vb) {
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    memcpy(vb, &vdi->vpu_common_memory, sizeof(vpu_buffer_t));
    return 0;
}

s32 allocate_common_memory(u32 core_idx) {
    vdi_info_t *vdi;
    vpu_buffer_t vdb;
    s32 i;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    memset(&vdb, 0x00, sizeof(vpu_buffer_t));

    vdb.size = SIZE_COMMON * MAX_NUM_VPU_CORE;
    vdb.cached = 0;
    if (ioctl(vdi->vpu_fd, VDI_IOCTL_GET_COMMON_MEMORY, &vdb) < 0) {
        VLOG(ERR, "[VDI] fail to vdi_allocate_dma_memory size=0x%x", vdb.size);
        return -1;
    }

    vdb.virt_addr = (ulong) mmap(NULL,
                                 vdb.size,
                                 PROT_READ | PROT_WRITE,
                                 MAP_SHARED,
                                 vdi->vpu_fd,
                                 vdb.phys_addr);
    if ((void *) vdb.virt_addr == MAP_FAILED) {
        VLOG(ERR, "[VDI] fail to map common memory phyaddr=0x%lx, size = 0x%x", vdb.phys_addr, vdb.size);
        return -1;
    }

    VLOG(DEBUG, "[VDI] allocate_common_memory, physaddr=0x%lx, virtaddr=0x%lx, size=%d", vdb.phys_addr, vdb.virt_addr, vdb.size);

    // convert os driver buffer type to vpu buffer type
    vdi->pvip->vpu_common_buffer.size = SIZE_COMMON;
    vdi->pvip->vpu_common_buffer.cached = vdb.cached;
    vdi->pvip->vpu_common_buffer.phys_addr = (ulong) vdb.phys_addr;
    vdi->pvip->vpu_common_buffer.base = (ulong) vdb.base;
    vdi->pvip->vpu_common_buffer.virt_addr = (ulong) vdb.virt_addr;
    memcpy(&vdi->vpu_common_memory, &vdi->pvip->vpu_common_buffer, sizeof(vpu_buffer_t));

    for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
        if (vdi->vpu_buffer_pool[i].inuse == 0) {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool_count++;
            vdi->vpu_buffer_pool[i].inuse = 1;
            VLOG(DEBUG, "[VDI] allocate_common_memory->update pool %d, physaddr=0x%lx, virtaddr=0x%lx ~ 0x%lx, size=0x%x",
                i,vdi->vpu_common_memory.phys_addr,  vdi->vpu_common_memory.virt_addr, vdi->vpu_common_memory.virt_addr + vdi->vpu_common_memory.size,vdi->vpu_common_memory.size);

            break;
        }
    }

    VLOG(INFO, "[VDI] allocate_common_memory physaddr=0x%lx, virtaddr=0x%lx, size=0x%x",
                 vdi->vpu_common_memory.phys_addr,  vdi->vpu_common_memory.virt_addr, vdi->vpu_common_memory.size);
    return 0;
}

vpu_instance_pool_t *vdi_get_instance_pool(u32 core_idx) {
    vdi_info_t *vdi;
    vpu_buffer_t vdb;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return NULL;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return NULL;

    memset(&vdb, 0x00, sizeof(vpu_buffer_t));
    memset(&vdi->instance_pool, 0x00, sizeof(vpu_buffer_t));
    if (!vdi->pvip) {
        vdb.size = sizeof(vpu_instance_pool_t);
        vdb.cached = 0;
        if (ioctl(vdi->vpu_fd, VDI_IOCTL_GET_INSTANCE_POOL, &vdb) < 0) {
            VLOG(ERR, "[VDI] fail to allocate get instance pool physical space=0x%x", vdb.size);
            return NULL;
        }

        vdb.virt_addr = (ulong) mmap(NULL, vdb.size, PROT_READ | PROT_WRITE, MAP_SHARED, vdi->vpu_fd, 0);
        if ((void *) vdb.virt_addr == MAP_FAILED) {
            VLOG(ERR, "[VDI] fail to map instance pool phyaddr=0x%lx, size = 0x%x", vdb.phys_addr, vdb.size);
            return NULL;
        }
        vdi->instance_pool = vdb;
        vdi->pvip = (vpu_instance_pool_t *) (vdb.virt_addr + (core_idx * sizeof(vpu_instance_pool_t)));
        VLOG(DEBUG, "[VDI] instance pool physaddr=0x%lx, virtaddr=0x%lx, base=0x%lx, size=0x%x",
                                                          vdb.phys_addr, vdb.virt_addr, vdb.base, vdb.size);
    }
    return (vpu_instance_pool_t *) vdi->pvip;
}

s32 vdi_open_instance(u32 core_idx, u32 inst_idx) {
    vdi_info_t *vdi;
    vpudrv_inst_info_t inst_info;
    u32* pCodecInst;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    inst_info.core_idx = core_idx;
    inst_info.inst_idx = inst_idx;
    if (ioctl(vdi->vpu_fd, VDI_IOCTL_OPEN_INSTANCE, &inst_info) < 0) {
        VLOG(ERR, "[VDI] fail to deliver open instance num inst_idx=%d", inst_idx);
        return -1;
    }
    vdi->pvip->vpu_instance_num = inst_info.inst_open_count;
    pCodecInst = (u32 *) vdi->pvip->codecInstPool[inst_idx];
    pCodecInst[0] = 1;
    return 0;
}

s32 vdi_close_instance(u32 core_idx, u32 inst_idx) {
    vdi_info_t *vdi;
    vpudrv_inst_info_t inst_info;
    u32* pCodecInst;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    inst_info.core_idx = core_idx;
    inst_info.inst_idx = inst_idx;
    if (ioctl(vdi->vpu_fd, VDI_IOCTL_CLOSE_INSTANCE, &inst_info) < 0) {
        VLOG(ERR, "[VDI] fail to deliver open instance num inst_idx=%d", inst_idx);
        return -1;
    }
    vdi->pvip->vpu_instance_num = inst_info.inst_open_count;
    pCodecInst = (u32 *) vdi->pvip->codecInstPool[inst_idx];
    pCodecInst[0] = 1;
    return 0;
}

s32 vdi_get_instance_num(u32 core_idx) {
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    return vdi->pvip->vpu_instance_num;
}

s32 vdi_hw_reset(u32 core_idx) {
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    return ioctl(vdi->vpu_fd, VDI_IOCTL_RESET, 0);
}

void vdi_write_register(u32 core_idx, u32 addr, u32 data) {
    vdi_info_t *vdi;
    ulong *reg_addr;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return;

    reg_addr = (ulong *) (addr + (ulong) vdi->vdb_register.virt_addr);
    *(volatile u32 *) reg_addr = data;
}

u32 vdi_read_register(u32 core_idx, u32 addr) {
    vdi_info_t *vdi;
    ulong *reg_addr;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return (u32) -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return (u32) -1;

    reg_addr = (ulong *) (addr + (ulong) vdi->vdb_register.virt_addr);
    return *(volatile u32 *) reg_addr;
}

u32 vdi_fio_read_register(u32 core_idx, u32 addr) {
    u32 ctrl;
    u32 count = 0;
    u32 data = 0xffffffff;

    ctrl = (addr & 0xffff);
    ctrl |= (0 << 16); /* read operation */
    vdi_write_register(core_idx, W4_VPU_FIO_CTRL_ADDR, ctrl);
    count = FIO_TIMEOUT;
    while (count--) {
        ctrl = vdi_read_register(core_idx, W4_VPU_FIO_CTRL_ADDR);
        if (ctrl & 0x80000000) {
            data = vdi_read_register(core_idx, W4_VPU_FIO_DATA);
            break;
        }
    }
    return data;
}

void vdi_fio_write_register(u32 core_idx, u32 addr, u32 data) {
    u32 ctrl;
    vdi_write_register(core_idx, W4_VPU_FIO_DATA, data);
    ctrl = (addr & 0xffff);
    ctrl |= (1 << 16); /* write operation */
    vdi_write_register(core_idx, W4_VPU_FIO_CTRL_ADDR, ctrl);
}

void WriteRegVCE(u32 core_idx, u32 vce_core_idx, u32 vce_addr, u32 udata) {
    s32 vcpu_reg_addr;

    vdi_fio_write_register(core_idx, VCORE_DBG_READY(vce_core_idx), 0);

    vcpu_reg_addr = vce_addr >> 2;
    vdi_fio_write_register(core_idx, VCORE_DBG_DATA(vce_core_idx), udata);
    vdi_fio_write_register(core_idx, VCORE_DBG_ADDR(vce_core_idx), (vcpu_reg_addr) & 0x00007FFF);

    if (vdi_fio_read_register(0, VCORE_DBG_READY(vce_core_idx)) < 0)
        VLOG(ERR, "failed to write VCE register: 0x%04x", vce_addr);
}

u32 ReadRegVCE(u32 core_idx, u32 vce_core_idx, u32 vce_addr) {
    s32 vcpu_reg_addr;
    s32 udata;
    s32 vce_core_base = 0x8000 + 0x1000 * vce_core_idx;

    vdi_fio_write_register(core_idx, VCORE_DBG_READY(vce_core_idx), 0);

    vcpu_reg_addr = vce_addr >> 2;

    vdi_fio_write_register(core_idx, VCORE_DBG_ADDR(vce_core_idx), vcpu_reg_addr + vce_core_base);

    if (vdi_fio_read_register(0, VCORE_DBG_READY(vce_core_idx)) == 1)
        udata = vdi_fio_read_register(0, VCORE_DBG_DATA(vce_core_idx));
    else
        udata = -1;
    return udata;
}

s32 vdi_clear_memory(u32 core_idx, u32 addr, u32 len) {
    vdi_info_t *vdi;
    vpu_buffer_t vdb;
    ulong offset;
    s32 i;
    u8 *zero;
    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    memset(&vdb, 0x00, sizeof(vpu_buffer_t));

    for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
        if (vdi->vpu_buffer_pool[i].inuse == 1) {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
        }
    }
    if (!vdb.size) {
        VLOG(ERR, "address 0x%08x is not mapped address!!!", addr);
        return -1;
    }

    zero = (u8 *) malloc(len);
    if (!zero)
        return -1;

    memset((void *) zero, 0x00, len);
    offset = addr - (ulong) vdb.phys_addr;
    memcpy((void *) ((ulong) vdb.virt_addr + offset), zero, len);
    free(zero);
    return len;
}

void vdi_set_sdram(u32 core_idx, u32 addr, u32 len, u8 data) {
    vdi_info_t *vdi;
    u8 *buf;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return;

    buf = (u8 *) malloc(len);
    if (!buf)
        return;

    memset(buf, data, len);
    vdi_write_memory(core_idx, addr, buf, len);
    free(buf);
}

s32 vdi_write_memory(u32 core_idx, u32 addr, u8 *data, u32 len) {
    vdi_info_t *vdi;
    vpu_buffer_t vdb;
    ulong offset;
    s32 i;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    memset(&vdb, 0x00, sizeof(vpu_buffer_t));

    for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
        if (vdi->vpu_buffer_pool[i].inuse == 1) {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
        }
    }

    if (!vdb.size) {
        VLOG(ERR, "address 0x%08x is not mapped address!!!", addr);
        return -1;
    }
    offset = addr - (ulong) vdb.phys_addr;
    memcpy((void *) ((ulong) vdb.virt_addr + offset), data, len);
    return len;
}

s32 vdi_read_memory(u32 core_idx, u32 addr, u8 *data, u32 len) {
    vdi_info_t *vdi;
    vpu_buffer_t vdb;
    ulong offset;
    s32 i;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    memset(&vdb, 0x00, sizeof(vpu_buffer_t));

    for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
        if (vdi->vpu_buffer_pool[i].inuse == 1) {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
        }
    }

    if (!vdb.size)
        return -1;

    offset = addr - (ulong) vdb.phys_addr;
    memcpy(data, (void *) ((ulong) vdb.virt_addr + offset), len);
    return len;
}

s32 vdi_allocate_dma_memory(u32 core_idx, vpu_buffer_t *vb) {
    vdi_info_t *vdi;
    s32 i;
    vpu_buffer_t vdb;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    memset(&vdb, 0x00, sizeof(vpu_buffer_t));

    vdb.size = vb->size;
    vdb.cached = vb->cached;
    if (ioctl(vdi->vpu_fd, VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY, &vdb) < 0) {
        VLOG(ERR, "[VDI] fail to vdi_allocate_dma_memory size=0x%x", vb->size);
        return -1;
    }

    vb->phys_addr = (ulong) vdb.phys_addr;
    vb->base = (ulong) vdb.base;

    //map to virtual address
    vdb.virt_addr = (ulong) mmap(NULL, vdb.size, PROT_READ | PROT_WRITE, MAP_SHARED, vdi->vpu_fd, vdb.phys_addr);
    if ((void *) vdb.virt_addr == MAP_FAILED) {
        memset(vb, 0x00, sizeof(vpu_buffer_t));
        return -1;
    }

    vb->virt_addr = vdb.virt_addr;
    for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
        if (vdi->vpu_buffer_pool[i].inuse == 0) {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool_count++;
            vdi->vpu_buffer_pool[i].inuse = 1;
            VLOG(DEBUG, "[VDI] vdi_allocate_dma_memory->update pool %d, physaddr=0x%lx, virtaddr=0x%lx~0x%lx, size=0x%x",
                i,vb->phys_addr, vb->virt_addr, vb->virt_addr + vb->size, vb->size);
            break;
        }
    }
    VLOG(INFO, "[VDI] vdi_allocate_dma_memory, physaddr=0x%lx, virtaddr=0x%lx~0x%lx, size=0x%x",
                                                  vb->phys_addr, vb->virt_addr, vb->virt_addr + vb->size, vb->size);
    return 0;
}

s32 vdi_attach_dma_memory(u32 core_idx, vpu_buffer_t *vb) {
    vdi_info_t *vdi;
    s32 i;
    vpu_buffer_t vdb;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    memset(&vdb, 0x00, sizeof(vpu_buffer_t));

    vdb.size = vb->size;
    vdb.phys_addr = vb->phys_addr;
    vdb.base = vb->base;
    vdb.virt_addr = vb->virt_addr;
    vdb.cached = vb->cached;

    for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr) {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool[i].inuse = 1;
            break;
        }
    }
    if (i == MAX_VPU_BUFFER_POOL) {
        for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
            if (vdi->vpu_buffer_pool[i].inuse == 0) {
                vdi->vpu_buffer_pool[i].vdb = vdb;
                vdi->vpu_buffer_pool_count++;
                vdi->vpu_buffer_pool[i].inuse = 1;
                break;
            }
        }
    }
    VLOG(INFO, "[VDI] vdi_attach_dma_memory, physaddr=0x%lx, virtaddr=0x%lx, size=0x%x, index=%d",
                                                                    vb->phys_addr, vb->virt_addr, vb->size, i);
    return 0;
}

s32 vdi_dettach_dma_memory(u32 core_idx, vpu_buffer_t *vb) {
    vdi_info_t *vdi;
    s32 i;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    if (vb->size == 0)
        return -1;

    for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr) {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            break;
        }
    }
    return 0;
}

void vdi_free_dma_memory(u32 core_idx, vpu_buffer_t *vb) {
    vdi_info_t *vdi;
    s32 i;
    vpu_buffer_t vdb;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return;

    if (vb->size == 0)
        return;

    memset(&vdb, 0x00, sizeof(vpu_buffer_t));

    for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr) {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            vdb = vdi->vpu_buffer_pool[i].vdb;
            break;
        }
    }

    if (!vdb.size) {
        VLOG(ERR, "[VDI] invalid buffer to free address = 0x%lx", vdb.virt_addr);
        return;
    }

    ioctl(vdi->vpu_fd, VDI_IOCTL_FREE_PHYSICALMEMORY, &vdb);

    if (munmap((void *) vdb.virt_addr, vdb.size) != 0) {
        VLOG(ERR, "[VDI] fail to vdi_free_dma_memory virtial address = 0x%lx", vdb.virt_addr);
    }
    memset(vb, 0, sizeof(vpu_buffer_t));
}

s32 vdi_set_clock_gate(u32 core_idx, u32 enable) {
    vdi_info_t *vdi;
    s32 ret;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    vdi->clock_state = enable;
    ret = ioctl(vdi->vpu_fd, VDI_IOCTL_SET_CLOCK_GATE, &enable);
    return ret;
}

s32 vdi_get_clock_gate(u32 core_idx) {
    vdi_info_t *vdi;
    s32 ret;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    ret = vdi->clock_state;
    return ret;
}

s32 vdi_wait_bus_busy(u32 core_idx, u32 timeoutMs, u32 gdi_busy_flag) {
    ulong elapse, cur;
    struct timeval tv;
    vdi_info_t *vdi;

    vdi = &s_vdi_info[core_idx];
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    gettimeofday(&tv, NULL);
    elapse = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    while (1) {
        if (vdi_fio_read_register(core_idx, gdi_busy_flag) == 0x738)
            break;
        gettimeofday(&tv, NULL);
        cur = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        if ((cur - elapse) > timeoutMs) {
            VLOG(ERR, "[VDI] vdi_wait_bus_busy timeout, timeoutMs: %d, PC=0x%x",
                                                     timeoutMs, vdi_read_register(core_idx, W4_VCPU_CUR_PC));
            return -1;
        }
        usleep(5);
    }
    return 0;

}

s32 vdi_wait_vpu_busy(u32 core_idx, u32 timeoutMs, u32 addr_bit_busy_flag) {
    ulong elapse, cur;
    struct timeval tv;
    s32 i;
    u32 normalReg = 1;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    gettimeofday(&tv, NULL);
    elapse = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    if (addr_bit_busy_flag & 0x8000)
        normalReg = 0;

    i = 0;
    while (1) {
        if (normalReg == 1) {
            if (vdi_read_register(core_idx, addr_bit_busy_flag) == 0)
                break;
        } else {
            if (vdi_fio_read_register(core_idx, addr_bit_busy_flag) == 0)
                break;
        }
        i++;
        gettimeofday(&tv, NULL);
        cur = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        if ((cur - elapse) > timeoutMs) {
            for (i = 0; i < 20; i++) {
                VLOG(ERR, "[VDI] vdi_wait_vpu_busy timeout, timeoutMs: %d, PC=0x%x",
						timeoutMs, vdi_read_register(core_idx, W4_VCPU_CUR_PC));
            }
            return -1;
        }
        usleep(5);
    }
    VLOG(DEBUG, "[VDI] vdi_wait_vpu_busy i = %d", i);
    return 0;
}

s32 vdi_wait_interrupt(u32 coreIdx, u32 timeout) {
    s32 intr_reason = 0;
    s32 ret;
    vdi_info_t *vdi;
    vpudrv_intr_info_t intr_info;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    intr_info.timeout = timeout;
    intr_info.intr_reason = 0;
    ret = ioctl(vdi->vpu_fd, VDI_IOCTL_WAIT_INTERRUPT, (void*) &intr_info);
    if (ret != 0)
        return -1;

    intr_reason = intr_info.intr_reason;
    return intr_reason;
}

s32 flush_memory(u32 core_idx, vpu_buffer_t *vb) {
    vdi_info_t *vdi;
    s32 i, ret = -1;
    vpu_buffer_t vdb;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    memset(&vdb, 0x00, sizeof(vpu_buffer_t));

    for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr) {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            break;
        }
    }
    VLOG(DEBUG, "[VDI] flush_memory vdb.size = %d", vdb.size);
    if (vdb.size > 0)
        ret = ioctl(vdi->vpu_fd, VDI_IOCTL_FLUSH_BUFFER, (void*) &vdb);
    return ret;
}
s32 vdi_config_dma(u32 core_idx, vpu_dma_buf_info_t info) {
    s32 ret;
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    ret = ioctl(vdi->vpu_fd, VDI_IOCTL_CONFIG_DMA, (void*)&info);

    return ret;
}
s32 vdi_unmap_dma(u32 core_idx) {
    s32 ret;
    vdi_info_t *vdi;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[core_idx];
    if (!vdi || vdi->vpu_fd < 0 || vdi->opened == 0)
        return -1;

    ret = ioctl(vdi->vpu_fd, VDI_IOCTL_UNMAP_DMA, 0);

    return ret;
}

