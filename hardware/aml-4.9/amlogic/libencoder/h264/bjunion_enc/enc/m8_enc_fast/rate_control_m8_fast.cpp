//#define LOG_NDEBUG 0
#define LOG_TAG "FASTVENC_RC"
#include <utils/Log.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include "enc_define.h"
#include "m8venclib_fast.h"
#include "rate_control_m8_fast.h"
#include <cutils/properties.h>

#define RC_MAX_QUANT 28
#define RC_MIN_QUANT 1   //cap to 10 to prevent rate fluctuation

#define INDENPENT_QP_INTERVAL  // use this to set indenpent qp interval for P frame
#ifdef INDENPENT_QP_INTERVAL
	#define P_RC_MAX_QUANT 26
	#define P_RC_MIN_QUANT 1
#else
	#define P_RC_MAX_QUANT RC_MAX_QUANT
	#define P_RC_MIN_QUANT RC_MIN_QUANT
#endif

static double QP2Qstep(int QP)
{
    int i;
    double Qstep;
    static const double QP2QSTEP[6] = { 0.625, 0.6875, 0.8125, 0.875, 1.0, 1.125 };

    Qstep = QP2QSTEP[QP % 6];
    for (i = 0; i < (QP / 6); i++)
        Qstep *= 2;

    return Qstep;
}

/* convert from step size to QP */
static int Qstep2QP(double Qstep)
{
    int q_per = 0, q_rem = 0;

    if (Qstep < QP2Qstep(0))
        return 0;
    else if (Qstep > QP2Qstep(51))
        return 51;

    while (Qstep > QP2Qstep(5)) {
        Qstep /= 2;
        q_per += 1;
    }

    if (Qstep <= (0.625 + 0.6875) / 2)
    {
        Qstep = 0.625;
        q_rem = 0;
    }
    else if (Qstep <= (0.6875 + 0.8125) / 2)
    {
        Qstep = 0.6875;
        q_rem = 1;
    }
    else if (Qstep <= (0.8125 + 0.875) / 2)
    {
        Qstep = 0.8125;
        q_rem = 2;
    }
    else if (Qstep <= (0.875 + 1.0) / 2)
    {
        Qstep = 0.875;
        q_rem = 3;
    }
    else if (Qstep <= (1.0 + 1.125) / 2)
    {
        Qstep = 1.0;
        q_rem = 4;
    }
    else
    {
        Qstep = 1.125;
        q_rem = 5;
    }

    return (q_per * 6 + q_rem);
}

static void updateRC_PostProc(FastEncRateControl *rateCtrl, bool IDR)
{
    int calculated_Qp;
    int Qp_diff;
    bool trap = false;
    int pframe_max_qp = rateCtrl->max_p_qp;
    calculated_Qp = Qstep2QP(rateCtrl->actual_Qstep);
    Qp_diff = calculated_Qp - rateCtrl->Qc;
    ALOGV("Calculate qp:%d,rateCtrl->actual_Qstep:%f,diff:%d",calculated_Qp,rateCtrl->actual_Qstep,Qp_diff);
    if (rateCtrl->max_inc_qp_step>0) {
        if (Qp_diff > rateCtrl->max_inc_qp_step) {
            rateCtrl->Qc += rateCtrl->max_inc_qp_step;
            rateCtrl->actual_Qstep = QP2Qstep(rateCtrl->Qc);
            trap = true;
        }
    }
    if (rateCtrl->max_dec_qp_step>0) {
        if (Qp_diff < 0 - rateCtrl->max_dec_qp_step) {
            rateCtrl->Qc -= rateCtrl->max_dec_qp_step;
            rateCtrl->actual_Qstep = QP2Qstep(rateCtrl->Qc);
            trap = true;
        }
    }

    if (trap == false) {
        rateCtrl->Qc += Qp_diff;
    }
    calculated_Qp = rateCtrl->Qc;

    if (IDR) {
        if (calculated_Qp <= RC_MIN_QUANT) {
            rateCtrl->Qc = RC_MIN_QUANT;
            rateCtrl->actual_Qstep = QP2Qstep(RC_MIN_QUANT);
        }
        if (calculated_Qp >= rateCtrl->max_idr_qp) {
            rateCtrl->Qc = rateCtrl->max_idr_qp;
            rateCtrl->actual_Qstep = QP2Qstep(rateCtrl->max_idr_qp);
        }
        rateCtrl->last_IDR_qp_delta = rateCtrl->Qc - rateCtrl->max_p_qp;
    }else{
        if (rateCtrl->last_IDR_qp_delta > 1) {
            pframe_max_qp = rateCtrl->max_p_qp + rateCtrl->last_IDR_qp_delta - 2;
            rateCtrl->last_IDR_qp_delta -= 2;
        }
        if (calculated_Qp <= P_RC_MIN_QUANT) {
            rateCtrl->Qc = P_RC_MIN_QUANT;
            rateCtrl->actual_Qstep = QP2Qstep(RC_MIN_QUANT);
        }
        if (calculated_Qp >= pframe_max_qp) {
            rateCtrl->Qc = pframe_max_qp;
            rateCtrl->actual_Qstep = QP2Qstep(rateCtrl->max_idr_qp); // if I frame qp != P frame qp, temp use I max qp.
        }
    }
    return;
}

