//#define LOG_NDEBUG 0
#define LOG_TAG "AMLVHEVCENC"
#ifdef MAKEANDROID
#include <utils/Log.h>
#endif
#include <stdio.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "./include/AML_HEVCEncoder.h"
//#include "enc_api.h"

#include "./include/vdi.h"
#include "./include/common_regdefine.h"
#include "./include/wave4_regdefine.h"
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "./include/vputypes.h"
#include "./include/monet.h"

static int gol_endian = 0xf;
static int enable_roi = 0;
static int rotMirMode = 0;
static int src_format = 1;
static int reset_mode = 2;
static int reset_error = 0;

#define wave420l_align4(a)      ((((a)+3)>>2)<<2)
#define wave420l_align8(a)      ((((a)+7)>>3)<<3)
#define wave420l_align16(a)     ((((a)+15)>>4)<<4)
#define wave420l_align32(a)     ((((a)+31)>>5)<<5)
#define wave420l_align64(a)     ((((a)+63)>>6)<<6)
#define wave420l_align128(a)    ((((a)+127)>>7)<<7)
#define wave420l_align256(a)    ((((a)+255)>>8)<<8)

#define FIOWrite(CORE, ADDR, DATA)             vdi_fio_write_register(CORE, ADDR, DATA)
#define FIORead(CORE, ADDR)                        vdi_fio_read_register(CORE, ADDR)

#define SUPPORT_SCALE 1
extern s32 vdi_init(u32 core_idx);
extern s32 vdi_release(u32 core_idx);


#if SUPPORT_SCALE
#include "./libge2d/ge2d_port.h"
#include "./libge2d/aml_ge2d.h"

extern aml_ge2d_t amlge2d;
aml_ge2d_info_t ge2dinfo;

static int SRC1_PIXFORMAT = PIXEL_FORMAT_YCrCb_420_SP;
static int SRC2_PIXFORMAT = PIXEL_FORMAT_YCrCb_420_SP;
static int DST_PIXFORMAT = PIXEL_FORMAT_YCrCb_420_SP;
static bool INIT_GE2D = false;

static GE2DOP OP = AML_GE2D_STRETCHBLIT;
static int do_strechblit(aml_ge2d_info_t *pge2dinfo, AMVHEVCEncFrameIO *input)
{
    int ret = -1;
    char code = 0;
    VLOG(DEBUG, "do_strechblit test case:\n");
    pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[0].canvas_w = input->pitch; //SX_SRC1;
    pge2dinfo->src_info[0].canvas_h = input->height; //SY_SRC1;
    pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
    if ((input->scale_width != 0) && (input->scale_height !=0)) {
        pge2dinfo->dst_info.canvas_w = input->scale_width; //SX_DST;
        pge2dinfo->dst_info.canvas_h = input->scale_height; //SY_DST;
    } else {
        pge2dinfo->dst_info.canvas_w = input->pitch - input->crop_left - input->crop_right;
        pge2dinfo->dst_info.canvas_h = input->height - input->crop_top - input->crop_bottom;
    }
    pge2dinfo->dst_info.format = DST_PIXFORMAT;

    pge2dinfo->src_info[0].rect.x = input->crop_left ;
    pge2dinfo->src_info[0].rect.y = input->crop_top;
    pge2dinfo->src_info[0].rect.w = input->pitch - input->crop_left - input->crop_right;
    pge2dinfo->src_info[0].rect.h = input->height - input->crop_top - input->crop_bottom;
    pge2dinfo->dst_info.rect.x = 0;
    pge2dinfo->dst_info.rect.y = 0;
    if ((input->scale_width != 0) && (input->scale_height !=0)) {
        pge2dinfo->dst_info.rect.w = input->scale_width; //SX_DST;
        pge2dinfo->dst_info.rect.h = input->scale_height; //SY_DST;
    } else {
        pge2dinfo->dst_info.rect.w = input->pitch - input->crop_left - input->crop_right;
        pge2dinfo->dst_info.rect.h = input->height - input->crop_top - input->crop_bottom;
    }
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
    ret = aml_ge2d_process(pge2dinfo);
    return ret;
}

static void set_ge2dinfo(aml_ge2d_info_t *pge2dinfo, AMVHEVCEncParams *encParam)
{
    pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[0].canvas_w = encParam->src_width; //SX_SRC1;
    pge2dinfo->src_info[0].canvas_h = encParam->src_height; //SY_SRC1;
    pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
    pge2dinfo->src_info[1].memtype = GE2D_CANVAS_TYPE_INVALID;
    pge2dinfo->src_info[1].canvas_w = 0;
    pge2dinfo->src_info[1].canvas_h = 0;
    pge2dinfo->src_info[1].format = SRC2_PIXFORMAT;

    pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->dst_info.canvas_w = encParam->width; //SX_DST;
    pge2dinfo->dst_info.canvas_h = encParam->height; //SY_DST;
    pge2dinfo->dst_info.format = DST_PIXFORMAT;
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
    pge2dinfo->offset = 0;
    pge2dinfo->ge2d_op = OP;
    pge2dinfo->blend_mode = BLEND_MODE_PREMULTIPLIED;
}
#endif

