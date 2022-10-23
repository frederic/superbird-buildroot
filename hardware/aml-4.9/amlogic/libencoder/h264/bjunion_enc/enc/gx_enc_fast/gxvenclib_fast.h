#ifndef AML_VIDEO_ENCODER_GX_FAST_
#define AML_VIDEO_ENCODER_GX_FAST_

#include <time.h>
#include <sys/time.h>
#include "AML_HWEncoder.h"
#include "enc_define.h"

#define FASTGX_AVC_IOC_MAGIC  'E'

#define FASTGX_AVC_IOC_GET_ADDR            _IOW(FASTGX_AVC_IOC_MAGIC, 0x00, unsigned int)
#define FASTGX_AVC_IOC_INPUT_UPDATE        _IOW(FASTGX_AVC_IOC_MAGIC, 0x01, unsigned int)
#define FASTGX_AVC_IOC_NEW_CMD             _IOW(FASTGX_AVC_IOC_MAGIC, 0x02, unsigned int)
#define FASTGX_AVC_IOC_GET_STAGE           _IOW(FASTGX_AVC_IOC_MAGIC, 0x03, unsigned int)
#define FASTGX_AVC_IOC_GET_OUTPUT_SIZE     _IOW(FASTGX_AVC_IOC_MAGIC, 0x04, unsigned int)
#define FASTGX_AVC_IOC_CONFIG_INIT         _IOW(FASTGX_AVC_IOC_MAGIC, 0x05, unsigned int)
#define FASTGX_AVC_IOC_FLUSH_CACHE         _IOW(FASTGX_AVC_IOC_MAGIC, 0x06, unsigned int)
#define FASTGX_AVC_IOC_FLUSH_DMA           _IOW(FASTGX_AVC_IOC_MAGIC, 0x07, unsigned int)
#define FASTGX_AVC_IOC_GET_BUFFINFO        _IOW(FASTGX_AVC_IOC_MAGIC, 0x08, unsigned int)
#define FASTGX_AVC_IOC_SUBMIT_ENCODE_DONE  _IOW(FASTGX_AVC_IOC_MAGIC, 0x09, unsigned int)
#define FASTGX_AVC_IOC_READ_CANVAS         _IOW(FASTGX_AVC_IOC_MAGIC, 0x0a, unsigned int)

#define TARGET_BITS_TABLE_SIZE 256

typedef struct {
    short mvx;
    short mvy;
} mv_t;

typedef struct {
    unsigned char imode;
    unsigned char LPred[16];
    unsigned char CPred;
    unsigned short sad;
} intra_info_t;

typedef struct {
    mv_t mv[16];
    unsigned short sad;
} inter_info_t;

typedef struct {
    uint32_t mbx;
    uint32_t mby;
    unsigned char mb_type;

    unsigned char quant;
    unsigned char cbp;
    unsigned short bits;

    intra_info_t intra;
    inter_info_t inter;

    int final_sad;
} mb_t;

typedef struct {
    uint32_t crop_top;
    uint32_t crop_bottom;
    uint32_t crop_left;
    uint32_t crop_right;
    uint32_t src_w;
    uint32_t src_h;
} gx_crop_info;

typedef struct {
    int pix_width;
    int pix_height;

    int mb_width;
    int mb_height;
    int mbsize;

    int framesize;
    AMVEncBufferType type;
    AMVEncFrameFmt fmt;
    uint32_t canvas;
    gx_crop_info crop;
    ulong plane[3];
    ulong mmapsize;
} gx_fast_input_t;

typedef struct {
    unsigned char* addr;
    uint32_t size;
} gx_fast_buff_t;

#define QP_TAB_NUM 32
#define ADJUSTED_QP_FLAG 64

typedef struct {
    uint8_t i4_qp[QP_TAB_NUM];
    uint8_t i16_qp[QP_TAB_NUM];
    uint8_t p16_qp[QP_TAB_NUM];
} qp_table_t;

typedef struct {
    uint32_t i4_bits[QP_TAB_NUM];
    uint32_t i16_bits[QP_TAB_NUM];
    uint32_t p16_bits[QP_TAB_NUM];

    uint32_t i4_avr_bits[QP_TAB_NUM];
    uint32_t i16_avr_bits[QP_TAB_NUM];
    uint32_t p16_avr_bits[QP_TAB_NUM];

    uint32_t i4_count[QP_TAB_NUM];
    uint32_t i16_count[QP_TAB_NUM];
    uint32_t p16_count[QP_TAB_NUM];

    int32_t i4_min[QP_TAB_NUM];
    int32_t i16_min[QP_TAB_NUM];
    int32_t p16_min[QP_TAB_NUM];

    int32_t i4_max[QP_TAB_NUM];
    int32_t i16_max[QP_TAB_NUM];
    int32_t p16_max[QP_TAB_NUM];

    qp_table_t qp_table;

    uint32_t i_count;
    uint32_t f_sad_count;
    int abs_mv_sum;
} qp_statistic_info_t;

typedef struct {
    int fd;
    bool IDRframe;
    bool mCancel;
    bool re_encode;

    uint32_t enc_width;
    uint32_t enc_height;
    uint32_t pre_quant;
    uint32_t target;
    uint32_t make_qptl;
    int32_t quant;

    int fix_qp;
    int nr_mode;
    bool gotSPS;
    uint32_t sps_len;
    bool gotPPS;
    uint32_t pps_len;

    uint32_t total_encode_frame;
    uint32_t total_encode_time;

    gx_fast_input_t src;

    uint32_t me_weight;
    uint32_t i4_weight;
    uint32_t i16_weight;

    gx_fast_buff_t mmap_buff;
    gx_fast_buff_t input_buf;
    gx_fast_buff_t output_buf;
    gx_fast_buff_t scale_buff;
    gx_fast_buff_t dump_buf;
    gx_fast_buff_t cbr_buff;

    mb_t* mb_info;

    bool scale_enable;
    qp_statistic_info_t qp_stic;
    bool logtime;
    struct timeval start_test;
    struct timeval end_test;
    uint8_t encode_once;
    int max_qp_count;
    int need_inc_me_weight;
    int inc_me_weight_count;

    uint32_t block_width;
    uint32_t block_width_n;
    uint32_t block_height;
    uint32_t block_height_n;
    uint32_t block_mb_size[TARGET_BITS_TABLE_SIZE];
    uint32_t block_sad_size[TARGET_BITS_TABLE_SIZE];
    bool cbr_hw;
    uint32_t *qp_tbl;
} gx_fast_enc_drv_t;

enum qp_table_type {curve, line};

extern void* GxInitFastEncode(int fd, amvenc_initpara_t* init_para);
extern AMVEnc_Status GxFastEncodeInitFrame(void *dev, ulong *yuv, AMVEncBufferType type, AMVEncFrameFmt fmt, bool IDRframe);
extern AMVEnc_Status GxFastEncodeSPS_PPS(void *dev, unsigned char* outptr, int* datalen);
extern AMVEnc_Status GxFastEncodeSlice(void *dev, unsigned char* outptr, int* datalen);
extern AMVEnc_Status GxFastEncodeCommit(void* dev, bool IDR);
extern void GxUnInitFastEncode(void *dev);

#endif
