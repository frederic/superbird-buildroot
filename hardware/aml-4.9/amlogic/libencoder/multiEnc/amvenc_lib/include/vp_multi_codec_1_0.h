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


#ifndef _INCLUDED_COM_VIDEO_MULTI_CODEC
#define _INCLUDED_COM_VIDEO_MULTI_CODEC

#include <stdlib.h>
#include <stdbool.h>

#define vl_codec_handle_t long


 typedef struct enc_frame_extra_info {
  int frame_type; /* encoded frame type as vl_frame_type_t */
  int average_qp_value; /* average qp value of the encoded frame */
  int intra_blocks;  /* intra blockes (in 8x8) of the encoded frame */
  int merged_blocks; /* merged blockes (in 8x8) of the encoded frame */
  int skipped_blocks; /* skipped blockes (in 8x8) of the encoded frame */
} enc_frame_extra_info_t;

/* encoded frame info */
typedef struct encoding_metadata_e {
  int encoded_data_length_in_bytes; /* size of the encoded buffer */
  bool is_key_frame; /* if true, the encoded frame is a keyframe */
  int timestamp_us;  /* timestamp in usec of the encode frame */
  bool is_valid;     /* if true, the encoding was successful */
  enc_frame_extra_info_t extra; /* extra info of encoded frame if is_valid true */
  int err_cod; /* error code if is_valid is false: >0 normal, others error */
} encoding_metadata_t;

typedef enum vl_codec_id_e {
  CODEC_ID_NONE,
  CODEC_ID_VP8,
  CODEC_ID_H261,
  CODEC_ID_H263,
  CODEC_ID_H264, /* must support */
  CODEC_ID_H265,
} vl_codec_id_t;

typedef enum vl_img_format_e {
  IMG_FMT_NONE,
  IMG_FMT_NV12, /* must support  */
  IMG_FMT_NV21,
  IMG_FMT_YUV420P,
  IMG_FMT_YV12
} vl_img_format_t;

typedef enum vl_frame_type_e {
  FRAME_TYPE_NONE,
  FRAME_TYPE_AUTO, /* encoder self-adaptation(default) */
  FRAME_TYPE_IDR,
  FRAME_TYPE_I,
  FRAME_TYPE_P,
  FRAME_TYPE_B,
} vl_frame_type_t;

typedef enum vl_fmt_type_e {
  AML_FMT_ENC = 0,
  AML_FMT_RAW = 1,
} vl_fmt_type_t;
/* buffer type*/
typedef enum {
  VMALLOC_TYPE = 0,
  CANVAS_TYPE = 1,
  PHYSICAL_TYPE = 2,
  DMA_TYPE = 3,
} vl_buffer_type_t;

/* encoder features configure flags bit masks for enc_feature_opts */
/* Enable RIO feature.
        bit field value 1: enable, 0: diablae (default) */
#define ENABLE_ROI_FEATURE      0x1
/* Encode parameter update on the fly.
        bit field value: 1: enable, 0: disable (default) */
#define ENABLE_PARA_UPDATE      0x2
/* Encode long term references feauture.
        bit field value: 1 enable, 0: disable (default)*/
#define ENABLE_LONG_TERM_REF    0x80
/* encoder info config */
typedef struct vl_encode_info {
  int width;
  int height;
  int frame_rate;
  int bit_rate;
  int gop;
  bool prepend_spspps_to_idr_frames;
  vl_img_format_t img_format;
  int qp_mode; /* 1: use customer QP range, 0:use default QP range */
  int enc_feature_opts; /* option features flag settings.*/
                        /* See above for fields definition in detail */
                       /* bit 0: qp hint(roi) 0:disable (default) 1: enable */
                       /* bit 1: param_change 0:disable (default) 1: enable */
                       /* bit 2 to 6: gop_opt:0 (default), 1:I only 2:IP, */
                       /*                     3: IBBBP, 4: IP_SVC1, 5:IP_SVC2 */
                       /*                     6: IP_SVC3, 7: IP_SVC4 */
                       /*                     see define of AMVGOPModeOPT */
                       /* bit 7:LTR control   0:disable (default) 1: enable*/
} vl_encode_info_t;

/* dma buffer info*/
/*for nv12/nv21, num_planes can be 1 or 2
  for yv12, num_planes can be 1 or 3
 */
typedef struct vl_dma_info {
  int shared_fd[3];
  unsigned int num_planes;//for nv12/nv21, num_planes can be 1 or 2
} vl_dma_info_t;

/*When the memory type is V4L2_MEMORY_DMABUF, dma_info.shared_fd is a
  file descriptor associated with a DMABUF buffer.
  When the memory type is V4L2_MEMORY_USERPTR, in_ptr is a userspace
  pointer to the memory allocated by an application.
*/

typedef union {
  vl_dma_info_t dma_info;
  unsigned long in_ptr[3];
} vl_buf_info_u;

/* input buffer info
 * buf_type = VMALLOC_TYPE correspond to  buf_info.in_ptr
   buf_type = DMA_TYPE correspond to  buf_info.dma_info
 */
typedef struct vl_buffer_info {
  vl_buffer_type_t buf_type;
  vl_buf_info_u buf_info;
} vl_buffer_info_t;

/* noise reduction type*/
typedef enum {
  NR_DISABLE = 0,
  NR_SPATIAL = 1,
  NR_TEMPORAL = 2,
  NR_BOTH = 3,
} nr_mode_type_t;

typedef struct vl_param_runtime {
  int* idr;
  int bitrate;
  int frame_rate;

  bool enable_vfr;
  int min_frame_rate;

  nr_mode_type_t nr_mode;
} vl_param_runtime_t;

