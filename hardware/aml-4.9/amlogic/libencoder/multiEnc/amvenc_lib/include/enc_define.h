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

#ifndef AMLOGIC_ENCODER_DEFINE_
#define AMLOGIC_ENCODER_DEFINE_

#include <malloc.h>
#include <stdint.h>
#include <string.h>

#define AVC_ABS(x) (((x) < 0) ? -(x) : (x))
#define AVC_MAX(x, y) ((x) > (y) ? (x) : (y))
#define AVC_MIN(x, y) ((x) < (y) ? (x) : (y))
#define AVC_MEDIAN(A, B, C)                              \
  ((A) > (B) ? ((A) < (C) ? (A) : (B) > (C) ? (B) : (C)) \
             : (B) < (C) ? (B) : (C) > (A) ? (C) : (A))

//---------------------------------------------------
// ENCODER_STATUS define
//---------------------------------------------------
#define ENCODER_IDLE 0
#define ENCODER_SEQUENCE 1
#define ENCODER_PICTURE 2
#define ENCODER_IDR 3
#define ENCODER_NON_IDR 4
#define ENCODER_MB_HEADER 5
#define ENCODER_MB_DATA 6

#define ENCODER_SEQUENCE_DONE 7
#define ENCODER_PICTURE_DONE 8
#define ENCODER_IDR_DONE 9
#define ENCODER_NON_IDR_DONE 10
#define ENCODER_MB_HEADER_DONE 11
#define ENCODER_MB_DATA_DONE 12

/* defines for H.264 IntraPredMode */
// 4x4 intra prediction modes
#define HENC_VERT_PRED 0
#define HENC_HOR_PRED 1
#define HENC_DC_PRED 2
#define HENC_DIAG_DOWN_LEFT_PRED 3
#define HENC_DIAG_DOWN_RIGHT_PRED 4
#define HENC_VERT_RIGHT_PRED 5
#define HENC_HOR_DOWN_PRED 6
#define HENC_VERT_LEFT_PRED 7
#define HENC_HOR_UP_PRED 8

// 16x16 intra prediction modes
#define HENC_VERT_PRED_16 0
#define HENC_HOR_PRED_16 1
#define HENC_DC_PRED_16 2
#define HENC_PLANE_16 3

// 8x8 chroma intra prediction modes
#define HENC_DC_PRED_8 0
#define HENC_HOR_PRED_8 1
#define HENC_VERT_PRED_8 2
#define HENC_PLANE_8 3

/********************************************
 * defines for H.264 mb_type
 ********************************************/
#define HENC_MB_Type_PBSKIP 0x0
#define HENC_MB_Type_PSKIP 0x0
#define HENC_MB_Type_BSKIP_DIRECT 0x0
#define HENC_MB_Type_P16x16 0x1
#define HENC_MB_Type_P16x8 0x2
#define HENC_MB_Type_P8x16 0x3
#define HENC_MB_Type_SMB8x8 0x4
#define HENC_MB_Type_SMB8x4 0x5
#define HENC_MB_Type_SMB4x8 0x6
#define HENC_MB_Type_SMB4x4 0x7
#define HENC_MB_Type_P8x8 0x8
#define HENC_MB_Type_I4MB 0x9
#define HENC_MB_Type_I16MB 0xa
#define HENC_MB_Type_IBLOCK 0xb
#define HENC_MB_Type_SI4MB 0xc
#define HENC_MB_Type_I8MB 0xd
#define HENC_MB_Type_IPCM 0xe
#define HENC_MB_Type_AUTO 0xf

#define HENC_MB_Type_P16MB_HORIZ_ONLY 0x7f
#define HENC_MB_CBP_AUTO 0xff
#define HENC_SKIP_RUN_AUTO 0xffff

#define ENCODER_BUFFER_INPUT 0
#define ENCODER_BUFFER_REF0 1
#define ENCODER_BUFFER_REF1 2
#define ENCODER_BUFFER_OUTPUT 3
#define ENCODER_BUFFER_INTER_INFO 4
#define ENCODER_BUFFER_INTRA_INFO 5
#define ENCODER_BUFFER_QP 6
#define ENCODER_BUFFER_DUMP 7
#define ENCODER_BUFFER_CBR 8

#define I_FRAME 2

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint64_t uint64;
typedef int64_t int64;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef enum {
  NO_DEFINE = -1,
  M8 = 0,
  MAX_DEV = 1,
} ENC_DEV_TYPE;

typedef enum {
  VMALLOC_BUFFER = 0,
  CANVAS_BUFFER = 1,
  PHYSICAL_BUFF = 2,
  DMA_BUFF = 3,
  MAX_TYPE = 4,
} AMVEncBufferType;

