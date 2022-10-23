//#define LOG_NDEBUG 0
#define LOG_TAG "GXFASTENCLIB"
#ifdef MAKEANDROID
#include <utils/Log.h>
#endif
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
//#include <utils/threads.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/prctl.h>

#include "gxvenclib_fast.h"
#include "enc_define.h"
#include "parser.h"
//#include <cutils/properties.h>

#define ENCODE_DONE_TIMEOUT 100

#define UCODE_MODE_FULL 0

#define START_TABLE_ID 8
#define CBR_LONG_THRESH 4

#define ME_WEIGHT_OFFSET 0x340
#define I4MB_WEIGHT_OFFSET 0x755
#define I16MB_WEIGHT_OFFSET 0x340
#define V3_IE_F_ZERO_SAD_I16 (I16MB_WEIGHT_OFFSET + 0x80)
#define V3_IE_F_ZERO_SAD_I4 (I4MB_WEIGHT_OFFSET + 0x80)
#define V3_ME_F_ZERO_SAD (ME_WEIGHT_OFFSET + 0x20)



static uint32_t qp_tbl_default[] = {
    0x00000000, 0x01010101,
    0x03030202, 0x05050404,
    0x06060606, 0x07070707,
    0x08080808, 0x09090909

};
static uint32_t qp_tbl_i4i16[] = {

    0x00000000, 0x01010101,
    0x03030202, 0x04040303,
    0x05050404, 0x06060505,
    0x07070606, 0x07070707
};

static uint32_t qp_tbl_i[] = {
    0x01010000, 0x03030202,
    0x05050404, 0x07070606,
    0x09090808, 0x0b0b0a0a,
    0x0d0d0c0c, 0x0f0f0e0e
};
static uint32_t qp_tbl_i_new[] = {
    0x00000000, 0x01010101,
    0x02020202, 0x03030303,
    0x04040404, 0x05050505,
    0x06060606, 0x07070707
};

static uint32_t qp_tbl_none_cbr_once[] = {
    0x00000000, 0x01010101,
    0x02020202, 0x03030303,
    0x04040404, 0x05050505,
    0x06060606, 0x07070707
};

static uint32_t qp_step_i_table[] = {
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x01010101, 0x00000000,
    0x00000000, 0x00000000,
    0x01010101, 0x00000000,
    0x00000000, 0x00000000,
    0x01010101, 0x00000000,
    0x00000000, 0x00000000,
};
static uint32_t qp_step_i_table_bigmotion[] = {
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x01010101, 0x00000000,
    0x01010101, 0x00000000,
    0x01010101, 0x00000000,
    0x01010101, 0x01010101,
    0x01010101, 0x01010101,
    0x01010101, 0x01010101,
};
static uint32_t  qp_step_p_table[] = {
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x01010101, 0x00000000,
    0x00000000, 0x00000000,
    0x01010101, 0x00000000,
    0x00000000, 0x00000000,
    0x01010101, 0x00000000,
    0x00000000, 0x00000000,
};
static uint32_t  qp_step_p_table_bigmotion[] = {
    0x00000000, 0x01010101,
    0x00000000, 0x01010101,
    0x00000000, 0x01010101,
    0x01010101, 0x01010101,
    0x01010101, 0x01010101,
    0x01010101, 0x01010101,
    0x01010101, 0x01010101,
    0x01010101, 0x01010101,
};

static int encode_poll(int fd, int timeout) {
    struct pollfd poll_fd[1];
    poll_fd[0].fd = fd;
    poll_fd[0].events = POLLIN | POLLERR;
    return poll(poll_fd, 1, timeout);
}

static void rgb32to24(const uint8_t *src, uint8_t *dst, int src_size) {
    int i;
    int num_pixels = src_size >> 2;
    for (i = 0; i < num_pixels; i++) {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        src++;
    }
}

static int RGBA32_To_RGB24Canvas(gx_fast_enc_drv_t* p) {
    unsigned char* src = NULL;
    unsigned char* dst = NULL;
    bool crop_flag = false;
    uint32_t offset = 0;
    int bytes_per_line = p->src.pix_width * 4;
    int canvas_w = (((p->src.pix_width * 3) + 31) >> 5) << 5;
    int mb_h = p->src.mb_height << 4;
    int i;
    if (p->src.pix_height < (p->src.mb_height << 4))
        crop_flag = true;
    src = (unsigned char*) p->src.plane[0];
    dst = p->input_buf.addr;
    if (canvas_w != (p->src.pix_width * 3)) {
        for (i = 0; i < p->src.pix_height; i++) {
            rgb32to24(src, dst, bytes_per_line);
            dst += canvas_w;
            src += bytes_per_line;
        }
    } else {
        rgb32to24(src, dst, bytes_per_line * p->src.pix_height);
    }
    offset = p->src.pix_height * canvas_w;

    if (crop_flag)
        memset(p->input_buf.addr + offset, 0,
               (mb_h - p->src.pix_height) * canvas_w);

    return canvas_w * mb_h;
}

static int RGB_To_RGBCanvas(gx_fast_enc_drv_t* p, int bpp) {
    unsigned char* src = NULL;
    unsigned char* dst = NULL;
    bool crop_flag = false;
    uint32_t offset = 0;
    int bytes_per_line = p->src.pix_width * bpp;
    int canvas_w = ((bytes_per_line + 31) >> 5) << 5;
    int mb_h = p->src.mb_height << 4;
    int i;
    if (p->src.pix_height < (p->src.mb_height << 4))
        crop_flag = true;
    src = (unsigned char*) p->src.plane[0];
    dst = p->input_buf.addr;
    if (bytes_per_line != canvas_w) {
        for (i = 0; i < p->src.pix_height; i++) {
            memcpy(dst, src, bytes_per_line);
            dst += canvas_w;
            src += bytes_per_line;
        }
    } else {
        memcpy(dst, src, bytes_per_line * p->src.pix_height);
    }
    offset = p->src.pix_height * canvas_w;

    if (crop_flag)
        memset(p->input_buf.addr + offset, 0,
               (mb_h - p->src.pix_height) * canvas_w);

    return canvas_w * mb_h;
}

static uint32_t copy_to_local(gx_fast_enc_drv_t* p, int src_w) {
    bool crop_flag = false;
    uint32_t offset = 0;
    int canvas_w = 0;
    int i = 0;
    uint32_t total_size = 0;
    unsigned char* src = NULL;
    unsigned char* dst = NULL;
    if (p->src.pix_height < (p->src.mb_height << 4))
        crop_flag = true;

    if (p->src.fmt != AMVENC_YUV420)
        canvas_w = ((p->src.pix_width + 31) >> 5) << 5;
    else
        canvas_w = ((p->src.pix_width + 63) >> 6) << 6;

    src = (unsigned char*) p->src.plane[0];
    dst = p->input_buf.addr;
    if (p->src.pix_width != canvas_w) {
        for (i = 0; i < p->src.pix_height; i++) {
            memcpy(dst, src, p->src.pix_width);
            dst += canvas_w;
            src += src_w;
        }
    } else {
        memcpy(dst, src, p->src.pix_width * p->src.pix_height);
    }
    offset = p->src.pix_height * canvas_w;

    if (crop_flag) {
        memset(p->input_buf.addr + offset, 0,
               ((p->src.mb_height << 4) - p->src.pix_height) * canvas_w);
        offset = canvas_w * p->src.mb_height << 4;
    }

    src = (unsigned char*) p->src.plane[1];
    dst = (unsigned char*) (p->input_buf.addr + offset);
    if ((p->src.fmt == AMVENC_NV12) || (p->src.fmt == AMVENC_NV21)) {
        if (p->src.pix_width != canvas_w) {
            for (i = 0; i < p->src.pix_height / 2; i++) {
                memcpy(dst, src, p->src.pix_width);
                dst += canvas_w;
                src += src_w;
            }
        } else {
            memcpy(dst, src, p->src.pix_width * p->src.pix_height / 2);
        }
        offset += p->src.pix_height * canvas_w / 2;
        if (crop_flag)
            memset(p->input_buf.addr + offset, 0x80,
                   ((p->src.mb_height << 4) - p->src.pix_height) * canvas_w / 2);
    } else if (p->src.fmt == AMVENC_YUV420) {
        if (p->src.pix_width != canvas_w) {
            for (i = 0; i < p->src.pix_height / 2; i++) {
                memcpy(dst, src, p->src.pix_width / 2);
                dst += canvas_w / 2;
                src += src_w / 2;
            }
        } else {
            memcpy(dst, src, p->src.pix_width * p->src.pix_height / 4);
        }
        offset += p->src.pix_height * canvas_w / 4;
        if (crop_flag) {
            memset(p->input_buf.addr + offset, 0x80,
                   ((p->src.mb_height << 4) - p->src.pix_height) * canvas_w / 4);
            offset = canvas_w * p->src.mb_height * 5 << 2;
        }
        src = (unsigned char*) p->src.plane[2];
        dst = (unsigned char*) (p->input_buf.addr + offset);
        if (p->src.pix_width != canvas_w) {
            for (i = 0; i < p->src.pix_height / 2; i++) {
                memcpy(dst, src, p->src.pix_width / 2);
                dst += canvas_w / 2;
                src += p->src.pix_width / 2;
            }
        } else {
            memcpy(dst, src, p->src.pix_width * p->src.pix_height / 4);
        }
        offset += p->src.pix_height * canvas_w / 4;
        if (crop_flag) {
            memset(p->input_buf.addr + offset, 0x80,
                   ((p->src.mb_height << 4) - p->src.pix_height) * canvas_w / 4);
        }
    }
    total_size = canvas_w * p->src.mb_height * 3 << 3;
    return total_size;
}