void PrintVpuStatus(u32 coreIdx) {
    if (1) {
        int rd, wr;
        Uint32 reg_val, num;
        Uint32 index;
        Uint32 vcpu_reg[31] = { 0, };

        VLOG(ERR, "------------------------------------------------------------------------------- ");
        VLOG(ERR, "------                            VCPU STATUS(ENC)                        ----- ");
        VLOG(ERR, "------------------------------------------------------------------------------- ");
        rd = VpuReadReg(coreIdx, W4_BS_RD_PTR);
        wr = VpuReadReg(coreIdx, W4_BS_WR_PTR);
        VLOG(ERR, "RD_PTR: 0x%08x WR_PTR: 0x%08x BS_OPT: 0x%08x BS_PARAM: 0x%08x\n",
                        rd, wr, VpuReadReg(coreIdx, W4_BS_OPTION), VpuReadReg(coreIdx, W4_BS_PARAM));

        // --------- VCPU register Dump
        VLOG(ERR, "[+] VCPU REG Dump\n");
        for (index = 0; index < 25; index++) {
            VpuWriteReg(coreIdx, W4_VPU_PDBG_IDX_REG, (1 << 9) | (index & 0xff));
            vcpu_reg[index] = VpuReadReg(coreIdx, W4_VPU_PDBG_RDATA_REG);

            if (index < 16) {
                VLOG(ERR, "0x%08x\t", vcpu_reg[index]);
                if ((index % 4) == 3)
                    VLOG(ERR, " ");
            } else {
                switch (index) {
                    case 16:
                        VLOG(ERR, "CR0: 0x%08x\t", vcpu_reg[index]);
                        break;
                    case 17:
                        VLOG(ERR, "CR1: 0x%08x\n", vcpu_reg[index]);
                        break;
                    case 18:
                        VLOG(ERR, "ML:  0x%08x\t", vcpu_reg[index]);
                        break;
                    case 19:
                        VLOG(ERR, "MH:  0x%08x\n", vcpu_reg[index]);
                        break;
                    case 21:
                        VLOG(ERR, "LR:  0x%08x\n", vcpu_reg[index]);
                        break;
                    case 22:
                        VLOG(ERR, "PC:  0x%08x\n", vcpu_reg[index]);
                        break;
                    case 23:
                        VLOG(ERR, "SR:  0x%08x\n", vcpu_reg[index]);
                        break;
                    case 24:
                        VLOG(ERR, "SSP: 0x%08x\n", vcpu_reg[index]);
                        break;
                }
            }
        }
        VLOG(ERR, "[-] VCPU REG Dump\n");
        VLOG(ERR, "vce run flag = %d\n", VpuReadReg(coreIdx, 0x1E8));
        // --------- BIT register Dump
        VLOG(ERR, "[+] BPU REG Dump\n");
        for (index = 0; index < 30; index++) {
            unsigned int temp;
            temp = FIORead(coreIdx, (W4_REG_BASE + 0x8000 + 0x18));
            VLOG(ERR, "BITPC = 0x%08x\n", temp);
            if (temp == 0xffffffff)
                return;
        }

        // --------- BIT HEVC Status Dump
        VLOG(ERR, "[-] BPU REG Dump\n");

        VLOG(ERR, "DBG_FIFO_VALID   [%08x] ", FIORead(coreIdx, (W4_REG_BASE + 0x8000 + 0x6C)));

        //SDMA debug information
        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x13c);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        reg_val = FIORead(0, (W4_REG_BASE + 0x8000 + 0x78));
        VLOG(ERR, "SDMA_DBG_INFO    [%08x] ", reg_val);
        VLOG(ERR, "\t- [   28] need_more_update  : 0x%x  ", (reg_val >> 28) & 0x1);
        VLOG(ERR, "\t- [27:25] tr_init_fsm       : 0x%x  ", (reg_val >> 25) & 0x7);
        VLOG(ERR, "\t- [24:18] remain_trans_size : 0x%x  ", (reg_val >> 18) & 0x7F);
        VLOG(ERR, "\t- [17:13] wdata_out_cnt     : 0x%x  ", (reg_val >> 13) & 0x1F);
        VLOG(ERR, "\t- [12:10] wdma_wd_fsm       : 0x%x  ", (reg_val >> 10) & 0x1F);
        VLOG(ERR, "\t- [ 9: 7] wdma_wa_fsm       : 0x%x ", (reg_val >> 7) & 0x7);
        if (((reg_val >> 7) & 0x7) == 3) {
            VLOG(ERR, "-->WDMA_WAIT_ADDR   ");
        } else {
            VLOG(ERR, " ");
        }
        VLOG(ERR, "\t- [ 6: 5] sdma_init_fsm     : 0x%x  ", (reg_val >> 5) & 0x3);
        VLOG(ERR, "\t- [ 4: 1] save_fsm          : 0x%x  ", (reg_val >> 1) & 0xF);
        VLOG(ERR, "\t- [    0] unalign_written   : 0x%x  ", (reg_val >> 0) & 0x1);

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x13b);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        reg_val = FIORead(0, (W4_REG_BASE + 0x8000 + 0x78));
        VLOG(ERR, "SDMA_NAL_MEM_INF [%08x] ", reg_val);
        VLOG(ERR, "\t- [ 7: 1] nal_mem_empty_room : 0x%x  ", (reg_val >> 1) & 0x3F);
        VLOG(ERR, "\t- [    0] ge_wnbuf_size      : 0x%x  ", (reg_val >> 0) & 0x1);

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x131);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        reg_val = FIORead(0, (W4_REG_BASE + 0x8000 + 0x78));
        VLOG(ERR, "SDMA_IRQ   [%08x]: [1]sdma_irq : 0x%x, [2]enable_sdma_irq : 0x%x\n", reg_val, (reg_val >> 1) & 0x1, (reg_val & 0x1));

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x134);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        VLOG(ERR, "SDMA_BS_BASE_ADDR [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x78)));

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x135);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        VLOG(ERR, "SDMA_NAL_STR_ADDR [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x78)));

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x136);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        VLOG(ERR, "SDMA_IRQ_ADDR     [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x78)));

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x137);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        VLOG(ERR, "SDMA_BS_END_ADDR [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x78)));

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x13A);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        VLOG(ERR, "SDMA_CUR_ADDR    [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x78)));

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x139);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        reg_val = FIORead(0, (W4_REG_BASE + 0x8000 + 0x78));
        VLOG(ERR, "SDMA_STATUS      [%08x] ", reg_val);
        VLOG(ERR, "\t- [2] all_wresp_done : 0x%x  ", (reg_val >> 2) & 0x1);
        VLOG(ERR, "\t- [1] sdma_init_busy : 0x%x  ", (reg_val >> 1) & 0x1);
        VLOG(ERR, "\t- [0] save_busy      : 0x%x  ", (reg_val >> 0) & 0x1);

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x164);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        reg_val = FIORead(0, (W4_REG_BASE + 0x8000 + 0x78));
        VLOG(ERR, "SHU_DBG        [%08x] : shu_unaligned_num (0x%x) ", reg_val, reg_val);

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x169);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        VLOG(ERR, "SHU_NBUF_WPTR    [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x78)));

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x16A);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        VLOG(ERR, "SHU_NBUF_RPTR    [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x78)));

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x16C);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        reg_val = FIORead(0, (W4_REG_BASE + 0x8000 + 0x78));
        VLOG(ERR, "SHU_NBUF_INFO    [%08x] ", reg_val);
        VLOG(ERR, "\t- [5:1] nbuf_remain_byte : 0x%x  ", (reg_val >> 1) & 0x1F);
        VLOG(ERR, "\t- [  0] nbuf_wptr_wrap   : 0x%x  ", (reg_val >> 0) & 0x1);

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x184);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        VLOG(ERR, "CTU_LAST_ENC_POS [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x78)));

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x187);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        VLOG(ERR, "CTU_POS_IN_PIC   [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x78)));

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x110);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        VLOG(ERR, "MIB_EXTADDR      [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x78)));

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x111);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        VLOG(ERR, "MIB_INTADDR      [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x78)));

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x113);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        VLOG(ERR, "MIB_CMD        [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x78)));

        FIOWrite(0, (W4_REG_BASE + 0x8000 + 0x74), (1 << 20) | (1 << 16) | 0x114);
        while ((FIORead(0,(W4_REG_BASE + 0x8000 + 0x7c)) & 0x1) == 0)
            ;
        VLOG(ERR, "MIB_BUSY     [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x78)));

        VLOG(ERR, "DBG_BPU_ENC_NB0    [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x40)));
        VLOG(ERR, "DBG_BPU_CTU_CTRL0  [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x44)));
        VLOG(ERR, "DBG_BPU_CAB_FSM0 [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x48)));
        VLOG(ERR, "DBG_BPU_BIN_GEN0 [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x4C)));
        VLOG(ERR, "DBG_BPU_CAB_MBAE0  [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x50)));
        VLOG(ERR, "DBG_BPU_BUS_BUSY [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x68)));
        VLOG(ERR, "DBG_FIFO_VALID   [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x6C)));
        VLOG(ERR, "DBG_BPU_CTU_CTRL1  [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x54)));
        VLOG(ERR, "DBG_BPU_CTU_CTRL2  [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x58)));
        VLOG(ERR, "DBG_BPU_CTU_CTRL3  [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x5C)));

        for (index = 0x80; index < 0xA0; index += 4) {
            reg_val = FIORead(0, (W4_REG_BASE + 0x8000 + index));
            num = (index - 0x80) / 2;
            VLOG(ERR, "DBG_BIT_STACK    [%08x] : Stack%02d (0x%04x), Stack%02d(0x%04x)  ", reg_val, num, reg_val >> 16, num + 1, reg_val & 0xffff);
        }

        reg_val = FIORead(0, (W4_REG_BASE + 0x8000 + 0xA0));
        VLOG(ERR, "DGB_BIT_CORE_INFO  [%08x] : pc_ctrl_id (0x%04x), pfu_reg_pc(0x%04x) ", reg_val, reg_val >> 16, reg_val & 0xffff);
        reg_val = FIORead(0, (W4_REG_BASE + 0x8000 + 0xA4));
        VLOG(ERR, "DGB_BIT_CORE_INFO  [%08x] : ACC0 (0x%08x) ", reg_val, reg_val);
        reg_val = FIORead(0, (W4_REG_BASE + 0x8000 + 0xA8));
        VLOG(ERR, "DGB_BIT_CORE_INFO  [%08x] : ACC1 (0x%08x) ", reg_val, reg_val);

        reg_val = FIORead(0, (W4_REG_BASE + 0x8000 + 0xAC));
        VLOG(ERR, "DGB_BIT_CORE_INFO  [%08x] : pfu_ibuff_id(0x%04x), pfu_ibuff_op(0x%04x) ", reg_val, reg_val >> 16, reg_val & 0xffff);

        for (num = 0; num < 5; num += 1) {
            reg_val = FIORead(0, (W4_REG_BASE + 0x8000 + 0xB0));
            VLOG(ERR, "DGB_BIT_CORE_INFO  [%08x] : core_pram_rd_en(0x%04x), core_pram_rd_addr(0x%04x) ", reg_val, reg_val >> 16, reg_val & 0xffff);
        }

        VLOG(ERR, "SAO_LUMA_OFFSET  [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0xB4)));
        VLOG(ERR, "SAO_CB_OFFSET  [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0xB8)));
        VLOG(ERR, "SAO_CR_OFFSET  [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0xBC)));

        VLOG(ERR, "GDI_NO_MORE_REQ    [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x8f0)));
        VLOG(ERR, "GDI_EMPTY_FLAG   [%08x] ", FIORead(0,(W4_REG_BASE + 0x8000 + 0x8f4)));

        // --------- VCE register Dump
        VLOG(ERR, "[+] VCE REG Dump\n");
        for (index = 0; index < 0xC4; index += 8) {
            VLOG(ERR, "VCE REG[0x%08x] -> 0x%08x \tVCE REG[0x%08x] -> 0x%08x  ", index, ReadRegVCE(0, 0, 0x0b00 + index), index + 4, ReadRegVCE(0, 0, 0x0b00 + index + 4));
        }
        WriteRegVCE(0, 0, 0x0bc8, 0);

        VLOG(ERR, "DEBUG----------- common ------------------------------- ");
        VLOG(ERR, "DEBUG1 REG[0x%08x] -> 0x%08x\n", 0xcc, ReadRegVCE(0, 0, 0x0bcc));
        VLOG(ERR, "DEBUG2 REG[0x%08x] -> 0x%08x\n", 0xd0, ReadRegVCE(0, 0, 0x0bd0));
        VLOG(ERR, "DEBUG----------- common ------------------------------- ");
        VLOG(ERR, "DEBUG----------- common ------------------------------- ");
        {
            reg_val = ReadRegVCE(0, 0, 0x0bcc);
        }
        VLOG(ERR, "DEBUG1 REG[0x%08x] -> 0x%08x ; SFU %d , LF %d , RDO %d IMD %d FME %d IME %d\n",
                0xcc, ReadRegVCE(0, 0, 0x0bcc),
                ((reg_val >> 5) & 0x1),
                ((reg_val >> 4) & 0x1),
                ((reg_val >> 3) & 0x1),
                ((reg_val >> 2) & 0x1),
                ((reg_val >> 1) & 0x1),
                ((reg_val >> 0) & 0x1));
        //VLOG(ERR,"DEBUG1 REG[0x%08x] -> 0x%08x\n", 0xcc, ReadRegVCE(0, 0, 0x0bcc));
        //VLOG(ERR,"DEBUG2 REG[0x%08x] -> 0x%08x\n", 0xd0, ReadRegVCE(0, 0, 0x0bd0));

        VLOG(ERR, "DEBUG----------- MODE 0 : DCI dequeue/queue counts ---------------------- ");
        WriteRegVCE(0, 0, 0x0bc8, 0); //mode 0 select
        VLOG(ERR, "DCI TUH  READ: %08x, WRITE : %08x  ", (ReadRegVCE(0, 0, 0x0bd4) >> 16), (ReadRegVCE(0, 0, 0x0bd4) & 0xffff));
        VLOG(ERR, "DCI CU   READ: %08x, WRITE : %08x  ", (ReadRegVCE(0, 0, 0x0bd8) >> 16), (ReadRegVCE(0, 0, 0x0bd8) & 0xffff));
        VLOG(ERR, "DCI CTU  READ: %08x, WRITE : %08x  ", (ReadRegVCE(0, 0, 0x0bdc) >> 16), (ReadRegVCE(0, 0, 0x0bdc) & 0xffff));
        VLOG(ERR, "DCI COEF READ: %08x, WRITE : %08x  ", (ReadRegVCE(0, 0, 0x0be0) >> 16), (ReadRegVCE(0, 0, 0x0be0) & 0xffff));
        VLOG(ERR, "DCI Full/Empty Info : %08x  ", ReadRegVCE(0, 0, 0x0be4));
        VLOG(ERR, "MODE0 RESERVED : %08x  ", ReadRegVCE(0, 0, 0x0be8));
        VLOG(ERR, "MODE0 RESERVED : %08x  ", ReadRegVCE(0, 0, 0x0bec));

        VLOG(ERR, "DEBUG----------- MODE 1 ---RDO debug --------------------- ");
        WriteRegVCE(0, 0, 0x0bc8, 1); //mode 1 select
        VLOG(ERR, "RDO_DBG1 : %08x  ", ReadRegVCE(0, 0, 0x0bd4));
        VLOG(ERR, "RDO_DBG2 : %08x  ", ReadRegVCE(0, 0, 0x0bd8));
        VLOG(ERR, "RDO_DBG3 : %08x  ", ReadRegVCE(0, 0, 0x0bdc));
        VLOG(ERR, "RDO_DBG4 : %08x  ", ReadRegVCE(0, 0, 0x0be0));
        VLOG(ERR, "RDO_DBG5 : %08x  ", ReadRegVCE(0, 0, 0x0be4));
        VLOG(ERR, "RDO_DBG6 : %08x  ", ReadRegVCE(0, 0, 0x0be8));
        VLOG(ERR, "MODE1 RESERVED : %08x  ", ReadRegVCE(0, 0, 0x0bec));

        VLOG(ERR, "DEBUG----------- MODE 2 ---LF debug ---------------------------- ");
        WriteRegVCE(0, 0, 0x0bc8, 2); //mode 2 select
        reg_val = ReadRegVCE(0, 0, 0x0bd4);
        VLOG(ERR, "LF : %08x  ", reg_val);
        VLOG(ERR, "\t- cur_enc_main_state   : 0x%x  ", (reg_val >> 27) & 0x1F);
        VLOG(ERR, "\t- i_sao_para_valie     : 0x%x  ", (reg_val >> 26) & 0x1);
        VLOG(ERR, "\t- i_sao_fetch_done     : 0x%x  ", (reg_val >> 25) & 0x1);
        VLOG(ERR, "\t- i_global_encode_en   : 0x%x  ", (reg_val >> 24) & 0x1);
        VLOG(ERR, "\t- i_bs_valid           : 0x%x  ", (reg_val >> 23) & 0x1);
        VLOG(ERR, "\t- i_rec_buf_rdo_ready  : 0x%x  ", (reg_val >> 22) & 0x1);
        VLOG(ERR, "\t- o_rec_buf_dbk_hold   : 0x%x  ", (reg_val >> 21) & 0x1);
        VLOG(ERR, "\t- cur_main_state       : 0x%x  ", (reg_val >> 16) & 0x1F);
        VLOG(ERR, "\t- r_lf_pic_dbk_disable : 0x%x  ", (reg_val >> 15) & 0x1);
        VLOG(ERR, "\t- r_lf_pic_sao_disable : 0x%x  ", (reg_val >> 14) & 0x1);
        VLOG(ERR, "\t- para_load_done       : 0x%x  ", (reg_val >> 13) & 0x1);
        VLOG(ERR, "\t- i_rdma_ack_wait      : 0x%x  ", (reg_val >> 12) & 0x1);
        VLOG(ERR, "\t- i_sao_intl_col_done  : 0x%x  ", (reg_val >> 11) & 0x1);
        VLOG(ERR, "\t- i_sao_outbuf_full    : 0x%x  ", (reg_val >> 10) & 0x1);
        VLOG(ERR, "\t- lf_sub_done          : 0x%x  ", (reg_val >> 9) & 0x1);
        VLOG(ERR, "\t- i_wdma_ack_wait      : 0x%x  ", (reg_val >> 8) & 0x1);
        VLOG(ERR, "\t- lf_all_sub_done      : 0x%x  ", (reg_val >> 6) & 0x1);
        VLOG(ERR, "\t- cur_ycbcr            : 0x%x  ", (reg_val >> 5) & 0x3);
        VLOG(ERR, "\t- sub8x8_done          : 0x%x  ", (reg_val >> 0) & 0xF);
        VLOG(ERR, "MODE2 RESERVED : %08x  ", ReadRegVCE(0, 0, 0x0bd4));
        VLOG(ERR, "MODE2 RESERVED : %08x  ", ReadRegVCE(0, 0, 0x0bd8));
        VLOG(ERR, "MODE2 RESERVED : %08x  ", ReadRegVCE(0, 0, 0x0bdc));
        VLOG(ERR, "MODE2 RESERVED : %08x  ", ReadRegVCE(0, 0, 0x0be0));
        VLOG(ERR, "MODE2 RESERVED : %08x  ", ReadRegVCE(0, 0, 0x0be4));
        VLOG(ERR, "MODE2 RESERVED : %08x  ", ReadRegVCE(0, 0, 0x0be8));
        VLOG(ERR, "MODE2 RESERVED : %08x  ", ReadRegVCE(0, 0, 0x0bec));

        VLOG(ERR, "DEBUG----------- MODE 3 --------SFU-------------------- ");
        WriteRegVCE(0, 0, 0x0bc8, 3); //mode 3 select
        VLOG(ERR, "SFU 0 : %08x  ", (ReadRegVCE(0, 0, 0x0bd4)));
        VLOG(ERR, "SFU 1 : %08x  ", (ReadRegVCE(0, 0, 0x0bd8)));
        VLOG(ERR, "SFU 2 : %08x  ", (ReadRegVCE(0, 0, 0x0bdc)));
        VLOG(ERR, "SFU 3 : %08x  ", (ReadRegVCE(0, 0, 0x0be0)));

        VLOG(ERR, "DEBUG----------- MODE 4 --------DCI read/write cnt-------------------- ");
        WriteRegVCE(0, 0, 0x0bc8, 4);
        VLOG(ERR, "DCI TUH  READ: %08x, WRITE : %08x  ", (ReadRegVCE(0, 0, 0x0bd4) >> 16), (ReadRegVCE(0, 0, 0x0bd4) & 0xffff));
        VLOG(ERR, "DCI CU   READ: %08x, WRITE : %08x  ", (ReadRegVCE(0, 0, 0x0bd8) >> 16), (ReadRegVCE(0, 0, 0x0bd8) & 0xffff));
        VLOG(ERR, "DCI CTU  READ: %08x, WRITE : %08x  ", (ReadRegVCE(0, 0, 0x0bdc) >> 16), (ReadRegVCE(0, 0, 0x0bdc) & 0xffff));
        VLOG(ERR, "DCI COEF READ: %08x, WRITE : %08x  ", (ReadRegVCE(0, 0, 0x0be0) >> 16), (ReadRegVCE(0, 0, 0x0be0) & 0xffff));

        VLOG(ERR, "DEBUG----------- MODE 5 --------DCI last read data-------------------- ");
        WriteRegVCE(0, 0, 0x0bc8, 5);
        VLOG(ERR, "DCI TUH    last read data : %08x  ", ReadRegVCE(0, 0, 0x0bd4));
        VLOG(ERR, "DCI CTU    last read data : %08x  ", ReadRegVCE(0, 0, 0x0bd8));
        VLOG(ERR, "DCI CU     last read data : %08x  ", ReadRegVCE(0, 0, 0x0bdc));
        VLOG(ERR, "DCI COEF_H last read data : %08x  ", ReadRegVCE(0, 0, 0x0be0));
        VLOG(ERR, "DCI COEF_L last read data : %08x  ", ReadRegVCE(0, 0, 0x0be4));
        VLOG(ERR, "DCI INFO 0               : %08x  ", ReadRegVCE(0, 0, 0x0be8));
        VLOG(ERR, "DCI INFO 1               : %08x  ", ReadRegVCE(0, 0, 0x0beC));

        VLOG(ERR, "DEBUG----------- MODE 6 --------DCI   Big buffer read/write cnt-------------------- ");
        WriteRegVCE(0, 0, 0x0bc8, 6);
        VLOG(ERR, "DCI TUH  READ: %08x, WRITE : %08x  ", (ReadRegVCE(0, 0, 0x0bd4) >> 16), (ReadRegVCE(0, 0, 0x0bd4) & 0xffff));
        VLOG(ERR, "DCI CU   READ: %08x, WRITE : %08x  ", (ReadRegVCE(0, 0, 0x0bd8) >> 16), (ReadRegVCE(0, 0, 0x0bd8) & 0xffff));
        VLOG(ERR, "DCI COEF READ: %08x, WRITE : %08x  ", (ReadRegVCE(0, 0, 0x0bdc) >> 16), (ReadRegVCE(0, 0, 0x0bdc) & 0xffff));
        VLOG(ERR, "DEBUG----------- MODE 6 --------DCI Small buffer read/write cnt-------------------- ");
        VLOG(ERR, "DCI TUH  READ: %08x, WRITE : %08x  ", (ReadRegVCE(0, 0, 0x0be0) >> 16), (ReadRegVCE(0, 0, 0x0be0) & 0xffff));
        VLOG(ERR, "DCI CU   READ: %08x, WRITE : %08x  ", (ReadRegVCE(0, 0, 0x0be4) >> 16), (ReadRegVCE(0, 0, 0x0be4) & 0xffff));
        VLOG(ERR, "DCI CTU  READ: %08x, WRITE : %08x  ", (ReadRegVCE(0, 0, 0x0be8) >> 16), (ReadRegVCE(0, 0, 0x0be8) & 0xffff));
        VLOG(ERR, "DCI COEF READ: %08x, WRITE : %08x  ", (ReadRegVCE(0, 0, 0x0bec) >> 16), (ReadRegVCE(0, 0, 0x0bec) & 0xffff));

        VLOG(ERR, "DEBUG----------- MODE 7 --------DCI   Big buffer read/write address--------------------");
        WriteRegVCE(0, 0, 0x0bc8, 7);
        VLOG(ERR, "DCI TUH  READ: %08x, WRITE : %08x", (ReadRegVCE(0, 0, 0x0bd4) >> 16), (ReadRegVCE(0, 0, 0x0bd4) & 0xffff));
        VLOG(ERR, "DCI CU   READ: %08x, WRITE : %08x", (ReadRegVCE(0, 0, 0x0bd8) >> 16), (ReadRegVCE(0, 0, 0x0bd8) & 0xffff));
        VLOG(ERR, "DCI COEF READ: %08x, WRITE : %08x", (ReadRegVCE(0, 0, 0x0bdc) >> 16), (ReadRegVCE(0, 0, 0x0bdc) & 0xffff));
        VLOG(ERR, "DEBUG----------- MODE 7 --------DCI Small buffer read/write address--------------------");
        VLOG(ERR, "DCI TUH  READ: %08x, WRITE : %08x", (ReadRegVCE(0, 0, 0x0be0) >> 16), (ReadRegVCE(0, 0, 0x0be0) & 0xffff));
        VLOG(ERR, "DCI CU   READ: %08x, WRITE : %08x", (ReadRegVCE(0, 0, 0x0be4) >> 16), (ReadRegVCE(0, 0, 0x0be4) & 0xffff));
        VLOG(ERR, "DCI CTU  READ: %08x, WRITE : %08x", (ReadRegVCE(0, 0, 0x0be8) >> 16), (ReadRegVCE(0, 0, 0x0be8) & 0xffff));
        VLOG(ERR, "DCI COEF READ: %08x, WRITE : %08x", (ReadRegVCE(0, 0, 0x0bec) >> 16), (ReadRegVCE(0, 0, 0x0bec) & 0xffff));

        VLOG(ERR, "DEBUG----------- Others --------------------");
        VLOG(ERR, "DEBUG10 : %08x", (ReadRegVCE(0, 0, 0x0bf0)));
        VLOG(ERR, "DEBUG11 : %08x", (ReadRegVCE(0, 0, 0x0bf4)));
        VLOG(ERR, "DEBUG12 : %08x", (ReadRegVCE(0, 0, 0x0bf8)));
        VLOG(ERR, "-------------------------------------------------------------------------------");
    }
}

void Wave4BitIssueCommand(Uint32 coreIdx, Uint32 instanceIndex, Uint32 cmd) {

	(void)instanceIndex;

    VpuWriteReg(coreIdx, W4_VPU_BUSY_STATUS, 1);
    VpuWriteReg(coreIdx, W4_RET_SUCCESS, 0);  //for debug
    //VpuWriteReg(coreIdx, W4_CORE_INDEX, 0);
    //VpuWriteReg(coreIdx, W4_INST_INDEX, (instanceIndex & 0xffff) | (codecMode << 16));

    VpuWriteReg(coreIdx, W4_COMMAND, cmd);
    VpuWriteReg(coreIdx, W4_VPU_HOST_INT_REQ, 1);
    return;
}

AMVEnc_Status Wave4VpuSleepWake(AMVHEVCEncHandle *Handle, u32 coreIdx, int iSleepWake, const u16* code, u32 size, u32 mode) {
    u32 regVal;
    vpu_buffer_t vb;
    u32 codeBase;
    u32 codeSize;
    u32 remapSize;
    uint32_t timeoutTicks = 0;

    (void)code;
    (void)size;

    if (vdi_wait_vpu_busy(coreIdx, VPU_BUSY_CHECK_TIMEOUT, W4_VPU_BUSY_STATUS) == -1) {
        VLOG(ERR, "Wave4VpuSleepWake timeout");
        reset_error = 1;
        return AMVENC_TIMEOUT;
    }

    if (iSleepWake == 1)  //saves
            {
        VpuWriteReg(coreIdx, W4_VPU_BUSY_STATUS, 1);
        VpuWriteReg(coreIdx, W4_RET_SUCCESS, 0);  //for debug
        VpuWriteReg(coreIdx, W4_CORE_INDEX, 0);

        VpuWriteReg(coreIdx, W4_COMMAND, SLEEP_VPU);
        VpuWriteReg(coreIdx, W4_VPU_HOST_INT_REQ, 1);
        if (vdi_wait_interrupt(0, VPU_BUSY_CHECK_TIMEOUT) == -1) {
            VLOG(ERR, "Wave4VpuSleepWake interrput error time out");
            reset_error = 1;
            return AMVENC_TIMEOUT;
        }

        regVal = VpuReadReg(coreIdx, W4_RET_SUCCESS);
        if (regVal == 0) {
            uint32_t reasonCode = VpuReadReg(coreIdx, W4_RET_FAIL_REASON);
            VLOG(ERR, "VPU Sleep (W4_RET_SUCCESS) failed(%d) REASON CODE(%08x) ", regVal, reasonCode);
            reset_error = 1;
            return AMVENC_HARDWARE;
        }
        VLOG(DEBUG, "VPU SLEEP Sucess");
    } else //restore
    {
        Uint32 hwOption = 0;

        vdi_get_common_memory(coreIdx, &vb);
        codeBase = vb.phys_addr;
        /* ALIGN TO 4KB */
        codeSize = (vb.size & ~0xfff);

        regVal = 0;
        VpuWriteReg(coreIdx, W4_PO_CONF, regVal);

        /* SW_RESET_SAFETY */
        regVal = mode; //W4_RST_BLOCK_ACLK_ALL | W4_RST_BLOCK_BCLK_ALL | W4_RST_BLOCK_CCLK_ALL;
        VpuWriteReg(coreIdx, W4_VPU_RESET_REQ, regVal);    // Reset All blocks

        /* Waiting reset done */
        if (vdi_wait_vpu_busy(coreIdx, VPU_BUSY_CHECK_TIMEOUT, W4_VPU_RESET_STATUS) == -1) {
            VpuWriteReg(coreIdx, W4_VPU_RESET_REQ, 0);
            VLOG(ERR, "VPU Wake Reset timeout");
            reset_error = 1;
            return AMVENC_TIMEOUT;
        }

        VpuWriteReg(coreIdx, W4_VPU_RESET_REQ, 0);
//        if (mode & 0xff000000)
//            vdi_clear_memory(0, stack_vb.phys_addr,stack_vb.size);
        /* remap page size */
        remapSize = (codeSize >> 12) & 0x1ff;
        regVal = 0x80000000 | (W4_REMAP_CODE_INDEX << 12) | (0 << 16) | (1 << 11) | remapSize;
        VpuWriteReg(coreIdx, W4_VPU_REMAP_CTRL, regVal);
        VpuWriteReg(coreIdx, W4_VPU_REMAP_VADDR, 0x00000000); /* DO NOT CHANGE! */
        VpuWriteReg(coreIdx, W4_VPU_REMAP_PADDR, codeBase);
        VpuWriteReg(coreIdx, W4_ADDR_CODE_BASE, codeBase);
        VpuWriteReg(coreIdx, W4_CODE_SIZE, codeSize);
        VpuWriteReg(coreIdx, W4_CODE_PARAM, 0);
        VpuWriteReg(coreIdx, W4_COMMAND_OPTION, 0);
        timeoutTicks = COMMAND_TIMEOUT * VCPU_CLOCK_IN_MHZ * (1000000 >> 15);
        timeoutTicks = timeoutTicks / 120;
        if (Handle->enc_width * Handle->enc_height <= 1280 * 720)
            timeoutTicks = timeoutTicks / 16; //  /8 == 20ms  /4 == 40 ms  /16 == 10ms
        else if (Handle->enc_width * Handle->enc_height <= 1920 * 1088)
            timeoutTicks = timeoutTicks / 8;
        else
            timeoutTicks = timeoutTicks / 2;
        VpuWriteReg(coreIdx, W4_TIMEOUT_CNT, timeoutTicks);

//        codeBase = (Uint32)stack_vb.phys_addr;
//        codeSize = (Uint32)stack_vb.size;
//        VpuWriteReg(coreIdx, W4_ADDR_STACK_BASE,     codeBase);
//        VpuWriteReg(coreIdx, W4_STACK_SIZE,          codeSize);

        VpuWriteReg(coreIdx, W4_HW_OPTION, hwOption);

        /* Interrupt */
        // for encoder interrupt
        regVal = (1 << W4_INT_ENC_PIC);
        regVal |= (1 << W4_INT_SLEEP_VPU);
        regVal |= (1 << W4_INT_SET_PARAM);
        // for decoder interrupt
        regVal |= (1 << W4_INT_DEC_PIC_HDR);
        regVal |= (1 << W4_INT_DEC_PIC);
        regVal |= (1 << W4_INT_QUERY_DEC);
        regVal |= (1 << W4_INT_SLEEP_VPU);
        regVal |= (1 << W4_INT_BSBUF_EMPTY);
        regVal = 0xfffff6ff; // remove wake up int bit 11
        VpuWriteReg(coreIdx, W4_VPU_VINT_ENABLE, regVal);

        VpuWriteReg(coreIdx, W4_VPU_BUSY_STATUS, 1);
        VpuWriteReg(coreIdx, W4_RET_SUCCESS, 0);  //for debug
        VpuWriteReg(coreIdx, W4_CORE_INDEX, 0);
        VpuWriteReg(coreIdx, W4_INST_INDEX, 0);
        VpuWriteReg(coreIdx, W4_COMMAND, INIT_VPU);
        VpuWriteReg(coreIdx, W4_VPU_HOST_INT_REQ, 1);
        VpuWriteReg(coreIdx, W4_VPU_REMAP_CORE_START, 1);

        if (vdi_wait_interrupt(0, VPU_BUSY_CHECK_TIMEOUT) == -1) {
            VLOG(ERR, "VPU Wake timeout");
            reset_error = 1;
            return AMVENC_TIMEOUT;
        }

        //if (vdi_wait_vpu_busy(coreIdx, VPU_BUSY_CHECK_TIMEOUT, W4_VPU_BUSY_STATUS) == -1) {
        //    VLOG(ERR, "VPU Wake timeout");
        //    reset_error = 1;
        //    return AMVENC_TIMEOUT;
        //}
        regVal = VpuReadReg(coreIdx, W4_RET_SUCCESS);
        if (regVal == 0) {
            uint32_t reasonCode = VpuReadReg(coreIdx, W4_RET_FAIL_REASON);
            VLOG(ERR, "VPU Wake(W4_RET_SUCCESS) failed(%d) REASON CODE(%08x) ", regVal, reasonCode);
            reset_error = 1;
            return AMVENC_HARDWARE;
        }
        VLOG(DEBUG, "VPU WAKE Sucess");
    }
    return AMVENC_SUCCESS;
}

AMVEnc_Status Wave4VpuReset(AMVHEVCEncHandle *Handle, Uint32 coreIdx, int resetMode) {
    u32 val = 0;

    // VPU doesn't send response. Force to set BUSY flag to 0.
    VpuWriteReg(coreIdx, W4_VPU_BUSY_STATUS, 0);

    if (resetMode == SW_RESET_SAFETY) {
        Wave4VpuSleepWake(Handle, coreIdx, 1, NULL, 0, 0);
    }

    switch (resetMode) {
    case SW_RESET_ON_BOOT:
        val = W4_RST_BLOCK_ALL;
        break;
    case SW_RESET_FORCE:
        val = W4_RST_BLOCK_BCLK_ALL | W4_RST_BLOCK_CCLK_ALL;
        break;
    case SW_RESET_SAFETY:
        val = W4_RST_BLOCK_ACLK_ALL | W4_RST_BLOCK_BCLK_ALL | W4_RST_BLOCK_CCLK_ALL;
        break;
    default:
        return AMVENC_NOT_SUPPORTED;
    }

    if (val) {
        VpuWriteReg(coreIdx, W4_VPU_RESET_REQ, val);
        if (vdi_wait_vpu_busy(coreIdx, VPU_BUSY_CHECK_TIMEOUT, W4_VPU_RESET_STATUS) == -1) {
            VpuWriteReg(coreIdx, W4_VPU_RESET_REQ, 0);
            VLOG(INFO, "Wave4VpuReset time out reset");
            reset_error = 1;
            return AMVENC_TIMEOUT;
        }
        VpuWriteReg(coreIdx, W4_VPU_RESET_REQ, 0);
    }

    if (resetMode == SW_RESET_SAFETY || resetMode == SW_RESET_ON_BOOT) {
        Wave4VpuSleepWake(Handle, coreIdx, 0, NULL, 0, val);
    }
    return AMVENC_SUCCESS;
}

AMVEnc_Status Wave4VpuInit(AMVHEVCEncHandle *Handle, Uint32 coreIdx) {
    vpu_buffer_t vb;
    Uint32 codeBase;
    Uint32* codeVaddr;
    Uint32 codeSize;
    Uint32 size;
    Uint32 i, regVal, remapSize;
    Uint32 hwOption = 0;
    uint32_t timeoutTicks = 0;

    vdi_get_common_memory(coreIdx, &vb);

    vdi_clear_memory(coreIdx, vb.phys_addr, vb.size);

    VLOG(DEBUG, "\nvdi_get_common_memory, paddr:0x%08lx\n", vb.phys_addr);

    codeSize = (vb.size & ~0xfff);

    size = sizeof(bit_code);
    if (size == 0 || size >= codeSize) {
        return AMVENC_INVALID_PARAM;
    }

    VLOG(DEBUG, "\nmonet.bin size :%d!!!\n", size);
    codeBase = (Uint32) vb.phys_addr;

    codeVaddr = (Uint32*) vb.virt_addr;

    VLOG(DEBUG, "\nVPU INIT Start!!!\n");

    //for (i=W4_CMD_REG_BASE; i<W4_CMD_REG_END; i+=4)
    //    VpuWriteReg(coreIdx, i, 0x00);

    VpuWriteMem(coreIdx, codeBase, (unsigned char* )bit_code, size);
    VLOG(DEBUG, "Code: 0x%08x, 0x%08x,0x%08x,0x%08x,\n", codeVaddr[0], codeVaddr[1], codeVaddr[2], codeVaddr[3]);
    vdi_set_bit_firmware_to_pm(coreIdx, (Uint16*) bit_code);

    regVal = 0;
    VpuWriteReg(coreIdx, W4_PO_CONF, regVal);

    regVal = VpuReadReg(coreIdx, W4_VPU_RESET_STATUS);
    VLOG(NONE, "W4_VPU_RESET_STATUS :0x%x\n", regVal);

    /* Reset All blocks */
    regVal = 0x7ffffff;
    VpuWriteReg(coreIdx, W4_VPU_RESET_REQ, regVal);    // Reset All blocks
    /* Waiting reset done */

    regVal = VpuReadReg(coreIdx, W4_VPU_RESET_STATUS);
    VLOG(NONE, "W4_VPU_RESET_STATUS :0x%x\n", regVal);

    if (vdi_wait_vpu_busy(coreIdx, VPU_BUSY_CHECK_TIMEOUT, W4_VPU_RESET_STATUS) == -1) {
        VLOG(ERR, "VPU init(W4_VPU_RESET_REQ) timeout\n");
        VpuWriteReg(coreIdx, W4_VPU_RESET_REQ, 0);
        return AMVENC_TIMEOUT;
    }

    VpuWriteReg(coreIdx, W4_VPU_RESET_REQ, 0);

    /* clear registers */
    for (i = W4_CMD_REG_BASE; i < W4_CMD_REG_END; i += 4)
        VpuWriteReg(coreIdx, i, 0x00);

    /* remap page size */
    remapSize = (codeSize >> 12) & 0x1ff;
    regVal = 0x80000000 | (0 << 16) | (W4_REMAP_CODE_INDEX << 12) | (1 << 11) | remapSize;
    VpuWriteReg(coreIdx, W4_VPU_REMAP_CTRL, regVal);
    VpuWriteReg(coreIdx, W4_VPU_REMAP_VADDR, 0x00000000); /* DO NOT CHANGE! */
    VpuWriteReg(coreIdx, W4_VPU_REMAP_PADDR, codeBase);
    VpuWriteReg(coreIdx, W4_ADDR_CODE_BASE, codeBase);
    VpuWriteReg(coreIdx, W4_CODE_SIZE, codeSize);
    VpuWriteReg(coreIdx, W4_CODE_PARAM, 0);

    regVal = VpuReadReg(coreIdx, W4_VPU_REMAP_PADDR);
    VLOG(NONE, "W4_VPU_REMAP_PADDR :0x%x\n", regVal);

    VpuWriteReg(coreIdx, W4_COMMAND_OPTION, 0);
    timeoutTicks = COMMAND_TIMEOUT * VCPU_CLOCK_IN_MHZ * (1000000 >> 15);
    timeoutTicks = timeoutTicks / 120;
    if (Handle->enc_width * Handle->enc_height <= 1280 * 720)
        timeoutTicks = timeoutTicks / 16; //  /8 == 20ms  /4 == 40 ms  /16 == 10ms
    else if (Handle->enc_width * Handle->enc_height <= 1920 * 1088)
        timeoutTicks = timeoutTicks / 8;
    else
        timeoutTicks = timeoutTicks / 2;
    VpuWriteReg(coreIdx, W4_TIMEOUT_CNT, timeoutTicks);

    VpuWriteReg(coreIdx, W4_HW_OPTION, hwOption);
    /* Interrupt */
    // for encoder interrupt
    regVal = (1 << W4_INT_ENC_PIC);
    regVal |= (1 << W4_INT_SLEEP_VPU);
    regVal |= (1 << W4_INT_SET_PARAM);
    // for decoder interrupt
    regVal |= (1 << W4_INT_DEC_PIC_HDR);
    regVal |= (1 << W4_INT_DEC_PIC);
    regVal |= (1 << W4_INT_QUERY_DEC);
    regVal |= (1 << W4_INT_SLEEP_VPU);
    regVal |= (1 << W4_INT_BSBUF_EMPTY);

    //regVal = 0xfffffefe;
    regVal = 0xfffff6ff; // remove wake up int bit 11
    VpuWriteReg(coreIdx, W4_VPU_VINT_ENABLE, regVal);

    VpuWriteReg(coreIdx, W4_VPU_BUSY_STATUS, 1);
    VpuWriteReg(coreIdx, W4_RET_SUCCESS, 0);  //for debug
    VpuWriteReg(coreIdx, W4_CORE_INDEX, 0);
    //VpuWriteReg(coreIdx, W4_INST_INDEX,  (instanceIndex&0xffff)|(codecMode<<16));
    VpuWriteReg(coreIdx, W4_INST_INDEX, 0);

    VpuWriteReg(coreIdx, W4_COMMAND, INIT_VPU);
    VpuWriteReg(coreIdx, W4_VPU_HOST_INT_REQ, 1);
    //Wave4BitIssueCommand(coreIdx,0, INIT_VPU);
    VpuWriteReg(coreIdx, W4_VPU_REMAP_CORE_START, 1);

    regVal = VpuReadReg(coreIdx, W4_VPU_BUSY_STATUS);
    VLOG(NONE, "W4_VPU_BUSY_STATUS :0x%x\n", regVal);

    //if (vdi_wait_vpu_busy(coreIdx, VPU_BUSY_CHECK_TIMEOUT, W4_VPU_BUSY_STATUS) == -1) {
    //    VLOG(ERR, "VPU init(W4_VPU_REMAP_CORE_START) timeout\n");
    //    return AMVENC_TIMEOUT;
    //}

    if (vdi_wait_interrupt(coreIdx, VPU_BUSY_CHECK_TIMEOUT) == -1) {
        VLOG(ERR, "VPU Wake timeout");
        reset_error = 1;
        return AMVENC_TIMEOUT;
    }

    regVal = VpuReadReg(coreIdx, W4_VCPU_CUR_PC);
    VLOG(NONE, "Cur PC :0x%x\n", regVal);
    regVal = VpuReadReg(coreIdx, W4_RET_SUCCESS);
    VLOG(DEBUG, "W4_RET_SUCCESS :0x%x\n", regVal);
    if (regVal == 0) {
        uint32_t reasonCode = VpuReadReg(coreIdx, W4_RET_FAIL_REASON);
        VLOG(ERR, "VPU init(W4_RET_SUCCESS) failed(%d) REASON CODE(%08x)\n", regVal, reasonCode);
        return AMVENC_HARDWARE;
    }

    VpuWriteReg(coreIdx, W4_VPU_BUSY_STATUS, 1);
    VpuWriteReg(coreIdx, W4_RET_SUCCESS, 0);  //for debug
    VpuWriteReg(coreIdx, W4_CORE_INDEX, 0);
    //VpuWriteReg(coreIdx, W4_INST_INDEX,  (instanceIndex&0xffff)|(codecMode<<16));

    VpuWriteReg(coreIdx, W4_COMMAND, GET_FW_VERSION);
    VpuWriteReg(coreIdx, W4_VPU_HOST_INT_REQ, 1);
    //Wave4BitIssueCommand(coreIdx, 0, GET_FW_VERSION);
    regVal = VpuReadReg(coreIdx, W4_VPU_BUSY_STATUS);
    VLOG(NONE, "W4_VPU_BUSY_STATUS :0x%x\n", regVal);
    if (vdi_wait_vpu_busy(coreIdx, VPU_BUSY_CHECK_TIMEOUT, W4_VPU_BUSY_STATUS) == -1) {
        VLOG(ERR, "VPU init(GET_FW_VERSION) timeout\n");
        return AMVENC_TIMEOUT;
    }
    regVal = VpuReadReg(coreIdx, W4_RET_SUCCESS);
    VLOG(NONE, "W4_RET_SUCCESS :0x%x\n", regVal);
    if (regVal == 0) {
        uint32_t reasonCode = VpuReadReg(coreIdx, W4_RET_FAIL_REASON);
        VLOG(ERR, "VPU init(GET_FW_VERSION) failed(%d) REASON CODE(%08x)\n", regVal, reasonCode);
        return AMVENC_HARDWARE;
    }

    regVal = VpuReadReg(coreIdx, W4_RET_PRODUCT_NAME);
    VLOG(DEBUG, "Product name %s\n", (Uint8*) &regVal);
    regVal = VpuReadReg(coreIdx, W4_RET_PRODUCT_VERSION);
    VLOG(DEBUG, "Product ver %x\n", regVal);

    regVal = VpuReadReg(coreIdx, W4_RET_STD_DEF0);
    VLOG(DEBUG, "Product W4_RET_STD_DEF0 %x\n", regVal);
    regVal = VpuReadReg(coreIdx, W4_RET_STD_DEF1);
    VLOG(DEBUG, "Product W4_RET_STD_DEF1 %x\n", regVal);
    regVal = VpuReadReg(coreIdx, W4_RET_CONF_FEATURE);
    VLOG(DEBUG, "Product W4_RET_CONF_FEATURE %x\n", regVal);
    regVal = VpuReadReg(coreIdx, W4_RET_CONFIG_DATE);
    VLOG(DEBUG, "Product W4_RET_CONFIG_DATE %x\n", regVal);
    regVal = VpuReadReg(coreIdx, W4_RET_CONFIG_REVISION);
    VLOG(DEBUG, "Product W4_RET_CONFIG_REVISION %x\n", regVal);
    regVal = VpuReadReg(coreIdx, W4_RET_CONFIG_TYPE);
    VLOG(DEBUG, "Product W4_RET_CONFIG_TYPE %x\n", regVal);

    regVal = VpuReadReg(coreIdx, W4_VPU_BUSY_STATUS);
    VLOG(DEBUG, "W4_VPU_BUSY_STATUS :0x%x\n", regVal);

    if (vdi_wait_vpu_busy(coreIdx, VPU_BUSY_CHECK_TIMEOUT, W4_VPU_BUSY_STATUS) == -1) {
        VLOG(ERR, "VPU init(Finish) timeout\n");
        return AMVENC_TIMEOUT;
    }
    return AMVENC_SUCCESS;
}

AMVEnc_Status Wave4VpuCreateInstance(AMVHEVCEncHandle *Handle, int alloc) {
    AMVEnc_Status ret = AMVENC_SUCCESS;
    Uint32 temp;
    if (alloc) {
        memset((void*) &Handle->work_vb, 0, sizeof(vpu_buffer_t));
        Handle->work_vb.size = WAVE4ENC_WORKBUF_SIZE;
        if (vdi_allocate_dma_memory(Handle->instance_id, &Handle->work_vb) < 0) {
            VLOG(ERR, "Wave4VpuCreateInstance error alloc\n");
            return AMVENC_MEMORY_FAIL;
        }
    }

    vdi_clear_memory(Handle->instance_id, Handle->work_vb.phys_addr, Handle->work_vb.size);

    // instance information
    temp = 0;
    temp |= (0 << 24); // [27:24] codec_std_aux
    temp |= (1 << 16); // [23:16] codec_std
    temp |= (Handle->instance_id << 0);  // [15: 0] Handle->instance_id
    VpuWriteReg(Handle->instance_id, W4_INST_INDEX, temp);

    VpuWriteReg(Handle->instance_id, W4_ADDR_WORK_BASE, Handle->work_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_WORK_SIZE, Handle->work_vb.size);
    VpuWriteReg(Handle->instance_id, W4_WORK_PARAM, 0);

    VpuWriteReg(Handle->instance_id, W4_VPU_BUSY_STATUS, 1);
    VpuWriteReg(Handle->instance_id, W4_RET_SUCCESS, 0);  //for debug
    VpuWriteReg(Handle->instance_id, W4_CORE_INDEX, 0);
    //VpuWriteReg(Handle->instance_id, W4_INST_INDEX,  (Handle->instance_id&0xffff)|(1<<16));

    VpuWriteReg(Handle->instance_id, W4_COMMAND, CREATE_INSTANCE);
    VpuWriteReg(Handle->instance_id, W4_VPU_HOST_INT_REQ, 1);
    //Wave4BitIssueCommand(0, 4, CREATE_INSTANCE);
    temp = VpuReadReg(Handle->instance_id, W4_VPU_BUSY_STATUS);
    VLOG(DEBUG, "W4_VPU_BUSY_STATUS :0x%x\n", temp);

    if (vdi_wait_interrupt(Handle->instance_id, VPU_BUSY_CHECK_TIMEOUT) == -1) {
        //vdi_free_dma_memory(0, &Handle->work_vb);
        //     memset((void*)&Handle->work_vb, 0, sizeof(vpu_buffer_t));
        VLOG(ERR, "Wave4VpuCreateInstance error time out\n");
        return AMVENC_TIMEOUT;
    }
    //if (vdi_wait_vpu_busy(0, VPU_BUSY_CHECK_TIMEOUT, W4_VPU_BUSY_STATUS) == -1) {
    //    vdi_free_dma_memory(0, &vb);
    //    VLOG(ERR, "Wave4VpuCreateInstance error time out\n");
    //    return AMVENC_TIMEOUT;
    //}

    if (VpuReadReg(Handle->instance_id, W4_RET_SUCCESS) == 0) {
        uint32_t reasonCode = VpuReadReg(Handle->instance_id, W4_RET_FAIL_REASON);
        VLOG(ERR, "Wave4VpuCreateInstance failedREASON CODE(%08x)\n", reasonCode);
        //vdi_free_dma_memory(0, &Handle->work_vb);
        //    memset((void*)&Handle->work_vb, 0, sizeof(vpu_buffer_t));
        ret = AMVENC_HARDWARE;
    }
    temp = VpuReadReg(Handle->instance_id, W4_RET_SUCCESS);
    VLOG(DEBUG, "W4_RET_SUCCESS %x\n", temp);
    temp = VpuReadReg(Handle->instance_id, W4_ADDR_WORK_BASE);
    VLOG(DEBUG, "W4_ADDR_WORK_BASE %x\n", temp);
    temp = VpuReadReg(Handle->instance_id, W4_WORK_SIZE);
    VLOG(DEBUG, "W4_WORK_SIZE %x\n", temp);
    return ret;
}

AMVEnc_Status Wave4VpuEncSeqInit(AMVHEVCEncHandle *Handle, int alloc) {
    int coreIdx = Handle->instance_id;
    Uint32 use_sec_axi = 0;    // [31:0]
    Uint32 sec_axi_buf_size = 0x00012800;   // 74KB
    Uint32 temp;   // [31:0]
    Uint32 rd_ptr, wr_ptr, src_num, fb_num;   // [31:0]
    VLOG(DEBUG, "Wave4VpuEnCSeqInit- start\n");
    // instance information
    if (alloc) {
        memset((void*) &Handle->temp_vb, 0, sizeof(vpu_buffer_t));
        Handle->temp_vb.size = WAVE420L_TEMP_BUF_SIZE;
        if (vdi_allocate_dma_memory(Handle->instance_id, &Handle->temp_vb) < 0) {
            VLOG(ERR, "Wave4VpuEnCSeqInit  error temp alloc\n");
            return AMVENC_MEMORY_FAIL;
        }
    }

    vdi_clear_memory(Handle->instance_id, Handle->temp_vb.phys_addr, Handle->temp_vb.size);

    temp = 0;
    temp |= (0 << 24); // [27:24] codec_std_aux
    temp |= (1 << 16); // [23:16] codec_std
    temp |= (Handle->instance_id << 0);  // [15: 0] Handle->instance_id
    VpuWriteReg(Handle->instance_id, W4_INST_INDEX, temp);

    // Work Buffer Setting
    VpuWriteReg(Handle->instance_id, W4_ADDR_WORK_BASE, Handle->work_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_WORK_SIZE, Handle->work_vb.size);
    VpuWriteReg(Handle->instance_id, W4_WORK_PARAM, 0);

    // Temp Buffer Setting
    VpuWriteReg(Handle->instance_id, W4_ADDR_TEMP_BASE, Handle->temp_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_TEMP_SIZE, Handle->temp_vb.size);
    VpuWriteReg(Handle->instance_id, W4_TEMP_PARAM, gol_endian);

    VpuWriteReg(Handle->instance_id, W4_ADDR_SEC_AXI, 0);
    VpuWriteReg(Handle->instance_id, W4_SEC_AXI_SIZE, sec_axi_buf_size);
    VpuWriteReg(Handle->instance_id, W4_USE_SEC_AXI, use_sec_axi);

    if (alloc) {
        memset((void*) &Handle->bs_vb, 0, sizeof(vpu_buffer_t));
        Handle->bs_vb.size = WAVE420L_STREAM_BUF_SIZE;
        Handle->bs_vb.cached = 1;
        if (vdi_allocate_dma_memory(Handle->instance_id, &Handle->bs_vb) < 0) {
            VLOG(ERR, "Wave4VpuEnCSeqInit  error Handle->bs_vb alloc\n");
            return AMVENC_MEMORY_FAIL;
        }
        /*TODO whether should indcude vdi_clear_memory */
        vdi_clear_memory(Handle->instance_id, Handle->bs_vb.phys_addr, Handle->bs_vb.size);
    }

    // Bitstream Buffer Setting
    VpuWriteReg(Handle->instance_id, W4_BS_START_ADDR, Handle->bs_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_BS_SIZE, Handle->bs_vb.size - 0x8000);
    VpuWriteReg(Handle->instance_id, W4_BS_PARAM, 0xf);
    VpuWriteReg(Handle->instance_id, W4_BS_OPTION, 0);

    VpuWriteReg(Handle->instance_id, W4_BS_RD_PTR, Handle->bs_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_BS_WR_PTR, Handle->bs_vb.phys_addr);

    // Report Buffer Setting
    //VpuWriteReg(Handle->instance_id, W4_CMD_ENC_ADDR_REPORT_BASE,   addr_report_buf);
    //VpuWriteReg(Handle->instance_id, W4_CMD_ENC_REPORT_SIZE,        report_buf_size);
    //VpuWriteReg(Handle->instance_id, W4_CMD_ENC_REPORT_PARAM,       report_endian);

    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SET_PARAM_OPTION, 0);
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SET_PARAM_ENABLE, (unsigned int )0xffffffff);

    VpuWriteReg(coreIdx, W4_CMD_ENC_SEQ_SRC_SIZE, (wave420l_align8(Handle->enc_height) << 16) | Handle->enc_width);
    VpuWriteReg(coreIdx, W4_CMD_ENC_SEQ_PARAM, 0x00020001); //8bit, profile 1:main, firmware determines a level, main tier.
    VpuWriteReg(coreIdx, W4_CMD_ENC_SEQ_GOP_PARAM, Handle->mGopIdx);

    temp = 0;
    temp |= Handle->idrPeriod << 16;
    temp |= 30 << 3;
    temp |= Handle->mEncParams.refresh_type; //[2:0] refresh type of intra picutre
    VpuWriteReg(coreIdx, W4_CMD_ENC_SEQ_INTRA_PARAM, temp);
    //VpuWriteReg(coreIdx, W4_CMD_ENC_SEQ_INTRA_PARAM, (Handle->idrPeriod << 16) | 0xf1);

    if (Handle->enc_height % 8)
        VpuWriteReg(coreIdx, W4_CMD_ENC_SEQ_CONF_WIN_TOP_BOT, (Handle->enc_height % 8) << 16);
    else
        VpuWriteReg(coreIdx, W4_CMD_ENC_SEQ_CONF_WIN_TOP_BOT, 0);
    VpuWriteReg(coreIdx, W4_CMD_ENC_SEQ_CONF_WIN_LEFT_RIGHT, 0);
    VpuWriteReg(coreIdx, W4_CMD_ENC_SEQ_FRAME_RATE, Handle->frame_rate);
    VpuWriteReg(coreIdx, W4_CMD_ENC_SEQ_INDEPENDENT_SLICE, 0);
    VpuWriteReg(coreIdx, W4_CMD_ENC_SEQ_DEPENDENT_SLICE, 0);
    VpuWriteReg(coreIdx, W4_CMD_ENC_SEQ_INTRA_REFRESH, 0);
    VpuWriteReg(coreIdx, W4_CMD_ENC_PARAM, 0x080104b0);
    VpuWriteReg(coreIdx, W4_CMD_ENC_RESERVED, 0);
    if (Handle->enc_counter > 0)
        VpuWriteReg(coreIdx,
                W4_CMD_ENC_RC_PARAM,
                (200 << 20) | (32 << 14) | (0 << 13) | (1 << 9) | (0 << 7) | (2 << 4) | 0xf);
        //enable rc, disable roi, initdelay 3000, initrc QP 63
    else
        VpuWriteReg(coreIdx, W4_CMD_ENC_RC_PARAM, 0xbb8fc22f); //enable rc, disable roi, initdelay 3000, initrc QP 63
    enable_roi = (0xbb8fc22f & (1 << 13)) >> 13;  // need check with cnm for bit13 or bit14
    VpuWriteReg(coreIdx, W4_CMD_ENC_RC_MIN_MAX_QP, 0x0000acc8);
    VpuWriteReg(coreIdx, W4_CMD_ENC_RC_BIT_RATIO_LAYER_0_3, 0x01010101);
    VpuWriteReg(coreIdx, W4_CMD_ENC_RC_BIT_RATIO_LAYER_4_7, 0x01010101);
    VpuWriteReg(coreIdx, W4_CMD_ENC_NR_PARAM, 0x00000008);
    VpuWriteReg(coreIdx, W4_CMD_ENC_NR_WEIGHT, 0x08421ce7);
    VpuWriteReg(coreIdx, W4_CMD_ENC_NUM_UNITS_IN_TICK, 0);
    VpuWriteReg(coreIdx, W4_CMD_ENC_TIME_SCALE, 0);
    VpuWriteReg(coreIdx, W4_CMD_ENC_NUM_TICKS_POC_DIFF_ONE, 0);
    VpuWriteReg(coreIdx, W4_CMD_ENC_RC_TRANS_RATE, Handle->bitrate * 6 / 5);
    VpuWriteReg(coreIdx, W4_CMD_ENC_RC_TARGET_RATE, Handle->bitrate);
    //VpuWriteReg(coreIdx, W4_CMD_ENC_RC_TRANS_RATE,  bitrate + bitrate / 5);
    //VpuWriteReg(coreIdx, W4_CMD_ENC_RC_TARGET_RATE,  bitrate);
    VpuWriteReg(coreIdx, W4_CMD_ENC_ROT_PARAM, rotMirMode);

    // issue command request
    VpuWriteReg(Handle->instance_id, W4_VPU_BUSY_STATUS, 1);
    VpuWriteReg(Handle->instance_id, W4_RET_SUCCESS, 0);  //for debug
    VpuWriteReg(Handle->instance_id, W4_CORE_INDEX, 0);
    //VpuWriteReg(Handle->instance_id, W4_INST_INDEX,  (Handle->instance_id&0xffff)|(1<<16));

    VpuWriteReg(Handle->instance_id, W4_COMMAND, SET_PARAM);
    VpuWriteReg(Handle->instance_id, W4_VPU_HOST_INT_REQ, 1);
    //Wave4BitIssueCommand(0, 4, CREATE_INSTANCE);
    temp = VpuReadReg(Handle->instance_id, W4_VPU_BUSY_STATUS);
    VLOG(NONE, "W4_VPU_BUSY_STATUS :0x%x\n", temp);
    if (vdi_wait_interrupt(Handle->instance_id, VPU_BUSY_CHECK_TIMEOUT) == -1) {
        VLOG(ERR, "Wave4VpuEnCSeqInit error time out\n");
        return AMVENC_TIMEOUT;
    }

    if (VpuReadReg(Handle->instance_id, W4_RET_SUCCESS) == 0) {
        uint32_t reasonCode = VpuReadReg(Handle->instance_id, W4_RET_FAIL_REASON);
        VLOG(ERR, "Wave4VpuEnCSeqInit failedREASON CODE(%08x)\n", reasonCode);
        return AMVENC_HARDWARE;
    }
    temp = VpuReadReg(Handle->instance_id, W4_RET_SUCCESS);
    VLOG(DEBUG, "W4_RET_SUCCESS %x\n", temp);
    rd_ptr = VpuReadReg(Handle->instance_id, W4_BS_RD_PTR);
    wr_ptr = VpuReadReg(Handle->instance_id, W4_BS_WR_PTR);
    wr_ptr = VpuReadReg(Handle->instance_id, W4_BS_WR_PTR);
    src_num = VpuReadReg(Handle->instance_id, W4_RET_ENC_MIN_SRC_BUF_NUM);
    fb_num = VpuReadReg(Handle->instance_id, W4_RET_ENC_MIN_FB_NUM);
    VLOG(DEBUG, "Wave4VpuEnCSeqInit, rd:0x%x, wr:0x%x, src:%d, fb:%d\n", rd_ptr, wr_ptr, src_num, fb_num);
    VLOG(DEBUG, "Wave4VpuEnCSeqInit- End\n");
    return AMVENC_SUCCESS;
}

AMVEnc_Status Wave4VpuEncSeqSet(AMVHEVCEncHandle *Handle) {
    int coreIdx = 0;
    Uint32 addr_report_buf = 0;    // [31:0]
    Uint32 report_buf_size = 0;    // [31:0]
    unsigned char report_endian = 0;    // [ 3:0]
    Uint32 use_sec_axi = 0;    // [31:0]
    Uint32 sec_axi_buf_size = 0x00012800;   // 74KB
    Uint32 temp;   // [31:0]
    Uint32 rd_ptr, wr_ptr, src_num, fb_num, i, j;   // [31:0]

    rd_ptr = VpuReadReg(Handle->instance_id, W4_BS_RD_PTR);
    wr_ptr = VpuReadReg(Handle->instance_id, W4_BS_WR_PTR);
    VLOG(DEBUG, "Wave4VpuEnCSeqSet, rd:0x%x, wr:0x%x\n", rd_ptr, wr_ptr);

    temp = 0;
    temp |= (0 << 24); // [27:24] codec_std_aux
    temp |= (1 << 16); // [23:16] codec_std
    temp |= (Handle->instance_id << 0);  // [15: 0] Handle->instance_id
    VpuWriteReg(Handle->instance_id, W4_INST_INDEX, temp);

    // Work Buffer Setting
    VpuWriteReg(Handle->instance_id, W4_ADDR_WORK_BASE, Handle->work_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_WORK_SIZE, Handle->work_vb.size);
    VpuWriteReg(Handle->instance_id, W4_WORK_PARAM, 0);

    // Temp Buffer Setting
    VpuWriteReg(Handle->instance_id, W4_ADDR_TEMP_BASE, Handle->temp_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_TEMP_SIZE, Handle->temp_vb.size);
    VpuWriteReg(Handle->instance_id, W4_TEMP_PARAM, gol_endian);

    VpuWriteReg(Handle->instance_id, W4_ADDR_SEC_AXI, 0);
    VpuWriteReg(Handle->instance_id, W4_SEC_AXI_SIZE, sec_axi_buf_size);
    VpuWriteReg(Handle->instance_id, W4_USE_SEC_AXI, use_sec_axi);

    // Bitstream Buffer Setting
    VpuWriteReg(Handle->instance_id, W4_BS_START_ADDR, Handle->bs_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_BS_SIZE, Handle->bs_vb.size - 0x8000);
    VpuWriteReg(Handle->instance_id, W4_BS_PARAM, 0xf);
    VpuWriteReg(Handle->instance_id, W4_BS_OPTION, 0);

    VpuWriteReg(Handle->instance_id, W4_BS_RD_PTR, rd_ptr);
    VpuWriteReg(Handle->instance_id, W4_BS_WR_PTR, wr_ptr);

    // Report Buffer Setting
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_ADDR_REPORT_BASE, addr_report_buf);
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_REPORT_SIZE, report_buf_size);
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_REPORT_PARAM, report_endian);

    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SET_PARAM_OPTION, 1);
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SET_PARAM_ENABLE, (unsigned int )0xffffffff);
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_CUSTOM_GOP_PARAM, 0x11);
    for (i = 0; i < 1; i++) {
        VpuWriteReg(coreIdx, W4_CMD_ENC_CUSTOM_GOP_PIC_PARAM_0 + (i*4), 0x785);
        VpuWriteReg(coreIdx, W4_CMD_ENC_CUSTOM_GOP_PIC_LAMBDA_0 + (i*4), 0);
    }

    for (j = i; j < 8; j++) {
        VpuWriteReg(coreIdx, W4_CMD_ENC_CUSTOM_GOP_PIC_PARAM_0 + (j*4), 0);
        VpuWriteReg(coreIdx, W4_CMD_ENC_CUSTOM_GOP_PIC_LAMBDA_0 + (j*4), 0);
    }

    // issue command request
    VpuWriteReg(Handle->instance_id, W4_VPU_BUSY_STATUS, 1);
    VpuWriteReg(Handle->instance_id, W4_RET_SUCCESS, 0);  //for debug
    VpuWriteReg(Handle->instance_id, W4_CORE_INDEX, 0);
    //VpuWriteReg(Handle->instance_id, W4_INST_INDEX,  (Handle->instance_id&0xffff)|(1<<16));

    VpuWriteReg(Handle->instance_id, W4_COMMAND, SET_PARAM);
    VpuWriteReg(Handle->instance_id, W4_VPU_HOST_INT_REQ, 1);
    temp = VpuReadReg(Handle->instance_id, W4_VPU_BUSY_STATUS);
    VLOG(DEBUG, "W4_VPU_BUSY_STATUS :0x%x\n", temp);
    if (vdi_wait_interrupt(Handle->instance_id, VPU_BUSY_CHECK_TIMEOUT) == -1) {
        VLOG(ERR, "Wave4VpuEnCSeqInit error time out\n");
        return AMVENC_TIMEOUT;
    }

    if (VpuReadReg(Handle->instance_id, W4_RET_SUCCESS) == 0) {
        uint32_t reasonCode = VpuReadReg(Handle->instance_id, W4_RET_FAIL_REASON);
        VLOG(ERR, "Wave4VpuEnCSeqSet failedREASON CODE(%08x)\n", reasonCode);
        return AMVENC_HARDWARE;
    }
    temp = VpuReadReg(Handle->instance_id, W4_RET_SUCCESS);
    VLOG(DEBUG, "W4_RET_SUCCESS %x\n", temp);
    rd_ptr = VpuReadReg(Handle->instance_id, W4_BS_RD_PTR);
    wr_ptr = VpuReadReg(Handle->instance_id, W4_BS_WR_PTR);
    wr_ptr = VpuReadReg(Handle->instance_id, W4_BS_WR_PTR);
    src_num = VpuReadReg(Handle->instance_id, W4_RET_ENC_MIN_SRC_BUF_NUM);
    fb_num = VpuReadReg(Handle->instance_id, W4_RET_ENC_MIN_FB_NUM);
    VLOG(DEBUG, "Wave4VpuEnCSeqSet, rd:0x%x, wr:0x%x, src:%d, fb:%d\n", rd_ptr, wr_ptr, src_num, fb_num);
    VLOG(DEBUG, "Wave4VpuEnCSeqSet- End\n");
    return AMVENC_SUCCESS;
}

AMVEnc_Status Wave4VpuEncRegisterFrame(AMVHEVCEncHandle *Handle, int alloc) {
    //Uint32 addr_report_buf = 0;    // [31:0]
    //Uint32 report_buf_size = 0;    // [31:0]
    //unsigned char report_endian = 0;    // [ 3:0]
    //Uint32 use_sec_axi = 0;    // [31:0]
    //Uint32 sec_axi_buf_size = 0x00012800;   // 74KB
    Uint32 temp;   // [31:0]
    Uint32 rd_ptr, wr_ptr, src_num, fb_num, i;   // [31:0
    Uint32 src_stride;
    Uint32 luma_stride, chroma_stride;
    Uint32 size_src_luma, size_src_chroma;
    Uint32 fb_width, fb_height, fb_stride;
    Uint32 fbc_luma_stride, fbc_chroma_stride;
    Uint32 size_fbc_luma, size_fbc_chroma;
    Uint32 size_fbc_luma_offset, size_fbc_chroma_offset;
    Uint32 size_mv_col, size_s2;

    Uint32 src_fb_addr[4][3];
    //Uint32 s2_addr;
    Uint32 fbc_fb_addr[32][4];

    Uint32 fb_set_start, fb_set_end;
    Uint32 fb_start_idx, fb_end_idx;
    //Uint32 addr_fb_arr[32];
    //Uint32 addr_mv_arr[8];
    rd_ptr = VpuReadReg(Handle->instance_id, W4_BS_RD_PTR);
    wr_ptr = VpuReadReg(Handle->instance_id, W4_BS_WR_PTR);

    src_num = VpuReadReg(Handle->instance_id, W4_RET_ENC_MIN_SRC_BUF_NUM);
    fb_num = VpuReadReg(Handle->instance_id, W4_RET_ENC_MIN_FB_NUM);
    VLOG(DEBUG, "Wave4VpuEncRegisterFrame, rd:0x%x, wr:0x%x, src:%d, fb:%d\n", rd_ptr, wr_ptr, src_num, fb_num);
    if (src_num > 4)
        src_num = 4;
    if (fb_num > 4)
        fb_num = 4;
    Handle->src_num = src_num;
    Handle->fb_num = fb_num;

    if (alloc) {
        for (i = 0; i < 4; i++) {
            memset((void*) &Handle->src_vb[i], 0, sizeof(vpu_buffer_t));
            memset((void*) &Handle->fb_vb[i], 0, sizeof(vpu_buffer_t));
        }
        memset((void*) &Handle->fbc_ltable_vb, 0, sizeof(vpu_buffer_t));
        memset((void*) &Handle->fbc_ctable_vb, 0, sizeof(vpu_buffer_t));
        memset((void*) &Handle->fbc_mv_vb, 0, sizeof(vpu_buffer_t));
        memset((void*) &Handle->subsample_vb, 0, sizeof(vpu_buffer_t));
    }

    src_stride = wave420l_align32(Handle->enc_width);
    luma_stride = (src_format & 0x4) ? src_stride * 2 : src_stride;
    chroma_stride = src_stride;

    // set frame buffers

    // calculate required buffer size
    size_src_luma = luma_stride * wave420l_align32(Handle->enc_height);

    size_src_chroma = luma_stride * (wave420l_align16(Handle->enc_height) / 2);

    fb_width = wave420l_align32(Handle->enc_width);
    fb_height = wave420l_align8(Handle->enc_height);
    fb_stride = src_stride;

    // bit-depth = 8
    fbc_luma_stride = wave420l_align32(wave420l_align16(fb_width&0x3fff)*4);
    fbc_chroma_stride = wave420l_align32(wave420l_align16((fb_width>>1)&0x1fff)*4);

    size_fbc_luma = fbc_luma_stride * (fb_height / 4);
    size_fbc_chroma = fbc_chroma_stride * (fb_height / 4);
    size_fbc_luma_offset = wave420l_align256(fb_width ) * wave420l_align16(fb_height) / 32;
    size_fbc_luma_offset = wave420l_align16(size_fbc_luma_offset);
    size_fbc_chroma_offset = wave420l_align256(fb_width/2) * wave420l_align16(fb_height) / 32;
    size_fbc_chroma_offset = wave420l_align16(size_fbc_chroma_offset);

    size_mv_col = wave420l_align64(fb_width) * wave420l_align64(fb_height) / 16 + 64;
    size_mv_col = wave420l_align16(size_mv_col);
    size_s2 = wave420l_align16((fb_width>>2)&0xfff) * wave420l_align8((fb_height >> 2) & 0xfff);
    size_s2 = wave420l_align16(size_s2);
    for (i = 0; i < src_num; i++) {
        if (alloc) {
            memset((void*) &Handle->src_vb[i], 0, sizeof(vpu_buffer_t));
            Handle->src_vb[i].size = size_src_luma + size_src_chroma;
            Handle->src_vb[i].size = ((Handle->src_vb[i].size + 4095) & ~4095) + 4096;
            Handle->src_vb[i].cached = 1;
            if (vdi_allocate_dma_memory(Handle->instance_id, &Handle->src_vb[i]) < 0) {
                VLOG(ERR, "Wave4VpuEncRegisterFrame  error Handle->src_vb alloc\n");
                return AMVENC_MEMORY_FAIL;
            }
            vdi_clear_memory(Handle->instance_id, Handle->src_vb[i].phys_addr, Handle->src_vb[i].size);
        }
        src_fb_addr[i][0] = Handle->src_vb[i].phys_addr;
        src_fb_addr[i][1] = Handle->src_vb[i].phys_addr + size_src_luma;
        src_fb_addr[i][2] = 0;
    }

    for (i = 0; i < fb_num; i++) {
        if (alloc) {
            memset((void*) &Handle->fb_vb[i], 0, sizeof(vpu_buffer_t));
            Handle->fb_vb[i].size = size_fbc_luma + size_fbc_chroma;
            Handle->fb_vb[i].size = ((Handle->fb_vb[i].size + 4095) & ~4095) + 4096;
            if (vdi_allocate_dma_memory(Handle->instance_id, &Handle->fb_vb[i]) < 0) {
                VLOG(ERR, "Wave4VpuEncRegisterFrame  error Handle->fb_vb alloc\n");
                return AMVENC_MEMORY_FAIL;
            }
            vdi_clear_memory(Handle->instance_id, Handle->fb_vb[i].phys_addr, Handle->fb_vb[i].size);
        }
        fbc_fb_addr[i][0] = Handle->fb_vb[i].phys_addr;
        fbc_fb_addr[i][1] = Handle->fb_vb[i].phys_addr + size_fbc_luma;
        fbc_fb_addr[i][2] = 0;
    }

    if (alloc) {
        memset((void*) &Handle->fbc_ltable_vb, 0, sizeof(vpu_buffer_t));
        Handle->fbc_ltable_vb.size = ((size_fbc_luma_offset * fb_num + 4095) & ~4095) + 4096;
        if (vdi_allocate_dma_memory(Handle->instance_id, &Handle->fbc_ltable_vb) < 0) {
            VLOG(ERR, "Wave4VpuEncRegisterFrame  error Handle->fbc_ltable_vb alloc\n");
            return AMVENC_MEMORY_FAIL;
        }
        vdi_clear_memory(Handle->instance_id, Handle->fbc_ltable_vb.phys_addr, Handle->fbc_ltable_vb.size);

        memset((void*) &Handle->fbc_ctable_vb, 0, sizeof(vpu_buffer_t));
        Handle->fbc_ctable_vb.size = ((size_fbc_chroma_offset * fb_num + 4095) & ~4095) + 4096;
        if (vdi_allocate_dma_memory(Handle->instance_id, &Handle->fbc_ctable_vb) < 0) {
            VLOG(ERR, "Wave4VpuEncRegisterFrame  error Handle->fbc_ctable_vb alloc\n");
            return AMVENC_MEMORY_FAIL;
        }
        vdi_clear_memory(Handle->instance_id, Handle->fbc_ctable_vb.phys_addr, Handle->fbc_ctable_vb.size);

        memset((void*) &Handle->fbc_mv_vb, 0, sizeof(vpu_buffer_t));
        Handle->fbc_mv_vb.size = ((size_mv_col * fb_num + 4095) & ~4095) + 4096;
        if (vdi_allocate_dma_memory(Handle->instance_id, &Handle->fbc_mv_vb) < 0) {
            VLOG(ERR, "Wave4VpuEncRegisterFrame  error Handle->fbc_mv_vb alloc\n");
            return AMVENC_MEMORY_FAIL;
        }
        vdi_clear_memory(Handle->instance_id, Handle->fbc_mv_vb.phys_addr, Handle->fbc_mv_vb.size);

        memset((void*) &Handle->subsample_vb, 0, sizeof(vpu_buffer_t));
        Handle->subsample_vb.size = ((size_s2 + 4095) & ~4095) + 4096;
        if (vdi_allocate_dma_memory(Handle->instance_id, &Handle->subsample_vb) < 0) {
            VLOG(ERR, "Wave4VpuEncRegisterFrame  error Handle->subsample_vb alloc\n");
            return AMVENC_MEMORY_FAIL;
        }
        vdi_clear_memory(Handle->instance_id, Handle->subsample_vb.phys_addr, Handle->subsample_vb.size);
    }

    temp = 0;
    temp |= (0 << 24); // [27:24] codec_std_aux
    temp |= (1 << 16); // [23:16] codec_std
    temp |= (Handle->instance_id << 0);  // [15: 0] Handle->instance_id
    VpuWriteReg(Handle->instance_id, W4_INST_INDEX, temp);

    // Work Buffer Setting
    VpuWriteReg(Handle->instance_id, W4_ADDR_WORK_BASE, Handle->work_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_WORK_SIZE, Handle->work_vb.size);
    VpuWriteReg(Handle->instance_id, W4_WORK_PARAM, 0);

    fb_set_end = 1;
    fb_set_start = 1;
    fb_start_idx = 0;
    fb_end_idx = fb_num - 1;
    temp = 0;
    temp |= ((fbc_luma_stride & 0xffff) << 16); // [31:16] fbc_luma_stride
    temp |= ((fbc_chroma_stride & 0xffff) << 0);  // [15: 0] fbc_chroma_stride
    VpuWriteReg(Handle->instance_id, W4_FBC_STRIDE, temp);

    temp = 0;
    temp |= (0 << 30); // [   30] fbc_disable
    temp |= (0 << 29); // [   29] nv21_flag
    temp |= (0 << 28); // [   28] bwb_flag
    temp |= (0 << 24); // [27:24] axi_id
    temp |= ((0 & 0xf) << 20); // [23:20] bwb_format[3:0]
    temp |= (0 << 16); // [   16] cbcr_interlv
    temp |= ((fb_stride & 0xffff) << 0);  // [15: 0] fb_stride
    VpuWriteReg(Handle->instance_id, W4_COMMON_PIC_INFO, temp);

    temp = 0;
    if (Handle->bufType == DMA_BUFF) {
        temp |= (wave420l_align16(Handle->enc_width) << 16); // [31:16]
    } else {
        temp |= (wave420l_align32(Handle->enc_width) << 16); // [31:16]
    }
    temp |= (wave420l_align8(Handle->enc_height) << 0);  // [15: 0]
    VpuWriteReg(Handle->instance_id, W4_PIC_SIZE, temp);

    temp = 0;
    temp |= (0xc << 20); // [25:20] fbc_mode[5:0]
    temp |= (gol_endian << 16); // [19:16] fb_endian[3:0]
    temp |= (fb_set_end << 4);  // [    4] fb_set_end
    temp |= (fb_set_start << 3);  // [    3] fb_set_start
    VpuWriteReg(Handle->instance_id, W4_SFB_OPTION, temp);
    temp = 0;
    temp |= (fb_start_idx << 8);  // [12: 8]
    temp |= (fb_end_idx << 0);  // [ 4: 0]
    VpuWriteReg(Handle->instance_id, W4_SET_FB_NUM, temp);

    for (i = 0; i < fb_num; i++) {
        VpuWriteReg(Handle->instance_id, W4_ADDR_LUMA_BASE0 +(i*16), fbc_fb_addr[i][0]);
        VpuWriteReg(Handle->instance_id, W4_ADDR_CB_BASE0 +(i*16), fbc_fb_addr[i][1]);
        VpuWriteReg(Handle->instance_id, W4_ADDR_FBC_Y_OFFSET0 +(i*16), Handle->fbc_ltable_vb.phys_addr + size_fbc_luma_offset * i);
        VpuWriteReg(Handle->instance_id, W4_ADDR_FBC_C_OFFSET0 +(i*16), Handle->fbc_ctable_vb.phys_addr + size_fbc_chroma_offset * i);
        VpuWriteReg(Handle->instance_id, W4_ADDR_MV_COL0+(i*4), Handle->fbc_mv_vb.phys_addr + size_mv_col * i);
    }

    VpuWriteReg(Handle->instance_id, W4_ADDR_SUB_SAMPLED_FB_BASE, Handle->subsample_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_SUB_SAMPLED_ONE_FB_SIZE, size_s2);

    // issue command request
    VpuWriteReg(Handle->instance_id, W4_VPU_BUSY_STATUS, 1);
    VpuWriteReg(Handle->instance_id, W4_RET_SUCCESS, 0);  //for debug
    VpuWriteReg(Handle->instance_id, W4_CORE_INDEX, 0);
    VpuWriteReg(Handle->instance_id, W4_COMMAND, SET_FRAMEBUF);
    VpuWriteReg(Handle->instance_id, W4_VPU_HOST_INT_REQ, 1);
    temp = VpuReadReg(Handle->instance_id, W4_VPU_BUSY_STATUS);
    VLOG(DEBUG, "W4_VPU_BUSY_STATUS :0x%x\n", temp);
    if (vdi_wait_interrupt(Handle->instance_id, VPU_BUSY_CHECK_TIMEOUT) == -1) {
        VLOG(ERR, "Wave4VpuEncRegisterFrame error time out\n");
        return AMVENC_TIMEOUT;
    }

    if (VpuReadReg(Handle->instance_id, W4_RET_SUCCESS) == 0) {
        uint32_t reasonCode = VpuReadReg(Handle->instance_id, W4_RET_FAIL_REASON);
        VLOG(ERR, "Wave4VpuEncRegisterFrame failedREASON CODE(%08x)\n", reasonCode);
        return AMVENC_HARDWARE;
    }
    temp = VpuReadReg(Handle->instance_id, W4_RET_SUCCESS);
    VLOG(DEBUG, "W4_RET_SUCCESS %x\n", temp);
    rd_ptr = VpuReadReg(Handle->instance_id, W4_BS_RD_PTR);
    wr_ptr = VpuReadReg(Handle->instance_id, W4_BS_WR_PTR);
    wr_ptr = VpuReadReg(Handle->instance_id, W4_BS_WR_PTR);
    src_num = VpuReadReg(Handle->instance_id, W4_RET_ENC_MIN_SRC_BUF_NUM);
    fb_num = VpuReadReg(Handle->instance_id, W4_RET_ENC_MIN_FB_NUM);
    VLOG(DEBUG, "Wave4VpuEncRegisterFrame, rd:0x%x, wr:0x%x, src:%d, fb:%d\n", rd_ptr, wr_ptr, src_num, fb_num);
    VLOG(DEBUG, "Wave4VpuEncRegisterFrame- End\n");
    return AMVENC_SUCCESS;
}

AMVEnc_Status Wave4VpuEncGetHeader(AMVHEVCEncHandle *Handle, unsigned char *buffer, unsigned int *buf_nal_size) {
    //int coreIdx = 0;
    Uint32 addr_report_buf = 0;    // [31:0]
    Uint32 report_buf_size = 0;    // [31:0]
    unsigned char report_endian = 0;    // [ 3:0]
    //Uint32 use_sec_axi = 0;    // [31:0]
    //Uint32 sec_axi_buf_size = 0x00012800;   // 74KB
    Uint32 temp;   // [31:0]
    Uint32 rd_ptr, wr_ptr;// src_num, fb_num, i, j;   // [31:0]

    // Bitstream Buffer Setting
    VpuWriteReg(Handle->instance_id, W4_BS_START_ADDR, Handle->bs_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_BS_SIZE, Handle->bs_vb.size - 0x8000);
    VpuWriteReg(Handle->instance_id, W4_BS_PARAM, 0xf);
    VpuWriteReg(Handle->instance_id, W4_BS_OPTION, 0);

    temp = 0;
    temp |= (0 << 24); // [27:24] codec_std_aux
    temp |= (1 << 16); // [23:16] codec_std
    temp |= (Handle->instance_id << 0);  // [15: 0] Handle->instance_id
    VpuWriteReg(Handle->instance_id, W4_INST_INDEX, temp);

    // Work Buffer Setting
    VpuWriteReg(Handle->instance_id, W4_ADDR_WORK_BASE, Handle->work_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_WORK_SIZE, Handle->work_vb.size);
    VpuWriteReg(Handle->instance_id, W4_WORK_PARAM, 0);

    // Temp Buffer Setting
    VpuWriteReg(Handle->instance_id, W4_ADDR_TEMP_BASE, Handle->temp_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_TEMP_SIZE, Handle->temp_vb.size);
    VpuWriteReg(Handle->instance_id, W4_TEMP_PARAM, gol_endian);

    VpuWriteReg(Handle->instance_id, W4_BS_RD_PTR, Handle->bs_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_BS_WR_PTR, Handle->bs_vb.phys_addr);

    /* Secondary AXI */
    VpuWriteReg(Handle->instance_id, W4_ADDR_SEC_AXI, 0);
    VpuWriteReg(Handle->instance_id, W4_SEC_AXI_SIZE, 0);
    VpuWriteReg(Handle->instance_id, W4_USE_SEC_AXI, 0);
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_ADDR_REPORT_BASE, addr_report_buf);
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_REPORT_SIZE, report_buf_size);
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_REPORT_PARAM, report_endian);

    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SRC_PIC_IDX, 0);
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_CODE_OPTION, 0x1c);

    VpuWriteReg(Handle->instance_id, W4_VPU_BUSY_STATUS, 1);
    VpuWriteReg(Handle->instance_id, W4_RET_SUCCESS, 0);  //for debug
    VpuWriteReg(Handle->instance_id, W4_CORE_INDEX, 0);
    VpuWriteReg(Handle->instance_id, W4_COMMAND, ENC_PIC);
    VpuWriteReg(Handle->instance_id, W4_VPU_HOST_INT_REQ, 1);
    temp = VpuReadReg(Handle->instance_id, W4_VPU_BUSY_STATUS);
    VLOG(DEBUG, "W4_VPU_BUSY_STATUS :0x%x\n", temp);
    if (vdi_wait_interrupt(Handle->instance_id, VPU_BUSY_CHECK_TIMEOUT) == -1) {
        VLOG(ERR, "Wave4VpuEncGetHeader error time out\n");
        return AMVENC_TIMEOUT;
    }

    if (VpuReadReg(Handle->instance_id, W4_RET_SUCCESS) == 0) {
        uint32_t reasonCode = VpuReadReg(Handle->instance_id, W4_RET_FAIL_REASON);
        VLOG(ERR, "Wave4VpuEncGetHeader failedREASON CODE(%08x)\n", reasonCode);
        return AMVENC_HARDWARE;
    }
    temp = VpuReadReg(Handle->instance_id, W4_RET_SUCCESS);
    VLOG(DEBUG, "W4_RET_SUCCESS %x\n", temp);
    rd_ptr = VpuReadReg(Handle->instance_id, W4_BS_RD_PTR);
    wr_ptr = VpuReadReg(Handle->instance_id, W4_BS_WR_PTR);
    temp = VpuReadReg(Handle->instance_id, W4_RET_ENC_PIC_BYTE);

    if (temp > Handle->mOutputBufferLen) {
        VLOG(ERR, "nal size %d bigger than output buffer %d!\n", temp, Handle->mOutputBufferLen);
        return AMVENC_OVERFLOW;
    }
    *buf_nal_size = temp;
    vdi_read_memory(Handle->instance_id, rd_ptr, (unsigned char *) buffer, temp);
    VpuWriteReg(Handle->instance_id, W4_BS_RD_PTR, wr_ptr);
    VLOG(DEBUG, "Wave4VpuEncGetHeader, rd:0x%x, wr:0x%x, ret bytes: %d, buffer size:%d\n", rd_ptr, wr_ptr, temp, wr_ptr - rd_ptr);
    return AMVENC_SUCCESS;
}

AMVEnc_Status Wave4VpuEncEncPic(AMVHEVCEncHandle *Handle, Uint32 idx, int end, unsigned char *buffer, unsigned int *buf_nal_size, int *nal_type) {
    int coreIdx = 0;
    Uint32 addr_report_buf = 0;    // [31:0]
    Uint32 report_buf_size = 0;    // [31:0]
    unsigned char report_endian = 0;    // [ 3:0]
    //Uint32 use_sec_axi = 0;    // [31:0]
    //Uint32 sec_axi_buf_size = 0x00012800;   // 74KB
    Uint32 temp;   // [31:0]
    Uint32 rd_ptr, wr_ptr;
    //Uint32 src_num, fb_num, i, j;   // [31:0]
    Uint32 src_stride;
    Uint32 luma_stride, chroma_stride;
    Uint32 size_src_luma, size_src_chroma;
   // char *y = NULL;
    if (Handle->bufType == DMA_BUFF) {
        src_stride = wave420l_align16(Handle->enc_width);
    } else {
        src_stride = wave420l_align32(Handle->enc_width);
    }
    luma_stride = src_stride;
    chroma_stride = src_stride;

    // set frame buffers

    // calculate required buffer size
    if (Handle->bufType == DMA_BUFF) {
        size_src_luma = luma_stride * wave420l_align16(Handle->enc_height);
    } else {
        size_src_luma = luma_stride * wave420l_align32(Handle->enc_height);
    }

    size_src_chroma = luma_stride * (wave420l_align16(Handle->enc_height) / 2);

    // Bitstream Buffer Setting
    VpuWriteReg(Handle->instance_id, W4_BS_START_ADDR, Handle->bs_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_BS_SIZE, Handle->bs_vb.size - 0x8000);
    VpuWriteReg(Handle->instance_id, W4_BS_PARAM, 0xf);
    VpuWriteReg(Handle->instance_id, W4_BS_OPTION, 0);

    temp = 0;
    temp |= (0 << 24); // [27:24] codec_std_aux
    temp |= (1 << 16); // [23:16] codec_std
    temp |= (Handle->instance_id << 0);  // [15: 0] Handle->instance_id
    VpuWriteReg(Handle->instance_id, W4_INST_INDEX, temp);

    // Work Buffer Setting
    VpuWriteReg(Handle->instance_id, W4_ADDR_WORK_BASE, Handle->work_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_WORK_SIZE, Handle->work_vb.size);
    VpuWriteReg(Handle->instance_id, W4_WORK_PARAM, 0);

    // Temp Buffer Setting
    VpuWriteReg(Handle->instance_id, W4_ADDR_TEMP_BASE, Handle->temp_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_TEMP_SIZE, Handle->temp_vb.size);
    VpuWriteReg(Handle->instance_id, W4_TEMP_PARAM, gol_endian);

    VpuWriteReg(Handle->instance_id, W4_BS_RD_PTR, Handle->bs_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_BS_WR_PTR, Handle->bs_vb.phys_addr);

    /* Secondary AXI */
    VpuWriteReg(Handle->instance_id, W4_ADDR_SEC_AXI, 0);
    VpuWriteReg(Handle->instance_id, W4_SEC_AXI_SIZE, 0);
    VpuWriteReg(Handle->instance_id, W4_USE_SEC_AXI, 0);

    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_ADDR_REPORT_BASE, addr_report_buf);
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_REPORT_SIZE, report_buf_size);
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_REPORT_PARAM, report_endian);

    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_CODE_OPTION, 3);
    if (Handle->idrPeriod == -1 || (Handle->op_flag & AMVEncFrameIO_FORCE_IDR_FLAG))
        VpuWriteReg(Handle->instance_id, W4_CMD_ENC_PIC_PARAM, (3 << 21) | (1 << 20));
    else
        VpuWriteReg(Handle->instance_id, W4_CMD_ENC_PIC_PARAM, 0);
    if (end)
        VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SRC_PIC_IDX, 0xffffffff);
    else
        VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SRC_PIC_IDX, idx);
    if (Handle->bufType == DMA_BUFF) {
        if (buffer != NULL) {
          vpu_dma_buf_info_t info;
          info.width = wave420l_align16(Handle->enc_width);
          info.height = Handle->enc_height;
          info.fmt = Handle->fmt;
          info.num_planes = Handle->mNumPlanes;
          info.fd[0] = Handle->shared_fd[0];
          info.fd[1] = Handle->shared_fd[1];
          info.fd[2] = Handle->shared_fd[2];
          vdi_config_dma(Handle->instance_id, info);
        }
    } else {
        VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SRC_ADDR_Y, Handle->src_vb[idx].phys_addr);
        VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SRC_ADDR_U, Handle->src_vb[idx].phys_addr + size_src_luma);
        if (Handle->fmt == AMVENC_YUV420) {
          VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SRC_ADDR_V, Handle->src_vb[idx].phys_addr + size_src_luma + size_src_luma / 4);
        } else {//nv12,nv21
          VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SRC_ADDR_V, Handle->src_vb[idx].phys_addr + size_src_luma);
        }
    }
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SRC_STRIDE, (src_stride << 16) | src_stride);
    if (Handle->fmt == AMVENC_YUV420) {
        VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SRC_FORMAT, ((0 << 0) | (0 << 3) | (gol_endian << 6)));
    } else {//nv12,nv21
        VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SRC_FORMAT, (((2 + Handle->mUvSwap) << 0) | (0 << 3) | (gol_endian << 6)));
    }

    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SEI_USER_ADDR, 0);
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SEI_USER_INFO, 0);
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SRC_TIMESTAMP_LOW, 0);
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SRC_TIMESTAMP_HIGH, 0);
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_LONGTERM_PIC, 0);
    // set ROI param
    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_ROI_PARAM, 0);

    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_ROI_ADDR_CTU_MAP, 0);

    VpuWriteReg(Handle->instance_id, W4_CMD_ENC_SUB_FRAME_SYNC_CONFIG, 0);

    VpuWriteReg(Handle->instance_id, W4_VPU_BUSY_STATUS, 1);
    VpuWriteReg(Handle->instance_id, W4_RET_SUCCESS, 0);  //for debug
    VpuWriteReg(Handle->instance_id, W4_CORE_INDEX, 0);
    VpuWriteReg(Handle->instance_id, W4_RET_FAIL_REASON, 0);  //for debug
    VpuWriteReg(Handle->instance_id, W4_COMMAND, ENC_PIC);
    VpuWriteReg(Handle->instance_id, W4_VPU_HOST_INT_REQ, 1);
    temp = VpuReadReg(Handle->instance_id, W4_VPU_BUSY_STATUS);
    VLOG(NONE, "W4_VPU_BUSY_STATUS :0x%x\n", temp);
    if (vdi_wait_interrupt(Handle->instance_id, VPU_BUSY_CHECK_TIMEOUT) == -1) {
        VLOG(ERR, "Wave4VpuEncEncPic error time out\n");
        Wave4VpuReset(Handle, coreIdx, reset_mode);
        if (Handle->bufType == DMA_BUFF && buffer != NULL) {
            vdi_unmap_dma(Handle->instance_id);
        }
        return AMVENC_TIMEOUT;
    }
    if (Handle->bufType == DMA_BUFF && buffer != NULL) {
        vdi_unmap_dma(Handle->instance_id);
    }
    if (VpuReadReg(Handle->instance_id, W4_RET_SUCCESS) == 0) {
        uint32_t reasonCode = VpuReadReg(Handle->instance_id, W4_RET_FAIL_REASON);
        VLOG(ERR, "Wave4VpuEncEncPic failedREASON CODE(%08x)\n", reasonCode);
        Wave4VpuReset(Handle, coreIdx, reset_mode);
        return AMVENC_HARDWARE;
    }
    temp = VpuReadReg(Handle->instance_id, W4_RET_SUCCESS);
    VLOG(ERR, "W4_RET_SUCCESS %x\n", temp);
    rd_ptr = VpuReadReg(Handle->instance_id, W4_BS_RD_PTR);
    wr_ptr = VpuReadReg(Handle->instance_id, W4_BS_WR_PTR);
    temp = VpuReadReg(Handle->instance_id, W4_RET_ENC_PIC_BYTE);
    VLOG(ERR, "Wave4VpuEncEncPic, rd:0x%x, wr:0x%x, ret bytes: %d, buffer size:%d\n", rd_ptr, wr_ptr, temp, wr_ptr - rd_ptr);

    if (temp > Handle->mOutputBufferLen) {
        VLOG(ERR, "nal size %d bigger than output buffer %d!\n", temp, Handle->mOutputBufferLen);
        return AMVENC_OVERFLOW;
    }
    if (buf_nal_size != NULL)
        *buf_nal_size = temp;
    if (temp && (buffer!= NULL))
        vdi_read_memory(Handle->instance_id, rd_ptr, (unsigned char *)buffer, temp);
    else if (buffer == NULL)
        VLOG(DEBUG, "HEVC flushing input");
    VLOG(DEBUG, "Wave4VpuEncEncPic %d, ret bytes: %d, buffer size:%d", Handle->enc_counter, temp, wr_ptr - rd_ptr);
    temp = VpuReadReg(Handle->instance_id, W4_RET_ENC_PIC_TYPE);
    if (nal_type != NULL) {
        if ((temp & 0xff) == I_SLICE) {
            if (Handle->mNumInputFrames == 1)
                *nal_type = HEVC_IDR;
            else
                *nal_type = Handle->mEncParams.refresh_type;
        } else if ((temp & 0xff) == P_SLICE || (temp & 0xff) == B_SLICE)
           *nal_type = NON_IRAP;
    }

    VLOG(DEBUG, "Wave4VpuEncEncPic PIC type: 0x%x", temp);

    VpuWriteReg(Handle->instance_id, W4_BS_RD_PTR, wr_ptr);
    VpuWriteReg(Handle->instance_id, W4_RET_ENC_PIC_BYTE, 0);
    return AMVENC_SUCCESS;
}

