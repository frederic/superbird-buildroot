/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */
#include "pred.h"
#include <stdlib.h>

#if defined LOG_TAG
#undef LOG_TAG
#define LOG_TAG "PRED_MODE"
#endif

#include <utils/Log.h>
#define TH_I4  0  /* threshold biasing toward I16 mode instead of I4 mode */
#define TH_Intra  0 /* threshold biasing toward INTER mode instead of intra mode */

#define FIXED_INTRAPRED_MODE  AML_I16
#define FIXED_I16_MODE  AML_I16_DC
#define FIXED_I4_MODE   AML_I4_Diagonal_Down_Left
#define FIXED_INTRA_CHROMA_MODE AML_IC_DC

#ifndef AML_MAX
#define AML_MAX(x,y) ((x)>(y)? (x):(y))
#endif
#ifndef AML_MIN
#define AML_MIN(x,y) ((x)<(y)? (x):(y))
#endif

#define CLIP_RESULT(x)      if((uint)x > 0xFF){ \
        x = 0xFF & (~(x>>31));}


#define SHIFT_QP        12
#define  LAMBDA_ACCURACY_BITS         16
#define  LAMBDA_FACTOR(lambda)        ((int)((double)(1<<LAMBDA_ACCURACY_BITS)*lambda+0.5))
//#define  AMLNumI4PredMode 3
/*
Availability of the neighboring top-right block relative to the current block. */
const static int BlkTopRight_new[16] = {2, 2, 2, 3,
                                        1, 1, 1, 3,
                                        1, 1, 1, 3,
                                        1, 1, 1, 3
                                       };

const static int BlkTopRight[16] = {2, 2, 2, 3,
                                    1, 0, 1, 0,
                                    1, 1, 1, 0,
                                    1, 0, 1, 0
                                   };
const static int QP2QUANT[40] =
{
    1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 4, 4, 4, 5, 6,
    6, 7, 8, 9, 10, 11, 13, 14,
    16, 18, 20, 23, 25, 29, 32, 36,
    40, 45, 51, 57, 64, 72, 81, 91
};
/** from [blk8indx][blk4indx] to raster scan index */
const static int blkId_2blkXY[4][4] = {{0, 1, 4, 5}, {2, 3, 6, 7}, {8, 9, 12, 13}, {10, 11, 14, 15}};

void InitNeighborAvailability_iframe(amvenc_pred_mode_t *predMB,
                                     /*amvenc_drv_t* p*/ amvenc_curr_pred_mode_t *cur)
{
    amvenc_neighbor_t *n = &cur->neighbor;
    int mb_x = n->mbx;
    int mb_y = n->mby;
    int mb_width = predMB->mb_width;
    int mb_height = predMB->mb_height;
    n->mbAvailA = 0 ;
    n->mbAvailB = 0 ;
    n->mbAvailC = 0 ;
    n->mbAvailD = 0 ;
    cur->mbNum = mb_x + mb_y * mb_width;
    //ALOGD("(%d, %d), cur=%p, n=%p\n", mb_x, mb_y, cur, n);
    if (mb_x)
    {
        n->mbAvailA = 1;
    }
    if (mb_y)
    {
        n->mbAvailB = 1;
        n->mbAvailC = 1;
    }
    if ((mb_y + 1) % mb_width == 0)
    {
        n->mbAvailC = 0;
    }
    n->mbAvailD = n->mbAvailA && n->mbAvailB;
    if (n->mbAvailA)
    {
        cur->mbAddrA = cur->mbNum - 1;
    }
    if (n->mbAvailB)
    {
        cur->mbAddrB = cur->mbNum - mb_width;
    }
    if (n->mbAvailC)
    {
        cur->mbAddrC = cur->mbNum - mb_width + 1;
    }
    if (n->mbAvailD)
    {
        cur->mbAddrD = cur->mbNum - mb_width - 1;
    }
}

amvenc_curr_pred_mode_t *InitcurrMBStruct()
{
    amvenc_curr_pred_mode_t *curr = NULL;
    curr = (amvenc_curr_pred_mode_t *)calloc(1, sizeof(amvenc_curr_pred_mode_t));
#if 0
    curr->i4_enable[AML_I4_Vertical] =  1;
    curr->i4_enable[AML_I4_Horizontal] =  1;
    curr->i4_enable[AML_I4_DC] =  1;
    curr->i4_enable[AML_I4_Diagonal_Down_Left] =  1;
    curr->i4_enable[AML_I4_Diagonal_Down_Right] =  1;
    curr->i4_enable[AML_I4_Vertical_Right] =  1;
    curr->i4_enable[AML_I4_Horizontal_Down] =  1;
    curr->i4_enable[AML_I4_Vertical_Left] =  1;
    curr->i4_enable[AML_I4_Horizontal_Up] = 1;
    curr->i16_enable[AML_I16_Vertical] = 1;
    curr->i16_enable[AML_I16_Horizontal] = 1;
    curr->i16_enable[AML_I16_DC] = 1;
#endif
    if (NULL == curr)
    {
        LOGAPI("%s failed\n", __func__);
    }
    return curr;
}

int ClearcurrMBStruct(void *p)
{
    if (p)
    {
        free(p);
        p = NULL;
    }
    return 0;
}

#if 1
inline void intrapred_luma_16x16(amvenc_pred_mode_obj_t *encvid)
{
    amvenc_curr_pred_mode_t *video = encvid->mb_node;
    amvenc_pred_mode_t *common = encvid->mb_list;
    int *intraAvail = &video->neighbor.mbAvailA;
    int x_pos = (video->neighbor.mbx) << 4;
    int y_pos = (video->neighbor.mby) << 4;
    int pitch = common->pitch;
    int offset = y_pos * pitch + x_pos;
    uint8_t *curL = encvid->YCbCr[0] + offset; /* point to reconstructed frame */
    uint8_t *pred = video->pred_i16[AML_I16_Vertical];
    asm volatile(
        "ldr        r7,  [%[intraAvail]]            @r7: video->intraAvailA\n\t"
        "ldr        r8,  [%[intraAvail],  #4]       @r8: video->intraAvailB\n\t"
        "VMOV.I32   d2,  #0                         @d2: sum\n\t"
        "cmp        r8,  #0                         \n\t"
        "beq        3f                              \n\t"
        "sub        r9,  %[curL],  %[pitch]         @r9: top = curL - pitch;\n\t"
        "VLD1.32    {d0,  d1},  [r9]                @Q0: word4 word3 word2 word1\n\t"
        "MOV        r10,  #16                       \n\t"
        "1:                                         \n\t"
        "VST1.32    {d0,  d1},  [%[pred]]!          \n\t"
        "subs       r10,  r10,  #1                  \n\t"
        "bgt        1b                              \n\t"
        "VPADDL.U8  Q0,  Q0                         \n\t"
        "VPADD.U16  d0,  d0,  d1                    \n\t"
        "VPADDL.U16 d0,  d0                         \n\t"
        "VPADDL.U32 d0,  d0                         \n\t"
        "VADD.U32   d2,  d2,  d0                    \n\t"
        "cmp        r7,  #0                         \n\t"
        "bne        3f                              \n\t"
        "VMOV.I32   d0,  #8                         \n\t"
        "VADD.U32   d2,  d2,  d0                    \n\t"
        "VSHR.U32   d2,  d2,  #4                    \n\t"
        "3:                                         \n\t"
        "cmp        r7,  #0                         \n\t"
        "beq        6f                              \n\t"
        "sub        r9,  %[curL],  #1               @r9: left = curL - 1\n\t"
        "mov        r11,  #16                       \n\t"
        "4:                                         \n\t"
        "ldrb       r12, [r9]                       \n\t"
        "add        r9, r9, %[pitch]                \n\t"
        "VMOV.U32   d0[0],  r12                     \n\t"
        "VADD.U32   d2,  d2,  d0                    \n\t"
        "VDUP.I8    Q0,  r12                        \n\t"
        "@pred = video->pred_i16[AML_I16_Horizontal]\n\t"
        "VST1.32    {d0,  d1},  [%[pred]]!          \n\t"
        "subs       r11,  r11,  #1                  \n\t"
        "bgt        4b                              \n\t"
        "cmp        r8,  #0                         \n\t"
        "beq        5f                              \n\t"
        "VMOV.I32   d0,  #16                        \n\t"
        "VADD.U32   d2,  d2,  d0                    \n\t"
        "VSHR.U32   d2,  d2,  #5                    \n\t"
        "b          6f                              \n\t"
        "5:                                         \n\t"
        "VMOV.I32   d0,  #8                         \n\t"
        "VADD.U32   d2,  d2,  d0                    \n\t"
        "VSHR.U32   d2,  d2,  #4                    \n\t"
        "6:                                         \n\t"
        "cmp        r7,  #0                         \n\t"
        "bne        7f                              \n\t"
        "cmp        r8,  #0                         \n\t"
        "bne        7f                              \n\t"
        "VMOV.I8    Q0,  #0x80                      \n\t"
        "b          8f                              \n\t"
        "7:                                         \n\t"
        "VDUP.I8    Q0,  d2[0]                      \n\t"
        "8:                                         \n\t"
        "mov        r10,  #16                       \n\t"
        "9:                                         \n\t"
        "@pred = encvid->pred_i16[AML_I16_DC]       \n\t"
        "VST1.32    {d0,  d1},  [%[pred]]!          \n\t"
        "subs       r10,  r10,  #1                  \n\t"
        "bgt        9b                              \n\t"
        "12:@end                                    \n\t"
        : [curL] "+r"(curL), [intraAvail] "+r"(intraAvail), [pred] "+r"(pred)
        : [pitch] "r"(pitch)
        : "cc", "memory",
        "r7", "r8", "r9", "r10", "r11", "r12",
        "q0", "q1"
    );
}
#endif

