
//#define LOG_NDEBUG 0
#define LOG_TAG "FASTENCLIB"
#include <utils/Log.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <utils/threads.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/prctl.h>

#include "m8venclib_fast.h"
#include "enc_define.h"
#include <cutils/properties.h>

#define ENCODE_DONE_TIMEOUT 100

#ifndef UCODE_MODE_FULL
#define UCODE_MODE_FULL 0
#endif

static int encode_poll(int fd, int timeout)
{
    struct pollfd poll_fd[1];
    poll_fd[0].fd = fd;
    poll_fd[0].events = POLLIN |POLLERR;
    return poll(poll_fd, 1, timeout);
}

static int RGBX32_To_RGB24Plane_NEON(unsigned char *src, unsigned char *dest, int width, int height)
{
    unsigned char *R;
    unsigned char *G;
    unsigned char *B;
    int canvas_w = ((width+31)>>5)<<5;
    int mb_h = ((height+15)>>4)<<4;
    int i, j;
    int aligned = canvas_w - width;
    if ( !src || !dest )
        return -1;

    R = dest;
    G = R + canvas_w * mb_h;
    B = G + canvas_w * mb_h;

    for ( i = 0; i < height; i += 1 ) {
        for ( j = 0; j < width; j += 8 ) {
            asm volatile (
                "vld4.8     {d0, d1, d2, d3}, [%[src]]!      \n"  // load  8 more ABGR pixels.
                "vst1.8     {d0}, [%[R]]!                    \n"  // store R.
                "vst1.8     {d1}, [%[G]]!                    \n"  // store G.
                "vst1.8     {d2}, [%[B]]!                    \n"  // store B.

                : [src] "+r" (src), [R] "+r" (R),
                  [G] "+r" (G), [B] "+r" (B)
                :
                : "cc", "memory", "d0", "d1", "d2", "d3"
            );
        }
        if (aligned) {
            R+=aligned;
            G+=aligned;
            B+=aligned;
        }
    }
    return canvas_w*mb_h*3;
}

static int RGB24_To_RGB24Plane_NEON(unsigned char *src, unsigned char *dest, int width, int height)
{
    unsigned char *R;
    unsigned char *G;
    unsigned char *B;
    int canvas_w = ((width+31)>>5)<<5;
    int mb_h = ((height+15)>>4)<<4;
    int i, j;
    int aligned = canvas_w - width;
    if ( !src || !dest )
        return -1;


    R = dest;
    G = R + canvas_w * height;
    B = G + canvas_w * height;

    for ( i = 0; i < height; i += 1 ) {
        for ( j = 0; j < width; j += 8 ) {
            asm volatile (
                "vld3.8     {d0, d1, d2}, [%[src]]!      \n"  // load 8 more BGR pixels.
                "vst1.8     {d0}, [%[R]]!                \n"  // store R.
                "vst1.8     {d1}, [%[G]]!                \n"  // store G.
                "vst1.8     {d2}, [%[B]]!                \n"  // store B.

                : [src] "+r" (src), [R] "+r" (R),
                  [G] "+r" (G), [B] "+r" (B)
                :
                : "cc", "memory", "d0", "d1", "d2"
            );
        }
        if (aligned) {
            R+=aligned;
            G+=aligned;
            B+=aligned;
        }
    }
    return canvas_w*mb_h*3;
}