static void targetBitCalculation(FastEncRateControl *rateCtrl, bool IDR)
{
      /* some stuff about frame dropping remained here to be done because pMP cannot be inserted into updateRateControl()*/
    updateRC_PostProc(rateCtrl, IDR);
    return;
}

void calculateQuantizer(fast_enc_drv_t *p, FastEncRateControl *rateCtrl,bool IDR)
{
    int median_quant;

    /* Mad based variable bit allocation */
    targetBitCalculation(rateCtrl, IDR);

    median_quant = RC_MIN_QUANT + (rateCtrl->max_idr_qp- RC_MIN_QUANT) / 2;
    if (IDR == true) {
        if (rateCtrl->Qc < median_quant + 5) { //to avoid the qp for idr is too samll
            rateCtrl->Qc += 5;
        }
        rateCtrl->Qc = AVC_MIN(rateCtrl->Qc,rateCtrl->max_idr_qp);
        rateCtrl->actual_quant = (double)rateCtrl->Qc;
    }
    return;
}


static void updateRateControl(FastEncRateControl *rateCtrl, bool IDR)
{
    int encode_frames;
    int64_t threshold;
    double rate,delta,decay;
    double target,current_target;
    int median_quant;
    rateCtrl->encoded_frames++;
    /**scene detact**/
    rateCtrl->reencode = false;
    rateCtrl->next_IDR = false;
    rateCtrl->skip_next_frame = 0;
    median_quant = RC_MIN_QUANT + (rateCtrl->max_idr_qp - RC_MIN_QUANT) / 2;
    ALOGV("rateCtrl->last_IDR_bits:%d,last_pframe_bits:%d,current:%d,QP:%d",rateCtrl->last_IDR_bits,rateCtrl->last_pframe_bits,rateCtrl->Rc,rateCtrl->Qc);
    if (rateCtrl->refresh == true) {
        float idr_ratio = 0.0;
        int old_bitsPerFrame = rateCtrl->bitsPerFrame;
        rateCtrl->bitsPerFrame = (int32)(rateCtrl->bitRate / rateCtrl->frame_rate);
        rateCtrl->T = rateCtrl->bitRate / rateCtrl->frame_rate;
        rateCtrl->average_rate = (double)rateCtrl->bitRate / rateCtrl->frame_rate;
        rateCtrl->reaction_rate = rateCtrl->average_rate;
        idr_ratio = (float)((float)rateCtrl->bitsPerFrame/(float)old_bitsPerFrame);
        rateCtrl->last_IDR_bits = (int)(rateCtrl->last_IDR_bits*idr_ratio);
        rateCtrl->refresh = false;
    }
    if (!IDR && rateCtrl->Rc > 1.2 * rateCtrl->last_IDR_bits) {
        targetBitCalculation(rateCtrl, true);
        if (rateCtrl->Qc < median_quant + 5) { //to avoid the qp for idr is too samll
            rateCtrl->Qc += 5;
        }
        rateCtrl->Qc = AVC_MIN(rateCtrl->Qc,rateCtrl->max_idr_qp);
        rateCtrl->actual_quant = (double)rateCtrl->Qc;
        rateCtrl->reencode = true;
        rateCtrl->reencode_cnt++;
        ALOGV("rateCtrl Reencode:%d,ratio:%f",rateCtrl->reencode_cnt,(double)rateCtrl->reencode_cnt / rateCtrl->encoded_frames);
        return;
    }else{
        rateCtrl->next_IDR = false;
    }

    /** calculate average rate**/
    ALOGV("bitRate:%d,frame_rate:%f",rateCtrl->bitRate,rateCtrl->frame_rate);
    rate = rateCtrl->average_rate;
    delta = rateCtrl->average_delta;
    decay = 1 - delta;
    rate = rate * decay + ((double)rateCtrl->Rc) * delta;
    rateCtrl->average_rate = rate;

    /**calculate current target bits**/
    target = (double)rateCtrl->bitsPerFrame;
    if (rate > target) {
        current_target = target - (rate - target);
        if (current_target < target * 0.75) {
            current_target = target * 0.75;
        }
    } else {
        current_target = target;
    }

    if (IDR) {
        rateCtrl->last_IDR_bits = rateCtrl->Rc;
        rateCtrl->last_pframe_bits = 0x7fffffff;
    } else {
        rateCtrl->last_pframe_bits = rateCtrl->Rc;
    }
    encode_frames = rateCtrl->encoded_frames - rateCtrl->reencode_cnt;
    rateCtrl->buffer_fullness += rateCtrl->Rc - rateCtrl->bitsPerFrame;
    threshold = rateCtrl->bitsPerFrame * encode_frames * 0.05;
    ALOGV("frame:%d :rateCtrl->bitsPerFrame:%d,current_bits:%d",encode_frames,rateCtrl->bitsPerFrame,rateCtrl->Rc);
    ALOGV("bufferer_fullness:%lld,threshold:%lld",rateCtrl->buffer_fullness,threshold);

    /**calculate current reaction rate**/
    rate = rateCtrl->reaction_rate;
    delta = rateCtrl->reaction_delta;
    decay = 1 - delta;
    rate = rate * decay + ((double)rateCtrl->Rc) * delta;
    rateCtrl->reaction_rate = rate;
    median_quant = RC_MIN_QUANT + (rateCtrl->max_idr_qp - RC_MIN_QUANT) / 2;

    ALOGV("rate:%f,current_target:%f",rate,current_target);
    /**reduce quantizer when the reaction rate is low**/
    if (rate < current_target && rateCtrl->buffer_fullness < threshold) {
    //if(rate < current_target){
        rateCtrl->actual_Qstep = rateCtrl->actual_Qstep * (1 - rateCtrl->reaction_delta * ((current_target - rate) / current_target / 0.20));
    }
    /**increase quantizer when the reaction rate is high**/
    if (rate > current_target && rateCtrl->buffer_fullness > 0) {
    //if(rate > current_target){
        /**slower increasement when the quant is higher than median*/
        if (rateCtrl->Qc > median_quant) {
            rateCtrl->actual_Qstep = rateCtrl->actual_Qstep * (1 + rateCtrl->reaction_delta / rateCtrl->reaction_ratio);
        } else if (rate > current_target * 1.20) {
            rateCtrl->actual_Qstep = rateCtrl->actual_Qstep * (1 + rateCtrl->reaction_delta);
        } else {
            rateCtrl->actual_Qstep = rateCtrl->actual_Qstep * (1 + rateCtrl->reaction_delta * ((rate - current_target) / current_target / 0.20));
        }
    }

    return;
}