/* evaluate each prediction mode of I16 */
void find_cost_16x16(amvenc_pred_mode_obj_t *encvid,
                     /*AMLEncObject *encvid,*/ uint8_t *orgY, int *min_cost, int flag)
{
    amvenc_curr_pred_mode_t *video = encvid->mb_node;
    amvenc_pred_mode_t *common = encvid->mb_list;
    //video->currMB = common->mblock + video->mbNum;
    AMLMacroblock *currMB = video->currMB;
    //AMLCommonObj *video = encvid->common;
    //AMLMacroblock *currMB = video->currMB;
    int cost;
    int cost_save[4];
    //int org_pitch = encvid->currInput->pitch;
    int org_pitch = common->pitch;
    memset(cost_save, 0, sizeof(cost_save));
    /* evaluate vertical mode */
    //if (video->intraAvailB)
    if (video->neighbor.mbAvailB && video->i16_enable[AML_I16_Vertical])
    {
        cost = pred_cost_i16(orgY, org_pitch, video->pred_i16[AML_I16_Vertical], *min_cost);
        if (flag & 1)
            cost = cost + 1;
        if (cost < *min_cost)
        {
            *min_cost = cost;
            currMB->mbMode = AML_I16;
            currMB->mb_intra = 1;
            currMB->i16Mode = AML_I16_Vertical;
        }
        cost_save[AML_I16_Vertical] = cost;
    }
    /* evaluate horizontal mode */
    //if (video->intraAvailA)
    if (video->neighbor.mbAvailA && video->i16_enable[AML_I16_Horizontal])
    {
        cost = pred_cost_i16(orgY, org_pitch, video->pred_i16[AML_I16_Horizontal], *min_cost);
        if (flag & 2)
            cost = cost + 1;
        if (cost < *min_cost)
        {
            *min_cost = cost;
            currMB->mbMode = AML_I16;
            currMB->mb_intra = 1;
            currMB->i16Mode = AML_I16_Horizontal;
        }
        cost_save[AML_I16_Horizontal] = cost;
    }
    /* evaluate DC mode */
    if (video->i16_enable[AML_I16_DC])
    {
        cost = pred_cost_i16(orgY, org_pitch, video->pred_i16[AML_I16_DC], *min_cost);
        if (cost < *min_cost)
        {
            *min_cost = cost;
            currMB->mbMode = AML_I16;
            currMB->mb_intra = 1;
            currMB->i16Mode = AML_I16_DC;
        }
        cost_save[AML_I16_DC] = cost;
    }
    return ;
}

/* perform searching for MB mode */
/* assuming that this is done inside the encoding loop,
no need to call InitNeighborAvailability */

int MBIntraSearch_iframe(amvenc_pred_mode_obj_t *encvid, unsigned char *input_addr)
{
    amvenc_curr_pred_mode_t *video = encvid->mb_node;
    amvenc_pred_mode_t *common = encvid->mb_list;
    //amvenc_pred_mode_t *predMB = (amvenc_pred_mode_t *) (&para->intra_mode);
    video->currMB = common->mblock + video->mbNum;
    AMLMacroblock *currMB = video->currMB;
    int min_cost;
    uint8_t *orgY;
    uint8_t *curL;
    int x_pos = (video->neighbor.mbx) << 4;
    int y_pos = (video->neighbor.mby) << 4;
    uint32_t *saved_inter;
    int j;
    AMLSliceType slice_type;
    int orgPitch = common->pitch;
    bool intra = true;
    static int count = 0;
    //currMB->CBP = 0;
    video->slice_type = AML_I_SLICE;
    curL = encvid->YCbCr[0] + y_pos * orgPitch + x_pos;
    unsigned int *p = (unsigned int *)input_addr;
    uint8_t mb_type = HENC_MB_Type_I4MB ;
    min_cost = 0x7fffffff;
    //LOGV("MBIntraSearch start, video type is %d\n ",video->slice_type);
    /* first do motion vector and variable block size search */
    //min_cost = common->min_cost[video->mbNum];
    /* now perform intra prediction search */
    /* need to add the check for encvid->intraSearch[video->mbNum] to skip intra
       if it's not worth checking. */
    /* i16 mode search */
    /* generate all the predictions */
    //    mb_type = HENC_MB_Type_I16MB;
    orgY = encvid->YCbCr[0] + y_pos * orgPitch + x_pos;
    intrapred_luma_16x16(encvid);
    /* evaluate them one by one */
    find_cost_16x16(encvid, orgY, &min_cost, 0);
    /* i4 mode search */
    mb_intra4x4_search(encvid, &min_cost, NULL);
#if 0
    switch (currMB->mbMode)
    {
        case AML_I4:
            p[2] = currMB->i4Mode[10] | currMB->i4Mode[11] << 4 |
                   currMB->i4Mode[14] << 8 | currMB->i4Mode[15] << 12 |
                   currMB->i4Mode[8] << 16 | currMB->i4Mode[9] << 20 |
                   currMB->i4Mode[12] << 24 | currMB->i4Mode[13] << 28;
            p[3] = currMB->i4Mode[2] | currMB->i4Mode[3] << 4 |
                   currMB->i4Mode[6] << 8 | currMB->i4Mode[7] << 12 |
                   currMB->i4Mode[0] << 16 | currMB->i4Mode[1] << 20 |
                   currMB->i4Mode[4] << 24 | currMB->i4Mode[5] << 28;
            input_addr[18] = HENC_MB_Type_I4MB;
            break;
        case AML_I16:
            input_addr[17] = currMB->i16Mode;
            input_addr[18] = HENC_MB_Type_I16MB;
            break;
        default:
            input_addr[17] = currMB->i16Mode;
            input_addr[18] = HENC_MB_Type_I16MB;
            break;
    }
#endif
    return min_cost;
}

