#ifndef __INTRA_SEARCH_M8__
#define __INTRA_SEARCH_M8__
#include <stdlib.h>
#include "m8venclib.h"
#include "../intra_search/pred.h"

/******************************************************
 *
 * call before every frame
 * return an pointer to an struct used for MB intra search;
 *
******************************************************/
amvenc_pred_mode_obj_t *MBIntraSearch_prepare_m8(m8venc_drv_t* p)
{
        amvenc_pred_mode_obj_t *encvid = NULL;

        encvid = (amvenc_pred_mode_obj_t *) calloc(1, sizeof(amvenc_pred_mode_obj_t));
        //ALOGD("encvid=%p\n", encvid);
        if (encvid) {
                encvid->mb_list = &p->intra_mode;
                encvid->mb_node = InitcurrMBStruct();
                ALOGV("mb_list=%p, mb_node=%p\n", encvid->mb_list, encvid->mb_node);
#if 1//def COMPILE_TEST
                encvid->YCbCr[0]=(uint8_t*)p->src.plane[0];
                encvid->YCbCr[1]=(uint8_t*)p->src.plane[1];
                encvid->YCbCr[2]=(uint8_t*)p->src.plane[2];
                ALOGV("YCbCr[0]=%p, plane=%x\n", encvid->YCbCr[0], p->src.plane[0]);
#endif
        } else {
                ALOGE("calloc failed, %s, %d\n", __func__, __LINE__);
        }

#if 0
        /*this may change*/
#if 1//def COMPILE_TEST
        encvid->lambda_mode     = 1;//motion_search->lambda_mode;
        encvid->lambda_motion   = 1;//motion_search->lambda_motion;
#endif
        //need to add
        //need to add
        //need to add
        //need to add
#endif
        return encvid;
}

int InitMBIntraSearchModule_m8( m8venc_drv_t *p)
{
    amvenc_pred_mode_t *predMB = (amvenc_pred_mode_t *) (&p->intra_mode);
	uint totalMB;
	uint i;
	int *cost;

	//currMB = predMB->currMB;
	//predMB->mb_x = mb_x;
	//predMB->mb_y = mb_y;
	predMB->mb_width = p->src.mb_width;
	predMB->mb_height = p->src.mb_height;
	predMB->pitch = p->src.pix_width;
	predMB->height = p->src.pix_height;
        //ALOGD("w=%d, h=%d, pitch=%d, h=%d\n",
        //        predMB->mb_width , predMB->mb_height, predMB->pitch, predMB->height);
	//currMB->CBP = 0;
	//predMB->lambda_mode = QP2QUANT[AML_MAX(0, (int)(p->quant-SHIFT_QP))];
	//predMB->lambda_motion = LAMBDA_FACTOR(predMB->lambda_mode);
	totalMB = predMB->mb_width * predMB->mb_height;

	//intramode->predMB = (amvenc_pred_mode_t*) malloc(sizeof(amvenc_pred_mode_t));
	//predMB->mbNum  = 0;
	predMB->mblock = (AMLMacroblock *)calloc(totalMB,sizeof(AMLMacroblock));
	//predMB->currMB = predMB->mblock;
	predMB->min_cost = (int *)calloc(totalMB,sizeof(int));
	cost = predMB->min_cost;

	if (NULL == predMB->mblock) {
		ALOGE("%s failed\n", __func__);
		return -1;
	}
	return 0;
}

int CleanMBIntraSearchModule_m8( m8venc_drv_t *p)
{
	amvenc_pred_mode_t *predMB = (amvenc_pred_mode_t *) (&p->intra_mode);

	if (predMB->mblock) {
		free( predMB->mblock);
		predMB->mblock = NULL;
	}

	if (predMB->min_cost) {
		free( predMB->min_cost);
		predMB->min_cost = NULL;
	}
	return 0;
}

#endif