static unsigned copy_to_local(fast_enc_drv_t* p)
{
    bool crop_flag = false;
    unsigned offset = 0;
    int canvas_w = 0;
    int i = 0;
    unsigned total_size = 0;
    unsigned char* src = NULL;
    unsigned char* dst = NULL;
    if (p->src.pix_height<(p->src.mb_height<<4))
        crop_flag = true;

    if (p->src.fmt != AMVENC_YUV420)
        canvas_w = ((p->src.pix_width+31)>>5)<<5;
    else
        canvas_w = ((p->src.pix_width+63)>>6)<<6;

    src = (unsigned char*)p->src.plane[0];
    dst = p->input_buf.addr;
    if (p->src.pix_width != canvas_w) {
        for (i =0; i<p->src.pix_height; i++) {
            memcpy(dst, src,p->src.pix_width);
            dst+=canvas_w;
            src+=p->src.pix_width;
        }
    }else{
        memcpy(dst, src,p->src.pix_width*p->src.pix_height);
    }
    offset = p->src.pix_height*canvas_w;

    if (crop_flag) {
        memset(p->input_buf.addr+offset, 0, ((p->src.mb_height<<4) -p->src.pix_height)*canvas_w);
        offset = canvas_w*p->src.mb_height<<4;
    }

    src = (unsigned char*)p->src.plane[1];
    dst = (unsigned char*)(p->input_buf.addr+offset);
    if ((p->src.fmt == AMVENC_NV12) || (p->src.fmt == AMVENC_NV21)) {
        if (p->src.pix_width != canvas_w) {
            for (i =0;  i<p->src.pix_height/2;i++) {
                memcpy(dst, src,p->src.pix_width);
                dst+=canvas_w;
                src+=p->src.pix_width;
            }
        } else {
            memcpy(dst, src,p->src.pix_width*p->src.pix_height/2);
        }
        offset += p->src.pix_height*canvas_w/2;
        if (crop_flag)
            memset(p->input_buf.addr+offset, 0x80, ((p->src.mb_height<<4) -p->src.pix_height)*canvas_w/2);
    } else if (p->src.fmt == AMVENC_YUV420) {
        if (p->src.pix_width != canvas_w) {
            for (i =0;i<p->src.pix_height/2;i++) {
                memcpy(dst, src,p->src.pix_width/2);
                dst+=canvas_w/2;
                src+=p->src.pix_width/2;
            }
        } else {
            memcpy(dst, src,p->src.pix_width*p->src.pix_height/4);
        }
        offset += p->src.pix_height*canvas_w/4;
        if (crop_flag) {
            memset(p->input_buf.addr+offset, 0x80, ((p->src.mb_height<<4) -p->src.pix_height)*canvas_w/4);
            offset = canvas_w*p->src.mb_height*5<<2;
        }
        src = (unsigned char*)p->src.plane[2];
        dst = (unsigned char*)(p->input_buf.addr+offset);
        if (p->src.pix_width != canvas_w) {
            for (i =0;i<p->src.pix_height/2;i++) {
                memcpy(dst, src,p->src.pix_width/2);
                dst+=canvas_w/2;
                src+=p->src.pix_width/2;
            }
        } else {
            memcpy(dst, src,p->src.pix_width*p->src.pix_height/4);
        }
        offset += p->src.pix_height*canvas_w/4;
        if (crop_flag) {
            memset(p->input_buf.addr+offset, 0x80, ((p->src.mb_height<<4) -p->src.pix_height)*canvas_w/4);
        }
    }
    total_size = canvas_w*p->src.mb_height*3<<3;
    return total_size;
}

static int set_input(fast_enc_drv_t* p, unsigned* yuv, unsigned enc_width, unsigned enc_height, AMVEncBufferType type, AMVEncFrameFmt fmt)
{
    int i;
    unsigned y = yuv[0];
    unsigned u = yuv[1];
    unsigned v = yuv[2];
    fast_input_t *src = &p->src;

    if (!y)
        return -1;

    src->pix_width  = enc_width;
    src->pix_height = enc_height;
    src->mb_width   = (src->pix_width+15)>>4;
    src->mb_height  = (src->pix_height+15)>>4;

    src->plane[1] = 0;
    src->plane[2] = 0;

    if (type == VMALLOC_BUFFER) {
        src->plane[0] = y;
        if ((fmt == AMVENC_NV21) || (fmt == AMVENC_NV12) || (fmt == AMVENC_YUV420))
            src->plane[1] = u;
        if (fmt == AMVENC_YUV420)
            src->plane[2] = v;
    }else{
        src->canvas = yuv[3];
		//need debug.
    }
    src->type = type;
    src->fmt  = fmt;
    if (src->type == VMALLOC_BUFFER) {
        if ((src->fmt != AMVENC_RGB888) && (src->fmt != AMVENC_RGBA8888)) {
            src->framesize = copy_to_local(p);
        } else if (p->src.fmt == AMVENC_RGB888) {
            src->framesize= RGB24_To_RGB24Plane_NEON((unsigned char*)p->src.plane[0],p->input_buf.addr,p->src.pix_width,p->src.pix_height);
            src->fmt = AMVENC_RGB888_PLANE;
        } else if (p->src.fmt == AMVENC_RGBA8888) {
            src->framesize = RGBX32_To_RGB24Plane_NEON((unsigned char*)p->src.plane[0],p->input_buf.addr,p->src.pix_width,p->src.pix_height);
            src->fmt = AMVENC_RGB888_PLANE;
        }
    }else{
        src->framesize = src->mb_height*src->pix_width*24;
    }
    return 0;
}