int MBIntraSearch_P16X16(amvenc_pred_mode_obj_t *encvid, unsigned char *input_addr , int cost, int flag)
{
    amvenc_curr_pred_mode_t *video = encvid->mb_node;
    amvenc_pred_mode_t *common = encvid->mb_list;
    //amvenc_pred_mode_t *predMB = (amvenc_pred_mode_t *) (&para->intra_mode);
    video->currMB = common->mblock + video->mbNum;
    AMLMacroblock *currMB = video->currMB;
    int min_cost;
    uint8_t *orgY;
    uint8_t *curL;
    int x_pos = (video->neighbor.mbx) << 4;
    int y_pos = (video->neighbor.mby) << 4;
    uint32_t *saved_inter;
    int j;
    AMLSliceType slice_type;
    int orgPitch = common->pitch;
    bool intra = true;
    static int count = 0;
    //currMB->CBP = 0;
    video->slice_type = AML_I_SLICE;
    curL = encvid->YCbCr[0] + y_pos * orgPitch + x_pos;
    unsigned int *p = (unsigned int *)input_addr;
    uint8_t mb_type = HENC_MB_Type_I4MB ;
    min_cost = cost;
    //LOGV("MBIntraSearch start, video type is %d\n ",video->slice_type);
    /* first do motion vector and variable block size search */
    //min_cost = common->min_cost[video->mbNum];
    /* now perform intra prediction search */
    /* need to add the check for encvid->intraSearch[video->mbNum] to skip intra
       if it's not worth checking. */
    /* i16 mode search */
    /* generate all the predictions */
    //    mb_type = HENC_MB_Type_I16MB;
    orgY = encvid->YCbCr[0] + y_pos * orgPitch + x_pos;
    intrapred_luma_16x16(encvid);
    /* evaluate them one by one */
    find_cost_16x16(encvid, orgY, &min_cost, flag);
    //input_addr[17]= currMB->i16Mode;
    //input_addr[18]= HENC_MB_Type_I16MB;
    return min_cost;
}

int MBIntraSearch_P4X4(amvenc_pred_mode_obj_t *encvid, unsigned char *input_addr , int cost, unsigned char *pred_mode)
{
    amvenc_curr_pred_mode_t *video = encvid->mb_node;
    amvenc_pred_mode_t *common = encvid->mb_list;
    //amvenc_pred_mode_t *predMB = (amvenc_pred_mode_t *) (&para->intra_mode);
    video->currMB = common->mblock + video->mbNum;
    AMLMacroblock *currMB = video->currMB;
    int min_cost;
    uint8_t *orgY;
    uint8_t *curL;
    int x_pos = (video->neighbor.mbx) << 4;
    int y_pos = (video->neighbor.mby) << 4;
    uint32_t *saved_inter;
    int j;
    AMLSliceType slice_type;
    int orgPitch = common->pitch;
    bool intra = true;
    static int count = 0;
    //currMB->CBP = 0;
    video->slice_type = AML_I_SLICE;
    curL = encvid->YCbCr[0] + y_pos * orgPitch + x_pos;
    unsigned int *p = (unsigned int *)input_addr;
    uint8_t mb_type = HENC_MB_Type_I4MB ;
    min_cost = cost;
    //LOGV("MBIntraSearch start, video type is %d\n ",video->slice_type);
    /* first do motion vector and variable block size search */
    //min_cost = common->min_cost[video->mbNum];
    /* now perform intra prediction search */
    /* need to add the check for encvid->intraSearch[video->mbNum] to skip intra
       if it's not worth checking. */
    /* i16 mode search */
    /* generate all the predictions */
    //    mb_type = HENC_MB_Type_I16MB;
    orgY = encvid->YCbCr[0] + y_pos * orgPitch + x_pos;
    /* i4 mode search */
    mb_intra4x4_search(encvid, &min_cost, pred_mode);
#if 0
    p[2] = currMB->i4Mode[10] | currMB->i4Mode[11] << 4 |
           currMB->i4Mode[14] << 8 | currMB->i4Mode[15] << 12 |
           currMB->i4Mode[8] << 16 | currMB->i4Mode[9] << 20 |
           currMB->i4Mode[12] << 24 | currMB->i4Mode[13] << 28;
    p[3] = currMB->i4Mode[2] | currMB->i4Mode[3] << 4 |
           currMB->i4Mode[6] << 8 | currMB->i4Mode[7] << 12 |
           currMB->i4Mode[0] << 16 | currMB->i4Mode[1] << 20 |
           currMB->i4Mode[4] << 24 | currMB->i4Mode[5] << 28;
    input_addr[18] = HENC_MB_Type_I4MB;
#endif
    return min_cost;
}
/******************************************************
 *
 * call after every frame
 * to release the struct which allocated in
 * MBIntraSearch_prepare(amvenc_drv_t* p)
 *
******************************************************/
void MBIntraSearch_clean(amvenc_pred_mode_obj_t *encvid)
{
    if (encvid)
    {
        if (encvid->mb_node)
            ClearcurrMBStruct(encvid->mb_node);
        free(encvid);
        encvid = NULL;
    }
    return ;
}
inline void chroma_intra_search(amvenc_neighbor_t *n, unsigned char *input_addr)
{
    uint8_t c_iMode = HENC_DC_PRED_8;
    /* evaluate DC mode */
    if ((!n->mbAvailA) && (!n->mbAvailB))
    {
        c_iMode = HENC_DC_PRED_8;
        goto Done;
    }
    /* evaluate vertical   mode */
    if (!n->mbAvailA)
    {
        c_iMode = HENC_VERT_PRED_8   ;
    }
    else
    {
        /* evaluate horizontal mode */
        c_iMode = HENC_HOR_PRED_8 ;
    }
Done:
    input_addr[16] = c_iMode ;
}

int MBIntraSearch_full(amvenc_pred_mode_obj_t *encvid,
                       int x, int y, unsigned char *input_mb)
{
    amvenc_curr_pred_mode_t *cur    = encvid->mb_node;
    amvenc_pred_mode_t      *common = encvid->mb_list;
    int min_cost;
    //ALOGD("%d, encvid=%p\n", __LINE__, encvid);
    cur->neighbor.mbx = x;
    cur->neighbor.mby = y;
    InitNeighborAvailability_iframe(common, cur);
#if 0
    if (HENC_MB_Type_P16MB_HORIZ_ONLY == input_mb[18])
    {
        cur->i16_enable[AML_I16_Vertical] = 0;
        cur->i16_enable[AML_I16_Horizontal] = 1;
        cur->i16_enable[AML_I16_DC] = 0;
        cur->neighbor.mbAvailB = 0;
        cur->neighbor.mbAvailC = 0;
        cur->neighbor.mbAvailD = 0;
    }
    else
    {
        cur->i16_enable[AML_I16_Vertical] = 1;
        cur->i16_enable[AML_I16_Horizontal] = 1;
        cur->i16_enable[AML_I16_DC] = 1;
    }
#endif
    min_cost = MBIntraSearch_iframe(encvid , input_mb);
    if (input_mb)
        chroma_intra_search(&cur->neighbor, input_mb);
    return min_cost;
}

int MBIntraSearch_P16(amvenc_pred_mode_obj_t *encvid,
                      int x, int y, unsigned char *input_mb, int cost, int flag)
{
    amvenc_curr_pred_mode_t *cur    = encvid->mb_node;
    amvenc_pred_mode_t      *common = encvid->mb_list;
    int min_cost;
    cur->neighbor.mbx = x;
    cur->neighbor.mby = y;
    InitNeighborAvailability_iframe(common, cur);
#if 0
    if (0) //HENC_MB_Type_P16MB_HORIZ_ONLY == input_mb[18]) {
    {
        cur->i16_enable[AML_I16_Vertical] = 0;
        cur->i16_enable[AML_I16_Horizontal] = 1;
        cur->i16_enable[AML_I16_DC] = 0;
        cur->neighbor.mbAvailB = 0;
        cur->neighbor.mbAvailC = 0;
        cur->neighbor.mbAvailD = 0;
    }
    else
    {
        cur->i16_enable[AML_I16_Vertical] = 1;
        cur->i16_enable[AML_I16_Horizontal] = 1;
        cur->i16_enable[AML_I16_DC] = 1;
    }
#endif
    min_cost = MBIntraSearch_P16X16(encvid , input_mb, cost, flag);
    if (input_mb)
        chroma_intra_search(&cur->neighbor, input_mb);
    return min_cost;
}