typedef enum {
  AMVENC_YUV422_SINGLE = 0,
  AMVENC_YUV444_SINGLE,
  AMVENC_NV21,
  AMVENC_NV12,
  AMVENC_YUV420P,
  AMVENC_YUV444_PLANE,
  AMVENC_RGB888,
  AMVENC_RGB888_PLANE,
  AMVENC_RGB565,
  AMVENC_RGBA8888,
  AMVENC_FRAME_FMT
} AMVEncFrameFmt;

typedef enum {
  AMV_AVC = 0,
  AMV_HEVC = 1,
  MAX_ENC_TYPE
} AMVEncStreamType;

typedef enum {
  /**
  Fail information, need to add more error code for more specific info
  */
  AMVENC_OVERFLOW = -39,
  AMVENC_INVALID_PARAM = -38,
  AMVENC_HARDWARE = -37,
  AMVENC_TIMEOUT = -36,
  AMVENC_TRAILINGONES_FAIL = -35,
  AMVENC_SLICE_EMPTY = -34,
  AMVENC_POC_FAIL = -33,
  AMVENC_CONSECUTIVE_NONREF = -32,
  AMVENC_CABAC_FAIL = -31,
  AMVENC_PRED_WEIGHT_TAB_FAIL = -30,
  AMVENC_DEC_REF_PIC_MARK_FAIL = -29,
  AMVENC_SPS_FAIL = -28,
  AMVENC_BITSTREAM_BUFFER_FULL = -27,
  AMVENC_BITSTREAM_INIT_FAIL = -26,
  AMVENC_CHROMA_QP_FAIL = -25,
  AMVENC_INIT_QS_FAIL = -24,
  AMVENC_INIT_QP_FAIL = -23,
  AMVENC_WEIGHTED_BIPRED_FAIL = -22,
  AMVENC_INVALID_INTRA_PERIOD = -21,
  AMVENC_INVALID_CHANGE_RATE = -20,
  AMVENC_INVALID_BETA_OFFSET = -19,
  AMVENC_INVALID_ALPHA_OFFSET = -18,
  AMVENC_INVALID_DEBLOCK_IDC = -17,
  AMVENC_INVALID_REDUNDANT_PIC = -16,
  AMVENC_INVALID_FRAMERATE = -15,
  AMVENC_INVALID_NUM_SLICEGROUP = -14,
  AMVENC_INVALID_POC_LSB = -13,
  AMVENC_INVALID_NUM_REF = -12,
  AMVENC_INVALID_FMO_TYPE = -11,
  AMVENC_ENCPARAM_MEM_FAIL = -10,
  AMVENC_LEVEL_NOT_SUPPORTED = -9,
  AMVENC_LEVEL_FAIL = -8,
  AMVENC_PROFILE_NOT_SUPPORTED = -7,
  AMVENC_TOOLS_NOT_SUPPORTED = -6,
  AMVENC_WRONG_STATE = -5,
  AMVENC_UNINITIALIZED = -4,
  AMVENC_ALREADY_INITIALIZED = -3,
  AMVENC_NOT_SUPPORTED = -2,
  AMVENC_MEMORY_FAIL = -1,
  AMVENC_FAIL = 0,
  /**
  Generic success value
  */
  AMVENC_SUCCESS = 1,
  AMVENC_PICTURE_READY = 2,
  AMVENC_NEW_IDR = 3, /* upon getting this, users have to call PVAVCEncodeSPS and PVAVCEncodePPS to get a new SPS and PPS*/
  AMVENC_SKIPPED_PICTURE = 4,  /* continuable error message */
  AMVENC_REENCODE_PICTURE = 5, /* reencode the picutre */
  AMVENC_FORCE_IDR_NEXT = 6,   /* force IDR next picutre */
} AMVEnc_Status;

typedef enum {
  AMVEnc_Initializing = 0,
  AMVEnc_Encoding_SPS,
  AMVEnc_Encoding_PPS,
  AMVEnc_Analyzing_Frame,
  AMVEnc_WaitingForBuffer,  // pending state
  AMVEnc_Encoding_Frame,
} AMVEnc_State;


// AVC profiles

#define AVC_BASELINE    66
#define AVC_MAIN        77
#define AVC_EXTENDED    88
#define AVC_HIGH        100
#define AVC_HIGH10      110
#define AVC_HIGH422     122
#define AVC_HIGH444     144

