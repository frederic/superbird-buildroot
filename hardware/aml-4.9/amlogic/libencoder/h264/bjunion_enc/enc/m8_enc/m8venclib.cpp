
//#define LOG_NDEBUG 0
#define LOG_TAG "M8VENCLIB"
#include <utils/Log.h>

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
#include <utils/threads.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "m8venclib.h"
#include "enc_define.h"
#include "intra_search_m8.h"
#include <cutils/properties.h>
#include "dump.h"
#include <sched.h>

#define ENABLE_MULTI_SLICE

#define ENCODE_DONE_TIMEOUT 150

//#define ENABLE_FULL_SEARCH
//#define INTRA_DYNAMIC_PADDING

#define QP_MIN 26
#define QP_MAX 36//36
#define INTRA_QP_MIN 26//20
#define INTRA_QP_MAX 36//32
#define HALF_PIPELINE_NUM 5
#define USE_MULTI_THREAD_FOR_DCT
#define DISABLE_CPU_AFFINITY

//#define NO_INTRA
const unsigned char p_head[] = {0x5a ,0xa5,0x55,0xaa,0x00,0x00,0x00,0x00,
								0x13,0x88,0x10,0x80,0x11,0x18,0x12,0x20,
								0x00,0x02,0x0f,0xff,0xff,0xff,0xff,0xff};
const unsigned char i_head[] = {0x5a ,0xa5,0x55,0xaa,0x00,0x00,0x00,0x00,
								0x13,0x88,0x10,0x80,0x11,0x18,0x12,0x20,
								0x00,0x02,0x09,0xff,0xff,0xff,0xff,0xff};

const double QP2QSTEP[52] = { 0.625, 0.6875, 0.8125, 0.875, 1.0, 1.125,
	                          1.25, 1.375, 1.625, 1.75, 2, 2.25,
	                          2.5, 2.75, 3.25, 3.5, 4, 4.5,
	                          5, 5.5, 6.5, 7, 8, 9,
	                          10, 11, 13, 14, 16, 18,
	                          20, 22, 26, 28, 32, 36,
	                          40, 44, 52, 56, 64, 72,
	                          80, 88, 104, 112, 128, 144,
	                          160, 176, 208, 224};//{ 0.625, 0.6875, 0.8125, 0.875, 1.0, 1.125 };


#define QP2Qstep(qp) QP2QSTEP[qp]

extern void pY_ext_asm(unsigned short *ydst, uint8_t *ysrc, uint8_t *yref, uint_t stride, int offset);
extern void NV21_pUV_ext_asm(unsigned short *dst, uint8_t *uvsrc, uint8_t *uvref, uint_t stride, int ref_x);
extern void YV12_pUV_ext_asm(unsigned short *dst, uint8_t *usrc, uint8_t *vsrc,uint8_t *uvref, uint_t stride, int offset);

extern void Y_ext_asm2(uint8_t *ydst, uint8_t *ysrc, uint_t stride);
extern void YV12_UV_ext_asm2(unsigned short *dst, uint8_t *usrc, uint8_t *vsrc, uint_t stride);
extern void NV21_UV_ext_asm2(unsigned short *dst, uint8_t *uvsrc, uint_t stride);
extern void Y_asm_direct(unsigned short *ydst, uint8_t *ysrc, uint8_t *yref, uint_t stride);
extern void YV12_pUV_ext_asm_direct(unsigned short *dst, uint8_t *usrc, uint8_t *vsrc, uint8_t *uref, uint8_t *vref, uint_t stride);
extern void NV21_pUV_ext_asm_direct(unsigned short *dst, uint8_t *uvsrc, uint8_t *uref, uint8_t *vref,  uint_t stride);

static int encode_poll(int fd, int timeout)
{
    struct pollfd poll_fd[1];
    poll_fd[0].fd = fd;
    poll_fd[0].events = POLLIN |POLLERR;
    return poll(poll_fd, 1, timeout);
}

void restructPixelWithXY_Multi_MV(m8venc_drv_t* para, unsigned char* input_addr,int mb_x,int mb_y)
{
    unsigned int stride = para->src.pix_width;
    unsigned int canvas_stride = (((stride +31)>>5)<<5);
    int mb_index = para->src.mb_width * mb_y + mb_x;
    int i, j,k;
    int cur_x,cur_y, ref_x, ref_y;
    uint8_t* ref_mb_y, *ref_mb_uv, *dst_y, *dst_u,*dst_v;
    uint8_t y_idx[] ={7, 6, 5, 4, 3, 2, 1, 0};
    uint8_t u_idx[] ={7, 7, 5, 5, 3, 3, 1, 1};
    uint8_t v_idx[] ={6, 6, 4, 4, 2, 2, 0, 0};
    int cal_x, cal_y, idx;
    int y00,y01;
    int y10,y11;
    unsigned char pixel_factor[4];
    int blkx, blky;
    int postion_by_line0[2];
    int postion_by_line1[2];

    cur_x = (mb_x<<4);
    cur_y = (mb_y<<4);
    //y
    idx = 0;
    dst_y = input_addr;
    dst_u = input_addr+256;
    dst_v = dst_u+64;

    for (j = 0; j < 4; j++) {
        blky = cur_y + (j<<2);
        blky = blky<<2; // for + mvy
        for (i =0; i<4;i++) {
            blkx = cur_x + (i<<2);
            blkx = blkx<<2; // for + mvx
            cal_x = para->mb_buffer[mb_index].me.mv_info.mv[idx].x;
            cal_y = para->mb_buffer[mb_index].me.mv_info.mv[idx].y;
            cal_x = blkx +cal_x;
            cal_y = blky +cal_y;
            if (cal_x<0) {
                cal_x = 0;
                para->mb_buffer[mb_index].me.mv_info.mv[idx].x = 0-blkx;
            }
            if (cal_x+16 >= (int)(stride<<2)) {
                cal_x = (stride<<2) -16;
                para->mb_buffer[mb_index].me.mv_info.mv[idx].x = cal_x-blkx;
            }
            if (cal_y < 0) {
                cal_y = 0;
                para->mb_buffer[mb_index].me.mv_info.mv[idx].y = 0-blky;
            }
            if (cal_y+16 >= (para->src.mb_height<<6)) {
                cal_y = (para->src.mb_height<<6) -16;
                para->mb_buffer[mb_index].me.mv_info.mv[idx].y = cal_y-blky;
            }
            ref_x = cal_x>>2;
            ref_y = cal_y>>2;
            ref_mb_y  = para->ref_info.y + (ref_y * canvas_stride);
            k = 0;
            dst_y  = input_addr + (j<<6)+(i<<2);
            dst_u  = input_addr + 256+(j<<4)+(i<<1);
            dst_v  = input_addr + 320+(j<<4)+(i<<1);
            if (cal_x&3) {
                postion_by_line0[0] = (ref_x&0xfffffff8)+y_idx[ref_x&7];
                postion_by_line0[1] = ((ref_x+1)&0xfffffff8)+y_idx[(ref_x+1)&7];
                if (cal_y&3) {
                    pixel_factor[1] =(cal_x&3)*(4-(cal_y&3));
                    pixel_factor[2] = (4-(cal_x&3))*(cal_y&3);
                    pixel_factor[3] = (cal_x&3)*(cal_y&3);
                    pixel_factor[0] = 16 - pixel_factor[3] -pixel_factor[2] -pixel_factor[1];
                } else {
                    pixel_factor[1] =(cal_x&3);
                    pixel_factor[0] = 4 - pixel_factor[1];
                }
                while (k<16) {
                    if (cal_y&3) {
                        y00 = (*(ref_mb_y+postion_by_line0[0]))*pixel_factor[0];
                        y01 = (*(ref_mb_y+postion_by_line0[1]))*pixel_factor[1];
                        y10 = (*(ref_mb_y+canvas_stride+postion_by_line0[0]))*pixel_factor[2];
                        y11 = (*(ref_mb_y+canvas_stride+postion_by_line0[1]))*pixel_factor[3];
                        dst_y[k&3] =  (y00+y01+y10+y11)>>4;
                    }else{
                        y00 = (*(ref_mb_y+postion_by_line0[0]))*pixel_factor[0];
                        y01 = (*(ref_mb_y+postion_by_line0[1]))*pixel_factor[1];
                        dst_y[k&3] =  (y00+y01)>>2;
                    }
                    k++;
                    if ((k&3) == 0) {
                        dst_y +=16;
                        ref_mb_y += canvas_stride;
                        postion_by_line0[0] = (ref_x&0xfffffff8)+y_idx[ref_x&7];
                        postion_by_line0[1] = ((ref_x+1)&0xfffffff8)+y_idx[(ref_x+1)&7];
                    }else{
                        postion_by_line0[0] = postion_by_line0[1];
                        postion_by_line0[1] = ref_x+(k&3)+1;
                        postion_by_line0[1] = (postion_by_line0[1]&0xfffffff8)+y_idx[postion_by_line0[1]&7];
                    }
                }
            }else{
                postion_by_line0[0] = (ref_x&0xfffffff8)+y_idx[ref_x&7];
                if (cal_y&3) {
                    pixel_factor[2] = (cal_y&3);
                    pixel_factor[0] = 4 - pixel_factor[2];
                }
                while (k<16) {
                    if (cal_y&3) {
                        y00 = (*(ref_mb_y+postion_by_line0[0]))*pixel_factor[0];
                        y10 = (*(ref_mb_y+canvas_stride+postion_by_line0[0]))*pixel_factor[2];
                        dst_y[k&3] =  (y00+y10)>>2;
                    }else{
                        dst_y[k&3] =  *(ref_mb_y+postion_by_line0[0]);
                    }
                    k++;
                    if ((k&3) == 0) {
                        dst_y +=16;
                        ref_mb_y += canvas_stride;
                        postion_by_line0[0] = (ref_x&0xfffffff8)+y_idx[ref_x&7];
                    }else{
                        postion_by_line0[0] = ref_x+(k&3);
                        postion_by_line0[0] = (postion_by_line0[0]&0xfffffff8)+y_idx[postion_by_line0[0]&7];
                    }
                }
            }
            //uv plane
            ref_x = cal_x>>2;
            ref_y = cal_y>>3;
            ref_mb_uv  = para->ref_info.uv + (ref_y * canvas_stride);
            k = 0;
            if (cal_x&7) {
                postion_by_line0[0] = ref_x&0xfffffffe;
                postion_by_line1[0] = (postion_by_line0[0]&0xfffffff8)+v_idx[postion_by_line0[0]&7];
                postion_by_line0[0] = (postion_by_line0[0]&0xfffffff8)+u_idx[postion_by_line0[0]&7];
                postion_by_line0[1] = (ref_x+1)&0xfffffffe;
                postion_by_line1[1] = (postion_by_line0[1]&0xfffffff8)+v_idx[postion_by_line0[1]&7];
                postion_by_line0[1] = (postion_by_line0[1]&0xfffffff8)+u_idx[postion_by_line0[1]&7];
                if (cal_y&7) {
                    pixel_factor[1] =(cal_x&7)*(8-(cal_y&7));
                    pixel_factor[2] = (8-(cal_x&7))*(cal_y&7);
                    pixel_factor[3] = (cal_x&7)*(cal_y&7);
                    pixel_factor[0] = 64 - pixel_factor[3] -pixel_factor[2] -pixel_factor[1];
                }else{
                    pixel_factor[1] =(cal_x&7);
                    pixel_factor[0] = 8 - pixel_factor[1];
                }
                while (k<4) {
                    //u
                    if (cal_y&7) {
                        y00 = (*(ref_mb_uv+postion_by_line0[0]))*pixel_factor[0];
                        y01 = (*(ref_mb_uv+postion_by_line0[1]))*pixel_factor[1];
                        y10 = (*(ref_mb_uv+canvas_stride+postion_by_line0[0]))*pixel_factor[2];
                        y11 = (*(ref_mb_uv+canvas_stride+postion_by_line0[1]))*pixel_factor[3];
                        dst_u[k&1] =  (y00+y01+y10+y11)>>6;

                        //v
                        y00 = (*(ref_mb_uv+postion_by_line1[0]))*pixel_factor[0];
                        y01 = (*(ref_mb_uv+postion_by_line1[1]))*pixel_factor[1];
                        y10 = (*(ref_mb_uv+canvas_stride+postion_by_line1[0]))*pixel_factor[2];
                        y11 = (*(ref_mb_uv+canvas_stride+postion_by_line1[1]))*pixel_factor[3];
                        dst_v[k&1] =  (y00+y01+y10+y11)>>6;
                    } else {
                        y00 = (*(ref_mb_uv+postion_by_line0[0]))*pixel_factor[0];
                        y01 = (*(ref_mb_uv+postion_by_line0[1]))*pixel_factor[1];
                        dst_u[k&1] =  (y00+y01)>>3;

                        y00 = (*(ref_mb_uv+postion_by_line1[0]))*pixel_factor[0];
                        y01 = (*(ref_mb_uv+postion_by_line1[1]))*pixel_factor[1];
                        dst_v[k&1] =  (y00+y01)>>3;
                    }
                    k++;
                    if ((k&1) == 0) {
                        dst_u +=8;
                        dst_v +=8;
                        ref_mb_uv += canvas_stride;
                        postion_by_line0[0] = (ref_x&0xfffffffe);
                        postion_by_line1[0] = (postion_by_line0[0]&0xfffffff8)+v_idx[postion_by_line0[0]&7];
                        postion_by_line0[0] = (postion_by_line0[0]&0xfffffff8)+u_idx[postion_by_line0[0]&7];
                        postion_by_line0[1] = (ref_x+1)&0xfffffffe;
                        postion_by_line1[1] = (postion_by_line0[1]&0xfffffff8)+v_idx[postion_by_line0[1]&7];
                        postion_by_line0[1] = (postion_by_line0[1]&0xfffffff8)+u_idx[postion_by_line0[1]&7];
                    }else{
                        postion_by_line0[0] = (ref_x+ ((k&1)<<1))&0xfffffffe;
                        postion_by_line1[0] = (postion_by_line0[0]&0xfffffff8)+v_idx[postion_by_line0[0]&7];
                        postion_by_line0[0] = (postion_by_line0[0]&0xfffffff8)+u_idx[postion_by_line0[0]&7];
                        postion_by_line0[0] = (ref_x+ ((k&1)<<1)+1)&0xfffffffe;
                        postion_by_line1[0] = (postion_by_line0[0]&0xfffffff8)+v_idx[postion_by_line0[0]&7];
                        postion_by_line0[0] = (postion_by_line0[0]&0xfffffff8)+u_idx[postion_by_line0[0]&7];
                    }
                }
            }else{
                postion_by_line0[0] = (ref_x&0xfffffffe);
                postion_by_line1[0] = (postion_by_line0[0]&0xfffffff8)+v_idx[postion_by_line0[0]&7];
                postion_by_line0[0] = (postion_by_line0[0]&0xfffffff8)+u_idx[postion_by_line0[0]&7];
                if (cal_y&7) {
                    pixel_factor[2] = (cal_y&7);
                    pixel_factor[0] = 8 - pixel_factor[2];
                }
                while (k<4) {
                    if (cal_y&7) {
                        y00 = (*(ref_mb_uv+postion_by_line0[0]))*pixel_factor[0];
                        y10 = (*(ref_mb_uv+canvas_stride+postion_by_line0[0]))*pixel_factor[2];
                        dst_u[k&1] =  (y00+y10)>>3;

                        y00 = (*(ref_mb_uv+postion_by_line1[0]))*pixel_factor[0];
                        y10 = (*(ref_mb_uv+canvas_stride+postion_by_line1[0]))*pixel_factor[2];
                        dst_v[k&1] =  (y00+y10)>>3;
                    }else{
                        dst_u[k&1] =  *(ref_mb_uv+postion_by_line0[0]);
                        dst_v[k&1] =  *(ref_mb_uv+postion_by_line1[0]);
                    }
                    k++;
                    if ((k&1) == 0) {
                        dst_u +=8;
                        dst_v +=8;
                        ref_mb_uv += canvas_stride;
                        postion_by_line0[0] = (ref_x&0xfffffffe);
                        postion_by_line1[0] = (postion_by_line0[0]&0xfffffff8)+v_idx[postion_by_line0[0]&7];
                        postion_by_line0[0] = (postion_by_line0[0]&0xfffffff8)+u_idx[postion_by_line0[0]&7];
                    }else{
                        postion_by_line0[0] = (ref_x+ ((k&1)<<1))&0xfffffffe;
                        postion_by_line1[0] = (postion_by_line0[0]&0xfffffff8)+v_idx[postion_by_line0[0]&7];
                        postion_by_line0[0] = (postion_by_line0[0]&0xfffffff8)+u_idx[postion_by_line0[0]&7];
                    }
                }
            }
            idx++;
        }
    }
}

void restructPixelWithXY(m8venc_drv_t* para, unsigned char* input_addr,int mb_x,int mb_y)
{
    unsigned int stride = para->src.pix_width;
    unsigned int canvas_stride = (((stride +31)>>5)<<5);
    int mb_index = para->src.mb_width * mb_y + mb_x;
    int k;
    int cal_x, cal_y, ref_x, ref_y;
    uint8_t* ref_mb_y, *ref_mb_uv, *dst_y, *dst_u,*dst_v;
    uint8_t y_idx[] ={7, 6, 5, 4, 3, 2, 1, 0};
    uint8_t u_idx[] ={7, 7, 5, 5, 3, 3, 1, 1};
    uint8_t v_idx[] ={6, 6, 4, 4, 2, 2, 0, 0};
    int y00,y01;
    int y10,y11;
    unsigned char pixel_factor[4];
    int blkx, blky;
    int postion_by_line0[2];
    int postion_by_line1[2];

    blkx = (mb_x<<6);
    blky = (mb_y<<6);
    dst_y = input_addr;
    dst_u = input_addr+256;
    dst_v = dst_u+64;

    cal_x = blkx +para->mb_buffer[mb_index].me.mv_info.average_mv.x;
    cal_y = blky +para->mb_buffer[mb_index].me.mv_info.average_mv.y;

#if 0 // has done it before in GetMvInfo
    if (cal_x<0) {
        cal_x = 0;
        para->mb_buffer[mb_index].me.mv_info.average_mv.x = 0-blkx;
    } else if (cal_x+64 >= (int)(stride<<2)) {
        cal_x = (stride<<2) -64;
        para->mb_buffer[mb_index].me.mv_info.average_mv.x = cal_x-blkx;
    }
    if (cal_y<0) {
        cal_y = 0;
        para->mb_buffer[mb_index].me.mv_info.average_mv.y = 0-blky;
    } else if (cal_y+64>=(para->src.mb_height<<6)) {
        cal_y = (para->src.mb_height<<6) -64;
        para->mb_buffer[mb_index].me.mv_info.average_mv.y = cal_y-blky;
    }
#endif
    ref_x = cal_x>>2;
    ref_y = cal_y>>2;
    ref_mb_y  = para->ref_info.y + (ref_y * canvas_stride);
    k = 0;
    dst_y  = input_addr;
    dst_u  = input_addr + 256;
    dst_v  = input_addr + 320;
    if (cal_x&3) {
        postion_by_line0[0] = (ref_x&0xfffffff8)+y_idx[ref_x&7];
        postion_by_line0[1] = ((ref_x+1)&0xfffffff8)+y_idx[(ref_x+1)&7];
        if (cal_y&3) {
            pixel_factor[1] =(cal_x&3)*(4-(cal_y&3));
            pixel_factor[2] = (4-(cal_x&3))*(cal_y&3);
            pixel_factor[3] = (cal_x&3)*(cal_y&3);
            pixel_factor[0] = 16 - pixel_factor[3] -pixel_factor[2] -pixel_factor[1];
        } else {
            pixel_factor[1] =(cal_x&3);
            pixel_factor[0] = 4 - pixel_factor[1];
        }
        while (k<256) {
            if (cal_y&3) {
                y00 = (*(ref_mb_y+postion_by_line0[0]))*pixel_factor[0];
                y01 = (*(ref_mb_y+postion_by_line0[1]))*pixel_factor[1];
                y10 = (*(ref_mb_y+canvas_stride+postion_by_line0[0]))*pixel_factor[2];
                y11 = (*(ref_mb_y+canvas_stride+postion_by_line0[1]))*pixel_factor[3];
                dst_y[k] =  (y00+y01+y10+y11)>>4;
            } else {
                y00 = (*(ref_mb_y+postion_by_line0[0]))*pixel_factor[0];
                y01 = (*(ref_mb_y+postion_by_line0[1]))*pixel_factor[1];
                dst_y[k] =  (y00+y01)>>2;
            }
            k++;
            if ((k&0xf) == 0) {
                ref_mb_y += canvas_stride;
                postion_by_line0[0] = (ref_x&0xfffffff8)+y_idx[ref_x&7];
                postion_by_line0[1] = ((ref_x+1)&0xfffffff8)+y_idx[(ref_x+1)&7];
            } else {
                postion_by_line0[0] = postion_by_line0[1];
                postion_by_line0[1] = ref_x+(k&0xf)+1;
                postion_by_line0[1] = (postion_by_line0[1]&0xfffffff8)+y_idx[postion_by_line0[1]&7];
            }
        }
    }else{
        postion_by_line0[0] = (ref_x&0xfffffff8)+y_idx[ref_x&7];
        if (cal_y&3) {
            pixel_factor[2] = (cal_y&3);
            pixel_factor[0] = 4 - pixel_factor[2];
        }
        while (k<256) {
            if (cal_y&3) {
                y00 = (*(ref_mb_y+postion_by_line0[0]))*pixel_factor[0];
                y10 = (*(ref_mb_y+canvas_stride+postion_by_line0[0]))*pixel_factor[2];
                dst_y[k] =  (y00+y10)>>2;
            }else{
                dst_y[k] =  *(ref_mb_y+postion_by_line0[0]);
            }
            k++;
            if ((k&0xf) == 0) {
                ref_mb_y += canvas_stride;
                postion_by_line0[0] = (ref_x&0xfffffff8)+y_idx[ref_x&7];
            } else {
                postion_by_line0[0] = ref_x+(k&0xf);
                postion_by_line0[0] = (postion_by_line0[0]&0xfffffff8)+y_idx[postion_by_line0[0]&7];
            }
        }
    }
    //uv plane
    ref_x = cal_x>>2;
    ref_y = cal_y>>3;
    ref_mb_uv  = para->ref_info.uv + (ref_y * canvas_stride);
    k = 0;
    if (cal_x&7) {
        postion_by_line0[0] = ref_x&0xfffffffe;
        postion_by_line1[0] = (postion_by_line0[0]&0xfffffff8)+v_idx[postion_by_line0[0]&7];
        postion_by_line0[0] = (postion_by_line0[0]&0xfffffff8)+u_idx[postion_by_line0[0]&7];
        postion_by_line0[1] = (ref_x+1)&0xfffffffe;
        postion_by_line1[1] = (postion_by_line0[1]&0xfffffff8)+v_idx[postion_by_line0[1]&7];
        postion_by_line0[1] = (postion_by_line0[1]&0xfffffff8)+u_idx[postion_by_line0[1]&7];
        if (cal_y&7) {
            pixel_factor[1] =(cal_x&7)*(8-(cal_y&7));
            pixel_factor[2] = (8-(cal_x&7))*(cal_y&7);
            pixel_factor[3] = (cal_x&7)*(cal_y&7);
            pixel_factor[0] = 64 - pixel_factor[3] -pixel_factor[2] -pixel_factor[1];
        } else {
            pixel_factor[1] =(cal_x&7);
            pixel_factor[0] = 8 - pixel_factor[1];
        }
        while (k<64) {
            if (cal_y&7) {
                //u
                y00 = (*(ref_mb_uv+postion_by_line0[0]))*pixel_factor[0];
                y01 = (*(ref_mb_uv+postion_by_line0[1]))*pixel_factor[1];
                y10 = (*(ref_mb_uv+canvas_stride+postion_by_line0[0]))*pixel_factor[2];
                y11 = (*(ref_mb_uv+canvas_stride+postion_by_line0[1]))*pixel_factor[3];
                dst_u[k] =  (y00+y01+y10+y11)>>6;
                //v
                y00 = (*(ref_mb_uv+postion_by_line1[0]))*pixel_factor[0];
                y01 = (*(ref_mb_uv+postion_by_line1[1]))*pixel_factor[1];
                y10 = (*(ref_mb_uv+canvas_stride+postion_by_line1[0]))*pixel_factor[2];
                y11 = (*(ref_mb_uv+canvas_stride+postion_by_line1[1]))*pixel_factor[3];
                dst_v[k] =  (y00+y01+y10+y11)>>6;
            }else{
                y00 = (*(ref_mb_uv+postion_by_line0[0]))*pixel_factor[0];
                y01 = (*(ref_mb_uv+postion_by_line0[1]))*pixel_factor[1];
                dst_u[k] =  (y00+y01)>>3;

                y00 = (*(ref_mb_uv+postion_by_line1[0]))*pixel_factor[0];
                y01 = (*(ref_mb_uv+postion_by_line1[1]))*pixel_factor[1];
                dst_v[k] =  (y00+y01)>>3;
            }
            k++;
            if ((k&7) == 0) {
                ref_mb_uv += canvas_stride;
                postion_by_line0[0] = (ref_x&0xfffffffe);
                postion_by_line1[0] = (postion_by_line0[0]&0xfffffff8)+v_idx[postion_by_line0[0]&7];
                postion_by_line0[0] = (postion_by_line0[0]&0xfffffff8)+u_idx[postion_by_line0[0]&7];
                postion_by_line0[1] = (ref_x+1)&0xfffffffe;
                postion_by_line1[1] = (postion_by_line0[1]&0xfffffff8)+v_idx[postion_by_line0[1]&7];
                postion_by_line0[1] = (postion_by_line0[1]&0xfffffff8)+u_idx[postion_by_line0[1]&7];
            }else{
                postion_by_line0[0] = (ref_x+ ((k&7)<<1))&0xfffffffe;
                postion_by_line1[0] = (postion_by_line0[0]&0xfffffff8)+v_idx[postion_by_line0[0]&7];
                postion_by_line0[0] = (postion_by_line0[0]&0xfffffff8)+u_idx[postion_by_line0[0]&7];
                postion_by_line0[1] = (ref_x+ ((k&7)<<1)+1)&0xfffffffe;
                postion_by_line1[1] = (postion_by_line0[1]&0xfffffff8)+v_idx[postion_by_line0[1]&7];
                postion_by_line0[1] = (postion_by_line0[1]&0xfffffff8)+u_idx[postion_by_line0[1]&7];
            }
        }
    }else{
        postion_by_line0[0] = (ref_x&0xfffffffe);
        postion_by_line1[0] = (postion_by_line0[0]&0xfffffffe)+v_idx[postion_by_line0[0]&7];
        postion_by_line0[0] = (postion_by_line0[0]&0xfffffffe)+u_idx[postion_by_line0[0]&7];
        if (cal_y&7) {
            pixel_factor[2] = (cal_y&7);
            pixel_factor[0] = 8 - pixel_factor[2];
        }
        while (k<64) {
            if (cal_y&7) {
                y00 = (*(ref_mb_uv+postion_by_line0[0]))*pixel_factor[0];
                y10 = (*(ref_mb_uv+canvas_stride+postion_by_line0[0]))*pixel_factor[2];
                dst_u[k] =  (y00+y10)>>3;

                y00 = (*(ref_mb_uv+postion_by_line1[0]))*pixel_factor[0];
                y10 = (*(ref_mb_uv+canvas_stride+postion_by_line1[0]))*pixel_factor[2];
                dst_v[k] =  (y00+y10)>>3;
            }else{
                dst_u[k] =  *(ref_mb_uv+postion_by_line0[0]);
                dst_v[k] =  *(ref_mb_uv+postion_by_line1[0]);
            }
            k++;
            if ((k&7) == 0) {
                ref_mb_uv += canvas_stride;
                postion_by_line0[0] = (ref_x&0xfffffffe);
                postion_by_line1[0] = (postion_by_line0[0]&0xfffffff8)+v_idx[postion_by_line0[0]&7];
                postion_by_line0[0] = (postion_by_line0[0]&0xfffffff8)+u_idx[postion_by_line0[0]&7];
            }else{
                postion_by_line0[0] = (ref_x+ ((k&7)<<1))&0xfffffffe;
                postion_by_line1[0] = (postion_by_line0[0]&0xfffffff8)+v_idx[postion_by_line0[0]&7];
                postion_by_line0[0] = (postion_by_line0[0]&0xfffffff8)+u_idx[postion_by_line0[0]&7];
            }
        }
    }
}