int MBIntraSearch_P4(amvenc_pred_mode_obj_t *encvid,
                     int x, int y, unsigned char *input_mb, int cost, unsigned char *pred_mode)
{
    amvenc_curr_pred_mode_t *cur    = encvid->mb_node;
    amvenc_pred_mode_t      *common = encvid->mb_list;
    int min_cost = 0;
    cur->neighbor.mbx = x;
    cur->neighbor.mby = y;
    InitNeighborAvailability_iframe(common, cur);
    min_cost = MBIntraSearch_P4X4(encvid , input_mb, cost, pred_mode);
    if (input_mb)
        chroma_intra_search(&cur->neighbor, input_mb);
    return min_cost;
}

int blk_intra4x4_fixed(amvenc_pred_mode_obj_t *encvid, int blkidx, uint8_t *cur, uint8_t *org, unsigned pred_mode)
{
    amvenc_curr_pred_mode_t *video = encvid->mb_node;
    AMLNeighborAvailability availability;
    amvenc_pred_mode_t *common = encvid->mb_list;
    AMLMacroblock *currMB = video->currMB;
    unsigned char top_left = 0;
    int pitch = common->pitch;
    uint32_t temp, DC;
    uint8_t *pred;
    int org_pitch = common->pitch;
    uint16_t min_cost, cost;
    int P_x, Q_x, R_x, P_y, Q_y, R_y, D, D0, D1;
    int P0, Q0, R0, S0, P1, Q1, R1, P2, Q2;
    uint8_t P_A, P_B, P_C, P_D, P_E, P_F, P_G, P_H, P_I, P_J, P_K, P_L, P_X;
    int r0, r1, r2, r3, r4, r5, r6, r7;
    int x0, x1, x2, x3, x4, x5;
    uint32_t temp1, temp2;
    int ipmode;
    int fixedcost = 4 * encvid->lambda_mode;
    int min_sad = 0x7FFF;
    bool pred_available = false;
    availability.left = true;
    availability.top = true;
    if (blkidx <= 3) /* top row block  (!block_y) */
    {
        /* check availability up */
        availability.top = video->neighbor.mbAvailB ;
    }
    if (!(blkidx & 0x3)) /* left column block (!block_x)*/
    {
        /* check availability left */
        availability.left = video->neighbor.mbAvailA ;
    }
    availability.top_right = BlkTopRight[blkidx];
    if (availability.top_right == 2)
    {
        availability.top_right = video->neighbor.mbAvailB;
    }
    else if (availability.top_right == 3)
    {
        availability.top_right = video->neighbor.mbAvailC;
    }
    if (availability.top == true)
    {
        temp = *(uint32_t *)(cur - pitch);
        P_A = temp & 0xFF;
        P_B = (temp >> 8) & 0xFF;
        P_C = (temp >> 16) & 0xFF;
        P_D = (temp >> 24) & 0xFF;
    }
    else
    {
        P_A = P_B = P_C = P_D = 128;
    }
    if (availability.top_right)
    {
        temp = *(uint32_t *)(cur - pitch + 4);
        P_E = temp & 0xFF;
        P_F = (temp >> 8) & 0xFF;
        P_G = (temp >> 16) & 0xFF;
        P_H = (temp >> 24) & 0xFF;
    }
    else
    {
        P_E = P_F = P_G = P_H = 128;
    }
    if (availability.left == true)
    {
        cur--;
        P_I = *cur;
        P_J = *(cur += pitch);
        P_K = *(cur += pitch);
        P_L = *(cur + pitch);
        cur -= (pitch << 1);
        cur++;
    }
    else
    {
        P_I = P_J = P_K = P_L = 128;
    }
    if (((blkidx > 3) && (blkidx & 0x3)) || ((blkidx > 3) && video->neighbor.mbAvailA)
            || ((blkidx & 0x3) && video->neighbor.mbAvailB)
            || (video->neighbor.mbAvailA && video->neighbor.mbAvailD && video->neighbor.mbAvailB))
    {
        top_left = 1;
        P_X = *(cur - pitch - 1);
    }
    else
    {
        P_X = 128;
    }
    //===== INTRA PREDICTION FOR 4x4 BLOCK =====
    /* vertical */
    if ((availability.top) && (pred_mode == AML_I4_Vertical))
    {
        pred = video->pred_i4[AML_I4_Vertical];
        temp = (P_D << 24) | (P_C << 16) | (P_B << 8) | P_A ;
        *((uint32_t *)pred) =  temp; /* write 4 at a time */
        *((uint32_t *)(pred += 4)) =  temp;
        *((uint32_t *)(pred += 4)) =  temp;
        *((uint32_t *)(pred += 4)) =  temp;
        pred_available = true;
    }
    else if ((availability.left) && (pred_mode == AML_I4_Horizontal))
    {
        pred = video->pred_i4[AML_I4_Horizontal];
        temp = P_I | (P_I << 8);
        temp = temp | (temp << 16);
        *((uint32_t *)pred) = temp;
        temp = P_J | (P_J << 8);
        temp = temp | (temp << 16);
        *((uint32_t *)(pred += 4)) = temp;
        temp = P_K | (P_K << 8);
        temp = temp | (temp << 16);
        *((uint32_t *)(pred += 4)) = temp;
        temp = P_L | (P_L << 8);
        temp = temp | (temp << 16);
        *((uint32_t *)(pred += 4)) = temp;
        pred_available = true;
    }
    else if ((availability.left) && (pred_mode == AML_I4_Horizontal_Up))
    {
        pred = video->pred_i4[AML_I4_Horizontal_Up];
        Q0 = (P_J + P_K + 1) >> 1;
        Q1 = (P_J + (P_K << 1) + P_L + 2) >> 2;
        P0 = ((P_I + P_J + 1) >> 1);
        P1 = ((P_I + (P_J << 1) + P_K + 2) >> 2);
        temp = P0 | (P1 << 8);      // [P0 P1 Q0 Q1]
        temp |= (Q0 << 16);     // [Q0 Q1 R0 DO]
        temp |= (Q1 << 24);     // [R0 D0 D1 D1]
        *((uint32_t *)pred) = temp;     // [D1 D1 D1 D1]
        D0 = (P_K + 3 * P_L + 2) >> 2;
        R0 = (P_K + P_L + 1) >> 1;
        temp = Q0 | (Q1 << 8);
        temp |= (R0 << 16);
        temp |= (D0 << 24);
        *((uint32_t *)(pred += 4)) = temp;
        D1 = P_L;
        temp = R0 | (D0 << 8);
        temp |= (D1 << 16);
        temp |= (D1 << 24);
        *((uint32_t *)(pred += 4)) = temp;
        temp = D1 | (D1 << 8);
        temp |= (temp << 16);
        *((uint32_t *)(pred += 4)) = temp;
        pred_available = true;
    }
    else if (pred_mode == AML_I4_DC)
    {
        pred = video->pred_i4[AML_I4_DC];
        if (availability.left)
        {
            DC = P_I + P_J + P_K + P_L;
            if (availability.top)
                DC = (P_A + P_B + P_C + P_D + DC + 4) >> 3;
            else
                DC = (DC + 2) >> 2;
        }
        else if (availability.top)
        {
            DC = (P_A + P_B + P_C + P_D + 2) >> 2;
        }
        else
        {
            DC = 128;
        }
        temp = DC | (DC << 8);
        temp = temp | (temp << 16);
        *((uint32_t *)pred) = temp;
        *((uint32_t *)(pred += 4)) = temp;
        *((uint32_t *)(pred += 4)) = temp;
        *((uint32_t *)(pred += 4)) = temp;
        pred_available = true;
    }
    else if ((availability.top) && (pred_mode == AML_I4_Diagonal_Down_Left))
    {
        pred = video->pred_i4[AML_I4_Diagonal_Down_Left];
        r0 = P_A;
        r1 = P_B;
        r2 = P_C;
        r3 = P_D;
        r0 += (r1 << 1);
        r0 += r2;
        r0 += 2;
        r0 >>= 2;
        r1 += (r2 << 1);
        r1 += r3;
        r1 += 2;
        r1 >>= 2;
        if (availability.top_right)
        {
            r4 = P_E;
            r5 = P_F;
            r6 = P_G;
            r7 = P_H;
            r2 += (r3 << 1);
            r2 += r4;
            r2 += 2;
            r2 >>= 2;
            r3 += (r4 << 1);
            r3 += r5;
            r3 += 2;
            r3 >>= 2;
            r4 += (r5 << 1);
            r4 += r6;
            r4 += 2;
            r4 >>= 2;
            r5 += (r6 << 1);
            r5 += r7;
            r5 += 2;
            r5 >>= 2;
            r6 += (3 * r7);
            r6 += 2;
            r6 >>= 2;
            temp = r0 | (r1 << 8);
            temp |= (r2 << 16);
            temp |= (r3 << 24);
            *((uint32_t *)pred) = temp;
            temp = (temp >> 8) | (r4 << 24);
            *((uint32_t *)(pred += 4)) = temp;
            temp = (temp >> 8) | (r5 << 24);
            *((uint32_t *)(pred += 4)) = temp;
            temp = (temp >> 8) | (r6 << 24);
            *((uint32_t *)(pred += 4)) = temp;
        }
        else
        {
            r2 += (r3 * 3);
            r2 += 2;
            r2 >>= 2;
            r3 = ((r3 << 2) + 2);
            r3 >>= 2;
            temp = r0 | (r1 << 8);
            temp |= (r2 << 16);
            temp |= (r3 << 24);
            *((uint32_t *)pred) = temp;
            temp = (temp >> 8) | (r3 << 24);
            *((uint32_t *)(pred += 4)) = temp;
            temp = (temp >> 8) | (r3 << 24);
            *((uint32_t *)(pred += 4)) = temp;
            temp = (temp >> 8) | (r3 << 24);
            *((uint32_t *)(pred += 4)) = temp;
        }
        pred_available = true;
    }
    if ((top_left == 1) && (pred_mode == AML_I4_Diagonal_Down_Right))
    {
        pred = video->pred_i4[AML_I4_Diagonal_Down_Right];
        Q_x = (P_A + 2 * P_B + P_C + 2) >> 2;
        R_x = (P_B + 2 * P_C + P_D + 2) >> 2;
        P_x = (P_X + 2 * P_A + P_B + 2) >> 2;
        D   = (P_A + 2 * P_X + P_I + 2) >> 2;
        P_y = (P_X + 2 * P_I + P_J + 2) >> 2;
        Q_y = (P_I + 2 * P_J + P_K + 2) >> 2;
        R_y = (P_J + 2 * P_K + P_L + 2) >> 2;
        temp =  D | (P_x << 8);   //[D   P_x Q_x R_x]
        temp |= (Q_x << 16); //[Q_y P_y D   P_x]
        temp |= (R_x << 24);  //[R_y Q_y P_y D  ]
        *((uint32_t *)pred) = temp;
        temp =  P_y | (D << 8);
        temp |= (P_x << 16);
        temp |= (Q_x << 24);
        *((uint32_t *)(pred += 4)) = temp;
        temp =  Q_y | (P_y << 8);
        temp |= (D << 16);
        temp |= (P_x << 24);
        *((uint32_t *)(pred += 4)) = temp;
        temp = R_y | (Q_y << 8);
        temp |= (P_y << 16);
        temp |= (D << 24);
        *((uint32_t *)(pred += 4)) = temp;
        pred_available = true;
    }
    else if ((top_left == 1) && (pred_mode == AML_I4_Vertical_Right))
    {
        pred = video->pred_i4[AML_I4_Vertical_Right];
        Q0 = P_A + P_B + 1;
        R0 = P_B + P_C + 1;
        S0 = P_C + P_D + 1;
        P0 = P_X + P_A + 1;
        D = (P_I + 2 * P_X + P_A + 2) >> 2;
        P1 = (P0 + Q0) >> 2;
        Q1 = (Q0 + R0) >> 2;
        R1 = (R0 + S0) >> 2;
        P0 >>= 1;
        Q0 >>= 1;
        R0 >>= 1;
        S0 >>= 1;
        P2 = (P_X + 2 * P_I + P_J + 2) >> 2;
        Q2 = (P_I + 2 * P_J + P_K + 2) >> 2;
        temp =  P0 | (Q0 << 8);  //[P0 Q0 R0 S0]
        temp |= (R0 << 16); //[P2 P0 Q0 R0]
        temp |= (S0 << 24); //[Q2 D  P1 Q1]
        *((uint32_t *)pred) =  temp;
        temp =  D | (P1 << 8);
        temp |= (Q1 << 16);
        temp |= (R1 << 24);
        *((uint32_t *)(pred += 4)) =  temp;
        temp = P2 | (P0 << 8);
        temp |= (Q0 << 16);
        temp |= (R0 << 24);
        *((uint32_t *)(pred += 4)) =  temp;
        temp = Q2 | (D << 8);
        temp |= (P1 << 16);
        temp |= (Q1 << 24);
        *((uint32_t *)(pred += 4)) =  temp;
        pred_available = true;
    }
    else if ((top_left == 1) && (pred_mode == AML_I4_Horizontal_Down))
    {
        pred = video->pred_i4[AML_I4_Horizontal_Down];
        Q2 = (P_A + 2 * P_B + P_C + 2) >> 2;
        P2 = (P_X + 2 * P_A + P_B + 2) >> 2;
        D = (P_I + 2 * P_X + P_A + 2) >> 2;
        P0 = P_X + P_I + 1;
        Q0 = P_I + P_J + 1;
        R0 = P_J + P_K + 1;
        S0 = P_K + P_L + 1;
        P1 = (P0 + Q0) >> 2;
        Q1 = (Q0 + R0) >> 2;
        R1 = (R0 + S0) >> 2;
        P0 >>= 1;
        Q0 >>= 1;
        R0 >>= 1;
        S0 >>= 1;
        temp = P0 | (D << 8);   //[P0 D  P2 Q2]
        temp |= (P2 << 16);  //[R0 Q1 Q0 P1]
        temp |= (Q2 << 24); //[S0 R1 R0 Q1]
        *((uint32_t *)pred) = temp;
        temp = Q0 | (P1 << 8);
        temp |= (P0 << 16);
        temp |= (D << 24);
        *((uint32_t *)(pred += 4)) = temp;
        temp = R0 | (Q1 << 8);
        temp |= (Q0 << 16);
        temp |= (P1 << 24);
        *((uint32_t *)(pred += 4)) = temp;
        temp = S0 | (R1 << 8);
        temp |= (R0 << 16);
        temp |= (Q1 << 24);
        *((uint32_t *)(pred += 4)) = temp;
        pred_available = true;
    }
    else if ((availability.top) && (pred_mode == AML_I4_Vertical_Left))
    {
        pred = video->pred_i4[AML_I4_Vertical_Left];
        x0 = P_A + P_B + 1;
        x1 = P_B + P_C + 1;
        x2 = P_C + P_D + 1;
        if (availability.top_right)
        {
            x3 = P_D + P_E + 1;
            x4 = P_E + P_F + 1;
            x5 = P_F + P_G + 1;
        }
        else
        {
            x3 = x4 = x5 = (P_D << 1) + 1;
        }
        temp1 = (x0 >> 1);
        temp1 |= ((x1 >> 1) << 8);
        temp1 |= ((x2 >> 1) << 16);
        temp1 |= ((x3 >> 1) << 24);
        *((uint32_t *)pred) = temp1;
        temp2 = ((x0 + x1) >> 2);
        temp2 |= (((x1 + x2) >> 2) << 8);
        temp2 |= (((x2 + x3) >> 2) << 16);
        temp2 |= (((x3 + x4) >> 2) << 24);
        *((uint32_t *)(pred += 4)) = temp2;
        temp1 = (temp1 >> 8) | ((x4 >> 1) << 24);   /* rotate out old value */
        *((uint32_t *)(pred += 4)) = temp1;
        temp2 = (temp2 >> 8) | (((x4 + x5) >> 2) << 24); /* rotate out old value */
        *((uint32_t *)(pred += 4)) = temp2;
        pred_available = true;
    }
    min_cost = 0xFFFF;
    if (pred_available == true)
    {
        //cost  = (ipmode == mostProbableMode) ? 0 : fixedcost;  // need init cost?
        cost = 0;
        pred = video->pred_i4[pred_mode];
        pred_cost_i4(org, org_pitch, pred, &cost);
        currMB->i4Mode[blkidx] = (AMLIntra4x4PredMode)pred_mode;
        min_cost = cost;
        //min_sad = cost - ((ipmode == mostProbableMode) ? 0 : fixedcost);
    }
    return min_cost;
}