typedef struct qp_param_s {
  int qp_min;
  int qp_max;
  int qp_I_base;
  int qp_P_base;
  int qp_B_base;
  int qp_I_min;
  int qp_I_max;
  int qp_P_min;
  int qp_P_max;
  int qp_B_min;
  int qp_B_max;
} qp_param_t;

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Getting version information
 *
 *@return : version information
 */
const char* vl_get_version();

/**
 * init encoder
 *
 *@param : codec_id: codec type
 *@param : vl_encode_info_t: encode info
 *         width:      video width
 *         height:     video height
 *         frame_rate: framerate
 *         bit_rate:   bitrate
 *         gop GOP:    max I frame interval
 *         prepend_spspps_to_idr_frames: if true, adds spspps header
 *         to all idr frames (keyframes).
 *         buf_type:
 *         0: need memcpy from input buf to encoder internal dma buffer.
 *         3: input buf is dma buffer, encoder use input buf without memcopy.
 *img_format: image format
 *@return : if success return encoder handle,else return <= 0
 */
vl_codec_handle_t vl_multi_encoder_init(vl_codec_id_t codec_id,
                                        vl_encode_info_t encode_info,
                                        qp_param_t* qp_tbl);


/**
 * encode video
 *
 *@param : handle
 *@param : type: frame type
 *@param : in: data to be encoded
 *@param : in_size: data size
 *@param : out: data output,need header(0x00，0x00，0x00，0x01),and format
 *         must be I420(apk set param out，through jni,so modify "out" in the
 *         function,don't change address point)
 *@param : in_buffer_info
 *         buf_type:
 *              VMALLOC_TYPE: need memcpy from input buf to encoder internal dma buffer.
 *              DMA_TYPE: input buf is dma buffer, encoder use input buf without memcopy.
 *         buf_info.dma_info: input buf dma info.
 *              num_planes:For nv12/nv21, num_planes can be 1 or 2.
 *                         For YV12, num_planes can be 1 or 3.
 *              shared_fd: DMA buffer fd.
 *         buf_info.in_ptr: input buf ptr.
 *@param : ret_buffer_info
 *         buf_type:
 *              VMALLOC_TYPE: need memcpy from input buf to encoder internal dma buffer.
 *              DMA_TYPE: input buf is dma buffer, encoder use input buf without memcopy.
 *              due to references and reordering, the DMA buffer may not return immedialtly.
 *         buf_info.dma_info: returned buf dma info if any.
 *              num_planes:For nv12/nv21, num_planes can be 1 or 2.
 *                         For YV12, num_planes can be 1 or 3.
 *              shared_fd: DMA buffer fd.
 *         buf_info.in_ptr: returned input buf ptr.
 *@return ：if success return encoded data length,else return <= 0
 */
encoding_metadata_t vl_multi_encoder_encode(vl_codec_handle_t handle,
                                           vl_frame_type_t type,
                                           unsigned char* out,
                                           vl_buffer_info_t *in_buffer_info,
                                           vl_buffer_info_t *ret_buffer_info);


/**
 *
 * vl_video_encoder_update_qp_hint
 *@param : handle
 *@param : pq_hint_table: the char pointer with hint qp value (0-51) of each
 *block in raster scan order.block size for AVC is 16x16, HEVC is 32x32)
 *@size : size of the pq_hint_table.it must be equal or larger than the total
 *number of the blocks of the whole frame. otherwise it will take no action.
 *
 *@return : if success return 0 ,else return <= 0
 */
int vl_video_encoder_update_qp_hint(vl_codec_handle_t handle,
                            unsigned char *pq_hint_table,
                            int size);
/**
 *
 * vl_video_encoder_change_bitrate
 * Change the taget bitrate in encoding
 *@param : handle
 *@param : bitRate: the new target encode bitrate
 *@return : if success return 0 ,else return <= 0
 */
int vl_video_encoder_change_bitrate(vl_codec_handle_t handle,
                            int bitRate);

/**
 *
 * vl_video_encoder_change_qp
 * Change the QP setings in the encoding
 *
 *@param : handle
 *@param : minQpI  the new min QP for I frame
 *@param : maxQpI  the new max QP for I frame
 *@param : maxDeltaQp  the new max QP differneces in a frame
 *@param : minQpP  the new min QP for P frame
 *@param : maxQpP  the new max QP for P frame
 *@param : minQpB  the new min QP for B frame
 *@param : maxQpB  the new max QP for B frame
 *@return : if success return 0 ,else return <= 0
 */
int vl_video_encoder_change_qp(vl_codec_handle_t handle,
                                int minQpI, int maxQpI, int maxDeltaQp,
                                int minQpP, int maxQpP,
                                int minQpB, int maxQpB);
/*
 * vl_video_encoder_change_gop
 * Change the Gop period settings
 *
 *@param : handle
 *@param : intraQP  the new init QP for the I frame
 *@param : GOPPeriod the new GOP period
 *@return : if success return 0 ,else return <= 0
 */
int vl_video_encoder_change_gop(vl_codec_handle_t handle,
                                int intraQP, int GOPPeriod);
/*
 * vl_video_encoder_longterm_ref
 * set up long term reference flags
 *
 *@param : handle
 *@param : LongtermRefFlags:
 *         bit 0: current frame use as long term reference
 *         bit 1: current frame use LTF as reference to encode.
 *
 *@return : if success return 0 ,else return <= 0
 */
int vl_video_encoder_longterm_ref(vl_codec_handle_t codec_handle,
                                  int LongtermRefFlags);
/**
 * destroy encoder
 *
 *@param ：handle: encoder handle
 *@return ：if success return 1,else return 0
 */
int vl_multi_encoder_destroy(vl_codec_handle_t handle);


#ifdef __cplusplus
}
#endif

#endif /* _INCLUDED_COM_VIDEO_MULTI_CODEC */
