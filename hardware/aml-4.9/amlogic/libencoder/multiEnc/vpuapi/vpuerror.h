
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
/************************************************************************/
/* Error code definitions depending on product                          */
/************************************************************************/

#ifndef ERROR_CODE_H_INCLUDED
#define ERROR_CODE_H_INCLUDED

/*
 * VP5
 */

/************************************************************************/
/* VP5 SYSTEM ERROR (FAIL_REASON)                                     */
/************************************************************************/
#define VP5_QUEUEING_FAIL                                             0x00000001
#define VP5_SYSERR_ACCESS_VIOLATION_HW                                0x00000040
#define VP5_RESULT_NOT_READY                                          0x00000800
#define VP5_VPU_STILL_RUNNING                                         0x00001000
#define VP5_INSTANCE_DESTROYED                                        0x00004000
#define VP5_SYSERR_DEC_VLC_BUF_FULL                                   0x00010000
#define VP5_SYSERR_WATCHDOG_TIMEOUT                                   0x00020000
#define VP5_ERROR_FW_FATAL                                            0x00200000


/************************************************************************/
/* VP5 ERROR ON ENCODER (ERR_INFO)                                    */
/************************************************************************/
#define VP5_SYSERR_ENC_VLC_BUF_FULL                                   0x00000100