static uint32_t copy_for_scale(gx_fast_enc_drv_t *p, int src_w, int src_h) {
    uint32_t offset = 0;
    int canvas_w = 0;
    int i = 0;
    uint32_t total_size = 0;
    unsigned char* src = NULL;
    unsigned char* dst = NULL;
    int pix_height = src_h;

    if (p->src.fmt != AMVENC_YUV420)
        canvas_w = ((src_w + 31) >> 5) << 5;
    else
        canvas_w = ((src_w + 63) >> 6) << 6;

    src = (unsigned char*) p->src.plane[0];
    dst = p->input_buf.addr;
    if (src_w != canvas_w) {
        for (i = 0; i < pix_height; i++) {
            memcpy(dst, src, src_w);
            dst += canvas_w;
            src += src_w;
        }
    } else {
        memcpy(dst, src, src_w * pix_height);
    }
    offset = pix_height * canvas_w;

    src = (unsigned char*) p->src.plane[1];
    dst = (unsigned char*) (p->input_buf.addr + offset);
    if ((p->src.fmt == AMVENC_NV12) || (p->src.fmt == AMVENC_NV21)) {
        if (src_w != canvas_w) {
            for (i = 0; i < pix_height / 2; i++) {
                memcpy(dst, src, src_w);
                dst += canvas_w;
                src += src_w;
            }
        } else {
            memcpy(dst, src, src_w * pix_height / 2);
        }
        offset += pix_height * canvas_w / 2;
    } else if (p->src.fmt == AMVENC_YUV420) {
        if (src_w != canvas_w) {
            for (i = 0; i < pix_height / 2; i++) {
                memcpy(dst, src, src_w / 2);
                dst += canvas_w / 2;
                src += src_w / 2;
            }
        } else {
            memcpy(dst, src, src_w * pix_height / 4);
        }
        offset += pix_height * canvas_w / 4;
        src = (unsigned char*) p->src.plane[2];
        dst = (unsigned char*) (p->input_buf.addr + offset);
        if (src_w != canvas_w) {
            for (i = 0; i < pix_height / 2; i++) {
                memcpy(dst, src, src_w / 2);
                dst += canvas_w / 2;
                src += src_w / 2;
            }
        } else {
            memcpy(dst, src, src_w * pix_height / 4);
        }
        offset += pix_height * canvas_w / 4;
    }
    total_size = (canvas_w * pix_height * 3) >> 1;
    return total_size;
}

static int set_input(gx_fast_enc_drv_t* p,
                     ulong* yuv,
                     uint32_t enc_width,
                     uint32_t enc_height,
                     AMVEncBufferType type,
                     AMVEncFrameFmt fmt) {
    int i;
    ulong y = yuv[0];
    ulong u = yuv[1];
    ulong v = yuv[2];
    ulong crop_top = yuv[5];
    ulong crop_bottom = yuv[6];
    ulong crop_left = yuv[7];
    ulong crop_right = yuv[8];
    ulong pitch = yuv[9];
    ulong height = yuv[10];
    ulong scale_width = yuv[11];
    ulong scale_height = yuv[12];
    gx_fast_input_t *src = &p->src;

    if (!y && type != DMA_BUFF) {
        LOGAPI("invalid Buffer ptr");
        return -1;
    }

    src->pix_width = enc_width;
    src->pix_height = enc_height;
    src->mb_width = (src->pix_width + 15) >> 4;
    src->mb_height = (src->pix_height + 15) >> 4;

    src->plane[1] = 0;
    src->plane[2] = 0;
    src->crop.crop_top = 0;
    src->crop.crop_bottom = 0;
    src->crop.crop_left = 0;
    src->crop.crop_right = 0;
    src->crop.src_w = 0;
    src->crop.src_h = 0;
    p->scale_enable = false;

    if ((scale_width) && (scale_height) || (fmt == AMVENC_BGR888)) {
        p->scale_enable = true;
        src->crop.crop_top = crop_top;
        src->crop.crop_bottom = crop_bottom;
        src->crop.crop_left = crop_left;
        src->crop.crop_right = crop_right;
        src->crop.src_w = pitch;
        src->crop.src_h = height;
    }
    if (type == VMALLOC_BUFFER) {
        src->plane[0] = y;
        if ((fmt == AMVENC_NV21) || (fmt == AMVENC_NV12) || (fmt == AMVENC_YUV420))
            src->plane[1] = u;
        if (fmt == AMVENC_YUV420)
            src->plane[2] = v;
    } else if (type == DMA_BUFF) {
        src->shared_fd[0] = yuv[0];
        src->shared_fd[1] = yuv[1];
        src->shared_fd[2] = yuv[2];
        src->num_planes = yuv[13];
        LOGAPI("num_planes %d, fd %d %d %d", src->num_planes, src->shared_fd[0],src->shared_fd[1],src->shared_fd[2]);
    } else {
        src->canvas = (uint32_t) yuv[3];
    }
    src->type = type;
    if ((type == PHYSICAL_BUFF) && (pitch % 32 != 0)) {
        if ((src->plane[0] != 0) && (src->mmapsize != 0)) {
            LOGAPI("Release old mmap buffer :0x%lx, size:0x%lx. fd:%d", src->plane[0],
                   src->mmapsize, p->fd);
            munmap((void*) src->plane[0], src->mmapsize);
            src->plane[0] = 0;
            src->mmapsize = 0;
        } else if ((src->plane[0] != 0) || (src->mmapsize != 0)) {
            LOGAPI("Buffer Ptr error:0x%lx, size:0x%lx. fd:%d", src->plane[0],
                   src->mmapsize, p->fd);
        }
        src->mmapsize = pitch * height;
        if ((fmt == AMVENC_NV21) || (fmt == AMVENC_NV12) || (fmt == AMVENC_YUV420))
            src->mmapsize = src->mmapsize * 3 / 2;
        else if ((fmt == AMVENC_RGB888) || (fmt == AMVENC_BGR888))
            src->mmapsize = src->mmapsize * 3;
        else if (fmt == AMVENC_RGBA8888)
            src->mmapsize = src->mmapsize * 4;
        else
            src->mmapsize = src->mmapsize * 3 / 2;

        y = (ulong) mmap(0, src->mmapsize, PROT_READ | PROT_WRITE, MAP_SHARED, p->fd,
                         (ulong) src->canvas);
        src->plane[0] = y;
        if ((fmt == AMVENC_NV21) || (fmt == AMVENC_NV12) || (fmt == AMVENC_YUV420))
            src->plane[1] = y + pitch * height;
        if (fmt == AMVENC_YUV420)
            src->plane[2] = y + pitch * height * 5 / 4;
        src->type = VMALLOC_BUFFER;
    }
    if ((type == DMA_BUFF) && (pitch % 32 != 0)) {
        //...to do
        LOGAPI("not support %d \n", pitch);
        return -1;
    }

    src->fmt = fmt;
    if (src->type == VMALLOC_BUFFER) {
        if ((src->fmt != AMVENC_RGB888) && (src->fmt != AMVENC_RGBA8888) && (src->fmt != AMVENC_BGR888)) {
            if (p->scale_enable) {
                uint32_t canvas_w = 0;
                uint32_t canvas_h = src->mb_height << 4;
                src->framesize = copy_for_scale(p, (int) pitch, (int) height);
                canvas_w = ((scale_width + 31) >> 5) << 5;
                src->plane[0] = (ulong) p->scale_buff.addr;
                src->plane[1] = src->plane[0] + canvas_w * canvas_h;
                src->plane[2] = 0;
            } else {
                src->framesize = copy_to_local(p, pitch);
            }
        } else if (p->src.fmt == AMVENC_RGB888 || p->src.fmt == AMVENC_BGR888) {
            src->framesize = RGB_To_RGBCanvas(p, 3);
        } else if (p->src.fmt == AMVENC_RGBA8888) {
            if (p->cbr_hw) {
                src->framesize = RGB_To_RGBCanvas(p, 4);
            } else {
                src->framesize = RGBA32_To_RGB24Canvas(p);
                src->fmt = AMVENC_RGB888;
            }
        }
    } else {
        if (p->src.fmt == AMVENC_RGBA8888)
            src->framesize = src->mb_height * src->pix_width * 16 * 4; //RGBA
        else
            src->framesize = src->mb_height * src->pix_width * 24; //nv21
        if (p->scale_enable) {
            uint32_t canvas_w = 0;
            uint32_t canvas_h = src->mb_height << 4;
            canvas_w = ((scale_width + 31) >> 5) << 5;
            src->plane[0] = (ulong) p->scale_buff.addr;
            src->plane[1] = src->plane[0] + canvas_w * canvas_h;
            src->plane[2] = 0;
        }
    }
    if ((type == PHYSICAL_BUFF) && (pitch % 32 != 0)) {
        if ((src->plane[0] != 0) && (src->mmapsize != 0)) {
            munmap((void*) src->plane[0], src->mmapsize);
            src->plane[0] = 0;
            src->mmapsize = 0;
        }
    }
    return 0;
}

