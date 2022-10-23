
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
#ifndef __ENC_REGISTER_DEFINE_H__
#define __ENC_REGISTER_DEFINE_H__

typedef enum {
    VP5_INIT_VPU        = 0x0001,
    VP5_WAKEUP_VPU      = 0x0002,
    VP5_SLEEP_VPU       = 0x0004,
    VP5_CREATE_INSTANCE = 0x0008,            /* queuing command */
    VP5_FLUSH_INSTANCE  = 0x0010,
    VP5_DESTROY_INSTANCE= 0x0020,            /* queuing command */
    VP5_INIT_SEQ        = 0x0040,            /* queuing command */
    VP5_SET_FB          = 0x0080,
    VP5_DEC_PIC         = 0x0100,            /* queuing command */
    VP5_ENC_PIC         = 0x0100,            /* queuing command */
    VP5_ENC_SET_PARAM   = 0x0200,            /* queuing command */
    VP5_QUERY           = 0x4000,
    VP5_UPDATE_BS       = 0x8000,
    VP5_RESET_VPU	   = 0x10000,
    VP5_MAX_VPU_COMD	= 0x10000,
} VP5_VPU_COMMAND;

typedef enum {
    GET_VPU_INFO        = 0,
    SET_WRITE_PROT      = 1,
    GET_RESULT          = 2,
    UPDATE_DISP_FLAG    = 3,
    GET_BW_REPORT       = 4,
    GET_BS_RD_PTR       = 5,    // for decoder
    GET_BS_WR_PTR       = 6,    // for encoder
    GET_SRC_BUF_FLAG    = 7,    // [FIX ME] better move to 8
    GET_DEBUG_INFO      = 0x61,
} QUERY_OPT;

#define VP5_REG_BASE                     0x00000000
#define VP5_CMD_REG_BASE                 0x00000100
#define VP5_CMD_REG_END                  0x00000200




/*
 * Common
 */
/* Power On Configuration
 * PO_DEBUG_MODE    [0]     1 - Power On with debug mode
 * USE_PO_CONF      [3]     1 - Use Power-On-Configuration
 */
#define VP5_PO_CONF                     (VP5_REG_BASE + 0x0000)
#define VP5_VCPU_CUR_PC                 (VP5_REG_BASE + 0x0004)
#define VP5_VCPU_CUR_LR                 (VP5_REG_BASE + 0x0008)
#define VP5_VPU_PDBG_STEP_MASK_V        (VP5_REG_BASE + 0x000C)
#define VP5_VPU_PDBG_CTRL               (VP5_REG_BASE + 0x0010)         // vCPU debugger ctrl register
#define VP5_VPU_PDBG_IDX_REG            (VP5_REG_BASE + 0x0014)         // vCPU debugger index register
#define VP5_VPU_PDBG_WDATA_REG          (VP5_REG_BASE + 0x0018)         // vCPU debugger write data register
#define VP5_VPU_PDBG_RDATA_REG          (VP5_REG_BASE + 0x001C)         // vCPU debugger read data register

#define VP5_VPU_FIO_CTRL_ADDR           (VP5_REG_BASE + 0x0020)
#define VP5_VPU_FIO_DATA                (VP5_REG_BASE + 0x0024)
#define VP5_VPU_VINT_REASON_USR         (VP5_REG_BASE + 0x0030)
#define VP5_VPU_VINT_REASON_CLR         (VP5_REG_BASE + 0x0034)
#define VP5_VPU_HOST_INT_REQ            (VP5_REG_BASE + 0x0038)
#define VP5_VPU_VINT_CLEAR              (VP5_REG_BASE + 0x003C)
#define VP5_VPU_HINT_CLEAR              (VP5_REG_BASE + 0x0040)
#define VP5_VPU_VPU_INT_STS             (VP5_REG_BASE + 0x0044)
#define VP5_VPU_VINT_ENABLE             (VP5_REG_BASE + 0x0048)
#define VP5_VPU_VINT_REASON             (VP5_REG_BASE + 0x004C)
#define VP5_VPU_RESET_REQ               (VP5_REG_BASE + 0x0050)
#define VP5_RST_BLOCK_CCLK(_core)       (1<<_core)
#define VP5_RST_BLOCK_CCLK_ALL          (0xff)
#define VP5_RST_BLOCK_BCLK(_core)       (0x100<<_core)
#define VP5_RST_BLOCK_BCLK_ALL          (0xff00)
#define VP5_RST_BLOCK_ACLK(_core)       (0x10000<<_core)
#define VP5_RST_BLOCK_ACLK_ALL          (0xff0000)
#define VP5_RST_BLOCK_VCPU              (0x1000000)
#define VP5_RST_BLOCK_VCLK              (0x2000000)
#define VP5_RST_BLOCK_MCLK              (0x4000000)
#define VP5_RST_BLOCK_ALL               (0xfffffff)
#define VP5_VPU_RESET_STATUS            (VP5_REG_BASE + 0x0054)

#define VP5_VCPU_RESTART                 (VP5_REG_BASE + 0x0058)
#define VP5_VPU_CLK_MASK                 (VP5_REG_BASE + 0x005C)


/* REMAP_CTRL
 * PAGE SIZE:   [8:0]   0x001 - 4K
 *                      0x002 - 8K
 *                      0x004 - 16K
 *                      ...
 *                      0x100 - 1M
 * REGION ATTR1 [10]    0     - Normal
 *                      1     - Make Bus error for the region
 * REGION ATTR2 [11]    0     - Normal
 *                      1     - Bypass region
 * REMAP INDEX  [15:12]       - 0 ~ 3 , 0 - Code, 1-Stack
 * ENDIAN       [19:16]       - See EndianMode in vdi.h
 * AXI-ID       [23:20]       - Upper AXI-ID
 * BUS_ERROR    [29]    0     - bypass
 *                      1     - Make BUS_ERROR for unmapped region
 * BYPASS_ALL   [30]    1     - Bypass all
 * ENABLE       [31]    1     - Update control register[30:16]
 */
