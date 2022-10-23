#ifndef AMLOGIC_ENCODER_API_
#define AMLOGIC_ENCODER_API_

#include "AML_HWEncoder.h"
#include "enc_define.h"

typedef void* (*AMVencHWFunc_Init)(int fd, amvenc_initpara_t* init_para);
typedef AMVEnc_Status (*AMVencHWFunc_InitFrame)(void *dev, ulong *yuv, AMVEncBufferType type, AMVEncFrameFmt fmt, bool IDRframe);
typedef AMVEnc_Status (*AMVencHWFunc_EncodeSPS_PPS)(void *dev, unsigned char* outptr, int* datalen);
typedef AMVEnc_Status (*AMVencHWFunc_EncodeSlice)(void *dev, unsigned char* outptr, int* datalen);
typedef AMVEnc_Status (*AMVencHWFunc_CommitEncode)(void *dev, bool IDR);
typedef void (*AMVencHWFunc_Release)(void *dev);

typedef struct tagAMVencHWFuncPtr {
    AMVencHWFunc_Init Initialize;
    AMVencHWFunc_InitFrame InitFrame;
    AMVencHWFunc_EncodeSPS_PPS EncodeSPS_PPS;
    AMVencHWFunc_EncodeSlice EncodeSlice;
    AMVencHWFunc_CommitEncode Commit;
    AMVencHWFunc_Release Release;
} AMVencHWFuncPtr;

typedef void* (*AMVencRCFunc_Init)(amvenc_initpara_t* init_para);
typedef AMVEnc_Status (*AMVencRCFunc_PreControl)(void *dev, void *rc, int frameInc, bool force_IDR);
typedef AMVEnc_Status (*AMVencRCFunc_PostControl)(void *dev, void *rc, bool IDR, int* skip_num, int numFrameBits);
typedef AMVEnc_Status (*AMVencFunc_InitFrameQP)(void *dev, void *rc, bool IDR, int bitrate, float frame_rate);
typedef void (*AMVencRCFunc_Release)(void *rc);

typedef struct tagAMVencRCFuncPtr {
    AMVencRCFunc_Init Initialize;
    AMVencRCFunc_PreControl PreControl;
    AMVencRCFunc_PostControl PostControl;
    AMVencFunc_InitFrameQP InitFrameQP;
    AMVencRCFunc_Release Release;
} AMVencRCFuncPtr;

extern AMVEnc_Status AMInitRateControlModule(amvenc_hw_t* hw_info);
extern AMVEnc_Status AMPreRateControl(amvenc_hw_t* hw_info, int frameInc, bool force_IDR);
extern AMVEnc_Status AMPostRateControl(amvenc_hw_t* hw_info, bool IDR, int* skip_num, int numFrameBits);
extern AMVEnc_Status AMRCInitFrameQP(amvenc_hw_t* hw_info, bool IDR, int bitrate, float frame_rate);
extern void AMCleanupRateControlModule(amvenc_hw_t* hw_info);

extern AMVEnc_Status InitAMVEncode(amvenc_hw_t* hw_info, int force_mode);
extern AMVEnc_Status AMVEncodeInitFrame(amvenc_hw_t* hw_info, ulong *yuv, AMVEncBufferType type, AMVEncFrameFmt fmt, bool IDRframe);
extern AMVEnc_Status AMVEncodeSPS_PPS(amvenc_hw_t* hw_info, unsigned char* outptr, int* datalen);
extern AMVEnc_Status AMVEncodeSlice(amvenc_hw_t* hw_info, unsigned char* outptr, int* datalen);
extern AMVEnc_Status AMVEncodeCommit(amvenc_hw_t* hw_info, bool IDR);
extern void UnInitAMVEncode(amvenc_hw_t* hw_info);
#endif