static void Prepare_CBR_BitsTable(gx_fast_enc_drv_t* p, bool rc)
{
    unsigned int offset, size;
    unsigned int block_size;
    unsigned int total_bits = 0;
    unsigned int i, j;
    double target;
    if (!p->cbr_hw)
        return;
    size = p->target << 5;
    block_size = p->block_width * p->block_height;
    target = (double) (size / block_size);
    for (i = 0; i < p->block_height_n; i++) {
        for (j = 0; j < p->block_width_n; j++) {
            offset = i * p->block_width_n + j;
            if (rc) {
                p->block_mb_size[offset] =
                    (uint32_t) (((unsigned long long) (target * p->block_sad_size[offset])) /p->qp_stic.f_sad_count);
                if (p->block_mb_size[offset] > 0xffff)
                    p->block_mb_size[offset] = 0xffff;
                total_bits = total_bits + (p->block_mb_size[offset] * block_size);
            } else {
                p->block_mb_size[offset] = 0xffff;
            }
        }
    }
    if (p->logtime && rc)
        LOGAPI("Prepare_CBR_BitsTable: want:%d, real:%d, diff:%d", size, total_bits, size - total_bits);
}

void smooth_tbl(uint32_t tbl[])
{
    uint8_t *max_value;
        for (int i = 0; i < 8; i++) {
            max_value = (uint8_t *) (&tbl[i]);
            max_value += 3;
            for (int j = 0; j < 4; j++) {
                if ( *max_value > 51) {
                    *max_value = 51;
                    max_value--;
                } else {
                    break;
                }
            }
            //LOGAPI("EB smooth qp %x", tbl[i]);
        }
}
static void smooth_tbl_mode(gx_fast_enc_drv_t* p, uint32_t tbl[]) {
  uint8_t* max_value;
  int qp_min, qp_max;

  if (p->qp_param == NULL)
    return;

  if (p->IDRframe) {
    qp_min = p->qp_param->qp_I_min;
    qp_max = p->qp_param->qp_I_max;
  } else {
    qp_min = p->qp_param->qp_P_min;
    qp_max = p->qp_param->qp_P_max;
  }

  for (int i = 0; i < 8; i++) {
    max_value = reinterpret_cast<uint8_t*>(&tbl[i]);
    max_value += 3;
    for (int j = 0; j < 4; j++) {
      if (*max_value > qp_max) {
        *max_value = qp_max;
      }
      if (*max_value < qp_min) {
        *max_value = qp_min;
      }
      max_value--;
    }
  }
}

static void Fill_CBR_Table(gx_fast_enc_drv_t* p, bool rc) {
    unsigned char qp =
        (p->quant > START_TABLE_ID) ? (p->quant - START_TABLE_ID) : 0;
    uint32_t qp_step;
    const uint32_t* qp_step_table;
    unsigned int i;
    unsigned char *tbl_addr;
    uint16_t *short_addr;
    uint32_t tbl[8];
    uint32_t tbl_i4i16[8];

    if (rc) {
        uint32_t qp_base =qp | qp << 8 | qp << 16 | qp << 24;
        qp_step = 0x01010101;
        for (i = 0; i < 8; i++) {
            tbl[i] = qp_base + p->qp_tbl[i];
            tbl_i4i16[i] = qp_base + qp_tbl_i4i16[i];
        }
    } else {
        memset(tbl, p->quant, sizeof(tbl));
        memset(tbl_i4i16, p->quant, sizeof(tbl_i4i16));
        qp_step = 0;
    }

    smooth_tbl_mode(p, tbl);
    smooth_tbl_mode(p, tbl_i4i16);

  // If more than 85% MB are P_SKIP, it should be a static scene,
  // use a better quality QP step. (use last frame's information)
  if (p->frame_stat_info->num_p_skip_blocks >=
      p->frame_stat_info->num_total_macro_blocks * 85 / 100) {
    if (p->IDRframe) {
      qp_step_table = qp_step_i_table;
    } else {
      qp_step_table = qp_step_p_table;
    }
  } else {
    if (p->IDRframe) {
      qp_step_table = qp_step_i_table_bigmotion;
    } else {
      qp_step_table = qp_step_p_table_bigmotion;
    }
  }
    tbl_addr = p->cbr_buff.addr;
    for (i = 0; i < 16; i++) {
        memcpy(tbl_addr, &tbl_i4i16[0], sizeof(tbl_i4i16));
        memcpy(tbl_addr+32, &tbl_i4i16[0], sizeof(tbl_i4i16));
        memcpy(tbl_addr+64, &tbl[0], sizeof(tbl));
        short_addr = (uint16_t *)(tbl_addr + 96);
        if (i >= 13) {
            *short_addr++ = I4MB_WEIGHT_OFFSET ;
            *short_addr++ = I16MB_WEIGHT_OFFSET;
            *short_addr++ = ME_WEIGHT_OFFSET;
            *short_addr++ = V3_IE_F_ZERO_SAD_I4+ ((i - 13) * 0x480);
            *short_addr++ = V3_IE_F_ZERO_SAD_I16 + ((i - 13) * 0x200);
            *short_addr++ = V3_ME_F_ZERO_SAD + ((i - 13) * 0x280);
        } else {
                *short_addr++ = I4MB_WEIGHT_OFFSET;
                *short_addr++ = I16MB_WEIGHT_OFFSET;
                *short_addr++ = ME_WEIGHT_OFFSET;
            *short_addr++ = V3_IE_F_ZERO_SAD_I4;
            *short_addr++ = V3_IE_F_ZERO_SAD_I16;
            *short_addr++ = V3_ME_F_ZERO_SAD;
        }
        *short_addr++ = 0x55aa;
        *short_addr++ = 0xaa55;
        tbl_addr += 128;

        qp_step = qp_step_table[i];
        for (int j = 0; j < 8; j++) {
            tbl[j] += qp_step;
            tbl_i4i16[j] += qp_step;
        }
        smooth_tbl_mode(p, tbl);
        smooth_tbl_mode(p, tbl_i4i16);
    }

    short_addr = (uint16_t *) (p->cbr_buff.addr + 0x800);
    for (i = 0; i < p->block_width_n * p->block_height_n; i++)
        short_addr[i] = (uint16_t) p->block_mb_size[i];
    return;
}

void gen_qp_table(gx_fast_enc_drv_t* p, uint32_t * dst, qp_table_type type) {
    if (type == curve) {
        int qp_base = p->quant | p->quant << 8 | p->quant << 16 | p->quant << 24;
        for (int i = 0; i < 8; i++) {
            *(dst + i) = qp_base + p->qp_tbl[i];
            *(dst + sizeof(qp_table_t) / 4 / 3 + i) = qp_base + p->qp_tbl[i];
            *(dst + sizeof(qp_table_t) / 4 / 3 * 2 + i) = qp_base + p->qp_tbl[i];
        }
    } else if (type == line) {
        if (p->quant < 4)
            p->quant = 4;
        for (int i = 0; i < 8; i++) {
            memset(dst + i, p->quant + (i - 4), 4);
            memset(dst + sizeof(qp_table_t) / 4 / 3 + i, p->quant + (i - 4), 4);
            memset(dst + sizeof(qp_table_t) / 4 / 3 * 2 + i, p->quant + (i - 4), 4);
        }
    }

    smooth_tbl_mode(p, dst);
    smooth_tbl_mode(p, dst + sizeof(qp_table_t) / 4 / 3);
    smooth_tbl_mode(p, dst + sizeof(qp_table_t) / 4 / 3 * 2);
}