//AVC LEVELS
#define AVC_LEVEL_AUTO  0
#define AVC_LEVEL1_B    9
#define AVC_LEVEL1      10
#define AVC_LEVEL1_1    11
#define AVC_LEVEL1_2    12
#define AVC_LEVEL1_3    13
#define AVC_LEVEL2      20
#define AVC_LEVEL2_1    21
#define AVC_LEVEL2_2    22
#define AVC_LEVEL3      30
#define AVC_LEVEL3_1    31
#define AVC_LEVEL3_2    32
#define AVC_LEVEL4      40
#define AVC_LEVEL4_1    41
#define AVC_LEVEL4_2    42
#define AVC_LEVEL5      50
#define AVC_LEVEL5_1    51

//AVCNalUnitType

#define AVC_NALTYPE_SLICE       1 /* non-IDR non-data partition */
#define AVC_NALTYPE_DPA         2 /* data partition A */
#define AVC_NALTYPE_DPB         3 /* data partition B */
#define AVC_NALTYPE_DPC         4 /* data partition C */
#define AVC_NALTYPE_IDR         5 /* IDR NAL */
#define AVC_NALTYPE_SEI         6 /* supplemental enhancement info */
#define AVC_NALTYPE_SPS         7 /* sequence parameter set */
#define AVC_NALTYPE_PPS         8 /* picture parameter set */
#define AVC_NALTYPE_AUD         9 /* access unit delimiter */
#define AVC_NALTYPE_EOSEQ       10 /* end of sequence */
#define AVC_NALTYPE_EOSTREAM    11 /* end of stream */
#define AVC_NALTYPE_FILL        12 /* filler data */
#define AVC_NALTYPE_EMPTY       0xff /* Empty Nal */

//AVCSliceType
#define AVC_P_SLICE             0
#define AVC_B_SLICE             1
#define AVC_I_SLICE             2
#define AVC_SP_SLICE            3
#define AVC_SI_SLICE            4
#define AVC_P_ALL_SLICE         5
#define AVC_B_ALL_SLICE         6
#define AVC_I_ALL_SLICE         7
#define AVC_SP_ALL_SLICE        8
#define AVC_SI_ALL_SLICE        9

//HEVCProfile;
#define HEVC_NONE               0
#define HEVC_MAIN               1
#define HEVC_MAIN10             2
#define HEVC_MAINSTILLPICTURE   3
#define HEVC_MAINREXT           4
#define HEVC_HIGHTHROUGHPUTREXT 5

//HEVCTier
#define HEVC_TIER_MAIN 0
#define HEVC_TIER_HIGH 1

//HEVCLevel
#define HEVC_LEVEL_NONE 0
#define HEVC_LEVEL1     30
#define HEVC_LEVEL2     60
#define HEVC_LEVEL2_1   63
#define HEVC_LEVEL3     90
#define HEVC_LEVEL3_1   93
#define HEVC_LEVEL4     120
#define HEVC_LEVEL4_1   123
#define HEVC_LEVEL5     150
#define HEVC_LEVEL5_1   153
#define HEVC_LEVEL5_2   156
#define HEVC_LEVEL6     180
#define HEVC_LEVEL6_1   183
#define HEVC_LEVEL6_2   186
#define HEVC_LEVEL8_5   255