enum {
    VP5_REMAP_CODE_INDEX=0,
};
#define VP5_VPU_REMAP_CTRL                       (VP5_REG_BASE + 0x0060)
#define VP5_VPU_REMAP_VADDR                      (VP5_REG_BASE + 0x0064)
#define VP5_VPU_REMAP_PADDR                      (VP5_REG_BASE + 0x0068)
#define VP5_VPU_REMAP_CORE_START                 (VP5_REG_BASE + 0x006C)
#define VP5_VPU_BUSY_STATUS                      (VP5_REG_BASE + 0x0070)
#define VP5_VPU_HALT_STATUS                      (VP5_REG_BASE + 0x0074)
#define VP5_VPU_VCPU_STATUS                      (VP5_REG_BASE + 0x0078)
#define VP5_VPU_PRESCAN_STATUS                   (VP5_REG_BASE + 0x007C)
#define VP5_VPU_RET_PRODUCT_VERSION              (VP5_REG_BASE + 0x0094)
/*
    assign vpu_config0          = {conf_map_converter_reg,      // [31]
    conf_map_converter_sig,         // [30]
    8'd0,                        // [29:22]
    conf_std_switch_en,          // [21]
    conf_bg_detect,              // [20]
    conf_3dnr_en,                // [19]
    conf_one_axi_en,             // [18]
    conf_sec_axi_en,             // [17]
    conf_bus_info,               // [16]
    conf_afbc_en,                // [15]
    conf_afbc_version_id,        // [14:12]
    conf_fbc_en,                 // [11]
    conf_fbc_version_id,         // [10:08]
    conf_scaler_en,              // [07]
    conf_scaler_version_id,      // [06:04]
    conf_bwb_en,                 // [03]
    3'd0};                       // [02:00]
*/
#define VP5_VPU_RET_VPU_CONFIG0                  (VP5_REG_BASE + 0x0098)
/*
    assign vpu_config1          = {4'd0,                        // [31:28]
    conf_perf_timer_en,          // [27]
    conf_multi_core_en,          // [26]
    conf_gcu_en,                 // [25]
    conf_cu_report,              // [24]
    4'd0,                        // [23:20]
    conf_vcore_id_3,             // [19]
    conf_vcore_id_2,             // [18]
    conf_vcore_id_1,             // [17]
    conf_vcore_id_0,             // [16]
    conf_bwb_opt,                // [15]
    7'd0,                        // [14:08]
    conf_cod_std_en_reserved_7,  // [7]
    conf_cod_std_en_reserved_6,  // [6]
    conf_cod_std_en_reserved_5,  // [5]
    conf_cod_std_en_reserved_4,  // [4]
    conf_cod_std_en_reserved_3,  // [3]
    conf_cod_std_en_reserved_2,  // [2]
    conf_cod_std_en_vp9,         // [1]
    conf_cod_std_en_hevc};       // [0]
    }
*/
#define VP5_VPU_RET_VPU_CONFIG1                  (VP5_REG_BASE + 0x009C)

#define VP5_VPU_DBG_REG0							(VP5_REG_BASE + 0x00f0)
#define VP5_VPU_DBG_REG1							(VP5_REG_BASE + 0x00f4)
#define VP5_VPU_DBG_REG2							(VP5_REG_BASE + 0x00f8)
#define VP5_VPU_DBG_REG3							(VP5_REG_BASE + 0x00fc)

#ifdef SUPPORT_SW_UART_V2
#define VP5_SW_UART_STATUS						VP5_VPU_DBG_REG0
#define VP5_SW_UART_TX_DATA						VP5_VPU_DBG_REG1
#endif



/************************************************************************/
/* PRODUCT INFORMATION                                                  */
/************************************************************************/
#define VP5_PRODUCT_NAME                     (VP5_REG_BASE + 0x1040)
#define VP5_PRODUCT_NUMBER                   (VP5_REG_BASE + 0x1044)

/************************************************************************/
/* DECODER/ENCODER COMMON                                               */
/************************************************************************/
#define VP5_COMMAND                              (VP5_REG_BASE + 0x0100)
#define VP5_COMMAND_OPTION                       (VP5_REG_BASE + 0x0104)
#define VP5_QUERY_OPTION                         (VP5_REG_BASE + 0x0104)
#define VP5_RET_SUCCESS                          (VP5_REG_BASE + 0x0108)
#define VP5_RET_FAIL_REASON                      (VP5_REG_BASE + 0x010C)
#define VP5_CMD_INSTANCE_INFO                    (VP5_REG_BASE + 0x0110)

#define VP5_RET_QUEUE_STATUS                     (VP5_REG_BASE + 0x01E0)
#define VP5_RET_BS_EMPTY_INST                    (VP5_REG_BASE + 0x01E4)
#define VP5_RET_QUEUE_CMD_DONE_INST              (VP5_REG_BASE + 0x01E8)
#define VP5_RET_STAGE0_INSTANCE_INFO             (VP5_REG_BASE + 0x01EC)
#define VP5_RET_STAGE1_INSTANCE_INFO             (VP5_REG_BASE + 0x01F0)
#define VP5_RET_STAGE2_INSTANCE_INFO             (VP5_REG_BASE + 0x01F4)

#define VP5_RET_SEQ_DONE_INSTANCE_INFO           (VP5_REG_BASE + 0x01FC)

#define VP5_BS_OPTION                            (VP5_REG_BASE + 0x0120)

#define VP5_CMD_CREATE_INST_SUB_FRAME_SYNC        (VP5_REG_BASE + 0x0128)

#define VP5_RET_VLC_BUF_SIZE                     (VP5_REG_BASE + 0x01B0)      // return info when QUERY (GET_RESULT) for en/decoder
#define VP5_RET_PARAM_BUF_SIZE                   (VP5_REG_BASE + 0x01B4)      // return info when QUERY (GET_RESULT) for en/decoder

#define VP5_CMD_SET_FB_ADDR_TASK_BUF             (VP5_REG_BASE + 0x01D4)      // set when SET_FB for en/decoder
#define VP5_CMD_SET_FB_TASK_BUF_SIZE             (VP5_REG_BASE + 0x01D8)