static AMVEnc_Status start_ime(fast_enc_drv_t* p, unsigned char* outptr,int* datalen)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    unsigned status;
    unsigned size = 0;
    unsigned control_info[16];
    unsigned total_time = 0;

    if (p->logtime)
        gettimeofday(&p->start_test, NULL);

    memset(control_info, 0,sizeof(control_info));
    control_info[0] = ENCODER_NON_IDR;
    control_info[1] = UCODE_MODE_FULL;
    control_info[2] = p->src.type;
    control_info[3] = p->src.fmt;
    control_info[4] = (p->src.type == VMALLOC_BUFFER)?(unsigned)p->input_buf.addr:(unsigned)p->src.canvas;
    control_info[5] = p->src.framesize;
    control_info[6] = (p->fix_qp >= 0)?p->fix_qp:p->quant;
    control_info[7] = AMVENC_FLUSH_FLAG_INPUT|AMVENC_FLUSH_FLAG_OUTPUT; // flush op;
    control_info[8] = ENCODE_DONE_TIMEOUT; // timeout op;
    ioctl(p->fd, FASTENC_AVC_IOC_NEW_CMD, &control_info[0]);

    if (encode_poll(p->fd, -1) <= 0) {
        ALOGE("start_ime: poll fail, fd:%d", p->fd);
        return AMVENC_TIMEOUT;
    }

    ioctl(p->fd, FASTENC_AVC_IOC_GET_STAGE, &status);
    ret = AMVENC_FAIL;
    if (status == ENCODER_IDR_DONE) {
        ioctl(p->fd, FASTENC_AVC_IOC_GET_OUTPUT_SIZE, &size);
        if ((size < p->output_buf.size) && (size>0)) {
            memcpy(outptr,p->output_buf.addr,size);
            *datalen  = size;
            ret = AMVENC_PICTURE_READY;
            ALOGV("start_ime: done size: %d, fd:%d ",size, p->fd);
        }
    }else{
        ALOGE("start_ime: encode timeout, status:%d, fd:%d",status, p->fd);
        ret = AMVENC_TIMEOUT;
    }
    if (ret == AMVENC_PICTURE_READY) {
        if (p->logtime) {
            gettimeofday(&p->end_test, NULL);
            total_time = p->end_test.tv_sec - p->start_test.tv_sec;
            total_time = total_time*1000000 + p->end_test.tv_usec -p->start_test.tv_usec;
            ALOGD("start_ime: need time: %d us, fd:%d",total_time, p->fd);
            p->total_encode_time +=total_time;
        }
        p->total_encode_frame++;
    }
    return ret;
}

