/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description:
*/


#define LOG_TAG "audio_hw_avsync"
//#define LOG_NDEBUG 0

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <cutils/log.h>


#include "hw_avsync.h"
#include "audio_hwsync.h"

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

const uint8_t HW_AVSYNC_HEADER_V1[] = {0x55, 0x55, 0x00, 0x01};
const uint8_t HW_AVSYNC_HEADER_V2[] = {0x55, 0x55, 0x00, 0x02};
static uint32_t deserialize_bytes_to_int32(uint8_t *val_serial)
{
    return (((uint32_t)val_serial[0]) << 24) |
            (((uint32_t)val_serial[1]) << 16) |
            (((uint32_t)val_serial[2]) << 8) |
            ((uint32_t)val_serial[3]);
}

static uint64_t deserialize_bytes_to_int64(uint8_t *val_serial)
{
    return (((uint64_t)val_serial[0]) << 56) |
            (((uint64_t)val_serial[1]) << 48) |
            (((uint64_t)val_serial[2]) << 40) |
            (((uint64_t)val_serial[3]) << 32) |
            (((uint64_t)val_serial[4]) << 24) |
            (((uint64_t)val_serial[5]) << 16) |
            (((uint64_t)val_serial[6]) << 8) |
            ((uint64_t)val_serial[7]);
}

void hwsync_header_extract(struct hw_avsync_header *header)
{
    uint32_t frame_size = deserialize_bytes_to_int32(&header->header[4]);
    uint64_t pts = deserialize_bytes_to_int64(&header->header[8]);
    hwsync_header_set_frame_size(header, frame_size);
    hwsync_header_set_pts(header, pts);
}

static inline bool hwsync_header_validate(uint8_t *header)
{
    return (header[0] == 0x55) &&
           (header[1] == 0x55) &&
           (header[2] == 0x00) &&
           (header[3] == 0x02);
}

void serialize_int32_to_bytes(uint8_t *byte, uint32_t val)
{
    int size = sizeof(uint32_t);
    int i = 0;
    for (i = 0; i < size; i++) {
        byte[i] = (val >> ((size - 1 - i) * 8)) & 0xff;
    }
}
void serialize_int64_to_bytes(uint8_t *byte, uint64_t val)
{
    int size = sizeof(uint64_t);
    int i = 0;
    for (i = 0; i < size; i++) {
        byte[i] = (val >> ((size - 1 - i) * 8)) & 0xff;
    }
}

void hwsync_header_construct(struct hw_avsync_header *header)
{
    size_t index = 0;

    header->header[index++] = 0x55;
    header->header[index++] = 0x55;
    header->header[index++] = 0x00;
    header->header[index++] = 0x02;
    serialize_int32_to_bytes(&header->header[index], header->frame_size);
    index += 4;
    serialize_int64_to_bytes(&header->header[index], header->pts);
    index += 8;
}

uint32_t hwsync_header_get_frame_size(struct hw_avsync_header *header)
{
    if (header->is_complete)
        return header->frame_size;
    ALOGE("%s(), header not complete", __func__);
    return 0;
}
static uint64_t hwsync_header_get_apts(struct hw_avsync_header *header)
{
    if (header->is_complete)
        return header->pts;
    return 0;
}

void hwsync_header_set_frame_size(struct hw_avsync_header *header, uint32_t frame_size)
{
    if (frame_size > HW_AVSYNC_FRAME_SIZE || frame_size == 0) {
        ALOGE("%s(), invalid frame size (%d), 0 or exeeds %d",
                __func__, frame_size, HW_AVSYNC_FRAME_SIZE);
    }
    header->frame_size = frame_size;
}

void hwsync_header_set_pts(struct hw_avsync_header *header, uint64_t pts)
{
    header->pts = pts;
}

int hwsync_read_header_byte(struct hw_avsync_header *header, uint8_t *byte)
{
    if (!header || !byte || header->bytes_read >= header->header_size)
        return -EINVAL;

    if (header->bytes_read == 0)
        hwsync_header_construct(header);

    *byte = header->header[header->bytes_read];
    header->bytes_read++;
    if (header->bytes_read >= header->header_size)
        header->is_complete = true;

    return 0;
}