AMVEnc_Status FastRCUpdateFrame(void *dev, void *rc, bool IDR, int* skip_num, int numFrameBits)
{
    fast_enc_drv_t *p = (fast_enc_drv_t *)dev;
    FastEncRateControl *rateCtrl = (FastEncRateControl *)rc;
    AMVEnc_Status status = AMVENC_SUCCESS;
    int diff_BTCounter;

    *skip_num = 0;

    if (rateCtrl->rcEnable == true) {

        //rateCtrl->Rc = rateCtrl->numFrameBits;  /* Total Bits for current frame */
        rateCtrl->Rc = numFrameBits;

        updateRateControl(rateCtrl, IDR);
        if (rateCtrl->skip_next_frame == -1) { // skip current frame
            status = AMVENC_SKIPPED_PICTURE;
            *skip_num = rateCtrl->skip_next_frame;
        } else if (rateCtrl->reencode == true) {
            p->reencode = true;
            p->quant = rateCtrl->Qc;
            status = AMVENC_REENCODE_PICTURE;
        }
    }
    ALOGV("Qp:%d",p->quant);
    return status;
}

AMVEnc_Status FastRCInitFrameQP(void *dev, void *rc,bool IDR,int bitrate, float frame_rate)
{
    fast_enc_drv_t *p = (fast_enc_drv_t *)dev;
    FastEncRateControl *rateCtrl = (FastEncRateControl *)rc;
    if (rateCtrl->rcEnable == true) {
        /* frame layer rate control */
        if (rateCtrl->encoded_frames == 0) {
            p->quant= rateCtrl->Qc = rateCtrl->initQP;
        } else {
            calculateQuantizer(p,rateCtrl,IDR);
            p->quant = rateCtrl->Qc;
            if (rateCtrl->frame_rate != frame_rate || rateCtrl->bitRate != bitrate || rateCtrl->force_IDR) {
                rateCtrl->refresh = true;
                ALOGD("we got new config, frame_rate:%f, bitrate:%d, force_IDR:%d",frame_rate, bitrate, rateCtrl->force_IDR);
            }else{
                rateCtrl->refresh = false;
            }
            rateCtrl->frame_rate = frame_rate;
            rateCtrl->bitRate = bitrate;
        }
        rateCtrl->numFrameBits = 0; // reset
    }else{// rcEnable
        p->quant = rateCtrl->initQP;
    }
    return AMVENC_SUCCESS;
}