static AMVEnc_Status start_intra(fast_enc_drv_t* p, unsigned char* outptr,int* datalen)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    unsigned status;
    unsigned size = 0;
    unsigned control_info[16];
    unsigned total_time = 0;
    if (p->logtime)
        gettimeofday(&p->start_test, NULL);

    memset(control_info, 0,sizeof(control_info));
    control_info[0] = ENCODER_IDR;
    control_info[1] = UCODE_MODE_FULL;
    control_info[2] = p->src.type;
    control_info[3] = p->src.fmt;
    control_info[4] = (p->src.type == VMALLOC_BUFFER)?(unsigned)p->input_buf.addr:(unsigned)p->src.canvas;
    control_info[5] = p->src.framesize; //(16X3/2)
    control_info[6] = (p->fix_qp >= 0)?p->fix_qp:p->quant;
    control_info[7] = (p->reencode == true)?(AMVENC_FLUSH_FLAG_OUTPUT):(AMVENC_FLUSH_FLAG_INPUT|AMVENC_FLUSH_FLAG_OUTPUT); // flush op;
    control_info[8] = ENCODE_DONE_TIMEOUT; // timeout op;
    ioctl(p->fd, FASTENC_AVC_IOC_NEW_CMD, &control_info[0]);

    if (encode_poll(p->fd, -1) <= 0) {
        ALOGE("start_intra: poll fail, fd:%d", p->fd);
        return AMVENC_TIMEOUT;
    }

    ioctl(p->fd, FASTENC_AVC_IOC_GET_STAGE, &status);
    ret = AMVENC_FAIL;
    if (status == ENCODER_IDR_DONE) {
        ioctl(p->fd, FASTENC_AVC_IOC_GET_OUTPUT_SIZE, &size);
        if ((size < p->output_buf.size) && (size>0)) {
            memcpy(outptr,p->output_buf.addr,size);
            *datalen  = size;
            ret = AMVENC_NEW_IDR;
            ALOGV("start_intra: done size: %d, fd:%d",size, p->fd);
        }
    } else {
        ALOGE("start_intra: encode timeout, status:%d, fd:%d",status, p->fd);
        ret = AMVENC_TIMEOUT;
    }

    if (ret == AMVENC_NEW_IDR) {
        if (p->reencode)
            p->reencode_frame++;
        else
            p->total_encode_frame++;
        if (p->logtime) {
            gettimeofday(&p->end_test, NULL);
            total_time = (p->end_test.tv_sec - p->start_test.tv_sec)*1000000 + p->end_test.tv_usec -p->start_test.tv_usec;
            if (p->reencode) {
                p->total_encode_time +=total_time;
                ALOGD("re-encode intra:%d need time: %d us, fd:%d",p->reencode_frame,total_time, p->fd);
            } else {
                p->total_encode_time +=total_time;
                ALOGD("start_intra: need time: %d us, fd:%d",total_time, p->fd);
            }
        }
    }
    p->reencode = false;
    return ret;
}