/************************************************************************/
/* INIT_VPU - COMMON                                                    */
/************************************************************************/
/* Note: VP5_ADDR_CODE_BASE should be aligned to 4KB */
#define VP5_ADDR_CODE_BASE                       (VP5_REG_BASE + 0x0110)
#define VP5_CODE_SIZE                            (VP5_REG_BASE + 0x0114)
#define VP5_CODE_PARAM                           (VP5_REG_BASE + 0x0118)
#define VP5_ADDR_TEMP_BASE                       (VP5_REG_BASE + 0x011C)
#define VP5_TEMP_SIZE                            (VP5_REG_BASE + 0x0120)
#define VP5_ADDR_SEC_AXI                         (VP5_REG_BASE + 0x0124)
#define VP5_SEC_AXI_SIZE                         (VP5_REG_BASE + 0x0128)
#define VP5_HW_OPTION                            (VP5_REG_BASE + 0x012C)
#define VP5_TIMEOUT_CNT                          (VP5_REG_BASE + 0x0130)
#define VP5_CMD_INIT_AXI_PARAM                   (VP5_REG_BASE + 0x017C)

/************************************************************************/
/* CREATE_INSTANCE - COMMON                                             */
/************************************************************************/
#define VP5_ADDR_WORK_BASE                       (VP5_REG_BASE + 0x0114)
#define VP5_WORK_SIZE                            (VP5_REG_BASE + 0x0118)
#define VP5_CMD_DEC_BS_START_ADDR                (VP5_REG_BASE + 0x011C)
#define VP5_CMD_DEC_BS_SIZE                      (VP5_REG_BASE + 0x0120)
#define VP5_CMD_BS_PARAM                         (VP5_REG_BASE + 0x0124)
#define VP5_CMD_EXT_ADDR_BASE                    (VP5_REG_BASE + 0x0138)
#define VP5_CMD_ENC_RING_BS_START_ADDR           (VP5_REG_BASE + 0x011C)
#define VP5_CMD_ENC_RING_BS_SIZE                 (VP5_REG_BASE + 0x0120)
#define VP5_CMD_NUM_CQ_DEPTH_M1                  (VP5_REG_BASE + 0x013C)

/************************************************************************/
/* DECODER - INIT_SEQ                                                   */
/************************************************************************/
#define VP5_BS_RD_PTR                            (VP5_REG_BASE + 0x0118)
#define VP5_BS_WR_PTR                            (VP5_REG_BASE + 0x011C)
/************************************************************************/
/* SET_FRAME_BUF                                                        */
/************************************************************************/
/* SET_FB_OPTION 0x00       REGISTER FRAMEBUFFERS
                 0x01       UPDATE FRAMEBUFFER, just one framebuffer(linear, fbc and mvcol)
 */
#define VP5_SFB_OPTION                           (VP5_REG_BASE + 0x0104)
#define VP5_COMMON_PIC_INFO                      (VP5_REG_BASE + 0x0118)
#define VP5_PIC_SIZE                             (VP5_REG_BASE + 0x011C)
#define VP5_SET_FB_NUM                           (VP5_REG_BASE + 0x0120)

#define VP5_ADDR_LUMA_BASE0                      (VP5_REG_BASE + 0x0134)
#define VP5_ADDR_CB_BASE0                        (VP5_REG_BASE + 0x0138)
#define VP5_ADDR_CR_BASE0                        (VP5_REG_BASE + 0x013C)
#define VP5_ADDR_FBC_Y_OFFSET0                   (VP5_REG_BASE + 0x013C)       // Compression offset table for Luma
#define VP5_ADDR_FBC_C_OFFSET0                   (VP5_REG_BASE + 0x0140)       // Compression offset table for Chroma
#define VP5_ADDR_LUMA_BASE1                      (VP5_REG_BASE + 0x0144)
#define VP5_ADDR_CB_ADDR1                        (VP5_REG_BASE + 0x0148)
#define VP5_ADDR_CR_ADDR1                        (VP5_REG_BASE + 0x014C)
#define VP5_ADDR_FBC_Y_OFFSET1                   (VP5_REG_BASE + 0x014C)       // Compression offset table for Luma
#define VP5_ADDR_FBC_C_OFFSET1                   (VP5_REG_BASE + 0x0150)       // Compression offset table for Chroma
#define VP5_ADDR_LUMA_BASE2                      (VP5_REG_BASE + 0x0154)
#define VP5_ADDR_CB_ADDR2                        (VP5_REG_BASE + 0x0158)
#define VP5_ADDR_CR_ADDR2                        (VP5_REG_BASE + 0x015C)
#define VP5_ADDR_FBC_Y_OFFSET2                   (VP5_REG_BASE + 0x015C)       // Compression offset table for Luma
#define VP5_ADDR_FBC_C_OFFSET2                   (VP5_REG_BASE + 0x0160)       // Compression offset table for Chroma
#define VP5_ADDR_LUMA_BASE3                      (VP5_REG_BASE + 0x0164)
#define VP5_ADDR_CB_ADDR3                        (VP5_REG_BASE + 0x0168)
#define VP5_ADDR_CR_ADDR3                        (VP5_REG_BASE + 0x016C)
#define VP5_ADDR_FBC_Y_OFFSET3                   (VP5_REG_BASE + 0x016C)       // Compression offset table for Luma
#define VP5_ADDR_FBC_C_OFFSET3                   (VP5_REG_BASE + 0x0170)       // Compression offset table for Chroma
#define VP5_ADDR_LUMA_BASE4                      (VP5_REG_BASE + 0x0174)
#define VP5_ADDR_CB_ADDR4                        (VP5_REG_BASE + 0x0178)
#define VP5_ADDR_CR_ADDR4                        (VP5_REG_BASE + 0x017C)
#define VP5_ADDR_FBC_Y_OFFSET4                   (VP5_REG_BASE + 0x017C)       // Compression offset table for Luma
#define VP5_ADDR_FBC_C_OFFSET4                   (VP5_REG_BASE + 0x0180)       // Compression offset table for Chroma
#define VP5_ADDR_LUMA_BASE5                      (VP5_REG_BASE + 0x0184)
#define VP5_ADDR_CB_ADDR5                        (VP5_REG_BASE + 0x0188)
#define VP5_ADDR_CR_ADDR5                        (VP5_REG_BASE + 0x018C)
#define VP5_ADDR_FBC_Y_OFFSET5                   (VP5_REG_BASE + 0x018C)       // Compression offset table for Luma
#define VP5_ADDR_FBC_C_OFFSET5                   (VP5_REG_BASE + 0x0190)       // Compression offset table for Chroma
#define VP5_ADDR_LUMA_BASE6                      (VP5_REG_BASE + 0x0194)
#define VP5_ADDR_CB_ADDR6                        (VP5_REG_BASE + 0x0198)
#define VP5_ADDR_CR_ADDR6                        (VP5_REG_BASE + 0x019C)
#define VP5_ADDR_FBC_Y_OFFSET6                   (VP5_REG_BASE + 0x019C)       // Compression offset table for Luma
#define VP5_ADDR_FBC_C_OFFSET6                   (VP5_REG_BASE + 0x01A0)       // Compression offset table for Chroma
#define VP5_ADDR_LUMA_BASE7                      (VP5_REG_BASE + 0x01A4)
#define VP5_ADDR_CB_ADDR7                        (VP5_REG_BASE + 0x01A8)
#define VP5_ADDR_CR_ADDR7                        (VP5_REG_BASE + 0x01AC)
#define VP5_ADDR_FBC_Y_OFFSET7                   (VP5_REG_BASE + 0x01AC)       // Compression offset table for Luma
#define VP5_ADDR_FBC_C_OFFSET7                   (VP5_REG_BASE + 0x01B0)       // Compression offset table for Chroma
#define VP5_ADDR_MV_COL0                         (VP5_REG_BASE + 0x01B4)
#define VP5_ADDR_MV_COL1                         (VP5_REG_BASE + 0x01B8)
#define VP5_ADDR_MV_COL2                         (VP5_REG_BASE + 0x01BC)
#define VP5_ADDR_MV_COL3                         (VP5_REG_BASE + 0x01C0)
#define VP5_ADDR_MV_COL4                         (VP5_REG_BASE + 0x01C4)
#define VP5_ADDR_MV_COL5                         (VP5_REG_BASE + 0x01C8)
#define VP5_ADDR_MV_COL6                         (VP5_REG_BASE + 0x01CC)
#define VP5_ADDR_MV_COL7                         (VP5_REG_BASE + 0x01D0)