AMVEnc_Status Wave4VpuEncFiniSeq(AMVHEVCEncHandle *Handle) {
    Uint32 temp;
    VLOG(0, "Wave4VpuEncFiniSeq enter\n");
    temp = VpuReadReg(Handle->instance_id, W4_ADDR_WORK_BASE);
    VLOG(DEBUG, "W4_ADDR_WORK_BASE %x\n", temp);
    temp = VpuReadReg(Handle->instance_id, W4_WORK_SIZE);
    VLOG(DEBUG, "W4_WORK_SIZE %x\n", temp);

    VpuWriteReg(Handle->instance_id, W4_ADDR_WORK_BASE, Handle->work_vb.phys_addr);
    VpuWriteReg(Handle->instance_id, W4_WORK_SIZE, Handle->work_vb.size);
    VpuWriteReg(Handle->instance_id, W4_WORK_PARAM, 0);

    VpuWriteReg(Handle->instance_id, W4_VPU_BUSY_STATUS, 1);
    VpuWriteReg(Handle->instance_id, W4_RET_SUCCESS, 0);  //for debug
    VpuWriteReg(Handle->instance_id, W4_CORE_INDEX, 0);
    VpuWriteReg(Handle->instance_id, W4_INST_INDEX, (Handle->instance_id & 0xffff) | (1 << 16));
    VpuWriteReg(Handle->instance_id, W4_COMMAND, FINI_SEQ);
    VpuWriteReg(Handle->instance_id, W4_VPU_HOST_INT_REQ, 1);
    //Wave4BitIssueCommand(0,4, FINI_SEQ);
    temp = VpuReadReg(Handle->instance_id, W4_VPU_BUSY_STATUS);
    VLOG(DEBUG, "W4_VPU_BUSY_STATUS :0x%x\n", temp);
    if (vdi_wait_interrupt(Handle->instance_id, VPU_BUSY_CHECK_TIMEOUT) == -1) {
        //vdi_free_dma_memory(0, &vb);
        VLOG(ERR, "Wave4VpuEncFiniSeq error time out\n");
        return AMVENC_TIMEOUT;
    }
    //if (vdi_wait_vpu_busy(0, VPU_BUSY_CHECK_TIMEOUT, W4_VPU_BUSY_STATUS) == -1)
    //     return AMVENC_TIMEOUT;
    temp = VpuReadReg(Handle->instance_id, W4_RET_SUCCESS);
    VLOG(DEBUG, "W4_RET_SUCCESS %x\n", temp);
    VLOG(DEBUG, "Wave4VpuEncFiniSeq ret\n");
    if (VpuReadReg(Handle->instance_id, W4_RET_SUCCESS) == 0) {
        uint32_t reasonCode = VpuReadReg(Handle->instance_id, W4_RET_FAIL_REASON);
        VLOG(ERR, "Wave4VpuEncFiniSeq failedREASON CODE(%08x)\n", reasonCode);
        return AMVENC_HARDWARE;
    }
    VLOG(DEBUG, "Wave4VpuEncFiniSeq succes\n");
    return AMVENC_SUCCESS;
}

