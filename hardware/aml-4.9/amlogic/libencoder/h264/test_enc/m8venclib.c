#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "m8venclib.h"
#include "enc_define.h"
#define ENCODE_DONE_TIMEOUT 100

#ifndef UCODE_MODE_FULL
#define UCODE_MODE_FULL 0
#endif

#define DEBUG_TIME
#ifdef DEBUG_TIME
static struct timeval start_test, end_test;
#endif

static int encode_poll(int fd, int timeout)
{
    struct pollfd poll_fd[1];
    poll_fd[0].fd = fd;
    poll_fd[0].events = POLLIN | POLLERR;
    return poll(poll_fd, 1, timeout);
}
static int dump_ref_fd = -1;

static int RGBX32_To_RGB24Plane_NEON(unsigned char *src, unsigned char *dest, int width, int height)
{
    int canvas_w = ((width + 31) >> 5) << 5;
    int mb_h = ((height + 15) >> 4) << 4;
    return canvas_w * mb_h * 3;
}

static int RGB24_To_RGB24Plane_NEON(unsigned char *src, unsigned char *dest, int width, int height)
{
    int canvas_w = ((width + 31) >> 5) << 5;
    int mb_h = ((height + 15) >> 4) << 4;
    return canvas_w * mb_h * 3;
}

static unsigned copy_to_local(fast_enc_drv_t *p)
{
    int crop_flag = 0;
    unsigned offset = 0;
    int canvas_w = 0;
    int i = 0;
    unsigned total_size = 0;
    unsigned char *src = NULL;
    unsigned char *dst = NULL;
    if (p->src.pix_height < (p->src.mb_height << 4))
        crop_flag = 1;
    if (p->src.fmt != AMVENC_YUV420)
        canvas_w = ((p->src.pix_width + 31) >> 5) << 5;
    else
        canvas_w = ((p->src.pix_width + 63) >> 6) << 6;
    src = (unsigned char *)p->src.plane[0];
    dst = p->input_buf.addr;
    if (p->src.pix_width != canvas_w)
    {
        for (i = 0; i < p->src.pix_height; i++)
        {
            memcpy(dst, src, p->src.pix_width);
            dst += canvas_w;
            src += p->src.pix_width;
        }
    }
    else
    {
        memcpy(dst, src, p->src.pix_width * p->src.pix_height);
    }
    offset = p->src.pix_height * canvas_w;
    if (crop_flag)
    {
        memset(p->input_buf.addr + offset, 0, ((p->src.mb_height << 4) - p->src.pix_height)*canvas_w);
        offset = canvas_w * p->src.mb_height << 4;
    }
    src = (unsigned char *)p->src.plane[1];
    dst = (unsigned char *)(p->input_buf.addr + offset);
    if ((p->src.fmt == AMVENC_NV12) || (p->src.fmt == AMVENC_NV21))
    {
        if (p->src.pix_width != canvas_w)
        {
            for (i = 0;  i < p->src.pix_height / 2; i++)
            {
                memcpy(dst, src, p->src.pix_width);
                dst += canvas_w;
                src += p->src.pix_width;
            }
        }
        else
        {
            memcpy(dst, src, p->src.pix_width * p->src.pix_height / 2);
        }
        offset += p->src.pix_height * canvas_w / 2;
        if (crop_flag)
            memset(p->input_buf.addr + offset, 0x80, ((p->src.mb_height << 4) - p->src.pix_height)*canvas_w / 2);
    }
    else if (p->src.fmt == AMVENC_YUV420)
    {
        if (p->src.pix_width != canvas_w)
        {
            for (i = 0; i < p->src.pix_height / 2; i++)
            {
                memcpy(dst, src, p->src.pix_width / 2);
                dst += canvas_w / 2;
                src += p->src.pix_width / 2;
            }
        }
        else
        {
            memcpy(dst, src, p->src.pix_width * p->src.pix_height / 4);
        }
        offset += p->src.pix_height * canvas_w / 4;
        if (crop_flag)
        {
            memset(p->input_buf.addr + offset, 0x80, ((p->src.mb_height << 4) - p->src.pix_height)*canvas_w / 4);
            offset = canvas_w * p->src.mb_height * 5 << 2;
        }
        src = (unsigned char *)p->src.plane[2];
        dst = (unsigned char *)(p->input_buf.addr + offset);
        if (p->src.pix_width != canvas_w)
        {
            for (i = 0; i < p->src.pix_height / 2; i++)
            {
                memcpy(dst, src, p->src.pix_width / 2);
                dst += canvas_w / 2;
                src += p->src.pix_width / 2;
            }
        }
        else
        {
            memcpy(dst, src, p->src.pix_width * p->src.pix_height / 4);
        }
        offset += p->src.pix_height * canvas_w / 4;
        if (crop_flag)
        {
            memset(p->input_buf.addr + offset, 0x80, ((p->src.mb_height << 4) - p->src.pix_height)*canvas_w / 4);
        }
    }
    total_size = canvas_w * p->src.mb_height * 3 << 3;
    return total_size;
}