/* UPDATE_FB */
/* CMD_SET_FB_STRIDE [15:0]     - FBC framebuffer stride
                     [31:15]    - Linear framebuffer stride
 */
#define VP5_CMD_SET_FB_STRIDE                    (VP5_REG_BASE + 0x0118)
#define VP5_CMD_SET_FB_INDEX                     (VP5_REG_BASE + 0x0120)
#define VP5_ADDR_LUMA_BASE                       (VP5_REG_BASE + 0x0134)
#define VP5_ADDR_CB_BASE                         (VP5_REG_BASE + 0x0138)
#define VP5_ADDR_CR_BASE                         (VP5_REG_BASE + 0x013C)
#define VP5_ADDR_MV_COL                          (VP5_REG_BASE + 0x0140)
#define VP5_ADDR_FBC_Y_BASE                      (VP5_REG_BASE + 0x0144)
#define VP5_ADDR_FBC_C_BASE                      (VP5_REG_BASE + 0x0148)
#define VP5_ADDR_FBC_Y_OFFSET                    (VP5_REG_BASE + 0x014C)
#define VP5_ADDR_FBC_C_OFFSET                    (VP5_REG_BASE + 0x0150)

/************************************************************************/
/* DECODER - DEC_PIC                                                    */
/************************************************************************/
#define VP5_CMD_DEC_VCORE_INFO                  (VP5_REG_BASE + 0x0194)
/* Sequence change enable mask register
 * CMD_SEQ_CHANGE_ENABLE_FLAG [5]   profile_idc
 *                            [16]  pic_width/height_in_luma_sample
 *                            [19]  sps_max_dec_pic_buffering, max_num_reorder, max_latency_increase
 */
#define VP5_CMD_SEQ_CHANGE_ENABLE_FLAG           (VP5_REG_BASE + 0x0128)
#define VP5_CMD_DEC_USER_MASK                    (VP5_REG_BASE + 0x012C)
#define VP5_CMD_DEC_TEMPORAL_ID_PLUS1            (VP5_REG_BASE + 0x0130)
#define VP5_CMD_DEC_REL_TEMPORAL_ID              (VP5_REG_BASE + 0x0130)
#define VP5_CMD_DEC_FORCE_FB_LATENCY_PLUS1       (VP5_REG_BASE + 0x0134)
#define VP5_USE_SEC_AXI                          (VP5_REG_BASE + 0x0150)
#define VP5_CMD_DEC_QOS_PARAM                    (VP5_REG_BASE + 0x0180)

/************************************************************************/
/* DECODER - QUERY : GET_VPU_INFO                                       */
/************************************************************************/
#define VP5_RET_FW_VERSION                       (VP5_REG_BASE + 0x0118)
#define VP5_RET_PRODUCT_NAME                     (VP5_REG_BASE + 0x011C)
#define VP5_RET_PRODUCT_VERSION                  (VP5_REG_BASE + 0x0120)
#define VP5_RET_STD_DEF0                         (VP5_REG_BASE + 0x0124)
#define VP5_RET_STD_DEF1                         (VP5_REG_BASE + 0x0128)
#define VP5_RET_CONF_FEATURE                     (VP5_REG_BASE + 0x012C)
#define VP5_RET_CONF_DATE                        (VP5_REG_BASE + 0x0130)
#define VP5_RET_CONF_REVISION                    (VP5_REG_BASE + 0x0134)
#define VP5_RET_CONF_TYPE                        (VP5_REG_BASE + 0x0138)
#define VP5_RET_PRODUCT_ID                       (VP5_REG_BASE + 0x013C)
#define VP5_RET_CUSTOMER_ID                      (VP5_REG_BASE + 0x0140)