void* InitFastEncode(int fd, amvenc_initpara_t* init_para)
{
    int addr_index = 0;
    int ret = 0;
    unsigned buff_info[30];
    unsigned mode = UCODE_MODE_FULL;
    fast_enc_drv_t* p = NULL;
    int i = 0;

    if (!init_para) {
        ALOGE("InitFastEncode init para error.  fd:%d", fd);
        return NULL;
    }

    p = (fast_enc_drv_t*)calloc(1,sizeof(fast_enc_drv_t));
    if (!p) {
        ALOGE("InitFastEncode calloc faill. fd:%d", fd);
        return NULL;
    }

    memset(p,0,sizeof(fast_enc_drv_t));
    p->fd = fd;
    if (p->fd < 0) {
        ALOGE("InitFastEncode open encode device fail, fd:%d", p->fd);
        free(p);
        return NULL;
    }

    memset(buff_info,0,sizeof(buff_info));
    ret = ioctl(p->fd, FASTENC_AVC_IOC_GET_BUFFINFO,&buff_info[0]);
    if ((ret) || (buff_info[0] == 0)) {
        ALOGE("InitFastEncode -- old venc driver. no buffer information! fd:%d", p->fd);
        free(p);
        return NULL;
    }

    p->mmap_buff.addr = (unsigned char*)mmap(0,buff_info[0], PROT_READ|PROT_WRITE , MAP_SHARED ,p->fd, 0);
    if (p->mmap_buff.addr == MAP_FAILED) {
        ALOGE("InitFastEncode mmap fail, fd:%d", p->fd);
        free(p);
        return NULL;
    }

    p->quant = init_para->initQP;
    p->enc_width = init_para->enc_width;
    p->enc_height = init_para->enc_height;
    p->mmap_buff.size = buff_info[0];
    p->src.pix_width= init_para->enc_width;
    p->src.pix_height= init_para->enc_height;
    p->src.mb_width = (init_para->enc_width+15)>>4;
    p->src.mb_height= (init_para->enc_height+15)>>4;
    p->src.mbsize = p->src.mb_height*p->src.mb_width;
    p->sps_len = 0;
    p->gotSPS = false;
    p->pps_len = 0;
    p->gotPPS = false;
    p->fix_qp = -1;

    buff_info[0] = mode;
    buff_info[1] = p->src.mb_height;
    buff_info[2] = p->enc_width;
    buff_info[3] = p->enc_height;
    ret = ioctl(p->fd, FASTENC_AVC_IOC_CONFIG_INIT,&buff_info[0]);
    if (ret) {
        ALOGE("InitM8Encode config init fai, fd:%dl", p->fd);
        munmap(p->mmap_buff.addr ,p->mmap_buff.size);
        free(p);
        return NULL;
    }

    p->input_buf.addr = p->mmap_buff.addr+buff_info[1];
    p->input_buf.size = buff_info[3]-buff_info[1];

    p->ref_buf_y[0].addr = p->mmap_buff.addr +buff_info[3];
    p->ref_buf_y[0].size = buff_info[4];
    p->ref_buf_uv[0].addr = p->mmap_buff.addr +buff_info[5];
    p->ref_buf_uv[0].size = buff_info[6];

    p->ref_buf_y[1].addr = p->mmap_buff.addr +buff_info[7];
    p->ref_buf_y[1].size = buff_info[8];
    p->ref_buf_uv[1].addr = p->mmap_buff.addr +buff_info[9];
    p->ref_buf_uv[1].size = buff_info[10];
    p->output_buf.addr = p->mmap_buff.addr +buff_info[11] ;
    p->output_buf.size = buff_info[12];



    p->mCancel = false;
    p->total_encode_frame  = 0;
    p->total_encode_time = 0;
    p->reencode = false;
    p->reencode_frame = 0;
    {
        char prop[PROPERTY_VALUE_MAX];
        int value = 0;
        p->logtime = false;
        memset(prop,0,sizeof(prop));
        if (property_get("hw.encoder.log.flag", prop, NULL) > 0) {
            sscanf(prop,"%d",&value);
        }
        if (value&0x8) {
            ALOGD("Enable Debug Time Log, fd:%d", p->fd);
            p->logtime = true;
        }

        value = -1;
        memset(prop,0,sizeof(prop));
        if (property_get("hw.encoder.fix_qp", prop, NULL) > 0) {
            sscanf(prop,"%d",&value);
            if ((value >= 0) && (value < 51)) {
                p->fix_qp = value;
                ALOGD("Enable fix qp mode: %d. fd:%d", p->fix_qp, p->fd);
            }
        }
    }
    return (void *)p;
}

AMVEnc_Status FastEncodeInitFrame(void *dev, unsigned *yuv, AMVEncBufferType type, AMVEncFrameFmt fmt, bool IDRframe)
{
    fast_enc_drv_t* p = (fast_enc_drv_t*)dev;
    AMVEnc_Status ret = AMVENC_FAIL;
    unsigned total_time = 0;

    if ((!p) || (!yuv))
        return ret;

    if (p->logtime)
        gettimeofday(&p->start_test, NULL);

    if (p->reencode) {
        ALOGE("M8VEncodeInitFrame , re-encode fail, fd:%d", p->fd);
        return AMVENC_FAIL;
    }

    set_input(p, yuv,p->enc_width, p->enc_height, type,fmt);

    p->IDRframe =IDRframe;
    p->reencode = false;
    if (p->IDRframe) {
        ret = AMVENC_NEW_IDR;
    } else {
        ret = AMVENC_SUCCESS;
    }
    if (p->logtime) {
        gettimeofday(&p->end_test, NULL);
        total_time = (p->end_test.tv_sec - p->start_test.tv_sec)*1000000 + p->end_test.tv_usec -p->start_test.tv_usec;
        p->total_encode_time +=total_time;
        ALOGD("M8VEncodeInitFrame: need time: %d us, ret:%d, fd:%d",total_time,ret, p->fd);
    }
    return ret;
}

