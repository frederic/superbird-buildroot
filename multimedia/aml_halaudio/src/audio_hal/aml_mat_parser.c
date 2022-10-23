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

#define LOG_TAG "audio_hw_primary"
// #define LOG_NDEBUG 0

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/prctl.h>
#include <string.h>

#include "log.h"
#include "aml_mat_parser.h"

#define MAT_SYNC_CHUNK_BYTES 2
#define IS_MAT_FORMAT_SYNC(a,b) ((((a) == 0x07) && ((b) == 0x9e)) || (((a) == 0x9e) && ((b) == 0x07)))
#define IS_IEC61937_FORMAT_SYNC(a,b,c,d) ((((a) == 0x72) && ((b) == 0xf8) && ((c) == 0x1f) && ((d) == 0x4e)) || (((b) == 0x72) && ((a) == 0xf8) && ((d) == 0x1f) && ((c) == 0x4e)))

/*basic fuction*/
static int convert_format (unsigned char *buffer, int size)
{
    int idx;
    unsigned char tmp;

    for (idx = 0; idx < size;)
    {
        //buffer[idx] = (unsigned short)BYTE_REV(buffer[idx]);
        tmp = buffer[idx];
        buffer[idx] = buffer[idx + 1];
        buffer[idx + 1] = tmp;
        idx = idx + 2;
    }
    return size;
}

/*MAT sync chunk length is 2bytes(16bits)*/
static int parser_mat_sync_chunk(const void *audio_buffer, size_t audio_bytes)
{
    char *sync_chunk = (char *)audio_buffer;

    if (!audio_buffer || (audio_bytes == 0)) {
        ALOGV("audio_buffer %p audio_bytes %#x\n", audio_buffer, audio_bytes);
        return -1;
    }
    else {
        if (IS_MAT_FORMAT_SYNC(sync_chunk[0], sync_chunk[1]))
            return 0;
        else
            return 1;
    }
}

/*
 *Syntax                        Word size (bits)
 *metadata_payload_chunk() {
 *metadata_ payload_length          16
 *metadata_ payload _bytes()        variable
 *unused_ metadata_ payload _bits   variable
 *metadata_payload_crc              16
 *}  //end of metadata chunk
when stream_profile=1
metadat payload chunk is
"0x03 0x00 0x01 0x84 0x01 0x01 0x00 0x80 0x** 0x**" MSB
"0x00 0x03 0x84 0x01 0x01 0x01 0x80 0x00 0x** 0x**" LSB
 */

static int find_mat_stream_profile_in_metadata_payload_chunk(const void *audio_buffer, size_t audio_bytes)
{
    char metadata[8] = {0};
    char metadata_convert[8] = {0};
    char metadata_payload_chunk[8] = {0x00, 0x03, 0x84, 0x01, 0x01, 0x01, 0x80, 0x00};
    // LOG(LEVEL_INFO, "%s line %d %2x %2x %2x %2x %2x %2x %2x %2x\n",
    //         __FUNCTION__, __LINE__, metadata_payload_chunk[0], metadata_payload_chunk[1]
    //         , metadata_payload_chunk[2], metadata_payload_chunk[3], metadata_payload_chunk[4]
    //         , metadata_payload_chunk[5], metadata_payload_chunk[6], metadata_payload_chunk[7]);

    if (!audio_buffer || (audio_bytes < 8)) {
        ALOGV("audio_buffer %p audio_bytes %#x\n", audio_buffer, audio_bytes);
        return -1;
    }
    else {
        memcpy(metadata, audio_buffer, sizeof(metadata));
        // LOG(LEVEL_INFO, "%2x %2x %2x %2x %2x %2x %2x %2x\n",
        //      metadata[0], metadata[1], metadata[2], metadata[3], metadata[4], metadata[5], metadata[6], metadata[7]);
        memcpy(metadata_convert, audio_buffer, sizeof(metadata_convert));
        convert_format((unsigned char *)metadata_convert, sizeof(metadata_convert));
        // LOG(LEVEL_INFO, "%2x %2x %2x %2x %2x %2x %2x %2x\n",
        //     metadata_convert[0], metadata_convert[1], metadata_convert[2]
        //     , metadata_convert[3], metadata_convert[4], metadata_convert[5], metadata_convert[6], metadata_convert[7]);
        if (!memcmp(metadata, metadata_payload_chunk, sizeof(metadata)) || !memcmp(metadata_convert, metadata_payload_chunk, sizeof(metadata)))
            return 0;
        else {
            return 1;
        }
    }
}


int is_truehd_within_mat(const void *audio_buffer, size_t audio_bytes)
{
    int ret = !parser_mat_sync_chunk(audio_buffer, audio_bytes);
    if (ret)
        ret = !find_mat_stream_profile_in_metadata_payload_chunk(audio_buffer + MAT_SYNC_CHUNK_BYTES, audio_bytes - MAT_SYNC_CHUNK_BYTES);
    return ret;
}

int is_dolby_mat_available(int first_papb, int next_papb)
{
    char a[4] = {0};
    char b[4] = {0};
    int ret = 0;

    memcpy(a, &first_papb, sizeof(a));
    memcpy(b, &next_papb, sizeof(b));

    // LOG(LEVEL_INFO, "first %2x %2x %2x %2x\n", a[0], a[1], a[2], a[3]);
    // LOG(LEVEL_INFO, "next %2x %2x %2x %2x\n", b[0], b[1], b[2], b[3]);

    ret = (IS_IEC61937_FORMAT_SYNC(a[0], a[1], a[2], a[3])) && (IS_IEC61937_FORMAT_SYNC(b[0], b[1], b[2], b[3]));
    return ret;
}