static unsigned SAD_Macroblock_0(unsigned char* cur, unsigned char* ref, unsigned stride)
{
    unsigned total = 0;
    unsigned cur_stride  = stride;
    unsigned ref_stride = (((stride+31)>>5)<<5);

    asm volatile(
        "mov r5, #0                                           \n\t"
        "vdup.32 q2, r5                                      \n\t"
        "mov r5, #15                                         \n\t"
        "0:                                                        \n\t"
        "vld1.8 {q0}, [%[cur]], %[cur_stride]      \n\t"
        "vld1.8 {q1}, [%[ref]], %[ref_stride]       \n\t"
        "vrev64.8 q1,q1                                     \n\t"
        "vabd.u8     q0, q0, q1                            \n\t"
        "vpaddl.u8  q0, q0                                  \n\t"
        "vadd.u16   q2, q2, q0                            \n\t"
        "subs r5, #1                                          \n\t"
        "bpl 0b                                                  \n\t"
        "vaddl.u16   q2, d4, d5                           \n\t"
        "vadd.u32    d4, d4, d5                           \n\t"
        "vpaddl.u32  d4, d4                                \n\t"
        "vmov.32 %[total], d4[0]                        \n\t"

        : [cur] "+r" (cur), [ref] "+r" (ref) ,[total] "+r" (total)
        : [cur_stride] "r" (cur_stride), [ref_stride] "r" (ref_stride)
        : "cc", "memory", "q0", "q1", "q2", "r5"
    );
    return total;
}