AMVEnc_Status ge2d_colorFormat(AMVEncFrameFmt format) {
    switch (format) {
        case AMVENC_RGB888:
            SRC1_PIXFORMAT = PIXEL_FORMAT_RGB_888;
            SRC2_PIXFORMAT = PIXEL_FORMAT_RGB_888;
            return AMVENC_SUCCESS;
        case AMVENC_RGBA8888:
            SRC1_PIXFORMAT = PIXEL_FORMAT_RGBA_8888;
            SRC2_PIXFORMAT = PIXEL_FORMAT_RGBA_8888;
            return AMVENC_SUCCESS;
        default:
            VLOG(ERR, "not support color format, format %d!",format);
            return AMVENC_FAIL;
    }
}

AMVEnc_Status AML_HEVCInitialize(AMVHEVCEncHandle *Handle, AMVHEVCEncParams *encParam, bool* has_mix, int force_mode) {
    AMVEnc_Status ret;
    (void)has_mix;
    (void)force_mode;

    Handle->instance_id = 0;
    Handle->src_idx = 0;
    Handle->enc_counter = 0;
    Handle->enc_width = encParam->width;
    Handle->enc_height = encParam->height;
    Handle->bitrate = encParam->bitrate;
    Handle->frame_rate = encParam->frame_rate;
    Handle->mPrependSPSPPSToIDRFrames = false;
    Handle->mOutputBufferLen = 0;
    Handle->mUvSwap = 1;
    Handle->mGopIdx = 0;

    Handle->idrPeriod = encParam->idr_period;
    Handle->mNumPlanes = 0;

    VLOG(DEBUG, "Handle->idrPeriod: %d\n", Handle->idrPeriod);

    reset_error = 0;

    if (vdi_init(Handle->instance_id) < 0)
        return AMVENC_FAIL;
    ret = Wave4VpuInit(Handle, Handle->instance_id);
    if (ret < AMVENC_SUCCESS)
        return ret;
    ret = Wave4VpuCreateInstance(Handle, 1);
    if (ret < AMVENC_SUCCESS)
        return ret;
    ret = Wave4VpuEncSeqInit(Handle, 1);
    if (ret < AMVENC_SUCCESS)
        return ret;
    //if(Handle->mGopIdx == 0)
    ret = Wave4VpuEncSeqSet(Handle);
    if (ret < AMVENC_SUCCESS)
        return ret;
    Handle->src_count = VpuReadReg(Handle->instance_id, W4_RET_ENC_MIN_SRC_BUF_NUM);
    VLOG(DEBUG, "src count:%d\n", Handle->src_count);
    Wave4VpuEncRegisterFrame(Handle, 1);

#if SUPPORT_SCALE
    if ((encParam->width != encParam->src_width) || (encParam->height != encParam->src_height) || true) {
        int ret = aml_ge2d_init();
        if (ret < 0) {
            VLOG(ERR, "encode open ge2d failed, ret=0x%x", ret);
            return AMVENC_FAIL;
        }

        memset(&amlge2d,0x0,sizeof(aml_ge2d_t));
        amlge2d.pge2d_info = &ge2dinfo;
        memset(&ge2dinfo, 0, sizeof(aml_ge2d_info_t));
        memset(&(ge2dinfo.src_info[0]), 0, sizeof(buffer_info_t));
        memset(&(ge2dinfo.src_info[1]), 0, sizeof(buffer_info_t));
        memset(&(ge2dinfo.dst_info), 0, sizeof(buffer_info_t));

        set_ge2dinfo(&ge2dinfo, encParam);
        INIT_GE2D = true;
    }
#endif
    VLOG(INFO, "AML_HEVCInitialize succeed\n");
    return AMVENC_SUCCESS;
}

