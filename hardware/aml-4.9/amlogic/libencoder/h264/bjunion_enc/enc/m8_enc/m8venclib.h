#ifndef AML_VIDEO_ENCODER_M8_
#define AML_VIDEO_ENCODER_M8_

#include <sys/time.h>
#include <utils/threads.h>
#include <semaphore.h>
#include "enc_define.h"
#include "../intra_search/pred.h"
#include "noise_reduction.h"
#include "AML_HWEncoder.h"

#define M8VENC_AVC_IOC_MAGIC  'E'

#define M8VENC_AVC_IOC_GET_ADDR					_IOW(M8VENC_AVC_IOC_MAGIC, 0x00, unsigned int)
#define M8VENC_AVC_IOC_INPUT_UPDATE				_IOW(M8VENC_AVC_IOC_MAGIC, 0x01, unsigned int)
#define M8VENC_AVC_IOC_NEW_CMD					_IOW(M8VENC_AVC_IOC_MAGIC, 0x02, unsigned int)
#define M8VENC_AVC_IOC_GET_STAGE					_IOW(M8VENC_AVC_IOC_MAGIC, 0x03, unsigned int)
#define M8VENC_AVC_IOC_GET_OUTPUT_SIZE			_IOW(M8VENC_AVC_IOC_MAGIC, 0x04, unsigned int)
#define M8VENC_AVC_IOC_CONFIG_INIT 				_IOW(M8VENC_AVC_IOC_MAGIC, 0x05, unsigned int)
#define M8VENC_AVC_IOC_FLUSH_CACHE 				_IOW(M8VENC_AVC_IOC_MAGIC, 0x06, unsigned int)
#define M8VENC_AVC_IOC_FLUSH_DMA 				_IOW(M8VENC_AVC_IOC_MAGIC, 0x07, unsigned int)
#define M8VENC_AVC_IOC_GET_BUFFINFO 				_IOW(M8VENC_AVC_IOC_MAGIC, 0x08, unsigned int)
#define M8VENC_AVC_IOC_SUBMIT_ENCODE_DONE 		_IOW(M8VENC_AVC_IOC_MAGIC, 0x09, unsigned int)
#define M8VENC_AVC_IOC_READ_CANVAS 				_IOW(M8VENC_AVC_IOC_MAGIC, 0x0a, unsigned int)

#define UCODE_MODE_FULL 0
#define UCODE_MODE_SW_MIX 1

#define MB_TYPE_MASK 0xf

typedef struct
{
    unsigned crop_top;
    unsigned crop_bottom;
    unsigned crop_left;
    unsigned crop_right;
    unsigned src_w;
    unsigned src_h;
} crop_info;

typedef struct
{
    int pix_width;
    int pix_height;

    int mb_width;
    int mb_height;
    int mbsize;

    int framesize;
    AMVEncBufferType type;
    AMVEncFrameFmt fmt;
    unsigned plane[3];
    unsigned mmapsize;
    unsigned canvas;
    unsigned char* crop_buffer;
    crop_info crop;
} input_t;

typedef struct{
    unsigned char* addr;
    unsigned size;
}m8venc_buff_t;

typedef struct{
    unsigned char* buff;
    unsigned char* y;
    unsigned char* uv;
    unsigned width;
    unsigned height;
    unsigned pitch;
    unsigned size;
}m8venc_reference_t;

typedef struct cordinate{
    short x;
    short y;
} cordinate_t;

typedef struct mv_info{
    cordinate_t mv[16];
    cordinate_t average_mv;
    cordinate_t average_ref;
    unsigned mv_distance;
    bool all_zero;
    bool multi_mv;
    bool half_mode;
} mv_info_t;

typedef struct me_info{
    short mv_bits;
    unsigned char mb_type;
    short coeff_bits;
    short mb_cost;
    mv_info_t mv_info;
    unsigned sad;
} me_info_t;

typedef struct ie_info{
    short coeff_bits;
    unsigned char i4_pred_mode_l[16];
    unsigned char pred_mode_c;

    unsigned char i16mode;
    int min_cost16;
    unsigned i4mode[2];
    int min_cost4;
    unsigned char imode;
    int SBE;
} ie_info_t;

typedef struct mb_info{
    me_info_t me;
    ie_info_t ie;
    int intra_mode;
    unsigned char qp;
    unsigned mem_offset;
    unsigned bytes;
} mb_info_t;

typedef struct mb_statics{
    unsigned total_coeff_bits;
    unsigned total_bits;
    unsigned mb_max_coeff_bits;
    unsigned mb_min_coeff_bits;
    unsigned average_mv_distance;
    unsigned intra_cnt;
    unsigned last_average_qp;
    unsigned gop_length;
    unsigned gop_average_qp;
    unsigned mb_predict;
} mb_statics_t;

