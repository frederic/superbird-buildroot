#ifndef AMLOGIC_ENCODER_DEFINE_
#define AMLOGIC_ENCODER_DEFINE_

#include <stdint.h>
#include <malloc.h>
#include <string.h>

#define AVC_ABS(x)   (((x)<0)? -(x) : (x))
#define AVC_MAX(x,y) ((x)>(y)? (x):(y))
#define AVC_MIN(x,y) ((x)<(y)? (x):(y))
#define AVC_MEDIAN(A,B,C) ((A) > (B) ? ((A) < (C) ? (A) : (B) > (C) ? (B) : (C)): (B) < (C) ? (B) : (C) > (A) ? (C) : (A))

#ifdef MAKEANDROID
	#define LOGAPI ALOGE
#else
#if defined(DEBUG)
	#define LOGAPI(fmt,...) printf(fmt "\n", ##__VA_ARGS__)
#else
	#define LOGAPI(fmt,...)
#endif
#endif
//---------------------------------------------------
// ENCODER_STATUS define
//---------------------------------------------------
#define ENCODER_IDLE              0
#define ENCODER_SEQUENCE          1
#define ENCODER_PICTURE           2
#define ENCODER_IDR               3
#define ENCODER_NON_IDR           4
#define ENCODER_MB_HEADER         5
#define ENCODER_MB_DATA           6

#define ENCODER_SEQUENCE_DONE          7
#define ENCODER_PICTURE_DONE           8
#define ENCODER_IDR_DONE               9
#define ENCODER_NON_IDR_DONE           10
#define ENCODER_MB_HEADER_DONE         11
#define ENCODER_MB_DATA_DONE           12

/* defines for H.264 IntraPredMode */
// 4x4 intra prediction modes
#define HENC_VERT_PRED              0
#define HENC_HOR_PRED               1
#define HENC_DC_PRED                2
#define HENC_DIAG_DOWN_LEFT_PRED    3
#define HENC_DIAG_DOWN_RIGHT_PRED   4
#define HENC_VERT_RIGHT_PRED        5
#define HENC_HOR_DOWN_PRED          6
#define HENC_VERT_LEFT_PRED         7
#define HENC_HOR_UP_PRED            8

// 16x16 intra prediction modes
#define HENC_VERT_PRED_16   0
#define HENC_HOR_PRED_16    1
#define HENC_DC_PRED_16     2
#define HENC_PLANE_16       3

// 8x8 chroma intra prediction modes
#define HENC_DC_PRED_8     0
#define HENC_HOR_PRED_8    1
#define HENC_VERT_PRED_8   2
#define HENC_PLANE_8       3

/********************************************
 * defines for H.264 mb_type
 ********************************************/
#define HENC_MB_Type_PBSKIP                      0x0
#define HENC_MB_Type_PSKIP                       0x0
#define HENC_MB_Type_BSKIP_DIRECT                0x0
#define HENC_MB_Type_P16x16                      0x1
#define HENC_MB_Type_P16x8                       0x2
#define HENC_MB_Type_P8x16                       0x3
#define HENC_MB_Type_SMB8x8                      0x4
#define HENC_MB_Type_SMB8x4                      0x5
#define HENC_MB_Type_SMB4x8                      0x6
#define HENC_MB_Type_SMB4x4                      0x7
#define HENC_MB_Type_P8x8                        0x8
#define HENC_MB_Type_I4MB                        0x9
#define HENC_MB_Type_I16MB                       0xa
#define HENC_MB_Type_IBLOCK                      0xb
#define HENC_MB_Type_SI4MB                       0xc
#define HENC_MB_Type_I8MB                        0xd
#define HENC_MB_Type_IPCM                        0xe
#define HENC_MB_Type_AUTO                        0xf

#define HENC_MB_Type_P16MB_HORIZ_ONLY            0x7f
#define HENC_MB_CBP_AUTO                         0xff
#define HENC_SKIP_RUN_AUTO                     0xffff