void static yuv_plane_memcpy(char *dst, char *src, uint32 width, uint32 height, uint32 stride, bool aligned) {
    if (dst == NULL || src == NULL) {
        VLOG(ERR, "yuv_plane_memcpy error ptr\n");
        return;
    }
    if (!aligned) {
        for (unsigned int i = 0; i < height; i++) {
            memcpy((void *)dst, (void *)src, width);
            dst += stride;
            src += width;
        }
    } else {
        memcpy(dst, (void *)src, stride * height);
    }
}
AMVEnc_Status AML_HEVCSetInput(AMVHEVCEncHandle *Handle, AMVHEVCEncFrameIO *input) {
    Uint32 src_stride;
    Uint32 luma_stride, chroma_stride;
    Uint32 size_src_luma, size_src_chroma;
    char *y_dst = NULL;
    char *u_dst = NULL;
    char *v_dst = NULL;
    char *src = NULL;
    bool width32alinged = true; //width is multiple of 32 or not
    if (Handle->enc_width % 32) {
        width32alinged = false;
    }

    Handle->op_flag = input->op_flag;
    Handle->fmt = input->fmt;
    if (Handle->fmt != AMVENC_NV12 && Handle->fmt != AMVENC_NV21 && Handle->fmt != AMVENC_YUV420) {
        if (INIT_GE2D) {
#if SUPPORT_SCALE
            if (ge2d_colorFormat(Handle->fmt) == AMVENC_SUCCESS) {
                VLOG(DEBUG, "The %d of color format that HEVC need ge2d to change!", Handle->fmt);
            } else
#endif
            {
                return AMVENC_NOT_SUPPORTED;
            }
        }
    }
    if (Handle->fmt == AMVENC_NV12) {
        Handle->mUvSwap = 0;
    }

    if (Handle->bitrate != input->bitrate) {
        Handle->bitrate = input->bitrate;
        VLOG(DEBUG, "HEVC set bitrate to %d", Handle->bitrate);
        Wave4VpuEncSeqInit(Handle, 0);
    }
    if (Handle->bufType == DMA_BUFF) {
        src_stride = wave420l_align16(Handle->enc_width);
    } else {
        src_stride = wave420l_align32(Handle->enc_width);
    }
    luma_stride = src_stride;
    chroma_stride = src_stride;
    if (input->type != DMA_BUFF) {
#if SUPPORT_SCALE
        if ((input->scale_width !=0 && input->scale_height !=0) || input->crop_left != 0 ||
            input->crop_right != 0 || input->crop_top != 0 || input->crop_bottom != 0 ||
            (Handle->fmt != AMVENC_NV12 && Handle->fmt != AMVENC_NV21 && Handle->fmt != AMVENC_YUV420)) {
            if (INIT_GE2D) {
                INIT_GE2D = false;
                ge2dinfo.src_info[0].format = SRC1_PIXFORMAT;
                ge2dinfo.src_info[1].format = SRC2_PIXFORMAT;
                int ret = aml_ge2d_mem_alloc(&ge2dinfo);
                if (ret < 0) {
                    VLOG(ERR, "encode ge2di mem alloc failed, ret=0x%x", ret);
                    return AMVENC_FAIL;
                }
                VLOG(DEBUG, "ge2d init successful!");
            }
            VLOG(DEBUG, "HEVC TEST sclale, enc_width:%d enc_height:%d  pitch:%d height:%d, line:%d",
                     Handle->enc_width, Handle->enc_height, input->pitch, input->height, __LINE__);
            if (input->pitch % 32) {
                VLOG(ERR, "HEVC crop and scale must be 32bit aligned");
                return AMVENC_FAIL;
            }
            if (Handle->fmt == AMVENC_RGBA8888)
                memcpy((void *)ge2dinfo.src_info[0].vaddr, (void *)input->YCbCr[0], 4 * input->pitch * input->height);
            else if (Handle->fmt == AMVENC_NV12 || Handle->fmt == AMVENC_NV21) {
                memcpy((void *)ge2dinfo.src_info[0].vaddr, (void *)input->YCbCr[0], input->pitch * input->height);
                memcpy((void *) ((char *)ge2dinfo.src_info[0].vaddr + input->pitch * input->height), (void *)input->YCbCr[1], input->pitch * input->height / 2);
            } else if (Handle->fmt == AMVENC_YUV420) {
                memcpy((void *)ge2dinfo.src_info[0].vaddr, (void *)input->YCbCr[0], input->pitch * input->height);
                memcpy((void *) ((char *)ge2dinfo.src_info[0].vaddr + input->pitch * input->height), (void *)input->YCbCr[1], input->pitch * input->height / 4);
                memcpy((void *) ((char *)ge2dinfo.src_info[0].vaddr + (input->pitch * input->height * 5) /4), (void *)input->YCbCr[2], input->pitch * input->height / 4);
            } else if (Handle->fmt == AMVENC_RGB888)
                memcpy((void *)ge2dinfo.src_info[0].vaddr, (void *)input->YCbCr[0], input->pitch * input->height * 3);
            do_strechblit(&ge2dinfo, input);
            aml_ge2d_invalid_cache(&ge2dinfo);
            size_src_luma = luma_stride * wave420l_align32(Handle->enc_height);
            size_src_chroma = luma_stride * (wave420l_align16(Handle->enc_height) / 2);
            y_dst = (char *) Handle->src_vb[Handle->src_idx].virt_addr;
            src = ge2dinfo.dst_info.vaddr;
            yuv_plane_memcpy(y_dst, src, Handle->enc_width, Handle->enc_height, luma_stride, width32alinged);

            u_dst = (char *) (Handle->src_vb[Handle->src_idx].virt_addr + size_src_luma);
            src = (char *)ge2dinfo.dst_info.vaddr + Handle->enc_width * Handle->enc_height;
            yuv_plane_memcpy(u_dst, src, Handle->enc_width, Handle->enc_height / 2, chroma_stride, width32alinged);
        } else
#endif
        {
            // set frame buffers
            // calculate required buffer size
            size_src_luma = luma_stride * wave420l_align32(Handle->enc_height);
            size_src_chroma = luma_stride * (wave420l_align16(Handle->enc_height) / 2);

            y_dst = (char *) Handle->src_vb[Handle->src_idx].virt_addr;
            src = (char *) input->YCbCr[0];
            yuv_plane_memcpy(y_dst, src, Handle->enc_width, Handle->enc_height, luma_stride, width32alinged);

            if (Handle->fmt == AMVENC_NV12 || Handle->fmt == AMVENC_NV21) {
                u_dst = (char *)(Handle->src_vb[Handle->src_idx].virt_addr + size_src_luma);
                src = (char *)input->YCbCr[1];
                yuv_plane_memcpy(u_dst, src, Handle->enc_width, Handle->enc_height / 2, chroma_stride, width32alinged);
            } else if (Handle->fmt == AMVENC_YUV420) {
                u_dst = (char *)(Handle->src_vb[Handle->src_idx].virt_addr + size_src_luma);
                src = (char *)input->YCbCr[1];
                yuv_plane_memcpy(u_dst, src, Handle->enc_width / 2, Handle->enc_height / 2, chroma_stride / 2, width32alinged);

                v_dst = (char *)(Handle->src_vb[Handle->src_idx].virt_addr + size_src_luma + size_src_luma / 4);
                src = (char *)input->YCbCr[2];
                yuv_plane_memcpy(v_dst, src, Handle->enc_width / 2, Handle->enc_height / 2, chroma_stride / 2, width32alinged);
            }
        }
        flush_memory(Handle->instance_id, &Handle->src_vb[Handle->src_idx]);
    } else {
        if (Handle->enc_width % 16) {
            //..to do
            VLOG(ERR, "dma buffer width must be multiple of 16!");
            return AMVENC_NOT_SUPPORTED;
        }
    }
    return AMVENC_SUCCESS;
}