/************************************************************************/
/* VP5 ERROR ON DECODER (ERR_INFO)                                    */
/************************************************************************/
#define VP5_SPSERR_SEQ_PARAMETER_SET_ID                               0x00001000
#define VP5_SPSERR_CHROMA_FORMAT_IDC                                  0x00001001
#define VP5_SPSERR_PIC_WIDTH_IN_LUMA_SAMPLES                          0x00001002
#define VP5_SPSERR_PIC_HEIGHT_IN_LUMA_SAMPLES                         0x00001003
#define VP5_SPSERR_CONF_WIN_LEFT_OFFSET                               0x00001004
#define VP5_SPSERR_CONF_WIN_RIGHT_OFFSET                              0x00001005
#define VP5_SPSERR_CONF_WIN_TOP_OFFSET                                0x00001006
#define VP5_SPSERR_CONF_WIN_BOTTOM_OFFSET                             0x00001007
#define VP5_SPSERR_BIT_DEPTH_LUMA_MINUS8                              0x00001008
#define VP5_SPSERR_BIT_DEPTH_CHROMA_MINUS8                            0x00001009
#define VP5_SPSERR_LOG2_MAX_PIC_ORDER_CNT_LSB_MINUS4                  0x0000100A
#define VP5_SPSERR_SPS_MAX_DEC_PIC_BUFFERING                          0x0000100B
#define VP5_SPSERR_SPS_MAX_NUM_REORDER_PICS                           0x0000100C
#define VP5_SPSERR_SPS_MAX_LATENCY_INCREASE                           0x0000100D
#define VP5_SPSERR_LOG2_MIN_LUMA_CODING_BLOCK_SIZE_MINUS3             0x0000100E
#define VP5_SPSERR_LOG2_DIFF_MAX_MIN_LUMA_CODING_BLOCK_SIZE           0x0000100F
#define VP5_SPSERR_LOG2_MIN_TRANSFORM_BLOCK_SIZE_MINUS2               0x00001010
#define VP5_SPSERR_LOG2_DIFF_MAX_MIN_TRANSFORM_BLOCK_SIZE             0x00001011
#define VP5_SPSERR_MAX_TRANSFORM_HIERARCHY_DEPTH_INTER                0x00001012
#define VP5_SPSERR_MAX_TRANSFORM_HIERARCHY_DEPTH_INTRA                0x00001013
#define VP5_SPSERR_SCALING_LIST                                       0x00001014
#define VP5_SPSERR_LOG2_DIFF_MIN_PCM_LUMA_CODING_BLOCK_SIZE_MINUS3    0x00001015
#define VP5_SPSERR_LOG2_DIFF_MAX_MIN_PCM_LUMA_CODING_BLOCK_SIZE       0x00001016
#define VP5_SPSERR_NUM_SHORT_TERM_REF_PIC_SETS                        0x00001017
#define VP5_SPSERR_NUM_LONG_TERM_REF_PICS_SPS                         0x00001018
#define VP5_SPSERR_GBU_PARSING_ERROR                                  0x00001019
#define VP5_SPSERR_RANGE_EXTENSION_FLAG                               0x0000101A
#define VP5_SPSERR_VUI_ERROR                                          0x0000101B
#define VP5_SPSERR_ACTIVATE_SPS                                       0x0000101C
#define VP5_PPSERR_PPS_PIC_PARAMETER_SET_ID                           0x00002000
#define VP5_PPSERR_PPS_SEQ_PARAMETER_SET_ID                           0x00002001
#define VP5_PPSERR_NUM_REF_IDX_L0_DEFAULT_ACTIVE_MINUS1               0x00002002
#define VP5_PPSERR_NUM_REF_IDX_L1_DEFAULT_ACTIVE_MINUS1               0x00002003
#define VP5_PPSERR_INIT_QP_MINUS26                                    0x00002004
#define VP5_PPSERR_DIFF_CU_QP_DELTA_DEPTH                             0x00002005
#define VP5_PPSERR_PPS_CB_QP_OFFSET                                   0x00002006
#define VP5_PPSERR_PPS_CR_QP_OFFSET                                   0x00002007
#define VP5_PPSERR_NUM_TILE_COLUMNS_MINUS1                            0x00002008
#define VP5_PPSERR_NUM_TILE_ROWS_MINUS1                               0x00002009
#define VP5_PPSERR_COLUMN_WIDTH_MINUS1                                0x0000200A
#define VP5_PPSERR_ROW_HEIGHT_MINUS1                                  0x0000200B
#define VP5_PPSERR_PPS_BETA_OFFSET_DIV2                               0x0000200C
#define VP5_PPSERR_PPS_TC_OFFSET_DIV2                                 0x0000200D
#define VP5_PPSERR_SCALING_LIST                                       0x0000200E
#define VP5_PPSERR_LOG2_PARALLEL_MERGE_LEVEL_MINUS2                   0x0000200F
#define VP5_PPSERR_NUM_TILE_COLUMNS_RANGE_OUT                         0x00002010
#define VP5_PPSERR_NUM_TILE_ROWS_RANGE_OUT                            0x00002011
#define VP5_PPSERR_MORE_RBSP_DATA_ERROR                               0x00002012
#define VP5_PPSERR_PPS_PIC_PARAMETER_SET_ID_RANGE_OUT                 0x00002013
#define VP5_PPSERR_PPS_SEQ_PARAMETER_SET_ID_RANGE_OUT                 0x00002014
#define VP5_PPSERR_NUM_REF_IDX_L0_DEFAULT_ACTIVE_MINUS1_RANGE_OUT     0x00002015
#define VP5_PPSERR_NUM_REF_IDX_L1_DEFAULT_ACTIVE_MINUS1_RANGE_OUT     0x00002016
#define VP5_PPSERR_PPS_CB_QP_OFFSET_RANGE_OUT                         0x00002017
#define VP5_PPSERR_PPS_CR_QP_OFFSET_RANGE_OUT                         0x00002018
#define VP5_PPSERR_COLUMN_WIDTH_MINUS1_RANGE_OUT                      0x00002019
#define VP5_PPSERR_ROW_HEIGHT_MINUS1_RANGE_OUT                        0x00002020
#define VP5_PPSERR_PPS_BETA_OFFSET_DIV2_RANGE_OUT                     0x00002021
#define VP5_PPSERR_PPS_TC_OFFSET_DIV2_RANGE_OUT                       0x00002022
#define VP5_SHERR_SLICE_PIC_PARAMETER_SET_ID                          0x00003000
#define VP5_SHERR_ACTIVATE_PPS                                        0x00003001
#define VP5_SHERR_ACTIVATE_SPS                                        0x00003002
#define VP5_SHERR_SLICE_TYPE                                          0x00003003
#define VP5_SHERR_FIRST_SLICE_IS_DEPENDENT_SLICE                      0x00003004
#define VP5_SHERR_SHORT_TERM_REF_PIC_SET_SPS_FLAG                     0x00003005
#define VP5_SHERR_SHORT_TERM_REF_PIC_SET                              0x00003006
#define VP5_SHERR_SHORT_TERM_REF_PIC_SET_IDX                          0x00003007
#define VP5_SHERR_NUM_LONG_TERM_SPS                                   0x00003008
#define VP5_SHERR_NUM_LONG_TERM_PICS                                  0x00003009
#define VP5_SHERR_LT_IDX_SPS_IS_OUT_OF_RANGE                          0x0000300A
#define VP5_SHERR_DELTA_POC_MSB_CYCLE_LT                              0x0000300B
#define VP5_SHERR_NUM_REF_IDX_L0_ACTIVE_MINUS1                        0x0000300C
#define VP5_SHERR_NUM_REF_IDX_L1_ACTIVE_MINUS1                        0x0000300D
#define VP5_SHERR_COLLOCATED_REF_IDX                                  0x0000300E
#define VP5_SHERR_PRED_WEIGHT_TABLE                                   0x0000300F
#define VP5_SHERR_FIVE_MINUS_MAX_NUM_MERGE_CAND                       0x00003010
#define VP5_SHERR_SLICE_QP_DELTA                                      0x00003011
#define VP5_SHERR_SLICE_QP_DELTA_IS_OUT_OF_RANGE                      0x00003012
#define VP5_SHERR_SLICE_CB_QP_OFFSET                                  0x00003013
#define VP5_SHERR_SLICE_CR_QP_OFFSET                                  0x00003014
#define VP5_SHERR_SLICE_BETA_OFFSET_DIV2                              0x00003015
#define VP5_SHERR_SLICE_TC_OFFSET_DIV2                                0x00003016
#define VP5_SHERR_NUM_ENTRY_POINT_OFFSETS                             0x00003017
#define VP5_SHERR_OFFSET_LEN_MINUS1                                   0x00003018
#define VP5_SHERR_SLICE_SEGMENT_HEADER_EXTENSION_LENGTH               0x00003019
#define VP5_SPECERR_OVER_PICTURE_WIDTH_SIZE                           0x00004000
#define VP5_SPECERR_OVER_PICTURE_HEIGHT_SIZE                          0x00004001
#define VP5_SPECERR_OVER_CHROMA_FORMAT                                0x00004002
#define VP5_SPECERR_OVER_BIT_DEPTH                                    0x00004003
#define VP5_ETCERR_INIT_SEQ_SPS_NOT_FOUND                             0x00005000
#define VP5_ETCERR_DEC_PIC_VCL_NOT_FOUND                              0x00005001
#define VP5_ETCERR_NO_VALID_SLICE_IN_AU                               0x00005002

