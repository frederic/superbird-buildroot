//------------------------------------------------------------------------------
// File: vdi.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef _VDI_H_
#define _VDI_H_

#ifdef MAKEANDROID
#define LOGCAT
#endif

#define MAX_NUM_INSTANCE                  4
#define MAX_NUM_VPU_CORE                  1
#define MAX_INST_HANDLE_SIZE            (32*1024)
#define WAVE4ENC_WORKBUF_SIZE           (128*1024)
#define WAVE420L_STACK_SIZE             0x00002000
#define WAVE420L_TEMP_BUF_SIZE          0x00A00000
#define WAVE420L_STREAM_BUF_SIZE        0x00B00000
#define WAVE420L_ROI_MAP_SIZE           0x00100000
#define WAVE420L_SEI_USER_SIZE          0x00100000
#define SIZE_COMMON                     (512*1024)
#define MAX_VPU_BUFFER_POOL           (64*MAX_NUM_INSTANCE)

#define COMMAND_TIMEOUT                 20               /* decoding timout in second */
#define VCPU_CLOCK_IN_MHZ               400             /* Vcpu clock in MHz. PLEASE MODIFY THIS VALUE FOR YOUR ENVIRONMENT. */
#define VPU_BUSY_CHECK_TIMEOUT          5000
#define FIO_TIMEOUT                     10000

typedef unsigned int u32;
typedef int s32;
typedef unsigned long ulong;
typedef unsigned short u16;
typedef unsigned char u8;
typedef char s8;

enum {
    NONE = 0,
    DEBUG,
    INFO,
    WARN,
    ERR,
    TRACE,
    MAX_LOG_LEVEL
};

#ifdef LOGCAT
#define VLOG(level, x...) \
    do { \
        if (level == INFO) \
            ALOGI(x); \
        else if (level == DEBUG) \
            ALOGD(x); \
        else if (level == WARN) \
            ALOGW(x); \
        else if (level >= ERR) \
            ALOGE(x); \
    }while(0);
#else
#define VLOG(level, x...) \
    do { \
        if (level >= INFO) { \
            printf(x); \
            printf("\n"); \
        } \
    }while(0);
#endif

typedef enum {
    SW_RESET_SAFETY, /**< It resets VPU in safe way. It waits until pending bus transaction is complete and then perform reset. */
    SW_RESET_FORCE, /**< It forces to reset VPU without waiting pending bus transaction to be complete. It is used for immediate termination such as system off. */
    SW_RESET_ON_BOOT /**< This is the default reset mode that is executed since system booting.  This mode is actually executed in VPU_Init(), so does not have to be used independently. */
} SWResetMode;

typedef struct vpu_bit_firmware_info_t {
    u32 size; /* size of this structure*/
    u32 core_idx;
    u32 reg_base_offset;
    u16 bit_code[512];
} vpu_bit_firmware_info_t;

typedef struct vpudrv_inst_info_t {
    u32 core_idx;
    u32 inst_idx;
    s32 inst_open_count; /* for output only*/
} vpudrv_inst_info_t;

typedef struct vpudrv_intr_info_t {
    u32 timeout;
    s32 intr_reason;
} vpudrv_intr_info_t;

typedef struct vpu_buffer_t {
    u32 size;
    u32 cached;
    ulong phys_addr;
    ulong base;
    ulong virt_addr;
} vpu_buffer_t;

typedef struct vpu_instance_pool_t {
    // Since VDI don't know the size of CodecInst structure, VDI should have the enough space not to overflow.
    u8 codecInstPool[MAX_NUM_INSTANCE][MAX_INST_HANDLE_SIZE];
    vpu_buffer_t vpu_common_buffer;
    u32 vpu_instance_num;
    u8 instance_pool_inited;
} vpu_instance_pool_t;

#define VPUDRV_BUF_LEN  vpu_buffer_t
#define VPUDRV_INST_LEN  vpudrv_inst_info_t

#define VDI_MAGIC  'V'
#define VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY \
  _IOW(VDI_MAGIC, 0, VPUDRV_BUF_LEN)

#define VDI_IOCTL_FREE_PHYSICALMEMORY \
  _IOW(VDI_MAGIC, 1, VPUDRV_BUF_LEN)

#define VDI_IOCTL_WAIT_INTERRUPT \
  _IOW(VDI_MAGIC, 2, vpudrv_intr_info_t)