AMVEnc_Status FastEncodeSPS_PPS(void* dev, unsigned char* outptr,int* datalen)
{
    fast_enc_drv_t* p = (fast_enc_drv_t*)dev;
    AMVEnc_Status ret = AMVENC_FAIL;
    unsigned status;
    unsigned size = 0;
    unsigned control_info[5];

    control_info[0] = ENCODER_SEQUENCE;
    control_info[1] = UCODE_MODE_FULL;
    control_info[2] = 26; //init qp;
    control_info[3] = AMVENC_FLUSH_FLAG_OUTPUT;
    control_info[4] = 0; // never timeout
    ioctl(p->fd, FASTENC_AVC_IOC_NEW_CMD, &control_info[0]);

    if (encode_poll(p->fd, -1) <= 0) {
        ALOGE("sps pps: poll fail, fd:%d", p->fd);
        return AMVENC_TIMEOUT;
    }

    ioctl(p->fd, FASTENC_AVC_IOC_GET_STAGE, &status);

    ALOGV("FastEncodeSPS_PPS status:%d, fd:%d", status, p->fd);
    ret = AMVENC_FAIL;
    if (status == ENCODER_PICTURE_DONE) {
        ioctl(p->fd, FASTENC_AVC_IOC_GET_OUTPUT_SIZE, &size);
        p->sps_len = (size >>16)&0xffff;
        p->pps_len = size & 0xffff;
        if (((p->sps_len+ p->pps_len) < p->output_buf.size) && (p->sps_len>0) && (p->pps_len>0)) {
            p->gotSPS = true;
            p->gotPPS= true;
            memcpy(outptr,p->output_buf.addr,p->pps_len+p->sps_len);
            *datalen  = p->pps_len+p->sps_len;
            ret = AMVENC_SUCCESS;
        }
    } else {
        ALOGE("sps pps timeout, status:%d, fd:%d",status, p->fd);
        ret = AMVENC_TIMEOUT;
    }
    return ret;
}

AMVEnc_Status FastEncodeSlice(void* dev, unsigned char* outptr,int* datalen)
{
    fast_enc_drv_t* p = (fast_enc_drv_t*)dev;
    AMVEnc_Status ret = AMVENC_FAIL;
    if ((!p) || (!outptr) || (!datalen))
        return ret;

    if (p->IDRframe || p->reencode) {
        ret = start_intra(p,outptr,datalen);
    } else {
        ret = start_ime(p,outptr,datalen);
    }
    return ret;
}

AMVEnc_Status FastEncodeCommit(void* dev,  bool IDR)
{
    fast_enc_drv_t* p = (fast_enc_drv_t*)dev;
    AMVEnc_Status ret = AMVENC_FAIL;
    int status = 0;
    if (!p)
        return ret;
    status = (IDR == true)?ENCODER_IDR:ENCODER_NON_IDR;
    if (ioctl(p->fd, FASTENC_AVC_IOC_SUBMIT_ENCODE_DONE ,&status)== 0)
        ret = AMVENC_SUCCESS;
    return ret;
}

void UnInitFastEncode(void* dev)
{
    fast_enc_drv_t* p = (fast_enc_drv_t*)dev;
    if (!p)
        return;

    p->mCancel = true;
    if (p->mmap_buff.addr)
        munmap(p->mmap_buff.addr ,p->mmap_buff.size);

    if (p->logtime)
        ALOGD("%s, %d, total_encode_frame: %d,  re-encode intra:%d, total_encode_time: %d ms, fd:%d",
                    __func__, __LINE__, p->total_encode_frame,p->reencode_frame,p->total_encode_time/1000, p->fd);
    free(p);
    return;
}