static AMVEnc_Status start_ime_cbr(gx_fast_enc_drv_t* p,
                                   unsigned char* outptr,
                                   int* datalen) {
    AMVEnc_Status ret = AMVENC_FAIL;
    uint32_t status;
    uint32_t i;
    uint32_t result[4];
    uint32_t control_info[17 + sizeof(qp_table_t) / 4 + 11];
    uint32_t total_time = 0;
    uint32_t qp_base;
    uint32_t tbl_offset = sizeof(qp_table_t) / 4 /3;
    uint32_t info_off;

    if (p->logtime)
        gettimeofday(&p->start_test, NULL);

    p->re_encode = false;
    control_info[0] = ENCODER_NON_IDR;
    control_info[1] = UCODE_MODE_FULL;
    control_info[2] = p->src.type;
    control_info[3] = p->src.fmt;
    control_info[4] = (p->src.type == CANVAS_BUFFER) ? p->src.canvas : 0;
    control_info[5] = p->src.framesize;
    control_info[6] = p->quant;
    control_info[7] = AMVENC_FLUSH_FLAG_INPUT | AMVENC_FLUSH_FLAG_DUMP |
                      AMVENC_FLUSH_FLAG_OUTPUT | AMVENC_FLUSH_FLAG_CBR;

    info_off = 17 + tbl_offset * 3;
    if (p->make_qptl == ADJUSTED_QP_FLAG) {
        control_info[6] = ADJUSTED_QP_FLAG;
        gen_qp_table(p, &control_info[17], curve);
    } else {
        memset(&control_info[17], p->quant, sizeof(qp_table_t));
    }
    Prepare_CBR_BitsTable(p, (p->make_qptl == ADJUSTED_QP_FLAG) ? true : false);
    Fill_CBR_Table(p, (p->make_qptl == ADJUSTED_QP_FLAG) ? true : false);
    control_info[info_off++] = 0;
    control_info[info_off++] = 0;
    control_info[info_off++] = 0;
    control_info[info_off++] = p->block_width;
    control_info[info_off++] = p->block_height;
    control_info[info_off++] = CBR_LONG_THRESH;
    control_info[info_off++] = START_TABLE_ID;

    if (p->src.type == DMA_BUFF) {
      control_info[info_off++] = p->src.num_planes;
      control_info[info_off++] = static_cast<uint32>(p->src.shared_fd[0]);
      control_info[info_off++] = static_cast<uint32>(p->src.shared_fd[1]);
      control_info[info_off++] = static_cast<uint32>(p->src.shared_fd[2]);
    } else {
      control_info[info_off++] = 0;
      control_info[info_off++] = 0;
      control_info[info_off++] = 0;
      control_info[info_off++] = 0;
    }

    control_info[8] = ENCODE_DONE_TIMEOUT; // timeout op;
    control_info[9] = p->src.crop.crop_top;
    control_info[10] = p->src.crop.crop_bottom;
    control_info[11] = p->src.crop.crop_left;
    control_info[12] = p->src.crop.crop_right;
    control_info[13] = p->src.crop.src_w;
    control_info[14] = p->src.crop.src_h;
    control_info[15] = p->scale_enable ? 1 : 0;
    control_info[16] = p->nr_mode; // nr mode 0: disable 1: snr 2: tnr  2: 3dnr
    ioctl(p->fd, FASTGX_AVC_IOC_NEW_CMD, &control_info[0]);

    if (encode_poll(p->fd, -1) <= 0) {
        LOGAPI("start_ime_cbr: poll fail, fd:%d", p->fd);
        return AMVENC_TIMEOUT;
    }

    ioctl(p->fd, FASTGX_AVC_IOC_GET_STAGE, &status);
    ret = AMVENC_FAIL;
    if (status == ENCODER_IDR_DONE) {
        ioctl(p->fd, FASTGX_AVC_IOC_GET_OUTPUT_SIZE, &result[0]);
        if ((result[0] < p->output_buf.size) && (result[0] > 0)) {
            memcpy(outptr, p->output_buf.addr, result[0]);
            *datalen = result[0];
            p->me_weight = result[1];
            p->i4_weight = result[2];
            p->i16_weight = result[3];
            Parser_DumpInfo(p);
            ret = AMVENC_PICTURE_READY;
            LOGAPI("start_ime_cbr: done size: %d, fd:%d", result[0], p->fd);
        }
    } else {
        LOGAPI("start_ime_cbr: encode timeout, status:%d, fd:%d", status, p->fd);
        ret = AMVENC_TIMEOUT;
    }

    if (ret == AMVENC_PICTURE_READY) {
        p->total_encode_frame++;
        if (p->logtime) {
            gettimeofday(&p->end_test, NULL);
            total_time = p->end_test.tv_sec - p->start_test.tv_sec;
            total_time = total_time * 1000000 + p->end_test.tv_usec - p->start_test.tv_usec;
            LOGAPI("start_ime_cbr: need time: %d us, base qp: %d, frame num:%d, fd:%d", total_time, p->quant, p->total_encode_frame, p->fd);
            p->total_encode_time += total_time;
        }
    }
    return ret;
}

static AMVEnc_Status start_intra_cbr_twice(gx_fast_enc_drv_t* p,
                                           unsigned char* outptr,
                                           int* datalen) {
    AMVEnc_Status ret = AMVENC_FAIL;
    uint32_t status;
    uint32_t i;
    uint32_t result[4];
    uint32_t control_info[17 + sizeof(qp_table_t) / 4 + 11];
    uint32_t total_time = 0;
    uint32_t qp_base;
    uint32_t tbl_offset = sizeof(qp_table_t) / 4 /3;
    uint32_t info_off;

    if (p->logtime)
        gettimeofday(&p->start_test, NULL);

    p->re_encode = false;
    control_info[0] = ENCODER_IDR;
    control_info[1] = UCODE_MODE_FULL;
    control_info[2] = p->src.type;
    control_info[3] = p->src.fmt;
    control_info[4] = (p->src.type == CANVAS_BUFFER) ? p->src.canvas : 0;
    control_info[5] = p->src.framesize;
    control_info[6] = p->quant;
    control_info[7] =
        AMVENC_FLUSH_FLAG_INPUT | AMVENC_FLUSH_FLAG_DUMP | AMVENC_FLUSH_FLAG_CBR;
    info_off = 17 + tbl_offset * 3;

    if (p->make_qptl != ADJUSTED_QP_FLAG)
        control_info[7] |= AMVENC_FLUSH_FLAG_OUTPUT;

    memset(&control_info[17], p->quant, sizeof(qp_table_t));
    Prepare_CBR_BitsTable(p, false);
    Fill_CBR_Table(p, false);
    control_info[info_off++] = 0;
    control_info[info_off++] = 0;
    control_info[info_off++] = 0;
    control_info[info_off++] = p->block_width;
    control_info[info_off++] = p->block_height;
    control_info[info_off++] = CBR_LONG_THRESH;
    control_info[info_off++] = START_TABLE_ID;

    if (p->src.type == DMA_BUFF) {
      control_info[info_off++] = p->src.num_planes;
      control_info[info_off++] = static_cast<uint32>(p->src.shared_fd[0]);
      control_info[info_off++] = static_cast<uint32>(p->src.shared_fd[1]);
      control_info[info_off++] = static_cast<uint32>(p->src.shared_fd[2]);
    } else {
      control_info[info_off++] = 0;
      control_info[info_off++] = 0;
      control_info[info_off++] = 0;
      control_info[info_off++] = 0;
    }

    control_info[8] = ENCODE_DONE_TIMEOUT; // timeout op;
    control_info[9] = p->src.crop.crop_top;
    control_info[10] = p->src.crop.crop_bottom;
    control_info[11] = p->src.crop.crop_left;
    control_info[12] = p->src.crop.crop_right;
    control_info[13] = p->src.crop.src_w;
    control_info[14] = p->src.crop.src_h;
    control_info[15] = p->scale_enable ? 1 : 0;

    if (p->scale_enable)
        p->src.fmt = AMVENC_NV21;

    if (p->total_encode_frame > 0)
        control_info[16] = p->nr_mode; // nr mode 0: disable 1: snr 2: tnr  2: 3dnr
    else
        control_info[16] = (p->nr_mode > 0) ? 1 : 0;
    control_info[16] = 0;
    ioctl(p->fd, FASTGX_AVC_IOC_NEW_CMD, &control_info[0]);

    if (encode_poll(p->fd, -1) <= 0) {
        LOGAPI("start_intra_cbr_twice: poll fail, fd:%d", p->fd);
        return AMVENC_TIMEOUT;
    }

    ioctl(p->fd, FASTGX_AVC_IOC_GET_STAGE, &status);
    ret = AMVENC_FAIL;
    if (status == ENCODER_IDR_DONE) {
        ioctl(p->fd, FASTGX_AVC_IOC_GET_OUTPUT_SIZE, &result[0]);
        if ((result[0] < p->output_buf.size) && (result[0] > 0)) {
            *datalen = result[0];
            if (p->make_qptl != ADJUSTED_QP_FLAG)
                memcpy(outptr, p->output_buf.addr, result[0]);
            p->me_weight = result[1];
            p->i4_weight = result[2];
            p->i16_weight = result[3];
            Parser_DumpInfo(p);
            ret = AMVENC_NEW_IDR;
            LOGAPI("start_intra_cbr_twice: done size: %d, fd:%d", result[0], p->fd);
        }
    } else {
        LOGAPI("start_intra_cbr_twice: encode timeout, status:%d, fd:%d", status, p->fd);
        ret = AMVENC_TIMEOUT;
    }

    if (p->make_qptl == ADJUSTED_QP_FLAG && status == ENCODER_IDR_DONE) {
        info_off = 17 + tbl_offset * 3;
        control_info[6] = ADJUSTED_QP_FLAG;
        control_info[7] = AMVENC_FLUSH_FLAG_DUMP | AMVENC_FLUSH_FLAG_OUTPUT |
                          AMVENC_FLUSH_FLAG_CBR;
        gen_qp_table(p, &control_info[17], curve);
        Prepare_CBR_BitsTable(p, true);
        Fill_CBR_Table(p, true);
        control_info[info_off++] = 0;
        control_info[info_off++] = 0;
        control_info[info_off++] = 0;
        control_info[info_off++] = p->block_width;
        control_info[info_off++] = p->block_height;
        control_info[info_off++] = CBR_LONG_THRESH;
        control_info[info_off++] = START_TABLE_ID;

        if (p->src.type == DMA_BUFF) {
            control_info[info_off++] = p->src.num_planes;
            control_info[info_off++] = static_cast<uint32>(p->src.shared_fd[0]);
            control_info[info_off++] = static_cast<uint32>(p->src.shared_fd[1]);
            control_info[info_off++] = static_cast<uint32>(p->src.shared_fd[2]);
        } else {
            control_info[info_off++] = 0;
            control_info[info_off++] = 0;
            control_info[info_off++] = 0;
            control_info[info_off++] = 0;
        }

        ioctl(p->fd, FASTGX_AVC_IOC_NEW_CMD, &control_info[0]);

        if (encode_poll(p->fd, -1) <= 0) {
            LOGAPI("start_intra_cbr_twice: poll fail, fd:%d", p->fd);
            return AMVENC_TIMEOUT;
        }

        ioctl(p->fd, FASTGX_AVC_IOC_GET_STAGE, &status);
        ret = AMVENC_FAIL;
        if (status == ENCODER_IDR_DONE) {
            ioctl(p->fd, FASTGX_AVC_IOC_GET_OUTPUT_SIZE, &result[0]);
            if ((result[0] < p->output_buf.size) && (result[0] > 0)) {
                memcpy(outptr, p->output_buf.addr, result[0]);
                *datalen = result[0];
                p->me_weight = result[1];
                p->i4_weight = result[2];
                p->i16_weight = result[3];
                Parser_DumpInfo(p);
                ret = AMVENC_NEW_IDR;
                LOGAPI("start_intra_cbr_twice: done size: %d, fd:%d", result[0], p->fd);
            }
        } else {
            LOGAPI("start_intra_cbr_twice: encode timeout, status:%d, fd:%d", status, p->fd);
            ret = AMVENC_TIMEOUT;
        }
    }

    if (ret == AMVENC_NEW_IDR) {
        p->total_encode_frame++;
        if (p->logtime) {
            gettimeofday(&p->end_test, NULL);
            total_time = (p->end_test.tv_sec - p->start_test.tv_sec) * 1000000 +
                         p->end_test.tv_usec - p->start_test.tv_usec;
            p->total_encode_time += total_time;
            LOGAPI("start_intra_cbr_twice: need time: %d us, base qp: %d, frame num:%d, fd:%d", total_time, p->quant, p->total_encode_frame, p->fd);
        }
    }
    return ret;
}