static unsigned SAD_Macroblock_1(unsigned char* cur, unsigned char* ref, unsigned stride)
{
    unsigned total = 0;
    unsigned cur_stride  = stride;
    unsigned ref_stride = (((stride+31)>>5)<<5)-16;

    asm volatile(
        "mov r5, #0                                           \n\t"
        "vdup.32 q3, r5                                      \n\t"
        "mov r5, #15                                         \n\t"
        "0:                                                        \n\t"
        "vld1.8 {q0}, [%[cur]], %[cur_stride]      \n\t"
        "vld1.8 {q1}, [%[ref]]!                          \n\t"
        "vld1.8 {d4}, [%[ref]], %[ref_stride]      \n\t"
        "vext.8 d2, d3, d2, #7                            \n\t"
        "vext.8 d3, d4, d3, #7                            \n\t"
        "vrev64.8 q1,q1                                     \n\t"
        "vabd.u8     q0, q0, q1                            \n\t"
        "vpaddl.u8  q0, q0                                  \n\t"
        "vadd.u16   q3, q3, q0                            \n\t"
        "subs r5, #1                                          \n\t"
        "bpl 0b                                                  \n\t"
        "vaddl.u16   q3, d6, d7                           \n\t"
        "vadd.u32    d6, d6, d7                           \n\t"
        "vpaddl.u32  d6, d6                                \n\t"
        "vmov.32 %[total], d6[0]                        \n\t"

        : [cur] "+r" (cur), [ref] "+r" (ref) ,[total] "+r" (total)
        : [cur_stride] "r" (cur_stride), [ref_stride] "r" (ref_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3", "r5"
    );
    return total;
}

static unsigned SAD_Macroblock_2(unsigned char* cur, unsigned char* ref, unsigned stride)
{
    unsigned total = 0;
    unsigned cur_stride  = stride;
    unsigned ref_stride = (((stride+31)>>5)<<5)-16;

    asm volatile(
        "mov r5, #0                                           \n\t"
        "vdup.32 q3, r5                                      \n\t"
        "mov r5, #15                                         \n\t"
        "0:                                                        \n\t"
        "vld1.8 {q0}, [%[cur]], %[cur_stride]      \n\t"
        "vld1.8 {q1}, [%[ref]]!                          \n\t"
        "vld1.8 {d4}, [%[ref]], %[ref_stride]      \n\t"
        "vext.8 d2, d3, d2, #6                            \n\t"
        "vext.8 d3, d4, d3, #6                            \n\t"
        "vrev64.8 q1,q1                                     \n\t"
        "vabd.u8     q0, q0, q1                            \n\t"
        "vpaddl.u8  q0, q0                                  \n\t"
        "vadd.u16   q3, q3, q0                            \n\t"
        "subs r5, #1                                          \n\t"
        "bpl 0b                                                  \n\t"
        "vaddl.u16   q3, d6, d7                           \n\t"
        "vadd.u32    d6, d6, d7                           \n\t"
        "vpaddl.u32  d6, d6                                \n\t"
        "vmov.32 %[total], d6[0]                        \n\t"

        : [cur] "+r" (cur), [ref] "+r" (ref) ,[total] "+r" (total)
        : [cur_stride] "r" (cur_stride), [ref_stride] "r" (ref_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3", "r5"
    );
    return total;
}

static unsigned SAD_Macroblock_3(unsigned char* cur, unsigned char* ref, unsigned stride)
{
    unsigned total = 0;
    unsigned cur_stride  = stride;
    unsigned ref_stride = (((stride+31)>>5)<<5)-16;

    asm volatile(
        "mov r5, #0                                           \n\t"
        "vdup.32 q3, r5                                      \n\t"
        "mov r5, #15                                         \n\t"
        "0:                                                        \n\t"
        "vld1.8 {q0}, [%[cur]], %[cur_stride]      \n\t"
        "vld1.8 {q1}, [%[ref]]!                          \n\t"
        "vld1.8 {d4}, [%[ref]], %[ref_stride]      \n\t"
        "vext.8 d2, d3, d2, #5                            \n\t"
        "vext.8 d3, d4, d3, #5                            \n\t"
        "vrev64.8 q1,q1                                     \n\t"
        "vabd.u8     q0, q0, q1                            \n\t"
        "vpaddl.u8  q0, q0                                  \n\t"
        "vadd.u16   q3, q3, q0                            \n\t"
        "subs r5, #1                                          \n\t"
        "bpl 0b                                                  \n\t"
        "vaddl.u16   q3, d6, d7                           \n\t"
        "vadd.u32    d6, d6, d7                           \n\t"
        "vpaddl.u32  d6, d6                                \n\t"
        "vmov.32 %[total], d6[0]                        \n\t"

        : [cur] "+r" (cur), [ref] "+r" (ref) ,[total] "+r" (total)
        : [cur_stride] "r" (cur_stride), [ref_stride] "r" (ref_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3", "r5"
    );
    return total;
}

static unsigned SAD_Macroblock_4(unsigned char* cur, unsigned char* ref, unsigned stride)
{
    unsigned total = 0;
    unsigned cur_stride  = stride;
    unsigned ref_stride = (((stride+31)>>5)<<5)-16;

    asm volatile(
        "mov r5, #0                                           \n\t"
        "vdup.32 q3, r5                                      \n\t"
        "mov r5, #15                                         \n\t"
        "0:                                                        \n\t"
        "vld1.8 {q0}, [%[cur]], %[cur_stride]      \n\t"
        "vld1.8 {q1}, [%[ref]]!                          \n\t"
        "vld1.8 {d4}, [%[ref]], %[ref_stride]      \n\t"
        "vext.8 d2, d3, d2, #4                            \n\t"
        "vext.8 d3, d4, d3, #4                            \n\t"
        "vrev64.8 q1,q1                                     \n\t"
        "vabd.u8     q0, q0, q1                            \n\t"
        "vpaddl.u8  q0, q0                                  \n\t"
        "vadd.u16   q3, q3, q0                            \n\t"
        "subs r5, #1                                          \n\t"
        "bpl 0b                                                  \n\t"
        "vaddl.u16   q3, d6, d7                           \n\t"
        "vadd.u32    d6, d6, d7                           \n\t"
        "vpaddl.u32  d6, d6                                \n\t"
        "vmov.32 %[total], d6[0]                        \n\t"

        : [cur] "+r" (cur), [ref] "+r" (ref) ,[total] "+r" (total)
        : [cur_stride] "r" (cur_stride), [ref_stride] "r" (ref_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3", "r5"
    );
    return total;
}

static unsigned SAD_Macroblock_5(unsigned char* cur, unsigned char* ref, unsigned stride)
{
    unsigned total = 0;
    unsigned cur_stride  = stride;
    unsigned ref_stride = (((stride+31)>>5)<<5)-16;

    asm volatile(
        "mov r5, #0                                           \n\t"
        "vdup.32 q3, r5                                      \n\t"
        "mov r5, #15                                         \n\t"
        "0:                                                        \n\t"
        "vld1.8 {q0}, [%[cur]], %[cur_stride]      \n\t"
        "vld1.8 {q1}, [%[ref]]!                          \n\t"
        "vld1.8 {d4}, [%[ref]], %[ref_stride]      \n\t"
        "vext.8 d2, d3, d2, #3                            \n\t"
        "vext.8 d3, d4, d3, #3                            \n\t"
        "vrev64.8 q1,q1                                     \n\t"
        "vabd.u8     q0, q0, q1                            \n\t"
        "vpaddl.u8  q0, q0                                  \n\t"
        "vadd.u16   q3, q3, q0                            \n\t"
        "subs r5, #1                                          \n\t"
        "bpl 0b                                                  \n\t"
        "vaddl.u16   q3, d6, d7                           \n\t"
        "vadd.u32    d6, d6, d7                           \n\t"
        "vpaddl.u32  d6, d6                                \n\t"
        "vmov.32 %[total], d6[0]                        \n\t"

        : [cur] "+r" (cur), [ref] "+r" (ref) ,[total] "+r" (total)
        : [cur_stride] "r" (cur_stride), [ref_stride] "r" (ref_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3", "r5"
    );
    return total;
}

static unsigned SAD_Macroblock_6(unsigned char* cur, unsigned char* ref, unsigned stride)
{
    unsigned total = 0;
    unsigned cur_stride  = stride;
    unsigned ref_stride = (((stride+31)>>5)<<5)-16;

    asm volatile(
        "mov r5, #0                                           \n\t"
        "vdup.32 q3, r5                                      \n\t"
        "mov r5, #15                                         \n\t"
        "0:                                                        \n\t"
        "vld1.8 {q0}, [%[cur]], %[cur_stride]      \n\t"
        "vld1.8 {q1}, [%[ref]]!                          \n\t"
        "vld1.8 {d4}, [%[ref]], %[ref_stride]      \n\t"
        "vext.8 d2, d3, d2, #2                            \n\t"
        "vext.8 d3, d4, d3, #2                            \n\t"
        "vrev64.8 q1,q1                                     \n\t"
        "vabd.u8     q0, q0, q1                            \n\t"
        "vpaddl.u8  q0, q0                                  \n\t"
        "vadd.u16   q3, q3, q0                            \n\t"
        "subs r5, #1                                          \n\t"
        "bpl 0b                                                  \n\t"
        "vaddl.u16   q3, d6, d7                           \n\t"
        "vadd.u32    d6, d6, d7                           \n\t"
        "vpaddl.u32  d6, d6                                \n\t"
        "vmov.32 %[total], d6[0]                        \n\t"

        : [cur] "+r" (cur), [ref] "+r" (ref) ,[total] "+r" (total)
        : [cur_stride] "r" (cur_stride), [ref_stride] "r" (ref_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3", "r5"
    );
    return total;
}

static unsigned SAD_Macroblock_7(unsigned char* cur, unsigned char* ref, unsigned stride)
{
    unsigned total = 0;
    unsigned cur_stride  = stride;
    unsigned ref_stride = (((stride+31)>>5)<<5)-16;

    asm volatile(
        "mov r5, #0                                           \n\t"
        "vdup.32 q3, r5                                      \n\t"
        "mov r5, #15                                         \n\t"
        "0:                                                        \n\t"
        "vld1.8 {q0}, [%[cur]], %[cur_stride]      \n\t"
        "vld1.8 {q1}, [%[ref]]!                          \n\t"
        "vld1.8 {d4}, [%[ref]], %[ref_stride]      \n\t"
        "vext.8 d2, d3, d2, #1                            \n\t"
        "vext.8 d3, d4, d3, #1                            \n\t"
        "vrev64.8 q1,q1                                     \n\t"
        "vabd.u8     q0, q0, q1                            \n\t"
        "vpaddl.u8  q0, q0                                  \n\t"
        "vadd.u16   q3, q3, q0                            \n\t"
        "subs r5, #1                                          \n\t"
        "bpl 0b                                                  \n\t"
        "vaddl.u16   q3, d6, d7                           \n\t"
        "vadd.u32    d6, d6, d7                           \n\t"
        "vpaddl.u32  d6, d6                                \n\t"
        "vmov.32 %[total], d6[0]                        \n\t"

        : [cur] "+r" (cur), [ref] "+r" (ref) ,[total] "+r" (total)
        : [cur_stride] "r" (cur_stride), [ref_stride] "r" (ref_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3", "r5"
    );
    return total;
}

static unsigned SAD_Macroblock(unsigned char* cur, unsigned char* ref, unsigned stride  ,unsigned char i_off, unsigned dmin)
{
    unsigned total = 0x7fffffff;
    if (i_off == 1)
        total = SAD_Macroblock_1(cur, ref, stride);
    else if (i_off == 2)
        total = SAD_Macroblock_2(cur, ref, stride);
    else if (i_off == 3)
        total = SAD_Macroblock_3(cur, ref, stride);
    else if (i_off == 4)
        total = SAD_Macroblock_4(cur, ref, stride);
    else if (i_off == 5)
        total = SAD_Macroblock_5(cur, ref, stride);
    else if (i_off == 6)
        total = SAD_Macroblock_6(cur, ref, stride);
    else if (i_off == 7)
        total = SAD_Macroblock_7(cur, ref, stride);
    else if (i_off == 0)
        total = SAD_Macroblock_0(cur, ref, stride);
    return total;
}

static int cal_sad(m8venc_drv_t* p, int mb_x, int mb_y,int mvx, int mvy)
{
    uint8_t *cand = NULL;
    uint8_t* cur = NULL;
    int dmin,i,j;
    int i0 = mb_x<<4; /* current position */
    int j0 = mb_y<<4;
    int i_temp = (i0>>3)<<3;
    int lx = p->src.pix_width;
    int i_off = i0 - i_temp;
    int mb_y_offset =0;
    i = i0 + mvx;
    j = j0 + mvy;
    i_temp = (i>>3)<<3;
    i_off = i - i_temp;
    mb_y_offset = (mb_x<<4) + (mb_y<<4) * lx;
    cur = (uint8_t*)p->src.plane[0] + mb_y_offset;
    cand  = p->ref_info.y + i_temp + j * (((lx+31)>>5)<<5);
    dmin = 65535;
    dmin = SAD_Macroblock(cur, cand, lx, i_off,dmin);
    return dmin;
}

inline void fill_i_buffer_spec_nv21(m8venc_drv_t* para, unsigned char* input_addr,int mb_x,int mb_y,int mb_index)
{
    unsigned int stride = para->src.pix_width;
    unsigned char*  p = input_addr;
    unsigned short* q =  (unsigned short*)input_addr;
    uint8_t  *cur_mb_y, *cur_mb_uv;
    unsigned mb_y_offset, mb_uv_offset;
    int* i4mode = (int*)input_addr;

    if ((para->src.type == CANVAS_BUFFER) || (para->scale_enable))
        stride = ((para->src.pix_width+31)>>5)<<5;

    memcpy(p , (char*)i_head ,24 );
    p[18] = para->mb_buffer[mb_index].ie.imode;
    if (p[18] == HENC_MB_Type_I16MB) {
        p[17] = para->mb_buffer[mb_index].ie.i16mode;
    } else {
        i4mode[2] = para->mb_buffer[mb_index].ie.i4mode[0];
        i4mode[3] = para->mb_buffer[mb_index].ie.i4mode[1];
    }
    p[16] = para->mb_buffer[mb_index].ie.pred_mode_c;

    if (para->force_skip) {
         i4mode[2] = 0x22222222;
         i4mode[3] = 0x22222222;
         p[16] = 0;
    }
    p+= 24;
    memset(p,0,72);

    q[2] = mb_x;
    q[3] = mb_y;
    q = (unsigned short*)(input_addr + 96);
    mb_y_offset = (mb_x<<4) + (mb_y<<4) * stride ;
    mb_uv_offset = (mb_x<<4) + (mb_y<<3) * stride ;
    if (para->force_skip) {
        cur_mb_y = (unsigned char *)para->ref_info.y + mb_y_offset;
        cur_mb_uv =  (unsigned char *)para->ref_info.uv + mb_uv_offset;
    } else {
        cur_mb_y = (uint8_t*)para->src.plane[0] + mb_y_offset;
        cur_mb_uv = (uint8_t*)para->src.plane[1] + mb_uv_offset;
    }
    Y_ext_asm2((unsigned char*)q, cur_mb_y, stride);
    NV21_UV_ext_asm2( q, cur_mb_uv, stride);
}

inline void fill_i_buffer_spec_yv12(m8venc_drv_t* para, unsigned char* input_addr,int mb_x,int mb_y, int mb_index)
{
    unsigned int stride = para->src.pix_width;
    unsigned char*  p = input_addr;
    unsigned short* q =  (unsigned short*)input_addr;
    uint8_t  *cur_mb_y, *cur_mb_u, *cur_mb_v;
    unsigned mb_y_offset, mb_u_offset, mb_v_offset;
    int* i4mode = (int*)input_addr;

    if (para->src.type == CANVAS_BUFFER)
        stride = ((para->src.pix_width+63)>>6)<<6;

    memcpy(p , (char*)i_head ,24 );
    p[18] = para->mb_buffer[mb_index].ie.imode;
    if (p[18] == HENC_MB_Type_I16MB) {
        p[17] = para->mb_buffer[mb_index].ie.i16mode;
    } else {
        i4mode[2] = para->mb_buffer[mb_index].ie.i4mode[0];
        i4mode[3] = para->mb_buffer[mb_index].ie.i4mode[1];
    }
    p[16] = para->mb_buffer[mb_index].ie.pred_mode_c;

    if (para->force_skip) {
         i4mode[2] = 0x22222222;
         i4mode[3] = 0x22222222;
         p[16] = 0;
    }
    p+= 24;
    memset(p,0,72);

    q[2] = mb_x;
    q[3] = mb_y;
    q = (unsigned short*)(input_addr + 96);

    if (para->force_skip) {
        mb_y_offset = (mb_x<<4) + (mb_y<<4) * stride ;
        mb_u_offset = (mb_x<<4) + (mb_y<<3) * stride ;
        cur_mb_y = (unsigned char *)para->ref_info.y + mb_y_offset;
        cur_mb_u =  (unsigned char *)para->ref_info.uv + mb_u_offset;
        Y_ext_asm2((unsigned char*)q, cur_mb_y, stride);
        NV21_UV_ext_asm2( q, cur_mb_u, stride);
    } else {
        mb_y_offset = (mb_x<<4) + (mb_y<<4) * stride ;
        mb_u_offset = (mb_x<<3) + (mb_y<<2) * stride ;
        mb_v_offset = (mb_x<<3) + (mb_y<<2) * stride ;
        cur_mb_y = (uint8_t*)para->src.plane[0] + mb_y_offset;
        cur_mb_u = (uint8_t*)para->src.plane[1] + mb_u_offset;
        cur_mb_v = (uint8_t*)para->src.plane[2] + mb_v_offset;
        Y_ext_asm2((unsigned char*)q, cur_mb_y, stride);
        YV12_UV_ext_asm2(q, cur_mb_u, cur_mb_v, stride);
    }
}

inline void fill_p_buffer_nv21(m8venc_drv_t* para, unsigned char* input_addr,int mb_x,int mb_y, unsigned char* ref_mb, bool multi_mv)
{
    unsigned int stride = para->src.pix_width;
    unsigned char*  p = input_addr;
    unsigned short* q =  (unsigned short*)input_addr;
    uint8_t* cur_mb_y, *cur_mb_uv;
    uint8_t* ref_mb_y, *ref_mb_uv;
    unsigned mb_y_offset, mb_uv_offset;
    int mvx, mvy;
    int ref_x,ref_y;
    int i = 0;
    short *temp = NULL;
    int mb_index = para->src.mb_width * mb_y + mb_x;
    mvx = para->mb_buffer[mb_index].me.mv_info.average_ref.x*4;
    mvy = para->mb_buffer[mb_index].me.mv_info.average_ref.y*4;
    unsigned char map[16] = { 0, 1, 4, 5,
		                               2, 3, 6, 7,
		                               8, 9, 12, 13,
		                               10, 11, 14, 15};

    if ((para->src.type == CANVAS_BUFFER) || (para->scale_enable))
        stride = ((para->src.pix_width+31)>>5)<<5;
    memcpy(p , (char*)p_head ,24 );
    p[18] = 0xf;//mb_info->mb_type;
    p+= 24;
    memset(p,0,72);

    q[2] = mb_x;
    q[3] = mb_y;
    temp = (short*)q;
    temp += 16;
    if (ref_mb) {
        while (i<32) {
            if (multi_mv) {
                temp[i] = para->mb_buffer[mb_index].me.mv_info.mv[map[i>>1]].y;
                temp[i+1] = para->mb_buffer[mb_index].me.mv_info.mv[map[i>>1]].x;
            } else {
                temp[i] = para->mb_buffer[mb_index].me.mv_info.average_mv.y;
                temp[i+1] = para->mb_buffer[mb_index].me.mv_info.average_mv.x;
            }
            i+=2;
        }
    } else {
        while (i<32) {
            temp[i] = (para->force_skip == true)?0:mvy;
            temp[i+1] = (para->force_skip == true)?0:mvx;
            i+=2;
        }
    }
    ref_x = (mb_x<<4)+para->mb_buffer[mb_index].me.mv_info.average_ref.x;
    ref_y = (mb_y<<4)+para->mb_buffer[mb_index].me.mv_info.average_ref.y;

    mb_y_offset = (mb_x<<4) + (mb_y<<4) * stride ;
    mb_uv_offset = (mb_x<<4)+ + (mb_y<<3) * stride ;
    cur_mb_y = (uint8_t*)para->src.plane[0] + mb_y_offset;
    cur_mb_uv = (uint8_t*)para->src.plane[1] + mb_uv_offset;
    if (ref_mb) {
        q = (unsigned short*)(input_addr + 96);
        Y_asm_direct(q, cur_mb_y, ref_mb, stride);
        NV21_pUV_ext_asm_direct(q, cur_mb_uv, ref_mb+320, ref_mb+256, stride);
    } else {
        mb_y_offset = ref_x + ref_y * (((stride +31)>>5)<<5);
        mb_uv_offset = (ref_x&0xfffffffe)+ (ref_y/2) * (((stride +31)>>5)<<5);
        ref_mb_y  = para->ref_info.y+ mb_y_offset;
        ref_mb_uv = para->ref_info.uv + mb_uv_offset;

        q = (unsigned short*)(input_addr + 96);
        if (para->force_skip == true) {
            memset(q, 0, 768);
        } else {
            int j, sad=0, total_sad, max_sad = 0;
            short * q_temp = (short *)q;
            pY_ext_asm(q, cur_mb_y, ref_mb_y, stride, ref_x&0x07);
            //para->mb_buffer[mb_index].me.sad =  0;
            //for(j = 0; j < 256;j++)
            //   para->mb_buffer[mb_index].me.sad += AVC_ABS(q_temp[j]);
            NV21_pUV_ext_asm(q, cur_mb_uv, ref_mb_uv, stride, ref_x&0x06);
            if ((mvy == 0) && (mvx == 0)) {
                total_sad = 0;
                for (j = 256; j < 320; j++) {
                    sad = AVC_ABS(q_temp[j]);
                    total_sad += sad;
                    max_sad = AVC_MAX(q_temp[j], max_sad);
                }
                if ((sad <=48) && (max_sad <=2))
                    memset(&q_temp[256], 0, 128);
            }
            if ((mvy == 0) && (mvx == 0)) {
                total_sad = 0;
                for (j = 320; j < 384; j++) {
                    sad = AVC_ABS(q_temp[j]);
                    total_sad += sad;
                    max_sad = AVC_MAX(q_temp[j], max_sad);
                }
                if ((sad <=48) && (max_sad <=2))
                    memset(&q_temp[320], 0, 128);
            }
        }
    }
}

inline void fill_p_buffer_yv12(m8venc_drv_t* para, unsigned char* input_addr,int mb_x,int mb_y, unsigned char* ref_mb, bool multi_mv)
{
    unsigned int stride = para->src.pix_width;
    unsigned char*  p = input_addr;
    unsigned short* q =  (unsigned short*)input_addr;
    uint8_t  *cur_mb_y, *cur_mb_u, *cur_mb_v;
    unsigned mb_y_offset, mb_u_offset, mb_v_offset;
    uint8_t* ref_mb_y, *ref_mb_u,*ref_mb_v;
    short mvx, mvy;
    int ref_x,ref_y;
    int i = 0;
    short * temp = NULL;
    int mb_index = para->src.mb_width * mb_y + mb_x;
    unsigned char map[16] = { 0, 1, 4, 5,
		                               2, 3, 6, 7,
		                               8, 9, 12, 13,
		                               10, 11, 14, 15};
    mvx = para->mb_buffer[mb_index].me.mv_info.average_ref.x*4;
    mvy = para->mb_buffer[mb_index].me.mv_info.average_ref.y*4;

    if (para->src.type == CANVAS_BUFFER)
        stride = ((para->src.pix_width+63)>>6)<<6;
    memcpy(p , (char*)p_head ,24 );
    p[18] = 0xf;//mb_info->mb_type;
    p+= 24;
    memset(p,0,72);

    q[2] = mb_x;
    q[3] = mb_y;
    temp = (short*)q;
    temp += 16;
    if (ref_mb) {
        while (i<32) {
            if (multi_mv) {
                temp[i] = para->mb_buffer[mb_index].me.mv_info.mv[map[i>>1]].y;
                temp[i+1] = para->mb_buffer[mb_index].me.mv_info.mv[map[i>>1]].x;
            } else {
                temp[i] = para->mb_buffer[mb_index].me.mv_info.average_mv.y;
                temp[i+1] = para->mb_buffer[mb_index].me.mv_info.average_mv.x;
            }
            i+=2;
        }
    } else {
        while (i<32) {
            temp[i] = (para->force_skip == true)?0:mvy;
            temp[i+1] = (para->force_skip == true)?0:mvx;
            i+=2;
        }
    }
    ref_x = (mb_x<<4)+para->mb_buffer[mb_index].me.mv_info.average_ref.x;
    ref_y = (mb_y<<4)+para->mb_buffer[mb_index].me.mv_info.average_ref.y;

    mb_y_offset = (mb_x<<4) + (mb_y<<4) * stride ;
    mb_u_offset = (mb_x<<3) + (mb_y<<2) * stride ;
    mb_v_offset = (mb_x<<3) + (mb_y<<2) * stride ;
    cur_mb_y = (uint8_t*)para->src.plane[0] + mb_y_offset;
    cur_mb_u = (uint8_t*)para->src.plane[1] + mb_u_offset;
    cur_mb_v = (uint8_t*)para->src.plane[2] + mb_v_offset;
    if (ref_mb) {
        q = (unsigned short*)(input_addr + 96);
        Y_asm_direct(q, cur_mb_y, ref_mb, stride);
        YV12_pUV_ext_asm_direct(q, cur_mb_u, cur_mb_v, ref_mb+256, ref_mb+320, stride);
    } else {
        mb_y_offset = ref_x + ref_y * (((stride +31)>>5)<<5);
        mb_u_offset = (ref_x&0xfffffffe)+ (ref_y/2) * (((stride +31)>>5)<<5);
        ref_mb_y  = para->ref_info.y + mb_y_offset;
        ref_mb_u = para->ref_info.uv + mb_u_offset;

        q = (unsigned short*)(input_addr + 96);
        if (para->force_skip == true) {
            memset(q, 0, 768);
        } else {
            int j, sad=0, total_sad, max_sad = 0;
            short * q_temp = (short *)q;
            pY_ext_asm(q, cur_mb_y, ref_mb_y, stride, ref_x&0x07);
            //para->mb_buffer[mb_index].me.sad =  0;
            //for(j = 0; j < 256;j++)
            //    para->mb_buffer[mb_index].me.sad += AVC_ABS(q_temp[j]);
            YV12_pUV_ext_asm(q, cur_mb_u, cur_mb_v, ref_mb_u, stride, ref_x & 0x06);
            if ((mvy == 0) && (mvx == 0)) {
                total_sad = 0;
                for (j = 256; j < 320; j++) {
                    sad = AVC_ABS(q_temp[j]);
                    total_sad += sad;
                    max_sad = AVC_MAX(q_temp[j], max_sad);
                }
                if ((sad <=48) && (max_sad <=2))
                    memset(&q_temp[256], 0, 128);
            }
            if ((mvy == 0) && (mvx == 0)) {
                total_sad = 0;
                for (j = 320; j < 384; j++) {
                    sad = AVC_ABS(q_temp[j]);
                    total_sad += sad;
                    max_sad = AVC_MAX(q_temp[j], max_sad);
                }
                if ((sad <=48) && (max_sad <=2))
                    memset(&q_temp[320], 0, 128);
            }
        }
    }
}

static bool intra_mb_around(m8venc_drv_t* p, int mbx, int mby, int mb_index)
{
    bool ret = false;
    if (mbx>0) {
        ret |= (p->mb_buffer[mb_index-1].intra_mode == 1)?true:false;
    }
    if (mby > 0) {
        ret |= (p->mb_buffer[mb_index - p->src.mb_width].intra_mode == 1)?true:false;
        ret |= (p->mb_buffer[mb_index - p->src.mb_width+1].intra_mode == 1)?true:false;
        if (mbx>0) {
            ret |= (p->mb_buffer[mb_index - p->src.mb_width-1].intra_mode == 1)?true:false;
        }
    }
    return ret;
}

#ifndef USE_MULTI_THREAD_FOR_DCT
static int fill_P_buffer(m8venc_drv_t* p)
{
    int mb_y,mb_x;
    uint8_t  *input_mb, *tmp;
    int mb_stride = p->src.mb_width;
    int total_bytes = 0;
    int insert_mb = 0;
    int index = 0;
    int k = 0;
    int flush_line = p->src.mb_height/HALF_PIPELINE_NUM;
    int flush_start = 0;
    total_bytes = 0;
    int pre_intra_mode = 0;
    unsigned addr_info[4];
    for (mb_y = 0; mb_y <= p->src.mb_height-1; ) {
        for (mb_x = 0; mb_x <= p->src.mb_width-1; ) {
            input_mb = p->input_buf.addr + ((mb_y*mb_stride+insert_mb+mb_x)*864);
            if (p->mb_buffer[index].intra_mode) {
                if ((p->src.fmt  == AMVENC_NV21) || (p->src.fmt == AMVENC_NV12))
                    fill_i_buffer_spec_nv21(p, input_mb, mb_x, mb_y,index); // or can use HENC_MB_Type_I4MB
                else
                    fill_i_buffer_spec_yv12(p, input_mb, mb_x, mb_y,index); // or can use HENC_MB_Type_I4MB
                if (pre_intra_mode == 0) {
                    int ii;
                    unsigned char extra_padding_flag = 0;
                    unsigned char ori_14 = input_mb[14];
                    unsigned char ori_17 = input_mb[17];
#ifdef INTRA_DYNAMIC_PADDING
                    extra_padding_flag = 1;
                    if (p->mb_buffer[index].ie.i16mode == HENC_HOR_PRED_16) {
                        if (mb_x < p->src.mb_width-1) {
                            if (!p->mb_buffer[index+1].intra_mode) {
                                extra_padding_flag = 0;
                            }
                            else if (p->mb_buffer[index+1].intra_mode && p->mb_buffer[index+1].ie.i16mode==HENC_HOR_PRED_16) {
                                extra_padding_flag = 0;
                            }
                        } else {
                            extra_padding_flag = 0;
                        }
                    }
#else
                    extra_padding_flag = 1;
#endif
                    if (extra_padding_flag) {
                        /* 3 padding */
                        input_mb[14]=0x22;
                        input_mb[17]=2;
                        for (ii=0;ii<2;ii++) {
                            tmp = input_mb;
                            input_mb += 864;
                            memcpy(input_mb, tmp,sizeof(char)*864);
                            total_bytes += 864;
                            insert_mb+=1;
                        }
                    } else {
                        /* one padding */
                        input_mb[14]=0x1;
                        input_mb[17]=0x1;
                    }

                    tmp = input_mb;
                    input_mb += 864;
                    memcpy(input_mb, tmp,sizeof(char)*864);
                    input_mb[14]=ori_14;
                    input_mb[17]=ori_17;
                    input_mb[22] = 0;
                    input_mb[23] = 0;

                    if (input_mb[18] == HENC_MB_Type_I4MB) {
                        unsigned char left_top_pre = input_mb[14]&0xf;
                        if (left_top_pre == AML_I4_Diagonal_Down_Right ||
                          left_top_pre == AML_I4_Vertical_Right ||
                          left_top_pre == AML_I4_Horizontal_Down) {
                            /* if the first intra is I4, some pre type is not supported for the left_top, force it to DC*/
                            input_mb[14]&=(~0xf);
                            input_mb[14]|=AML_I4_DC;
                        }
                    }
                    total_bytes += 864;
                    insert_mb+=1;
                }else {
                    input_mb[22] = 0;
                    input_mb[23] = 0;
                }
                pre_intra_mode = 1;
            }else {
                if (p->slot[0].ref_mb_buff && (p->mb_buffer[index].me.mv_info.half_mode == true)) {
                    bool no_intra_around = (p->control.multi_mv == false)?false:(!intra_mb_around(p, mb_x, mb_y,index));
                    if (no_intra_around)
                        restructPixelWithXY_Multi_MV(p, p->slot[0].ref_mb_buff, mb_x, mb_y);
                    else
                        restructPixelWithXY(p, p->slot[0].ref_mb_buff, mb_x, mb_y);
                    if ((p->src.fmt  == AMVENC_NV21) || (p->src.fmt == AMVENC_NV12))
                        fill_p_buffer_nv21(p, input_mb, mb_x, mb_y, p->slot[0].ref_mb_buff, no_intra_around);
                    else
                        fill_p_buffer_yv12(p, input_mb, mb_x, mb_y, p->slot[0].ref_mb_buff, no_intra_around);
                } else {
                    if ((p->src.fmt  == AMVENC_NV21) || (p->src.fmt == AMVENC_NV12))
                        fill_p_buffer_nv21(p, input_mb, mb_x, mb_y, NULL, false);
                    else
                        fill_p_buffer_yv12(p, input_mb, mb_x, mb_y, NULL, false);
                }

                pre_intra_mode = 0;
            }
            total_bytes += 864;
            mb_x++;
            index++;
        }
        mb_y++;
        if (mb_y == flush_line) {
            addr_info[0] = ENCODER_BUFFER_INPUT;
            addr_info[1] = flush_start;
            addr_info[3] = 0;
            if (mb_y == p->src.mb_height) {
                total_bytes += 864;
                total_bytes = (total_bytes+0xff)&0xffffff00;
                addr_info[3] = 1;
            }
            addr_info[2] = total_bytes;
            ioctl(p->fd, M8VENC_AVC_IOC_INPUT_UPDATE, addr_info);
            flush_line = flush_line+p->src.mb_height/HALF_PIPELINE_NUM;
            if (flush_line >p->src.mb_height)
                flush_line = p->src.mb_height;
            flush_start = total_bytes;
        }
    }
    dump_encode_data(p, ENCODER_BUFFER_INPUT, total_bytes);
    return total_bytes;
}

static int fill_I_buffer(m8venc_drv_t* p)
{
    unsigned char* input_mb = NULL;
    int mb_y,mb_x;
    int mb_stride = p->src.mb_width;
    int total_bytes = 0;
    int index = 0;
    int flush_line = p->src.mb_height/HALF_PIPELINE_NUM;
    int flush_start = 0;
    total_bytes = 0;
    unsigned addr_info[4];
    for (mb_y = 0; mb_y <= p->src.mb_height-1; ) {
        for (mb_x = 0; mb_x <= p->src.mb_width-1; ) {
            input_mb = p->input_buf.addr + ((mb_y*mb_stride+mb_x)*864);
            if ((p->src.fmt  == AMVENC_NV21) || (p->src.fmt == AMVENC_NV12))
                fill_i_buffer_spec_nv21(p, input_mb, mb_x, mb_y,index); // or can use HENC_MB_Type_I4MB
            else
                fill_i_buffer_spec_yv12(p, input_mb, mb_x, mb_y,index); // or can use HENC_MB_Type_I4MB
            total_bytes += 864;
            mb_x++;
            index++;
        }
        mb_y++;
        if (mb_y == flush_line) {
            addr_info[0] = ENCODER_BUFFER_INPUT;
            addr_info[1] = flush_start;
            addr_info[3] = 0;
            if (mb_y == p->src.mb_height) {
                total_bytes += 864;
                total_bytes = (total_bytes+0xff)&0xffffff00;
                addr_info[3] = 1;
            }
            addr_info[2] = total_bytes;
            ioctl(p->fd, M8VENC_AVC_IOC_INPUT_UPDATE, addr_info);
            flush_line =flush_line+p->src.mb_height/HALF_PIPELINE_NUM;
            if (flush_line >p->src.mb_height)
                flush_line = p->src.mb_height;
            flush_start = total_bytes;
        }
    }
    dump_encode_data(p, ENCODER_BUFFER_INPUT, total_bytes);
    return total_bytes;
}
#else

static int fill_P_buffer(m8venc_drv_t* p, int slot)
{
    m8venc_slot_t* cur_slot = (m8venc_slot_t*)&p->slot[slot];
    int mb_y, mb_x;
    int mb_stride = p->src.mb_width;
    int index = cur_slot->start_mby*mb_stride+cur_slot->start_mbx;
    uint8_t  *input_mb, *tmp;
    int pre_intra_mode = 0;
    unsigned process_line  = 0;

    for (mb_y = cur_slot->start_mby; mb_y <= cur_slot->end_mby; ) {
        index = mb_y*mb_stride+cur_slot->start_mbx;
        for (mb_x = cur_slot->start_mbx; mb_x <= cur_slot->end_mbx; ) {
            if (index > 0)
                pre_intra_mode = p->mb_buffer[index-1].intra_mode;
            else
                pre_intra_mode = 0;
            input_mb = p->input_buf.addr+p->mb_buffer[index].mem_offset;
            if (p->mb_buffer[index].intra_mode) {
                if ((p->src.fmt  == AMVENC_NV21) || (p->src.fmt == AMVENC_NV12))
                    fill_i_buffer_spec_nv21(p, input_mb, mb_x, mb_y,index); // or can use HENC_MB_Type_I4MB
                else
                    fill_i_buffer_spec_yv12(p, input_mb, mb_x, mb_y,index); // or can use HENC_MB_Type_I4MB
                if (pre_intra_mode == 0) {
                    int ii;
                    unsigned char ori_14 = input_mb[14];
                    unsigned char ori_17 = input_mb[17];
                    /* 3 padding */
                    input_mb[14]=0x22;
                    input_mb[17]=2;
                    for (ii=0; ii<2; ii++) {
                        tmp = input_mb;
                        input_mb += 864;
                        memcpy(input_mb, tmp,sizeof(char)*864);
                    }

                    tmp = input_mb;
                    input_mb += 864;
                    memcpy(input_mb, tmp,sizeof(char)*864);
                    input_mb[14]=ori_14;
                    input_mb[17]=ori_17;
                    input_mb[22] = 0;
                    input_mb[23] = 0;

                    if (input_mb[18] == HENC_MB_Type_I4MB) {
                        unsigned char left_top_pre = input_mb[14]&0xf;
                        if (left_top_pre == AML_I4_Diagonal_Down_Right ||
                          left_top_pre == AML_I4_Vertical_Right ||
                          left_top_pre == AML_I4_Horizontal_Down) {
                            /* if the first intra is I4, some pre type is not supported for the left_top, force it to DC*/
                            input_mb[14]&=(~0xf);
                            input_mb[14]|=AML_I4_DC;
                        }
                    }
                }else {
                    input_mb[22] = 0;
                    input_mb[23] = 0;
                }
            }else {
                if (cur_slot->ref_mb_buff && (p->mb_buffer[index].me.mv_info.half_mode == true) && (p->force_skip == false)) {
                    bool no_intra_around = (p->control.multi_mv == false)?false:(!intra_mb_around(p, mb_x, mb_y,index));
                    if (no_intra_around)
                        restructPixelWithXY_Multi_MV(p, cur_slot->ref_mb_buff, mb_x, mb_y);
                    else
                        restructPixelWithXY(p, cur_slot->ref_mb_buff, mb_x, mb_y);
                    if ((p->src.fmt  == AMVENC_NV21) || (p->src.fmt == AMVENC_NV12))
                        fill_p_buffer_nv21(p, input_mb, mb_x, mb_y, cur_slot->ref_mb_buff, no_intra_around);
                    else
                        fill_p_buffer_yv12(p, input_mb, mb_x, mb_y, cur_slot->ref_mb_buff, no_intra_around);
                } else {
                    if ((p->src.fmt  == AMVENC_NV21) || (p->src.fmt == AMVENC_NV12))
                        fill_p_buffer_nv21(p, input_mb, mb_x, mb_y, NULL, false);
                    else
                        fill_p_buffer_yv12(p, input_mb, mb_x, mb_y, NULL, false);
                }
            }
            mb_x += cur_slot->x_step;
            index++;
        }
        mb_y += cur_slot->y_step;
        process_line++;
        if ((process_line >= cur_slot->update_lines) && (mb_y <= cur_slot->end_mby)) {
            process_line = 0;
            sem_post(&cur_slot->semdone);
        }
    }
    if ((p->mCancel == false) && (p->mStart == true)) {
        cur_slot->finish = true;
        cur_slot->ret = 0;
        sem_post(&cur_slot->semdone);
    }
    return 0;
}

static int fill_I_buffer(m8venc_drv_t* p, int slot)
{
    m8venc_slot_t* cur_slot = (m8venc_slot_t*)&p->slot[slot];
    unsigned char* input_mb = NULL;
    int mb_y, mb_x;
    int mb_stride = p->src.mb_width;
    int index = cur_slot->start_mby*mb_stride+cur_slot->start_mbx;
    unsigned process_line  = 0;

    for (mb_y = cur_slot->start_mby; mb_y <= cur_slot->end_mby; ) {
        index = mb_y*mb_stride+cur_slot->start_mbx;
        for (mb_x = cur_slot->start_mbx; mb_x <= cur_slot->end_mbx; ) {
            input_mb = p->input_buf.addr + p->mb_buffer[index].mem_offset;
            if ((p->src.fmt  == AMVENC_NV21) || (p->src.fmt == AMVENC_NV12))
                fill_i_buffer_spec_nv21(p, input_mb, mb_x, mb_y,index); // or can use HENC_MB_Type_I4MB
            else
                fill_i_buffer_spec_yv12(p, input_mb, mb_x, mb_y,index); // or can use HENC_MB_Type_I4MB
            mb_x+=cur_slot->x_step;
            index++;
        }
        mb_y+=cur_slot->y_step;
        process_line++;
        if ((process_line >= cur_slot->update_lines) && (mb_y <= cur_slot->end_mby)) {
            process_line = 0;
            sem_post(&cur_slot->semdone);
        }
    }
    if ((p->mCancel == false) && (p->mStart == true)) {
        cur_slot->finish = true;
        cur_slot->ret = 0;
        sem_post(&cur_slot->semdone);
    }
    return 0;
}
#endif

static int GetM8VencRefBuf(int fd)
{
    int ret = -1;
    int addr_index = 0;
    if (fd >= 0) {
        ioctl(fd, M8VENC_AVC_IOC_GET_ADDR, &addr_index);
        if (addr_index == ENCODER_BUFFER_REF0) {
            ret= 0;
        } else {
            ret = 1;
        }
    }
    return ret;
}

static int GetMvInfo(m8venc_drv_t* p, int index)
{
    int ret = 0;
    int k;
    short average_x = 0;
    short average_y = 0;
    int mb_x;
    int mb_y;
    int cur_x, cur_y,cal_x,cal_y;
    int ref_mv_x = p->mb_buffer[index].me.mv_info.mv[0].x ;
    int ref_mv_y = p->mb_buffer[index].me.mv_info.mv[0].y ;

    p->mb_buffer[index].me.mv_info.multi_mv = false;
    for (k = 0; k < 16; k++) {
        if (p->mb_buffer[index].me.mv_info.mv[k].x != 0 || p->mb_buffer[index].me.mv_info.mv[k].y != 0)
            ret++;
        if ((ref_mv_x != p->mb_buffer[index].me.mv_info.mv[k].x) || (ref_mv_y != p->mb_buffer[index].me.mv_info.mv[k].y))
            p->mb_buffer[index].me.mv_info.multi_mv = true;
    }
    if (ret <= 1) {
        memset(p->mb_buffer[index].me.mv_info.mv, 0, sizeof(cordinate_t) * 16);//maybe (0,0) is the best choice
        p->mb_buffer[index].me.mv_info.average_mv.x = 0;
        p->mb_buffer[index].me.mv_info.average_mv.y = 0;
        ret = 0;
        p->mb_buffer[index].me.mv_info.all_zero = true;
        p->mb_buffer[index].me.mv_info.multi_mv = false;
        p->mb_buffer[index].me.mv_info.half_mode = false;
    } else {
        p->mb_buffer[index].me.mv_info.all_zero = false;
        //p->mb_buffer[index].me.mv_info.multi_mv = true;
        for (k = 0; k < 16; k++) {
            average_x += p->mb_buffer[index].me.mv_info.mv[k].x;
            average_y += p->mb_buffer[index].me.mv_info.mv[k].y;
        }
        average_x /= 16;
        average_y /= 16;

        p->mb_buffer[index].me.mv_info.average_mv.x = average_x;
        p->mb_buffer[index].me.mv_info.average_mv.y = average_y;
        cur_x = index % p->src.mb_width * 16;
        cur_y = index / p->src.mb_width * 16;
        cal_x = (cur_x<<2) + p->mb_buffer[index].me.mv_info.average_mv.x;
        if (cal_x < 0)
            p->mb_buffer[index].me.mv_info.average_mv.x = 0 - (cur_x<<2);
        else if (cal_x +64 >= (p->src.pix_width<<2))
            p->mb_buffer[index].me.mv_info.average_mv.x = (p->src.pix_width<<2) - 64- (cur_x<<2);

        cal_y = (cur_y<<2) + p->mb_buffer[index].me.mv_info.average_mv.y;
        if (cal_y < 0)
            p->mb_buffer[index].me.mv_info.average_mv.y = 0 - (cur_y<<2);
        else if (cal_y +64 >= (p->src.mb_height<<6))
            p->mb_buffer[index].me.mv_info.average_mv.y = (p->src.mb_height<<6) - 64- (cur_y<<2);

        short temp_x = AVC_ABS(average_x);
        short temp_y = AVC_ABS(average_y);
        if ((temp_x&3) || (temp_y&3)) {
            p->mb_buffer[index].me.mv_info.half_mode = true;
        } else {
            p->mb_buffer[index].me.mv_info.half_mode = false;
        }
    }
    return ret;
}

void parse_mv_info(m8venc_drv_t* p)
{
    int mb_number;
    int index, idx,seq;
    int cur_x, cur_y, stride;
    int cal_x, cal_y;
    short average_x, average_y;
    unsigned char *mv = p->inter_mv_info.addr;
    unsigned *me = (unsigned *)p->inter_bits_info.addr;
    unsigned matrix[16] = {1, 0, 5, 4, 3, 2, 7, 6, 9, 8, 13, 12, 11, 10, 15, 14};
    int key = 0;
    int crop_height = (p->src.mb_height<<4) -p->src.pix_height; // need or not
    int mv_stride = (((p->src.mb_width * 64 + 511) >> 9) << 9) - p->src.mb_width * 64;
    int me_stride = ((((((p->src.mb_width + 1) >> 1)<< 1) * 4 + 63) >> 6) << 4) - (((p->src.mb_width + 1) >> 1)<< 1); // because me is unsign *,so strid divide 4;
    int line_len = 0;
    mb_number = p->src.mbsize;

    p->mb_statics.average_mv_distance = 0;
    p->mb_statics.total_coeff_bits = 0;
    p->mb_statics.mb_max_coeff_bits = 0;
    p->mb_statics.mb_min_coeff_bits = 0x7fff;
    p->mb_statics.total_bits = 0;
    for (index = 0,seq = 0; index < mb_number; index++) {
        cur_x = index % p->src.mb_width * 16;
        cur_y = index / p->src.mb_width * 16;
        for (idx = 0; idx < 16; idx++) {
            stride =  idx * 4;
            key = matrix[idx];
            p->mb_buffer[index].me.mv_info.mv[key].x = (short)((mv[stride + 3] << 8) + mv[stride + 2]);
            p->mb_buffer[index].me.mv_info.mv[key].y = (short)((mv[stride + 1] << 8) + mv[stride]);
        }

        GetMvInfo(p, index);

        average_x = p->mb_buffer[index].me.mv_info.average_mv.x;
        average_y = p->mb_buffer[index].me.mv_info.average_mv.y;

        if (average_x < 0)
            average_x = (average_x -1)/4;
        else
            average_x = (average_x +1)/4;
        if (average_y < 0)
            average_y = (average_y -1)/4;
        else
            average_y = (average_y +1)/4;

        if (cur_x + average_x < 0)
            average_x = -cur_x;
        if (cur_x + average_x >= (int)(p->enc_width - 16))
            average_x = p->enc_width - cur_x- 16;
        if (cur_y + average_y < 0)
            average_y = -cur_y;
        if (cur_y + average_y >= (int)(p->enc_height - 16 + crop_height))
            average_y = p->enc_height - cur_y - 16 + crop_height;

        p->mb_buffer[index].me.mv_info.average_ref.x = average_x;
        p->mb_buffer[index].me.mv_info.average_ref.y = average_y;
        average_x = p->mb_buffer[index].me.mv_info.average_mv.x;
        average_y = p->mb_buffer[index].me.mv_info.average_mv.y;
        p->mb_buffer[index].me.mv_info.mv_distance = AVC_ABS(average_x)*AVC_ABS(average_x) + AVC_ABS(average_y)*AVC_ABS(average_y);
        //p->mb_statics.average_mv_distance += p->mb_buffer[index].me.mv_info.mv_distance;
        mv += 64;

        if (seq < mb_number) {
            p->mb_buffer[seq].me.mv_bits = (short)((me[1]>>20)&0xfff);
            p->mb_buffer[seq].me.mb_type = (unsigned char)((me[1]>>16)&0xf);
            p->mb_buffer[seq].me.coeff_bits = (short)(me[1] & 0xffff);
            seq++;
            line_len++;
            if (line_len < p->src.mb_width - 1) {
                p->mb_buffer[seq].me.mv_bits = (short)((me[0]>>20)&0xfff);
                p->mb_buffer[seq].me.mb_type = (unsigned char)((me[0]>>16)&0xf);
                p->mb_buffer[seq].me.coeff_bits = (short)(me[0] & 0xffff);
                seq++;
                line_len++;
            } else {
                if (line_len == p->src.mb_width - 1) {
                    p->mb_buffer[seq].me.mv_bits = (short)((me[0]>>20)&0xfff);
                    p->mb_buffer[seq].me.mb_type = (unsigned char)((me[0]>>16)&0xf);
                    p->mb_buffer[seq].me.coeff_bits = (short)(me[0] & 0xffff);
                    seq++;
                }
                line_len = 0;
            }
            me += 2;
            if ((seq - 1) % p->src.mb_width == p->src.mb_width - 1) {
                me += me_stride;
            }
        }
        if (p->mb_buffer[index].me.mv_info.all_zero == true)
            p->mb_buffer[index].me.mv_bits = 0;
        p->mb_buffer[index].me.mb_cost = p->mb_buffer[index].me.mv_bits + p->mb_buffer[index].me.coeff_bits;
        p->mb_buffer[index].intra_mode = 0;
        p->mb_statics.total_coeff_bits += p->mb_buffer[index].me.coeff_bits;
        p->mb_statics.total_bits += p->mb_buffer[index].me.mb_cost;

        if (index % p->src.mb_width == p->src.mb_width - 1) {
            mv += mv_stride;
        }
    }
    //p->mb_statics.average_mv_distance /= mb_number;
    return;
}

void parse_intra_mode_info(m8venc_drv_t* p)
{
    int mb_number;
    int index,idx;
    unsigned char *buff = p->intra_pred_info.addr;
    mb_number = p->src.mbsize;
    unsigned sep[8] = {2, 0, 3, 1, 6, 4, 7, 5};
    unsigned key = 0;
    float v_score[9] = {1.0, 0, 0, 0.5, 0.5, 0.75, 0.25, 0.75, 0};
    float h_score[9] = {0, 1, 0, 0, 0, 0, 0, 0, 0};
    float dc_score[9] = {0, 0, 1, 0, 0, 0, 0, 0, 0};
    float statics[3] = {0.0};
    unsigned char mode1,mode2;
    int stride = (((p->src.mb_width * 8 + 511) >> 9) << 9) - p->src.mb_width * 8;
    for (index = 0; index < mb_number; index++) {
        statics[0] = statics[1] = statics[2] = 0.0;
        for (idx = 0; idx < 16; idx += 2) {
            key = idx >>1;
            key = sep[key];
            mode1 = buff[key] & 0x0f;
            mode2 = (buff[key] >> 4) & 0x0f;
            p->mb_buffer[index].ie.i4_pred_mode_l[idx] = mode1;
            p->mb_buffer[index].ie.i4_pred_mode_l[idx + 1] = mode2;
            statics[0] += v_score[mode1] + v_score[mode2];
            statics[1] += h_score[mode1] + h_score[mode2];
            statics[2] += dc_score[mode1] + dc_score[mode2];
        }
        p->mb_buffer[index].ie.i4mode[0] = p->mb_buffer[index].ie.i4_pred_mode_l[10] | p->mb_buffer[index].ie.i4_pred_mode_l[11] << 4 |
            p->mb_buffer[index].ie.i4_pred_mode_l[14] << 8| p->mb_buffer[index].ie.i4_pred_mode_l[15]<< 12 |
            p->mb_buffer[index].ie.i4_pred_mode_l[8] << 16| p->mb_buffer[index].ie.i4_pred_mode_l[9] << 20 |
            p->mb_buffer[index].ie.i4_pred_mode_l[12]<< 24 | p->mb_buffer[index].ie.i4_pred_mode_l[13] << 28;

        p->mb_buffer[index].ie.i4mode[1] = p->mb_buffer[index].ie.i4_pred_mode_l[2] | p->mb_buffer[index].ie.i4_pred_mode_l[3] << 4 |
            p->mb_buffer[index].ie.i4_pred_mode_l[6] << 8| p->mb_buffer[index].ie.i4_pred_mode_l[7]<< 12 |
            p->mb_buffer[index].ie.i4_pred_mode_l[0] << 16| p->mb_buffer[index].ie.i4_pred_mode_l[1] << 20 |
            p->mb_buffer[index].ie.i4_pred_mode_l[4]<< 24 | p->mb_buffer[index].ie.i4_pred_mode_l[5] << 28;

        if (p->control.i16_searchmode >0) {
            p->mb_buffer[index].ie.imode = HENC_MB_Type_I4MB;
            if (p->control.i16_searchmode == 1) {
                for (idx = 0; idx < 3; idx++) {
                    if (statics[idx] > 8) {
                        if (index % p->src.mb_width != 0 && (index>p->src.mb_width)) {
                            p->mb_buffer[index].ie.i16mode = idx;
                            p->mb_buffer[index].ie.imode = HENC_MB_Type_I16MB;
                        } else if ((index == 0) && (idx == 2)) {
                            p->mb_buffer[index].ie.i16mode = idx;
                            p->mb_buffer[index].ie.imode = HENC_MB_Type_I16MB;
                        } else if ((index < p->src.mb_width) && (idx != 0)) {
                            p->mb_buffer[index].ie.i16mode = idx;
                            p->mb_buffer[index].ie.imode = HENC_MB_Type_I16MB;
                        } else if ((index % p->src.mb_width == 0) && (idx != 1)) {
                            p->mb_buffer[index].ie.i16mode = idx;
                            p->mb_buffer[index].ie.imode = HENC_MB_Type_I16MB;
                        }
                        break;
                    }
                }
            }
        }
        buff += 8;
        if (index % p->src.mb_width == p->src.mb_width - 1)
            buff += stride;
    }
    return;
}

void parse_intra_info(m8venc_drv_t* p)
{
    int mb_number;
    int index,idx;
    unsigned *buff = (unsigned *)p->intra_bits_info.addr;
    mb_number = p->src.mbsize;
    int stride = ((((((p->src.mb_width + 1) >> 1)<< 1) * 4 + 63) >> 6) << 4) - (((p->src.mb_width + 1) >> 1)<< 1); // because buff is unsign *,so strid divide 4;
    int line_len = 0;
    for (index = 0,line_len = 0; index < mb_number;) {
        p->mb_buffer[index].ie.pred_mode_c = (unsigned char)((buff[1]>>16) & MB_TYPE_MASK);
        p->mb_buffer[index].ie.coeff_bits = (short)(buff[1] & 0xffff);
        if (index%p->src.mb_width == 0) {
            if ((p->mb_buffer[index].ie.pred_mode_c == HENC_HOR_PRED_8) || (p->mb_buffer[index].ie.pred_mode_c == HENC_PLANE_8)) {
                p->mb_buffer[index].ie.pred_mode_c = HENC_VERT_PRED_8;
            }
        }
        index++;
        line_len++;
        if (line_len < p->src.mb_width - 1) {
            p->mb_buffer[index].ie.pred_mode_c = (unsigned char)((buff[0]>>16) & MB_TYPE_MASK);
            p->mb_buffer[index].ie.coeff_bits = (short)(buff[0] & 0xffff);
            if (index%p->src.mb_width == 0) {
                if ((p->mb_buffer[index].ie.pred_mode_c == HENC_HOR_PRED_8) || (p->mb_buffer[index].ie.pred_mode_c == HENC_PLANE_8)) {
                    p->mb_buffer[index].ie.pred_mode_c = HENC_VERT_PRED_8;
                }
            }
            index++;
            line_len++;
        } else {
            if (line_len == p->src.mb_width - 1) {
                p->mb_buffer[index].ie.pred_mode_c = (unsigned char)((buff[0]>>16) & MB_TYPE_MASK);
                p->mb_buffer[index].ie.coeff_bits = (short)(buff[0] & 0xffff);
                if (index%p->src.mb_width == 0) {
                    if ((p->mb_buffer[index].ie.pred_mode_c == HENC_HOR_PRED_8) || (p->mb_buffer[index].ie.pred_mode_c == HENC_PLANE_8)) {
                        p->mb_buffer[index].ie.pred_mode_c = HENC_VERT_PRED_8;
                    }
                }
                index++;
            }
            line_len = 0;
        }
        buff += 2;
        if ((index - 1) % p->src.mb_width == p->src.mb_width - 1) {
            buff += stride;
        }
    }
    return;
}

/* convert from step size to QP */
static int Qstep2QP(double Qstep)
{
    int q_per = 0, q_rem = 0;

    if (Qstep < 0.625)
        return 0;
    else if ((unsigned)Qstep > 224)
        return 51;

    while (Qstep > 1.125) {
        Qstep /= 2;
        q_per += 1;
    }

    if (Qstep <= (0.625 + 0.6875) / 2)
    {
        q_rem = 0;
    }
    else if (Qstep <= (0.6875 + 0.8125) / 2)
    {
        q_rem = 1;
    }
    else if (Qstep <= (0.8125 + 0.875) / 2)
    {
        q_rem = 2;
    }
    else if (Qstep <= (0.875 + 1.0) / 2)
    {
        q_rem = 3;
    }
    else if (Qstep <= (1.0 + 1.125) / 2)
    {
        q_rem = 4;
    }
    else
    {
        q_rem = 5;
    }
    return (q_per * 6 + q_rem);
}

#ifdef ENABLE_FULL_SEARCH
static int FullSearch(m8venc_drv_t *p,int *imin, int *jmin, , int pmvx, int pmvy, int range, int qp, int* min_cost)
{
    uint8_t *cand;
    int i, j, k, l;
    int d, dmin;
    int i0 = *imin; /* current position */
    int j0 = *jmin;
    int i_temp = (i0>>3)<<3;
    int lx = p->src.pix_width;
    int offset = i_temp + j0 * lx;
    int i_off = i0 - i_temp;
    int lambda_mode = QP2QUANT_m8[AVC_MAX(0, qp)];
    int lambda_motion = LAMBDA_FACTOR(lambda_mode);
    uint8_t *mvbits = NULL, *mvbits_array = NULL;
    int mvshift = 2;
    int mvcost;
    int min_sad = 65535;
    int ilow = 0, ihigh = p->src.pix_width, jlow = 0, jhigh = p->src.pix_height;
    uint8_t *prev = p->ref_info.y;
    uint8_t *cur = p->src.plane[0] + i0+j0*lx;
    int number_of_subpel_positions = 4 * (2 * range + 3);
    int max_mv_bits, max_mvd;
    int temp_bits = 0;
    int bits, imax, imin, i;

    while (number_of_subpel_positions > 0) {
        temp_bits++;
        number_of_subpel_positions >>= 1;
    }
    max_mv_bits = 3 + 2 * temp_bits;
    max_mvd  = (1 << (max_mv_bits >> 1)) - 1;
    mvbits_array = (uint8_t*)calloc(1,sizeof(uint8_t) * (2 * max_mvd + 1));
    if (mvbits_array == NULL) {
        *min_cost = 65535;
        return min_sad;
    }
    mvbits = mvbits_array + max_mvd;

    mvbits[0] = 1;
    for (bits = 3; bits <= max_mv_bits; bits += 2)
    {
        imax = 1    << (bits >> 1);
        imin = imax >> 1;

        for (i = imin; i < imax; i++)   mvbits[-i] = mvbits[i] = bits;
    }

    ilow = i0 - range;
    if (i0 - ilow > 2047) /* clip to conform with the standard */
    {
        ilow = i0 - 2047;
    }

    if (ilow < 0)  // change it from -15 to -13 because of 6-tap filter needs extra 2 lines.
    {
        ilow = 0;
    }

    ihigh = i0 + range - 1;
    if (ihigh - i0 > 2047) /* clip to conform with the standard */
    {
        ihigh = i0 + 2047;
    }

    if (ihigh > p->src.pix_width - 16)
    {
        ihigh = p->src.pix_width - 16;  // change from width-1 to width-3 for the same reason as above
    }

    jlow = j0 - range;
    if (j0 - jlow >(MAX_VMVR - 1)) /* clip to conform with the standard */
    {
        jlow = j0 - MAX_VMVR + 1;
    }
    if (jlow < 0)     // same reason as above
    {
        jlow = 0;
    }

    jhigh = j0 + range - 1;
    if (jhigh - j0 > (MAX_VMVR - 1)) /* clip to conform with the standard */
    {
        jhigh = j0 + MAX_VMVR - 1;
    }

    if (jhigh > p->src.pix_height - 16) // same reason as above
    {
        jhigh = p->src.pix_height - 16;
    }

    cand = prev + offset;
    dmin = 65535;
    dmin = SAD_Macroblock(cur, cand, lx, i_off,dmin);
    mvcost = MV_COST(lambda_motion, mvshift, 0, 0, pmvx, pmvy);
    dmin += mvcost;
    /* perform spiral search */
    for (k = 1; k <= range; k++) {
        i = i0 - k;
        j = j0 - k;
        i_temp = (i>>3)<<3;
        i_off = i - i_temp;
        //i_temp = i;
        cand = prev + i_temp + j * lx;
        for (l = 0; l < 8*k; l++) {
            /* no need for boundary checking again */
            if (i >= ilow && i <= ihigh && j >= jlow && j <= jhigh) {
                d = SAD_Macroblock(cur, cand, lx, i_off,dmin);
                mvcost = MV_COST(lambda_motion, mvshift, i - i0, j - j0, pmvx, pmvy);
                d +=  mvcost;
                if (d < dmin) {
                    dmin = d;
                    *imin = i;
                    *jmin = j;
                    min_sad = d - mvcost; // for rate control
                }
            }

            if (l < (k << 1)) {
                i++;
                //cand++;
                i_off++;
                if (i_off >= 8) {
                    i_off -= 8;
                    cand += 8;
                }
            }else if (l < (k << 2)) {
                j++;
                cand += lx;
            }else if (l < ((k << 2) + (k << 1))) {
                i--;
                //cand--;
                i_off--;
                if (i_off<0) {
                    i_off += 8;
                    cand -= 8;
                }
            } else {
                j--;
                cand -= lx;
            }
        }
    }
    free(mvbits_array);
    *min_cost = dmin;
    return min_sad;
}
#endif

bool IntraDecisionABE(uint8_t *cur, int pitch)
{
    int j;
    uint8_t *out;
    int temp, SBE;

    SBE = 0;
    /* top neighbor */
    out = cur - pitch;
    for (j = 0; j < 16; j++)
    {
        temp = out[j] - cur[j];
        SBE += ((temp >= 0) ? temp : -temp);
    }

    /* left neighbor */
    out = cur - 1;
    out -= pitch;
    cur -= pitch;
    for (j = 0; j < 16; j++)
    {
        temp = *(out += pitch) - *(cur += pitch);
        SBE += ((temp >= 0) ? temp : -temp);
    }

    /* compare mincost/384 and SBE/64 */
    SBE = SBE <<3; //ABE = SBE/64.0;
    return SBE;
}

#define INTRA4_MB_PRED_BITS  56
#define INTRA16_MB_PRED_BITS  8

static int Intra_Search(m8venc_drv_t* p, int slot, bool intra16)
{
    m8venc_slot_t* cur_slot = (m8venc_slot_t*)&p->slot[slot];
    int mb_y, mb_x;
    int mb_stride = p->src.mb_width;
    amvenc_curr_pred_mode_t *curr = NULL;
    AMLMacroblock *currMB = NULL;
    int index = cur_slot->start_mby*mb_stride+cur_slot->start_mbx;
    int min_cost = 0x7fffffff;
    uint8_t*org = NULL;
    int skip_flag = 0;
    int flag = 0;

    cur_slot->mbObj = MBIntraSearch_prepare_m8(p);

    for (mb_y = cur_slot->start_mby; mb_y <= cur_slot->end_mby; ) {
        for (mb_x = cur_slot->start_mbx; mb_x <= cur_slot->end_mbx; ) {
            if ((!p->mStart) || (p->mCancel))
                break;
            curr = cur_slot->mbObj->mb_node;
            currMB = p->intra_mode.mblock + index;
            if ((NULL == curr) || (NULL == currMB)) {
                ALOGD("%s, %d, curr=%p,currMB=%p, slot id: %d. fd:%d", __func__, __LINE__, curr, currMB, slot, p->fd);
            }
            if (intra16) {
                curr->i16_enable[AML_I16_Vertical] = 1;
                curr->i16_enable[AML_I16_DC] = 1;
                curr->i16_enable[AML_I16_Horizontal] = 1;

#ifdef ENABLE_MULTI_SLICE
                if (mb_y%p->rows_per_slice == 0)
                    curr->i16_enable[AML_I16_Vertical] = 0;
#endif
                flag = 0;
                if (p->IDRframe) {
                    if ((mb_y>0) && (mb_x>0)) {
                        if (p->mb_buffer[index-mb_stride].ie.min_cost16 > p->mb_buffer[index-1].ie.min_cost16)
                            flag = 1;
                        else
                            flag = 2;
                    }
                }
                p->mb_buffer[index].ie.min_cost16 = MBIntraSearch_P16(cur_slot->mbObj, mb_x, mb_y, NULL, min_cost, flag);
                p->mb_buffer[index].ie.i16mode = currMB->i16Mode;
                //org = (uint8_t*)p->src.plane[0] + (mb_x<<4) + (mb_y<<4) * p->src.pix_width;
                //p->mb_buffer[index].ie.SBE = IntraDecisionABE(org,p->src.pix_width);
            }else {
                int qp = p->base_quant;
                if (p->control.fix_qp >= 0)
                    qp = p->control.fix_qp;
                if (qp > 40)
                    qp = 40;
                cur_slot->mbObj->lambda_mode = QP2QUANT_m8[qp];	// use fixed qp for Intra mb
                cur_slot->mbObj->lambda_motion = LAMBDA_FACTOR(cur_slot->mbObj->lambda_mode);
                skip_flag = 0;
                if ((mb_y == 0) && (mb_x == 0))
                    skip_flag = 1;

                if ((p->control.i16_searchmode == 1) && (skip_flag == 0)) {
                    curr->i16_enable[AML_I16_Vertical] = 0;
                    curr->i16_enable[AML_I16_DC] = 0;
                    curr->i16_enable[AML_I16_Horizontal] = 0;
                    if (p->mb_buffer[index].ie.imode == HENC_MB_Type_I16MB) {
                        curr->i16_enable[p->mb_buffer[index].ie.i16mode] = 1;
                    } else {
                        skip_flag = 1;
                    }
#ifdef ENABLE_MULTI_SLICE
                    if (mb_y%p->rows_per_slice == 0) {
                        curr->i16_enable[AML_I16_Vertical] = 0;
                        if (p->mb_buffer[index].ie.i16mode == AML_I16_Vertical)
                            skip_flag = 1;
                    }
#endif
                    if (!skip_flag) {
                        p->mb_buffer[index].ie.min_cost16 = MBIntraSearch_P16(cur_slot->mbObj, mb_x, mb_y, NULL, min_cost,0);
                        p->mb_buffer[index].ie.i16mode = currMB->i16Mode;
                        //reset curr point;
                        curr = cur_slot->mbObj->mb_node;
                        currMB = p->intra_mode.mblock + index;
                        if ((NULL == curr) || (NULL == currMB)) {
                            ALOGD("%s, %d, curr=%p,currMB=%p, slot id: %d. fd:%d", __func__, __LINE__, curr, currMB, slot, p->fd);
                        }
                    }
                }

                if (!skip_flag) {
                    p->mb_buffer[index].ie.min_cost4 = MBIntraSearch_P4(cur_slot->mbObj, mb_x, mb_y, NULL,p->mb_buffer[index].ie.min_cost16,(unsigned char*)&p->mb_buffer[index].ie.i4_pred_mode_l[0]);
                }
                if ((currMB->mbMode == AML_I4) || (skip_flag)) {
                    p->mb_buffer[index].ie.imode = HENC_MB_Type_I4MB;
                } else {
                    unsigned temp_coeff  = p->mb_buffer[index].ie.coeff_bits +INTRA4_MB_PRED_BITS;
                    temp_coeff = (temp_coeff *p->mb_buffer[index].ie.min_cost16)/p->mb_buffer[index].ie.min_cost4;
                    p->mb_buffer[index].ie.imode = HENC_MB_Type_I16MB;
                    if (temp_coeff > INTRA16_MB_PRED_BITS)
                        p->mb_buffer[index].ie.coeff_bits = temp_coeff-INTRA16_MB_PRED_BITS;
                    else
                        p->mb_buffer[index].ie.coeff_bits = 1;
                }
            }
            mb_x+=cur_slot->x_step;
            index++;
        }
        mb_y+=cur_slot->y_step;
    }
    if (cur_slot->mbObj)
        MBIntraSearch_clean(cur_slot->mbObj);
    if ((p->mCancel == false) && (p->mStart == true)) {
        cur_slot->finish = true;
        cur_slot->ret = 0;
        sem_post(&cur_slot->semdone);
    }
    return 0;
}

static void* Process_Thread(m8venc_drv_t* p, int slot)
{
    m8venc_slot_t* cur_slot = (m8venc_slot_t*)&p->slot[slot];
    while (1) {
        if (p->mCancel == false)
            sem_wait(&(cur_slot->semstart));
        if (p->mCancel) {
            int sem_count = 0;
            sem_getvalue(&cur_slot->semdone,&sem_count);
            if (sem_count<0) {
                sem_post(&cur_slot->semdone);
                usleep(50000);
            }
            break;
        }
        if (cur_slot->mode == SearchMode_I16) {
            Intra_Search(p, slot, true);
        } else if (cur_slot->mode == SearchMode_I4) {
            Intra_Search(p, slot, false);
        } else if (cur_slot->mode == SearchMode_Fill_I) {
            fill_I_buffer(p, slot);
        } else if (cur_slot->mode == SearchMode_Fill_P) {
            fill_P_buffer(p, slot);
        } else if (cur_slot->mode != SearchMode_idle) {
            ALOGE("SLOT %d, mode is wrong!: %d, fd:%d",slot,cur_slot->mode, p->fd);
        }
    }
    return NULL;
}

static void set_thread_affinity(int cpu_num)
{
#ifndef DISABLE_CPU_AFFINITY
    int num = sysconf(_SC_NPROCESSORS_CONF);
    int i;
    cpu_set_t mask;
    cpu_set_t get;

    ALOGD("system has %i processor(s). want cpu num: %d", num, cpu_num);

    if (cpu_num >= num)
        cpu_num = 0;

    CPU_ZERO(&mask);
    CPU_SET(cpu_num, &mask);

    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
        ALOGD("warning: could not set CPU affinity, continuing...");
    }
    CPU_ZERO(&get);
    if (sched_getaffinity(0, sizeof(get), &get) == -1) {
        ALOGD("warning: cound not get cpu affinity, continuing...");
    }
    for (i = 0; i < num; i++) {
        if (CPU_ISSET(i, &get)) {
            ALOGD("this process %d is running processor : %d, prefer cpu is : %d",getpid(), i, cpu_num);
        }
    }
#endif
    return;
}

static void* Search_Thread_0(void *cookie)
{
    m8venc_drv_t *p = (m8venc_drv_t *)cookie;
    int slot = 0 ;
    prctl(PR_SET_NAME, (unsigned long)"M8Venc_SLOT0", 0, 0, 0);
    ALOGV("Thread slot %d is created. fd:%d" ,slot, p->fd);
    set_thread_affinity(slot);
    return Process_Thread(p,slot);
}

static void* Search_Thread_1(void *cookie)
{
    m8venc_drv_t *p = (m8venc_drv_t *)cookie;
    int slot = 1 ;
    prctl(PR_SET_NAME, (unsigned long)"M8Venc_SLOT1", 0, 0, 0);
    ALOGV("Thread slot %d is created. fd:%d" ,slot, p->fd);
    set_thread_affinity(slot);
    return Process_Thread(p,slot);
}

static void* Search_Thread_2(void *cookie)
{
    m8venc_drv_t *p = (m8venc_drv_t *)cookie;
    int slot = 2 ;
    prctl(PR_SET_NAME, (unsigned long)"M8Venc_SLOT2", 0, 0, 0);
    ALOGV("Thread slot %d is created. fd:%d" ,slot, p->fd);
    set_thread_affinity(slot);
    return Process_Thread(p,slot);
}

static void* Search_Thread_3(void *cookie)
{
    m8venc_drv_t *p = (m8venc_drv_t *)cookie;
    int slot = 3 ;
    prctl(PR_SET_NAME, (unsigned long)"M8Venc_SLOT3", 0, 0, 0);
    ALOGV("Thread slot %d is created. fd:%d" ,slot, p->fd);
    set_thread_affinity(slot);
    return Process_Thread(p,slot);
}

static int start_search_thread(m8venc_drv_t* p, SearchMode mode)
{
    int i = 0;
    if (p->mStart == true) {
        ALOGE("Search thread already running! runniny mode :%d, start mode:%d. fd:%d",p->slot[0].mode, mode, p->fd);
        return -1;
    }
    for (i = 0; i <p->control.thread_num;i++) {
        p->slot[i].finish = false;
        p->slot[i].start_mbx = 0;
        p->slot[i].start_mby = p->src.mb_height*i/p->control.thread_num;
        p->slot[i].x_step = 1;
        p->slot[i].y_step = 1;
        p->slot[i].end_mbx = p->src.mb_width - 1;
        p->slot[i].end_mby = (p->src.mb_height*(i+1)/p->control.thread_num) - 1;
        p->slot[i].update_lines = 0;
        p->slot[i].mode = mode;
        p->slot[i].ret = -1;
        sem_init(&(p->slot[i].semdone), 0,0);
    }
    p->mStart = true;

    for (i = 0; i < p->control.thread_num; i++)
        sem_post(&(p->slot[i].semstart));
    return 0;
}

static void wait_search_thread(m8venc_drv_t* p)
{
    if (p->mStart) {
        for (int i = 0; i < p->control.thread_num; i++)
            sem_wait(&(p->slot[i].semdone));
        p->mStart = false;
    } else {
        ALOGV("Search thread not start, no wait. fd:%d", p->fd);
    }
}

static void get_intra_range(m8venc_drv_t* p, int mb_x, int mb_y,unsigned *intra_min, unsigned *intra_max)
{
    if (p->control.fix_qp >= 0) {
        *intra_max = p->control.fix_qp;
        *intra_min = p->control.fix_qp;
    } else {
        *intra_max = p->control.max_qp;
        *intra_min = p->control.min_qp;
    }
    return;
}

static void get_inter_range(m8venc_drv_t* p, int mb_x, int mb_y,unsigned *inter_min, unsigned *inter_max)
{
    if (p->control.fix_qp >= 0) {
        *inter_max = p->control.fix_qp;
        *inter_max = p->control.fix_qp;
    } else {
        *inter_max = p->control.max_qp;
        *inter_min = p->control.min_qp;
    }
    return;
}

static unsigned calc_target_qp(unsigned cur_bits, unsigned cur_qp, unsigned target_bits, double qp_scale)
{
    unsigned ret_qp = 51;
    unsigned qp = 51;
    float ratio = 0.0;
    double pre_step;
    double cur_step;
    if (cur_bits == 0)
        cur_bits = 1;
    ratio = (float)cur_bits/target_bits;
    ratio = log2(ratio+1);
    pre_step = QP2Qstep(cur_qp);
    //if(ratio < 3.0)// depend to fix quant
        //cur_step = (double)pre_step*(p->mb_statics.mb_predict/mb_target);
    //else
    //    cur_step = (double)(pre_step*(2.0+sqrt(ratio-2.0)));//cur_step = (double)pre_step*sqrt(p->mb_statics.mb_predict/mb_target);
    if (ratio > 0.5 && ratio < 2.0)
        cur_step = (double)(pre_step * (sqrt(ratio) + ratio) / 2.0 + 0.9); /* Quadratic and linear approximation */
    else
        cur_step = (double)(pre_step * (sqrt(ratio) + pow(ratio, 1.0 / 3.0)) / 2.0 + 0.9);
    //cur_step = (double)(pre_step*ratio);
    cur_step = (double) (cur_step*qp_scale);
    qp = Qstep2QP(cur_step);
    ret_qp = AVC_MEDIAN(0, 51, qp);
    return ret_qp;
}

unsigned prepare_I_qp_info(m8venc_drv_t* p)
{
    unsigned frame_target;
    unsigned mb_target;
    int index;
    int mb_number;
    unsigned qp;
    int mb_x,mb_y;
    float ratio;
    unsigned average_mb_target;
    unsigned stride = ((p->src.mb_width + 63) >> 6)<<6;
    unsigned char *qp_info = p->qp_info.addr;
    bool qp_change = false;
    memset(p->qp_info.addr,0,p->qp_info.size);
    frame_target = p->target;
    mb_number = p->src.mb_width * p->src.mb_height;
    average_mb_target = frame_target / mb_number;
    p->mb_statics.intra_cnt = 0;
    int k = 0, j = 0;
    mb_target = 0;
    int cost = 0;
    unsigned intra_min = p->control.min_qp;
    unsigned intra_max = p->control.max_qp;
    unsigned mem_offset = 0;
    double qp_scale = 1.0;

    p->mb_statics.gop_average_qp /= p->mb_statics.gop_length;
    p->mb_statics.last_average_qp = 0;
    for (mb_y = 0; mb_y <  p->src.mb_height; mb_y++) {
        for (mb_x = 0; mb_x < p->src.mb_width; mb_x++) {
            index = mb_y * p->src.mb_width + mb_x;
            qp_change = true;
            p->mb_buffer[index].mem_offset = mem_offset;
            if ((mb_x >= p->src.mb_width/3) && (mb_x <= p->src.mb_width*2/3)
                && (mb_y >= p->src.mb_height/3) && (mb_y <= p->src.mb_height*3/4))
                qp_scale = 0.9;
            else
                qp_scale = 1.1;
            get_intra_range(p, mb_x, mb_y, &intra_min, &intra_max);
            {
                unsigned ie_coeff_bits = 0;
                unsigned pred_bits = 0;
                if (frame_target > 0)
                    mb_target = (int)((float)frame_target / (mb_number - index + 1));
                else
                    mb_target = average_mb_target;

                pred_bits = (p->mb_buffer[index].ie.imode == HENC_MB_Type_I4MB)?INTRA4_MB_PRED_BITS:INTRA16_MB_PRED_BITS;
                if (p->mb_buffer[index].ie.imode == HENC_MB_Type_I4MB)
                    mb_target = mb_target*2;
                if (pred_bits < mb_target)
                    ie_coeff_bits = mb_target - pred_bits;
                else
                    ie_coeff_bits = 1;
                if (((pred_bits+p->mb_buffer[index].ie.coeff_bits) <= mb_target) || (p->mb_buffer[index].ie.coeff_bits == 0))
                    qp = p->base_quant;
                else
                    qp = calc_target_qp((unsigned)p->mb_buffer[index].ie.coeff_bits,p->base_quant, ie_coeff_bits, (double)qp_scale);
                if (frame_target > mb_target)
                    frame_target -= mb_target;
                else
                    frame_target = 0;
            }
            qp = AVC_MEDIAN(intra_min, intra_max, qp);
            p->mb_buffer[index].intra_mode = 1;
            p->mb_buffer[index].qp = qp;
            p->mb_statics.last_average_qp += qp;
            k = mb_x/8;
            k = k*8;
            j = mb_x%8;
            qp_info[k+(7-j)] = (unsigned char)((0 << 7) | (1 << 6) | (qp & 0x3f));
            p->mb_buffer[index].bytes = 864;
            index++;
            mem_offset += 864;
        }
        qp_info += stride;
    }
    p->mb_statics.last_average_qp /= mb_number;
    p->mb_statics.gop_length = 1;
    p->mb_statics.gop_average_qp = p->mb_statics.last_average_qp;
    return p->src.mb_height * stride;
}

unsigned prepare_P_qp_info(m8venc_drv_t* p)
{
    unsigned frame_target;
    unsigned mb_target;
    unsigned index;
    unsigned mb_number;
    unsigned qp;
    unsigned mb_type;
    bool intra_mode = false;
    int mb_x,mb_y;
    float ratio;
    unsigned average_mb_target;
    unsigned stride = ((p->src.mb_width + 63) >> 6)<<6;
    unsigned char *qp_info = p->qp_info.addr;
    bool qp_change = false;
    unsigned k = 0, j = 0;
    mb_target = 0;
    unsigned mv_bits = 0;
    unsigned cost = 0;
    unsigned intra_min = p->control.min_qp;
    unsigned intra_max = p->control.max_qp;
    unsigned inter_min = p->control.min_qp;
    unsigned inter_max = p->control.max_qp;
    unsigned bits_cost = 0;
    unsigned re_check_flag = 0;
    bool redo = false;
    bool all_intra = false;
    unsigned mem_offset= 0;
    unsigned pre_intra_mode = 0;
    double qp_scale = 1.0;
    unsigned int cost_scale = 256;
    unsigned int mv_distance = 0;
    unsigned int mv_count = 0;
    unsigned int intra_count = 0;

    frame_target = p->target;
    mb_number = p->src.mb_width * p->src.mb_height;
    p->mb_statics.intra_cnt = 0;
AGAIN:
    mem_offset = 0;
    pre_intra_mode = 0;
    qp_info = p->qp_info.addr;
    memset(p->qp_info.addr,0,p->qp_info.size);
    average_mb_target = frame_target / mb_number;
    p->mb_statics.last_average_qp = 0;
    for (mb_y = 0; mb_y <  p->src.mb_height; mb_y++) {
        for (mb_x = 0; mb_x < p->src.mb_width; mb_x++) {
            index = mb_y * p->src.mb_width + mb_x;
            mb_type = 0;
            intra_mode = false;
            qp_change = true;
            qp_scale = 1.0;
            cost_scale = 256;
            mv_distance = 0;
            mv_count = 0;
            intra_count = 0;
            if (mb_x > 0) {
                 mv_distance += p->mb_buffer[index-1].me.mv_info.mv_distance;
                 mv_count++;
                 intra_count += p->mb_buffer[index-1].intra_mode;
                 if (mb_y > 0) {
                     mv_distance += p->mb_buffer[index-p->src.mb_width-1].me.mv_info.mv_distance;
                     intra_count += p->mb_buffer[index-p->src.mb_width-1].intra_mode;
                     mv_distance += p->mb_buffer[index-p->src.mb_width].me.mv_info.mv_distance;
                     intra_count += p->mb_buffer[index-p->src.mb_width].intra_mode;
                     mv_count+=2;
                     if (mb_x < p->src.mb_width-1) {
                         mv_distance += p->mb_buffer[index-p->src.mb_width+1].me.mv_info.mv_distance;
                         intra_count += p->mb_buffer[index-p->src.mb_width+1].intra_mode;
                         mv_count++;
                     }
                 }
                 if (mv_distance*3/2< p->mb_buffer[index].me.mv_info.mv_distance)
                     cost_scale = 205;//90%
                 if (intra_count)
                     cost_scale -= intra_count*10;
            }else if (mb_y > 0) {
                 mv_distance += p->mb_buffer[index-p->src.mb_width].me.mv_info.mv_distance;
                 mv_distance += p->mb_buffer[index-p->src.mb_width+1].me.mv_info.mv_distance;
                 intra_count += p->mb_buffer[index-p->src.mb_width].intra_mode;
                 intra_count += p->mb_buffer[index-p->src.mb_width+1].intra_mode;
                 mv_count+=2;
                 if (mv_distance*3/2< p->mb_buffer[index].me.mv_info.mv_distance)
                     cost_scale = 205;//90%
                 if (intra_count)
                     cost_scale -= intra_count*10;
            }
            if (p->pre_intra == false) {
                int mv_diff;
                mb_info_t *pre_info = p->mb_ref_buffer[p->mb_buffer_id^1];
                mv_diff = p->mb_buffer[index].me.mv_info.mv_distance - pre_info[index].me.mv_info.mv_distance;
                if (mv_diff < 0)
                    cost_scale = 0;
            }
            p->mb_buffer[index].mem_offset = mem_offset;
            if ((mb_y != 0) || (mb_x != 0)) {
                int intra_bits = (p->mb_buffer[index].ie.imode == HENC_MB_Type_I4MB)?(p->mb_buffer[index].ie.coeff_bits + INTRA4_MB_PRED_BITS):(p->mb_buffer[index].ie.coeff_bits + INTRA16_MB_PRED_BITS);
                int ie_coeff_bits = p->mb_buffer[index].ie.coeff_bits;
                ie_coeff_bits = (ie_coeff_bits*cost_scale) >>8;
                intra_bits = (intra_bits*cost_scale) >>8;
                if ((ie_coeff_bits < (int)p->mb_buffer[index].me.coeff_bits) && (intra_bits < (int)p->mb_buffer[index].me.mb_cost)) {
                    intra_mode = true;
                }
            }
            if ((index >= p->NextIntraRefreshStart) && (index <= p->NextIntraRefreshEnd) && (p->MBsIntraRefresh>0))
                intra_mode = true;

            re_check_flag = 0;
#if 0
RE_CHECK:
            {
                // calc inter mb target
                unsigned target_inter_qp = 51;
                unsigned target_intra_qp = 51;
                unsigned me_coeff_bits = 0;
                unsigned ie_coeff_bits = 0;
                bool do_intra = true;
                bool do_inter = true;
                unsigned pred_bits = 0;
                unsigned ie_mb_target = 0;
                unsigned target_coeff_bits = 0;
                mv_bits = 0;
                if (frame_target > 0)
                    mb_target = (int)((float)frame_target / (mb_number - index + 1));
                else
                    mb_target = average_mb_target;

                ie_mb_target = mb_target;

                if ((mb_x == 0) && (mb_y == 0)) {
                    do_intra = false;
                    do_inter = true;
                } else if (((mb_x ==  p->src.mb_width -1) && (mb_y == p->src.mb_height -1)) || (all_intra == true)) {
                    do_intra = true;
                    do_inter = false;
                }
#ifdef ENABLE_MULTI_SLICE
                else if ((mb_y%p->rows_per_slice == 0) && (mb_x == 0)) {
                    do_intra = false;
                    do_inter = true;
                }
#endif
                mv_bits = p->mb_buffer[index].me.mb_cost - p->mb_buffer[index].me.coeff_bits;
                if (mv_bits < mb_target)
                    me_coeff_bits = mb_target - mv_bits;
                else
                    me_coeff_bits = 1;
                pred_bits = (p->mb_buffer[index].ie.imode == HENC_MB_Type_I4MB)?INTRA4_MB_PRED_BITS:INTRA16_MB_PRED_BITS;
                if (p->mb_buffer[index].ie.imode == HENC_MB_Type_I4MB)
                    ie_mb_target = mb_target*2;
                if (pred_bits < ie_mb_target)
                    ie_coeff_bits = ie_mb_target - pred_bits;
                else
                    ie_coeff_bits = 1;

                //target_coeff_bits = AVC_MAX(ie_coeff_bits,me_coeff_bits);//(ie_coeff_bits+me_coeff_bits+1)>>1;

                if (do_inter) {
                    if ((unsigned)p->mb_buffer[index].me.mb_cost <= mb_target) {
                        target_inter_qp = p->base_quant;
                        mb_target = p->mb_buffer[index].me.mb_cost;
                        intra_mode = false;
                        do_intra = false;
                    } else {
                        target_inter_qp = calc_target_qp((unsigned)p->mb_buffer[index].me.coeff_bits,p->base_quant, me_coeff_bits);
                    }
                }
                if (do_intra) {
                    if ((pred_bits+p->mb_buffer[index].ie.coeff_bits) <= ie_mb_target) {
                        target_intra_qp = p->base_quant;
                    } else {
                        target_intra_qp = calc_target_qp((unsigned)p->mb_buffer[index].ie.coeff_bits,p->base_quant, ie_coeff_bits);
                    }
                }
                if (target_intra_qp<target_inter_qp) {
                    intra_mode = true;
                }

                if ((p->mb_buffer[index].me.mb_cost > (int)(p->mb_buffer[index].ie.coeff_bits+pred_bits)) && (do_intra))
                    intra_mode = true;
                if ((re_check_flag == 1) && (do_intra) && (intra_mb_around(p, mb_x, mb_y, index) == true) && (p->mb_buffer[index].me.mv_info.multi_mv == true))
                    intra_mode = true;

                qp = (intra_mode == true)?target_intra_qp:target_inter_qp;
                bits_cost = (intra_mode == true)?ie_mb_target:mb_target;
            }
#else
            if ((mb_x== 0) && (mb_y == 0)) {
                intra_mode = false;
            } else if ((mb_x ==  p->src.mb_width -1) && (mb_y == p->src.mb_height -1)) {
                intra_mode = true;
            }
#ifdef ENABLE_MULTI_SLICE
            else if ((mb_y%p->rows_per_slice == 0) && (mb_x == 0)) {
                intra_mode = false;
            }
#endif
#ifdef NO_INTRA
            intra_mode = false;
#endif
            if ((mb_x >= p->src.mb_width/3) && (mb_x <= p->src.mb_width*2/3)
                && (mb_y >= p->src.mb_height/3) && (mb_y <= p->src.mb_height*3/4))
                qp_scale = 0.9;
            else
                qp_scale = 1.1;

            if (intra_mode == false) {
                if (p->mb_buffer[index].me.mv_info.mv_distance <10*16)
                     qp_scale *= 0.9;//0.75
                else if (p->mb_buffer[index].me.mv_info.mv_distance <50*16)
                     qp_scale *= 1.0;//0.875
                else if (p->mb_buffer[index].me.mv_info.mv_distance > 150*16)
                     qp_scale *= 1.2;// 1.25
            }

            mv_bits = p->mb_buffer[index].me.mb_cost - p->mb_buffer[index].me.coeff_bits;
            int coeff_bits = (intra_mode == true)?p->mb_buffer[index].ie.coeff_bits:p->mb_buffer[index].me.coeff_bits;
            if (coeff_bits>0) {
                if (frame_target > 0) {
                    mb_target = (int)((float)frame_target / (mb_number - index + 1));
                    if (intra_mode) {
                        cost = coeff_bits;
                        if (p->mb_buffer[index].ie.imode == HENC_MB_Type_I4MB)
                            cost += INTRA4_MB_PRED_BITS;
                        else
                            cost += INTRA16_MB_PRED_BITS;
                    } else {
                        cost = p->mb_buffer[index].me.mb_cost;
                    }
                    if (cost < mb_target) {
                        mb_target = cost;
                        qp_change = false;
                    } else if (cost < 1.5 * mb_target) {
                        mb_target = mb_target;
                    } else if (cost < 3 * mb_target) {
                        mb_target *= 1.5;
                    } else {
                        mb_target *= 2.5;
                    }
                } else {
                    mb_target = average_mb_target;
                }
                bits_cost = mb_target;

                if (qp_change == true) {
                    if (intra_mode == false) {
                        if (mb_target <= mv_bits)
                            mb_target = 1;
                        else
                            mb_target = mb_target - mv_bits;
                        p->mb_statics.mb_predict = p->mb_buffer[index].me.coeff_bits;
                    } else if (p->mb_buffer[index].ie.imode == HENC_MB_Type_I16MB) {
                        if (mb_target <= INTRA16_MB_PRED_BITS)
                            mb_target = 1;
                        else
                            mb_target = mb_target - INTRA16_MB_PRED_BITS;
                        p->mb_statics.mb_predict = p->mb_buffer[index].ie.coeff_bits;
                    } else {
                        if (mb_target <= INTRA4_MB_PRED_BITS)
                            mb_target = 1;
                        else
                            mb_target = mb_target - INTRA4_MB_PRED_BITS;
                        p->mb_statics.mb_predict = p->mb_buffer[index].ie.coeff_bits;
                    }
                    qp = p->base_quant;
                    if (p->mb_statics.mb_predict > mb_target) {
                        qp = calc_target_qp(p->mb_statics.mb_predict, p->base_quant,mb_target, qp_scale);
                    }
                }else {
                    qp = p->base_quant;
                }
            }else {
                qp = p->base_quant;
            }
#endif
            if (intra_mode) {
                get_intra_range(p, mb_x, mb_y, &intra_min, &intra_max);
                qp = AVC_MEDIAN(intra_min, intra_max, qp);
            } else {
                get_inter_range(p, mb_x, mb_y, &inter_min, &inter_max);
                qp = AVC_MEDIAN(inter_min, inter_max, qp);
            }
            if (p->control.mv_correction == true) {
                if ((p->enc_width <= 864 && p->enc_height <= 480) && (intra_mode == false) && (!re_check_flag) && (p->mb_buffer[index].me.mv_info.all_zero == false)) {
                    unsigned sad1,sad2;
                    sad1 = cal_sad(p,mb_x,mb_y,p->mb_buffer[index].me.mv_info.average_ref.x,p->mb_buffer[index].me.mv_info.average_ref.y);
                    sad2 = cal_sad(p,mb_x,mb_y,0,0);
                    if (sad1 >= sad2) {
                        p->mb_buffer[index].me.mv_info.average_ref.x = 0;
                        p->mb_buffer[index].me.mv_info.average_ref.y = 0;
                        p->mb_buffer[index].me.mv_info.average_mv.x = 0;
                        p->mb_buffer[index].me.mv_info.average_mv.y = 0;
                        //p->mb_buffer[index].me.mv_info.mv_distance = 0;
                        p->mb_buffer[index].me.mv_info.half_mode = false;
                        //p->mb_buffer[index].me.coeff_bits = (sad1* p->mb_buffer[index].me.coeff_bits)/sad2;
                        memset(p->mb_buffer[index].me.mv_info.mv, 0, sizeof(cordinate_t) * 16);
                        p->mb_buffer[index].me.mv_info.multi_mv = false;
                        re_check_flag = 1;
                        //goto RE_CHECK;
                    }
                }
            }
            if (frame_target > bits_cost)
                frame_target -= bits_cost;
            else
                frame_target = 0;

            if (p->force_skip == true) {
                intra_mode = false;
                get_inter_range(p, mb_x, mb_y, &inter_min, &inter_max);
                qp = inter_min;
                if ((mb_x ==  p->src.mb_width -1) && (mb_y == p->src.mb_height -1)) {
                    intra_mode = true;
                    qp = 32;
                }
            }
            if (intra_mode) {
                mb_type = 0;
                p->mb_buffer[index].intra_mode = 1;
                mb_type = 1;//change this later;
                p->mb_statics.intra_cnt ++;
            } else {
                mb_type = 0;
            }
            p->mb_buffer[index].intra_mode = mb_type;
            p->mb_buffer[index].qp = qp;
            p->mb_statics.last_average_qp += qp;
            k = mb_x/8;
            k = k*8;
            j = mb_x%8;
            qp_info[k+(7-j)] = (unsigned char)((0 << 7) | (mb_type << 6) | (qp & 0x3f));
            if ((pre_intra_mode == 0) && (mb_type == 1)) {
                p->mb_buffer[index].bytes = 3456;
                mem_offset += 3456; // 864x4
            } else {
                p->mb_buffer[index].bytes = 864;
                mem_offset += 864;
            }
            index++;
            pre_intra_mode = mb_type;
        }
        intra_mode = false; // we arrive row boundry, so reset;
        qp_info += stride;
    }
    if ((p->mb_statics.intra_cnt>(mb_number*45)/100) && (redo == false)) {
        redo = true;
        frame_target = ((mb_number+p->mb_statics.intra_cnt)*p->target)/mb_number;
        //if(p->mb_statics.intra_cnt>(mb_number*55/100)){
        //    all_intra = true;
        //    frame_target = p->target<<1;
        //}
        p->mb_statics.intra_cnt = 0;
        goto AGAIN;
    }
    p->mb_statics.last_average_qp /= mb_number;
    p->mb_statics.gop_length++;
    p->mb_statics.gop_average_qp += p->mb_statics.last_average_qp;
    ALOGV("intra sum = %d,  average qp:%d. fd:%d",p->mb_statics.intra_cnt,p->mb_statics.last_average_qp, p->fd);
    return p->src.mb_height * stride;
}

static AMVEnc_Status start_ime_half(m8venc_drv_t* p, unsigned char* outptr,int* datalen)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    unsigned cmd[6] ,status;
    unsigned size = 0;
    unsigned qp_info_size;
    unsigned total_time = 0;
    struct timeval temp_test;
    int i = 0;
    unsigned addr_info[4];
    bool all_finish = false;
    unsigned process_lines = 0;
    unsigned last_process_lines = 0;

    if (p->control.logtime)
        gettimeofday(&p->start_test, NULL);

    qp_info_size = prepare_P_qp_info(p)+10;

    dump_encode_data(p, ENCODER_BUFFER_QP, qp_info_size);

    if (p->control.logtime) {
        gettimeofday(&temp_test, NULL);
        total_time = temp_test.tv_sec - p->start_test.tv_sec;
        total_time = total_time*1000000 + temp_test.tv_usec -p->start_test.tv_usec;
        ALOGD("start_ime_half--prepare_qp_info: need time: %d us. fd:%d",total_time, p->fd);
    }

    cmd[0] = ENCODER_NON_IDR;
    cmd[1] = UCODE_MODE_SW_MIX;
    cmd[2] = p->quant;
    cmd[3] = qp_info_size;
    cmd[4] = AMVENC_FLUSH_FLAG_OUTPUT|AMVENC_FLUSH_FLAG_QP|AMVENC_FLUSH_FLAG_REFERENCE; // flush op;
    cmd[5] = p->timeout_value; // result op;
    ioctl(p->fd, M8VENC_AVC_IOC_NEW_CMD, &cmd);

    wait_search_thread(p);

    memset(addr_info,0,sizeof(addr_info));

    process_lines = (p->src.mb_height/p->control.thread_num)/HALF_PIPELINE_NUM;
    if (process_lines == 0)
        process_lines = 1;
    for (i = 0; i <p->control.thread_num;i++) {
        p->slot[i].finish = false;
        p->slot[i].start_mbx = 0;
        p->slot[i].start_mby = i;
        p->slot[i].x_step = 1;
        p->slot[i].y_step = p->control.thread_num;
        p->slot[i].end_mbx = p->src.mb_width - 1;
        p->slot[i].end_mby = p->src.mb_height -1;
        p->slot[i].update_lines = process_lines;
        p->slot[i].mode = SearchMode_Fill_P;
        p->slot[i].ret = -1;
        sem_init(&(p->slot[i].semdone), 0,0);
    }
    p->mStart = true;

    process_lines = 0;
    for (i = 0; i < p->control.thread_num; i++)
        sem_post(&(p->slot[i].semstart));

    while (p->mCancel == false) {
        all_finish = true;
        for (i = 0; i < p->control.thread_num; i++) {
            if (p->slot[i].finish == false)
                sem_wait(&(p->slot[i].semdone));
            all_finish &=p->slot[i].finish;
        }

        if (all_finish)
            process_lines = p->src.mb_height;
        else
            process_lines += (p->slot[0].update_lines*p->control.thread_num);

        if (process_lines>last_process_lines) {
            unsigned index = (process_lines*p->src.mb_width)-1;
            addr_info[2] = p->mb_buffer[index].mem_offset+p->mb_buffer[index].bytes;
            addr_info[0] = ENCODER_BUFFER_INPUT;
            addr_info[3] = 0;
            if (all_finish) {
                addr_info[2] += 864;
                addr_info[2] = (addr_info[2]+0xff)&0xffffff00;
                addr_info[3] = 1;
            }
            ioctl(p->fd, M8VENC_AVC_IOC_INPUT_UPDATE, addr_info);
            addr_info[1] = addr_info[2];
            last_process_lines = process_lines;
        }

        if (all_finish) {
            p->mStart = false;
            for (i = 0; i < p->control.thread_num; i++)
                sem_init(&(p->slot[i].semdone), 0,0);
            break;
        }
    }

    dump_encode_data(p, ENCODER_BUFFER_INPUT, addr_info[2]);

    if (encode_poll(p->fd, -1) <= 0) {
        ALOGD("start_ime_half: poll fail. fd:%d",p->fd);
        return AMVENC_TIMEOUT;
    }
    ioctl(p->fd, M8VENC_AVC_IOC_GET_STAGE, &status);

    ret = AMVENC_FAIL;
    if (status == ENCODER_IDR_DONE) {
        ioctl(p->fd, M8VENC_AVC_IOC_GET_OUTPUT_SIZE, &size);
        if ((size < p->output_buf.size) && (size>0)) {
            memcpy(outptr,p->output_buf.addr,size);
            *datalen  = size;
            ret = AMVENC_PICTURE_READY;
            ALOGV("start_ime_half: done size: %d. fd:%d",size, p->fd);
        }
    } else {
        ALOGE("start_ime_half: encode timeout, status:%d. fd:%d",status, p->fd);
        ret = AMVENC_TIMEOUT;
    }

    if (ret == AMVENC_PICTURE_READY) {
        if (p->control.logtime) {
            gettimeofday(&p->end_test, NULL);
            total_time = p->end_test.tv_sec - p->start_test.tv_sec;
            total_time = total_time*1000000 + p->end_test.tv_usec -p->start_test.tv_usec;
            ALOGD("start_ime_half: need time: %d us. fd:%d",total_time, p->fd);
            p->total_encode_time +=total_time;
        }
        p->total_encode_frame++;
    }
    return ret;
}

static AMVEnc_Status start_intra_half(m8venc_drv_t* p, unsigned char* outptr,int* datalen)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    unsigned cmd[6] ,status;
    unsigned size = 0;
    unsigned qp_info_size;
    unsigned total_time = 0;
    struct timeval temp_test;
    int i = 0;
    unsigned addr_info[4];
    bool all_finish = false;
    unsigned process_lines = 0;
    unsigned last_process_lines = 0;

    if (p->control.logtime)
        gettimeofday(&p->start_test, NULL);

    wait_search_thread(p); // wait i4x4 pred mode

    qp_info_size = prepare_I_qp_info(p) + 10;
    dump_encode_data(p, ENCODER_BUFFER_QP, qp_info_size);

    if (p->control.logtime) {
        gettimeofday(&temp_test, NULL);
        total_time = temp_test.tv_sec - p->start_test.tv_sec;
        total_time = total_time*1000000 + temp_test.tv_usec - p->start_test.tv_usec;
        ALOGD("start_intra_half--prepare_qp_info: need time: %d us. fd:%d",total_time, p->fd);
    }

    cmd[0] = ENCODER_IDR;
    cmd[1] = UCODE_MODE_SW_MIX;
    cmd[2] = p->quant;
    cmd[3] = qp_info_size;
    cmd[4] = AMVENC_FLUSH_FLAG_OUTPUT|AMVENC_FLUSH_FLAG_QP|AMVENC_FLUSH_FLAG_REFERENCE; // flush op;
    cmd[5] = p->timeout_value; // timeout op;
    ioctl(p->fd, M8VENC_AVC_IOC_NEW_CMD, &cmd);

    memset(addr_info,0,sizeof(addr_info));

    process_lines = (p->src.mb_height/p->control.thread_num)/HALF_PIPELINE_NUM;
    if (process_lines == 0)
        process_lines = 1;
    for (i = 0; i <p->control.thread_num;i++) {
        p->slot[i].finish = false;
        p->slot[i].start_mbx = 0;
        p->slot[i].start_mby = i;
        p->slot[i].x_step = 1;
        p->slot[i].y_step = p->control.thread_num;
        p->slot[i].end_mbx = p->src.mb_width - 1;
        p->slot[i].end_mby = p->src.mb_height -1;
        p->slot[i].update_lines = process_lines;
        p->slot[i].mode = SearchMode_Fill_I;
        p->slot[i].ret = -1;
        sem_init(&(p->slot[i].semdone), 0,0);
    }
    p->mStart = true;

    process_lines = 0;
    for (i = 0; i < p->control.thread_num; i++)
        sem_post(&(p->slot[i].semstart));

    while (p->mCancel == false) {
        all_finish = true;
        for (i = 0; i < p->control.thread_num; i++) {
            if (p->slot[i].finish == false)
                sem_wait(&(p->slot[i].semdone));
            all_finish &=p->slot[i].finish;
        }
        if (all_finish)
            process_lines = p->src.mb_height;
        else
            process_lines += (p->slot[0].update_lines*p->control.thread_num);

        if (process_lines>last_process_lines) {
            unsigned index = (process_lines*p->src.mb_width)-1;
            addr_info[2] = p->mb_buffer[index].mem_offset+p->mb_buffer[index].bytes;
            addr_info[0] = ENCODER_BUFFER_INPUT;
            addr_info[3] = 0;
            if (all_finish) {
                addr_info[2] += 864;
                addr_info[2] = (addr_info[2]+0xff)&0xffffff00;
                addr_info[3] = 1;
            }
            ioctl(p->fd, M8VENC_AVC_IOC_INPUT_UPDATE, addr_info);
            addr_info[1] = addr_info[2];
            last_process_lines = process_lines;
        }

        if (all_finish) {
            p->mStart = false;
            for (i = 0; i < p->control.thread_num; i++)
                sem_init(&(p->slot[i].semdone), 0,0);
            break;
        }
    }
    dump_encode_data(p, ENCODER_BUFFER_INPUT, addr_info[2]);

    if (encode_poll(p->fd, -1) <= 0) {
        ALOGD("start_intra_half: poll fail. fd:%d", p->fd);
        return AMVENC_TIMEOUT;
    }
    ioctl(p->fd, M8VENC_AVC_IOC_GET_STAGE, &status);

    ret = AMVENC_FAIL;
    if (status == ENCODER_IDR_DONE) {
        ioctl(p->fd, M8VENC_AVC_IOC_GET_OUTPUT_SIZE, &size);
        if ((size < p->output_buf.size) && (size>0)) {
            memcpy(outptr,p->output_buf.addr,size);
            *datalen  = size;
            ret = AMVENC_NEW_IDR;
            ALOGV("start_intra_half: done size: %d. fd:%d",size, p->fd);
        }
    } else {
        ALOGE("start_intra_half: encode timeout, status:%d. fd:%d",status, p->fd);
        ret = AMVENC_TIMEOUT;
    }

    if (p->src.type == CANVAS_BUFFER) {
        if (p->src.plane[0] != 0 && p->src.mmapsize != 0)
            munmap((void*)p->src.plane[0] ,p->src.mmapsize);
        p->src.plane[0] = 0;
        p->src.plane[1] = 0;
        p->src.plane[2] = 0;
        p->src.mmapsize = 0;
    }

    if (ret == AMVENC_NEW_IDR) {
        p->total_encode_frame++;
        if (p->control.logtime) {
            gettimeofday(&p->end_test, NULL);
            total_time = (p->end_test.tv_sec - p->start_test.tv_sec)*1000000 + p->end_test.tv_usec -p->start_test.tv_usec;
            p->total_encode_time +=total_time;
            ALOGD("start_intra_half: need time: %d us. fd:%d",total_time, p->fd);
        }
    }
    return ret;
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
    if (!src || !dest)
        return -1;


    R = dest;
    G = R + canvas_w * height;
    B = G + canvas_w * height;

    for ( i = 0; i < height; i += 1) {
        for ( j = 0; j < width; j += 8) {
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

static int copy_to_crop(m8venc_drv_t* p)
{
    unsigned offset = 0;
    int i = 0;
    unsigned total_size = 0;
    unsigned char* src = NULL;
    unsigned char* dst = NULL;
    if (p->src.pix_height == (p->src.mb_height<<4))
        return 0;

    src = (unsigned char*)p->src.plane[0];
    dst = p->src.crop_buffer;
    memcpy(dst, src,p->src.pix_width*p->src.pix_height);
    offset = p->src.pix_height*p->src.pix_width;

    memset(p->src.crop_buffer+offset, 0, ((p->src.mb_height<<4) -p->src.pix_height)*p->src.pix_width);
    offset = p->src.pix_width*p->src.mb_height<<4;

    p->src.plane[0] = (unsigned)p->src.crop_buffer;

    src = (unsigned char*)p->src.plane[1];
    dst = (unsigned char*)(p->src.crop_buffer+offset);
    if ((p->src.fmt == AMVENC_NV12) || (p->src.fmt == AMVENC_NV21)) {
        memcpy(dst, src,p->src.pix_width*p->src.pix_height/2);
        offset += p->src.pix_height*p->src.pix_width/2;
        memset(p->src.crop_buffer+offset, 0x80, ((p->src.mb_height<<4) -p->src.pix_height)*p->src.pix_width/2);
        p->src.plane[1] = (unsigned)dst;
    } else if (p->src.fmt == AMVENC_YUV420) {
        memcpy(dst, src,p->src.pix_width*p->src.pix_height/4);
        offset += p->src.pix_height*p->src.pix_width/4;
        memset(p->src.crop_buffer+offset, 0x80, ((p->src.mb_height<<4) -p->src.pix_height)*p->src.pix_width/4);
        offset = p->src.pix_width*p->src.mb_height*5<<2;
        p->src.plane[1] = (unsigned)dst;
        src = (unsigned char*)p->src.plane[2];
        dst = (unsigned char*)(p->src.crop_buffer+offset);
        memcpy(dst, src,p->src.pix_width*p->src.pix_height/4);
        offset += p->src.pix_height*p->src.pix_width/4;
        memset(p->src.crop_buffer+offset, 0x80, ((p->src.mb_height<<4) -p->src.pix_height)*p->src.pix_width/4);
        p->src.plane[2] = (unsigned)dst;
    }
    total_size = p->src.mb_width*p->src.mb_height*256;
    return total_size;
}

static unsigned copy_for_scale(m8venc_drv_t* p, int src_w, int src_h)
{
    unsigned offset = 0;
    int canvas_w = 0;
    int i = 0;
    unsigned total_size = 0;
    unsigned char* src = NULL;
    unsigned char* dst = NULL;
    int pix_height = src_h;

    if (p->src.fmt != AMVENC_YUV420)
        canvas_w = ((src_w+31)>>5)<<5;
    else
        canvas_w = ((src_w+63)>>6)<<6;

    src = (unsigned char*)p->src.plane[0];
    dst = p->input_buf.addr;
    if (src_w != canvas_w) {
        for (i =0; i<pix_height; i++) {
            memcpy(dst, src,src_w);
            dst+=canvas_w;
            src+=src_w;
        }
    } else {
        memcpy(dst, src,src_w*pix_height);
    }
    offset = pix_height*canvas_w;

    src = (unsigned char*)p->src.plane[1];
    dst = (unsigned char*)(p->input_buf.addr+offset);
    if ((p->src.fmt == AMVENC_NV12) || (p->src.fmt == AMVENC_NV21)) {
        if (src_w != canvas_w) {
            for (i =0;  i<pix_height/2;i++) {
                memcpy(dst, src,src_w);
                dst+=canvas_w;
                src+=src_w;
            }
        } else {
            memcpy(dst, src,src_w*pix_height/2);
        }
        offset += pix_height*canvas_w/2;
    } else if (p->src.fmt == AMVENC_YUV420) {
        if (src_w != canvas_w) {
            for (i =0;i<pix_height/2;i++) {
                memcpy(dst, src,src_w/2);
                dst+=canvas_w/2;
                src+=src_w/2;
            }
        } else {
            memcpy(dst, src,src_w*pix_height/4);
        }
        offset += pix_height*canvas_w/4;
        src = (unsigned char*)p->src.plane[2];
        dst = (unsigned char*)(p->input_buf.addr+offset);
        if (src_w != canvas_w) {
            for (i =0;i<pix_height/2;i++) {
                memcpy(dst, src,src_w/2);
                dst+=canvas_w/2;
                src+=src_w/2;
            }
        } else {
            memcpy(dst, src,src_w*pix_height/4);
        }
        offset += pix_height*canvas_w/4;
    }
    total_size = (canvas_w*pix_height*3)>>1;
    return total_size;
}

static unsigned copy_to_local(m8venc_drv_t* p)
{
    bool crop_flag = false;
    unsigned offset = 0;
    int canvas_w = 0;
    int i = 0;
    unsigned total_size = 0;
    unsigned char* src = NULL;
    unsigned char* dst = NULL;
    int pix_height = p->src.pix_height;
    if (p->src.pix_height<(p->src.mb_height<<4)) {
        //crop_flag = true;
        pix_height = p->src.mb_height<<4;
    }

    if (p->src.fmt != AMVENC_YUV420)
        canvas_w = ((p->src.pix_width+31)>>5)<<5;
    else
        canvas_w = ((p->src.pix_width+63)>>6)<<6;

    src = (unsigned char*)p->src.plane[0];
    dst = p->input_buf.addr;
    if (p->src.pix_width != canvas_w) {
        for (i =0; i<pix_height; i++) {
            memcpy(dst, src,p->src.pix_width);
            dst+=canvas_w;
            src+=p->src.pix_width;
        }
    } else {
        memcpy(dst, src,p->src.pix_width*pix_height);
    }
    offset = pix_height*canvas_w;

    if (crop_flag) {
        memset(p->input_buf.addr+offset, 0, ((p->src.mb_height<<4) -p->src.pix_height)*canvas_w);
        offset = canvas_w*p->src.mb_height<<4;
    }

    src = (unsigned char*)p->src.plane[1];
    dst = (unsigned char*)(p->input_buf.addr+offset);
    if ((p->src.fmt == AMVENC_NV12) || (p->src.fmt == AMVENC_NV21)) {
        if (p->src.pix_width != canvas_w) {
            for (i =0;  i<pix_height/2;i++) {
                memcpy(dst, src,p->src.pix_width);
                dst+=canvas_w;
                src+=p->src.pix_width;
            }
        } else {
            memcpy(dst, src,p->src.pix_width*pix_height/2);
        }
        offset += pix_height*canvas_w/2;
        if (crop_flag)
            memset(p->input_buf.addr+offset, 0x80, ((p->src.mb_height<<4) -p->src.pix_height)*canvas_w/2);
    } else if (p->src.fmt == AMVENC_YUV420) {
        if (p->src.pix_width != canvas_w) {
            for (i =0;i<pix_height/2;i++) {
                memcpy(dst, src,p->src.pix_width/2);
                dst+=canvas_w/2;
                src+=p->src.pix_width/2;
            }
        } else {
            memcpy(dst, src,p->src.pix_width*pix_height/4);
        }
        offset += pix_height*canvas_w/4;
        if (crop_flag) {
            memset(p->input_buf.addr+offset, 0x80, ((p->src.mb_height<<4) -p->src.pix_height)*canvas_w/4);
            offset = canvas_w*p->src.mb_height*5<<2;
        }
        src = (unsigned char*)p->src.plane[2];
        dst = (unsigned char*)(p->input_buf.addr+offset);
        if (p->src.pix_width != canvas_w) {
            for (i =0;i<pix_height/2;i++) {
                memcpy(dst, src,p->src.pix_width/2);
                dst+=canvas_w/2;
                src+=p->src.pix_width/2;
            }
        } else {
            memcpy(dst, src,p->src.pix_width*pix_height/4);
        }
        offset += pix_height*canvas_w/4;
        if (crop_flag) {
            memset(p->input_buf.addr+offset, 0x80, ((p->src.mb_height<<4) -p->src.pix_height)*canvas_w/4);
        }
    }
    total_size = canvas_w*p->src.mb_height*3<<3;
    return total_size;
}


static int set_input(m8venc_drv_t* p, unsigned* yuv, unsigned enc_width, unsigned enc_height, AMVEncBufferType type, AMVEncFrameFmt fmt, bool IDR)
{
    int i;
    unsigned y = yuv[0];
    unsigned u = yuv[1];
    unsigned v = yuv[2];
    unsigned crop_top = yuv[5];
    unsigned crop_bottom = yuv[6];
    unsigned crop_left = yuv[7];
    unsigned crop_right = yuv[8];
    unsigned pitch = yuv[9];
    unsigned height = yuv[10];
    unsigned scale_width = yuv[11];
    unsigned scale_height = yuv[12];
    input_t *src = &p->src;
    unsigned addr[6];

    if (!y)
        return -1;

    src->pix_width  = enc_width;
    src->pix_height = enc_height;
    src->mb_width   = (src->pix_width+15)>>4;
    src->mb_height  = (src->pix_height+15)>>4;

    src->canvas  = 0xffffffff;
    src->plane[1] = 0;
    src->plane[2] = 0;
    src->crop.crop_top = 0;
    src->crop.crop_bottom = 0;
    src->crop.crop_left = 0;
    src->crop.crop_right = 0;
    src->crop.src_w = 0;
    src->crop.src_h = 0;
    p->scale_enable = false;
    src->mmapsize = 0;

    if (type == VMALLOC_BUFFER) {
        if ((p->nr_enable) && ((fmt == AMVENC_NV21) || (fmt == AMVENC_NV12) || (fmt == AMVENC_YUV420))) {
            unsigned total_time = 0;
            struct timeval start_noise, end_noise;
            if (p->control.logtime) {
                gettimeofday(&start_noise, NULL);
            }
            if (p->Prm_Nr.mode == 2) {
                noise_reduction_g2((uint8_t*)y, enc_width, enc_height, &(p->Prm_Nr));
                src->plane[0] = y;
                if ((fmt == AMVENC_NV21) || (fmt == AMVENC_NV12) || (fmt == AMVENC_YUV420))
                    src->plane[1] = u;
                if (fmt == AMVENC_YUV420)
                    src->plane[2] = v;
            } else {
                if (fmt == AMVENC_YUV420) {
                    p->Prm_Nr.prm_is_nv21 = 0;
                    noise_reduction((unsigned char*)y, enc_width, enc_height, &(p->Prm_Nr));
                    src->plane[0] = (unsigned)(p->Prm_Nr.pNrY);
                    src->plane[1] = (unsigned)(p->Prm_Nr.pNrY + enc_width * enc_height);
                    src->plane[2] = (unsigned)(p->Prm_Nr.pNrY + enc_width * enc_height * 5 / 4);
                } else {
                    p->Prm_Nr.prm_is_nv21 = 1;
                    noise_reduction((unsigned char*)y, enc_width, enc_height, &(p->Prm_Nr));
                    src->plane[0] = (unsigned)(p->Prm_Nr.pNrY);
                    src->plane[1] = (unsigned)(p->Prm_Nr.pNrY + enc_width * enc_height);
                }
            }
            if (p->control.logtime) {
                gettimeofday(&end_noise, NULL);
                total_time = end_noise.tv_sec - start_noise.tv_sec;
                total_time = total_time*1000000 + end_noise.tv_usec -start_noise.tv_usec;
                ALOGD("noise reduction--: spend time: %d us, fd:%d",total_time, p->fd);
            }
        }else {
            src->plane[0] = y;
            if ((fmt == AMVENC_NV21) || (fmt == AMVENC_NV12) || (fmt == AMVENC_YUV420))
                src->plane[1] = u;
            if (fmt == AMVENC_YUV420)
                src->plane[2] = v;
            if ((scale_width) && (scale_height)) {
                p->scale_enable = true;
                src->crop.crop_top = crop_top;
                src->crop.crop_bottom = crop_bottom;
                src->crop.crop_left = crop_left;
                src->crop.crop_right = crop_right;
                src->crop.src_w = pitch;
                src->crop.src_h = height;
            }
        }
    }else {
        src->canvas = yuv[3];
        {
            unsigned canvas_w = 0;
            addr[0] = src->canvas;
            if ((src->plane[0] != 0) && (src->mmapsize != 0)) {
                ALOGD("Release old mmap buffer :0x%x, size:0x%x. fd:%d",src->plane[0],src->mmapsize,p->fd);
                munmap((void*)src->plane[0] ,src->mmapsize);
                src->plane[0] = 0;
                src->mmapsize = 0;
            } else if ((src->plane[0] != 0) || (src->mmapsize != 0)) {
                ALOGE("Buffer Ptr error:0x%x, size:0x%x. fd:%d",src->plane[0],src->mmapsize, p->fd);
            }
            if (p->src.fmt != AMVENC_YUV420)  // need check;
                canvas_w = ((p->src.pix_width+31)>>5)<<5;
            else
                canvas_w = ((p->src.pix_width+63)>>6)<<6;
            ioctl(p->fd, M8VENC_AVC_IOC_READ_CANVAS,&addr[0]);
            if (addr[0] != 0) {
                y = (unsigned)mmap(0,addr[1], PROT_READ|PROT_WRITE , MAP_SHARED ,p->fd, addr[0]);
                src->plane[0] = y;
                src->mmapsize = addr[1];
                if ((fmt == AMVENC_NV21) || (fmt == AMVENC_NV12) || (fmt == AMVENC_YUV420))
                    src->plane[1] = src->plane[0]+canvas_w*p->src.pix_height;
                if (fmt == AMVENC_YUV420)
                    src->plane[2] = src->plane[0]+canvas_w*p->src.pix_height*5/4;
            }
        }
    }
    src->type = type;
    src->fmt  = fmt;
    if (src->type == VMALLOC_BUFFER) {
        if ((src->fmt != AMVENC_RGB888) && (src->fmt != AMVENC_RGBA8888)) {
            if (p->scale_enable) {
                unsigned canvas_w = 0;
                unsigned canvas_h = src->mb_height<<4;
                src->framesize = copy_for_scale(p, (int)pitch, (int)height);
                canvas_w = ((scale_width+31)>>5)<<5;
                src->plane[0] = (unsigned)p->scale_buff.addr;
                src->plane[1] = src->plane[0]+canvas_w*canvas_h;
                src->plane[2] = 0;
            } else {
                if (p->src.crop_buffer)
                    copy_to_crop(p);
                src->framesize = copy_to_local(p);
            }
        } else if (p->src.fmt == AMVENC_RGB888) {
            src->framesize= RGB24_To_RGB24Plane_NEON((unsigned char*)p->src.plane[0],p->input_buf.addr,p->src.pix_width,p->src.pix_height);
            src->fmt = AMVENC_RGB888_PLANE;
        } else if (p->src.fmt == AMVENC_RGBA8888) {
            src->framesize = RGBX32_To_RGB24Plane_NEON((unsigned char*)p->src.plane[0],p->input_buf.addr,p->src.pix_width,p->src.pix_height);
            src->fmt = AMVENC_RGB888_PLANE;
        }
    }else {
        src->framesize = src->mb_height*src->pix_width*24;
    }
    return 0;
}

static AMVEnc_Status start_ime_full(m8venc_drv_t* p, unsigned char* outptr,int* datalen, int pre_encode)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    unsigned status;
    unsigned size = 0;
    unsigned control_info[16];
    unsigned total_time = 0;

    if (p->control.logtime)
        gettimeofday(&p->start_test, NULL);

    control_info[0] = ENCODER_NON_IDR;
    control_info[1] = UCODE_MODE_FULL;
    control_info[2] = p->src.type;
    control_info[3] = p->src.fmt;
    control_info[4] = (p->src.type == VMALLOC_BUFFER)?(unsigned)p->input_buf.addr:(unsigned)p->src.canvas;
    control_info[5] = p->src.framesize;
    control_info[6] = (pre_encode == 1)?p->base_quant:p->quant;
    control_info[7] = (pre_encode == 1)?(AMVENC_FLUSH_FLAG_INTER_INFO):(AMVENC_FLUSH_FLAG_INPUT|AMVENC_FLUSH_FLAG_OUTPUT); // flush op;
    control_info[8] = p->timeout_value; // timeout op;
    control_info[9] = p->src.crop.crop_top;
    control_info[10] = p->src.crop.crop_bottom;
    control_info[11] = p->src.crop.crop_left;
    control_info[12] = p->src.crop.crop_right;
    control_info[13] = p->src.crop.src_w;
    control_info[14] = p->src.crop.src_h;
    control_info[15] = p->scale_enable?1:0;
    ioctl(p->fd, M8VENC_AVC_IOC_NEW_CMD, &control_info[0]);

    if (pre_encode) {
        parse_intra_mode_info(p);
        parse_intra_info(p);
        dump_ie_bits(p);
        if (p->control.i16_searchmode == 0) {
            wait_search_thread(p);
            start_search_thread(p,SearchMode_I4);
        } else if (p->control.i16_searchmode == 1) {
            start_search_thread(p,SearchMode_I4);
        }
    }

    if (encode_poll(p->fd, -1) <= 0) {
        ALOGE("start_ime_full: poll fail. fd:%d", p->fd);
        return AMVENC_TIMEOUT;
    }

    ioctl(p->fd, M8VENC_AVC_IOC_GET_STAGE, &status);
    ret = AMVENC_FAIL;
    if (status == ENCODER_IDR_DONE) {
        ioctl(p->fd, M8VENC_AVC_IOC_GET_OUTPUT_SIZE, &size);
        if ((size < p->output_buf.size) && (size>0)) {
            if (pre_encode == 0) {
                memcpy(outptr,p->output_buf.addr,size);
            } else {
                parse_mv_info(p);
                dump_mv_bits(p);
                if (p->control.i16_searchmode < 2) {
                    wait_search_thread(p);
                }
            }
            *datalen  = size;
            ret = AMVENC_PICTURE_READY;
            ALOGV("start_ime_full: done size: %d. fd:%d",size, p->fd);
        }
    } else {
        ALOGE("start_ime_full: encode timeout, status:%d. fd:%d",status, p->fd);
        ret = AMVENC_TIMEOUT;
    }
    if (ret == AMVENC_PICTURE_READY) {
        if (pre_encode == 0)
            p->total_encode_frame++;
        if (p->control.logtime) {
            gettimeofday(&p->end_test, NULL);
            total_time = p->end_test.tv_sec - p->start_test.tv_sec;
            total_time = total_time*1000000 + p->end_test.tv_usec -p->start_test.tv_usec;
            ALOGD("start_ime_full: need time: %d us. fd:%d",total_time, p->fd);
            p->total_encode_time +=total_time;
        }
    }
    return ret;
}

static AMVEnc_Status start_intra_full(m8venc_drv_t* p, unsigned char* outptr,int* datalen, int pre_encode)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    unsigned status;
    unsigned size = 0;
    unsigned control_info[16];
    unsigned total_time = 0;

    if (p->control.logtime)
        gettimeofday(&p->start_test, NULL);

    control_info[0] = ENCODER_IDR;
    control_info[1] = UCODE_MODE_FULL;
    control_info[2] = p->src.type;
    control_info[3] = p->src.fmt;
    control_info[4] = (p->src.type == VMALLOC_BUFFER)?(unsigned)p->input_buf.addr:(unsigned)p->src.canvas;
    control_info[5] = p->src.framesize; //(16X3/2)
    control_info[6] = (pre_encode == 1)?p->base_quant:p->quant;
    control_info[7] = (pre_encode == 1)?(AMVENC_FLUSH_FLAG_INPUT|AMVENC_FLUSH_FLAG_INTRA_INFO):(AMVENC_FLUSH_FLAG_INPUT|AMVENC_FLUSH_FLAG_OUTPUT); // flush op;
    control_info[8] = p->timeout_value; // result op;
    control_info[9] = p->src.crop.crop_top;
    control_info[10] = p->src.crop.crop_bottom;
    control_info[11] = p->src.crop.crop_left;
    control_info[12] = p->src.crop.crop_right;
    control_info[13] = p->src.crop.src_w;
    control_info[14] = p->src.crop.src_h;
    control_info[15] = p->scale_enable?1:0;

    if (p->scale_enable)
        p->src.fmt = AMVENC_NV21;
    ioctl(p->fd, M8VENC_AVC_IOC_NEW_CMD, &control_info[0]);

    if ((pre_encode) && (p->control.i16_searchmode == 0))
        start_search_thread(p,SearchMode_I16);

    if (encode_poll(p->fd, -1) <= 0) {
        ALOGE("start_intra_full: poll fail. fd:%d", p->fd);
        return AMVENC_TIMEOUT;
    }

    ioctl(p->fd, M8VENC_AVC_IOC_GET_STAGE, &status);
    ret = AMVENC_FAIL;
    if (status == ENCODER_IDR_DONE) {
        ioctl(p->fd, M8VENC_AVC_IOC_GET_OUTPUT_SIZE, &size);
        if ((size < p->output_buf.size) && (size>0)) {
            if (pre_encode == 0) {
                memcpy(outptr,p->output_buf.addr,size);
            } else {
                if (p->IDRframe) {
                    parse_intra_mode_info(p);
                    parse_intra_info(p);
                    dump_ie_bits(p);
                    if (p->control.i16_searchmode == 0) {
                        wait_search_thread(p);
                        start_search_thread(p,SearchMode_I4);
                    } else if (p->control.i16_searchmode == 1) {
                        start_search_thread(p,SearchMode_I4);
                    }
                } // if not idr, we can wait in P pre-encode
            }
            *datalen  = size;
            ret = AMVENC_NEW_IDR;
            ALOGV("start_intra_full: done size: %d. fd:%d",size, p->fd);
        }
    }else {
        ALOGE("start_intra_full: encode timeout, status:%d. fd:%d",status, p->fd);
        ret = AMVENC_TIMEOUT;
    }

    if (ret == AMVENC_NEW_IDR) {
        if (pre_encode == 0)
            p->total_encode_frame++;
        if (p->control.logtime) {
            gettimeofday(&p->end_test, NULL);
            total_time = (p->end_test.tv_sec - p->start_test.tv_sec)*1000000 + p->end_test.tv_usec -p->start_test.tv_usec;
            p->total_encode_time +=total_time;
            ALOGD("start_intra_full: need time: %d us. fd:%d",total_time, p->fd);
        }
    }
    return ret;
}

static void get_prop_control(m8venc_drv_t* p)
{
    char prop[PROPERTY_VALUE_MAX];
    int value = 0;

    value = 1;
    memset(prop,0,sizeof(prop));
    if (property_get("hw.encoder.half_pixel", prop, "0") > 0) {
        sscanf(prop,"%d",&value);
        if (value == 0) {
            p->control.half_pixel_mode= false;
            ALOGD("Disable half pixel mode. fd:%d", p->fd);
        }
    }
    value = 1;
    memset(prop,0,sizeof(prop));
    if (property_get("hw.encoder.multi_mv", prop, NULL) > 0) {
        sscanf(prop,"%d",&value);
        if (value == 1) {
            p->control.multi_mv = true;
            ALOGD("Enable multi mv. fd:%d", p->fd);
        }
    }
    value = 0;
    memset(prop,0,sizeof(prop));
    if (property_get("hw.encoder.log.flag", prop, NULL) > 0) {
        sscanf(prop,"%d",&value);
        if (value&0x8) {
            ALOGD("Enable Debug Time Log. fd:%d", p->fd);
            p->control.logtime = true;
        }
    }

    value = -1;
    memset(prop,0,sizeof(prop));
    if (property_get("hw.encoder.fix_qp", prop, NULL) > 0) {
        sscanf(prop,"%d",&value);
        if ((value >= 0) && (value < 51)) {
            p->control.fix_qp = value;
            ALOGD("Enable fix qp mode: %d. fd:%d", p->control.fix_qp, p->fd);
        }
    }

    value = 1;
    memset(prop,0,sizeof(prop));
    if (property_get("hw.encoder.mv_correct", prop, NULL) > 0) {
        sscanf(prop,"%d",&value);
        if (value == 0) {
            p->control.mv_correction = false;
            ALOGD("Disable mv correction. fd:%d", p->fd);
        }
    }

    value = 0;
    memset(prop,0,sizeof(prop));
    if (property_get("hw.encoder.i16_search", prop, "2") > 0) {
        sscanf(prop,"%d",&value);
        if ((value <= 2) && (value >= 0)) {
            p->control.i16_searchmode = value;
            ALOGD("i16 search mode:%d, fd:%d", p->control.i16_searchmode, p->fd);
        }
    }

    value = 0;
    memset(prop,0,sizeof(prop));
    if (property_get("hw.encoder.thread_num", prop, "3") > 0) {
        sscanf(prop,"%d",&value);
        if ((value <= 4) && (value >= 1)) {
            p->control.thread_num = value;
            ALOGD("Set thread num:%d, fd:%d", p->control.thread_num, p->fd);
        }
    }

    value = -1;
    memset(prop,0,sizeof(prop));
    if (property_get("hw.encoder.min_qp", prop, NULL) > 0) {
        sscanf(prop,"%d",&value);
        if ((value >= 0) && (value < 51)) {
            p->control.min_qp = value;
            ALOGD("Set min qp: %d. fd:%d", p->control.min_qp, p->fd);
        }
    }

    value = -1;
    memset(prop,0,sizeof(prop));
    if (property_get("hw.encoder.max_qp", prop, NULL) > 0) {
        sscanf(prop,"%d",&value);
        if ((value >= 0) && (value < 51)) {
            p->control.max_qp= value;
            ALOGD("Set max qp: %d. fd:%d", p->control.max_qp, p->fd);
        }
    }
    if (p->control.max_qp<p->control.min_qp) {
        ALOGD("Set max and min qp wrong, ,max: %d, min:%d. use default value.  fd:%d", p->control.max_qp, p->control.min_qp, p->fd);
        p->control.min_qp= QP_MIN;
        p->control.max_qp =  QP_MAX;
    }
    return;
}

static void FreeEncodeBuffer(m8venc_drv_t* p)
{
    if (p->mmap_buff.addr)
        munmap(p->mmap_buff.addr ,p->mmap_buff.size);
    p->mmap_buff.addr = NULL;
    if (p->mb_ref_buffer[0])
        free(p->mb_ref_buffer[0]);
    p->mb_ref_buffer[0] = NULL;
    if (p->mb_ref_buffer[1])
        free(p->mb_ref_buffer[1]);
    p->mb_ref_buffer[1] = NULL;
    if (p->src.crop_buffer)
        free(p->src.crop_buffer);
    p->src.crop_buffer = NULL;
    return;
}

void* InitM8VEncode(int fd, amvenc_initpara_t* init_para)
{
    int ret = 0;
    unsigned buff_info[30];
    unsigned mode = UCODE_MODE_FULL;
    int rows_per_slice = 0;
    m8venc_drv_t* p = NULL;
    int i = 0;

    if (!init_para) {
        ALOGE("InitM8VEncode init para error, fd:%d",fd);
        return NULL;
    }

    p = (m8venc_drv_t*)calloc(1,sizeof(m8venc_drv_t));
    if (!p) {
        ALOGE("InitM8VEncode calloc fail, fd:%d",fd);
        return NULL;
    }

    memset(p,0,sizeof(m8venc_drv_t));
    p->fd = fd;
    if (p->fd < 0) {
        ALOGE("InitM8VEncode open encode device fail. fd:%d", p->fd);
        free(p);
        return NULL;
    }

    memset(buff_info,0,sizeof(buff_info));
    ret = ioctl(p->fd, M8VENC_AVC_IOC_GET_BUFFINFO,&buff_info[0]);
    if ((ret) || (buff_info[0] == 0)) {
        ALOGE("InitM8VEncode -- old venc driver. no buffer information! fd:%d", p->fd);
        free(p);
        return NULL;
    }

    p->mmap_buff.addr = (unsigned char*)mmap(0,buff_info[0], PROT_READ|PROT_WRITE , MAP_SHARED ,p->fd, 0);
    if (p->mmap_buff.addr == MAP_FAILED) {
        ALOGE("InitM8VEncode mmap fail. fd:%d", p->fd);
        free(p);
        return NULL;
    }

    p->quant = init_para->initQP;
    p->enc_width = init_para->enc_width;
    p->enc_height = init_para->enc_height;
    p->MBsIntraRefresh = init_para->MBsIntraRefresh;
    p->MBsIntraOverlap = init_para->MBsIntraOverlap;
    p->NextIntraRefreshStart = 0;
    p->NextIntraRefreshEnd = 0;
    p->mmap_buff.size = buff_info[0];
    p->src.pix_width= init_para->enc_width;
    p->src.pix_height= init_para->enc_height;
    p->src.mb_width = (init_para->enc_width+15)>>4;
    p->src.mb_height= (init_para->enc_height+15)>>4;
    p->src.mbsize = p->src.mb_height*p->src.mb_width;
    p->mb_buffer = NULL;
    p->mb_ref_buffer[0] = (mb_info_t *)malloc(p->src.mbsize * sizeof(mb_info_t));
    if (p->mb_ref_buffer[0] == NULL) {
        ALOGE("ALLOC mb info memory failed. fd:%d", p->fd);
        FreeEncodeBuffer(p);
        free(p);
        return NULL;
    }
    p->mb_ref_buffer[1] = (mb_info_t *)malloc(p->src.mbsize * sizeof(mb_info_t));
    if (p->mb_ref_buffer[1] == NULL) {
        ALOGE("ALLOC mb info memory failed. fd:%d",p->fd);
        FreeEncodeBuffer(p);
        free(p);
        return NULL;
    }
    p->mb_buffer_id  = 0;
    p->mb_buffer = p->mb_ref_buffer[p->mb_buffer_id];
    p->sps_len = 0;
    p->gotSPS = false;
    p->pps_len = 0;
    p->gotPPS = false;
    p->ref_info.width = init_para->enc_width;
    p->ref_info.height = init_para->enc_height;
    p->ref_info.pitch = init_para->enc_width;
    p->ref_info.size = init_para->enc_width*init_para->enc_height*3/2;
    p->ref_info.y = NULL;
    p->ref_info.uv = NULL;

    p->control.half_pixel_mode = false;
    p->control.fix_qp = -1;
    p->control.multi_mv = false;
    p->control.mv_correction = false;
    p->control.i16_searchmode = 0;
    p->control.thread_num = 3;
    p->control.logtime =  false;
    p->control.min_qp= QP_MIN;
    p->control.max_qp =  QP_MAX;
    get_prop_control(p);

    if (p->enc_width*p->enc_height <=  640*480)
        p->control.i16_searchmode = 0;
    p->mb_statics.last_average_qp = init_para->initQP;
    p->mb_statics.gop_length = 1;
    p->mb_statics.gop_average_qp = init_para->initQP;

    if (InitMBIntraSearchModule_m8(p) < 0) {
        ALOGE("Alloc MBIntrasearch info memroy failed. fd:%d",p->fd);
        FreeEncodeBuffer(p);
        free(p);
        return NULL;
    }
    rows_per_slice = init_para->nSliceHeaderSpacing/p->src.mb_width;
    if (rows_per_slice >0 && rows_per_slice < p->src.mb_height)
        p->rows_per_slice = rows_per_slice;
    else
        p->rows_per_slice = p->src.mb_height;

    buff_info[0] = mode;
    buff_info[1] = p->rows_per_slice;//12;
    buff_info[2] = p->enc_width;
    buff_info[3] = p->enc_height;
    ret = ioctl(p->fd, M8VENC_AVC_IOC_CONFIG_INIT,&buff_info[0]);
    if (ret) {
        ALOGE("InitM8Encode config init fail. fd:%d",p->fd);
        CleanMBIntraSearchModule_m8(p);
        FreeEncodeBuffer(p);
        free(p);
        return  NULL;
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
    p->inter_bits_info.addr = p->mmap_buff.addr +buff_info[13] ;
    p->inter_bits_info.size = buff_info[14];
    p->inter_mv_info.addr = p->mmap_buff.addr +buff_info[15] ;
    p->inter_mv_info.size = buff_info[16];

    p->intra_bits_info.addr = p->mmap_buff.addr +buff_info[17] ;
    p->intra_bits_info.size = buff_info[18];
    p->intra_pred_info.addr = p->mmap_buff.addr +buff_info[19] ;
    p->intra_pred_info.size = buff_info[20];

    p->qp_info.addr = p->mmap_buff.addr +buff_info[21] ;
    p->qp_info.size = buff_info[22];
    p->scale_buff.addr = p->mmap_buff.addr +buff_info[23] ;
    p->scale_buff.size = buff_info[24];

    p->src.crop_buffer = NULL;
    if (init_para->enc_height%16 != 0) {
        p->src.crop_buffer = (unsigned char *)malloc(p->src.mbsize*256*3*sizeof(unsigned char));
        if (p->src.crop_buffer == NULL) {
            ALOGE("InitM8Encode malloc crop buffer. fd:%d",p->fd);
            CleanMBIntraSearchModule_m8(p);
            FreeEncodeBuffer(p);
            free(p);
            return  NULL;
        }
    }

    if (p->src.mb_height<p->control.thread_num) {
        ALOGD("Thread_num :%d is bigger than mb height :%d",p->control.thread_num,p->src.mb_height);
        p->control.thread_num = p->src.mb_height;
    }
    typedef void* (*THREAD_FUN)(void*) ;
    THREAD_FUN thread_fun[4];
    void *dummy = NULL;
    pthread_attr_t attr;

    for (i =0; i< p->control.thread_num ; i++) {
        sem_init(&(p->slot[i].semdone), 0,0);
        sem_init(&(p->slot[i].semstart), 0,0);
        if ((init_para->enc_width <= 640) && (init_para->enc_height <= 480) && (p->control.half_pixel_mode == true)) {
            p->slot[i].ref_mb_buff = (unsigned char *)malloc(384 * sizeof(unsigned char));
            ALOGV("p->slot[%d].ref_mb_buff : 0x%x. fd:%d", i,(unsigned)p->slot[i].ref_mb_buff, p->fd);
        }
    }
    thread_fun[0] = Search_Thread_0;
    thread_fun[1] = Search_Thread_1;
    thread_fun[2] = Search_Thread_2;
    thread_fun[3] = Search_Thread_3;
    for (i = 0;i<p->control.thread_num;i++) {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        if (pthread_create(&(p->slot[i].mThread), &attr, thread_fun[i], p) != 0) {
            int j = 0;
            pthread_attr_destroy(&attr);
            for (j = 0; j < i ;j++) {
                sem_post(&(p->slot[j].semstart));
                pthread_join(p->slot[j].mThread, &dummy);
                sem_destroy(&(p->slot[j].semdone));
                sem_destroy(&(p->slot[j].semstart));
                if (p->slot[j].ref_mb_buff)
                    free(p->slot[j].ref_mb_buff);
                p->slot[j].ref_mb_buff = NULL;
            }
            ALOGE("InitM8Encode create thread fail. fd:%d", p->fd);
            CleanMBIntraSearchModule_m8(p);
            FreeEncodeBuffer(p);
            free(p);
            return NULL;
        }
        pthread_attr_destroy(&attr);
    }
    p->mStart = false;
    p->mCancel = false;
    p->total_encode_frame  = 0;
    p->total_encode_time = 0;
    p->scale_enable = false;
    p->timeout_value = ENCODE_DONE_TIMEOUT;
    p->src.plane[0] = 0;
    p->src.mmapsize = 0;
    p->pre_intra = false;
    ret = noise_redution_init(&(p->Prm_Nr), init_para->enc_width, init_para->enc_height);
    if (ret < 0) {
        p->nr_enable = false;
    } else {
        p->nr_enable = true;
    }
    dump_init(init_para->enc_width, init_para->enc_height);
    return (void *)p;
}

AMVEnc_Status M8VEncodeInitFrame(void *dev, unsigned *yuv, AMVEncBufferType type, AMVEncFrameFmt fmt, bool IDRframe)
{
    m8venc_drv_t* p = (m8venc_drv_t*)dev;
    AMVEnc_Status ret = AMVENC_FAIL;
    unsigned total_time = 0;

    if ((!p) || (!yuv)) {
        if (IDRframe == true)
            p->IDRframe = IDRframe;
        return ret;
    }

    if (p->control.logtime)
        gettimeofday(&p->start_test, NULL);

    p->force_skip = false;
    set_input(p, yuv,p->enc_width, p->enc_height, type,fmt, IDRframe);

    p->IDRframe = IDRframe;
    if (p->IDRframe) {
        ret = AMVENC_NEW_IDR;
    } else {
        if (yuv[4] == 1)
            p->force_skip = true;
        ret = AMVENC_SUCCESS;
    }
    if (p->control.logtime) {
        gettimeofday(&p->end_test, NULL);
        total_time = (p->end_test.tv_sec - p->start_test.tv_sec)*1000000 + p->end_test.tv_usec -p->start_test.tv_usec;
        p->total_encode_time +=total_time;
        ALOGD("M8VEncodeInitFrame: need time: %d us, ret:%d. fd:%d",total_time,ret, p->fd);
    }
    return ret;
}

AMVEnc_Status M8VEncodeSPS_PPS(void* dev, unsigned char* outptr,int* datalen)
{
    m8venc_drv_t* p = (m8venc_drv_t*)dev;
    AMVEnc_Status ret = AMVENC_FAIL;
    unsigned status;
    unsigned size = 0;
    unsigned control_info[5];

    control_info[0] = ENCODER_SEQUENCE;
    control_info[1] = UCODE_MODE_FULL;
    control_info[2] = 26; //init qp;
    control_info[3] = AMVENC_FLUSH_FLAG_OUTPUT;
    control_info[4] = 0; // never timeout
    ioctl(p->fd, M8VENC_AVC_IOC_NEW_CMD, &control_info[0]);

    if (encode_poll(p->fd, -1) <= 0) {
        ALOGE("sps pps: poll fail. fd:%d",p->fd);
        return AMVENC_TIMEOUT;
    }

    ioctl(p->fd, M8VENC_AVC_IOC_GET_STAGE, &status);

    ALOGV("M8VEncodeSPS_PPS status:%d. fd:%d", status, p->fd);
    ret = AMVENC_FAIL;
    if (status == ENCODER_PICTURE_DONE) {
        ioctl(p->fd, M8VENC_AVC_IOC_GET_OUTPUT_SIZE, &size);
        p->sps_len = (size >>16)&0xffff;
        p->pps_len = size & 0xffff;
        if (((p->sps_len+ p->pps_len)< p->output_buf.size) && (p->sps_len>0) && (p->pps_len>0)) {
            p->gotSPS = true;
            p->gotPPS= true;
            memcpy(outptr,p->output_buf.addr,p->pps_len+p->sps_len);
            *datalen  = p->pps_len+p->sps_len;
            ret = AMVENC_SUCCESS;
        }
    } else {
        ALOGE("sps pps timeout, status:%d. fd:%d",status, p->fd);
        ret = AMVENC_TIMEOUT;
    }
    return ret;
}

AMVEnc_Status M8VEncodeSlice(void* dev, unsigned char* outptr,int* datalen)
{
    m8venc_drv_t* p = (m8venc_drv_t*)dev;
    AMVEnc_Status ret = AMVENC_FAIL;
    int status = 0;
    int cur_data_len = *datalen;
    if ((!p) || (!outptr) || (!datalen))
        return ret;

    if (p->control.fix_qp >= 0) {
        p->base_quant = p->control.fix_qp;
        p->quant = p->control.fix_qp;
    } else {
        if (p->IDRframe)
            p->base_quant = p->control.min_qp;
        else
            p->base_quant = p->control.min_qp;
    }

    if (p->IDRframe) {
        ret = start_intra_full(p,outptr,&cur_data_len, 1);
        if ( ret == AMVENC_NEW_IDR) {
            cur_data_len = *datalen;
            ret = start_intra_half(p,outptr,&cur_data_len);
            *datalen = cur_data_len;
        }
    } else {
        ret = start_intra_full(p,outptr,&cur_data_len, 1);
        if (ret == AMVENC_NEW_IDR) {
            if (p->force_skip) {
                ret = start_ime_half(p,outptr,&cur_data_len);
            } else {
                ret = start_ime_full(p,outptr,&cur_data_len, 1);
                if (ret == AMVENC_PICTURE_READY) {
                    ret = start_ime_half(p,outptr,&cur_data_len);
                }
            }
        }
        *datalen = cur_data_len;
    }

    if (p->src.type == CANVAS_BUFFER) {
        if (p->src.plane[0]!= 0 && p->src.mmapsize!= 0)
            munmap((void*)p->src.plane[0] ,p->src.mmapsize);
        p->src.plane[0] = 0;
        p->src.plane[1] = 0;
        p->src.plane[2] = 0;
        p->src.mmapsize = 0;
    }
    wait_search_thread(p);
    return ret;
}

AMVEnc_Status M8VEncodeCommit(void* dev,  bool IDR)
{
    m8venc_drv_t* p = (m8venc_drv_t*)dev;
    AMVEnc_Status ret = AMVENC_FAIL;
    int status = 0;
    if (!p)
        return ret;
    status = (IDR == true)?ENCODER_IDR:ENCODER_NON_IDR;
    if (IDR == true) {
        if (p->MBsIntraRefresh>0) {
            p->NextIntraRefreshStart = 0;
            p->NextIntraRefreshEnd = p->NextIntraRefreshStart+p->MBsIntraRefresh -1;
        }
    } else {
        if (p->MBsIntraRefresh>0) {
            p->NextIntraRefreshStart = p->NextIntraRefreshStart + p->MBsIntraRefresh - p->MBsIntraOverlap;
            if (p->NextIntraRefreshStart >= (unsigned)p->src.mb_width*p->src.mb_height)
                p->NextIntraRefreshStart = 0;
            p->NextIntraRefreshEnd = p->NextIntraRefreshStart+p->MBsIntraRefresh -1;
            if (p->NextIntraRefreshEnd >= (unsigned)p->src.mb_width*p->src.mb_height)
                p->NextIntraRefreshEnd = p->src.mb_width*p->src.mb_height -1;
        }
    }
    if (ioctl(p->fd, M8VENC_AVC_IOC_SUBMIT_ENCODE_DONE ,&status) == 0) {
        unsigned total_time = 0;
        ret = AMVENC_SUCCESS;
        if (p->control.logtime)
            gettimeofday(&p->start_test, NULL);

        p->ref_id = GetM8VencRefBuf(p->fd);
        //unsigned addr_info[3];
        //addr_info[0] = (p->ref_id == 0)?ENCODER_BUFFER_REF0:ENCODER_BUFFER_REF1;
        //addr_info[1] = 0 ;
        //addr_info[2] = ((p->enc_width+31)>>5<<5)*(p->src.mb_height<<4)*3/2;
        //ioctl(p->fd, M8VENC_AVC_IOC_FLUSH_DMA ,addr_info);
        p->ref_info.y = p->ref_buf_y[p->ref_id].addr;
        p->ref_info.uv = p->ref_buf_uv[p->ref_id].addr;
        dump_reference_buffer(p);
        p->mb_buffer_id = p->mb_buffer_id^1;
        p->mb_buffer = p->mb_ref_buffer[p->mb_buffer_id];

        if (p->control.logtime) {
            gettimeofday(&p->end_test, NULL);
            total_time = p->end_test.tv_sec - p->start_test.tv_sec;
            total_time = total_time*1000000 + p->end_test.tv_usec -p->start_test.tv_usec;
            ALOGD("M8VEncodeCommit: flush ref buffer need time: %d us. fd: %d",total_time, p->fd);
            p->total_encode_time += total_time;
        }
    }
    p->pre_intra = IDR;
    return ret;
}

void UnInitM8VEncode(void* dev)
{
    m8venc_drv_t* p = (m8venc_drv_t*)dev;
    int i = 0;
    void *dummy = NULL;
    if (!p)
        return;

    p->mCancel = true;
    for (i = 0;i<p->control.thread_num;i++) {
        sem_post(&(p->slot[i].semstart));
        pthread_join(p->slot[i].mThread, &dummy);
        sem_destroy(&(p->slot[i].semdone));
        sem_destroy(&(p->slot[i].semstart));
        if (p->slot[i].ref_mb_buff)
            free(p->slot[i].ref_mb_buff);
        p->slot[i].ref_mb_buff = NULL;
    }

    CleanMBIntraSearchModule_m8(p);

    if (p->src.type == CANVAS_BUFFER) {
        if (p->src.plane[0] != 0 && p->src.mmapsize != 0) {
            ALOGD("UnInitM8VEncode, release p->src.plane[0]:0x%x. fd:%d",p->src.plane[0], p->fd);
            munmap((void*)p->src.plane[0] ,p->src.mmapsize);
        }
        p->src.plane[0] = 0;
        p->src.plane[1] = 0;
        p->src.plane[2] = 0;
        p->src.mmapsize = 0;
    }

    FreeEncodeBuffer(p);

    dump_release();
    if (p->nr_enable)
        noise_reduction_release(&(p->Prm_Nr));
    p->nr_enable = false;

    if (p->control.logtime)
        ALOGD("total_encode_frame: %d,  total_encode_time: %d ms, fd:%d",p->total_encode_frame,p->total_encode_time/1000, p->fd);
    free(p);
    return;
}

