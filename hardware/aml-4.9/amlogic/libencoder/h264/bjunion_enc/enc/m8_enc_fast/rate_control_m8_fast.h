#ifndef AMLOGIC_M8_FAST_RATECONTROL_
#define AMLOGIC_M8_FAST_RATECONTROL_

#include "m8venclib_fast.h"

typedef struct FastEncRateControl_s
{
    bool rcEnable;  /* enable rate control, '1' on, '0' const QP */
    int initQP; /* initial QP */
    int bitRate;  /* target bit rate for the overall clip in bits/second*/
    float frame_rate; /* frame rate */
    int skip_next_frame;

    int32 cpbSize;  /* coded picture buffer size in bytes */

    /* this part comes from MPEG4 rate control */
    int Rc;     /*bits used for the current frame. It is the bit count obtained after encoding. */

    /*If the macroblock is intra coded, the original spatial pixel values are summed.*/
    int Qc;     /*quantization level used for the current frame. */
    int T;      /*target bit to be used for the current frame.*/
    int Bs; /*buffer size e.g., R/2 */

    int numFrameBits; /* keep track of number of bits of the current frame */
    int bitsPerFrame;

    /* BX rate control, something like TMN8 rate control*/


    int     encoded_frames; /* counter for all encoded frames */
    /* End BX */
    double  average_rate;
    double  average_delta;
    double  reaction_rate;
    double  reaction_delta;
    int     reaction_ratio;
    double  actual_quant;
    double  average_qp;
    bool    next_IDR;
    bool    reencode;
    int		reencode_cnt;
    int 	last_IDR_bits;
    int 	last_pframe_bits;
    double	actual_Qstep;
    int64_t		buffer_fullness;
    int max_idr_qp;
    int max_p_qp;
    int last_IDR_qp_delta;
    int max_inc_qp_step;
    int max_dec_qp_step;

    bool refresh;
    bool force_IDR;
} FastEncRateControl;

extern AMVEnc_Status FastRCUpdateFrame(void *dev, void *rc, bool IDR, int* skip_num, int numFrameBits);
extern AMVEnc_Status FastRCInitFrameQP(void *dev, void *rc,bool IDR,int bitrate, float frame_rate);
extern AMVEnc_Status FastRCUpdateBuffer(void *dev, void *rc, int timecode, bool force_IDR);
extern void FastCleanupRateControlModule(void *rc);
extern void* FastInitRateControlModule(amvenc_initpara_t* init_para);

#endif
