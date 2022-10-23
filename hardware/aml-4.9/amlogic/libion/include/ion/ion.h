/*
 *  ion.h
 *
 * Memory Allocator functions for ion
 *
 *   Copyright 2011 Google, Inc
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef __SYS_CORE_ION_H
#define __SYS_CORE_ION_H

#include <sys/types.h>

__BEGIN_DECLS

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
#define ION_HEAP_CARVEOUT_MASK (1 << ION_HEAP_TYPE_CARVEOUT)
#define ION_HEAP_TYPE_DMA_MASK (1 << ION_HEAP_TYPE_DMA)

int ion_open();
int ion_close(int fd);
int ion_alloc(int fd, size_t len, size_t align, unsigned int heap_mask,
              unsigned int flags, ion_user_handle_t *handle);
int ion_alloc_fd(int fd, size_t len, size_t align, unsigned int heap_mask,
              unsigned int flags, int *handle_fd);
int ion_sync_fd(int fd, int handle_fd);
int ion_free(int fd, ion_user_handle_t handle);
int ion_map(int fd, ion_user_handle_t handle, size_t length, int prot,
            int flags, off_t offset, unsigned char **ptr, int *map_fd);
int ion_share(int fd, ion_user_handle_t handle, int *share_fd);
int ion_import(int fd, int share_fd, ion_user_handle_t *handle);

/**
  * Add 4.12+ kernel ION interfaces here for forward compatibility
  * This should be needed till the pre-4.12+ ION interfaces are backported.
  */
int ion_query_heap_cnt(int fd, int* cnt);
int ion_query_get_heaps(int fd, int cnt, void* buffers);

int ion_is_legacy(int fd);
int ion_cache_invalid(int fd, int handle_fd);
__END_DECLS

#endif /* __SYS_CORE_ION_H */