static AMVEnc_Status start_intra_cbr(gx_fast_enc_drv_t* p,
                                     unsigned char* outptr,
                                     int* datalen) {
    AMVEnc_Status ret = AMVENC_FAIL;
    uint32_t status;
    uint32_t i;
    uint32_t result[4];
    uint32_t control_info[17 + sizeof(qp_table_t) / 4 + 11];
    uint32_t total_time = 0;
    uint32_t qp_base;
    uint32_t tbl_offset = sizeof(qp_table_t) / 4 /3;
    uint32_t info_off;

    if (p->logtime)
        gettimeofday(&p->start_test, NULL);

    p->re_encode = false;
    control_info[0] = ENCODER_IDR;
    control_info[1] = UCODE_MODE_FULL;
    control_info[2] = p->src.type;
    control_info[3] = p->src.fmt;
    control_info[4] = (p->src.type == CANVAS_BUFFER) ? p->src.canvas : 0;
    control_info[5] = p->src.framesize;
    control_info[6] = p->quant;
    control_info[7] = AMVENC_FLUSH_FLAG_INPUT | AMVENC_FLUSH_FLAG_DUMP;
    info_off = 17 + tbl_offset * 3;

    control_info[7] |= (AMVENC_FLUSH_FLAG_OUTPUT | AMVENC_FLUSH_FLAG_CBR);
    if (p->make_qptl == ADJUSTED_QP_FLAG) {
        gen_qp_table(p, &control_info[17], curve);
    } else {
        memset(&control_info[17], p->quant, sizeof(qp_table_t));
    }
    Prepare_CBR_BitsTable(p, (p->make_qptl == ADJUSTED_QP_FLAG) ? true : false);
    Fill_CBR_Table(p, (p->make_qptl == ADJUSTED_QP_FLAG) ? true : false);
    control_info[info_off++] = 0;
    control_info[info_off++] = 0;
    control_info[info_off++] = 0;
    control_info[info_off++] = p->block_width;
    control_info[info_off++] = p->block_height;
    control_info[info_off++] = CBR_LONG_THRESH;
    control_info[info_off++] = START_TABLE_ID;

    if (p->src.type == DMA_BUFF) {
        control_info[info_off++] = p->src.num_planes;
        control_info[info_off++] = static_cast<uint32>(p->src.shared_fd[0]);
        control_info[info_off++] = static_cast<uint32>(p->src.shared_fd[1]);
        control_info[info_off++] = static_cast<uint32>(p->src.shared_fd[2]);
    } else {
        control_info[info_off++] = 0;
        control_info[info_off++] = 0;
        control_info[info_off++] = 0;
        control_info[info_off++] = 0;
    }

    control_info[8] = ENCODE_DONE_TIMEOUT; // timeout op;
    control_info[9] = p->src.crop.crop_top;
    control_info[10] = p->src.crop.crop_bottom;
    control_info[11] = p->src.crop.crop_left;
    control_info[12] = p->src.crop.crop_right;
    control_info[13] = p->src.crop.src_w;
    control_info[14] = p->src.crop.src_h;
    control_info[15] = p->scale_enable ? 1 : 0;

    if (p->scale_enable)
        p->src.fmt = AMVENC_NV21;

    if (p->total_encode_frame > 0)
        control_info[16] = p->nr_mode; // nr mode 0: disable 1: snr 2: tnr  2: 3dnr
    else
        control_info[16] = (p->nr_mode > 0) ? 1 : 0;
    control_info[16] = 0;
    ioctl(p->fd, FASTGX_AVC_IOC_NEW_CMD, &control_info[0]);

    if (encode_poll(p->fd, -1) <= 0) {
        LOGAPI("start_intra_cbr: poll fail, fd:%d", p->fd);
        return AMVENC_TIMEOUT;
    }

    ioctl(p->fd, FASTGX_AVC_IOC_GET_STAGE, &status);
    ret = AMVENC_FAIL;
    if (status == ENCODER_IDR_DONE) {
        ioctl(p->fd, FASTGX_AVC_IOC_GET_OUTPUT_SIZE, &result[0]);
        if ((result[0] < p->output_buf.size) && (result[0] > 0)) {
            memcpy(outptr, p->output_buf.addr, result[0]);
            *datalen = result[0];
            p->me_weight = result[1];
            p->i4_weight = result[2];
            p->i16_weight = result[3];
            Parser_DumpInfo(p);
            ret = AMVENC_NEW_IDR;
            LOGAPI("start_intra_cbr: done size: %d, fd:%d", result[0], p->fd);
        }
    } else {
        LOGAPI("start_intra_cbr: encode timeout, status:%d, fd:%d", status, p->fd);
        ret = AMVENC_TIMEOUT;
    }

    if (ret == AMVENC_NEW_IDR) {
        p->total_encode_frame++;
        if (p->logtime) {
            gettimeofday(&p->end_test, NULL);
            total_time = (p->end_test.tv_sec - p->start_test.tv_sec) * 1000000 +
                          p->end_test.tv_usec - p->start_test.tv_usec;
            p->total_encode_time += total_time;

            LOGAPI(
                "start_intra_cbr: need time: %d us, base qp: %d, frame num:%d, fd:%d",
                total_time, p->quant, p->total_encode_frame, p->fd);
        }
    }
    return ret;
}

static AMVEnc_Status start_ime_one_pass(gx_fast_enc_drv_t* p,
                                        unsigned char* outptr,
                                        int* datalen) {
    AMVEnc_Status ret = AMVENC_FAIL;
    uint32_t status;
    uint32_t i;
    uint32_t result[4];
    uint32_t control_info[17 + sizeof(qp_table_t) / 4 + 7];
    uint32_t total_time = 0;
    uint32_t qp_base;
    uint32_t tbl_offset = sizeof(qp_table_t) / 4 /3;
    uint32_t info_off;

    if (p->logtime)
        gettimeofday(&p->start_test, NULL);

    p->re_encode = false;
    control_info[0] = ENCODER_NON_IDR;
    control_info[1] = UCODE_MODE_FULL;
    control_info[2] = p->src.type;
    control_info[3] = p->src.fmt;
    control_info[4] = (p->src.type == VMALLOC_BUFFER) ? 0 : p->src.canvas;
    control_info[5] = p->src.framesize;
    control_info[6] = p->quant;
    control_info[7] = AMVENC_FLUSH_FLAG_INPUT | AMVENC_FLUSH_FLAG_DUMP |
                      AMVENC_FLUSH_FLAG_OUTPUT;
    info_off = 17 + tbl_offset * 3;
    if (p->make_qptl == ADJUSTED_QP_FLAG) {
        control_info[6] = ADJUSTED_QP_FLAG;
        gen_qp_table(p, &control_info[17], line);
    }
    control_info[info_off++] = 0;
    control_info[info_off++] = 0;
    control_info[info_off++] = 0;

    control_info[8] = ENCODE_DONE_TIMEOUT; // timeout op;
    control_info[9] = p->src.crop.crop_top;
    control_info[10] = p->src.crop.crop_bottom;
    control_info[11] = p->src.crop.crop_left;
    control_info[12] = p->src.crop.crop_right;
    control_info[13] = p->src.crop.src_w;
    control_info[14] = p->src.crop.src_h;
    control_info[15] = p->scale_enable ? 1 : 0;
    control_info[16] = p->nr_mode; // nr mode 0: disable 1: snr 2: tnr  2: 3dnr
    ioctl(p->fd, FASTGX_AVC_IOC_NEW_CMD, &control_info[0]);

    if (encode_poll(p->fd, -1) <= 0) {
        LOGAPI("start_ime_one_pass: poll fail, fd:%d", p->fd);
        return AMVENC_TIMEOUT;
    }

    ioctl(p->fd, FASTGX_AVC_IOC_GET_STAGE, &status);
    ret = AMVENC_FAIL;
    if (status == ENCODER_IDR_DONE) {
        ioctl(p->fd, FASTGX_AVC_IOC_GET_OUTPUT_SIZE, &result[0]);
        if ((result[0] < p->output_buf.size) && (result[0] > 0)) {
            memcpy(outptr, p->output_buf.addr, result[0]);
            *datalen = result[0];
            p->me_weight = result[1];
            p->i4_weight = result[2];
            p->i16_weight = result[3];
            Parser_DumpInfo(p);
            ret = AMVENC_PICTURE_READY;
            LOGAPI("start_ime_one_pass: done size: %d, fd:%d", result[0], p->fd);
        }
    } else {
        LOGAPI("start_ime_one_pass: encode timeout, status:%d, fd:%d", status,
               p->fd);
        ret = AMVENC_TIMEOUT;
    }

    if (ret == AMVENC_PICTURE_READY) {
        p->total_encode_frame++;
        if (p->logtime) {
            gettimeofday(&p->end_test, NULL);
            total_time = p->end_test.tv_sec - p->start_test.tv_sec;
            total_time =
             total_time * 1000000 + p->end_test.tv_usec - p->start_test.tv_usec;
            LOGAPI("start_ime_one_pass: need time: %d us, frame num:%d, fd:%d",
                   total_time, p->total_encode_frame, p->fd);
            p->total_encode_time += total_time;
        }
    }
    return ret;
}