/************************************************************************/
/* DECODER - QUERY : GET_RESULT                                         */
/************************************************************************/
#define VP5_CMD_DEC_ADDR_REPORT_BASE         (VP5_REG_BASE + 0x0114)
#define VP5_CMD_DEC_REPORT_SIZE              (VP5_REG_BASE + 0x0118)
#define VP5_CMD_DEC_REPORT_PARAM             (VP5_REG_BASE + 0x011C)

#define VP5_RET_DEC_BS_RD_PTR                (VP5_REG_BASE + 0x011C)
#define VP5_RET_DEC_SEQ_PARAM                (VP5_REG_BASE + 0x0120)
#define VP5_RET_DEC_COLOR_SAMPLE_INFO        (VP5_REG_BASE + 0x0124)
#define VP5_RET_DEC_ASPECT_RATIO             (VP5_REG_BASE + 0x0128)
#define VP5_RET_DEC_BIT_RATE                 (VP5_REG_BASE + 0x012C)
#define VP5_RET_DEC_FRAME_RATE_NR            (VP5_REG_BASE + 0x0130)
#define VP5_RET_DEC_FRAME_RATE_DR            (VP5_REG_BASE + 0x0134)
#define VP5_RET_DEC_NUM_REQUIRED_FB          (VP5_REG_BASE + 0x0138)
#define VP5_RET_DEC_NUM_REORDER_DELAY        (VP5_REG_BASE + 0x013C)
#define VP5_RET_DEC_SUB_LAYER_INFO           (VP5_REG_BASE + 0x0140)
#define VP5_RET_DEC_NOTIFICATION             (VP5_REG_BASE + 0x0144)
#define VP5_RET_DEC_USERDATA_IDC             (VP5_REG_BASE + 0x0148)
#define VP5_RET_DEC_PIC_SIZE                 (VP5_REG_BASE + 0x014C)
#define VP5_RET_DEC_CROP_TOP_BOTTOM          (VP5_REG_BASE + 0x0150)
#define VP5_RET_DEC_CROP_LEFT_RIGHT          (VP5_REG_BASE + 0x0154)
#define VP5_RET_DEC_AU_START_POS             (VP5_REG_BASE + 0x0158)
#define VP5_RET_DEC_AU_END_POS               (VP5_REG_BASE + 0x015C)
#define VP5_RET_DEC_PIC_TYPE                 (VP5_REG_BASE + 0x0160)
#define VP5_RET_DEC_PIC_POC                  (VP5_REG_BASE + 0x0164)
#define VP5_RET_DEC_RECOVERY_POINT           (VP5_REG_BASE + 0x0168)
#define VP5_RET_DEC_DEBUG_INDEX              (VP5_REG_BASE + 0x016C)
#define VP5_RET_DEC_DECODED_INDEX            (VP5_REG_BASE + 0x0170)
#define VP5_RET_DEC_DISPLAY_INDEX            (VP5_REG_BASE + 0x0174)
#define VP5_RET_DEC_REALLOC_INDEX            (VP5_REG_BASE + 0x0178)
#define VP5_RET_DEC_DISP_IDC                 (VP5_REG_BASE + 0x017C)
#define VP5_RET_DEC_ERR_CTB_NUM              (VP5_REG_BASE + 0x0180)

#define VP5_RET_DEC_HOST_CMD_TICK            (VP5_REG_BASE + 0x01B8)
#define VP5_RET_DEC_SEEK_START_TICK          (VP5_REG_BASE + 0x01BC)
#define VP5_RET_DEC_SEEK_END__TICK           (VP5_REG_BASE + 0x01C0)
#define VP5_RET_DEC_PARSING_START_TICK       (VP5_REG_BASE + 0x01C4)
#define VP5_RET_DEC_PARSING_END_TICK         (VP5_REG_BASE + 0x01C8)
#define VP5_RET_DEC_DECODING_START_TICK      (VP5_REG_BASE + 0x01CC)
#define VP5_RET_DEC_DECODING_ENC_TICK        (VP5_REG_BASE + 0x01D0)
#ifdef SUPPORT_SW_UART
#define VP5_SW_UART_STATUS					(VP5_REG_BASE + 0x01D4)
#define VP5_SW_UART_TX_DATA					(VP5_REG_BASE + 0x01D8)
//#define VP5_RET_DEC_WARN_INFO                (VP5_REG_BASE + 0x01D4)
//#define VP5_RET_DEC_ERR_INFO                 (VP5_REG_BASE + 0x01D8)
#else
#define VP5_RET_DEC_WARN_INFO                (VP5_REG_BASE + 0x01D4)
#define VP5_RET_DEC_ERR_INFO                 (VP5_REG_BASE + 0x01D8)
#endif
#define VP5_RET_DEC_DECODING_SUCCESS         (VP5_REG_BASE + 0x01DC)

/************************************************************************/
/* DECODER - FLUSH_INSTANCE                                             */
/************************************************************************/
#define VP5_CMD_FLUSH_INST_OPT               (VP5_REG_BASE + 0x104)

/************************************************************************/
/* DECODER - QUERY : UPDATE_DISP_FLAG                                   */
/************************************************************************/
#define VP5_CMD_DEC_SET_DISP_IDC             (VP5_REG_BASE + 0x0118)
#define VP5_CMD_DEC_CLR_DISP_IDC             (VP5_REG_BASE + 0x011C)

/************************************************************************/
/* DECODER - QUERY : GET_BS_RD_PTR                                      */
/************************************************************************/
#define VP5_RET_QUERY_DEC_BS_RD_PTR          (VP5_REG_BASE + 0x011C)

/************************************************************************/
/* QUERY : GET_DEBUG_INFO                                               */
/************************************************************************/
#define VP5_RET_QUERY_DEBUG_PRI_REASON       (VP5_REG_BASE + 0x114)

/************************************************************************/
/* GDI register for Debugging                                           */
/************************************************************************/
#define VP5_GDI_BASE                         0x8800
#define VP5_GDI_BUS_CTRL                     (VP5_GDI_BASE + 0x0F0)
#define VP5_GDI_BUS_STATUS                   (VP5_GDI_BASE + 0x0F4)