#define ENCODER_BUFFER_INPUT               0
#define ENCODER_BUFFER_REF0                1
#define ENCODER_BUFFER_REF1                2
#define ENCODER_BUFFER_OUTPUT              3
#define ENCODER_BUFFER_INTER_INFO          4
#define ENCODER_BUFFER_INTRA_INFO          5
#define ENCODER_BUFFER_QP                  6
#define ENCODER_BUFFER_DUMP                7
#define ENCODER_BUFFER_CBR                 8

#define I_FRAME   2

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;
typedef int64_t int64;
typedef uint64_t uint64;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef enum
{
    NO_DEFINE = -1,
    M8 = 0,
    MAX_DEV = 1,
} ENC_DEV_TYPE;

typedef enum
{
    VMALLOC_BUFFER = 0,
    CANVAS_BUFFER = 1,
    PHYSICAL_BUFF = 2,
    MAX_TYPE = 3,
} AMVEncBufferType;

typedef enum
{
    AMVENC_YUV422_SINGLE = 0,
    AMVENC_YUV444_SINGLE,
    AMVENC_NV21,
    AMVENC_NV12,
    AMVENC_YUV420,
    AMVENC_YUV444_PLANE,
    AMVENC_RGB888,
    AMVENC_RGB888_PLANE,
    AMVENC_RGB565,
    AMVENC_RGBA8888,
    AMVENC_YUV422_12BIT,
    AMVENC_YUV444_10BIT,
    AMVENC_YUV422_10BIT,
    AMVENC_BGR888,
    AMVENC_FRAME_FMT
} AMVEncFrameFmt;

typedef enum
{
    /**
    Fail information, need to add more error code for more specific info
    */
    AMVENC_TIMEOUT = -36,
    AMVENC_TRAILINGONES_FAIL = -35,
    AMVENC_SLICE_EMPTY = -34,
    AMVENC_POC_FAIL = -33,
    AMVENC_CONSECUTIVE_NONREF = -32,
    AMVENC_CABAC_FAIL = -31,
    AMVENC_PRED_WEIGHT_TAB_FAIL = -30,
    AMVENC_DEC_REF_PIC_MARK_FAIL = -29,
    AMVENC_SPS_FAIL = -28,
    AMVENC_BITSTREAM_BUFFER_FULL    = -27,
    AMVENC_BITSTREAM_INIT_FAIL = -26,
    AMVENC_CHROMA_QP_FAIL = -25,
    AMVENC_INIT_QS_FAIL = -24,
    AMVENC_INIT_QP_FAIL = -23,
    AMVENC_WEIGHTED_BIPRED_FAIL = -22,
    AMVENC_INVALID_INTRA_PERIOD = -21,
    AMVENC_INVALID_CHANGE_RATE = -20,
    AMVENC_INVALID_BETA_OFFSET = -19,
    AMVENC_INVALID_ALPHA_OFFSET = -18,
    AMVENC_INVALID_DEBLOCK_IDC = -17,
    AMVENC_INVALID_REDUNDANT_PIC = -16,
    AMVENC_INVALID_FRAMERATE = -15,
    AMVENC_INVALID_NUM_SLICEGROUP = -14,
    AMVENC_INVALID_POC_LSB = -13,
    AMVENC_INVALID_NUM_REF = -12,
    AMVENC_INVALID_FMO_TYPE = -11,
    AMVENC_ENCPARAM_MEM_FAIL = -10,
    AMVENC_LEVEL_NOT_SUPPORTED = -9,
    AMVENC_LEVEL_FAIL = -8,
    AMVENC_PROFILE_NOT_SUPPORTED = -7,
    AMVENC_TOOLS_NOT_SUPPORTED = -6,
    AMVENC_WRONG_STATE = -5,
    AMVENC_UNINITIALIZED = -4,
    AMVENC_ALREADY_INITIALIZED = -3,
    AMVENC_NOT_SUPPORTED = -2,
    AMVENC_MEMORY_FAIL = -1,
    AMVENC_FAIL = 0,
    /**
    Generic success value
    */
    AMVENC_SUCCESS = 1,
    AMVENC_PICTURE_READY = 2,
    AMVENC_NEW_IDR = 3, /* upon getting this, users have to call PVAVCEncodeSPS and PVAVCEncodePPS to get a new SPS and PPS*/
    AMVENC_SKIPPED_PICTURE = 4, /* continuable error message */
    AMVENC_REENCODE_PICTURE = 5, /* reencode the picutre */
    AMVENC_FORCE_IDR_NEXT = 6, /* force IDR next picutre */
} AMVEnc_Status;