/************************************************************************/
/* VP5 WARNING ON DECODER (WARN_INFO)                                 */
/************************************************************************/
#define VP5_SPSWARN_MAX_SUB_LAYERS_MINUS1                             0x00000001
#define VP5_SPSWARN_GENERAL_RESERVED_ZERO_44BITS                      0x00000002
#define VP5_SPSWARN_RESERVED_ZERO_2BITS                               0x00000004
#define VP5_SPSWARN_SUB_LAYER_RESERVED_ZERO_44BITS                    0x00000008
#define VP5_SPSWARN_GENERAL_LEVEL_IDC                                 0x00000010
#define VP5_SPSWARN_SPS_MAX_DEC_PIC_BUFFERING_VALUE_OVER              0x00000020
#define VP5_SPSWARN_RBSP_TRAILING_BITS                                0x00000040
#define VP5_SPSWARN_ST_RPS_UE_ERROR                                   0x00000080
#define VP5_PPSWARN_RBSP_TRAILING_BITS                                0x00000100
#define VP5_SHWARN_FIRST_SLICE_SEGMENT_IN_PIC_FLAG                    0x00001000
#define VP5_SHWARN_NO_OUTPUT_OF_PRIOR_PICS_FLAG                       0x00002000
#define VP5_SHWARN_PIC_OUTPUT_FLAG                                    0x00004000
#define VP5_ETCWARN_INIT_SEQ_VCL_NOT_FOUND                            0x00010000
#define VP5_ETCWARN_MISSING_REFERENCE_PICTURE                         0x00020000
#define VP5_ETCWARN_WRONG_TEMPORAL_ID                                 0x00040000
#define VP5_ETCWARN_ERROR_PICTURE_IS_REFERENCED                       0x00080000
#define VP5_SPECWARN_OVER_PROFILE                                     0x00100000
#define VP5_SPECWARN_OVER_LEVEL                                       0x00200000

#endif /* ERROR_CODE_H_INCLUDED */

