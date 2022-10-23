//----------------------------------------------------------------------------
//   The confidential and proprietary information contained in this file may
//   only be used by a person authorised under and to the extent permitted
//   by a subsisting licensing agreement from ARM Limited or its affiliates.
//
//          (C) COPYRIGHT [2018] ARM Limited or its affiliates.
//              ALL RIGHTS RESERVED
//
//   This entire notice must be reproduced on all copies of this file
//   and copies of this file may only be made by a person if such person is
//   permitted to do so under the terms of a subsisting license agreement
//   from ARM Limited or its affiliates.
//----------------------------------------------------------------------------

#ifndef __CAPTURE_H__
#define __CAPTURE_H__
/*
 * Apical(ARM) V4L2 test application 2016
 *
 * This is debugging purpose SW tool running on JUNO.
 */

#include <linux/videodev2.h>

#define FRAME_PACK_QUEUE_SIZE   2
#define FILE_NAME_LENGTH        64

typedef enum {
    V4L2_TEST_CAPTURE_NONE = 0,
    V4L2_TEST_CAPTURE_LEGACY,
	V4L2_TEST_CAPTURE_FRM,
    V4L2_TEST_CAPTURE_DNG,
    V4L2_TEST_CAPTURE_MAX,
} capture_type_t;

typedef struct _frame_t {
    uint32_t            vfd;
    struct v4l2_buffer  vbuf;
    uint32_t 			pixelformat;
    int                 width[VIDEO_MAX_PLANES];
    int                 height[VIDEO_MAX_PLANES];
    int                 bit_depth[VIDEO_MAX_PLANES];
    int                 bytes_per_line[VIDEO_MAX_PLANES];
    void                * paddr[VIDEO_MAX_PLANES];
    int 				num_planes;
} frame_t;

typedef struct _frame_pack_t {
    uint32_t            frame_id;
    uint32_t            frame_flag;
    frame_t             frame_data[ARM_V4L2_TEST_STREAM_MAX];
} frame_pack_t;

typedef struct _capture_module_t {
    /* frame queue */
    frame_pack_t        frame_pack_queue[FRAME_PACK_QUEUE_SIZE];
    pthread_mutex_t     vmutex;

    /* capture hitsory */
    char                last_capture_filename[ARM_V4L2_TEST_STREAM_MAX][FILE_NAME_LENGTH];
    int                 capture_performed;
    uint32_t            last_buf_size;
    uint32_t            last_header_size;
#if ARM_V4L2_TEST_HAS_META
    uint32_t            last_metadata_buf_size;
#endif
} capture_module_t;

void init_capture_module(capture_module_t * cap_mod);
void release_capture_module_stream(capture_module_t * cap_mod, int stream_type);
int  enqueue_buffer(capture_module_t *cap_mod, uint32_t stream_type,
                    frame_t *newframe, capture_type_t try_capture);
#endif