#define VDI_IOCTL_SET_CLOCK_GATE \
  _IOW(VDI_MAGIC, 3, u32)

#define VDI_IOCTL_RESET \
  _IOW(VDI_MAGIC, 4, u32)

#define VDI_IOCTL_GET_INSTANCE_POOL \
  _IOW(VDI_MAGIC, 5, VPUDRV_BUF_LEN)

#define VDI_IOCTL_GET_COMMON_MEMORY \
  _IOW(VDI_MAGIC, 6, VPUDRV_BUF_LEN)

#define VDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO \
  _IOW(VDI_MAGIC, 8, VPUDRV_BUF_LEN)

#define VDI_IOCTL_OPEN_INSTANCE \
  _IOW(VDI_MAGIC, 9, VPUDRV_INST_LEN)

#define VDI_IOCTL_CLOSE_INSTANCE \
  _IOW(VDI_MAGIC, 10, VPUDRV_INST_LEN)

#define VDI_IOCTL_GET_INSTANCE_NUM \
  _IOW(VDI_MAGIC, 11, VPUDRV_INST_LEN)

#define VDI_IOCTL_GET_REGISTER_INFO \
  _IOW(VDI_MAGIC, 12, VPUDRV_BUF_LEN)

#define VDI_IOCTL_FLUSH_BUFFER \
  _IOW(VDI_MAGIC, 13, VPUDRV_BUF_LEN)

#define VpuWriteReg(CORE, ADDR, DATA)                 vdi_write_register(CORE, ADDR, DATA)
#define VpuReadReg(CORE, ADDR)                        vdi_read_register(CORE, ADDR)
#define VpuWriteMem(CORE, ADDR, DATA, LEN)            vdi_write_memory(CORE, ADDR, DATA, LEN)
#define VpuReadMem(CORE, ADDR, DATA, LEN)             vdi_read_memory(CORE, ADDR, DATA, LEN)

s32 vdi_init(u32 core_idx);
s32 vdi_release(u32 core_idx); //this function may be called only at system off.
vpu_instance_pool_t * vdi_get_instance_pool(u32 core_idx);
s32 vdi_get_common_memory(u32 core_idx, vpu_buffer_t *vb);
s32 vdi_allocate_dma_memory(u32 core_idx, vpu_buffer_t *vb);
s32 vdi_attach_dma_memory(u32 core_idx, vpu_buffer_t *vb);
void vdi_free_dma_memory(u32 core_idx, vpu_buffer_t *vb);
s32 vdi_dettach_dma_memory(u32 core_idx, vpu_buffer_t *vb);
s32 vdi_wait_interrupt(u32 core_idx, u32 timeoutMs);
s32 vdi_wait_vpu_busy(u32 core_idx, u32 timeoutMs, u32 addr_bit_busy_flag);
s32 vdi_wait_bus_busy(u32 core_idx, u32 timeoutMs, u32 gdi_busy_flag);
s32 vdi_hw_reset(u32 core_idx);
s32 vdi_set_clock_gate(u32 core_idx, u32 enable);
s32 vdi_get_clock_gate(u32 core_idx);
s32 vdi_get_instance_num(u32 core_idx);
void vdi_write_register(u32 core_idx, u32 addr, u32 data);
u32 vdi_read_register(u32 core_idx, u32 addr);
void vdi_fio_write_register(u32 core_idx, u32 addr, u32 data);
u32 vdi_fio_read_register(u32 core_idx, u32 addr);
void WriteRegVCE(u32 core_idx, u32 vce_core_idx, u32 vce_addr, u32 udata);
u32 ReadRegVCE(u32 core_idx, u32 vce_core_idx, u32 vce_addr);
s32 vdi_clear_memory(u32 core_idx, u32 addr, u32 len);
s32 vdi_write_memory(u32 core_idx, u32 addr, u8 *data, u32 len);
s32 vdi_read_memory(u32 core_idx, u32 addr, u8 *data, u32 len);
void vdi_set_sdram(u32 core_idx, u32 addr, u32 len, u8 data);
s32 vdi_open_instance(u32 core_idx, u32 inst_idx);
s32 vdi_close_instance(u32 core_idx, u32 inst_idx);
s32 vdi_set_bit_firmware_to_pm(u32 core_idx, const u16 *code);
s32 flush_memory(u32 core_idx, vpu_buffer_t *vb);
#endif //#ifndef _VDI_H_