#define VP5_BACKBONE_BASE_VCPU               0xFE00
#define VP5_BACKBONE_PROG_AXI_ID             (VP5_BACKBONE_BASE_VCPU + 0x00C)
#define VP5_BACKBONE_QOS_PROC_R_CH_PRIOR     (VP5_BACKBONE_BASE_VCPU + 0x064)
#define VP5_BACKBONE_QOS_PROC_W_CH_PRIOR     (VP5_BACKBONE_BASE_VCPU + 0x06C)
#define VP5_BACKBONE_PROC_EXT_ADDR           (VP5_BACKBONE_BASE_VCPU + 0x0C0)
#define VP5_BACKBONE_AXI_PARAM               (VP5_BACKBONE_BASE_VCPU + 0x0E0)

#define VP5_COMBINED_BACKBONE_BASE           0xFE00
#define VP5_COMBINED_BACKBONE_BUS_CTRL       (VP5_COMBINED_BACKBONE_BASE + 0x010)
#define VP5_COMBINED_BACKBONE_BUS_STATUS     (VP5_COMBINED_BACKBONE_BASE + 0x014)

#define VP5_BACKBONE_BASE_VCORE0             0x8E00
#define VP5_BACKBONE_BUS_CTRL_VCORE0         (VP5_BACKBONE_BASE_VCORE0 + 0x010)
#define VP5_BACKBONE_BUS_STATUS_VCORE0       (VP5_BACKBONE_BASE_VCORE0 + 0x014)

#define VP5_BACKBONE_BASE_VCORE1             0x9E00  // for dual-core product
#define VP5_BACKBONE_BUS_CTRL_VCORE1         (VP5_BACKBONE_BASE_VCORE1 + 0x010)
#define VP5_BACKBONE_BUS_STATUS_VCORE1       (VP5_BACKBONE_BASE_VCORE1 + 0x014)

/************************************************************************/
/*                                                                      */
/*               For  ENCODER                                           */
/*                                                                      */
/************************************************************************/
#define VP5_RET_STAGE3_INSTANCE_INFO             (VP5_REG_BASE + 0x1F8)
/************************************************************************/
/* ENCODER - CREATE_INSTANCE                                            */
/************************************************************************/
// 0x114 ~ 0x124 : defined above (CREATE_INSTANCE COMMON)
#define VP5_CMD_ENC_VCORE_INFO                   (VP5_REG_BASE + 0x0194)
#define VP5_CMD_ENC_SRC_OPTIONS                  (VP5_REG_BASE + 0x0128)

/************************************************************************/
/* ENCODER - SET_FB                                                     */
/************************************************************************/
#define VP5_FBC_STRIDE                           (VP5_REG_BASE + 0x128)
#define VP5_ADDR_SUB_SAMPLED_FB_BASE             (VP5_REG_BASE + 0x12C)
#define VP5_SUB_SAMPLED_ONE_FB_SIZE              (VP5_REG_BASE + 0x130)

/************************************************************************/
/* ENCODER - ENC_SET_PARAM (COMMON & CHANGE_PARAM)                      */
/************************************************************************/
#define VP5_CMD_ENC_SEQ_SET_PARAM_OPTION         (VP5_REG_BASE + 0x104)
#define VP5_CMD_ENC_SEQ_SET_PARAM_ENABLE         (VP5_REG_BASE + 0x118)
#define VP5_CMD_ENC_SEQ_SRC_SIZE                 (VP5_REG_BASE + 0x11C)
#define VP5_CMD_ENC_SEQ_CUSTOM_MAP_ENDIAN        (VP5_REG_BASE + 0x120)
#define VP5_CMD_ENC_SEQ_SPS_PARAM                (VP5_REG_BASE + 0x124)
#define VP5_CMD_ENC_SEQ_PPS_PARAM                (VP5_REG_BASE + 0x128)
#define VP5_CMD_ENC_SEQ_GOP_PARAM                (VP5_REG_BASE + 0x12C)
#define VP5_CMD_ENC_SEQ_INTRA_PARAM              (VP5_REG_BASE + 0x130)
#define VP5_CMD_ENC_SEQ_CONF_WIN_TOP_BOT         (VP5_REG_BASE + 0x134)
#define VP5_CMD_ENC_SEQ_CONF_WIN_LEFT_RIGHT      (VP5_REG_BASE + 0x138)
#define VP5_CMD_ENC_SEQ_RDO_PARAM                (VP5_REG_BASE + 0x13C)
#define VP5_CMD_ENC_SEQ_INDEPENDENT_SLICE        (VP5_REG_BASE + 0x140)
#define VP5_CMD_ENC_SEQ_DEPENDENT_SLICE          (VP5_REG_BASE + 0x144)
#define VP5_CMD_ENC_SEQ_INTRA_REFRESH            (VP5_REG_BASE + 0x148)

#define VP5_CMD_ENC_SEQ_INPUT_SRC_PARAM          (VP5_REG_BASE + 0x14C)

#define VP5_CMD_ENC_SEQ_RC_FRAME_RATE            (VP5_REG_BASE + 0x150)
#define VP5_CMD_ENC_SEQ_RC_TARGET_RATE           (VP5_REG_BASE + 0x154)
#define VP5_CMD_ENC_SEQ_RC_PARAM                 (VP5_REG_BASE + 0x158)
#define VP5_CMD_ENC_SEQ_RC_MIN_MAX_QP            (VP5_REG_BASE + 0x15C)
#define VP5_CMD_ENC_SEQ_RC_BIT_RATIO_LAYER_0_3   (VP5_REG_BASE + 0x160)
#define VP5_CMD_ENC_SEQ_RC_BIT_RATIO_LAYER_4_7   (VP5_REG_BASE + 0x164)
#define VP5_CMD_ENC_SEQ_RC_INTER_MIN_MAX_QP      (VP5_REG_BASE + 0x168)
#define VP5_CMD_ENC_SEQ_RC_WEIGHT_PARAM          (VP5_REG_BASE + 0x16C)

#define VP5_CMD_ENC_SEQ_ROT_PARAM                (VP5_REG_BASE + 0x170)
#define VP5_CMD_ENC_SEQ_NUM_UNITS_IN_TICK        (VP5_REG_BASE + 0x174)
#define VP5_CMD_ENC_SEQ_TIME_SCALE               (VP5_REG_BASE + 0x178)
#define VP5_CMD_ENC_SEQ_NUM_TICKS_POC_DIFF_ONE   (VP5_REG_BASE + 0x17C)

