#ifndef AMLOGIC_GX_FAST_RATECONTROL_
#define AMLOGIC_GX_FAST_RATECONTROL_

#include "gxvenclib_fast.h"

typedef struct GxFastEncRateControl_s {
    bool rcEnable; /* enable rate control, '1' on, '0' const QP */
    int initQP; /* initial QP */
    int bitRate; /* target bit rate for the overall clip in bits/second*/
    float frame_rate; /* frame rate */
    int skip_next_frame;

    int32 cpbSize; /* coded picture buffer size in bytes */

    /* this part comes from MPEG4 rate control */
    int Rc; /*bits used for the current frame. It is the bit count obtained after encoding. */

    /*If the macroblock is intra coded, the original spatial pixel values are summed.*/
    int Qc; /*quantization level used for the current frame. */

    //int numFrameBits; /* keep track of number of bits of the current frame */
    int bitsPerFrame;

    int encoded_frames; /* counter for all encoded frames */

    int last_IDR_bits;
    int last_pframe_bits;
    int64_t buffer_fullness;

    int target;
    int last_timecode;
    int timecode;
    float estimate_fps;

    bool refresh;
    bool force_IDR;
    bool BitrateScale;
} GxFastEncRateControl;

extern AMVEnc_Status GxFastRCUpdateFrame(void *dev, void *rc, bool IDR, int* skip_num, int numFrameBits);
extern AMVEnc_Status GxFastRCInitFrameQP(void *dev, void *rc, bool IDR, int bitrate, float frame_rate);
extern AMVEnc_Status GxFastRCUpdateBuffer(void *dev, void *rc, int timecode, bool force_IDR);
extern void GxFastCleanupRateControlModule(void *rc);
extern void* GxFastInitRateControlModule(amvenc_initpara_t* init_para);

#endif
