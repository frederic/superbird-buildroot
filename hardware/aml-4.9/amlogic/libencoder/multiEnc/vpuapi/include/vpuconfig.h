
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
#ifndef _VPU_CONFIG_H_
#define _VPU_CONFIG_H_

#define ENC_STREAM_BUF_SIZE  0x200000
#define ENC_STREAM_BUF_COUNT 4

#define VP512_CODE                    0x5120
#define VP515_CODE                    0x5150

#define VP511_CODE                    0x5110
#define VP521_CODE                    0x5210
#define VP521C_CODE                   0x521c

#define PRODUCT_CODE_VP_SERIES(x) (x == VP512_CODE || x == VP515_CODE || x == VP511_CODE || x == VP521_CODE || x == VP521C_CODE)

#define VP5_MAX_CODE_BUF_SIZE             (1024*1024)
#define VP521ENC_WORKBUF_SIZE             (128*1024)      //HEVC 128K, AVC 40K

#define MAX_INST_HANDLE_SIZE            48              /* DO NOT CHANGE THIS VALUE */
#define MAX_NUM_INSTANCE                6
#define MAX_NUM_VPU_CORE                1
#define MAX_NUM_VCORE                   1

#define MAX_ENC_AVC_PIC_WIDTH           4096
#define MAX_ENC_AVC_PIC_HEIGHT          2304
#define MAX_ENC_PIC_WIDTH               4096
#define MAX_ENC_PIC_HEIGHT              2304
#define MIN_ENC_PIC_WIDTH               96
#define MIN_ENC_PIC_HEIGHT              16

// for VPU420
#define VP_MIN_ENC_PIC_WIDTH            256
#define VP_MIN_ENC_PIC_HEIGHT           128
#define VP_MAX_ENC_PIC_WIDTH            8192
#define VP_MAX_ENC_PIC_HEIGHT           8192

#define MAX_CTU_NUM                     0x4000      // CTU num for max resolution = 8192x8192/(64x64)
#define MAX_SUB_CTU_NUM	                (MAX_CTU_NUM*4)
#define MAX_MB_NUM                      0x40000     // MB num for max resolution = 8192x8192/(16x16)

//  Application specific configuration
#define VPU_ENC_TIMEOUT                 60000
#define VPU_DEC_TIMEOUT                 20000
#define VPU_BUSY_CHECK_TIMEOUT          10000

// codec specific configuration
#define VPU_REORDER_ENABLE              1   // it can be set to 1 to handle reordering DPB in host side.
#define CBCR_INTERLEAVE			        1 //[default 1 for BW checking with CnMViedo Conformance] 0 (chroma separate mode), 1 (chroma interleave mode) // if the type of tiledmap uses the kind of MB_RASTER_MAP. must set to enable CBCR_INTERLEAVE
#define VPU_ENABLE_BWB			        1

#define HOST_ENDIAN                     VDI_128BIT_LITTLE_ENDIAN
#define VPU_FRAME_ENDIAN                HOST_ENDIAN
#define VPU_STREAM_ENDIAN               HOST_ENDIAN
#define VPU_USER_DATA_ENDIAN            HOST_ENDIAN
#define VPU_SOURCE_ENDIAN               HOST_ENDIAN
#define DRAM_BUS_WIDTH                  16


// for VPU encoder
#define USE_SRC_PRP_AXI         0
#define USE_SRC_PRI_AXI         1
#define DEFAULT_SRC_AXI         USE_SRC_PRP_AXI

/************************************************************************/
/* VPU COMMON MEMORY                                                    */
/************************************************************************/
#define VLC_BUF_NUM              (3)
#define COMMAND_QUEUE_DEPTH             (6)

#define ENC_SRC_BUF_NUM             (8+COMMAND_QUEUE_DEPTH)          //!< case of GOPsize = 8 (IBBBBBBBP), max src buffer num  = 12

#define ONE_TASKBUF_SIZE_FOR_VP5DEC_CQ         (8*1024*1024)   /* upto 8Kx4K, need 8Mbyte per task*/
#define ONE_TASKBUF_SIZE_FOR_VP5ENC_CQ         (8*1024*1024)  /* upto 8Kx8K, need 8Mbyte per task.*/
#define ONE_TASKBUF_SIZE_FOR_VP511DEC_CQ       (8*1024*1024)  /* upto 8Kx8K, need 8Mbyte per task.*/

/* FW will return one TASKBUF size base on MaxCPB (according to the SPEC), but this size will be quite big depend on profile/level.*/
/* Thus, if host can set size limitation for one TASKBUF size. (but, small size limitation can cause processing error)  */
#define ONE_TASKBUF_MAX_SIZE_LIMIT_DEC          (8*1024*1024)
#define ONE_TASKBUF_MAX_SIZE_LIMIT_ENC          (20*1024*1024)

#define ONE_TASKBUF_SIZE_FOR_CQ     (0)
#define SIZE_COMMON                 (2*1024*1024)

//=====4. VPU REPORT MEMORY  ======================//
#define SIZE_REPORT_BUF                 (0x10000)

#define STREAM_END_SIZE                 0
#define STREAM_END_SET_FLAG             0
#define STREAM_END_CLEAR_FLAG           -1
#define EXPLICIT_END_SET_FLAG           -2

#define UPDATE_NEW_BS_BUF               0


#define USE_BIT_INTERNAL_BUF            1
#define USE_IP_INTERNAL_BUF             1
#define USE_DBKY_INTERNAL_BUF           1
#define USE_DBKC_INTERNAL_BUF           1
#define USE_OVL_INTERNAL_BUF            1
#define USE_BTP_INTERNAL_BUF            1
#define USE_ME_INTERNAL_BUF             1

/* VPU410 only */
#define USE_BPU_INTERNAL_BUF            1
#define USE_VCE_IP_INTERNAL_BUF         1
#define USE_VCE_LF_ROW_INTERNAL_BUF     1

/* VPU420 only */
#define USE_IMD_INTERNAL_BUF            1
#define USE_RDO_INTERNAL_BUF            1
#define USE_LF_INTERNAL_BUF             1


#define VP5_UPPER_PROC_AXI_ID     0x0

#define VP5_PROC_AXI_ID           0x0
#define VP5_PRP_AXI_ID            0x0
#define VP5_FBD_Y_AXI_ID          0x0
#define VP5_FBC_Y_AXI_ID          0x0
#define VP5_FBD_C_AXI_ID          0x0
#define VP5_FBC_C_AXI_ID          0x0
#define VP5_SEC_AXI_ID            0x0
#define VP5_PRI_AXI_ID            0x0

#endif  /* _VPU_CONFIG_H_ */