static AMVEnc_Status start_intra_one_pass(gx_fast_enc_drv_t* p,
                                          unsigned char* outptr,
                                          int* datalen) {
    AMVEnc_Status ret = AMVENC_FAIL;
    uint32_t status;
    uint32_t i;
    uint32_t result[4];
    uint32_t control_info[17 + sizeof(qp_table_t) / 4 + 7];
    uint32_t total_time = 0;
    uint32_t qp_base;
    uint32_t tbl_offset = sizeof(qp_table_t) / 4 /3;
    uint32_t info_off;

    if (p->logtime)
        gettimeofday(&p->start_test, NULL);

    p->re_encode = false;
    control_info[0] = ENCODER_IDR;
    control_info[1] = UCODE_MODE_FULL;
    control_info[2] = p->src.type;
    control_info[3] = p->src.fmt;
    control_info[4] = (p->src.type == VMALLOC_BUFFER) ? 0 : p->src.canvas;
    control_info[5] = p->src.framesize;
    control_info[6] = p->quant;
    control_info[7] = AMVENC_FLUSH_FLAG_INPUT | AMVENC_FLUSH_FLAG_DUMP |
                      AMVENC_FLUSH_FLAG_OUTPUT;
    info_off = 17 + tbl_offset * 3;

    control_info[7] |= AMVENC_FLUSH_FLAG_OUTPUT;
    if (p->make_qptl == ADJUSTED_QP_FLAG) {
        control_info[6] = ADJUSTED_QP_FLAG;
        gen_qp_table(p, &control_info[17], line);
    }
    control_info[info_off++] = 0;
    control_info[info_off++] = 0;
    control_info[info_off++] = 0;

    control_info[8] = ENCODE_DONE_TIMEOUT; // timeout op;
    control_info[9] = p->src.crop.crop_top;
    control_info[10] = p->src.crop.crop_bottom;
    control_info[11] = p->src.crop.crop_left;
    control_info[12] = p->src.crop.crop_right;
    control_info[13] = p->src.crop.src_w;
    control_info[14] = p->src.crop.src_h;
    control_info[15] = p->scale_enable ? 1 : 0;

    if (p->scale_enable)
        p->src.fmt = AMVENC_NV21;

    if (p->total_encode_frame > 0)
        control_info[16] = p->nr_mode; // nr mode 0: disable 1: snr 2: tnr  2: 3dnr
    else
        control_info[16] = (p->nr_mode > 0) ? 1 : 0;
    control_info[16] = 0;
    ioctl(p->fd, FASTGX_AVC_IOC_NEW_CMD, &control_info[0]);

    if (encode_poll(p->fd, -1) <= 0) {
        LOGAPI("start_intra_one_pass: poll fail, fd:%d", p->fd);
        return AMVENC_TIMEOUT;
    }

    ioctl(p->fd, FASTGX_AVC_IOC_GET_STAGE, &status);
    ret = AMVENC_FAIL;
    if (status == ENCODER_IDR_DONE) {
        ioctl(p->fd, FASTGX_AVC_IOC_GET_OUTPUT_SIZE, &result[0]);
        if ((result[0] < p->output_buf.size) && (result[0] > 0)) {
            memcpy(outptr, p->output_buf.addr, result[0]);
            *datalen = result[0];
            p->me_weight = result[1];
            p->i4_weight = result[2];
            p->i16_weight = result[3];
            Parser_DumpInfo(p);
            ret = AMVENC_NEW_IDR;
            LOGAPI("start_intra_one_pass: done size: %d, fd:%d", result[0], p->fd);
        }
    } else {
        LOGAPI("start_intra_one_pass: encode timeout, status:%d, fd:%d", status,
                p->fd);
        ret = AMVENC_TIMEOUT;
    }

    if (ret == AMVENC_NEW_IDR) {
        p->total_encode_frame++;
        if (p->logtime) {
            gettimeofday(&p->end_test, NULL);
            total_time = (p->end_test.tv_sec - p->start_test.tv_sec) * 1000000 +
                         p->end_test.tv_usec - p->start_test.tv_usec;
            p->total_encode_time += total_time;
            LOGAPI("start_intra_one_pass: need time: %d us, frame num:%d, fd:%d",
                   total_time, p->total_encode_frame, p->fd);
        }
    }
    return ret;
}

static AMVEnc_Status start_ime_two_pass(gx_fast_enc_drv_t* p,
                                        unsigned char* outptr,
                                        int* datalen) {
    AMVEnc_Status ret = AMVENC_FAIL;
    uint32_t status;
    uint32_t i;
    uint32_t result[4];
    uint32_t control_info[17 + sizeof(qp_table_t) / 4 + 7];
    uint32_t total_time = 0;
    uint32_t qp_base;
    uint32_t tbl_offset = sizeof(qp_table_t) / 4 /3;
    uint32_t info_off;

    if (p->logtime)
        gettimeofday(&p->start_test, NULL);

    p->re_encode = false;
    control_info[0] = ENCODER_NON_IDR;
    control_info[1] = UCODE_MODE_FULL;
    control_info[2] = p->src.type;
    control_info[3] = p->src.fmt;
    control_info[4] = (p->src.type == VMALLOC_BUFFER) ? 0 : p->src.canvas;
    control_info[5] = p->src.framesize;
    control_info[6] = p->quant;
    control_info[7] = AMVENC_FLUSH_FLAG_INPUT | AMVENC_FLUSH_FLAG_DUMP;

    control_info[8] = ENCODE_DONE_TIMEOUT; // timeout op;
    control_info[9] = p->src.crop.crop_top;
    control_info[10] = p->src.crop.crop_bottom;
    control_info[11] = p->src.crop.crop_left;
    control_info[12] = p->src.crop.crop_right;
    control_info[13] = p->src.crop.src_w;
    control_info[14] = p->src.crop.src_h;
    control_info[15] = p->scale_enable ? 1 : 0;
    control_info[16] = p->nr_mode; // nr mode 0: disable 1: snr 2: tnr  2: 3dnr
    ioctl(p->fd, FASTGX_AVC_IOC_NEW_CMD, &control_info[0]);

    if (encode_poll(p->fd, -1) <= 0) {
        LOGAPI("start_ime: poll fail, fd:%d", p->fd);
        return AMVENC_TIMEOUT;
    }

    ioctl(p->fd, FASTGX_AVC_IOC_GET_STAGE, &status);
    ret = AMVENC_FAIL;
    if (status == ENCODER_IDR_DONE) {
        ioctl(p->fd, FASTGX_AVC_IOC_GET_OUTPUT_SIZE, &result[0]);
        if ((result[0] < p->output_buf.size) && (result[0] > 0)) {
            *datalen = result[0];
            p->me_weight = result[1];
            p->i4_weight = result[2];
            p->i16_weight = result[3];
            Parser_DumpInfo(p);
            ret = AMVENC_PICTURE_READY;
            LOGAPI("start_ime_two_pass: done size: %d, fd:%d", result[0], p->fd);
        }
    } else {
        LOGAPI("start_ime_two_pass: encode timeout, status:%d, fd:%d", status,
               p->fd);
        ret = AMVENC_TIMEOUT;
    }

    if (status == ENCODER_IDR_DONE) {
        info_off = 17 + tbl_offset * 3;
        p->re_encode = true;
        control_info[6] = ADJUSTED_QP_FLAG;
        control_info[7] = AMVENC_FLUSH_FLAG_DUMP | AMVENC_FLUSH_FLAG_OUTPUT;
        memcpy(&control_info[17], &p->qp_stic.qp_table, sizeof(qp_table_t));
        if (((uint32_t)(p->src.mb_width * p->src.mb_height) < 3 * p->qp_stic.i_count) && (p->quant > 36)) {
            /* I4 I16 ME weight */
            if (p->need_inc_me_weight == 1) {
                control_info[info_off++] = -2000;
                control_info[info_off++] = 0x00;
                control_info[info_off++] = -784;
            } else {
                control_info[info_off++] = -2000;
                control_info[info_off++] = 0x00;
                control_info[info_off++] = -48;
            }
        } else if (p->quant > 28 && p->quant <= 36) {
            control_info[info_off++] = -1024;
            control_info[info_off++] = 0x00;
            control_info[info_off++] = 0;
        } else {
            control_info[info_off++] = -32;
            control_info[info_off++] = 0;
            control_info[info_off++] = 0;
        }
        ioctl(p->fd, FASTGX_AVC_IOC_NEW_CMD, &control_info[0]);

        if (encode_poll(p->fd, -1) <= 0) {
            LOGAPI("start_ime_two_pass: poll fail, fd:%d", p->fd);
            return AMVENC_TIMEOUT;
        }

        ioctl(p->fd, FASTGX_AVC_IOC_GET_STAGE, &status);
        ret = AMVENC_FAIL;
        if (status == ENCODER_IDR_DONE) {
            ioctl(p->fd, FASTGX_AVC_IOC_GET_OUTPUT_SIZE, &result[0]);
            if ((result[0] < p->output_buf.size) && (result[0] > 0)) {
                memcpy(outptr, p->output_buf.addr, result[0]);
                *datalen = result[0];
                p->me_weight = result[1];
                p->i4_weight = result[2];
                p->i16_weight = result[3];
                //Parser_DumpInfo(p);
                ret = AMVENC_PICTURE_READY;
                LOGAPI("start_ime_two_pass: done size: %d, fd:%d", result[0], p->fd);
            }
        } else {
            LOGAPI("start_ime_two_pass: encode timeout, status:%d, fd:%d", status,
                   p->fd);
            ret = AMVENC_TIMEOUT;
        }
    }

    if (ret == AMVENC_PICTURE_READY) {
        p->total_encode_frame++;
        if (p->logtime) {
            gettimeofday(&p->end_test, NULL);
            total_time = p->end_test.tv_sec - p->start_test.tv_sec;
            total_time =
                 total_time * 1000000 + p->end_test.tv_usec - p->start_test.tv_usec;
            LOGAPI("start_ime_two_pass: need time: %d us, frame num:%d, fd:%d",
                   total_time, p->total_encode_frame, p->fd);
            p->total_encode_time += total_time;
        }
    }
    return ret;
}