typedef enum
{
    AMVEnc_Initializing = 0,
    AMVEnc_Encoding_SPS,
    AMVEnc_Encoding_PPS,
    AMVEnc_Analyzing_Frame,
    AMVEnc_WaitingForBuffer,  // pending state
    AMVEnc_Encoding_Frame,
} AMVEnc_State;

typedef enum
{
    AVC_BASELINE = 66,
    AVC_MAIN = 77,
    AVC_EXTENDED = 88,
    AVC_HIGH = 100,
    AVC_HIGH10 = 110,
    AVC_HIGH422 = 122,
    AVC_HIGH444 = 144
} AVCProfile;

typedef enum
{
    AVC_LEVEL_AUTO = 0,
    AVC_LEVEL1_B = 9,
    AVC_LEVEL1 = 10,
    AVC_LEVEL1_1 = 11,
    AVC_LEVEL1_2 = 12,
    AVC_LEVEL1_3 = 13,
    AVC_LEVEL2 = 20,
    AVC_LEVEL2_1 = 21,
    AVC_LEVEL2_2 = 22,
    AVC_LEVEL3 = 30,
    AVC_LEVEL3_1 = 31,
    AVC_LEVEL3_2 = 32,
    AVC_LEVEL4 = 40,
    AVC_LEVEL4_1 = 41,
    AVC_LEVEL4_2 = 42,
    AVC_LEVEL5 = 50,
    AVC_LEVEL5_1 = 51
} AVCLevel;

typedef enum
{
    AVC_NALTYPE_SLICE = 1,  /* non-IDR non-data partition */
    AVC_NALTYPE_DPA = 2,    /* data partition A */
    AVC_NALTYPE_DPB = 3,    /* data partition B */
    AVC_NALTYPE_DPC = 4,    /* data partition C */
    AVC_NALTYPE_IDR = 5,    /* IDR NAL */
    AVC_NALTYPE_SEI = 6,    /* supplemental enhancement info */
    AVC_NALTYPE_SPS = 7,    /* sequence parameter set */
    AVC_NALTYPE_PPS = 8,    /* picture parameter set */
    AVC_NALTYPE_AUD = 9,    /* access unit delimiter */
    AVC_NALTYPE_EOSEQ = 10, /* end of sequence */
    AVC_NALTYPE_EOSTREAM = 11, /* end of stream */
    AVC_NALTYPE_FILL = 12, /* filler data */
    AVC_NALTYPE_EMPTY = 0xff /* Empty Nal */
} AVCNalUnitType;

typedef enum
{
    AVC_P_SLICE = 0,
    AVC_B_SLICE = 1,
    AVC_I_SLICE = 2,
    AVC_SP_SLICE = 3,
    AVC_SI_SLICE = 4,
    AVC_P_ALL_SLICE = 5,
    AVC_B_ALL_SLICE = 6,
    AVC_I_ALL_SLICE = 7,
    AVC_SP_ALL_SLICE = 8,
    AVC_SI_ALL_SLICE = 9
} AVCSliceType;

typedef enum
{
    AVC_OFF = 0,
    AVC_ON = 1
} AVCFlag;

#define AMVEncFrameIO_NONE_FLAG 0x00000000
#define AMVEncFrameIO_FORCE_IDR_FLAG 0x00000001
#define AMVEncFrameIO_FORCE_SKIP_FLAG 0x00000002

#define AMVENC_FLUSH_FLAG_INPUT             0x1
#define AMVENC_FLUSH_FLAG_OUTPUT        0x2
#define AMVENC_FLUSH_FLAG_REFERENCE         0x4
#define AMVENC_FLUSH_FLAG_INTRA_INFO    0x8
#define AMVENC_FLUSH_FLAG_INTER_INFO    0x10
#define AMVENC_FLUSH_FLAG_QP            0x20
#define AMVENC_FLUSH_FLAG_DUMP		0x40
#define AMVENC_FLUSH_FLAG_CBR		0x80

#endif