static int set_input(fast_enc_drv_t *p, unsigned *yuv, unsigned enc_width, unsigned enc_height, AMVEncBufferType type, AMVEncFrameFmt fmt)
{
    unsigned y = yuv[0];
    unsigned u = yuv[1];
    unsigned v = yuv[2];
    fast_input_t *src = &p->src;
    if (!y)
        return -1;
    src->pix_width  = enc_width;
    src->pix_height = enc_height;
    src->mb_width   = (src->pix_width + 15) >> 4;
    src->mb_height  = (src->pix_height + 15) >> 4;
    src->plane[1] = 0;
    src->plane[2] = 0;
    if (type == VMALLOC_BUFFER)
    {
        src->plane[0] = y;
        if ((fmt == AMVENC_NV21) || (fmt == AMVENC_NV12) || (fmt == AMVENC_YUV420))
            src->plane[1] = u;
        if (fmt == AMVENC_YUV420)
            src->plane[2] = v;
    }
    else
    {
        src->canvas = yuv[3];
        //need debug.
    }
    src->type = type;
    src->fmt  = fmt;
    if (src->type == VMALLOC_BUFFER)
    {
        if ((src->fmt != AMVENC_RGB888) && (src->fmt != AMVENC_RGBA8888))
        {
            src->framesize = copy_to_local(p);
        }
        else if (p->src.fmt == AMVENC_RGB888)
        {
            src->framesize = RGB24_To_RGB24Plane_NEON((unsigned char *)p->src.plane[0], p->input_buf.addr, p->src.pix_width, p->src.pix_height);
            src->fmt = AMVENC_RGB888_PLANE;
        }
        else if (p->src.fmt == AMVENC_RGBA8888)
        {
            src->framesize = RGBX32_To_RGB24Plane_NEON((unsigned char *)p->src.plane[0], p->input_buf.addr, p->src.pix_width, p->src.pix_height);
            src->fmt = AMVENC_RGB888_PLANE;
        }
    }
    else
    {
        src->framesize = src->mb_height * src->pix_width * 24;
    }
    return 0;
}

static AMVEnc_Status start_ime(fast_enc_drv_t *p, unsigned char *outptr, int *datalen)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    unsigned status;
    unsigned size = 0;
    unsigned control_info[9];
#ifdef DEBUG_TIME
    unsigned total_time = 0;
    gettimeofday(&start_test, NULL);
#endif
    control_info[0] = ENCODER_NON_IDR;
    control_info[1] = UCODE_MODE_FULL;
    control_info[2] = p->src.type;
    control_info[3] = p->src.fmt;
    control_info[4] = (p->src.type == VMALLOC_BUFFER) ? (unsigned)p->input_buf.addr : (unsigned)p->src.canvas;
    control_info[5] = p->src.framesize;
    control_info[6] = (p->fix_qp >= 0) ? p->fix_qp : p->quant;
    control_info[7] = AMVENC_FLUSH_FLAG_INPUT | AMVENC_FLUSH_FLAG_OUTPUT; // flush op;
    control_info[8] = ENCODE_DONE_TIMEOUT; // timeout op;
    ioctl(p->fd, FASTENC_AVC_IOC_NEW_CMD, &control_info[0]);
    if (encode_poll(p->fd, -1) <= 0)
    {
        printf("start_ime: poll fail, fd:%d\n", p->fd);
        return AMVENC_TIMEOUT;
    }
    ioctl(p->fd, FASTENC_AVC_IOC_GET_STAGE, &status);
    ret = AMVENC_FAIL;
    if (status == ENCODER_IDR_DONE)
    {
        ioctl(p->fd, FASTENC_AVC_IOC_GET_OUTPUT_SIZE, &size);
        if ((size < p->output_buf.size) && (size > 0))
        {
            memcpy(outptr, p->output_buf.addr, size);
            *datalen  = size;
            ret = AMVENC_PICTURE_READY;
            printf("start_ime: done size: %d, fd:%d\n", size, p->fd);
        }
    }
    else
    {
        printf("start_ime: encode timeout, status:%d, fd:%d \n", status, p->fd);
        ret = AMVENC_TIMEOUT;
    }
    if (ret == AMVENC_PICTURE_READY)
    {
        p->total_encode_frame++;
    }
