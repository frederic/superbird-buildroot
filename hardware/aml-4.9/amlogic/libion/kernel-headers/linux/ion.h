/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef _UAPI_LINUX_ION_H
#define _UAPI_LINUX_ION_H
#include <linux/ioctl.h>
#include <linux/types.h>
#include <ion/ion.h>
typedef  __kernel_size_t size_t;
#define ION_NUM_HEAP_IDS (sizeof(unsigned int) * 8)
#define ION_FLAG_CACHED 1
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ION_FLAG_CACHED_NEEDS_SYNC 2
struct ion_allocation_data {
 size_t len;
 size_t align;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int heap_id_mask;
 unsigned int flags;
 ion_user_handle_t handle;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct ion_fd_data {
 ion_user_handle_t handle;
 int fd;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct ion_handle_data {
 ion_user_handle_t handle;
};
struct ion_custom_data {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int cmd;
 unsigned long arg;
};
#define ION_IOC_MAGIC 'I'
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ION_IOC_ALLOC _IOWR(ION_IOC_MAGIC, 0,   struct ion_allocation_data)
#define ION_IOC_FREE _IOWR(ION_IOC_MAGIC, 1, struct ion_handle_data)
#define ION_IOC_MAP _IOWR(ION_IOC_MAGIC, 2, struct ion_fd_data)
#define ION_IOC_SHARE _IOWR(ION_IOC_MAGIC, 4, struct ion_fd_data)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ION_IOC_IMPORT _IOWR(ION_IOC_MAGIC, 5, struct ion_fd_data)
#define ION_IOC_SYNC _IOWR(ION_IOC_MAGIC, 7, struct ion_fd_data)
#define ION_IOC_CUSTOM _IOWR(ION_IOC_MAGIC, 6, struct ion_custom_data)
#define ION_IOC_INVALID_CACHE _IOWR(ION_IOC_MAGIC, 9, struct ion_fd_data)
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