static AMVEnc_Status start_intra_two_pass(gx_fast_enc_drv_t* p,
                                          unsigned char* outptr,
                                          int* datalen) {
    AMVEnc_Status ret = AMVENC_FAIL;
    uint32_t status;
    uint32_t i;
    uint32_t result[4];
    uint32_t control_info[17 + sizeof(qp_table_t) / 4 + 7];
    uint32_t total_time = 0;
    uint32_t qp_base;
    uint32_t tbl_offset = sizeof(qp_table_t) / 4 /3;
    uint32_t info_off;

    if (p->logtime)
        gettimeofday(&p->start_test, NULL);

    p->re_encode = false;
    control_info[0] = ENCODER_IDR;
    control_info[1] = UCODE_MODE_FULL;
    control_info[2] = p->src.type;
    control_info[3] = p->src.fmt;
    control_info[4] = (p->src.type == VMALLOC_BUFFER) ? 0 : p->src.canvas;
    control_info[5] = p->src.framesize;
    control_info[6] = p->quant;
    control_info[7] = AMVENC_FLUSH_FLAG_INPUT | AMVENC_FLUSH_FLAG_DUMP;
    control_info[8] = ENCODE_DONE_TIMEOUT; // timeout op;
    control_info[9] = p->src.crop.crop_top;
    control_info[10] = p->src.crop.crop_bottom;
    control_info[11] = p->src.crop.crop_left;
    control_info[12] = p->src.crop.crop_right;
    control_info[13] = p->src.crop.src_w;
    control_info[14] = p->src.crop.src_h;
    control_info[15] = p->scale_enable ? 1 : 0;

    if (p->scale_enable)
        p->src.fmt = AMVENC_NV21;

    if (p->total_encode_frame > 0)
        control_info[16] = p->nr_mode; // nr mode 0: disable 1: snr 2: tnr  2: 3dnr
    else
        control_info[16] = (p->nr_mode > 0) ? 1 : 0;
    control_info[16] = 0;
    ioctl(p->fd, FASTGX_AVC_IOC_NEW_CMD, &control_info[0]);

    if (encode_poll(p->fd, -1) <= 0) {
        LOGAPI("start_intra_two_pass: poll fail, fd:%d", p->fd);
        return AMVENC_TIMEOUT;
    }

    ioctl(p->fd, FASTGX_AVC_IOC_GET_STAGE, &status);
    ret = AMVENC_FAIL;
    if (status == ENCODER_IDR_DONE) {
        ioctl(p->fd, FASTGX_AVC_IOC_GET_OUTPUT_SIZE, &result[0]);
        if ((result[0] < p->output_buf.size) && (result[0] > 0)) {
            *datalen = result[0];
            p->me_weight = result[1];
            p->i4_weight = result[2];
            p->i16_weight = result[3];
            Parser_DumpInfo(p);
            ret = AMVENC_NEW_IDR;
            LOGAPI("start_intra_two_pass: done size: %d, fd:%d", result[0], p->fd);
        }
    } else {
        LOGAPI("start_intra_two_pass: encode timeout, status:%d, fd:%d", status,
               p->fd);
        ret = AMVENC_TIMEOUT;
    }

    if (status == ENCODER_IDR_DONE) {
        info_off = 17 + tbl_offset * 3;
        p->re_encode = true;
        control_info[6] = ADJUSTED_QP_FLAG;
        control_info[7] = AMVENC_FLUSH_FLAG_DUMP | AMVENC_FLUSH_FLAG_OUTPUT;
        memcpy(&control_info[17], &p->qp_stic.qp_table, sizeof(qp_table_t));

        /* I4 I16 ME weight */
        control_info[info_off++] = 0;
        control_info[info_off++] = 0x400;
        control_info[info_off++] = 0;

        ioctl(p->fd, FASTGX_AVC_IOC_NEW_CMD, &control_info[0]);

        if (encode_poll(p->fd, -1) <= 0) {
            LOGAPI("start_intra_two_pass: poll fail, fd:%d", p->fd);
            return AMVENC_TIMEOUT;
        }

        ioctl(p->fd, FASTGX_AVC_IOC_GET_STAGE, &status);
        ret = AMVENC_FAIL;
        if (status == ENCODER_IDR_DONE) {
            ioctl(p->fd, FASTGX_AVC_IOC_GET_OUTPUT_SIZE, &result[0]);
            if ((result[0] < p->output_buf.size) && (result[0] > 0)) {
                memcpy(outptr, p->output_buf.addr, result[0]);
                *datalen = result[0];
                p->me_weight = result[1];
                p->i4_weight = result[2];
                p->i16_weight = result[3];
                //Parser_DumpInfo(p);
                ret = AMVENC_NEW_IDR;
                LOGAPI("start_intra_two_pass: done size: %d, fd:%d", result[0], p->fd);
            }
        } else {
            LOGAPI("start_intra_two_pass: encode timeout, status:%d, fd:%d", status,
                   p->fd);
            ret = AMVENC_TIMEOUT;
        }
    }

    if (ret == AMVENC_NEW_IDR) {
        p->total_encode_frame++;
        if (p->logtime) {
            gettimeofday(&p->end_test, NULL);
            total_time = (p->end_test.tv_sec - p->start_test.tv_sec) * 1000000 +
                         p->end_test.tv_usec - p->start_test.tv_usec;
            p->total_encode_time += total_time;
            LOGAPI("start_intra_two_pass: need time: %d us, frame num:%d, fd:%d",
                   total_time, p->total_encode_frame, p->fd);
        }
    }
    return ret;
}

static int get_block_resolution(gx_fast_enc_drv_t* p,
                                uint32_t mb_width,
                                uint32_t mb_height) {
    int block_width, block_height;
    int block_width_n, block_height_n;
    int block_height_num = (int) sqrt((double) (TARGET_BITS_TABLE_SIZE * mb_height / mb_width));
    int block_width_num = (int) sqrt((double) (TARGET_BITS_TABLE_SIZE * mb_width / mb_height));

    p->block_width = mb_width / block_width_num;
    if (mb_width % block_width_num)
        p->block_width += 1;

    p->block_height = mb_height / block_height_num;
    if (mb_height % block_height_num)
        p->block_height += 1;

    p->block_width_n = mb_width / p->block_width;
    if (mb_width  % p->block_width)
        p->block_width_n += 1;

    p->block_height_n = mb_height / p->block_height;
    if (mb_height % p->block_height)
        p->block_height_n += 1;

    LOGAPI(
        "EB block width:%d, height:%d mb_width:%d, mb_height:%d, "
        "block_width_num:%d, block_height_num:%d, block_width_n:%d, "
        "block_height_n:%d, block_width:%d, block_height:%d",
        p->enc_width, p->enc_height, mb_width, mb_height,
        block_width_num, block_height_num,
        p->block_width_n, p->block_height_n,
        p->block_width, p->block_height);
    return 0;
}