AMVEnc_Status AML_HEVCEncHeader(AMVHEVCEncHandle *Handle, unsigned char *buffer, unsigned int *buf_nal_size, int *nal_type) {
    AMVEnc_Status ret;
    (void)nal_type;
    ret = Wave4VpuEncGetHeader(Handle, buffer, buf_nal_size);
    return ret;
}

AMVEnc_Status AML_HEVCEncNAL(AMVHEVCEncHandle *Handle, unsigned char *buffer, unsigned int *buf_nal_size, int *nal_type) {
    AMVEnc_Status ret = AMVENC_FAIL;
    int end = 0;
    int reset_count = 0;

    if (reset_error)
        return ret;
retry:
    ret = Wave4VpuEncEncPic(Handle, Handle->src_idx, end, buffer, buf_nal_size, nal_type);

    if (ret != AMVENC_SUCCESS) {
        if (ret == AMVENC_OVERFLOW)
            return ret;
        if (reset_mode == 2) {
            Wave4VpuCreateInstance(Handle, 0);
        }
        Wave4VpuEncSeqInit(Handle, 0);
        if (Handle->mGopIdx == 0)
            Wave4VpuEncSeqSet(Handle);
        Handle->src_count = VpuReadReg(Handle->instance_id, W4_RET_ENC_MIN_SRC_BUF_NUM);
        Wave4VpuEncRegisterFrame(Handle, 0);
        Handle->src_idx = 0;
        reset_count++;
        if (reset_count < 5)
            goto retry;

        //ret = AMVENC_REENCODE_PICTURE;
        //return ret;
    }
    Handle->src_idx++;
    Handle->src_idx = Handle->src_idx % Handle->src_count;
    Handle->enc_counter++;

    return AMVENC_PICTURE_READY;
}