#ifdef DEBUG_TIME
    gettimeofday(&end_test, NULL);
    total_time = end_test.tv_sec - start_test.tv_sec;
    total_time = total_time * 1000000 + end_test.tv_usec - start_test.tv_usec;
    printf("start_ime: need time: %d us \n", total_time);
    p->total_encode_frame++;
    p->total_encode_time += total_time;
#endif
    return ret;
}

static AMVEnc_Status start_intra(fast_enc_drv_t *p, unsigned char *outptr, int *datalen)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    unsigned status;
    unsigned size = 0;
    unsigned control_info[9];
#ifdef DEBUG_TIME
    unsigned total_time = 0;
    gettimeofday(&start_test, NULL);
#endif
    control_info[0] = ENCODER_IDR;
    control_info[1] = UCODE_MODE_FULL;
    control_info[2] = p->src.type;
    control_info[3] = p->src.fmt;
    control_info[4] = (p->src.type == VMALLOC_BUFFER) ? (unsigned)p->input_buf.addr : (unsigned)p->src.canvas;
    control_info[5] = p->src.framesize; //(16X3/2)
    control_info[6] = (p->fix_qp >= 0) ? p->fix_qp : p->quant;
    control_info[7] = (p->reencode == 1) ? (AMVENC_FLUSH_FLAG_OUTPUT) : (AMVENC_FLUSH_FLAG_INPUT | AMVENC_FLUSH_FLAG_OUTPUT); // flush op;
    control_info[8] = ENCODE_DONE_TIMEOUT; // timeout op;
    ioctl(p->fd, FASTENC_AVC_IOC_NEW_CMD, &control_info[0]);
    if (encode_poll(p->fd, -1) <= 0)
    {
        printf("start_intra: poll fail, fd:%d\n", p->fd);
        return AMVENC_TIMEOUT;
    }
    ioctl(p->fd, FASTENC_AVC_IOC_GET_STAGE, &status);
    ret = AMVENC_FAIL;
    if (status == ENCODER_IDR_DONE)
    {
        ioctl(p->fd, FASTENC_AVC_IOC_GET_OUTPUT_SIZE, &size);
        if ((size < p->output_buf.size) && (size > 0))
        {
            memcpy(outptr, p->output_buf.addr, size);
            *datalen  = size;
            ret = AMVENC_NEW_IDR;
            printf("start_intra: done size: %d \n", size);
        }
    }
    else
    {
        printf("start_intra: encode timeout, status:%d, fd:%d \n", status, p->fd);
        ret = AMVENC_TIMEOUT;
    }
    if (ret == AMVENC_NEW_IDR)
    {
        if (p->reencode)
            p->reencode_frame++;
        else
            p->total_encode_frame++;
    }
#ifdef DEBUG_TIME
    gettimeofday(&end_test, NULL);
    total_time = (end_test.tv_sec - start_test.tv_sec) * 1000000 + end_test.tv_usec - start_test.tv_usec;
    p->total_encode_time += total_time;
    printf("start_intra: need time: %d us \n", total_time);
#endif
    p->reencode = 0;
    return ret;
}

