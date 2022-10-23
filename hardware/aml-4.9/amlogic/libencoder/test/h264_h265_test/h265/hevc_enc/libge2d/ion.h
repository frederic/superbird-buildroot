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
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
typedef int ion_user_handle_t;
enum ion_heap_type {
 ION_HEAP_TYPE_SYSTEM,
 ION_HEAP_TYPE_SYSTEM_CONTIG,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 ION_HEAP_TYPE_CARVEOUT,
 ION_HEAP_TYPE_CHUNK,
 ION_HEAP_TYPE_DMA,
 ION_HEAP_TYPE_CUSTOM,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 ION_NUM_HEAPS = 16,
};
#define ION_HEAP_SYSTEM_MASK (1 << ION_HEAP_TYPE_SYSTEM)
#define ION_HEAP_SYSTEM_CONTIG_MASK (1 << ION_HEAP_TYPE_SYSTEM_CONTIG)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ION_HEAP_CARVEOUT_MASK (1 << ION_HEAP_TYPE_CARVEOUT)
#define ION_HEAP_TYPE_DMA_MASK (1 << ION_HEAP_TYPE_DMA)
#define ION_NUM_HEAP_IDS sizeof(unsigned int) * 8
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
int ion_open();
int ion_close(int fd);
static int ion_ioctl(int fd, int req, void *arg);
int ion_alloc(int fd, size_t len, size_t align, unsigned int heap_mask,
              unsigned int flags, ion_user_handle_t *handle);
int ion_free(int fd, ion_user_handle_t handle);
int ion_map(int fd, ion_user_handle_t handle, size_t length, int prot,
            int flags, off_t offset, unsigned char **ptr, int *map_fd);
int ion_share(int fd, ion_user_handle_t handle, int *share_fd);
int ion_alloc_fd(int fd, size_t len, size_t align, unsigned int heap_mask,
                 unsigned int flags, int *handle_fd);
int ion_import(int fd, int share_fd, ion_user_handle_t *handle);
int ion_sync_fd(int fd, int handle_fd);
int ion_cache_invalid(int fd, int handle_fd);