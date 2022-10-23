#ifndef __AMLENC_PRED_H__
#define __AMLENC_PRED_H__

#define AMLNumI4PredMode  9
#define AMLNumI16PredMode  4
#define AMLNumIChromaMode  4
#include <stdlib.h>
#include "enc_define.h"

/**
Types of the macroblock and partition. PV Created.
@publishedAll
*/
typedef enum
{
    /* intra */
    AML_I4,
    AML_I16,
    AML_I_PCM,
    AML_SI4,

    /* inter for both P and B*/
    AML_BDirect16,
    AML_P16,
    AML_P16x8,
    AML_P8x16,
    AML_P8,
    AML_P8ref0,
    AML_SKIP
} AMLMBMode;

/**
Mode of intra 4x4 prediction. Table 8-2
@publishedAll
*/
typedef enum
{
    AML_I4_Vertical = 0,
    AML_I4_Horizontal,
    AML_I4_DC,
    AML_I4_Diagonal_Down_Left,
    AML_I4_Diagonal_Down_Right,
    AML_I4_Vertical_Right,
    AML_I4_Horizontal_Down,
    AML_I4_Vertical_Left,
    AML_I4_Horizontal_Up
} AMLIntra4x4PredMode;

/**
Mode of intra 16x16 prediction. Table 8-3
@publishedAll
*/
typedef enum
{
    AML_I16_Vertical = 0,
    AML_I16_Horizontal,
    AML_I16_DC,
    AML_I16_Plane
} AMLIntra16x16PredMode;

/**
This slice type follows Table 7-3. The bottom 5 items may not needed.
@publishedAll
*/
typedef enum
{
    AML_P_SLICE = 0,
    AML_B_SLICE,
    AML_I_SLICE,
    AML_SP_SLICE,
    AML_SI_SLICE,
    AML_P_ALL_SLICE,
    AML_B_ALL_SLICE,
    AML_I_ALL_SLICE,
    AML_SP_ALL_SLICE,
    AML_SI_ALL_SLICE,
} AMLSliceType;

typedef struct
{
    int mbx;
    int mby;
    int mbAvailA;
    int mbAvailB;
    int mbAvailC;
    int mbAvailD;
} amvenc_neighbor_t;

/**
This structure contains macroblock related variables.
@publishedAll
*/
typedef struct tagMcrblck
{
    uint    mb_intra; /* intra flag */

    AMLMBMode mbMode;   /* type of MB prediction */
    AMLIntra16x16PredMode i16Mode; /* Intra16x16PredMode */
    AMLIntra4x4PredMode i4Mode[16]; /* Intra4x4PredMode, in raster scan order */
} AMLMacroblock;

typedef struct
{

    AMLMacroblock *mblock;
    int *min_cost;/* Minimum cost for the all MBs */

    int mb_width;
    int mb_height;
    int pitch;
    int height;

} amvenc_pred_mode_t;


typedef struct
{

    /********* intra prediction scratch memory **********************/
    uint8_t pred_i16[AMLNumI16PredMode][256]; /* save prediction for MB */
    uint8_t pred_i4[AMLNumI4PredMode][16];  /* save prediction for blk */
    uint8_t pred_ic[AMLNumIChromaMode][128];  /* for 2 chroma */

    int     mostProbableI4Mode[16]; /* in raster scan order */

    AMLMacroblock *currMB;
    AMLSliceType slice_type;
    uint                mbNum; /* number of current MB */

    int mbAddrA, mbAddrB, mbAddrC, mbAddrD; /* address of neighboring MBs */

    amvenc_neighbor_t neighbor;
    unsigned char i16_enable[AMLNumI16PredMode];
    unsigned char i4_enable[AMLNumI4PredMode];

} amvenc_curr_pred_mode_t;

typedef struct
{
    amvenc_pred_mode_t      *mb_list;
    amvenc_curr_pred_mode_t *mb_node;

    int lambda_mode;
    int lambda_motion;
    uint8_t *YCbCr[3];
    //unsigned plane[3];
} amvenc_pred_mode_obj_t;

void InitNeighborAvailability_iframe(amvenc_pred_mode_t *predMB,
                                     amvenc_curr_pred_mode_t *cur);

/* ******************************************************
 * ****************************** split
 * ******************************************************/
typedef struct tagNeighborAvail
{
    int left;
    int top;    /* macroblock address of the current pixel, see below */
    int top_right;      /* x,y positions of current pixel relative to the macroblock mb_addr */
} AMLNeighborAvailability;

amvenc_curr_pred_mode_t *InitcurrMBStruct();
int ClearcurrMBStruct(void *p);

void MBIntraSearch_clean(amvenc_pred_mode_obj_t *encvid);

int MBIntraSearch_full(amvenc_pred_mode_obj_t *encvid,
                       int x, int y, unsigned char *input_mb);
int MBIntraSearch_P16(amvenc_pred_mode_obj_t *encvid,
                      int x, int y, unsigned char *input_mb, int cost, int flag);
int MBIntraSearch_P4(amvenc_pred_mode_obj_t *encvid,
                     int x, int y, unsigned char *input_mb, int cost, unsigned char *pred_mode);
int MBIntraSearch_iframe(amvenc_pred_mode_obj_t *encvid,
                         unsigned char *input_addr);
void mb_intra4x4_search(amvenc_pred_mode_obj_t *encvid, int *min_cost, unsigned char *pred_mode);
int blk_intra4x4_search_iframe(amvenc_pred_mode_obj_t *encvid, int blkidx,
                               uint8_t *cur, uint8_t *org);
//int blk_intra4x4_fixed(amvenc_pred_mode_obj_t *encvid, int blkidx,
//        uint8_t *cur, uint8_t *org, unsigned char pred_mode);
int FindMostProbableI4Mode(amvenc_pred_mode_obj_t *encvid, int blkidx);
void find_cost_16x16(amvenc_pred_mode_obj_t *encvid,
                     /*AVCEncObject *encvid,*/ uint8_t *orgY, int *min_cost, int flag);

/**
This function calculates the cost of a given I4 prediction mode.
\param "org"    "Pointer to the original block."
\param "org_pitch"  "Stride size of the original frame."
\param "pred"   "Pointer to the prediction block. (encvid->pred_i4)"
\param "cost"   "Pointer to the minimal cost (to be updated)."
\return "void"
*/
extern "C" void pred_cost_i4(uint8_t *org, int org_pitch, uint8_t *pred, uint16_t *cost);
extern "C" int pred_cost_i16(uint8_t *org, int org_pitch, uint8_t *pred, int min_cost);
//intra mode

#endif