typedef enum
{
    SearchMode_idle = 0,
    SearchMode_I16,
    SearchMode_I4,
    SearchMode_Fill_I,
    SearchMode_Fill_P,
}SearchMode;

typedef struct{
    pthread_t mThread;
    sem_t semdone;
    sem_t semstart;

    int start_mbx;
    int start_mby;
    int end_mbx;
    int end_mby;
    int x_step;
    int y_step;
    unsigned update_lines;
    SearchMode mode;
    int ret;
    amvenc_pred_mode_obj_t *mbObj;//one thread, one object
    unsigned char* ref_mb_buff;
    bool finish;
}m8venc_slot_t;

typedef struct extra_control{
    bool mv_correction;
    bool multi_mv;
    bool half_pixel_mode;
    unsigned i16_searchmode; // 0: all, 1: part, 2: none
    int fix_qp;
    int max_qp;
    int min_qp;
    bool logtime;
    int thread_num;
} extra_control_t;

typedef struct
{
    int fd;
    bool IDRframe;
    bool mCancel;
    bool mStart;

    unsigned enc_width;
    unsigned enc_height;
    unsigned quant;
    unsigned target;
    bool gotSPS;
    unsigned sps_len;
    bool gotPPS;
    unsigned pps_len;

    unsigned rows_per_slice;
    unsigned MBsIntraRefresh;
    unsigned MBsIntraOverlap;
    unsigned NextIntraRefreshStart;
    unsigned NextIntraRefreshEnd;
    unsigned total_encode_frame;
    unsigned total_encode_time;

    input_t src;

    unsigned char ref_id;

    m8venc_buff_t mmap_buff;
    m8venc_buff_t input_buf;
    m8venc_buff_t ref_buf_y[2];
    m8venc_buff_t ref_buf_uv[2];
    m8venc_buff_t output_buf;
    m8venc_buff_t inter_bits_info;
    m8venc_buff_t inter_mv_info;
    m8venc_buff_t intra_bits_info;
    m8venc_buff_t intra_pred_info;
    m8venc_buff_t qp_info;
    m8venc_buff_t scale_buff;

    m8venc_reference_t ref_info;
    mb_info_t *mb_buffer;
    mb_info_t *mb_ref_buffer[2];
    unsigned mb_buffer_id;
    mb_statics_t mb_statics;
    amvenc_pred_mode_t intra_mode;
    m8venc_slot_t slot[3];
    int timeout_value;
    int base_quant;
    PRM_NR Prm_Nr;
    bool nr_enable;
    bool force_skip;
    bool scale_enable;
    bool pre_intra;

    extra_control_t control;

    struct timeval start_test;
    struct timeval end_test;
}m8venc_drv_t;

extern void* InitM8VEncode(int fd, amvenc_initpara_t* init_para);
extern AMVEnc_Status M8VEncodeInitFrame(void *dev, unsigned* yuv, AMVEncBufferType type, AMVEncFrameFmt fmt,bool IDRframe);
extern AMVEnc_Status M8VEncodeSPS_PPS(void *dev, unsigned char* outptr,int* datalen);
extern AMVEnc_Status M8VEncodeSlice(void *dev, unsigned char* outptr,int* datalen);
extern AMVEnc_Status M8VEncodeCommit(void* dev,  bool IDR);
extern void UnInitM8VEncode(void *dev);

const  int QP2QUANT_m8[40] =
{
    1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 4, 4, 4, 5, 6,
    6, 7, 8, 9, 10, 11, 13, 14,
    16, 18, 20, 23, 25, 29, 32, 36,
    40, 45, 51, 57, 64, 72, 81, 91
};
#define MAX_VMVR  80 //512
#define MIN_QP          0
#define MAX_QP          51
#define SHIFT_QP        12
#define LAMBDA_ACCURACY_BITS         16
#define LAMBDA_FACTOR(lambda)        ((int)((double)(1<<LAMBDA_ACCURACY_BITS)*lambda+0.5))

#define WEIGHTED_COST(factor,bits)   (((factor)*(bits))>>LAMBDA_ACCURACY_BITS)
#define MV_COST(f,s,cx,cy,px,py)     (WEIGHTED_COST(f,mvbits[((cx)<<(s))-px]+mvbits[((cy)<<(s))-py]))
#define MV_COST_S(f,cx,cy,px,py)     (WEIGHTED_COST(f,mvbits[cx-px]+mvbits[cy-py]))

#endif
