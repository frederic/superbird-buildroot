#define LOG_TAG "M8VENC_RC"
#include <utils/Log.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include "enc_define.h"
#include "rate_control_m8.h"
#include <cutils/properties.h>
#include "m8venclib.h"
#include <sys/ioctl.h>

#define MAX_SKIP_COUNT_PER_SECOND 6
#define SKIP_INTERVAL	4
#define FORCE_IDR_PERIOD 5
#define IDR_SCALER_RATIO 4
//#define ENABLE_SKIP

//#define ENABLE_ESTIMATE_FPS

static void BitrateScale(M8VencRateControl *rateCtrl)
{
    int bitsPerframe = (int)(rateCtrl->bitRate / rateCtrl->frame_rate);
    float ratio = 1.0;
    if (rateCtrl->BitrateScale == false)
        return;

    if (rateCtrl->bitRate >= 5000000)
        return;

    if (rateCtrl->frame_rate < 10.0)
        ratio = 1.3;
    else if (rateCtrl->frame_rate < 15.0)
        ratio = 1.2;
    else if (rateCtrl->frame_rate < 20.0)
        ratio = 1.1;

    if (bitsPerframe < 80000) {
        bitsPerframe = bitsPerframe*11/10;
        bitsPerframe = (int)80000*ratio;
    } else if (bitsPerframe < 100000) {
        bitsPerframe = bitsPerframe*16/10;
        bitsPerframe = (int)bitsPerframe*ratio;
    } else if (bitsPerframe < 150000) {
        bitsPerframe = bitsPerframe*12/10;
        bitsPerframe = (int)bitsPerframe*ratio;
    }

    if (bitsPerframe >= 180000)
        bitsPerframe = 180000;

    rateCtrl->bitRate = (int)(bitsPerframe * rateCtrl->frame_rate);
    return;
}
static void updateRateControl(M8VencRateControl *rateCtrl, bool IDR)
{
    rateCtrl->skip_next_frame = 0;
    rateCtrl->skip_interval--;
    rateCtrl->bits_per_second -= rateCtrl->Rc;
    rateCtrl->encoded_frames++;
    rateCtrl->frame_position++;
#ifdef ENABLE_SKIP
    if (!IDR) {
        //if(rateCtrl->buffer_fullness > threshold && rateCtrl->Rc > 1.5 * rateCtrl->target) {
        if (rateCtrl->Rc > 1.2 * rateCtrl->target || rateCtrl->Rc > 1.0 * rateCtrl->last_IDR_bits) {
            if (rateCtrl->skip_interval < 0 && rateCtrl->skip_cnt_per_second <= MAX_SKIP_COUNT_PER_SECOND) {
                rateCtrl->skip_next_frame = -1;
                rateCtrl->skip_cnt ++;
                rateCtrl->skip_cnt_per_second ++;
                rateCtrl->skip_interval = SKIP_INTERVAL;
                ALOGV("skip current frame ratio:%f",(double)rateCtrl->skip_cnt / rateCtrl->encoded_frames);
                rateCtrl->buffer_fullness -= rateCtrl->Rc;
                rateCtrl->bits_per_second += rateCtrl->Rc;
                return;
            }
        }
    }
#endif

    if (IDR)
        rateCtrl->last_IDR_bits = rateCtrl->Rc;
    return;
}

AMVEnc_Status M8RCUpdateFrame(void *dev, void *rc, bool IDR, int* skip_num, int numFrameBits)
{
    m8venc_drv_t *p = (m8venc_drv_t *)dev;
    M8VencRateControl *rateCtrl = (M8VencRateControl *)rc;
    AMVEnc_Status status = AMVENC_SUCCESS;

    *skip_num = 0;
    if (rateCtrl->rcEnable == true) {
        rateCtrl->Rc = numFrameBits;
        if (IDR == false) {
            rateCtrl->p_frame_cnt++;
            if ((numFrameBits > 2 * rateCtrl->bitsPerFrame) && (numFrameBits > rateCtrl->last_IDR_bits * 4 / 5) && (rateCtrl->p_frame_cnt > FORCE_IDR_PERIOD)) {
               //ALOGI("re-encode, cur framebit:%d, bitPerFrame:%d",numFrameBits , rateCtrl->bitsPerFrame);
               updateRateControl(rateCtrl, IDR);
               status = AMVENC_FORCE_IDR_NEXT;
               return status;
            }
        } else {
            rateCtrl->p_frame_cnt = 0;
        }

        updateRateControl(rateCtrl, IDR);
        if (rateCtrl->skip_next_frame == -1) { // skip current frame
            status = AMVENC_SKIPPED_PICTURE;
            *skip_num = rateCtrl->skip_next_frame;
        }
    }
    return status;
}