void mb_intra4x4_search(amvenc_pred_mode_obj_t *encvid, int *min_cost, unsigned char *pred_mode)
{
    amvenc_curr_pred_mode_t *video = encvid->mb_node;
    AMLMacroblock *currMB;
    amvenc_pred_mode_t *common = encvid->mb_list;
    currMB = video->currMB;
    int pitch = common->pitch;
    int org_pitch = common->pitch;
    int offset;
    uint8_t *curL, *comp, *org4, *org8;
    int y = video->neighbor.mby << 4;
    int x = video->neighbor.mbx << 4;
    int b8, b4, cost4x4, blkidx;
    int cost = 0;
    int numcoef;
    int dummy = 0;
    int mb_intra = currMB->mb_intra; // save the original value
    unsigned char cur_pred = 0xf;
    offset = y * pitch + x;
    curL = encvid->YCbCr[0] + offset;
    org8 = encvid->YCbCr[0] + y * org_pitch + x;
    cost = (int)(6.0 * encvid->lambda_mode + 0.4999); // need init cost ?
    cost <<= 2;
    currMB->mb_intra = 1;  // temporary set this to one to enable the IDCT
    // operation inside dct_luma
    for (b8 = 0; b8 < 4; b8++)
    {
        comp = curL;
        org4 = org8;
        for (b4 = 0; b4 < 4; b4++)
        {
            blkidx = blkId_2blkXY[b8][b4];
            if (pred_mode)
            {
                cur_pred =  pred_mode[blkidx];
                cost4x4 = blk_intra4x4_fixed(encvid, blkidx, comp, org4, cur_pred);
            }
            else
            {
                cost4x4 = blk_intra4x4_search_iframe(encvid, blkidx, comp, org4);
            }
            cost += cost4x4;
            if ((cost > *min_cost) && (pred_mode == NULL))
            {
                currMB->mb_intra = mb_intra; // restore the value
                return ;
            }
            if (b4 & 1)
            {
                comp += ((pitch << 2) - 4);
                org4 += ((org_pitch << 2) - 4);
            }
            else
            {
                comp += 4;
                org4 += 4;
            }
        }
        if (b8 & 1)
        {
            curL += ((pitch << 3) - 8);
            org8 += ((org_pitch << 3) - 8);
        }
        else
        {
            curL += 8;
            org8 += 8;
        }
    }
    currMB->mb_intra = mb_intra; // restore the value
    if (cost < *min_cost)
    {
        *min_cost = cost;
        currMB->mbMode = AML_I4;
        currMB->mb_intra = 1;
    }
    if (pred_mode)
        *min_cost = cost;
    return ;
}