AMVEnc_Status AML_HEVCRelease(AMVHEVCEncHandle *Handle) {
    int end = 1;

    if (Wave4VpuEncEncPic(Handle, Handle->src_idx, end, NULL, NULL, NULL) != AMVENC_SUCCESS) {
        VLOG(ERR, "Wave4VpuEncEncPic err\n");
    }
    Handle->src_idx++;
    Handle->src_idx = Handle->src_idx % Handle->src_count;
    Wave4VpuEncFiniSeq(Handle);

    for (unsigned int i = 0; i < Handle->src_num; i++) {
        if (Handle->src_vb[i].size) {
            vdi_free_dma_memory(Handle->instance_id, &Handle->src_vb[i]);
        }
    }
    for (unsigned int i = 0; i < Handle->fb_num; i++) {
        if (Handle->fb_vb[i].size) {
            vdi_free_dma_memory(Handle->instance_id, &Handle->fb_vb[i]);
        }
    }
    if (Handle->fbc_ltable_vb.size) {
        vdi_free_dma_memory(Handle->instance_id, &Handle->fbc_ltable_vb);
    }
    if (Handle->fbc_ctable_vb.size) {
        vdi_free_dma_memory(Handle->instance_id, &Handle->fbc_ctable_vb);
    }
    if (Handle->fbc_mv_vb.size) {
        vdi_free_dma_memory(Handle->instance_id, &Handle->fbc_mv_vb);
    }
    if (Handle->subsample_vb.size) {
        vdi_free_dma_memory(Handle->instance_id, &Handle->subsample_vb);
    }
    if (Handle->temp_vb.size) {
        vdi_free_dma_memory(Handle->instance_id, &Handle->temp_vb);
    }
    if (Handle->work_vb.size) {
        vdi_free_dma_memory(Handle->instance_id, &Handle->work_vb);
    }
    if (Handle->bs_vb.size) {
        vdi_free_dma_memory(Handle->instance_id, &Handle->bs_vb);
    }
    vdi_release(Handle->instance_id);

#if SUPPORT_SCALE
    if (ge2dinfo.src_info[0].vaddr != NULL) {
        aml_ge2d_mem_free(&ge2dinfo);
        aml_ge2d_exit();
        ge2dinfo.src_info[0].vaddr = NULL;
        VLOG(DEBUG, "ge2d exit!!!\n");
    }
#endif
    VLOG(INFO, "AML_HEVCRelease succeed\n");
    return AMVENC_SUCCESS;
}
