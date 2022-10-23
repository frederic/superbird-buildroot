#ifndef AMLOGIC_M8_RATECONTROL_
#define AMLOGIC_M8_RATECONTROL_

#include "m8venclib.h"

typedef struct M8VencRateControl_s
{
    bool rcEnable;  /* enable rate control, '1' on, '0' const QP */
    int initQP; /* initial QP */
    int bitRate;  /* target bit rate for the overall clip in bits/second*/
    float frame_rate; /* frame rate */
    int skip_next_frame;

    /* this part comes from MPEG4 rate control */
    int Rc;     /*bits used for the current frame. It is the bit count obtained after encoding. */

    /*If the macroblock is intra coded, the original spatial pixel values are summed.*/
    int Qc;     /*quantization level used for the current frame. */
    int bitsPerFrame;

    /* BX rate control, something like TMN8 rate control*/
    int encoded_frames; /* counter for all encoded frames */
    /* End BX */
    int last_IDR_bits;
    unsigned long buffer_fullness;
    int frame_position;
    int target;
    int skip_cnt;
    int skip_cnt_per_second;
    int skip_interval;
    int bits_per_second;
    bool pre_encode;
    int last_timecode;
    int timecode;
    float estimate_fps;
    bool refresh;
    bool force_IDR;
    int p_frame_cnt;
    bool BitrateScale;
} M8VencRateControl;

extern AMVEnc_Status M8RCUpdateFrame(void *dev, void *rc, bool IDR, int* skip_num, int numFrameBits);
extern AMVEnc_Status M8RCInitFrameQP(void *dev, void *rc, bool IDR, int bitrate, float frame_rate);
extern AMVEnc_Status M8RCUpdateBuffer(void *dev, void *rc, int timecode, bool force_IDR);
extern void M8CleanupRateControlModule(void *rc);
extern void* M8InitRateControlModule(amvenc_initpara_t* init_para);

#endif