/* search for i4 mode for a 4x4 block */
int blk_intra4x4_search_iframe(amvenc_pred_mode_obj_t *encvid, int blkidx, uint8_t *cur, uint8_t *org)
{
    amvenc_curr_pred_mode_t *video = encvid->mb_node;
    AMLNeighborAvailability availability;
    amvenc_pred_mode_t *common = encvid->mb_list;
    //video->currMB = common->mblock + video->mbNum;
    AMLMacroblock *currMB = video->currMB;
    unsigned char top_left = 0;
    int pitch = common->pitch;
    uint8_t mode_avail[AMLNumI4PredMode];
    //uint8 mode_avail[AMLNumI4PredMode];
    uint32_t temp, DC;
    uint8_t *pred;
    int org_pitch = common->pitch;
    uint16_t min_cost, cost;
    //amvenc_neighbor_t *n = &video->neighbor;
    int P_x, Q_x, R_x, P_y, Q_y, R_y, D, D0, D1;
    int P0, Q0, R0, S0, P1, Q1, R1, P2, Q2;
    uint8_t P_A, P_B, P_C, P_D, P_E, P_F, P_G, P_H, P_I, P_J, P_K, P_L, P_X;
    int r0, r1, r2, r3, r4, r5, r6, r7;
    int x0, x1, x2, x3, x4, x5;
    uint32_t temp1, temp2;
    int ipmode, mostProbableMode;
    int fixedcost = 4 * encvid->lambda_mode;
    int min_sad = 0x7FFF;
    availability.left = true;
    availability.top = true;
    if (blkidx <= 3) /* top row block  (!block_y) */
    {
        /* check availability up */
        availability.top = video->neighbor.mbAvailB ;
    }
    if (!(blkidx & 0x3)) /* left column block (!block_x)*/
    {
        /* check availability left */
        availability.left = video->neighbor.mbAvailA ;
    }
    availability.top_right = BlkTopRight[blkidx];
    if (availability.top_right == 2)
    {
        availability.top_right = video->neighbor.mbAvailB;
    }
    else if (availability.top_right == 3)
    {
        availability.top_right = video->neighbor.mbAvailC;
    }
    if (availability.top == true)
    {
        temp = *(uint32_t *)(cur - pitch);
        P_A = temp & 0xFF;
        P_B = (temp >> 8) & 0xFF;
        P_C = (temp >> 16) & 0xFF;
        P_D = (temp >> 24) & 0xFF;
    }
    else
    {
        P_A = P_B = P_C = P_D = 128;
    }
    if (availability.top_right)
    {
        temp = *(uint32_t *)(cur - pitch + 4);
        P_E = temp & 0xFF;
        P_F = (temp >> 8) & 0xFF;
        P_G = (temp >> 16) & 0xFF;
        P_H = (temp >> 24) & 0xFF;
    }
    else
    {
        P_E = P_F = P_G = P_H = 128;
    }
    if (availability.left == true)
    {
        cur--;
        P_I = *cur;
        P_J = *(cur += pitch);
        P_K = *(cur += pitch);
        P_L = *(cur + pitch);
        cur -= (pitch << 1);
        cur++;
    }
    else
    {
        P_I = P_J = P_K = P_L = 128;
    }
    if (((blkidx > 3) && (blkidx & 0x3)) || ((blkidx > 3) && video->neighbor.mbAvailA)
            || ((blkidx & 0x3) && video->neighbor.mbAvailB)
            || (video->neighbor.mbAvailA && video->neighbor.mbAvailD && video->neighbor.mbAvailB))
    {
        top_left = 1;
        P_X = *(cur - pitch - 1);
    }
    else
    {
        P_X = 128;
    }
    //===== INTRA PREDICTION FOR 4x4 BLOCK =====
    /* vertical */
    mode_avail[AML_I4_Vertical] = 0;
    if (availability.top)
    {
        mode_avail[AML_I4_Vertical] = 1;
        pred = video->pred_i4[AML_I4_Vertical];
        temp = (P_D << 24) | (P_C << 16) | (P_B << 8) | P_A ;
        *((uint32_t *)pred) =  temp; /* write 4 at a time */
        *((uint32_t *)(pred += 4)) =  temp;
        *((uint32_t *)(pred += 4)) =  temp;
        *((uint32_t *)(pred += 4)) =  temp;
    }
    /* horizontal */
    mode_avail[AML_I4_Horizontal] = 0;
    mode_avail[AML_I4_Horizontal_Up] = 0;
    if (availability.left)
    {
        mode_avail[AML_I4_Horizontal] = 1;
        pred = video->pred_i4[AML_I4_Horizontal];
        temp = P_I | (P_I << 8);
        temp = temp | (temp << 16);
        *((uint32_t *)pred) = temp;
        temp = P_J | (P_J << 8);
        temp = temp | (temp << 16);
        *((uint32_t *)(pred += 4)) = temp;
        temp = P_K | (P_K << 8);
        temp = temp | (temp << 16);
        *((uint32_t *)(pred += 4)) = temp;
        temp = P_L | (P_L << 8);
        temp = temp | (temp << 16);
        *((uint32_t *)(pred += 4)) = temp;
        mode_avail[AML_I4_Horizontal_Up] = 1;
        pred = video->pred_i4[AML_I4_Horizontal_Up];
        Q0 = (P_J + P_K + 1) >> 1;
        Q1 = (P_J + (P_K << 1) + P_L + 2) >> 2;
        P0 = ((P_I + P_J + 1) >> 1);
        P1 = ((P_I + (P_J << 1) + P_K + 2) >> 2);
        temp = P0 | (P1 << 8);      // [P0 P1 Q0 Q1]
        temp |= (Q0 << 16);     // [Q0 Q1 R0 DO]
        temp |= (Q1 << 24);     // [R0 D0 D1 D1]
        *((uint32_t *)pred) = temp;     // [D1 D1 D1 D1]
        D0 = (P_K + 3 * P_L + 2) >> 2;
        R0 = (P_K + P_L + 1) >> 1;
        temp = Q0 | (Q1 << 8);
        temp |= (R0 << 16);
        temp |= (D0 << 24);
        *((uint32_t *)(pred += 4)) = temp;
        D1 = P_L;
        temp = R0 | (D0 << 8);
        temp |= (D1 << 16);
        temp |= (D1 << 24);
        *((uint32_t *)(pred += 4)) = temp;
        temp = D1 | (D1 << 8);
        temp |= (temp << 16);
        *((uint32_t *)(pred += 4)) = temp;
    }
    /* DC */
    mode_avail[AML_I4_DC] = 1;
    pred = video->pred_i4[AML_I4_DC];
    if (availability.left)
    {
        DC = P_I + P_J + P_K + P_L;
        if (availability.top)
        {
            DC = (P_A + P_B + P_C + P_D + DC + 4) >> 3;
        }
        else
        {
            DC = (DC + 2) >> 2;
        }
    }
    else if (availability.top)
    {
        DC = (P_A + P_B + P_C + P_D + 2) >> 2;
    }
    else
    {
        DC = 128;
    }
    temp = DC | (DC << 8);
    temp = temp | (temp << 16);
    *((uint32_t *)pred) = temp;
    *((uint32_t *)(pred += 4)) = temp;
    *((uint32_t *)(pred += 4)) = temp;
    *((uint32_t *)(pred += 4)) = temp;
    /* Down-left */
    mode_avail[AML_I4_Diagonal_Down_Left] = 0;
    if (availability.top)
    {
        mode_avail[AML_I4_Diagonal_Down_Left] = 1;
        pred = video->pred_i4[AML_I4_Diagonal_Down_Left];
        r0 = P_A;
        r1 = P_B;
        r2 = P_C;
        r3 = P_D;
        r0 += (r1 << 1);
        r0 += r2;
        r0 += 2;
        r0 >>= 2;
        r1 += (r2 << 1);
        r1 += r3;
        r1 += 2;
        r1 >>= 2;
        if (availability.top_right)
        {
            r4 = P_E;
            r5 = P_F;
            r6 = P_G;
            r7 = P_H;
            r2 += (r3 << 1);
            r2 += r4;
            r2 += 2;
            r2 >>= 2;
            r3 += (r4 << 1);
            r3 += r5;
            r3 += 2;
            r3 >>= 2;
            r4 += (r5 << 1);
            r4 += r6;
            r4 += 2;
            r4 >>= 2;
            r5 += (r6 << 1);
            r5 += r7;
            r5 += 2;
            r5 >>= 2;
            r6 += (3 * r7);
            r6 += 2;
            r6 >>= 2;
            temp = r0 | (r1 << 8);
            temp |= (r2 << 16);
            temp |= (r3 << 24);
            *((uint32_t *)pred) = temp;
            temp = (temp >> 8) | (r4 << 24);
            *((uint32_t *)(pred += 4)) = temp;
            temp = (temp >> 8) | (r5 << 24);
            *((uint32_t *)(pred += 4)) = temp;
            temp = (temp >> 8) | (r6 << 24);
            *((uint32_t *)(pred += 4)) = temp;
        }
        else
        {
            r2 += (r3 * 3);
            r2 += 2;
            r2 >>= 2;
            r3 = ((r3 << 2) + 2);
            r3 >>= 2;
            temp = r0 | (r1 << 8);
            temp |= (r2 << 16);
            temp |= (r3 << 24);
            *((uint32_t *)pred) = temp;
            temp = (temp >> 8) | (r3 << 24);
            *((uint32_t *)(pred += 4)) = temp;
            temp = (temp >> 8) | (r3 << 24);
            *((uint32_t *)(pred += 4)) = temp;
            temp = (temp >> 8) | (r3 << 24);
            *((uint32_t *)(pred += 4)) = temp;
        }
    }
    /* Down Right */
    mode_avail[AML_I4_Diagonal_Down_Right] = 0;
    /* Diagonal Vertical Right */
    mode_avail[AML_I4_Vertical_Right] = 0;
    /* Horizontal Down */
    mode_avail[AML_I4_Horizontal_Down] = 0;
    if (top_left == 1)
    {
        mode_avail[AML_I4_Diagonal_Down_Right] = 1;
        pred = video->pred_i4[AML_I4_Diagonal_Down_Right];
        Q_x = (P_A + 2 * P_B + P_C + 2) >> 2;
        R_x = (P_B + 2 * P_C + P_D + 2) >> 2;
        P_x = (P_X + 2 * P_A + P_B + 2) >> 2;
        D   = (P_A + 2 * P_X + P_I + 2) >> 2;
        P_y = (P_X + 2 * P_I + P_J + 2) >> 2;
        Q_y = (P_I + 2 * P_J + P_K + 2) >> 2;
        R_y = (P_J + 2 * P_K + P_L + 2) >> 2;
        temp =  D | (P_x << 8);   //[D   P_x Q_x R_x]
        temp |= (Q_x << 16); //[Q_y P_y D   P_x]
        temp |= (R_x << 24);  //[R_y Q_y P_y D  ]
        *((uint32_t *)pred) = temp;
        temp =  P_y | (D << 8);
        temp |= (P_x << 16);
        temp |= (Q_x << 24);
        *((uint32_t *)(pred += 4)) = temp;
        temp =  Q_y | (P_y << 8);
        temp |= (D << 16);
        temp |= (P_x << 24);
        *((uint32_t *)(pred += 4)) = temp;
        temp = R_y | (Q_y << 8);
        temp |= (P_y << 16);
        temp |= (D << 24);
        *((uint32_t *)(pred += 4)) = temp;
        mode_avail[AML_I4_Vertical_Right] = 1;
        pred = video->pred_i4[AML_I4_Vertical_Right];
        Q0 = P_A + P_B + 1;
        R0 = P_B + P_C + 1;
        S0 = P_C + P_D + 1;
        P0 = P_X + P_A + 1;
        D = (P_I + 2 * P_X + P_A + 2) >> 2;
        P1 = (P0 + Q0) >> 2;
        Q1 = (Q0 + R0) >> 2;
        R1 = (R0 + S0) >> 2;
        P0 >>= 1;
        Q0 >>= 1;
        R0 >>= 1;
        S0 >>= 1;
        P2 = (P_X + 2 * P_I + P_J + 2) >> 2;
        Q2 = (P_I + 2 * P_J + P_K + 2) >> 2;
        temp =  P0 | (Q0 << 8);  //[P0 Q0 R0 S0]
        temp |= (R0 << 16); //[P2 P0 Q0 R0]
        temp |= (S0 << 24); //[Q2 D  P1 Q1]
        *((uint32_t *)pred) =  temp;
        temp =  D | (P1 << 8);
        temp |= (Q1 << 16);
        temp |= (R1 << 24);
        *((uint32_t *)(pred += 4)) =  temp;
        temp = P2 | (P0 << 8);
        temp |= (Q0 << 16);
        temp |= (R0 << 24);
        *((uint32_t *)(pred += 4)) =  temp;
        temp = Q2 | (D << 8);
        temp |= (P1 << 16);
        temp |= (Q1 << 24);
        *((uint32_t *)(pred += 4)) =  temp;
        mode_avail[AML_I4_Horizontal_Down] = 1;
        pred = video->pred_i4[AML_I4_Horizontal_Down];
        Q2 = (P_A + 2 * P_B + P_C + 2) >> 2;
        P2 = (P_X + 2 * P_A + P_B + 2) >> 2;
        D = (P_I + 2 * P_X + P_A + 2) >> 2;
        P0 = P_X + P_I + 1;
        Q0 = P_I + P_J + 1;
        R0 = P_J + P_K + 1;
        S0 = P_K + P_L + 1;
        P1 = (P0 + Q0) >> 2;
        Q1 = (Q0 + R0) >> 2;
        R1 = (R0 + S0) >> 2;
        P0 >>= 1;
        Q0 >>= 1;
        R0 >>= 1;
        S0 >>= 1;
        temp = P0 | (D << 8);   //[P0 D  P2 Q2]
        temp |= (P2 << 16);  //[R0 Q1 Q0 P1]
        temp |= (Q2 << 24); //[S0 R1 R0 Q1]
        *((uint32_t *)pred) = temp;
        temp = Q0 | (P1 << 8);
        temp |= (P0 << 16);
        temp |= (D << 24);
        *((uint32_t *)(pred += 4)) = temp;
        temp = R0 | (Q1 << 8);
        temp |= (Q0 << 16);
        temp |= (P1 << 24);
        *((uint32_t *)(pred += 4)) = temp;
        temp = S0 | (R1 << 8);
        temp |= (R0 << 16);
        temp |= (Q1 << 24);
        *((uint32_t *)(pred += 4)) = temp;
    }
    /* vertical left */
    mode_avail[AML_I4_Vertical_Left] = 0;
    if (availability.top)
    {
        mode_avail[AML_I4_Vertical_Left] = 1;
        pred = video->pred_i4[AML_I4_Vertical_Left];
        x0 = P_A + P_B + 1;
        x1 = P_B + P_C + 1;
        x2 = P_C + P_D + 1;
        if (availability.top_right)
        {
            x3 = P_D + P_E + 1;
            x4 = P_E + P_F + 1;
            x5 = P_F + P_G + 1;
        }
        else
        {
            x3 = x4 = x5 = (P_D << 1) + 1;
        }
        temp1 = (x0 >> 1);
        temp1 |= ((x1 >> 1) << 8);
        temp1 |= ((x2 >> 1) << 16);
        temp1 |= ((x3 >> 1) << 24);
        *((uint32_t *)pred) = temp1;
        temp2 = ((x0 + x1) >> 2);
        temp2 |= (((x1 + x2) >> 2) << 8);
        temp2 |= (((x2 + x3) >> 2) << 16);
        temp2 |= (((x3 + x4) >> 2) << 24);
        *((uint32_t *)(pred += 4)) = temp2;
        temp1 = (temp1 >> 8) | ((x4 >> 1) << 24);   /* rotate out old value */
        *((uint32_t *)(pred += 4)) = temp1;
        temp2 = (temp2 >> 8) | (((x4 + x5) >> 2) << 24); /* rotate out old value */
        *((uint32_t *)(pred += 4)) = temp2;
    }
    //===== LOOP OVER ALL 4x4 INTRA PREDICTION MODES =====
    // can re-order the search here instead of going in order
    // find most probable mode
    video->mostProbableI4Mode[blkidx] = mostProbableMode = FindMostProbableI4Mode(encvid, blkidx);
    min_cost = 0xFFFF;
    for (ipmode = 0; ipmode < AMLNumI4PredMode; ipmode++)
    {
        if ((mode_avail[ipmode] == true) && (video->i4_enable[ipmode]))
        {
            cost  = (ipmode == mostProbableMode) ? 0 : fixedcost;
            pred = video->pred_i4[ipmode];
            pred_cost_i4(org, org_pitch, pred, &cost);
            if (cost < min_cost)
            {
                currMB->i4Mode[blkidx] = (AMLIntra4x4PredMode)ipmode;
                min_cost   = cost;
                min_sad = cost - ((ipmode == mostProbableMode) ? 0 : fixedcost);
            }
        }
    }
    return min_cost;
}