void* GxInitFastEncode(int fd, amvenc_initpara_t* init_para) {
    int addr_index = 0;
    int ret = 0;
    uint32_t buff_info[30];
    uint32_t mode = UCODE_MODE_FULL;
    gx_fast_enc_drv_t* p = NULL;
    int i = 0;

    if (!init_para) {
        LOGAPI("InitFastEncode init para error.  fd:%d", fd);
        return NULL;
    }

    p = (gx_fast_enc_drv_t*) calloc(1, sizeof(gx_fast_enc_drv_t));
    if (!p) {
        LOGAPI("InitFastEncode calloc faill. fd:%d", fd);
        return NULL;
    }

    memset(p, 0, sizeof(gx_fast_enc_drv_t));
    p->fd = fd;
    if (p->fd < 0) {
        LOGAPI("InitFastEncode open encode device fail, fd:%d", p->fd);
        free(p);
        return NULL;
    }

    memset(buff_info, 0, sizeof(buff_info));
    ret = ioctl(p->fd, FASTGX_AVC_IOC_GET_BUFFINFO,&buff_info[0]);
    if ((ret) || (buff_info[0] == 0)) {
        LOGAPI("InitFastEncode -- old venc driver. no buffer information! fd:%d",
               p->fd);
        free(p);
        return NULL;
    }

    p->mmap_buff.addr = (unsigned char*) mmap(0, buff_info[0], PROT_READ | PROT_WRITE, MAP_SHARED, p->fd, 0);
    if (p->mmap_buff.addr == MAP_FAILED) {
        LOGAPI("InitFastEncode mmap fail, fd:%d", p->fd);
        free(p);
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
    p->encode_once = init_para->encode_once;
    p->src.mbsize = p->src.mb_height * p->src.mb_width;
    p->sps_len = 0;
    p->gotSPS = false;
    p->pps_len = 0;
    p->gotPPS = false;
    p->fix_qp = -1;
    p->nr_mode = 0;
    p->cbr_hw = init_para->cbr_hw;
    if (p->cbr_hw)
        get_block_resolution(p, p->src.mb_width, p->src.mb_height);

    buff_info[0] = mode;
    buff_info[1] = p->src.mb_height;
    buff_info[2] = p->enc_width;
    buff_info[3] = p->enc_height;
    ret = ioctl(p->fd, FASTGX_AVC_IOC_CONFIG_INIT,&buff_info[0]);
    if (ret) {
        LOGAPI("InitFastEncode config init fai, fd:%dl", p->fd);
        munmap(p->mmap_buff.addr, p->mmap_buff.size);
        free(p);
        return NULL;
    }

    p->mb_info = (mb_t *) malloc(p->src.mbsize * sizeof(mb_t));
    if (p->mb_info == NULL) {
        LOGAPI("ALLOC mb info memory failed. fd:%d", p->fd);
        munmap(p->mmap_buff.addr, p->mmap_buff.size);
        free(p);
        return NULL;
    }

    p->input_buf.addr = p->mmap_buff.addr + buff_info[1];
    p->input_buf.size = buff_info[2];
    p->output_buf.addr = p->mmap_buff.addr + buff_info[3];
    p->output_buf.size = buff_info[4];
    p->scale_buff.addr = p->mmap_buff.addr + buff_info[5];
    p->scale_buff.size = buff_info[6];
    p->dump_buf.addr = p->mmap_buff.addr + buff_info[7];
    p->dump_buf.size = buff_info[8];
    p->cbr_buff.addr = p->mmap_buff.addr + buff_info[9];
    p->cbr_buff.size = buff_info[10];

    p->mCancel = false;
    p->total_encode_frame = 0;
    p->total_encode_time = 0;
    p->scale_enable = false;
    p->make_qptl = ADJUSTED_QP_FLAG;
    {
        p->logtime = false;
        p->logtime = true;

        if (init_para->rcEnable == false || p->fix_qp >= 0) {
            p->make_qptl = 0;
            p->encode_once = 1;
        }
    }
    p->qp_mode = init_para->qp_mode;
    if (p->qp_mode == 1) {
        ret = ioctl(p->fd, FASTGX_AVC_IOC_QP_MODE, &(p->qp_mode));
        if (ret)
            LOGAPI("set qp mode failed!\n");
    }
    p->qp_param = init_para->qp_param;
    p->bias = init_para->frame_stat_info->mv_hist_bias;
    p->frame_stat_info = init_para->frame_stat_info;
    return (void *) p;
}


AMVEnc_Status GxFastEncodeInitFrame(void* dev,
                                    ulong* yuv,
                                    AMVEncBufferType type,
                                    AMVEncFrameFmt fmt,
                                    bool IDRframe) {
    gx_fast_enc_drv_t* p = (gx_fast_enc_drv_t*) dev;
    AMVEnc_Status ret = AMVENC_FAIL;
    uint32_t total_time = 0;

    if ((!p) || (!yuv))
        return ret;

    if (p->logtime)
        gettimeofday(&p->start_test, NULL);

    set_input(p, yuv, p->enc_width, p->enc_height, type, fmt);

    p->IDRframe = IDRframe;
    if (p->IDRframe) {
        ret = AMVENC_NEW_IDR;
    } else {
        ret = AMVENC_SUCCESS;
    }
    if (p->logtime) {
        gettimeofday(&p->end_test, NULL);
        total_time = (p->end_test.tv_sec - p->start_test.tv_sec) * 1000000 + p->end_test.tv_usec - p->start_test.tv_usec;
        p->total_encode_time += total_time;
        LOGAPI("GxVEncodeInitFrame: need time: %d us, ret:%d, fd:%d", total_time, ret, p->fd);
    }
    return ret;
}

AMVEnc_Status GxFastEncodeSPS_PPS(void* dev,
                                  unsigned char* outptr,
                                  int* datalen) {
    gx_fast_enc_drv_t* p = (gx_fast_enc_drv_t*) dev;
    AMVEnc_Status ret = AMVENC_FAIL;
    uint32_t status;
    uint32_t result[4];
    uint32_t control_info[5];

    control_info[0] = ENCODER_SEQUENCE;
    control_info[1] = UCODE_MODE_FULL;
    control_info[2] = 26; //init qp;
    control_info[3] = AMVENC_FLUSH_FLAG_OUTPUT;
    control_info[4] = 0; // never timeout
    ioctl(p->fd, FASTGX_AVC_IOC_NEW_CMD, &control_info[0]);

    if (encode_poll(p->fd, -1) <= 0) {
        LOGAPI("sps pps: poll fail, fd:%d", p->fd);
        return AMVENC_TIMEOUT;
    }

    ioctl(p->fd, FASTGX_AVC_IOC_GET_STAGE, &status);

    LOGAPI("FastEncodeSPS_PPS status:%d, fd:%d", status, p->fd);
    ret = AMVENC_FAIL;
    if (status == ENCODER_PICTURE_DONE) {
        ioctl(p->fd, FASTGX_AVC_IOC_GET_OUTPUT_SIZE, &result[0]);
        p->sps_len = (result[0] >> 16) & 0xffff;
        p->pps_len = result[0] & 0xffff;
        if (((p->sps_len + p->pps_len) < p->output_buf.size) && (p->sps_len > 0) && (p->pps_len > 0)) {
            p->gotSPS = true;
            p->gotPPS = true;
            memcpy(outptr, p->output_buf.addr, p->pps_len + p->sps_len);
            *datalen = p->pps_len + p->sps_len;
            ret = AMVENC_SUCCESS;
        }
    } else {
        LOGAPI("sps pps timeout, status:%d, fd:%d", status, p->fd);
        ret = AMVENC_TIMEOUT;
    }
    return ret;
}

AMVEnc_Status GxFastEncodeSlice(void* dev,
                                unsigned char* outptr,
                                int* datalen) {
    gx_fast_enc_drv_t* p = (gx_fast_enc_drv_t*) dev;
    AMVEnc_Status ret = AMVENC_FAIL;
    if ((!p) || (!outptr) || (!datalen))
        return ret;

    if (p->fix_qp >= 0)
        p->quant = p->fix_qp;

    if (p->cbr_hw) {
        if (p->IDRframe) {
            p->qp_tbl = qp_tbl_i_new;
            ret = start_intra_cbr_twice(p, outptr, datalen);
        } else {
            p->qp_tbl = qp_tbl_default;
            ret = start_ime_cbr(p, outptr, datalen);
        }
    } else if (p->encode_once) {
            p->qp_tbl = qp_tbl_none_cbr_once;
        if (p->IDRframe)
            ret = start_intra_one_pass(p, outptr, datalen);
        else
            ret = start_ime_one_pass(p, outptr, datalen);
    } else {
        if (p->IDRframe)
            ret = start_intra_two_pass(p, outptr, datalen);
        else
            ret = start_ime_two_pass(p, outptr, datalen);
    }
    return ret;
}

AMVEnc_Status GxFastEncodeCommit(void* dev, bool IDR) {
    gx_fast_enc_drv_t* p = (gx_fast_enc_drv_t*) dev;
    AMVEnc_Status ret = AMVENC_FAIL;
    int status = 0;
    if (!p)
        return ret;
    status = (IDR == true) ? ENCODER_IDR : ENCODER_NON_IDR;
    if (ioctl(p->fd, FASTGX_AVC_IOC_SUBMIT_ENCODE_DONE ,&status)== 0)
        ret = AMVENC_SUCCESS;
    return ret;
}

void GxUnInitFastEncode(void* dev) {
    gx_fast_enc_drv_t* p = (gx_fast_enc_drv_t*) dev;
    if (!p)
        return;

    p->mCancel = true;

    if (p->mb_info)
        free(p->mb_info);

    if (p->mmap_buff.addr)
        munmap(p->mmap_buff.addr, p->mmap_buff.size);

    if ((p->src.plane[0] != 0) && (p->src.mmapsize != 0))
        munmap((void*) p->src.plane[0], p->src.mmapsize);

    if (p->logtime)
        LOGAPI("total_encode_frame: %d, total_encode_time: %d ms, fd:%d", p->total_encode_frame, p->total_encode_time / 1000, p->fd);
    free(p);
    return;
}