int hwsync_write_header_byte(struct hw_avsync_header *header, uint8_t byte)
{
    size_t size = sizeof(HW_AVSYNC_HEADER_V2);
    if (!header || (header->version_num > 0 && header->bytes_written >= header->header_size)) {
        ALOGE("%s(), header null or inval written bytes", __func__);
        return -EINVAL;
    }
    ALOGV("header->bytes_written:%d byte:%0x",header->bytes_written,byte);
    if (header->bytes_written < (HW_SYNC_VERSION_SIZE - 1) &&
        byte == HW_AVSYNC_HEADER_V2[header->bytes_written]) {
        header->header[header->bytes_written++] = byte;
    } else if (header->bytes_written == (HW_SYNC_VERSION_SIZE - 1)) {
        header->header[header->bytes_written++] = byte;
        if (byte == 1) {
            ALOGV("version_num %d ",byte);
            header->version_num = byte;
            header->header_size = HW_AVSYNC_HEADER_SIZE_V1;
        } else if (byte == 2) {
            ALOGV("version_num %d ",byte);
            header->version_num = byte;
            header->header_size = HW_AVSYNC_HEADER_SIZE_V2;
        } else {
            ALOGE("invalid version_num %d ",header->version_num);
        }
    } else if (header->bytes_written >= HW_SYNC_VERSION_SIZE ) {
        header->header[header->bytes_written++] = byte;
        if (header->bytes_written >= header->header_size &&
            (header->version_num == 2 || header->version_num == 1)) {
            header->is_complete = true;
            hwsync_header_extract(header);
        }
    } else {
        ALOGE("%s(), invalid data %d, bytes_wrtten %d",
                __func__, byte, header->bytes_written);
        header->bytes_written = 0;
        return -EINVAL;
    }

    return 0;
}

bool hwsync_is_header_complete(struct hw_avsync_header *header)
{
    return header->is_complete;
}

int hwsync_header_reset(struct hw_avsync_header *header)
{
    if (!header)
        return -EINVAL;
    ALOGV("%s()", __func__);
    memset(header, 0, sizeof(struct hw_avsync_header));
    header->bytes_read = 0;
    header->bytes_written = 0;
    header->frame_size = 0;
    header->is_complete = false;
    header->pts = 0;

    return 0;
}

size_t hwsync_header_get_bytes_remaining(struct hw_avsync_header *header)
{
    return header->header_size - header->bytes_written;
}

void extractor_consume_output(struct hw_avsync_header_extractor *header_extractor)
{
    size_t written = 0;
    while (header_extractor->data_size_bytes  > 0) {
        ALOGV("++%s, data size =%d", __func__, header_extractor->data_size_bytes);
        written = header_extractor->consume_output_data(
            header_extractor->cbk_cookie,
            header_extractor->data,
            header_extractor->data_size_bytes);
        ALOGV("%s, header_extractor->data_size_bytes = %d, written =%d", __func__,
                header_extractor->data_size_bytes, written);
        if (written <= header_extractor->data_size_bytes) {
            header_extractor->sync_frame_written += written;
            header_extractor->data_size_bytes -= written;
            memmove(header_extractor->data,
                header_extractor->data + written, header_extractor->data_size_bytes);
        } else {
            ALOGE("%s(), fatal error", __func__);
        }
        //header_extractor->sync_frame_written += written;
        //header_extractor->data_size_bytes -= written;
        ALOGV("--%s, data size =%d", __func__, header_extractor->data_size_bytes);
        ALOGV("--%s, sync_frame_written =%d", __func__, header_extractor->sync_frame_written);
    }
}

static void extractor_reset(struct hw_avsync_header_extractor *header_extractor)
{
    ALOGV("%s()", __func__);
    header_extractor->is_reading_avsync_header = true;
    header_extractor->data_size_bytes = 0;
    header_extractor->sync_frame_written = 0;
    hwsync_header_reset(&header_extractor->avsync_header);
}

