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



#ifndef AMLOGIC_HWENCODER_
#define AMLOGIC_HWENCODER_

#include "enc_define.h"
#include <stdbool.h>

#define amv_enc_handle_t long
#define NUM_CUSTOM_LAMBDA   (2*52)

typedef struct FrameIO_s {
  ulong YCbCr[3];
  // long long YCbCr[3];
  AMVEncBufferType type;
  AMVEncFrameFmt fmt;
  int pitch;
  int height;
  uint32 disp_order;
  uint is_reference;
  unsigned long long coding_timestamp;
  uint32 op_flag;
  uint32 canvas;
  uint32 bitrate;
  float frame_rate;
  uint32 scale_width;
  uint32 scale_height;
  uint32 crop_left;
  uint32 crop_right;
  uint32 crop_top;
  uint32 crop_bottom;
  uint32 num_planes;
  int shared_fd[3];
  // OUTPUT  bit-stream information
  int encoded_frame_type; //define of PicType in vpuapi.h
  int enc_average_qp; //average qp value of the encoded frame
  int enc_intra_blocks; //intra blockes (in 8x8) of the encoded frame
  int enc_merged_blocks; //merged blockes (in 8x8) of the encoded frame
  int enc_skipped_blocks; //skipped blockes (in 8x8) of the encoded frame
} AMVMultiEncFrameIO;

typedef enum AMVGOPModeOPT_S {
  GOP_OPT_NONE,
  GOP_ALL_I,
  GOP_IP,
  GOP_IBBBP,
  GOP_MAX_OPT,
} AMVGOPModeOPT;

typedef struct EncInitParams_s {
  AMVEncStreamType stream_type; /* encoded stream type AVC/HEVC */
  /* if profile/level is set to zero, encoder will choose the closest one for
   * you */
  uint32 profile;   /* profile of the bitstream to be compliant with*/
  uint32 level;     /* level of the bitstream to be compliant with*/
  uint32 hevc_tier;  /*heve tie 0 Main, 1 high tie */
  uint32 refresh_type;

  int width;  /* width of an input frame in pixel */
  int height; /* height of an input frame in pixel */
  AMVGOPModeOPT GopPreset; /* preset GOP structure */

  int num_ref_frame;   /* number of reference frame used */
  int num_slice_group; /* number of slice group */

  uint32 nSliceHeaderSpacing;

  uint32 auto_scd; /* scene change detection on or off */
  int idr_period;   /* idr frame refresh rate in number of target encoded */
                    /* frame (no concept of actual time).*/
  bool prepend_spspps_to_idr_frames; /* if true, prepends spspps header into all
                                    idr frames.*/
  uint32 fullsearch; /* enable full-pel full-search mode */
  int search_range;   /* search range for motion vector in */
                      /* (-search_range, +search_range) pixels */
  // AVCFlag sub_pel;    /* enable sub pel prediction */
  // AVCFlag submb_pred; /* enable sub MB partition mode */

  uint32 rate_control; /* rate control enable, on: RC on, off: constant QP */
  uint32 bitrate;       /* target encoding bit rate in bits/second */
  uint32 CPB_size;      /* coded picture buffer in number of bits */
  uint32 init_CBP_removal_delay; /* initial CBP removal delay in msec */

  uint32 frame_rate;
  /* note, frame rate is only needed by the rate control, AVC is timestamp
   * agnostic. */

  uint32 MBsIntraRefresh;
  uint32 MBsIntraOverlap;

  uint32 out_of_band_param_set; /* flag to set whether param sets are to be */
                                 /* retrieved up front or not */
  uint32 FreeRun;
  uint32 BitrateScale;
  uint32 dev_id;     /* ID to identify the hardware encoder version */
  uint8 encode_once; /* flag to indicate encode once or twice */
  uint32 src_width;  /*src buffer width before crop and scale */
  uint32 src_height; /*src buffer height before crop and scale */
  uint32 dev_fd;     /*actual encoder device fd*/
  AMVEncFrameFmt fmt;
  uint32 rotate_angle; // input frame rotate angle 0, 90, 180, 270
  uint32 mirror; /*frame mirror: 0-none,1-vert, 2, hor, 3, both V and H.*/
  uint32 roi_enable; /* enable roi  */
  uint32 lambda_map_enable; /* enable lambda map */
  uint32 mode_map_enable; /* enable mode map*/
  uint32 weight_pred_enable; /* enable weighted pred */
  uint32 param_change_enable; /* enable on the fly change parameters*/

  int qp_mode;
  int maxQP;
  int minQP;
  int initQP;           /* initial QP */
  int initQP_P;         /* initial QP P frame*/
  int initQP_B;         /* initial QP B frame */
  int maxDeltaQP;            /* max QP delta*/
  int maxQP_P;          /* max QP P frame */
  int minQP_P;          /* min QP P frame */
  int maxQP_B;          /* max QP B frame*/
  int minQP_B;          /* min QP B frame */
} AMVEncInitParams;

extern amv_enc_handle_t AML_MultiEncInitialize(AMVEncInitParams* encParam);
extern AMVEnc_Status AML_MultiEncSetInput(amv_enc_handle_t handle,
                                AMVMultiEncFrameIO *input);
extern AMVEnc_Status AML_MultiEncNAL(amv_enc_handle_t handle,
                                unsigned char* buffer,
                                unsigned int* buf_nal_size,
                                AMVMultiEncFrameIO *Retframe);
extern AMVEnc_Status AML_MultiEncHeader(amv_enc_handle_t handle,
                                unsigned char* buffer,
                                unsigned int* buf_nal_size);
extern AMVEnc_Status AML_MultiEncUpdateRoi(amv_enc_handle_t handle,
                                unsigned char* buffer,
                                int size);
extern AMVEnc_Status AML_MultiEncChangeBitRate(amv_enc_handle_t ctx_handle,
                                int BitRate);
extern AMVEnc_Status AML_MultiEncChangeQPMinMax(amv_enc_handle_t ctx_handle,
                                int minQpI, int maxQpI, int maxDeltaQp,
                                int minQpP, int maxQpP,
                                int minQpB, int maxQpB);
extern AMVEnc_Status AML_MultiEncChangeIntraPeriod(amv_enc_handle_t ctx_handle,
                                int IntraQP, int IntraPeriod);
extern AMVEnc_Status AML_MultiEncRelease(amv_enc_handle_t handle);

#endif