//HEVCNalUnitType
//trailing non-IRAP pictures
#define NAL_UNIT_CODED_SLICE_TRAIL_N    0 // 0, non-reference
#define NAL_UNIT_CODED_SLICE_TRAIL_R    1 // 1 reference
#define NAL_UNIT_CODED_SLICE_TSA_N      2 // 2, non-reference
#define NAL_UNIT_CODED_SLICE_TSA_R      3 // 3, reference
#define  NAL_UNIT_CODED_SLICE_STSA_N    4 // 4, non-reference
#define NAL_UNIT_CODED_SLICE_STSA_R     5 // 5, reference
//leading pictures
#define NAL_UNIT_CODED_SLICE_RADL_N     6 // 6, non-reference
#define NAL_UNIT_CODED_SLICE_RADL_R     7 // 7, reference
#define NAL_UNIT_CODED_SLICE_RASL_N     8 // 8, non-reference
#define NAL_UNIT_CODED_SLICE_RASL_R     9 // 9, reference
//reserved non-IRAP
#define NAL_UNIT_RESERVED_VCL_N10       10
#define  NAL_UNIT_RESERVED_VCL_R11      11
#define NAL_UNIT_RESERVED_VCL_N12       12
#define NAL_UNIT_RESERVED_VCL_R13       13
#define NAL_UNIT_RESERVED_VCL_N14       14
#define NAL_UNIT_RESERVED_VCL_R15       15
//Intra random access point(IRAP) pictures
#define NAL_UNIT_CODED_SLICE_BLA_W_LP   16
#define NAL_UNIT_CODED_SLICE_BLA_W_RADL 17
#define NAL_UNIT_CODED_SLICE_BLA_N_LP   18
#define NAL_UNIT_CODED_SLICE_IDR_W_RADL 19
#define NAL_UNIT_CODED_SLICE_IDR_N_LP   20
#define NAL_UNIT_CODED_SLICE_CRA        21
//reserved IRAP
#define NAL_UNIT_RESERVED_IRAP_VCL22    22
#define NAL_UNIT_RESERVED_IRAP_VCL23    23
//reserved non-IRAP
#define NAL_UNIT_RESERVED_VCL24         24
#define NAL_UNIT_RESERVED_VCL25         25
#define NAL_UNIT_RESERVED_VCL26         26
#define NAL_UNIT_RESERVED_VCL27         27
#define NAL_UNIT_RESERVED_VCL28         28
#define NAL_UNIT_RESERVED_VCL29         29
#define NAL_UNIT_RESERVED_VCL30         30
#define NAL_UNIT_RESERVED_VCL31         31
#define NAL_UNIT_VPS                    32
#define NAL_UNIT_SPS                    33
#define NAL_UNIT_PPS                    34
#define NAL_UNIT_ACCESS_UNIT_DELIMITER  35
#define NAL_UNIT_EOS                    36
#define NAL_UNIT_EOB                    37
#define NAL_UNIT_FILLER_DATA            38
#define NAL_UNIT_PREFIX_SEI             39
#define NAL_UNIT_SUFFIX_SEI             40
//reserved
#define NAL_UNIT_RESERVED_NVCL41        41
#define NAL_UNIT_RESERVED_NVCL42        42
#define NAL_UNIT_RESERVED_NVCL43        43
#define NAL_UNIT_RESERVED_NVCL44        44
#define NAL_UNIT_RESERVED_NVCL45        45
#define NAL_UNIT_RESERVED_NVCL46        46
#define NAL_UNIT_RESERVED_NVCL47        47
#define NAL_UNIT_UNSPECIFIED_48         48
#define NAL_UNIT_UNSPECIFIED_49         49
#define NAL_UNIT_UNSPECIFIED_50         50
#define  NAL_UNIT_UNSPECIFIED_51        51
#define NAL_UNIT_UNSPECIFIED_52         52
#define NAL_UNIT_UNSPECIFIED_53         53
#define NAL_UNIT_UNSPECIFIED_54         54
#define NAL_UNIT_UNSPECIFIED_55         55
#define NAL_UNIT_UNSPECIFIED_56         56
#define NAL_UNIT_UNSPECIFIED_57         57
#define NAL_UNIT_UNSPECIFIED_58         58
#define NAL_UNIT_UNSPECIFIED_59         59
#define NAL_UNIT_UNSPECIFIED_60         60
#define NAL_UNIT_UNSPECIFIED_61         61
#define NAL_UNIT_UNSPECIFIED_62         62
#define NAL_UNIT_UNSPECIFIED_63         63
#define NAL_UNIT_INVALID                64

//HEVCSliceType
#define HEVC_I_SLICE    0
#define HEVC_P_SLICE    1
#define HEVC_B_SLICE    2

//HEVCRefreshType
#define HEVC_NON_IRAP   0
#define HEVC_CRA        1
#define HEVC_IDR        2
#define HEVC_BLA        3

// seting flags
#define ENC_SETTING_OFF 0
#define ENC_SETTING_ON  1

#define AMVEncFrameIO_NONE_FLAG         0x00000000
#define AMVEncFrameIO_FORCE_IDR_FLAG    0x00000001
#define AMVEncFrameIO_FORCE_SKIP_FLAG   0x00000002
#define AMVEncFrameIO_USE_AS_LTR_FLAG   0x00000004
#define AMVEncFrameIO_USE_LTR_FLAG      0x00000008

#define AMVENC_FLUSH_FLAG_INPUT 0x1
#define AMVENC_FLUSH_FLAG_OUTPUT 0x2
#define AMVENC_FLUSH_FLAG_REFERENCE 0x4
#define AMVENC_FLUSH_FLAG_INTRA_INFO 0x8
#define AMVENC_FLUSH_FLAG_INTER_INFO 0x10
#define AMVENC_FLUSH_FLAG_QP 0x20
#define AMVENC_FLUSH_FLAG_DUMP 0x40
#define AMVENC_FLUSH_FLAG_CBR 0x80

#endif