ssize_t header_extractor_write(struct hw_avsync_header_extractor *header_extractor,
            const void *buffer, size_t bytes)
{
    size_t bytes_remaining = bytes;
    const uint8_t *data = (const uint8_t *)buffer;
    struct hw_avsync_header *sync_header = &header_extractor->avsync_header;

    while (bytes_remaining > 0) {
        if (header_extractor->is_reading_avsync_header) {
            ALOGV("--%s() writing header byte val %#x", __func__, *data);
            int ret = hwsync_write_header_byte(sync_header, *data);
            if (ret < 0) {
                ALOGE("%s(), invalid data!!, bytes_remaining %d", __func__, bytes_remaining);
                extractor_reset(header_extractor);
                bytes_remaining--;
                data++;
                //continue;
                return ret;
            }
            if (hwsync_is_header_complete(sync_header)) {
                uint32_t frame_size = hwsync_header_get_frame_size(sync_header);

                // means to start read body frames
                header_extractor->is_reading_avsync_header = false;
                // callback to consume the meta data
                header_extractor->consume_meta_data(
                        header_extractor->cbk_cookie,
                        frame_size,
                        hwsync_header_get_apts(sync_header),
                        header_extractor->payload_offset);
                // prepare the body data
                header_extractor->data_size_bytes = 0;
                memset(header_extractor->data, 0, HW_AVSYNC_FRAME_SIZE);
                // accumulate the payload consumed
                header_extractor->payload_offset += frame_size;
                ALOGV("%s() filling header complete, framesize = %d, payload offset %lld",
                        __func__, frame_size, header_extractor->payload_offset);
            }
            bytes_remaining--;
            data++;
        } else {
            ALOGV("start dealing body read, bytes_remaing %d, sync_frame_written %d",
                    bytes_remaining, header_extractor->sync_frame_written);
            size_t bytes_to_copy = min(bytes_remaining,
                    hwsync_header_get_frame_size(sync_header) - header_extractor->sync_frame_written);
            ALOGV("%s() writing body_bytes= %d,data_size_bytes= %d",
                    __func__, bytes_to_copy, header_extractor->data_size_bytes);

            if (header_extractor->data_size_bytes + bytes_to_copy > HW_AVSYNC_FRAME_SIZE) {
                ALOGE("%s() buffer overflow", __func__);
                return bytes;
            }
            memcpy(header_extractor->data + header_extractor->data_size_bytes, data, bytes_to_copy);
            header_extractor->data_size_bytes += bytes_to_copy;
            bytes_remaining -= bytes_to_copy;
            data += bytes_to_copy;

            if (header_extractor->sync_frame_written + bytes_to_copy >= hwsync_header_get_frame_size(sync_header)) {
                // prepare for next header
                header_extractor->is_reading_avsync_header = true;
                extractor_consume_output(header_extractor);
                extractor_reset(header_extractor);
                header_extractor->sync_frame_written = 0;
                ALOGV("%s() reading body over, next to reading header, return bytes %d",
                        __func__, bytes - bytes_remaining);
                continue;
                //return bytes - bytes_remaining;
            }
        }
    }

    if (header_extractor->data_size_bytes > 0) {
        extractor_consume_output(header_extractor);
    }
    ALOGV("--%s()", __func__);
    return bytes;
}

struct hw_avsync_header_extractor *
new_hw_avsync_header_extractor(
        consume_meta_data_t consume_meta_data,
        consume_output_data_t consume_output_data,
        //int (*consume_meta_data)(void *cookie, uint32_t frame_size, int64_t pts, uint32_t offset),
        //int (*consume_output_data)(void *cookie, const void *buffer, size_t bytes),
        void *cookie)
{
    struct hw_avsync_header_extractor *header_extractor = NULL;

    header_extractor = (struct hw_avsync_header_extractor *)calloc(1, sizeof(*header_extractor));
    if (!header_extractor) {
        return NULL;
    }

    extractor_reset(header_extractor);
    header_extractor->consume_meta_data = consume_meta_data;
    header_extractor->consume_output_data = consume_output_data;
    header_extractor->cbk_cookie = cookie;
    //header_extractor->tsync_fd = aml_hwsync_open_tsync();
    //ALOGV("header_extractor->tsync_fd = %d", header_extractor->tsync_fd);
    //if (header_extractor->tsync_fd < 0)
    //    ALOGE("%s(), fail to open tsync", __func__);
    return header_extractor;
}

void delete_hw_avsync_header_extractor(struct hw_avsync_header_extractor *header_extractor)
{
    if (header_extractor) {
        //ALOGV("header_extractor->tsync_fd = %d", header_extractor->tsync_fd);
        //aml_hwsync_close_tsync(header_extractor->tsync_fd);
        //header_extractor->tsync_fd = -1;
        free(header_extractor);
        header_extractor = NULL;
    }
}

void flush_hw_avsync_header_extractor(struct hw_avsync_header_extractor *header_extractor)
{
    if (header_extractor) {
        extractor_reset(header_extractor);
        header_extractor->payload_offset = 0;
    }
}