AMVEnc_Status M8RCInitFrameQP(void *dev, void *rc,bool IDR,int bitrate, float frame_rate)
{
    m8venc_drv_t *p = (m8venc_drv_t *)dev;
    M8VencRateControl *rateCtrl = (M8VencRateControl *)rc;
    int fps = (int)(rateCtrl->estimate_fps + 0.5);
    rateCtrl->bitsPerFrame = (int)(rateCtrl->bitRate / rateCtrl->estimate_fps);

    if (rateCtrl->frame_rate != frame_rate || rateCtrl->bitRate != bitrate) {
        rateCtrl->refresh = true;
        rateCtrl->frame_rate = frame_rate;
        rateCtrl->bitRate = bitrate;
        BitrateScale(rateCtrl);
        ALOGD("we got new config, frame_rate:%f, bitrate:%d (%d)",frame_rate, bitrate, rateCtrl->bitRate);
    }else{
        rateCtrl->refresh = false;
    }

    if (rateCtrl->rcEnable == true) {
        /* frame layer rate control */
        if (rateCtrl->encoded_frames == 0) {
            p->quant = rateCtrl->Qc = rateCtrl->initQP;
            rateCtrl->target = rateCtrl->bitsPerFrame*IDR_SCALER_RATIO;
            p->target = rateCtrl->target;
            rateCtrl->refresh = false;
        }else{
            if (rateCtrl->frame_position >= (fps+1) || rateCtrl->refresh || IDR || rateCtrl->force_IDR) {
                int old_bitsPerFrame = rateCtrl->bitsPerFrame;
                rateCtrl->bits_per_second = rateCtrl->bitRate;
                rateCtrl->estimate_fps = rateCtrl->frame_rate;
                ALOGV("bitrate:%d,fps:%f",rateCtrl->bits_per_second,rateCtrl->estimate_fps);
                rateCtrl->bitsPerFrame = (int)(rateCtrl->bitRate / rateCtrl->estimate_fps);
                rateCtrl->target = rateCtrl->bitsPerFrame;
                if (rateCtrl->refresh) {
                    float idr_ratio = 0.0;
                    idr_ratio = (float)((float)rateCtrl->bitsPerFrame/(float)old_bitsPerFrame);
                    rateCtrl->last_IDR_bits = (int)(rateCtrl->last_IDR_bits*idr_ratio);
                }
                rateCtrl->skip_cnt_per_second = 0;
                rateCtrl->frame_position = 0;
            }else {
                float scale_ratio = 1.0;
                if ((rateCtrl->frame_position == 1) || ((int)(rateCtrl->estimate_fps+1 - rateCtrl->frame_position) == 2))
                    scale_ratio = 1.2;
                if (rateCtrl->bits_per_second > 0) {
                    rateCtrl->target = (int)((float)(rateCtrl->bits_per_second / (rateCtrl->estimate_fps+1 - rateCtrl->frame_position)*scale_ratio));
                }else{
                    rateCtrl->target = rateCtrl->bitsPerFrame*2/3;
                }
            }
            if (IDR) {
                rateCtrl->target *= IDR_SCALER_RATIO;
            }
            ALOGV("rateCtrl->bits_per_second, %d,rateCtrl->estimate_fps:%f, rateCtrl->frame_position:%d, rateCtrl->target:%d, rateCtrl->bitsPerFrame:%d",
                rateCtrl->bits_per_second, rateCtrl->estimate_fps,rateCtrl->frame_position,rateCtrl->target,rateCtrl->bitsPerFrame);
            p->quant = rateCtrl->Qc;
            p->target = rateCtrl->target;
        }
    }else {// rcEnable
        p->quant = rateCtrl->initQP;
    }
    return AMVENC_SUCCESS;
}

AMVEnc_Status M8RCUpdateBuffer(void *dev, void *rc, int timecode, bool force_IDR)
{
    m8venc_drv_t *p = (m8venc_drv_t *)dev;
    M8VencRateControl *rateCtrl = (M8VencRateControl *)rc;
    rateCtrl->last_timecode = rateCtrl->timecode;
    rateCtrl->timecode = timecode;
    rateCtrl->force_IDR = force_IDR;
#ifdef ENABLE_ESTIMATE_FPS
    /**estimate fps ***/
    int period = rateCtrl->timecode - rateCtrl->last_timecode;
    if (period > 0)
    {
        rateCtrl->estimate_fps = (float) 1000 / period;
    }
    rateCtrl->last_timecode = timecode;
    ALOGV("estimate fps:%f",rateCtrl->estimate_fps);
#else
    //rateCtrl->estimate_fps = rateCtrl->frame_rate;
#endif
    return AMVENC_SUCCESS;
}

void M8CleanupRateControlModule(void *rc)
{
    M8VencRateControl *rateCtrl = (M8VencRateControl *)rc;
    free(rateCtrl);
    return;
}

void* M8InitRateControlModule(amvenc_initpara_t* init_para)
{
    M8VencRateControl *rateCtrl = NULL;

    if (!init_para)
        return NULL;

    rateCtrl = (M8VencRateControl*)calloc(1, sizeof(M8VencRateControl));
    if (!rateCtrl)
        return NULL;

    memset(rateCtrl,0,sizeof(M8VencRateControl));

    rateCtrl->rcEnable = init_para->rcEnable;
    rateCtrl->initQP = init_para->initQP;
    rateCtrl->frame_rate = init_para->frame_rate;
    rateCtrl->bitRate = init_para->bitrate;
    rateCtrl->BitrateScale = init_para->bitrate_scale;
    BitrateScale(rateCtrl);

    if (rateCtrl->rcEnable == true) {
        rateCtrl->bitsPerFrame = (int32)(rateCtrl->bitRate / rateCtrl->frame_rate);
        rateCtrl->skip_next_frame = 0; /* must be initialized */
        rateCtrl->encoded_frames = 0;
        rateCtrl->Qc = rateCtrl->initQP;
        rateCtrl->buffer_fullness = 0;
        rateCtrl->frame_position = 0;
        rateCtrl->skip_cnt = 0;
        rateCtrl->skip_cnt_per_second = 0;
        rateCtrl->skip_interval = 0;
        rateCtrl->target = rateCtrl->bitsPerFrame;
        rateCtrl->bits_per_second = rateCtrl->bitRate * 1.0;
        rateCtrl->estimate_fps = (float)rateCtrl->frame_rate;
        rateCtrl->timecode = 0;
        rateCtrl->last_timecode = 0;
        rateCtrl->refresh = false;
        rateCtrl->force_IDR = false;
        rateCtrl->p_frame_cnt = 0;
        rateCtrl->Rc = 0;
    }
    return (void*)rateCtrl;
CLEANUP_RC:
    M8CleanupRateControlModule((void*)rateCtrl);
    return NULL;
}

