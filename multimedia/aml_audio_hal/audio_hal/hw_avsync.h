/*
 * Copyright (C) 2018 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef _HW_AVSYNC_H_
#define _HW_AVSYNC_H_

#include <stdint.h>
#include <stdbool.h>

#define HW_SYNC_VERSION_SIZE 4
#define HW_AVSYNC_HEADER_SIZE_V1 16
#define HW_AVSYNC_HEADER_SIZE_V2 20
/*  NTS will send frame size 24576 byte(128ms) */
#define HW_AVSYNC_FRAME_SIZE (8192 * 3)

struct hw_avsync_header {
    uint8_t header[HW_AVSYNC_HEADER_SIZE_V2];
    uint8_t version_num;
    uint32_t frame_size;
    uint64_t pts;
    size_t bytes_read;
    size_t bytes_written;
    size_t header_size;
    bool is_complete;
    int (*extract)(struct hw_avsync_header *);
    int (*construct)(struct hw_avsync_header *);
};

typedef int (*consume_meta_data_t)(void *cookie,
        uint32_t frame_size, int64_t pts, uint64_t offset);
typedef int (*consume_output_data_t)(void *cookie,
        const void *buffer, size_t bytes);

struct hw_avsync_header_extractor {
    struct hw_avsync_header avsync_header;
    int8_t data[HW_AVSYNC_FRAME_SIZE];
    unsigned int data_size_bytes;
    ssize_t (*write)(struct hw_avsync_header_extractor *header_extractor,
            const void *buffer, size_t bytes);
    consume_meta_data_t consume_meta_data;
    consume_output_data_t consume_output_data;
    //int (*consume_meta_data)(void *cookie, uint32_t frame_size, int64_t pts, uint32_t offset);
    //int (*consume_output_data)(void *cookie, const void *buffer, size_t bytes);
    void *cbk_cookie;
    size_t sync_frame_written;
    bool is_reading_avsync_header;
    uint64_t payload_offset;
    //int tsync_fd;
};

void hwsync_header_set_frame_size(struct hw_avsync_header *header, uint32_t fream_size);
void hwsync_header_set_pts(struct hw_avsync_header *header, uint64_t pts);

ssize_t header_extractor_write(struct hw_avsync_header_extractor *header_extractor,
            const void *buffer, size_t bytes);
void hwsync_header_construct(struct hw_avsync_header *header);

struct hw_avsync_header_extractor *
new_hw_avsync_header_extractor(
    consume_meta_data_t consume_meta_data,
    consume_output_data_t consume_output_data,
    //int (*consume_meta_data)(void *cookie, uint32_t frame_size, int64_t pts, uint32_t offset),
    //int (*consume_output_data)(void *cookie, const void *buffer, size_t bytes),
    void *cookie);
void delete_hw_avsync_header_extractor(struct hw_avsync_header_extractor *header_extractor);
void flush_hw_avsync_header_extractor(struct hw_avsync_header_extractor *header_extractor);

#endif /* _HW_AVSYNC_H_ */
