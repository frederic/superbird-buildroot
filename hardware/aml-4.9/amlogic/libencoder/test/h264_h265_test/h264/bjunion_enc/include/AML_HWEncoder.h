#ifndef AMLOGIC_HWENCODER_
#define AMLOGIC_HWENCODER_


#include <queue>
#include "../vpcodec_1_0.h"
#include "./enc_define.h"

typedef struct amvenc_initpara_s {
    uint32 enc_width;
    uint32 enc_height;
    uint32 nSliceHeaderSpacing;
    uint32 MBsIntraRefresh;
    uint32 MBsIntraOverlap;
    int initQP;
    int initQP_P;
    bool rcEnable;
    uint32 bitrate;
    uint32 frame_rate;
    uint32 cpbSize;
    bool bitrate_scale;
    uint8 encode_once;
    bool cbr_hw;
    int qp_mode;
    qp_param_t* qp_param;
    amvenc_frame_stat_t* frame_stat_info;
} amvenc_initpara_t;

typedef struct amvenc_hw_s {
    ENC_DEV_TYPE dev_id;
    int dev_fd;
    void* rc_data;
    void* dev_data;
    amvenc_initpara_t init_para;
} amvenc_hw_t;

typedef struct FrameIO_s {
    ulong YCbCr[3];
    AMVEncBufferType type;
    AMVEncFrameFmt fmt;
    int pitch;
    int height;
    uint32 disp_order;
    uint is_reference;
    uint64_t coding_timestamp;
    uint32 op_flag;
    uint32 canvas;
    uint32 bitrate;
    double frame_rate;
    uint32 scale_width;
    uint32 scale_height;
    uint32 crop_left;
    uint32 crop_right;
    uint32 crop_top;
    uint32 crop_bottom;
    uint32 num_planes;
} AMVEncFrameIO;

typedef struct EncParams_s {
    /* if profile/level is set to zero, encoder will choose the closest one for you */
    AVCProfile profile; /* profile of the bitstream to be compliant with*/
    AVCLevel level; /* level of the bitstream to be compliant with*/

    int width; /* width of an input frame in pixel */
    int height; /* height of an input frame in pixel */

    int num_ref_frame; /* number of reference frame used */
    int num_slice_group; /* number of slice group */

    uint32 nSliceHeaderSpacing;

    AVCFlag auto_scd; /* scene change detection on or off */
    int idr_period; /* idr frame refresh rate in number of target encoded frame (no concept of actual time).*/

    AVCFlag fullsearch; /* enable full-pel full-search mode */
    int search_range; /* search range for motion vector in (-search_range,+search_range) pixels */
    //AVCFlag sub_pel;    /* enable sub pel prediction */
    //AVCFlag submb_pred; /* enable sub MB partition mode */

    AVCFlag rate_control; /* rate control enable, on: RC on, off: constant QP */
    int initQP; /* initial QP */
    int initQP_P;         /* initial QP P frame*/
    uint32 bitrate; /* target encoding bit rate in bits/second */
    uint32 CPB_size; /* coded picture buffer in number of bits */
    uint32 init_CBP_removal_delay; /* initial CBP removal delay in msec */

    uint32 frame_rate; /* frame rate in the unit of frames per 1000 second */
    /* note, frame rate is only needed by the rate control, AVC is timestamp agnostic. */
    uint32 MBsIntraRefresh;
    uint32 MBsIntraOverlap;

    AVCFlag out_of_band_param_set; /* flag to set whether param sets are to be retrieved up front or not */
    AVCFlag FreeRun;
    AVCFlag BitrateScale;
    uint32 dev_id; /* ID to identify the hardware encoder version */
    uint8 encode_once; /* flag to indicate encode once or twice */
    AMVEncFrameFmt fmt;
    int qp_mode;
    qp_param_t* qp_param;
    amvenc_frame_stat_t frame_stat_info;
} AMVEncParams;

typedef struct {
    uint32 enc_width;
    uint32 enc_height;
    AMVEnc_State state;
    AVCFlag outOfBandParamSet;
    AVCFlag fullsearch_enable;
    AVCFlag scdEnable;
    int search_range;
    bool rcEnable; /* rate control enable, on: RC on, off: constant QP */
    int initQP; /* initial QP */
    int initQP_P;           /* initial QP P frame*/
    uint32 bitrate; /* target encoding bit rate in bits/second */
    uint32 cpbSize; /* coded picture buffer in number of bits */
    uint32 initDelayOffset; /* initial CBP removal delay in msec */
    AVCNalUnitType nal_unit_type;
    AVCSliceType slice_type;
    uint32 nSliceHeaderSpacing;
    uint32 MBsIntraRefresh;
    uint32 MBsIntraOverlap;

    uint64 modTimeRef; /* Reference modTime update every I-Vop*/
    uint32 frame_rate; /* frame rate */
    int idrPeriod; /* IDR period in number of frames */
    bool first_frame; /* a flag for the first frame */
    int skip_next_frame;
    int late_frame_count;
    uint prevProcFrameNum; /* previously processed frame number, could be skipped */
    uint prevProcFrameNumOffset;
    uint64 lastTimeRef;
    bool freerun;
    uint frame_in_gop;
    amvenc_hw_t hw_info;
    uint64 secTimeRef; /* update every second*/
    uint prevFrameNum; /* previously processed frame number */
    int32_t keepFrameNumPerSecond; /*no skip frame number per second*/
} amvenc_info_t;

typedef struct AMVEncHandle_s {
    void *object;
    void *userData;
    uint32 debugEnable;

    AMVEncParams mEncParams;
    bool mSpsPpsHeaderReceived;
    uint8_t mSPSPPSDataSize;
    uint8_t *mSPSPPSData;
    int32_t mNumInputFrames;
    bool mKeyFrameRequested;


    //For skip
    bool mEnableVFR;
    uint32 mContinuousSkipCnt;
    uint32 mTotalSizeInSegment;
    uint32 mTotalFrameInSegment;
    uint32 mLastSize;
    uint32 mLastTwoSize;
    uint32 mLastType;
    uint32 mLastTwoType;
    int32_t mMinFramerate;
    int32_t mCurrFramerate; /*no skip frame number per second*/
    uint32 mSkippedFrame;
    int32_t mLastBitrateRatio;
    std::queue<int> frameQueue;

    AMVEncBufferType bufType;

} AMVEncHandle;

extern AMVEnc_Status AML_HWEncInitialize(AMVEncHandle* Handle,
                                         AMVEncParams* encParam,
                                         bool* has_mix,
                                         int force_mode);
extern AMVEnc_Status AML_HWSetInput(AMVEncHandle *Handle, AMVEncFrameIO *input);
extern AMVEnc_Status AML_HWEncNAL(AMVEncHandle* Handle,
                                  unsigned char* buffer,
                                  unsigned int* buf_nal_size,
                                  int* nal_type);
extern AMVEnc_Status AML_HWEncRelease(AMVEncHandle *Handle);

#endif