#define VP5_CMD_ENC_SEQ_CUSTOM_MD_PU04           (VP5_REG_BASE + 0x184)
#define VP5_CMD_ENC_SEQ_CUSTOM_MD_PU08           (VP5_REG_BASE + 0x188)
#define VP5_CMD_ENC_SEQ_CUSTOM_MD_PU16           (VP5_REG_BASE + 0x18C)
#define VP5_CMD_ENC_SEQ_CUSTOM_MD_PU32           (VP5_REG_BASE + 0x190)
#define VP5_CMD_ENC_SEQ_CUSTOM_MD_CU08           (VP5_REG_BASE + 0x194)
#define VP5_CMD_ENC_SEQ_CUSTOM_MD_CU16           (VP5_REG_BASE + 0x198)
#define VP5_CMD_ENC_SEQ_CUSTOM_MD_CU32           (VP5_REG_BASE + 0x19C)
#define VP5_CMD_ENC_SEQ_NR_PARAM                 (VP5_REG_BASE + 0x1A0)
#define VP5_CMD_ENC_SEQ_NR_WEIGHT                (VP5_REG_BASE + 0x1A4)
#define VP5_CMD_ENC_SEQ_BG_PARAM                 (VP5_REG_BASE + 0x1A8)
#define VP5_CMD_ENC_SEQ_CUSTOM_LAMBDA_ADDR       (VP5_REG_BASE + 0x1AC)
#define VP5_CMD_ENC_SEQ_USER_SCALING_LIST_ADDR   (VP5_REG_BASE + 0x1B0)


/************************************************************************/
/* ENCODER - ENC_SET_PARAM (CUSTOM_GOP)                                 */
/************************************************************************/
#define VP5_CMD_ENC_CUSTOM_GOP_PARAM             (VP5_REG_BASE + 0x11C)
#define VP5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_0       (VP5_REG_BASE + 0x120)
#define VP5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_1       (VP5_REG_BASE + 0x124)
#define VP5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_2       (VP5_REG_BASE + 0x128)
#define VP5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_3       (VP5_REG_BASE + 0x12C)
#define VP5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_4       (VP5_REG_BASE + 0x130)
#define VP5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_5       (VP5_REG_BASE + 0x134)
#define VP5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_6       (VP5_REG_BASE + 0x138)
#define VP5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_7       (VP5_REG_BASE + 0x13C)

/************************************************************************/
/* ENCODER - ENC_PIC                                                    */
/************************************************************************/
#define VP5_CMD_ENC_BS_START_ADDR                (VP5_REG_BASE + 0x118)
#define VP5_CMD_ENC_BS_SIZE                      (VP5_REG_BASE + 0x11C)
#define VP5_CMD_ENC_PIC_USE_SEC_AXI              (VP5_REG_BASE + 0x124)
#define VP5_CMD_ENC_PIC_REPORT_PARAM             (VP5_REG_BASE + 0x128)
#define VP5_CMD_ENC_PIC_REPORT_ENDIAN            (VP5_REG_BASE + 0x12C)

#define VP5_CMD_ENC_PIC_CUSTOM_MAP_OPTION_PARAM  (VP5_REG_BASE + 0x138)
#define VP5_CMD_ENC_PIC_CUSTOM_MAP_OPTION_ADDR   (VP5_REG_BASE + 0x13C)

#define VP5_CMD_ENC_PIC_SRC_PIC_IDX              (VP5_REG_BASE + 0x144)
#define VP5_CMD_ENC_PIC_SRC_ADDR_Y               (VP5_REG_BASE + 0x148)
#define VP5_CMD_ENC_PIC_SRC_ADDR_U               (VP5_REG_BASE + 0x14C)
#define VP5_CMD_ENC_PIC_SRC_ADDR_V               (VP5_REG_BASE + 0x150)
#define VP5_CMD_ENC_PIC_SRC_STRIDE               (VP5_REG_BASE + 0x154)
#define VP5_CMD_ENC_PIC_SRC_FORMAT               (VP5_REG_BASE + 0x158)
#define VP5_CMD_ENC_PIC_SRC_AXI_SEL              (VP5_REG_BASE + 0x160)
#define VP5_CMD_ENC_PIC_CODE_OPTION              (VP5_REG_BASE + 0x164)
#define VP5_CMD_ENC_PIC_PIC_PARAM                (VP5_REG_BASE + 0x168)
#define VP5_CMD_ENC_PIC_LONGTERM_PIC             (VP5_REG_BASE + 0x16C)
#define VP5_CMD_ENC_PIC_WP_PIXEL_SIGMA_Y         (VP5_REG_BASE + 0x170)
#define VP5_CMD_ENC_PIC_WP_PIXEL_SIGMA_C         (VP5_REG_BASE + 0x174)
#define VP5_CMD_ENC_PIC_WP_PIXEL_MEAN_Y          (VP5_REG_BASE + 0x178)
#define VP5_CMD_ENC_PIC_WP_PIXEL_MEAN_C          (VP5_REG_BASE + 0x17C)
#define VP5_CMD_ENC_PIC_LF_PARAM_0               (VP5_REG_BASE + 0x180)
#define VP5_CMD_ENC_PIC_LF_PARAM_1               (VP5_REG_BASE + 0x184)
#define VP5_CMD_ENC_PIC_CF50_Y_OFFSET_TABLE_ADDR  (VP5_REG_BASE + 0x188)
#define VP5_CMD_ENC_PIC_CF50_CB_OFFSET_TABLE_ADDR (VP5_REG_BASE + 0x18C)
#define VP5_CMD_ENC_PIC_CF50_CR_OFFSET_TABLE_ADDR (VP5_REG_BASE + 0x190)
#define VP5_CMD_ENC_QOS_PARAM                     (VP5_REG_BASE + 0x198)
/************************************************************************/
/* ENCODER - QUERY (GET_RESULT)                                         */
/************************************************************************/

