#ifndef AML_VIDEO_ENCODER_M8_
#define AML_VIDEO_ENCODER_M8_

#include "enc_define.h"

#define FASTENC_AVC_IOC_MAGIC  'E'

#define FASTENC_AVC_IOC_GET_ADDR                    _IOW(FASTENC_AVC_IOC_MAGIC, 0x00, unsigned int)
#define FASTENC_AVC_IOC_INPUT_UPDATE                _IOW(FASTENC_AVC_IOC_MAGIC, 0x01, unsigned int)
#define FASTENC_AVC_IOC_NEW_CMD                 _IOW(FASTENC_AVC_IOC_MAGIC, 0x02, unsigned int)
#define FASTENC_AVC_IOC_GET_STAGE               _IOW(FASTENC_AVC_IOC_MAGIC, 0x03, unsigned int)
#define FASTENC_AVC_IOC_GET_OUTPUT_SIZE         _IOW(FASTENC_AVC_IOC_MAGIC, 0x04, unsigned int)
#define FASTENC_AVC_IOC_CONFIG_INIT                 _IOW(FASTENC_AVC_IOC_MAGIC, 0x05, unsigned int)
#define FASTENC_AVC_IOC_FLUSH_CACHE                 _IOW(FASTENC_AVC_IOC_MAGIC, 0x06, unsigned int)
#define FASTENC_AVC_IOC_FLUSH_DMA               _IOW(FASTENC_AVC_IOC_MAGIC, 0x07, unsigned int)
#define FASTENC_AVC_IOC_GET_BUFFINFO            _IOW(FASTENC_AVC_IOC_MAGIC, 0x08, unsigned int)
#define FASTENC_AVC_IOC_SUBMIT_ENCODE_DONE      _IOW(FASTENC_AVC_IOC_MAGIC, 0x09, unsigned int)
#define FASTENC_AVC_IOC_READ_CANVAS                 _IOW(FASTENC_AVC_IOC_MAGIC, 0x0a, unsigned int)

#define UCODE_MODE_FULL 0
#define UCODE_MODE_SW_MIX 1

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
    unsigned canvas;
    unsigned plane[3];
} fast_input_t;

typedef struct
{
    unsigned char *addr;
    unsigned size;
} fast_buff_t;

typedef struct
{
    int fd;
    int IDRframe;
    int mCancel;

    unsigned enc_width;
    unsigned enc_height;
    unsigned quant;

    int fix_qp;

    int gotSPS;
    unsigned sps_len;
    int gotPPS;
    unsigned pps_len;

    unsigned total_encode_frame;
    unsigned total_encode_time;

    fast_input_t src;

    fast_buff_t mmap_buff;
    fast_buff_t input_buf;
    fast_buff_t ref_buf_y[2];
    fast_buff_t ref_buf_uv[2];
    fast_buff_t output_buf;
    unsigned reencode_frame;
    int reencode;
} fast_enc_drv_t;

extern void *InitFastEncode(int fd, amvenc_initpara_t *init_para);
extern AMVEnc_Status FastEncodeInitFrame(void *dev, unsigned *yuv, AMVEncBufferType type, AMVEncFrameFmt fmt, int IDRframe);
extern AMVEnc_Status FastEncodeSPS_PPS(void *dev, unsigned char *outptr, int *datalen);
//extern AMVEnc_Status FastEncodePPS(void *dev, unsigned char* outptr,int* datalen);
extern AMVEnc_Status FastEncodeSlice(void *dev, unsigned char *outptr, int *datalen);
extern AMVEnc_Status FastEncodeCommit(void *dev,  int IDR);
extern void UnInitFastEncode(void *dev);

#endif