void *InitFastEncode(int fd, amvenc_initpara_t *init_para)
{
    int ret = 0;
    unsigned buff_info[30];
    unsigned mode = UCODE_MODE_FULL;
    fast_enc_drv_t *p = NULL;
    if (!init_para)
    {
        printf("InitFastEncode init para error.  fd:%d \n", fd);
        return NULL;
    }
    p = (fast_enc_drv_t *)calloc(1, sizeof(fast_enc_drv_t));
    if (!p)
    {
        printf("InitFastEncode calloc faill. fd:%d \n", fd);
        return NULL;
    }
    dump_ref_fd = open("/root/tmmp/ref.dump", O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (dump_ref_fd < 0)
    {
        printf("InitM8VEncode ref file error!\n");
    }
    memset(p, 0, sizeof(fast_enc_drv_t));
    p->fd = fd;
    if (p->fd < 0)
    {
        printf("InitM8VEncode open encode device fail \n");
        free(p);
        if (dump_ref_fd < 0)
            close(dump_ref_fd);
        dump_ref_fd = -1;
        return NULL;
    }
    memset(buff_info, 0, sizeof(buff_info));
    ret = ioctl(p->fd, FASTENC_AVC_IOC_GET_BUFFINFO, &buff_info[0]);
    if ((ret) || (buff_info[0] == 0))
    {
        printf("InitM8VEncode -- old venc driver. no buffer information!\n");
        free(p);
        if (dump_ref_fd < 0)
            close(dump_ref_fd);
        dump_ref_fd = -1;
        return NULL;
    }
    p->mmap_buff.addr = (unsigned char *)mmap(0, buff_info[0], PROT_READ | PROT_WRITE , MAP_SHARED , p->fd, 0);
    if (p->mmap_buff.addr == MAP_FAILED)
    {
        printf("InitM8VEncode mmap fail \n");
        free(p);
        if (dump_ref_fd < 0)
            close(dump_ref_fd);
        dump_ref_fd = -1;
        return NULL;
    }
    p->quant = init_para->initQP;
    p->enc_width = init_para->enc_width;
    p->enc_height = init_para->enc_height;
    p->mmap_buff.size = buff_info[0];
    p->src.pix_width = init_para->enc_width;
    p->src.pix_height = init_para->enc_height;
    p->src.mb_width = (init_para->enc_width + 15) >> 4;
    p->src.mb_height = (init_para->enc_height + 15) >> 4;
    p->src.mbsize = p->src.mb_height * p->src.mb_width;
    p->sps_len = 0;
    p->gotSPS = 0;
    p->pps_len = 0;
    p->gotPPS = 0;
    p->fix_qp = -1;
    buff_info[0] = mode;
    buff_info[1] = p->src.mb_height;
    buff_info[2] = p->enc_width;
    buff_info[3] = p->enc_height;
    ret = ioctl(p->fd, FASTENC_AVC_IOC_CONFIG_INIT, &buff_info[0]);
    if (ret)
    {
        printf("InitM8Encode config init fail, fd:%d \n", p->fd);
        munmap(p->mmap_buff.addr , p->mmap_buff.size);
        free(p);
        if (dump_ref_fd < 0)
            close(dump_ref_fd);
        dump_ref_fd = -1;
        return NULL;
    }
    p->input_buf.addr = p->mmap_buff.addr + buff_info[1];
    p->input_buf.size = buff_info[3] - buff_info[1];
    p->ref_buf_y[0].addr = p->mmap_buff.addr + buff_info[3];
    p->ref_buf_y[0].size = buff_info[4];
    p->ref_buf_uv[0].addr = p->mmap_buff.addr + buff_info[5];
    p->ref_buf_uv[0].size = buff_info[6];
    p->ref_buf_y[1].addr = p->mmap_buff.addr + buff_info[7];
    p->ref_buf_y[1].size = buff_info[8];
    p->ref_buf_uv[1].addr = p->mmap_buff.addr + buff_info[9];
    p->ref_buf_uv[1].size = buff_info[10];
    p->output_buf.addr = p->mmap_buff.addr + buff_info[11] ;
    p->output_buf.size = buff_info[12];
    p->mCancel = 0;
    p->total_encode_frame  = 0;
    p->total_encode_time = 0;
    p->reencode = 0;
    p->reencode_frame = 0;
    return (void *)p;
}

AMVEnc_Status FastEncodeInitFrame(void *dev, unsigned *yuv, AMVEncBufferType type, AMVEncFrameFmt fmt, int IDRframe)
{
    fast_enc_drv_t *p = (fast_enc_drv_t *)dev;
    AMVEnc_Status ret = AMVENC_FAIL;
    unsigned total_time = 0;
    if ((!p) || (!yuv))
        return ret;
#ifdef DEBUG_TIME
    gettimeofday(&start_test, NULL);
#endif
    if (p->reencode)
    {
        printf("M8VEncodeInitFrame , re-encode fail, fd:%d \n", p->fd);
        return AMVENC_FAIL;
    }
    set_input(p, yuv, p->enc_width, p->enc_height, type, fmt);
    p->IDRframe = IDRframe;
    p->reencode = 0;
    if (p->IDRframe)
    {
        ret = AMVENC_NEW_IDR;
    }
    else
    {
        ret = AMVENC_SUCCESS;
    }
#ifdef DEBUG_TIME
    gettimeofday(&end_test, NULL);
    total_time = (end_test.tv_sec - start_test.tv_sec) * 1000000 + end_test.tv_usec - start_test.tv_usec;
    p->total_encode_time += total_time;
    printf("M8VEncodeInitFrame: need time: %d us, ret:%d \n", total_time, ret);
#endif
    return ret;
}

AMVEnc_Status FastEncodeSPS_PPS(void *dev, unsigned char *outptr, int *datalen)
{
    fast_enc_drv_t *p = (fast_enc_drv_t *)dev;
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
    if (encode_poll(p->fd, -1) <= 0)
    {
        printf("sps pps: poll fail, fd:%d\n", p->fd);
        return AMVENC_TIMEOUT;
    }
    ioctl(p->fd, FASTENC_AVC_IOC_GET_STAGE, &status);
    ret = AMVENC_FAIL;
    if (status == ENCODER_PICTURE_DONE)
    {
        ioctl(p->fd, FASTENC_AVC_IOC_GET_OUTPUT_SIZE, &size);
        p->sps_len = (size >> 16) & 0xffff;
        p->pps_len = size & 0xffff;
        if (((p->sps_len + p->pps_len) < p->output_buf.size) && (p->sps_len > 0) && (p->pps_len > 0))
        {
            p->gotSPS = 1;
            p->gotPPS = 1;
            memcpy(outptr, p->output_buf.addr, p->pps_len + p->sps_len);
            *datalen  = p->pps_len + p->sps_len;
            ret = AMVENC_SUCCESS;
        }
    }
    else
    {
        printf("sps pps timeout, status:%d, fd:%d \n", status, p->fd);
        ret = AMVENC_TIMEOUT;
    }
    return ret;
}

static int GetM8VencRefBuf(int fd)
{
    int ret = -1;
    int addr_index = 0;
    if (fd >= 0)
    {
        ioctl(fd, FASTENC_AVC_IOC_GET_ADDR, &addr_index);
        if (addr_index == ENCODER_BUFFER_REF0)
        {
            ret = 0;
        }
        else
        {
            ret = 1;
        }
    }
    return ret;
}

AMVEnc_Status FastEncodeSlice(void *dev, unsigned char *outptr, int *datalen)
{
    fast_enc_drv_t *p = (fast_enc_drv_t *)dev;
    AMVEnc_Status ret = AMVENC_FAIL;
    if ((!p) || (!outptr) || (!datalen))
        return ret;
    if (p->IDRframe || p->reencode)
    {
        ret = start_intra(p, outptr, datalen);
    }
    else
    {
        ret = start_ime(p, outptr, datalen);
    }
    int ref_id = GetM8VencRefBuf(p->fd);
    if (dump_ref_fd > 0)
    {
        write(dump_ref_fd, (unsigned char *)p->ref_buf_y[ref_id].addr, p->enc_height * p->enc_width);
        write(dump_ref_fd, (unsigned char *)p->ref_buf_uv[ref_id].addr, p->enc_height * p->enc_width / 2);
    }
    return ret;
}

AMVEnc_Status FastEncodeCommit(void *dev,  int IDR)
{
    fast_enc_drv_t *p = (fast_enc_drv_t *)dev;
    AMVEnc_Status ret = AMVENC_FAIL;
    int status = 0;
    if (!p)
        return ret;
    status = (IDR == 1) ? ENCODER_IDR : ENCODER_NON_IDR;
    if (ioctl(p->fd, FASTENC_AVC_IOC_SUBMIT_ENCODE_DONE , &status) == 0)
        ret = AMVENC_SUCCESS;
    return ret;
}

void UnInitFastEncode(void *dev)
{
    fast_enc_drv_t *p = (fast_enc_drv_t *)dev;
    if (!p)
        return;
    p->mCancel = 1;
    if (p->mmap_buff.addr)
        munmap(p->mmap_buff.addr , p->mmap_buff.size);
    if (dump_ref_fd < 0)
        close(dump_ref_fd);
    dump_ref_fd = -1;
#ifdef DEBUG_TIME
    printf("total_encode_frame: %d,  total_encode_time: %d ms \n", p->total_encode_frame, p->total_encode_time / 1000);
#endif
    free(p);
    return;
}