#define VP5_RET_ENC_NUM_REQUIRED_FB              (VP5_REG_BASE + 0x11C)
#define VP5_RET_ENC_MIN_SRC_BUF_NUM              (VP5_REG_BASE + 0x120)
#define VP5_RET_ENC_PIC_TYPE                     (VP5_REG_BASE + 0x124)
#define VP5_RET_ENC_PIC_POC                      (VP5_REG_BASE + 0x128)
#define VP5_RET_ENC_PIC_IDX                      (VP5_REG_BASE + 0x12C)
#define VP5_RET_ENC_PIC_SLICE_NUM                (VP5_REG_BASE + 0x130)
#define VP5_RET_ENC_PIC_SKIP                     (VP5_REG_BASE + 0x134)
#define VP5_RET_ENC_PIC_NUM_INTRA                (VP5_REG_BASE + 0x138)
#define VP5_RET_ENC_PIC_NUM_MERGE                (VP5_REG_BASE + 0x13C)

#define VP5_RET_ENC_PIC_NUM_SKIP                 (VP5_REG_BASE + 0x144)
#define VP5_RET_ENC_PIC_AVG_CTU_QP               (VP5_REG_BASE + 0x148)
#define VP5_RET_ENC_PIC_BYTE                     (VP5_REG_BASE + 0x14C)
#define VP5_RET_ENC_GOP_PIC_IDX                  (VP5_REG_BASE + 0x150)
#define VP5_RET_ENC_USED_SRC_IDX                 (VP5_REG_BASE + 0x154)
#define VP5_RET_ENC_PIC_NUM                      (VP5_REG_BASE + 0x158)
#define VP5_RET_ENC_VCL_NUT                      (VP5_REG_BASE + 0x15C)

#define VP5_RET_ENC_PIC_DIST_LOW                 (VP5_REG_BASE + 0x164)
#define VP5_RET_ENC_PIC_DIST_HIGH                (VP5_REG_BASE + 0x168)

#define VP5_RET_ENC_PIC_MAX_LATENCY_PICTURES     (VP5_REG_BASE + 0x16C)
#define VP5_RET_ENC_SVC_LAYER                    (VP5_REG_BASE + 0x170)


#define VP5_RET_QUERY_CORE_IDC                   (VP5_REG_BASE + 0x19C)

//#define VP5_RET_ENC_PREPARE_CYCLE                (VP5_REG_BASE + 0x1C0)
//#define VP5_RET_ENC_PROCESSING_CYCLE             (VP5_REG_BASE + 0x1C4)
//#define VP5_RET_ENC_ENCODING_CYCLE               (VP5_REG_BASE + 0x1C8)
#define RET_ENC_HOST_CMD_TICK                   (VP5_REG_BASE + 0x1B8)
#define RET_ENC_PREPARE_START_TICK              (VP5_REG_BASE + 0x1BC)
#define RET_ENC_PREPARE_END_TICK                (VP5_REG_BASE + 0x1C0)
#define RET_ENC_PROCESSING_START_TICK           (VP5_REG_BASE + 0x1C4)
#define RET_ENC_PROCESSING_END_TICK             (VP5_REG_BASE + 0x1C8)
#define RET_ENC_ENCODING_START_TICK             (VP5_REG_BASE + 0x1CC)
#define RET_ENC_ENCODING_END_TICK               (VP5_REG_BASE + 0x1D0)


#define VP5_RET_ENC_WARN_INFO                    (VP5_REG_BASE + 0x1D4)
#define VP5_RET_ENC_ERR_INFO                     (VP5_REG_BASE + 0x1D8)
#define VP5_RET_ENC_ENCODING_SUCCESS             (VP5_REG_BASE + 0x1DC)

#define VP5_RET_QUERY_REPORT_PARAM               (VP5_REG_BASE + 0x178)
#define VP5_CMD_QUERY_REPORT_BASE                (VP5_REG_BASE + 0x17C)

/************************************************************************/
/* ENCODER - QUERY (GET_BW_REPORT)                                      */
/************************************************************************/
#define VP5_RET_ENC_RD_PTR                       (VP5_REG_BASE + 0x114)
#define VP5_RET_ENC_WR_PTR                       (VP5_REG_BASE + 0x118)
#define VP5_CMD_ENC_REASON_SEL                   (VP5_REG_BASE + 0x11C)
/************************************************************************/
/* ENCODER - QUERY (GET_BW_REPORT)                                      */
/************************************************************************/
#define RET_QUERY_BW_PRP_AXI_READ               (VP5_REG_BASE + 0x118)
#define RET_QUERY_BW_PRP_AXI_WRITE              (VP5_REG_BASE + 0x11C)
#define RET_QUERY_BW_FBD_Y_AXI_READ             (VP5_REG_BASE + 0x120)
#define RET_QUERY_BW_FBC_Y_AXI_WRITE            (VP5_REG_BASE + 0x124)
#define RET_QUERY_BW_FBD_C_AXI_READ             (VP5_REG_BASE + 0x128)
#define RET_QUERY_BW_FBC_C_AXI_WRITE            (VP5_REG_BASE + 0x12C)
#define RET_QUERY_BW_PRI_AXI_READ               (VP5_REG_BASE + 0x130)
#define RET_QUERY_BW_PRI_AXI_WRITE              (VP5_REG_BASE + 0x134)
#define RET_QUERY_BW_SEC_AXI_READ               (VP5_REG_BASE + 0x138)
#define RET_QUERY_BW_SEC_AXI_WRITE              (VP5_REG_BASE + 0x13C)
#define RET_QUERY_BW_PROC_AXI_READ              (VP5_REG_BASE + 0x140)
#define RET_QUERY_BW_PROC_AXI_WRITE             (VP5_REG_BASE + 0x144)

/************************************************************************/
/* ENCODER - QUERY (GET_SRC_FLAG)                                       */
/************************************************************************/
#define VP5_RET_ENC_SRC_BUF_FLAG                 (VP5_REG_BASE + 0x18C)
#define VP5_RET_RELEASED_SRC_INSTANCE            (VP5_REG_BASE + 0x1EC)



#endif /* __ENC_REGISTER_DEFINE_H__ */