int FindMostProbableI4Mode(amvenc_pred_mode_obj_t *encvid, int blkidx)
{
    int dcOnlyPredictionFlag;
    amvenc_curr_pred_mode_t *video = encvid->mb_node;
    AMLMacroblock *currMB = video->currMB;
    amvenc_pred_mode_t *common = encvid->mb_list;
    int intra4x4PredModeA = AML_I4_DC;
    int intra4x4PredModeB = AML_I4_DC;
    int predIntra4x4PredMode = AML_I4_DC;
    dcOnlyPredictionFlag = 0;
    if (blkidx & 0x3)
    {
        intra4x4PredModeA = currMB->i4Mode[blkidx - 1]; // block to the left
    }
    else /* for blk 0, 4, 8, 12 */
    {
        if (video->neighbor.mbAvailA)
        {
#if 0
            if (video->mblock[video->mbAddrA].mbMode == AML_I4)
            {
                intra4x4PredModeA = video->mblock[video->mbAddrA].i4Mode[blkidx + 3];
            }
            else
            {
                intra4x4PredModeA = AML_I4_DC;
            }
#else
            intra4x4PredModeA = common->mblock[video->mbAddrA].i4Mode[blkidx + 3];
#endif
        }
        else
        {
            dcOnlyPredictionFlag = 1;
            goto PRED_RESULT_READY;  // skip below
        }
    }
    if (blkidx >> 2)
    {
        intra4x4PredModeB = currMB->i4Mode[blkidx - 4]; // block above
    }
    else /* block 0, 1, 2, 3 */
    {
        if (video->neighbor.mbAvailB)
        {
#if 0
            if (video->mblock[video->mbAddrB].mbMode == AML_I4)
            {
                intra4x4PredModeB = video->mblock[video->mbAddrB].i4Mode[blkidx + 12];
            }
            else
            {
                intra4x4PredModeB = AML_I4_DC;
            }
#else
            intra4x4PredModeB = common->mblock[video->mbAddrB].i4Mode[blkidx + 12];
#endif
        }
        else
        {
            dcOnlyPredictionFlag = 1;
        }
    }
PRED_RESULT_READY:
    if (dcOnlyPredictionFlag)
    {
        intra4x4PredModeA = intra4x4PredModeB = AML_I4_DC;
    }
    predIntra4x4PredMode = AML_MIN(intra4x4PredModeA, intra4x4PredModeB);
    return predIntra4x4PredMode;
}
