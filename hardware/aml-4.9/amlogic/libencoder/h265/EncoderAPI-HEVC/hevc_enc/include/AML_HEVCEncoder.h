#ifndef AMLOGIC_HEVCENCODER_
#define AMLOGIC_HEVCENCODER_

#include "enc_define.h"
#include "vdi.h"

typedef struct amvhevcenc_initpara_s {
    uint32 enc_width;
    uint32 enc_height;
    uint32 nSliceHeaderSpacing;
    uint32 MBsIntraRefresh;
    uint32 MBsIntraOverlap;
    int initQP;
    bool rcEnable;
    uint32 bitrate;
    uint32 frame_rate;
    uint32 cpbSize;
    bool bitrate_scale;
    uint8 encode_once;
} amvhevcenc_initpara_t;

typedef struct amvhevcenc_hw_s {
    ENC_DEV_TYPE dev_id;
    int dev_fd;
    void* rc_data;
    void* dev_data;
    amvhevcenc_initpara_t init_para;
} amvhevcenc_hw_t;

typedef struct FrameIOhevc_s {
    ulong YCbCr[3];
    AMVEncBufferType type;
    AMVEncFrameFmt fmt;
    int pitch;
    int height;
    uint32 disp_order;
    uint is_reference;
    uint32 coding_timestamp;
    uint32 op_flag;
    uint32 canvas;
    uint32 bitrate;
    float frame_rate;
    uint32 scale_width;
    uint32 scale_height;
    uint32 crop_left;
    uint32 crop_right;
    uint32 crop_top;
    uint32 crop_bottom;
} AMVHEVCEncFrameIO;

typedef struct HEVCEncParams_s {
    HEVCProfile profile; /* profile of the bitstream to be compliant with*/
    HEVCLevel level; /* level of the bitstream to be compliant with*/
    HEVCTier tier; /* tier of the bitstream to be compliant with*/

    int width; /* width of an input frame in pixel */
    int height; /* height of an input frame in pixel */

    int num_ref_frame; /* number of reference frame used */
    int num_slice_group; /* number of slice group */

    uint32 nSliceHeaderSpacing;

    HEVCFlag auto_scd; /* scene change detection on or off */
    int idr_period; /* idr frame refresh rate in number of target encoded frame (no concept of actual time).*/

    HEVCFlag fullsearch; /* enable full-pel full-search mode */
    int search_range; /* search range for motion vector in (-search_range,+search_range) pixels */
    //HEVCFlag sub_pel;    /* enable sub pel prediction */
    //HEVCFlag submb_pred; /* enable sub MB partition mode */

    HEVCFlag rate_control; /* rate control enable, on: RC on, off: constant QP */
    int initQP; /* initial QP */
    uint32 bitrate; /* target encoding bit rate in bits/second */
    uint32 CPB_size; /* coded picture buffer in number of bits */
    uint32 init_CBP_removal_delay; /* initial CBP removal delay in msec */

    uint32 frame_rate; /* frame rate in the unit of frames per 1000 second */
    /* note, frame rate is only needed by the rate control, AVC is timestamp agnostic. */

    uint32 MBsIntraRefresh;
    uint32 MBsIntraOverlap;

    HEVCFlag out_of_band_param_set; /* flag to set whether param sets are to be retrieved up front or not */
    HEVCFlag FreeRun;
    HEVCFlag BitrateScale;
    uint32 dev_id; /* ID to identify the hardware encoder version */
    uint8 encode_once; /* flag to indicate encode once or twice */

    uint32 src_width;  /*src buffer width before crop and scale */
    uint32 src_height; /*src buffer height before crop and scale */
    HEVCRefreshType refresh_type; /*refresh type of intra picture*/
} AMVHEVCEncParams;

typedef struct {
    uint32 enc_width;
    uint32 enc_height;
    AMVEnc_State state;
    bool rcEnable; /* rate control enable, on: RC on, off: constant QP */
    int initQP; /* initial QP */
    uint32 bitrate; /* target encoding bit rate in bits/second */
    uint32 cpbSize; /* coded picture buffer in number of bits */
    AVCNalUnitType nal_unit_type;
    AVCSliceType slice_type;

    uint32 modTimeRef; /* Reference modTime update every I-Vop*/
    uint32 wrapModTime; /* Offset to modTime Ref, rarely used */
    uint32 frame_rate; /* frame rate */
    int idrPeriod; /* IDR period in number of frames */
    bool first_frame; /* a flag for the first frame */
    uint prevProcFrameNum; /* previously processed frame number, could be skipped */
    uint prevProcFrameNumOffset;
    uint32 lastTimeRef;
    uint frame_in_gop;
    amvhevcenc_hw_t hw_info;
} hevc_info_t;

typedef struct AMVHEVCEncHandle_s {
    uint32 instance_id;
    uint32 src_idx;
    uint32 src_count;
    uint32 enc_width;
    uint32 enc_height;
    uint32 bitrate; /* target encoding bit rate in bits/second */
    uint32 frame_rate; /* frame rate */
    int32 idrPeriod; /* IDR period in number of frames */
    uint32 op_flag;

    uint32 src_num;
    uint32 fb_num;

    uint32 enc_counter;

    AMVEncFrameFmt fmt;

    uint32 initQP; /* initial QP */
    AVCNalUnitType nal_unit_type;
    bool rcEnable; /* rate control enable, on: RC on, off: constant QP */

    vpu_buffer_t work_vb;
    vpu_buffer_t temp_vb;
    vpu_buffer_t bs_vb;
    vpu_buffer_t src_vb[4];
    vpu_buffer_t fb_vb[4];
    vpu_buffer_t fbc_ltable_vb;
    vpu_buffer_t fbc_ctable_vb;
    vpu_buffer_t fbc_mv_vb;
    vpu_buffer_t subsample_vb;

    AMVHEVCEncParams mEncParams;
    bool mSpsPpsHeaderReceived;
    uint8_t mSPSPPSDataSize;
    uint8_t *mSPSPPSData;
    int32_t mNumInputFrames;
    bool mKeyFrameRequested;
    bool mPrependSPSPPSToIDRFrames;
    uint32 mOutputBufferLen;
    uint32 mGopIdx;
    uint32 mUvSwap;
} AMVHEVCEncHandle;

extern AMVEnc_Status AML_HEVCInitialize(AMVHEVCEncHandle *Handle, AMVHEVCEncParams *encParam, bool* has_mix, int force_mode);
extern AMVEnc_Status AML_HEVCSetInput(AMVHEVCEncHandle *Handle, AMVHEVCEncFrameIO *input);
extern AMVEnc_Status AML_HEVCEncNAL(AMVHEVCEncHandle *Handle, unsigned char *buffer, unsigned int *buf_nal_size, int *nal_type);
extern AMVEnc_Status AML_HEVCEncHeader(AMVHEVCEncHandle *Handle, unsigned char *buffer, unsigned int *buf_nal_size, int *nal_type);
extern AMVEnc_Status AML_HEVCRelease(AMVHEVCEncHandle *Handle);

#endif