AMVEnc_Status FastRCUpdateBuffer(void *dev, void *rc, int timecode, bool force_IDR)
{
    fast_enc_drv_t *p = (fast_enc_drv_t *)dev;
    FastEncRateControl *rateCtrl = (FastEncRateControl *)rc;
    rateCtrl->force_IDR = force_IDR;
    return AMVENC_SUCCESS;
}

void FastCleanupRateControlModule(void *rc)
{
    FastEncRateControl *rateCtrl = (FastEncRateControl *)rc;
    free(rateCtrl);
    return;
}

void* FastInitRateControlModule(amvenc_initpara_t* init_para)
{
    FastEncRateControl *rateCtrl = NULL;

    if (!init_para)
        return NULL;

    rateCtrl = (FastEncRateControl*)calloc(1, sizeof(FastEncRateControl));
    if (!rateCtrl)
        return NULL;

    memset(rateCtrl,0,sizeof(FastEncRateControl));

    rateCtrl->rcEnable = init_para->rcEnable;
    rateCtrl->initQP = init_para->initQP;
    rateCtrl->frame_rate = (float)init_para->frame_rate;
    rateCtrl->bitRate = init_para->bitrate;
    rateCtrl->cpbSize = init_para->cpbSize;

    char qp_str[PROPERTY_VALUE_MAX];
    int qp = 0;
    memset(qp_str,0,sizeof(qp_str));
    if (property_get("ro.amlenc.maxqp.I", qp_str, NULL) > 0) {
        sscanf(qp_str,"%d",&qp);
        if (qp <= 51)
            rateCtrl->max_idr_qp = qp;
        else
            rateCtrl->max_idr_qp = RC_MAX_QUANT;
    }else{
        rateCtrl->max_idr_qp = RC_MAX_QUANT;
    }
    qp = 0;
    memset(qp_str,0,sizeof(qp_str));
    if (property_get("ro.amlenc.maxqp.P", qp_str, NULL) > 0) {
        sscanf(qp_str,"%d",&qp);
        if (qp <= 51)
            rateCtrl->max_p_qp = qp;
        else
            rateCtrl->max_p_qp = P_RC_MAX_QUANT;
    }else{
        rateCtrl->max_p_qp = P_RC_MAX_QUANT;
    }
    qp = 0;
    memset(qp_str,0,sizeof(qp_str));
    // not limit for increase qp step as default;
    if (property_get("ro.amlenc.qpstep.MaxInc", qp_str, NULL) > 0) {
        sscanf(qp_str,"%d",&qp);
        if ((qp <= 0) || (qp >= 51))
            rateCtrl->max_inc_qp_step = 0;
        else
            rateCtrl->max_inc_qp_step = qp;
    }else{
        rateCtrl->max_inc_qp_step = 0;
    }
    qp = 0;
    memset(qp_str,0,sizeof(qp_str));
    // limit 3 for decrease qp step as default;
    if (property_get("ro.amlenc.qpstep.MaxDec", qp_str, NULL) > 0) {
        sscanf(qp_str,"%d",&qp);
        if ((qp < 0) || (qp >= 51))
            rateCtrl->max_dec_qp_step = 3;
        else
            rateCtrl->max_dec_qp_step = qp;
    }else{
        rateCtrl->max_dec_qp_step = 3;
    }

    ALOGV("Max I frame qp: %d, Max P frame qp, %d, MaxInc qp step:%d, dec step:%d",rateCtrl->max_idr_qp,rateCtrl->max_p_qp,rateCtrl->max_inc_qp_step,rateCtrl->max_dec_qp_step);
    if (rateCtrl->rcEnable == true) {
        rateCtrl->bitsPerFrame = (int32)(rateCtrl->bitRate / rateCtrl->frame_rate);
        rateCtrl->skip_next_frame = 0; /* must be initialized */
        rateCtrl->Bs = rateCtrl->cpbSize;
        rateCtrl->encoded_frames = 0;

        /* Setting the bitrate and framerate */
        rateCtrl->Qc = rateCtrl->initQP;
        rateCtrl->T = rateCtrl->bitRate / rateCtrl->frame_rate;
        rateCtrl->average_rate = (double)rateCtrl->bitRate / rateCtrl->frame_rate;
        rateCtrl->average_delta = 1.0 / 200; // this value can change later
        rateCtrl->reaction_rate = rateCtrl->average_rate;
        rateCtrl->reaction_delta = 1.0 / 5; // this value can change later
        rateCtrl->reaction_ratio = 4;//according to paper,may change later;
        rateCtrl->actual_quant = (double)rateCtrl->Qc;
        rateCtrl->next_IDR = false;
        rateCtrl->reencode = false;
        rateCtrl->reencode_cnt = 0;
        rateCtrl->actual_Qstep = QP2Qstep(rateCtrl->Qc);
        rateCtrl->last_pframe_bits = 0x7fffffff;
        rateCtrl->buffer_fullness = 0;
        rateCtrl->last_IDR_qp_delta = rateCtrl->Qc - rateCtrl->max_p_qp;
        rateCtrl->refresh = false;
        rateCtrl->force_IDR = false;
    }
    return (void*)rateCtrl;

CLEANUP_RC:
    FastCleanupRateControlModule((void*)rateCtrl);
    return NULL;
}

