#ifndef AML_VIDEO_FILL_BUFFER_CPP
#define AML_VIDEO_FILL_BUFFER_CPP

#include "enc_define.h"

typedef unsigned char uint8_t;
typedef unsigned int uint_t;

#ifndef BYTE_ALIGN
#define  BYTE_ALIGN 8
#endif

void fill_i_buffer_spec_neon(unsigned char *cur_mb_y  , unsigned char *cur_mb_u  , unsigned char *cur_mb_v  , unsigned short *input_addr)
{
    unsigned short *q = input_addr;
    unsigned char *src_y = cur_mb_y;
    unsigned char *src_u = cur_mb_u;
    unsigned char *src_v = cur_mb_v;
    unsigned char *dst = (unsigned char *)&q[0];
    asm volatile(
        "mov r0, #0\n\t"
        "vdup.32 q1,r0 \n\t"
        "mov r1, #240\n\t"
        "loop_y:\n\t"
        "vld1.8 {q0},[%[iny]]!\n\t"
        "subs r1, #16\n\t"
        "vst2.8 {q0,q1},[%[out]]!\n\t"
        "bpl loop_y\n\t"
        "mov r1, #48\n\t"
        "loop_u:\n\t"
        "vld1.8 {q0},[%[inu]]!\n\t"
        "subs r1, #16\n\t"
        "vst2.8 {q0,q1},[%[out]]!\n\t"
        "bpl loop_u\n\t"
        "mov r1, #48\n\t"
        "loop_v:\n\t"
        "vld1.8 {q0},[%[inv]]!\n\t"
        "subs r1, #16\n\t"
        "vst2.8 {q0,q1},[%[out]]!\n\t"
        "bpl loop_v\n\t"
        :[out] "+r"(dst)
        :[iny] "r"(src_y), [inu] "r"(src_u), [inv] "r"(src_v)
        :"q0", "q1", "r0", "r1"
    );
}

void Y_ext_asm2(uint8_t *ydst, uint8_t *ysrc, uint_t stride)
{
    stride -= 8;
    asm volatile(
        "vld1.64 {d0}, [%[ysrc]]!              \n\t"
        "vld1.64 {d4}, [%[ysrc]], %[stride]    \n\t"
        "vld1.64 {d1}, [%[ysrc]]!              \n\t"
        "vld1.64 {d5}, [%[ysrc]], %[stride]    \n\t"
        "vld1.64 {d2}, [%[ysrc]]!              \n\t"
        "vld1.64 {d6}, [%[ysrc]], %[stride]    \n\t"
        "vld1.64 {d3}, [%[ysrc]]!              \n\t"
        "vld1.64 {d7}, [%[ysrc]], %[stride]    \n\t"
        "vld1.64 {d8}, [%[ysrc]]!              \n\t"
        "vld1.64 {d12},[%[ysrc]], %[stride]   \n\t"
        "vld1.64 {d9}, [%[ysrc]]!              \n\t"
        "vld1.64 {d13},[%[ysrc]], %[stride]   \n\t"
        "vld1.64 {d10},[%[ysrc]]!             \n\t"
        "vld1.64 {d14},[%[ysrc]], %[stride]   \n\t"
        "vld1.64 {d11},[%[ysrc]]!             \n\t"
        "vld1.64 {d15},[%[ysrc]], %[stride]   \n\t"
        "vswp q2, q4                           \n\t"
        "vswp q3, q5                           \n\t"
        "VMOVL.U8  q8, d0                      \n\t"
        "vst1.64   {d16}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q9, d1                      \n\t"
        "vst1.64   {d18}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q10, d2                      \n\t"
        "vst1.64   {d20}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q11, d3                      \n\t"
        "vst1.64   {d22}, [%[ydst]]!           \n\t"
        "vst1.64   {d17}, [%[ydst]]!           \n\t"
        "vst1.64   {d19}, [%[ydst]]!           \n\t"
        "vst1.64   {d21}, [%[ydst]]!           \n\t"
        "vst1.64   {d23}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q8, d4                     \n\t"
        "vst1.64   {d16}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q9, d5                      \n\t"
        "vst1.64   {d18}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q10,d6                      \n\t"
        "vst1.64   {d20}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q11, d7                      \n\t"
        "vst1.64   {d22}, [%[ydst]]!           \n\t"
        "vst1.64   {d17}, [%[ydst]]!           \n\t"
        "vst1.64   {d19}, [%[ydst]]!           \n\t"
        "vst1.64   {d21}, [%[ydst]]!           \n\t"
        "vst1.64   {d23}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q8, d8                     \n\t"
        "vst1.64   {d16}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q9, d9                      \n\t"
        "vst1.64   {d18}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q10,d10                      \n\t"
        "vst1.64   {d20}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q11, d11                    \n\t"
        "vst1.64   {d22}, [%[ydst]]!           \n\t"
        "vst1.64   {d17}, [%[ydst]]!           \n\t"
        "vst1.64   {d19}, [%[ydst]]!           \n\t"
        "vst1.64   {d21}, [%[ydst]]!           \n\t"
        "vst1.64   {d23}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q8, d12                     \n\t"
        "vst1.64   {d16}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q9, d13                      \n\t"
        "vst1.64   {d18}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q10,d14                      \n\t"
        "vst1.64   {d20}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q11, d15                      \n\t"
        "vst1.64   {d22}, [%[ydst]]!           \n\t"
        "vst1.64   {d17}, [%[ydst]]!           \n\t"
        "vst1.64   {d19}, [%[ydst]]!           \n\t"
        "vst1.64   {d21}, [%[ydst]]!           \n\t"
        "vst1.64   {d23}, [%[ydst]]!           \n\t"
        "vld1.64 {d0}, [%[ysrc]]!              \n\t"
        "vld1.64 {d4}, [%[ysrc]], %[stride]    \n\t"
        "vld1.64 {d1}, [%[ysrc]]!              \n\t"
        "vld1.64 {d5}, [%[ysrc]], %[stride]    \n\t"
        "vld1.64 {d2}, [%[ysrc]]!              \n\t"
        "vld1.64 {d6}, [%[ysrc]], %[stride]    \n\t"
        "vld1.64 {d3}, [%[ysrc]]!              \n\t"
        "vld1.64 {d7}, [%[ysrc]], %[stride]    \n\t"
        "vld1.64 {d8}, [%[ysrc]]!              \n\t"
        "vld1.64 {d12}, [%[ysrc]], %[stride]   \n\t"
        "vld1.64 {d9}, [%[ysrc]]!              \n\t"
        "vld1.64 {d13}, [%[ysrc]], %[stride]   \n\t"
        "vld1.64 {d10}, [%[ysrc]]!             \n\t"
        "vld1.64 {d14}, [%[ysrc]], %[stride]   \n\t"
        "vld1.64 {d11}, [%[ysrc]]!             \n\t"
        "vld1.64 {d15}, [%[ysrc]], %[stride]   \n\t"
        "vswp q2, q4                            \n\t"
        "vswp q3, q5                            \n\t"
        "VMOVL.U8  q8, d0                      \n\t"
        "vst1.64   {d16}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q9, d1                      \n\t"
        "vst1.64   {d18}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q10, d2                      \n\t"
        "vst1.64   {d20}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q11, d3                      \n\t"
        "vst1.64   {d22}, [%[ydst]]!           \n\t"
        "vst1.64   {d17}, [%[ydst]]!           \n\t"
        "vst1.64   {d19}, [%[ydst]]!           \n\t"
        "vst1.64   {d21}, [%[ydst]]!           \n\t"
        "vst1.64   {d23}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q8, d4                     \n\t"
        "vst1.64   {d16}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q9, d5                      \n\t"
        "vst1.64   {d18}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q10,d6                      \n\t"
        "vst1.64   {d20}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q11, d7                      \n\t"
        "vst1.64   {d22}, [%[ydst]]!           \n\t"
        "vst1.64   {d17}, [%[ydst]]!           \n\t"
        "vst1.64   {d19}, [%[ydst]]!           \n\t"
        "vst1.64   {d21}, [%[ydst]]!           \n\t"
        "vst1.64   {d23}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q8, d8                     \n\t"
        "vst1.64   {d16}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q9, d9                      \n\t"
        "vst1.64   {d18}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q10,d10                      \n\t"
        "vst1.64   {d20}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q11, d11                    \n\t"
        "vst1.64   {d22}, [%[ydst]]!           \n\t"
        "vst1.64   {d17}, [%[ydst]]!           \n\t"
        "vst1.64   {d19}, [%[ydst]]!           \n\t"
        "vst1.64   {d21}, [%[ydst]]!           \n\t"
        "vst1.64   {d23}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q8, d12                     \n\t"
        "vst1.64   {d16}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q9, d13                      \n\t"
        "vst1.64   {d18}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q10,d14                      \n\t"
        "vst1.64   {d20}, [%[ydst]]!           \n\t"
        "VMOVL.U8  q11, d15                      \n\t"
        "vst1.64   {d22}, [%[ydst]]!           \n\t"
        "vst1.64   {d17}, [%[ydst]]!           \n\t"
        "vst1.64   {d19}, [%[ydst]]!           \n\t"
        "vst1.64   {d21}, [%[ydst]]!           \n\t"
        "vst1.64   {d23}, [%[ydst]]!           \n\t"
        : [ydst] "+r"(ydst)
        : [ysrc] "r"(ysrc), [stride] "r"(stride)
        : "cc",  "memory",  "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7",
        "q8", "q9", "q10", "q11"
    );
}

void YV12_UV_ext_asm2(unsigned short *dst, uint8_t *usrc, uint8_t *vsrc, uint_t stride)
{
    unsigned short *udst;
    unsigned short *vdst;
    udst = &dst[256];
    vdst = &dst[320];
    stride = stride / 2;
    asm volatile(
        "vld1.64 {d0}, [%[usrc]], %[stride]     \n\t"
        "vld1.64 {d1}, [%[usrc]], %[stride]     \n\t"
        "vld1.64 {d2}, [%[usrc]], %[stride]     \n\t"
        "vld1.64 {d3}, [%[usrc]], %[stride]     \n\t"
        "vld1.64 {d4}, [%[usrc]], %[stride]     \n\t"
        "vld1.64 {d5}, [%[usrc]], %[stride]     \n\t"
        "vld1.64 {d6}, [%[usrc]], %[stride]     \n\t"
        "vld1.64 {d7}, [%[usrc]], %[stride]     \n\t"
        "VMOVL.U8  q4, d0                       \n\t"
        "vst1.64   {d8}, [%[udst]]!           \n\t"
        "VMOVL.U8  q5, d1                      \n\t"
        "vst1.64   {d10}, [%[udst]]!           \n\t"
        "VMOVL.U8  q6, d2                      \n\t"
        "vst1.64   {d12}, [%[udst]]!           \n\t"
        "VMOVL.U8  q7, d3                      \n\t"
        "vst1.64   {d14}, [%[udst]]!           \n\t"
        "vst1.64   {d9}, [%[udst]]!           \n\t"
        "vst1.64   {d11}, [%[udst]]!           \n\t"
        "vst1.64   {d13}, [%[udst]]!           \n\t"
        "vst1.64   {d15}, [%[udst]]!           \n\t"
        "VMOVL.U8  q4, d4                     \n\t"
        "vst1.64   {d8}, [%[udst]]!           \n\t"
        "VMOVL.U8  q5, d5                      \n\t"
        "vst1.64   {d10}, [%[udst]]!           \n\t"
        "VMOVL.U8  q6, d6                      \n\t"
        "vst1.64   {d12}, [%[udst]]!           \n\t"
        "VMOVL.U8  q7, d7                      \n\t"
        "vst1.64   {d14}, [%[udst]]!           \n\t"
        "vst1.64   {d9}, [%[udst]]!           \n\t"
        "vst1.64   {d11}, [%[udst]]!           \n\t"
        "vst1.64   {d13}, [%[udst]]!           \n\t"
        "vst1.64   {d15}, [%[udst]]!           \n\t"
        "vld1.64 {d0}, [%[vsrc]], %[stride]      \n\t"
        "vld1.64 {d1}, [%[vsrc]], %[stride]      \n\t"
        "vld1.64 {d2}, [%[vsrc]], %[stride]      \n\t"
        "vld1.64 {d3}, [%[vsrc]], %[stride]      \n\t"
        "vld1.64 {d4}, [%[vsrc]], %[stride]      \n\t"
        "vld1.64 {d5}, [%[vsrc]], %[stride]      \n\t"
        "vld1.64 {d6}, [%[vsrc]], %[stride]      \n\t"
        "vld1.64 {d7}, [%[vsrc]], %[stride]      \n\t"
        "VMOVL.U8  q4, d0                     \n\t"
        "vst1.64   {d8}, [%[vdst]]!           \n\t"
        "VMOVL.U8  q5, d1                      \n\t"
        "vst1.64   {d10}, [%[vdst]]!           \n\t"
        "VMOVL.U8  q6, d2                      \n\t"
        "vst1.64   {d12}, [%[vdst]]!           \n\t"
        "VMOVL.U8  q7, d3                      \n\t"
        "vst1.64   {d14}, [%[vdst]]!           \n\t"
        "vst1.64   {d9}, [%[vdst]]!           \n\t"
        "vst1.64   {d11}, [%[vdst]]!           \n\t"
        "vst1.64   {d13}, [%[vdst]]!           \n\t"
        "vst1.64   {d15}, [%[vdst]]!           \n\t"
        "VMOVL.U8  q4, d4                     \n\t"
        "vst1.64   {d8}, [%[vdst]]!           \n\t"
        "VMOVL.U8  q5, d5                      \n\t"
        "vst1.64   {d10}, [%[vdst]]!           \n\t"
        "VMOVL.U8  q6, d6                      \n\t"
        "vst1.64   {d12}, [%[vdst]]!           \n\t"
        "VMOVL.U8  q7, d7                      \n\t"
        "vst1.64   {d14}, [%[vdst]]!           \n\t"
        "vst1.64   {d9}, [%[vdst]]!           \n\t"
        "vst1.64   {d11}, [%[vdst]]!           \n\t"
        "vst1.64   {d13}, [%[vdst]]!           \n\t"
        "vst1.64   {d15}, [%[vdst]]!           \n\t"
        : [udst] "+r"(udst), [vdst] "+r"(vdst)
        : [usrc] "r"(usrc), [vsrc] "r"(vsrc), [stride] "r"(stride)
        : "cc", "memory", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7"
    );
}

void NV21_UV_ext_asm2(unsigned short *dst, uint8_t *uvsrc, uint_t stride)
{
    unsigned short *udst;
    unsigned short *vdst;
    udst = &dst[256];
    vdst = &dst[320];
    asm volatile(
        "vld2.8 {d0,d1}, [%[uvsrc]], %[stride]      \n\t"
        "vld2.8 {d2,d3}, [%[uvsrc]], %[stride]      \n\t"
        "vld2.8 {d4,d5}, [%[uvsrc]], %[stride]      \n\t"
        "vld2.8 {d6,d7}, [%[uvsrc]], %[stride]      \n\t"
        "vld2.8 {d8,d9}, [%[uvsrc]], %[stride]      \n\t"
        "vld2.8 {d10,d11}, [%[uvsrc]], %[stride]    \n\t"
        "vld2.8 {d12,d13}, [%[uvsrc]], %[stride]    \n\t"
        "vld2.8 {d14,d15}, [%[uvsrc]], %[stride]    \n\t"
        "VMOVL.U8  q8, d0                      \n\t"
        "vst1.64   {d16}, [%[vdst]]!           \n\t"
        "VMOVL.U8  q9, d2                      \n\t"
        "vst1.64   {d18}, [%[vdst]]!           \n\t"
        "VMOVL.U8  q10, d4                      \n\t"
        "vst1.64   {d20}, [%[vdst]]!           \n\t"
        "VMOVL.U8  q11, d6                      \n\t"
        "vst1.64   {d22}, [%[vdst]]!           \n\t"
        "vst1.64   {d17}, [%[vdst]]!           \n\t"
        "vst1.64   {d19}, [%[vdst]]!           \n\t"
        "vst1.64   {d21}, [%[vdst]]!           \n\t"
        "vst1.64   {d23}, [%[vdst]]!           \n\t"
        "VMOVL.U8  q8, d8                     \n\t"
        "vst1.64   {d16}, [%[vdst]]!           \n\t"
        "VMOVL.U8  q9, d10                      \n\t"
        "vst1.64   {d18}, [%[vdst]]!           \n\t"
        "VMOVL.U8  q10, d12                      \n\t"
        "vst1.64   {d20}, [%[vdst]]!           \n\t"
        "VMOVL.U8  q11, d14                      \n\t"
        "vst1.64   {d22}, [%[vdst]]!           \n\t"
        "vst1.64   {d17}, [%[vdst]]!           \n\t"
        "vst1.64   {d19}, [%[vdst]]!           \n\t"
        "vst1.64   {d21}, [%[vdst]]!           \n\t"
        "vst1.64   {d23}, [%[vdst]]!           \n\t"
        "VMOVL.U8  q8, d1                    \n\t"
        "vst1.64   {d16}, [%[udst]]!           \n\t"
        "VMOVL.U8  q9, d3                      \n\t"
        "vst1.64   {d18}, [%[udst]]!           \n\t"
        "VMOVL.U8  q10, d5                      \n\t"
        "vst1.64   {d20}, [%[udst]]!           \n\t"
        "VMOVL.U8  q11, d7                      \n\t"
        "vst1.64   {d22}, [%[udst]]!           \n\t"
        "vst1.64   {d17}, [%[udst]]!           \n\t"
        "vst1.64   {d19}, [%[udst]]!           \n\t"
        "vst1.64   {d21}, [%[udst]]!           \n\t"
        "vst1.64   {d23}, [%[udst]]!           \n\t"
        "VMOVL.U8  q8, d9                    \n\t"
        "vst1.64   {d16}, [%[udst]]!           \n\t"
        "VMOVL.U8  q9, d11                      \n\t"
        "vst1.64   {d18}, [%[udst]]!           \n\t"
        "VMOVL.U8  q10, d13                      \n\t"
        "vst1.64   {d20}, [%[udst]]!           \n\t"
        "VMOVL.U8  q11, d15                      \n\t"
        "vst1.64   {d22}, [%[udst]]!           \n\t"
        "vst1.64   {d17}, [%[udst]]!           \n\t"
        "vst1.64   {d19}, [%[udst]]!           \n\t"
        "vst1.64   {d21}, [%[udst]]!           \n\t"
        "vst1.64   {d23}, [%[udst]]!           \n\t"
        : [udst] "+r"(udst), [vdst] "+r"(vdst)
        : [uvsrc] "r"(uvsrc), [stride] "r"(stride)
        : "cc", "memory", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7",
        "q8", "q9", "q10", "q11"
    );
}

inline void pY_ext_asm_shft0(unsigned short *ydst, uint8_t *ysrc, uint8_t *yref,
                             uint_t stride)
{
    uint8_t cycle = 2;
    uint_t ref_stride;
    ref_stride = (((stride + 31) >> 5) << 5);
    asm volatile(
        "0:@two cycle, 128 bytes every cycle    \n\t"
        ////================================
        "vld2.32 {d0,d2}, [%[ysrc]], %[stride]  \n\t"
        "vld2.32 {d1,d3}, [%[yref]], %[ref_stride]  \n\t"
        "vrev32.8 d1, d1                        \n\t"
        "vrev32.8 d3, d3                        \n\t"
        "vswp d1, d3                            \n\t"
        "vsubl.u8 q0, d0, d1                    \n\t"
        "vsubl.u8 q1, d2, d3                    \n\t"
        ////================================
        "vld2.32 {d4,d6}, [%[ysrc]], %[stride]  \n\t"
        "vld2.32 {d5,d7}, [%[yref]], %[ref_stride]  \n\t"
        "vrev32.8 d5, d5                        \n\t"
        "vrev32.8 d7, d7                        \n\t"
        "vswp d5, d7                            \n\t"
        "vsubl.u8 q2,d4, d5                     \n\t"
        "vsubl.u8 q3,d6, d7                     \n\t"
        ////================================
        "vld2.32 {d8,d10},[%[ysrc]], %[stride]  \n\t"
        "vld2.32 {d9,d11},[%[yref]], %[ref_stride]  \n\t"
        "vrev32.8 d9, d9                        \n\t"
        "vrev32.8 d11,d11                       \n\t"
        "vswp d9, d11                           \n\t"
        "vsubl.u8 q4,d8, d9                     \n\t"
        "vsubl.u8 q5,d10,d11                    \n\t"
        ////================================
        "vld2.32 {d12,d14},[%[ysrc]], %[stride] \n\t"
        "vld2.32 {d13,d15},[%[yref]], %[ref_stride] \n\t"
        "vrev32.8 d13,d13                       \n\t"
        "vrev32.8 d15,d15                       \n\t"
        "vswp d13, d15                          \n\t"
        "vsubl.u8 q6,d12,d13                    \n\t"
        "vsubl.u8 q7,d14,d15                    \n\t"
        ////================================
        "vld2.32 {d16,d18},[%[ysrc]], %[stride] \n\t"
        "vld2.32 {d17,d19},[%[yref]], %[ref_stride] \n\t"
        "vrev32.8 d17,d17                       \n\t"
        "vrev32.8 d19,d19                       \n\t"
        "vswp d17, d19                          \n\t"
        "vsubl.u8 q8,d16,d17                    \n\t"
        "vsubl.u8 q9,d18,d19                    \n\t"
        ////================================
        "vld2.32 {d20,d22},[%[ysrc]], %[stride] \n\t"
        "vld2.32 {d21,d23},[%[yref]], %[ref_stride] \n\t"
        "vrev32.8 d21,d21                       \n\t"
        "vrev32.8 d23,d23                       \n\t"
        "vswp d21, d23                          \n\t"
        "vsubl.u8 q10,d20,d21                   \n\t"
        "vsubl.u8 q11,d22,d23                   \n\t"
        ////================================
        "vld2.32 {d24,d26},[%[ysrc]], %[stride] \n\t"
        "vld2.32 {d25,d27},[%[yref]], %[ref_stride] \n\t"
        "vrev32.8 d25,d25                       \n\t"
        "vrev32.8 d27,d27                       \n\t"
        "vswp d25, d27                          \n\t"
        "vsubl.u8 q12,d24,d25                   \n\t"
        "vsubl.u8 q13,d26,d27                   \n\t"
        ////================================
        "vld2.32 {d28,d30},[%[ysrc]], %[stride] \n\t"
        "vld2.32 {d29,d31},[%[yref]], %[ref_stride] \n\t"
        "vrev32.8 d29,d29                       \n\t"
        "vrev32.8 d31,d31                       \n\t"
        "vswp d29, d31                          \n\t"
        "vsubl.u8 q14,d28,d29                   \n\t"
        "vsubl.u8 q15,d30,d31                   \n\t"
        ////================================
        "vst1.64 {d0}, [%[ydst]]!               \n\t"
        "vst1.64 {d4}, [%[ydst]]!               \n\t"
        "vst1.64 {d8}, [%[ydst]]!               \n\t"
        "vst1.64 {d12},[%[ydst]]!               \n\t"
        "vst1.64 {d2}, [%[ydst]]!               \n\t"
        "vst1.64 {d6}, [%[ydst]]!               \n\t"
        "vst1.64 {d10},[%[ydst]]!               \n\t"
        "vst1.64 {d14},[%[ydst]]!               \n\t"
        "vst1.64 {d16},[%[ydst]]!               \n\t"
        "vst1.64 {d20},[%[ydst]]!               \n\t"
        "vst1.64 {d24},[%[ydst]]!               \n\t"
        "vst1.64 {d28},[%[ydst]]!               \n\t"
        "vst1.64 {d18},[%[ydst]]!               \n\t"
        "vst1.64 {d22},[%[ydst]]!               \n\t"
        "vst1.64 {d26},[%[ydst]]!               \n\t"
        "vst1.64 {d30},[%[ydst]]!               \n\t"
        "vst1.64 {d1}, [%[ydst]]!               \n\t"
        "vst1.64 {d5}, [%[ydst]]!               \n\t"
        "vst1.64 {d9}, [%[ydst]]!               \n\t"
        "vst1.64 {d13},[%[ydst]]!               \n\t"
        "vst1.64 {d3}, [%[ydst]]!               \n\t"
        "vst1.64 {d7}, [%[ydst]]!               \n\t"
        "vst1.64 {d11},[%[ydst]]!               \n\t"
        "vst1.64 {d15},[%[ydst]]!               \n\t"
        "vst1.64 {d17},[%[ydst]]!              \n\t"
        "vst1.64 {d21},[%[ydst]]!              \n\t"
        "vst1.64 {d25},[%[ydst]]!              \n\t"
        "vst1.64 {d29},[%[ydst]]!              \n\t"
        "vst1.64 {d19},[%[ydst]]!              \n\t"
        "vst1.64 {d23},[%[ydst]]!              \n\t"
        "vst1.64 {d27},[%[ydst]]!              \n\t"
        "vst1.64 {d31},[%[ydst]]!              \n\t"
        ////================================
        "subs %[cycle], %[cycle], #1           \n\t"
        "bne 0b                                \n\t"
        : [ydst] "+r"(ydst),
        [cycle]"+r"(cycle)
        : [ysrc] "r"(ysrc),
        [yref] "r"(yref), [stride] "r"(stride),
        [ref_stride] "r"(ref_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7",
        "q8", "q9", "q10", "q11",
        "q12", "q13", "q14", "q15"
    );
}

inline void pY_ext_asm_shft1(unsigned short *ydst, uint8_t *ysrc, uint8_t *yref,
                             uint_t stride)
{
    uint8_t cycle = 2;
    uint_t ref_stride;
    ref_stride = (((stride + 31) >> 5) << 5) - 16;
    asm volatile(
        "0:@four cycle\n\t"
        ////================================
        "vld1.64 {d1}, [%[yref]]!               \n\t"
        "vld1.64 {d2}, [%[yref]]!               \n\t"
        "vld1.64 {d3}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d1, d1                       \n\t"
        "vrev64.8  q1, q1                       \n\t"
        "vext.8    d1, d1, d2, #1               \n\t"
        "vext.8    d3, d2, d3, #1               \n\t"
        "vtrn.32   d1, d3                       \n\t"
        "vld2.32 {d0,d2}, [%[ysrc]], %[stride]  \n\t"
        "vsubl.u8 q0, d0, d1                    \n\t"
        "vsubl.u8 q1, d2, d3                    \n\t"
        ////================================
        "vld1.64 {d5}, [%[yref]]!               \n\t"
        "vld1.64 {d6}, [%[yref]]!               \n\t"
        "vld1.64 {d7}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d5, d5                       \n\t"
        "vrev64.8  q3, q3                       \n\t"
        "vext.8    d5, d5, d6, #1               \n\t"
        "vext.8    d7, d6, d7, #1               \n\t"
        "vtrn.32   d5, d7                       \n\t"
        "vld2.32 {d4,d6}, [%[ysrc]], %[stride]  \n\t"
        "vsubl.u8 q2,d4, d5                     \n\t"
        "vsubl.u8 q3,d6, d7                     \n\t"
        ////================================
        "vld1.64 {d9},  [%[yref]]!              \n\t"
        "vld1.64 {d10}, [%[yref]]!              \n\t"
        "vld1.64 {d11}, [%[yref]],%[ref_stride] \n\t"
        "vrev64.8  d9, d9                       \n\t"
        "vrev64.8  q5, q5                       \n\t"
        "vext.8    d9, d9, d10, #1              \n\t"
        "vext.8    d11,d10,d11, #1              \n\t"
        "vtrn.32   d9, d11                      \n\t"
        "vld2.32 {d8,d10},[%[ysrc]], %[stride]  \n\t"
        "vsubl.u8 q4,d8, d9                     \n\t"
        "vsubl.u8 q5,d10,d11                    \n\t"
        ////================================
        "vld1.64 {d13}, [%[yref]]!              \n\t"
        "vld1.64 {d14}, [%[yref]]!              \n\t"
        "vld1.64 {d15}, [%[yref]],%[ref_stride] \n\t"
        "vrev64.8  d13, d13                     \n\t"
        "vrev64.8  q7, q7                       \n\t"
        "vext.8    d13, d13, d14, #1            \n\t"
        "vext.8    d15, d14, d15, #1            \n\t"
        "vtrn.32   d13, d15                     \n\t"
        "vld2.32 {d12,d14},[%[ysrc]], %[stride] \n\t"
        "vsubl.u8 q6,d12,d13                    \n\t"
        "vsubl.u8 q7,d14,d15                    \n\t"
        ////================================
        "vld1.64 {d17}, [%[yref]]!              \n\t"
        "vld1.64 {d18}, [%[yref]]!              \n\t"
        "vld1.64 {d19}, [%[yref]],%[ref_stride] \n\t"
        "vrev64.8  d17, d17                     \n\t"
        "vrev64.8  q9, q9                       \n\t"
        "vext.8    d17, d17, d18, #1            \n\t"
        "vext.8    d19, d18, d19, #1            \n\t"
        "vtrn.32   d17, d19                     \n\t"
        "vld2.32 {d16,d18},[%[ysrc]], %[stride] \n\t"
        "vsubl.u8 q8,d16,d17                    \n\t"
        "vsubl.u8 q9,d18,d19                    \n\t"
        ////================================
        "vld1.64 {d21}, [%[yref]]!              \n\t"
        "vld1.64 {d22}, [%[yref]]!              \n\t"
        "vld1.64 {d23}, [%[yref]],%[ref_stride] \n\t"
        "vrev64.8  d21, d21                     \n\t"
        "vrev64.8  q11, q11                     \n\t"
        "vext.8    d21, d21, d22, #1            \n\t"
        "vext.8    d23, d22, d23, #1            \n\t"
        "vtrn.32   d21, d23                     \n\t"
        "vld2.32 {d20,d22},[%[ysrc]], %[stride] \n\t"
        "vsubl.u8 q10,d20,d21                   \n\t"
        "vsubl.u8 q11,d22,d23                   \n\t"
        ////================================
        "vld1.64 {d25}, [%[yref]]!               \n\t"
        "vld1.64 {d26}, [%[yref]]!               \n\t"
        "vld1.64 {d27}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d25, d25                       \n\t"
        "vrev64.8  q13, q13                       \n\t"
        "vext.8    d25, d25, d26, #1               \n\t"
        "vext.8    d27, d26, d27, #1               \n\t"
        "vtrn.32   d25, d27                       \n\t"
        "vld2.32 {d24,d26},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q12,d24,d25                   \n\t"
        "vsubl.u8 q13,d26,d27                   \n\t"
        ////================================
        "vld1.64 {d29}, [%[yref]]!               \n\t"
        "vld1.64 {d30}, [%[yref]]!               \n\t"
        "vld1.64 {d31}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d29, d29                       \n\t"
        "vrev64.8  q15, q15                       \n\t"
        "vext.8    d29, d29, d30, #1               \n\t"
        "vext.8    d31, d30, d31, #1               \n\t"
        "vtrn.32   d29, d31                       \n\t"
        "vld2.32 {d28,d30},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q14,d28,d29                   \n\t"
        "vsubl.u8 q15,d30,d31                   \n\t"
        ////================================
        "vst1.64 {d0}, [%[ydst]]!              \n\t"
        "vst1.64 {d4}, [%[ydst]]!              \n\t"
        "vst1.64 {d8}, [%[ydst]]!              \n\t"
        "vst1.64 {d12},[%[ydst]]!              \n\t"
        "vst1.64 {d2}, [%[ydst]]!              \n\t"
        "vst1.64 {d6}, [%[ydst]]!              \n\t"
        "vst1.64 {d10},[%[ydst]]!              \n\t"
        "vst1.64 {d14},[%[ydst]]!              \n\t"
        "vst1.64 {d16},[%[ydst]]!              \n\t"
        "vst1.64 {d20},[%[ydst]]!              \n\t"
        "vst1.64 {d24},[%[ydst]]!              \n\t"
        "vst1.64 {d28},[%[ydst]]!              \n\t"
        "vst1.64 {d18},[%[ydst]]!              \n\t"
        "vst1.64 {d22},[%[ydst]]!              \n\t"
        "vst1.64 {d26},[%[ydst]]!              \n\t"
        "vst1.64 {d30},[%[ydst]]!              \n\t"
        "vst1.64 {d1}, [%[ydst]]!              \n\t"
        "vst1.64 {d5}, [%[ydst]]!              \n\t"
        "vst1.64 {d9},[%[ydst]]!              \n\t"
        "vst1.64 {d13},[%[ydst]]!              \n\t"
        "vst1.64 {d3}, [%[ydst]]!              \n\t"
        "vst1.64 {d7}, [%[ydst]]!              \n\t"
        "vst1.64 {d11},[%[ydst]]!              \n\t"
        "vst1.64 {d15},[%[ydst]]!              \n\t"
        "vst1.64 {d17},[%[ydst]]!              \n\t"
        "vst1.64 {d21},[%[ydst]]!              \n\t"
        "vst1.64 {d25},[%[ydst]]!              \n\t"
        "vst1.64 {d29},[%[ydst]]!              \n\t"
        "vst1.64 {d19},[%[ydst]]!              \n\t"
        "vst1.64 {d23},[%[ydst]]!              \n\t"
        "vst1.64 {d27},[%[ydst]]!              \n\t"
        "vst1.64 {d31},[%[ydst]]!              \n\t"
        ////================================
        "subs %[cycle], %[cycle], #1            \n\t"
        "bne 0b                                \n\t"
        : [ydst] "+r"(ydst), [cycle]"+r"(cycle)
        : [ysrc] "r"(ysrc),
        [yref] "r"(yref), [stride] "r"(stride),
        [ref_stride] "r"(ref_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7",
        "q8", "q9", "q10", "q11",
        "q12", "q13", "q14", "q15"
    );
}

inline void pY_ext_asm_shft2(unsigned short *ydst, uint8_t *ysrc, uint8_t *yref,
                             uint_t stride)
{
    uint8_t cycle = 2;
    uint_t ref_stride;
    ref_stride = (((stride + 31) >> 5) << 5) - 16;
    asm volatile(
        "0:@four cycle\n\t"
        ////================================
        "vld1.64 {d1}, [%[yref]]!               \n\t"
        "vld1.64 {d2}, [%[yref]]!               \n\t"
        "vld1.64 {d3}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d1, d1                       \n\t"
        "vrev64.8  q1, q1                       \n\t"
        "vext.8    d1, d1, d2, #2               \n\t"
        "vext.8    d3, d2, d3, #2               \n\t"
        "vtrn.32   d1, d3                       \n\t"
        "vld2.32 {d0,d2}, [%[ysrc]], %[stride]  \n\t"
        "vsubl.u8 q0, d0, d1                    \n\t"
        "vsubl.u8 q1, d2, d3                    \n\t"
        ////================================
        "vld1.64 {d5}, [%[yref]]!               \n\t"
        "vld1.64 {d6}, [%[yref]]!               \n\t"
        "vld1.64 {d7}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d5, d5                       \n\t"
        "vrev64.8  q3, q3                       \n\t"
        "vext.8    d5, d5, d6, #2               \n\t"
        "vext.8    d7, d6, d7, #2               \n\t"
        "vtrn.32   d5, d7                       \n\t"
        "vld2.32 {d4,d6}, [%[ysrc]], %[stride]  \n\t"
        "vsubl.u8 q2,d4, d5                     \n\t"
        "vsubl.u8 q3,d6, d7                     \n\t"
        ////================================
        "vld1.64 {d9},  [%[yref]]!               \n\t"
        "vld1.64 {d10}, [%[yref]]!               \n\t"
        "vld1.64 {d11}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d9, d9                       \n\t"
        "vrev64.8  q5, q5                       \n\t"
        "vext.8    d9, d9, d10, #2               \n\t"
        "vext.8    d11,d10,d11, #2               \n\t"
        "vtrn.32   d9, d11                       \n\t"
        "vld2.32 {d8,d10},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q4,d8, d9                    \n\t"
        "vsubl.u8 q5,d10,d11                   \n\t"
        ////================================
        "vld1.64 {d13}, [%[yref]]!               \n\t"
        "vld1.64 {d14}, [%[yref]]!               \n\t"
        "vld1.64 {d15}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d13, d13                       \n\t"
        "vrev64.8  q7, q7                       \n\t"
        "vext.8    d13, d13, d14, #2               \n\t"
        "vext.8    d15, d14, d15, #2               \n\t"
        "vtrn.32   d13, d15                       \n\t"
        "vld2.32 {d12,d14},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q6,d12,d13                   \n\t"
        "vsubl.u8 q7,d14,d15                   \n\t"
        ////================================
        "vld1.64 {d17}, [%[yref]]!               \n\t"
        "vld1.64 {d18}, [%[yref]]!               \n\t"
        "vld1.64 {d19}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d17, d17                       \n\t"
        "vrev64.8  q9, q9                       \n\t"
        "vext.8    d17, d17, d18, #2               \n\t"
        "vext.8    d19, d18, d19, #2               \n\t"
        "vtrn.32   d17, d19                       \n\t"
        "vld2.32 {d16,d18},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q8,d16,d17                   \n\t"
        "vsubl.u8 q9,d18,d19                   \n\t"
        ////================================
        "vld1.64 {d21}, [%[yref]]!               \n\t"
        "vld1.64 {d22}, [%[yref]]!               \n\t"
        "vld1.64 {d23}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d21, d21                       \n\t"
        "vrev64.8  q11, q11                       \n\t"
        "vext.8    d21, d21, d22, #2               \n\t"
        "vext.8    d23, d22, d23, #2               \n\t"
        "vtrn.32   d21, d23                       \n\t"
        "vld2.32 {d20,d22},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q10,d20,d21                   \n\t"
        "vsubl.u8 q11,d22,d23                   \n\t"
        ////================================
        "vld1.64 {d25}, [%[yref]]!               \n\t"
        "vld1.64 {d26}, [%[yref]]!               \n\t"
        "vld1.64 {d27}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d25, d25                       \n\t"
        "vrev64.8  q13, q13                       \n\t"
        "vext.8    d25, d25, d26, #2               \n\t"
        "vext.8    d27, d26, d27, #2               \n\t"
        "vtrn.32   d25, d27                       \n\t"
        "vld2.32 {d24,d26},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q12,d24,d25                   \n\t"
        "vsubl.u8 q13,d26,d27                   \n\t"
        ////================================
        "vld1.64 {d29}, [%[yref]]!               \n\t"
        "vld1.64 {d30}, [%[yref]]!               \n\t"
        "vld1.64 {d31}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d29, d29                       \n\t"
        "vrev64.8  q15, q15                       \n\t"
        "vext.8    d29, d29, d30, #2               \n\t"
        "vext.8    d31, d30, d31, #2               \n\t"
        "vtrn.32   d29, d31                       \n\t"
        "vld2.32 {d28,d30},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q14,d28,d29                   \n\t"
        "vsubl.u8 q15,d30,d31                   \n\t"
        ////================================
        "vst1.64 {d0}, [%[ydst]]!              \n\t"
        "vst1.64 {d4}, [%[ydst]]!              \n\t"
        "vst1.64 {d8}, [%[ydst]]!              \n\t"
        "vst1.64 {d12},[%[ydst]]!              \n\t"
        "vst1.64 {d2}, [%[ydst]]!              \n\t"
        "vst1.64 {d6}, [%[ydst]]!              \n\t"
        "vst1.64 {d10},[%[ydst]]!              \n\t"
        "vst1.64 {d14},[%[ydst]]!              \n\t"
        "vst1.64 {d16},[%[ydst]]!              \n\t"
        "vst1.64 {d20},[%[ydst]]!              \n\t"
        "vst1.64 {d24},[%[ydst]]!              \n\t"
        "vst1.64 {d28},[%[ydst]]!              \n\t"
        "vst1.64 {d18},[%[ydst]]!              \n\t"
        "vst1.64 {d22},[%[ydst]]!              \n\t"
        "vst1.64 {d26},[%[ydst]]!              \n\t"
        "vst1.64 {d30},[%[ydst]]!              \n\t"
        "vst1.64 {d1}, [%[ydst]]!              \n\t"
        "vst1.64 {d5}, [%[ydst]]!              \n\t"
        "vst1.64 {d9},[%[ydst]]!              \n\t"
        "vst1.64 {d13},[%[ydst]]!              \n\t"
        "vst1.64 {d3}, [%[ydst]]!              \n\t"
        "vst1.64 {d7}, [%[ydst]]!              \n\t"
        "vst1.64 {d11},[%[ydst]]!              \n\t"
        "vst1.64 {d15},[%[ydst]]!              \n\t"
        "vst1.64 {d17},[%[ydst]]!              \n\t"
        "vst1.64 {d21},[%[ydst]]!              \n\t"
        "vst1.64 {d25},[%[ydst]]!              \n\t"
        "vst1.64 {d29},[%[ydst]]!              \n\t"
        "vst1.64 {d19},[%[ydst]]!              \n\t"
        "vst1.64 {d23},[%[ydst]]!              \n\t"
        "vst1.64 {d27},[%[ydst]]!              \n\t"
        "vst1.64 {d31},[%[ydst]]!              \n\t"
        ////================================
        "subs %[cycle], %[cycle], #1            \n\t"
        "bne 0b                                \n\t"
        : [ydst] "+r"(ydst),
        [cycle]"+r"(cycle)
        : [ysrc] "r"(ysrc),
        [yref] "r"(yref), [stride] "r"(stride),
        [ref_stride] "r"(ref_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7",
        "q8", "q9", "q10", "q11",
        "q12", "q13", "q14", "q15"
    );
}

inline void pY_ext_asm_shft3(unsigned short *ydst, uint8_t *ysrc, uint8_t *yref,
                             uint_t stride)
{
    uint8_t cycle = 2;
    uint_t ref_stride;
    ref_stride = (((stride + 31) >> 5) << 5) - 16;
    asm volatile(
        "0:@four cycle\n\t"
        ////================================
        "vld1.64 {d1}, [%[yref]]!               \n\t"
        "vld1.64 {d2}, [%[yref]]!               \n\t"
        "vld1.64 {d3}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d1, d1                       \n\t"
        "vrev64.8  q1, q1                       \n\t"
        "vext.8    d1, d1, d2, #3               \n\t"
        "vext.8    d3, d2, d3, #3               \n\t"
        "vtrn.32   d1, d3                       \n\t"
        "vld2.32 {d0,d2}, [%[ysrc]], %[stride]  \n\t"
        "vsubl.u8 q0, d0, d1                    \n\t"
        "vsubl.u8 q1, d2, d3                    \n\t"
        ////================================
        "vld1.64 {d5}, [%[yref]]!               \n\t"
        "vld1.64 {d6}, [%[yref]]!               \n\t"
        "vld1.64 {d7}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d5, d5                       \n\t"
        "vrev64.8  q3, q3                       \n\t"
        "vext.8    d5, d5, d6, #3               \n\t"
        "vext.8    d7, d6, d7, #3               \n\t"
        "vtrn.32   d5, d7                       \n\t"
        "vld2.32 {d4,d6}, [%[ysrc]], %[stride]  \n\t"
        "vsubl.u8 q2,d4, d5                     \n\t"
        "vsubl.u8 q3,d6, d7                     \n\t"
        ////================================
        "vld1.64 {d9},  [%[yref]]!               \n\t"
        "vld1.64 {d10}, [%[yref]]!               \n\t"
        "vld1.64 {d11}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d9, d9                       \n\t"
        "vrev64.8  q5, q5                       \n\t"
        "vext.8    d9, d9, d10, #3               \n\t"
        "vext.8    d11,d10,d11, #3               \n\t"
        "vtrn.32   d9, d11                       \n\t"
        "vld2.32 {d8,d10},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q4,d8, d9                    \n\t"
        "vsubl.u8 q5,d10,d11                   \n\t"
        ////================================
        "vld1.64 {d13}, [%[yref]]!               \n\t"
        "vld1.64 {d14}, [%[yref]]!               \n\t"
        "vld1.64 {d15}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d13, d13                       \n\t"
        "vrev64.8  q7, q7                       \n\t"
        "vext.8    d13, d13, d14, #3               \n\t"
        "vext.8    d15, d14, d15, #3               \n\t"
        "vtrn.32   d13, d15                       \n\t"
        "vld2.32 {d12,d14},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q6,d12,d13                   \n\t"
        "vsubl.u8 q7,d14,d15                   \n\t"
        ////================================
        "vld1.64 {d17}, [%[yref]]!               \n\t"
        "vld1.64 {d18}, [%[yref]]!               \n\t"
        "vld1.64 {d19}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d17, d17                       \n\t"
        "vrev64.8  q9, q9                       \n\t"
        "vext.8    d17, d17, d18, #3               \n\t"
        "vext.8    d19, d18, d19, #3               \n\t"
        "vtrn.32   d17, d19                       \n\t"
        "vld2.32 {d16,d18},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q8,d16,d17                   \n\t"
        "vsubl.u8 q9,d18,d19                   \n\t"
        ////================================
        "vld1.64 {d21}, [%[yref]]!               \n\t"
        "vld1.64 {d22}, [%[yref]]!               \n\t"
        "vld1.64 {d23}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d21, d21                       \n\t"
        "vrev64.8  q11, q11                       \n\t"
        "vext.8    d21, d21, d22, #3               \n\t"
        "vext.8    d23, d22, d23, #3               \n\t"
        "vtrn.32   d21, d23                       \n\t"
        "vld2.32 {d20,d22},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q10,d20,d21                   \n\t"
        "vsubl.u8 q11,d22,d23                   \n\t"
        ////================================
        "vld1.64 {d25}, [%[yref]]!               \n\t"
        "vld1.64 {d26}, [%[yref]]!               \n\t"
        "vld1.64 {d27}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d25, d25                       \n\t"
        "vrev64.8  q13, q13                       \n\t"
        "vext.8    d25, d25, d26, #3               \n\t"
        "vext.8    d27, d26, d27, #3               \n\t"
        "vtrn.32   d25, d27                       \n\t"
        "vld2.32 {d24,d26},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q12,d24,d25                   \n\t"
        "vsubl.u8 q13,d26,d27                   \n\t"
        ////================================
        "vld1.64 {d29}, [%[yref]]!               \n\t"
        "vld1.64 {d30}, [%[yref]]!               \n\t"
        "vld1.64 {d31}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d29, d29                       \n\t"
        "vrev64.8  q15, q15                       \n\t"
        "vext.8    d29, d29, d30, #3               \n\t"
        "vext.8    d31, d30, d31, #3               \n\t"
        "vtrn.32   d29, d31                       \n\t"
        "vld2.32 {d28,d30},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q14,d28,d29                   \n\t"
        "vsubl.u8 q15,d30,d31                   \n\t"
        ////================================
        "vst1.64 {d0}, [%[ydst]]!              \n\t"
        "vst1.64 {d4}, [%[ydst]]!              \n\t"
        "vst1.64 {d8}, [%[ydst]]!              \n\t"
        "vst1.64 {d12},[%[ydst]]!              \n\t"
        "vst1.64 {d2}, [%[ydst]]!              \n\t"
        "vst1.64 {d6}, [%[ydst]]!              \n\t"
        "vst1.64 {d10},[%[ydst]]!              \n\t"
        "vst1.64 {d14},[%[ydst]]!              \n\t"
        "vst1.64 {d16},[%[ydst]]!              \n\t"
        "vst1.64 {d20},[%[ydst]]!              \n\t"
        "vst1.64 {d24},[%[ydst]]!              \n\t"
        "vst1.64 {d28},[%[ydst]]!              \n\t"
        "vst1.64 {d18},[%[ydst]]!              \n\t"
        "vst1.64 {d22},[%[ydst]]!              \n\t"
        "vst1.64 {d26},[%[ydst]]!              \n\t"
        "vst1.64 {d30},[%[ydst]]!              \n\t"
        "vst1.64 {d1}, [%[ydst]]!              \n\t"
        "vst1.64 {d5}, [%[ydst]]!              \n\t"
        "vst1.64 {d9},[%[ydst]]!              \n\t"
        "vst1.64 {d13},[%[ydst]]!              \n\t"
        "vst1.64 {d3}, [%[ydst]]!              \n\t"
        "vst1.64 {d7}, [%[ydst]]!              \n\t"
        "vst1.64 {d11},[%[ydst]]!              \n\t"
        "vst1.64 {d15},[%[ydst]]!              \n\t"
        "vst1.64 {d17},[%[ydst]]!              \n\t"
        "vst1.64 {d21},[%[ydst]]!              \n\t"
        "vst1.64 {d25},[%[ydst]]!              \n\t"
        "vst1.64 {d29},[%[ydst]]!              \n\t"
        "vst1.64 {d19},[%[ydst]]!              \n\t"
        "vst1.64 {d23},[%[ydst]]!              \n\t"
        "vst1.64 {d27},[%[ydst]]!              \n\t"
        "vst1.64 {d31},[%[ydst]]!              \n\t"
        ////================================
        "subs %[cycle], %[cycle], #1            \n\t"
        "bne 0b                                \n\t"
        : [ydst] "+r"(ydst),
        [cycle]"+r"(cycle)
        : [ysrc] "r"(ysrc),
        [yref] "r"(yref), [stride] "r"(stride),
        [ref_stride] "r"(ref_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7",
        "q8", "q9", "q10", "q11",
        "q12", "q13", "q14", "q15"
    );
}

inline void pY_ext_asm_shft4(unsigned short *ydst, uint8_t *ysrc, uint8_t *yref,
                             uint_t stride)
{
    uint8_t cycle = 2;
    uint_t ref_stride;
    ref_stride = (((stride + 31) >> 5) << 5) - 16;
    asm volatile(
        "0:@four cycle\n\t"
        ////================================
        "vld1.64 {d1}, [%[yref]]!               \n\t"
        "vld1.64 {d2}, [%[yref]]!               \n\t"
        "vld1.64 {d3}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d1, d1                       \n\t"
        "vrev64.8  q1, q1                       \n\t"
        "vext.8    d1, d1, d2, #4               \n\t"
        "vext.8    d3, d2, d3, #4               \n\t"
        "vtrn.32   d1, d3                       \n\t"
        "vld2.32 {d0,d2}, [%[ysrc]], %[stride]  \n\t"
        "vsubl.u8 q0, d0, d1                    \n\t"
        "vsubl.u8 q1, d2, d3                    \n\t"
        ////================================
        "vld1.64 {d5}, [%[yref]]!               \n\t"
        "vld1.64 {d6}, [%[yref]]!               \n\t"
        "vld1.64 {d7}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d5, d5                       \n\t"
        "vrev64.8  q3, q3                       \n\t"
        "vext.8    d5, d5, d6, #4               \n\t"
        "vext.8    d7, d6, d7, #4               \n\t"
        "vtrn.32   d5, d7                       \n\t"
        "vld2.32 {d4,d6}, [%[ysrc]], %[stride]  \n\t"
        "vsubl.u8 q2,d4, d5                     \n\t"
        "vsubl.u8 q3,d6, d7                     \n\t"
        ////================================
        "vld1.64 {d9},  [%[yref]]!               \n\t"
        "vld1.64 {d10}, [%[yref]]!               \n\t"
        "vld1.64 {d11}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d9, d9                       \n\t"
        "vrev64.8  q5, q5                       \n\t"
        "vext.8    d9, d9, d10, #4               \n\t"
        "vext.8    d11,d10,d11, #4               \n\t"
        "vtrn.32   d9, d11                       \n\t"
        "vld2.32 {d8,d10},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q4,d8, d9                    \n\t"
        "vsubl.u8 q5,d10,d11                   \n\t"
        ////================================
        "vld1.64 {d13}, [%[yref]]!               \n\t"
        "vld1.64 {d14}, [%[yref]]!               \n\t"
        "vld1.64 {d15}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d13, d13                       \n\t"
        "vrev64.8  q7, q7                       \n\t"
        "vext.8    d13, d13, d14, #4               \n\t"
        "vext.8    d15, d14, d15, #4               \n\t"
        "vtrn.32   d13, d15                       \n\t"
        "vld2.32 {d12,d14},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q6,d12,d13                   \n\t"
        "vsubl.u8 q7,d14,d15                   \n\t"
        ////================================
        "vld1.64 {d17}, [%[yref]]!               \n\t"
        "vld1.64 {d18}, [%[yref]]!               \n\t"
        "vld1.64 {d19}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d17, d17                       \n\t"
        "vrev64.8  q9, q9                       \n\t"
        "vext.8    d17, d17, d18, #4               \n\t"
        "vext.8    d19, d18, d19, #4               \n\t"
        "vtrn.32   d17, d19                       \n\t"
        "vld2.32 {d16,d18},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q8,d16,d17                   \n\t"
        "vsubl.u8 q9,d18,d19                   \n\t"
        ////================================
        "vld1.64 {d21}, [%[yref]]!               \n\t"
        "vld1.64 {d22}, [%[yref]]!               \n\t"
        "vld1.64 {d23}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d21, d21                       \n\t"
        "vrev64.8  q11, q11                       \n\t"
        "vext.8    d21, d21, d22, #4               \n\t"
        "vext.8    d23, d22, d23, #4               \n\t"
        "vtrn.32   d21, d23                       \n\t"
        "vld2.32 {d20,d22},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q10,d20,d21                   \n\t"
        "vsubl.u8 q11,d22,d23                   \n\t"
        ////================================
        "vld1.64 {d25}, [%[yref]]!               \n\t"
        "vld1.64 {d26}, [%[yref]]!               \n\t"
        "vld1.64 {d27}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d25, d25                       \n\t"
        "vrev64.8  q13, q13                       \n\t"
        "vext.8    d25, d25, d26, #4               \n\t"
        "vext.8    d27, d26, d27, #4               \n\t"
        "vtrn.32   d25, d27                       \n\t"
        "vld2.32 {d24,d26},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q12,d24,d25                   \n\t"
        "vsubl.u8 q13,d26,d27                   \n\t"
        ////================================
        "vld1.64 {d29}, [%[yref]]!               \n\t"
        "vld1.64 {d30}, [%[yref]]!               \n\t"
        "vld1.64 {d31}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d29, d29                       \n\t"
        "vrev64.8  q15, q15                       \n\t"
        "vext.8    d29, d29, d30, #4               \n\t"
        "vext.8    d31, d30, d31, #4               \n\t"
        "vtrn.32   d29, d31                       \n\t"
        "vld2.32 {d28,d30},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q14,d28,d29                   \n\t"
        "vsubl.u8 q15,d30,d31                   \n\t"
        ////================================
        "vst1.64 {d0}, [%[ydst]]!              \n\t"
        "vst1.64 {d4}, [%[ydst]]!              \n\t"
        "vst1.64 {d8}, [%[ydst]]!              \n\t"
        "vst1.64 {d12},[%[ydst]]!              \n\t"
        "vst1.64 {d2}, [%[ydst]]!              \n\t"
        "vst1.64 {d6}, [%[ydst]]!              \n\t"
        "vst1.64 {d10},[%[ydst]]!              \n\t"
        "vst1.64 {d14},[%[ydst]]!              \n\t"
        "vst1.64 {d16},[%[ydst]]!              \n\t"
        "vst1.64 {d20},[%[ydst]]!              \n\t"
        "vst1.64 {d24},[%[ydst]]!              \n\t"
        "vst1.64 {d28},[%[ydst]]!              \n\t"
        "vst1.64 {d18},[%[ydst]]!              \n\t"
        "vst1.64 {d22},[%[ydst]]!              \n\t"
        "vst1.64 {d26},[%[ydst]]!              \n\t"
        "vst1.64 {d30},[%[ydst]]!              \n\t"
        "vst1.64 {d1}, [%[ydst]]!              \n\t"
        "vst1.64 {d5}, [%[ydst]]!              \n\t"
        "vst1.64 {d9},[%[ydst]]!              \n\t"
        "vst1.64 {d13},[%[ydst]]!              \n\t"
        "vst1.64 {d3}, [%[ydst]]!              \n\t"
        "vst1.64 {d7}, [%[ydst]]!              \n\t"
        "vst1.64 {d11},[%[ydst]]!              \n\t"
        "vst1.64 {d15},[%[ydst]]!              \n\t"
        "vst1.64 {d17},[%[ydst]]!              \n\t"
        "vst1.64 {d21},[%[ydst]]!              \n\t"
        "vst1.64 {d25},[%[ydst]]!              \n\t"
        "vst1.64 {d29},[%[ydst]]!              \n\t"
        "vst1.64 {d19},[%[ydst]]!              \n\t"
        "vst1.64 {d23},[%[ydst]]!              \n\t"
        "vst1.64 {d27},[%[ydst]]!              \n\t"
        "vst1.64 {d31},[%[ydst]]!              \n\t"
        ////================================
        "subs %[cycle], %[cycle], #1            \n\t"
        "bne 0b                                \n\t"
        : [ydst] "+r"(ydst),
        [cycle]"+r"(cycle)
        : [ysrc] "r"(ysrc),
        [yref] "r"(yref), [stride] "r"(stride),
        [ref_stride] "r"(ref_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7",
        "q8", "q9", "q10", "q11",
        "q12", "q13", "q14", "q15"
    );
}

inline void pY_ext_asm_shft5(unsigned short *ydst, uint8_t *ysrc, uint8_t *yref,
                             uint_t stride)
{
    uint8_t cycle = 2;
    uint_t ref_stride;
    ref_stride = (((stride + 31) >> 5) << 5) - 16;
    asm volatile(
        "0:@four cycle\n\t"
        ////================================
        "vld1.64 {d1}, [%[yref]]!               \n\t"
        "vld1.64 {d2}, [%[yref]]!               \n\t"
        "vld1.64 {d3}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d1, d1                       \n\t"
        "vrev64.8  q1, q1                       \n\t"
        "vext.8    d1, d1, d2, #5               \n\t"
        "vext.8    d3, d2, d3, #5               \n\t"
        "vtrn.32   d1, d3                       \n\t"
        "vld2.32 {d0,d2}, [%[ysrc]], %[stride]  \n\t"
        "vsubl.u8 q0, d0, d1                    \n\t"
        "vsubl.u8 q1, d2, d3                    \n\t"
        ////================================
        "vld1.64 {d5}, [%[yref]]!               \n\t"
        "vld1.64 {d6}, [%[yref]]!               \n\t"
        "vld1.64 {d7}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d5, d5                       \n\t"
        "vrev64.8  q3, q3                       \n\t"
        "vext.8    d5, d5, d6, #5               \n\t"
        "vext.8    d7, d6, d7, #5               \n\t"
        "vtrn.32   d5, d7                       \n\t"
        "vld2.32 {d4,d6}, [%[ysrc]], %[stride]  \n\t"
        "vsubl.u8 q2,d4, d5                     \n\t"
        "vsubl.u8 q3,d6, d7                     \n\t"
        ////================================
        "vld1.64 {d9},  [%[yref]]!               \n\t"
        "vld1.64 {d10}, [%[yref]]!               \n\t"
        "vld1.64 {d11}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d9, d9                       \n\t"
        "vrev64.8  q5, q5                       \n\t"
        "vext.8    d9, d9, d10, #5               \n\t"
        "vext.8    d11,d10,d11, #5               \n\t"
        "vtrn.32   d9, d11                       \n\t"
        "vld2.32 {d8,d10},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q4,d8, d9                    \n\t"
        "vsubl.u8 q5,d10,d11                   \n\t"
        ////================================
        "vld1.64 {d13}, [%[yref]]!               \n\t"
        "vld1.64 {d14}, [%[yref]]!               \n\t"
        "vld1.64 {d15}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d13, d13                       \n\t"
        "vrev64.8  q7, q7                       \n\t"
        "vext.8    d13, d13, d14, #5               \n\t"
        "vext.8    d15, d14, d15, #5               \n\t"
        "vtrn.32   d13, d15                       \n\t"
        "vld2.32 {d12,d14},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q6,d12,d13                   \n\t"
        "vsubl.u8 q7,d14,d15                   \n\t"
        ////================================
        "vld1.64 {d17}, [%[yref]]!               \n\t"
        "vld1.64 {d18}, [%[yref]]!               \n\t"
        "vld1.64 {d19}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d17, d17                       \n\t"
        "vrev64.8  q9, q9                       \n\t"
        "vext.8    d17, d17, d18, #5               \n\t"
        "vext.8    d19, d18, d19, #5              \n\t"
        "vtrn.32   d17, d19                       \n\t"
        "vld2.32 {d16,d18},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q8,d16,d17                   \n\t"
        "vsubl.u8 q9,d18,d19                   \n\t"
        ////================================
        "vld1.64 {d21}, [%[yref]]!               \n\t"
        "vld1.64 {d22}, [%[yref]]!               \n\t"
        "vld1.64 {d23}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d21, d21                       \n\t"
        "vrev64.8  q11, q11                       \n\t"
        "vext.8    d21, d21, d22, #5               \n\t"
        "vext.8    d23, d22, d23, #5               \n\t"
        "vtrn.32   d21, d23                       \n\t"
        "vld2.32 {d20,d22},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q10,d20,d21                   \n\t"
        "vsubl.u8 q11,d22,d23                   \n\t"
        ////================================
        "vld1.64 {d25}, [%[yref]]!               \n\t"
        "vld1.64 {d26}, [%[yref]]!               \n\t"
        "vld1.64 {d27}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d25, d25                       \n\t"
        "vrev64.8  q13, q13                       \n\t"
        "vext.8    d25, d25, d26, #5               \n\t"
        "vext.8    d27, d26, d27, #5               \n\t"
        "vtrn.32   d25, d27                       \n\t"
        "vld2.32 {d24,d26},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q12,d24,d25                   \n\t"
        "vsubl.u8 q13,d26,d27                   \n\t"
        ////================================
        "vld1.64 {d29}, [%[yref]]!               \n\t"
        "vld1.64 {d30}, [%[yref]]!               \n\t"
        "vld1.64 {d31}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d29, d29                       \n\t"
        "vrev64.8  q15, q15                       \n\t"
        "vext.8    d29, d29, d30, #5               \n\t"
        "vext.8    d31, d30, d31, #5               \n\t"
        "vtrn.32   d29, d31                       \n\t"
        "vld2.32 {d28,d30},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q14,d28,d29                   \n\t"
        "vsubl.u8 q15,d30,d31                   \n\t"
        ////================================
        "vst1.64 {d0}, [%[ydst]]!              \n\t"
        "vst1.64 {d4}, [%[ydst]]!              \n\t"
        "vst1.64 {d8}, [%[ydst]]!              \n\t"
        "vst1.64 {d12},[%[ydst]]!              \n\t"
        "vst1.64 {d2}, [%[ydst]]!              \n\t"
        "vst1.64 {d6}, [%[ydst]]!              \n\t"
        "vst1.64 {d10},[%[ydst]]!              \n\t"
        "vst1.64 {d14},[%[ydst]]!              \n\t"
        "vst1.64 {d16},[%[ydst]]!              \n\t"
        "vst1.64 {d20},[%[ydst]]!              \n\t"
        "vst1.64 {d24},[%[ydst]]!              \n\t"
        "vst1.64 {d28},[%[ydst]]!              \n\t"
        "vst1.64 {d18},[%[ydst]]!              \n\t"
        "vst1.64 {d22},[%[ydst]]!              \n\t"
        "vst1.64 {d26},[%[ydst]]!              \n\t"
        "vst1.64 {d30},[%[ydst]]!              \n\t"
        "vst1.64 {d1}, [%[ydst]]!              \n\t"
        "vst1.64 {d5}, [%[ydst]]!              \n\t"
        "vst1.64 {d9},[%[ydst]]!              \n\t"
        "vst1.64 {d13},[%[ydst]]!              \n\t"
        "vst1.64 {d3}, [%[ydst]]!              \n\t"
        "vst1.64 {d7}, [%[ydst]]!              \n\t"
        "vst1.64 {d11},[%[ydst]]!              \n\t"
        "vst1.64 {d15},[%[ydst]]!              \n\t"
        "vst1.64 {d17},[%[ydst]]!              \n\t"
        "vst1.64 {d21},[%[ydst]]!              \n\t"
        "vst1.64 {d25},[%[ydst]]!              \n\t"
        "vst1.64 {d29},[%[ydst]]!              \n\t"
        "vst1.64 {d19},[%[ydst]]!              \n\t"
        "vst1.64 {d23},[%[ydst]]!              \n\t"
        "vst1.64 {d27},[%[ydst]]!              \n\t"
        "vst1.64 {d31},[%[ydst]]!              \n\t"
        ////================================
        "subs %[cycle], %[cycle], #1            \n\t"
        "bne 0b                                \n\t"
        : [ydst] "+r"(ydst),
        [cycle]"+r"(cycle)
        : [ysrc] "r"(ysrc),
        [yref] "r"(yref), [stride] "r"(stride),
        [ref_stride] "r"(ref_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7",
        "q8", "q9", "q10", "q11",
        "q12", "q13", "q14", "q15"
    );
}

inline void pY_ext_asm_shft6(unsigned short *ydst, uint8_t *ysrc, uint8_t *yref,
                             uint_t stride)
{
    uint8_t cycle = 2;
    uint_t ref_stride;
    ref_stride = (((stride + 31) >> 5) << 5) - 16;
    asm volatile(
        "0:@four cycle\n\t"
        ////================================
        "vld1.64 {d1}, [%[yref]]!               \n\t"
        "vld1.64 {d2}, [%[yref]]!               \n\t"
        "vld1.64 {d3}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d1, d1                       \n\t"
        "vrev64.8  q1, q1                       \n\t"
        "vext.8    d1, d1, d2, #6               \n\t"
        "vext.8    d3, d2, d3, #6               \n\t"
        "vtrn.32   d1, d3                       \n\t"
        "vld2.32 {d0,d2}, [%[ysrc]], %[stride]  \n\t"
        "vsubl.u8 q0, d0, d1                    \n\t"
        "vsubl.u8 q1, d2, d3                    \n\t"
        ////================================
        "vld1.64 {d5}, [%[yref]]!               \n\t"
        "vld1.64 {d6}, [%[yref]]!               \n\t"
        "vld1.64 {d7}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d5, d5                       \n\t"
        "vrev64.8  q3, q3                       \n\t"
        "vext.8    d5, d5, d6, #6               \n\t"
        "vext.8    d7, d6, d7, #6               \n\t"
        "vtrn.32   d5, d7                       \n\t"
        "vld2.32 {d4,d6}, [%[ysrc]], %[stride]  \n\t"
        "vsubl.u8 q2,d4, d5                     \n\t"
        "vsubl.u8 q3,d6, d7                     \n\t"
        ////================================
        "vld1.64 {d9},  [%[yref]]!               \n\t"
        "vld1.64 {d10}, [%[yref]]!               \n\t"
        "vld1.64 {d11}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d9, d9                       \n\t"
        "vrev64.8  q5, q5                       \n\t"
        "vext.8    d9, d9, d10, #6               \n\t"
        "vext.8    d11,d10,d11, #6               \n\t"
        "vtrn.32   d9, d11                       \n\t"
        "vld2.32 {d8,d10},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q4,d8, d9                    \n\t"
        "vsubl.u8 q5,d10,d11                   \n\t"
        ////================================
        "vld1.64 {d13}, [%[yref]]!               \n\t"
        "vld1.64 {d14}, [%[yref]]!               \n\t"
        "vld1.64 {d15}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d13, d13                       \n\t"
        "vrev64.8  q7, q7                       \n\t"
        "vext.8    d13, d13, d14, #6               \n\t"
        "vext.8    d15, d14, d15, #6               \n\t"
        "vtrn.32   d13, d15                       \n\t"
        "vld2.32 {d12,d14},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q6,d12,d13                   \n\t"
        "vsubl.u8 q7,d14,d15                   \n\t"
        ////================================
        "vld1.64 {d17}, [%[yref]]!               \n\t"
        "vld1.64 {d18}, [%[yref]]!               \n\t"
        "vld1.64 {d19}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d17, d17                       \n\t"
        "vrev64.8  q9, q9                       \n\t"
        "vext.8    d17, d17, d18, #6               \n\t"
        "vext.8    d19, d18, d19, #6               \n\t"
        "vtrn.32   d17, d19                       \n\t"
        "vld2.32 {d16,d18},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q8,d16,d17                   \n\t"
        "vsubl.u8 q9,d18,d19                   \n\t"
        ////================================
        "vld1.64 {d21}, [%[yref]]!               \n\t"
        "vld1.64 {d22}, [%[yref]]!               \n\t"
        "vld1.64 {d23}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d21, d21                       \n\t"
        "vrev64.8  q11, q11                       \n\t"
        "vext.8    d21, d21, d22, #6               \n\t"
        "vext.8    d23, d22, d23, #6               \n\t"
        "vtrn.32   d21, d23                       \n\t"
        "vld2.32 {d20,d22},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q10,d20,d21                   \n\t"
        "vsubl.u8 q11,d22,d23                   \n\t"
        ////================================
        "vld1.64 {d25}, [%[yref]]!               \n\t"
        "vld1.64 {d26}, [%[yref]]!               \n\t"
        "vld1.64 {d27}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d25, d25                       \n\t"
        "vrev64.8  q13, q13                       \n\t"
        "vext.8    d25, d25, d26, #6               \n\t"
        "vext.8    d27, d26, d27, #6               \n\t"
        "vtrn.32   d25, d27                       \n\t"
        "vld2.32 {d24,d26},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q12,d24,d25                   \n\t"
        "vsubl.u8 q13,d26,d27                   \n\t"
        ////================================
        "vld1.64 {d29}, [%[yref]]!               \n\t"
        "vld1.64 {d30}, [%[yref]]!               \n\t"
        "vld1.64 {d31}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d29, d29                       \n\t"
        "vrev64.8  q15, q15                       \n\t"
        "vext.8    d29, d29, d30, #6               \n\t"
        "vext.8    d31, d30, d31, #6               \n\t"
        "vtrn.32   d29, d31                       \n\t"
        "vld2.32 {d28,d30},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q14,d28,d29                   \n\t"
        "vsubl.u8 q15,d30,d31                   \n\t"
        ////================================
        "vst1.64 {d0}, [%[ydst]]!              \n\t"
        "vst1.64 {d4}, [%[ydst]]!              \n\t"
        "vst1.64 {d8}, [%[ydst]]!              \n\t"
        "vst1.64 {d12},[%[ydst]]!              \n\t"
        "vst1.64 {d2}, [%[ydst]]!              \n\t"
        "vst1.64 {d6}, [%[ydst]]!              \n\t"
        "vst1.64 {d10},[%[ydst]]!              \n\t"
        "vst1.64 {d14},[%[ydst]]!              \n\t"
        "vst1.64 {d16},[%[ydst]]!              \n\t"
        "vst1.64 {d20},[%[ydst]]!              \n\t"
        "vst1.64 {d24},[%[ydst]]!              \n\t"
        "vst1.64 {d28},[%[ydst]]!              \n\t"
        "vst1.64 {d18},[%[ydst]]!              \n\t"
        "vst1.64 {d22},[%[ydst]]!              \n\t"
        "vst1.64 {d26},[%[ydst]]!              \n\t"
        "vst1.64 {d30},[%[ydst]]!              \n\t"
        "vst1.64 {d1}, [%[ydst]]!              \n\t"
        "vst1.64 {d5}, [%[ydst]]!              \n\t"
        "vst1.64 {d9},[%[ydst]]!              \n\t"
        "vst1.64 {d13},[%[ydst]]!              \n\t"
        "vst1.64 {d3}, [%[ydst]]!              \n\t"
        "vst1.64 {d7}, [%[ydst]]!              \n\t"
        "vst1.64 {d11},[%[ydst]]!              \n\t"
        "vst1.64 {d15},[%[ydst]]!              \n\t"
        "vst1.64 {d17},[%[ydst]]!              \n\t"
        "vst1.64 {d21},[%[ydst]]!              \n\t"
        "vst1.64 {d25},[%[ydst]]!              \n\t"
        "vst1.64 {d29},[%[ydst]]!              \n\t"
        "vst1.64 {d19},[%[ydst]]!              \n\t"
        "vst1.64 {d23},[%[ydst]]!              \n\t"
        "vst1.64 {d27},[%[ydst]]!              \n\t"
        "vst1.64 {d31},[%[ydst]]!              \n\t"
        ////================================
        "subs %[cycle], %[cycle], #1            \n\t"
        "bne 0b                                \n\t"
        : [ydst] "+r"(ydst),
        [cycle]"+r"(cycle)
        : [ysrc] "r"(ysrc),
        [yref] "r"(yref), [stride] "r"(stride),
        [ref_stride] "r"(ref_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7",
        "q8", "q9", "q10", "q11",
        "q12", "q13", "q14", "q15"
    );
}

inline void pY_ext_asm_shft7(unsigned short *ydst, uint8_t *ysrc, uint8_t *yref,
                             uint_t stride)
{
    uint8_t cycle = 2;
    uint_t ref_stride;
    ref_stride = (((stride + 31) >> 5) << 5) - 16;
    asm volatile(
        "0:@four cycle\n\t"
        ////================================
        "vld1.64 {d1}, [%[yref]]!               \n\t"
        "vld1.64 {d2}, [%[yref]]!               \n\t"
        "vld1.64 {d3}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d1, d1                       \n\t"
        "vrev64.8  q1, q1                       \n\t"
        "vext.8    d1, d1, d2, #7               \n\t"
        "vext.8    d3, d2, d3, #7               \n\t"
        "vtrn.32   d1, d3                       \n\t"
        "vld2.32 {d0,d2}, [%[ysrc]], %[stride]  \n\t"
        "vsubl.u8 q0, d0, d1                    \n\t"
        "vsubl.u8 q1, d2, d3                    \n\t"
        ////================================
        "vld1.64 {d5}, [%[yref]]!               \n\t"
        "vld1.64 {d6}, [%[yref]]!               \n\t"
        "vld1.64 {d7}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d5, d5                       \n\t"
        "vrev64.8  q3, q3                       \n\t"
        "vext.8    d5, d5, d6, #7               \n\t"
        "vext.8    d7, d6, d7, #7               \n\t"
        "vtrn.32   d5, d7                       \n\t"
        "vld2.32 {d4,d6}, [%[ysrc]], %[stride]  \n\t"
        "vsubl.u8 q2,d4, d5                     \n\t"
        "vsubl.u8 q3,d6, d7                     \n\t"
        ////================================
        "vld1.64 {d9},  [%[yref]]!               \n\t"
        "vld1.64 {d10}, [%[yref]]!               \n\t"
        "vld1.64 {d11}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d9, d9                       \n\t"
        "vrev64.8  q5, q5                       \n\t"
        "vext.8    d9, d9, d10, #7               \n\t"
        "vext.8    d11,d10,d11, #7               \n\t"
        "vtrn.32   d9, d11                       \n\t"
        "vld2.32 {d8,d10},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q4,d8, d9                    \n\t"
        "vsubl.u8 q5,d10,d11                   \n\t"
        ////================================
        "vld1.64 {d13}, [%[yref]]!               \n\t"
        "vld1.64 {d14}, [%[yref]]!               \n\t"
        "vld1.64 {d15}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d13, d13                       \n\t"
        "vrev64.8  q7, q7                       \n\t"
        "vext.8    d13, d13, d14, #7               \n\t"
        "vext.8    d15, d14, d15, #7               \n\t"
        "vtrn.32   d13, d15                       \n\t"
        "vld2.32 {d12,d14},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q6,d12,d13                   \n\t"
        "vsubl.u8 q7,d14,d15                   \n\t"
        ////================================
        "vld1.64 {d17}, [%[yref]]!               \n\t"
        "vld1.64 {d18}, [%[yref]]!               \n\t"
        "vld1.64 {d19}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d17, d17                       \n\t"
        "vrev64.8  q9, q9                       \n\t"
        "vext.8    d17, d17, d18, #7               \n\t"
        "vext.8    d19, d18, d19, #7               \n\t"
        "vtrn.32   d17, d19                       \n\t"
        "vld2.32 {d16,d18},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q8,d16,d17                   \n\t"
        "vsubl.u8 q9,d18,d19                   \n\t"
        ////================================
        "vld1.64 {d21}, [%[yref]]!               \n\t"
        "vld1.64 {d22}, [%[yref]]!               \n\t"
        "vld1.64 {d23}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d21, d21                       \n\t"
        "vrev64.8  q11, q11                       \n\t"
        "vext.8    d21, d21, d22, #7               \n\t"
        "vext.8    d23, d22, d23, #7               \n\t"
        "vtrn.32   d21, d23                       \n\t"
        "vld2.32 {d20,d22},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q10,d20,d21                   \n\t"
        "vsubl.u8 q11,d22,d23                   \n\t"
        ////================================
        "vld1.64 {d25}, [%[yref]]!               \n\t"
        "vld1.64 {d26}, [%[yref]]!               \n\t"
        "vld1.64 {d27}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d25, d25                       \n\t"
        "vrev64.8  q13, q13                       \n\t"
        "vext.8    d25, d25, d26, #7               \n\t"
        "vext.8    d27, d26, d27, #7               \n\t"
        "vtrn.32   d25, d27                       \n\t"
        "vld2.32 {d24,d26},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q12,d24,d25                   \n\t"
        "vsubl.u8 q13,d26,d27                   \n\t"
        ////================================
        "vld1.64 {d29}, [%[yref]]!               \n\t"
        "vld1.64 {d30}, [%[yref]]!               \n\t"
        "vld1.64 {d31}, [%[yref]],%[ref_stride]  \n\t"
        "vrev64.8  d29, d29                       \n\t"
        "vrev64.8  q15, q15                       \n\t"
        "vext.8    d29, d29, d30, #7               \n\t"
        "vext.8    d31, d30, d31, #7               \n\t"
        "vtrn.32   d29, d31                       \n\t"
        "vld2.32 {d28,d30},[%[ysrc]], %[stride]    \n\t"
        "vsubl.u8 q14,d28,d29                   \n\t"
        "vsubl.u8 q15,d30,d31                   \n\t"
        ////================================
        "vst1.64 {d0}, [%[ydst]]!              \n\t"
        "vst1.64 {d4}, [%[ydst]]!              \n\t"
        "vst1.64 {d8}, [%[ydst]]!              \n\t"
        "vst1.64 {d12},[%[ydst]]!              \n\t"
        "vst1.64 {d2}, [%[ydst]]!              \n\t"
        "vst1.64 {d6}, [%[ydst]]!              \n\t"
        "vst1.64 {d10},[%[ydst]]!              \n\t"
        "vst1.64 {d14},[%[ydst]]!              \n\t"
        "vst1.64 {d16},[%[ydst]]!              \n\t"
        "vst1.64 {d20},[%[ydst]]!              \n\t"
        "vst1.64 {d24},[%[ydst]]!              \n\t"
        "vst1.64 {d28},[%[ydst]]!              \n\t"
        "vst1.64 {d18},[%[ydst]]!              \n\t"
        "vst1.64 {d22},[%[ydst]]!              \n\t"
        "vst1.64 {d26},[%[ydst]]!              \n\t"
        "vst1.64 {d30},[%[ydst]]!              \n\t"
        "vst1.64 {d1}, [%[ydst]]!              \n\t"
        "vst1.64 {d5}, [%[ydst]]!              \n\t"
        "vst1.64 {d9},[%[ydst]]!              \n\t"
        "vst1.64 {d13},[%[ydst]]!              \n\t"
        "vst1.64 {d3}, [%[ydst]]!              \n\t"
        "vst1.64 {d7}, [%[ydst]]!              \n\t"
        "vst1.64 {d11},[%[ydst]]!              \n\t"
        "vst1.64 {d15},[%[ydst]]!              \n\t"
        "vst1.64 {d17},[%[ydst]]!              \n\t"
        "vst1.64 {d21},[%[ydst]]!              \n\t"
        "vst1.64 {d25},[%[ydst]]!              \n\t"
        "vst1.64 {d29},[%[ydst]]!              \n\t"
        "vst1.64 {d19},[%[ydst]]!              \n\t"
        "vst1.64 {d23},[%[ydst]]!              \n\t"
        "vst1.64 {d27},[%[ydst]]!              \n\t"
        "vst1.64 {d31},[%[ydst]]!              \n\t"
        ////================================
        "subs %[cycle], %[cycle], #1            \n\t"
        "bne 0b                                \n\t"
        : [ydst] "+r"(ydst),
        [cycle]"+r"(cycle)
        : [ysrc] "r"(ysrc),
        [yref] "r"(yref), [stride] "r"(stride),
        [ref_stride] "r"(ref_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7",
        "q8", "q9", "q10", "q11",
        "q12", "q13", "q14", "q15"
    );
}

inline void NV21_pUV_ext_asm_shft0(unsigned short *dst, uint8_t *uvsrc, uint8_t *uvref, uint_t stride)
{
    unsigned short *udst;
    unsigned short *vdst;
    uint_t ref_stride = (((stride + 31) >> 5) << 5);
    uint8_t cycle = 2;
    udst = &dst[256];
    vdst = &dst[320];
    asm volatile(
        "0:@two cycle                               \n\t"
        "vld2.8 {d0,d2},  [%[uvsrc]], %[stride]     \n\t"
        "vld2.8 {d1,d3},  [%[uvref]], %[ref_stride]     \n\t"
        //"vswp d0,d2                                      \n\t"
        "vrev32.8 d1, d1                            \n\t"
        "vrev32.8 d3, d3                            \n\t"
        "vsubl.u8 q0,d0,d1                          \n\t"
        "vsubl.u8 q1,d2,d3                          \n\t"
        "vld2.8 {d4,d6},  [%[uvsrc]], %[stride]     \n\t"
        "vld2.8 {d5,d7},  [%[uvref]], %[ref_stride]     \n\t"
        //"vswp d4,d6                                      \n\t"
        "vrev32.8 d5, d5                            \n\t"
        "vrev32.8 d7, d7                            \n\t"
        "vsubl.u8 q2,d4,d5                          \n\t"
        "vsubl.u8 q3,d6,d7                          \n\t"
        "vld2.8 {d8,d10}, [%[uvsrc]], %[stride]     \n\t"
        "vld2.8 {d9,d11}, [%[uvref]], %[ref_stride]     \n\t"
        //"vswp d8,d10                                      \n\t"
        "vrev32.8 d9, d9                            \n\t"
        "vrev32.8 d11,d11                           \n\t"
        "vsubl.u8 q4,d8,d9                          \n\t"
        "vsubl.u8 q5,d10,d11                        \n\t"
        "vld2.8 {d12,d14},[%[uvsrc]], %[stride]     \n\t"
        "vld2.8 {d13,d15},[%[uvref]], %[ref_stride]     \n\t"
        //"vswp d12,d14                                      \n\t"
        "vrev32.8 d13,d13                            \n\t"
        "vrev32.8 d15,d15                           \n\t"
        "vsubl.u8 q6,d12,d13                        \n\t"
        "vsubl.u8 q7,d14,d15                        \n\t"
        "vst1.64 {d0}, [%[vdst]]!                   \n\t"
        "vst1.64 {d4}, [%[vdst]]!                   \n\t"
        "vst1.64 {d8}, [%[vdst]]!                   \n\t"
        "vst1.64 {d12},[%[vdst]]!                   \n\t"
        "vst1.64 {d1}, [%[vdst]]!                   \n\t"
        "vst1.64 {d5}, [%[vdst]]!                   \n\t"
        "vst1.64 {d9}, [%[vdst]]!                   \n\t"
        "vst1.64 {d13},[%[vdst]]!                   \n\t"
        "vst1.64 {d2}, [%[udst]]!                   \n\t"
        "vst1.64 {d6}, [%[udst]]!                   \n\t"
        "vst1.64 {d10},[%[udst]]!                   \n\t"
        "vst1.64 {d14},[%[udst]]!                   \n\t"
        "vst1.64 {d3}, [%[udst]]!                   \n\t"
        "vst1.64 {d7}, [%[udst]]!                   \n\t"
        "vst1.64 {d11},[%[udst]]!                   \n\t"
        "vst1.64 {d15},[%[udst]]!                   \n\t"
        "subs %[cycle], %[cycle], #1                 \n\t"
        "bne 0b"
        : [udst] "+r"(udst), [vdst] "+r"(vdst), [cycle] "+r"(cycle)
        : [uvsrc] "r"(uvsrc), [uvref] "r"(uvref), [stride] "r"(stride),
        [ref_stride] "r"(ref_stride)
        : "cc",  "memory",  "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7"
    );
}

inline void NV21_pUV_ext_asm_shft1(unsigned short *dst, uint8_t *uvsrc, uint8_t *uvref, uint_t stride)
{
    unsigned short *udst;
    unsigned short *vdst;
    uint_t ref_stride = (((stride + 31) >> 5) << 5) - 16;
    uint8_t cycle = 2;
    udst = &dst[256];
    vdst = &dst[320];
    asm volatile(
        "0:@two cycle                               \n\t"
        "vld2.8 {d0,d2},  [%[uvref]]!               \n\t"
        "vld2.8 {d1,d3},  [%[uvref]], %[ref_stride] \n\t"
        //"vswp d0,d2                               \n\t"
        "vrev32.8 q0, q0                            \n\t"
        "vrev32.8 q1, q1                            \n\t"
        "vext.8 d1, d0, d1, #1                      \n\t"
        "vext.8 d3, d2, d3, #1                      \n\t"
        "vld2.8 {d0,d2},  [%[uvsrc]], %[stride]     \n\t"
        "vsubl.u8 q0,d0,d1                          \n\t"
        "vsubl.u8 q1,d2,d3                          \n\t"
        ///============
        "vld2.8 {d4,d6},  [%[uvref]]!               \n\t"
        "vld2.8 {d5,d7},  [%[uvref]], %[ref_stride] \n\t"
        //"vswp d4,d6                                      \n\t"
        "vrev32.8 q2, q2                            \n\t"
        "vrev32.8 q3, q3                            \n\t"
        "vext.8 d5, d4, d5, #1                      \n\t"
        "vext.8 d7, d6, d7, #1                      \n\t"
        "vld2.8 {d4,d6},  [%[uvsrc]], %[stride]     \n\t"
        "vsubl.u8 q2,d4,d5                          \n\t"
        "vsubl.u8 q3,d6,d7                          \n\t"
        ///============
        "vld2.8 {d8, d10}, [%[uvref]]!              \n\t"
        "vld2.8 {d9, d11}, [%[uvref]], %[ref_stride]\n\t"
        //"vswp d8,d10                                      \n\t"
        "vrev32.8 q4, q4                            \n\t"
        "vrev32.8 q5, q5                            \n\t"
        "vext.8 d9, d8, d9, #1                      \n\t"
        "vext.8 d11,d10,d11,#1                      \n\t"
        "vld2.8 {d8,d10}, [%[uvsrc]], %[stride]     \n\t"
        "vsubl.u8 q4,d8,d9                          \n\t"
        "vsubl.u8 q5,d10,d11                        \n\t"
        ///============
        "vld2.8 {d12,d14},[%[uvref]]!               \n\t"
        "vld2.8 {d13,d15},[%[uvref]], %[ref_stride] \n\t"
        //"vswp d12,d14                                      \n\t"
        "vrev32.8 q6, q6                            \n\t"
        "vrev32.8 q7, q7                            \n\t"
        "vext.8 d13,d12,d13,#1                      \n\t"
        "vext.8 d15,d14,d15,#1                      \n\t"
        "vld2.8 {d12,d14},[%[uvsrc]], %[stride]     \n\t"
        "vsubl.u8 q6,d12,d13                        \n\t"
        "vsubl.u8 q7,d14,d15                        \n\t"
        ///============
        "vst1.64 {d0}, [%[vdst]]!                   \n\t"
        "vst1.64 {d4}, [%[vdst]]!                   \n\t"
        "vst1.64 {d8}, [%[vdst]]!                   \n\t"
        "vst1.64 {d12},[%[vdst]]!                   \n\t"
        "vst1.64 {d1}, [%[vdst]]!                   \n\t"
        "vst1.64 {d5}, [%[vdst]]!                   \n\t"
        "vst1.64 {d9}, [%[vdst]]!                   \n\t"
        "vst1.64 {d13},[%[vdst]]!                   \n\t"
        "vst1.64 {d2}, [%[udst]]!                   \n\t"
        "vst1.64 {d6}, [%[udst]]!                   \n\t"
        "vst1.64 {d10},[%[udst]]!                   \n\t"
        "vst1.64 {d14},[%[udst]]!                   \n\t"
        "vst1.64 {d3}, [%[udst]]!                   \n\t"
        "vst1.64 {d7}, [%[udst]]!                   \n\t"
        "vst1.64 {d11},[%[udst]]!                   \n\t"
        "vst1.64 {d15},[%[udst]]!                   \n\t"
        "subs %[cycle], %[cycle], #1                \n\t"
        "bne 0b"
        : [udst] "+r"(udst), [vdst] "+r"(vdst), [cycle] "+r"(cycle)
        : [uvsrc] "r"(uvsrc), [uvref] "r"(uvref),
        [stride] "r"(stride), [ref_stride] "r"(ref_stride)
        : "cc",  "memory",  "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7"
    );
}

inline void NV21_pUV_ext_asm_shft2(unsigned short *dst, uint8_t *uvsrc, uint8_t *uvref, uint_t stride)
{
    unsigned short *udst;
    unsigned short *vdst;
    uint_t ref_stride = (((stride + 31) >> 5) << 5) - 16;
    uint8_t cycle = 2;
    udst = &dst[256];
    vdst = &dst[320];
    asm volatile(
        "0:@two cycle                               \n\t"
        "vld2.8 {d0,d2},  [%[uvref]]!               \n\t"
        "vld2.8 {d1,d3},  [%[uvref]], %[ref_stride] \n\t"
        //"vswp d0,d2                               \n\t"
        "vrev32.8 q0, q0                            \n\t"
        "vrev32.8 q1, q1                            \n\t"
        "vext.8 d1, d0, d1, #2                      \n\t"
        "vext.8 d3, d2, d3, #2                      \n\t"
        "vld2.8 {d0,d2},  [%[uvsrc]], %[stride]     \n\t"
        "vsubl.u8 q0,d0,d1                          \n\t"
        "vsubl.u8 q1,d2,d3                          \n\t"
        ///============
        "vld2.8 {d4,d6},  [%[uvref]]!               \n\t"
        "vld2.8 {d5,d7},  [%[uvref]], %[ref_stride] \n\t"
        //"vswp d4,d6                                      \n\t"
        "vrev32.8 q2, q2                            \n\t"
        "vrev32.8 q3, q3                            \n\t"
        "vext.8 d5, d4, d5, #2                      \n\t"
        "vext.8 d7, d6, d7, #2                      \n\t"
        "vld2.8 {d4,d6},  [%[uvsrc]], %[stride]     \n\t"
        "vsubl.u8 q2,d4,d5                          \n\t"
        "vsubl.u8 q3,d6,d7                          \n\t"
        ///============
        "vld2.8 {d8, d10}, [%[uvref]]!              \n\t"
        "vld2.8 {d9, d11}, [%[uvref]], %[ref_stride]\n\t"
        //"vswp d8,d10                                      \n\t"
        "vrev32.8 q4, q4                            \n\t"
        "vrev32.8 q5, q5                            \n\t"
        "vext.8 d9, d8, d9, #2                      \n\t"
        "vext.8 d11,d10,d11,#2                      \n\t"
        "vld2.8 {d8,d10}, [%[uvsrc]], %[stride]     \n\t"
        "vsubl.u8 q4,d8,d9                          \n\t"
        "vsubl.u8 q5,d10,d11                        \n\t"
        ///============
        "vld2.8 {d12,d14},[%[uvref]]!               \n\t"
        "vld2.8 {d13,d15},[%[uvref]], %[ref_stride] \n\t"
        //"vswp d12,d14                                      \n\t"
        "vrev32.8 q6, q6                            \n\t"
        "vrev32.8 q7, q7                            \n\t"
        "vext.8 d13,d12,d13,#2                      \n\t"
        "vext.8 d15,d14,d15,#2                      \n\t"
        "vld2.8 {d12,d14},[%[uvsrc]], %[stride]     \n\t"
        "vsubl.u8 q6,d12,d13                        \n\t"
        "vsubl.u8 q7,d14,d15                        \n\t"
        ///============
        "vst1.64 {d0}, [%[vdst]]!                   \n\t"
        "vst1.64 {d4}, [%[vdst]]!                   \n\t"
        "vst1.64 {d8}, [%[vdst]]!                   \n\t"
        "vst1.64 {d12},[%[vdst]]!                   \n\t"
        "vst1.64 {d1}, [%[vdst]]!                   \n\t"
        "vst1.64 {d5}, [%[vdst]]!                   \n\t"
        "vst1.64 {d9}, [%[vdst]]!                   \n\t"
        "vst1.64 {d13},[%[vdst]]!                   \n\t"
        "vst1.64 {d2}, [%[udst]]!                   \n\t"
        "vst1.64 {d6}, [%[udst]]!                   \n\t"
        "vst1.64 {d10},[%[udst]]!                   \n\t"
        "vst1.64 {d14},[%[udst]]!                   \n\t"
        "vst1.64 {d3}, [%[udst]]!                   \n\t"
        "vst1.64 {d7}, [%[udst]]!                   \n\t"
        "vst1.64 {d11},[%[udst]]!                   \n\t"
        "vst1.64 {d15},[%[udst]]!                   \n\t"
        "subs %[cycle], %[cycle], #1                 \n\t"
        "bne 0b"
        : [udst] "+r"(udst), [vdst] "+r"(vdst), [cycle] "+r"(cycle)
        : [uvsrc] "r"(uvsrc), [uvref] "r"(uvref),
        [stride] "r"(stride), [ref_stride] "r"(ref_stride)
        : "cc",  "memory",  "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7"
    );
}

inline void NV21_pUV_ext_asm_shft3(unsigned short *dst, uint8_t *uvsrc, uint8_t *uvref, uint_t stride)
{
    unsigned short *udst;
    unsigned short *vdst;
    uint_t ref_stride = (((stride + 31) >> 5) << 5) - 16;
    uint8_t cycle = 2;
    udst = &dst[256];
    vdst = &dst[320];
    asm volatile(
        "0:@two cycle                               \n\t"
        "vld2.8 {d0,d2},  [%[uvref]]!               \n\t"
        "vld2.8 {d1,d3},  [%[uvref]], %[ref_stride] \n\t"
        //"vswp d0,d2                               \n\t"
        "vrev32.8 q0, q0                            \n\t"
        "vrev32.8 q1, q1                            \n\t"
        "vext.8 d1, d0, d1, #3                      \n\t"
        "vext.8 d3, d2, d3, #3                      \n\t"
        "vld2.8 {d0,d2},  [%[uvsrc]], %[stride]     \n\t"
        "vsubl.u8 q0,d0,d1                          \n\t"
        "vsubl.u8 q1,d2,d3                          \n\t"
        ///============
        "vld2.8 {d4,d6},  [%[uvref]]!               \n\t"
        "vld2.8 {d5,d7},  [%[uvref]], %[ref_stride] \n\t"
        //"vswp d4,d6                                      \n\t"
        "vrev32.8 q2, q2                            \n\t"
        "vrev32.8 q3, q3                            \n\t"
        "vext.8 d5, d4, d5, #3                      \n\t"
        "vext.8 d7, d6, d7, #3                      \n\t"
        "vld2.8 {d4,d6},  [%[uvsrc]], %[stride]     \n\t"
        "vsubl.u8 q2,d4,d5                          \n\t"
        "vsubl.u8 q3,d6,d7                          \n\t"
        ///============
        "vld2.8 {d8, d10}, [%[uvref]]!              \n\t"
        "vld2.8 {d9, d11}, [%[uvref]], %[ref_stride]\n\t"
        //"vswp d8,d10                                      \n\t"
        "vrev32.8 q4, q4                            \n\t"
        "vrev32.8 q5, q5                            \n\t"
        "vext.8 d9, d8, d9, #3                      \n\t"
        "vext.8 d11,d10,d11,#3                      \n\t"
        "vld2.8 {d8,d10}, [%[uvsrc]], %[stride]     \n\t"
        "vsubl.u8 q4,d8,d9                          \n\t"
        "vsubl.u8 q5,d10,d11                        \n\t"
        ///============
        "vld2.8 {d12,d14},[%[uvref]]!               \n\t"
        "vld2.8 {d13,d15},[%[uvref]], %[ref_stride] \n\t"
        //"vswp d12,d14                                      \n\t"
        "vrev32.8 q6, q6                            \n\t"
        "vrev32.8 q7, q7                            \n\t"
        "vext.8 d13,d12,d13,#3                      \n\t"
        "vext.8 d15,d14,d15,#3                      \n\t"
        "vld2.8 {d12,d14},[%[uvsrc]], %[stride]     \n\t"
        "vsubl.u8 q6,d12,d13                        \n\t"
        "vsubl.u8 q7,d14,d15                        \n\t"
        ///============
        "vst1.64 {d0}, [%[vdst]]!                   \n\t"
        "vst1.64 {d4}, [%[vdst]]!                   \n\t"
        "vst1.64 {d8}, [%[vdst]]!                   \n\t"
        "vst1.64 {d12},[%[vdst]]!                   \n\t"
        "vst1.64 {d1}, [%[vdst]]!                   \n\t"
        "vst1.64 {d5}, [%[vdst]]!                   \n\t"
        "vst1.64 {d9}, [%[vdst]]!                   \n\t"
        "vst1.64 {d13},[%[vdst]]!                   \n\t"
        "vst1.64 {d2}, [%[udst]]!                   \n\t"
        "vst1.64 {d6}, [%[udst]]!                   \n\t"
        "vst1.64 {d10},[%[udst]]!                   \n\t"
        "vst1.64 {d14},[%[udst]]!                   \n\t"
        "vst1.64 {d3}, [%[udst]]!                   \n\t"
        "vst1.64 {d7}, [%[udst]]!                   \n\t"
        "vst1.64 {d11},[%[udst]]!                   \n\t"
        "vst1.64 {d15},[%[udst]]!                   \n\t"
        "subs %[cycle], %[cycle], #1                 \n\t"
        "bne 0b"
        : [udst] "+r"(udst), [vdst] "+r"(vdst), [cycle] "+r"(cycle)
        : [uvsrc] "r"(uvsrc), [uvref] "r"(uvref),
        [stride] "r"(stride), [ref_stride] "r"(ref_stride)
        : "cc",  "memory",  "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7"
    );
}

inline void YV12_pUV_ext_asm_shft0(unsigned short *dst, uint8_t *usrc, uint8_t *vsrc,
                                   uint8_t *uvref, uint_t stride)
{
    unsigned short *udst;
    unsigned short *vdst;
    uint8_t cycle = 2;
    uint_t src_stride = stride / 2;
    stride = (((stride + 31) >> 5) << 5);
    udst = &dst[256];
    vdst = &dst[320];
    asm volatile(
        "0:@two cycle                               \n\t"
        "vld2.8 {d1,d3},  [%[uvref]], %[stride]     \n\t"
        "vrev32.8 d1, d1                            \n\t"
        "vrev32.8 d3, d3                            \n\t"
        "vswp     d1, d3                            \n\t"
        "vld1.64 {d0}, [%[usrc]], %[src_stride]     \n\t"
        "vld1.64 {d2}, [%[vsrc]], %[src_stride]     \n\t"
        "vsubl.u8 q0,d0,d1                          \n\t"
        "vsubl.u8 q1,d2,d3                          \n\t"
        "vld2.8 {d5,d7},  [%[uvref]], %[stride]      \n\t"
        "vrev32.8 d5, d5                            \n\t"
        "vrev32.8 d7, d7                            \n\t"
        "vswp     d5, d7                            \n\t"
        "vld1.64 {d4}, [%[usrc]], %[src_stride]      \n\t"
        "vld1.64 {d6}, [%[vsrc]], %[src_stride]      \n\t"
        "vsubl.u8 q2,d4,d5                          \n\t"
        "vsubl.u8 q3,d6,d7                          \n\t"
        "vld2.8 {d9,d11},  [%[uvref]], %[stride]    \n\t"
        "vrev32.8 d9, d9                            \n\t"
        "vrev32.8 d11, d11                          \n\t"
        "vswp     d9, d11                           \n\t"
        "vld1.64 {d8}, [%[usrc]], %[src_stride]     \n\t"
        "vld1.64 {d10}, [%[vsrc]], %[src_stride]    \n\t"
        "vsubl.u8 q4,d8,d9                          \n\t"
        "vsubl.u8 q5,d10,d11                        \n\t"
        "vld2.8 {d13,d15},  [%[uvref]], %[stride]   \n\t"
        "vrev32.8 d13, d13                          \n\t"
        "vrev32.8 d15, d15                          \n\t"
        "vswp     d13, d15                          \n\t"
        "vld1.64 {d12}, [%[usrc]], %[src_stride]    \n\t"
        "vld1.64 {d14}, [%[vsrc]], %[src_stride]    \n\t"
        "vsubl.u8 q6,d12,d13                        \n\t"
        "vsubl.u8 q7,d14,d15                        \n\t"
        "vst1.64 {d0}, [%[udst]]!                   \n\t"
        "vst1.64 {d4}, [%[udst]]!                   \n\t"
        "vst1.64 {d8}, [%[udst]]!                   \n\t"
        "vst1.64 {d12},[%[udst]]!                   \n\t"
        "vst1.64 {d1}, [%[udst]]!                   \n\t"
        "vst1.64 {d5}, [%[udst]]!                   \n\t"
        "vst1.64 {d9}, [%[udst]]!                   \n\t"
        "vst1.64 {d13},[%[udst]]!                   \n\t"
        "vst1.64 {d2}, [%[vdst]]!                   \n\t"
        "vst1.64 {d6}, [%[vdst]]!                   \n\t"
        "vst1.64 {d10},[%[vdst]]!                   \n\t"
        "vst1.64 {d14},[%[vdst]]!                   \n\t"
        "vst1.64 {d3}, [%[vdst]]!                   \n\t"
        "vst1.64 {d7}, [%[vdst]]!                   \n\t"
        "vst1.64 {d11},[%[vdst]]!                   \n\t"
        "vst1.64 {d15},[%[vdst]]!                   \n\t"
        "subs %[cycle], %[cycle], #1                 \n\t"
        "bne 0b"
        : [udst] "+r"(udst), [vdst] "+r"(vdst),
        [cycle] "+r"(cycle)
        : [usrc] "r"(usrc), [vsrc] "r"(vsrc), [uvref] "r"(uvref),
        [stride] "r"(stride), [src_stride] "r"(src_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7"
    );
}

inline void YV12_pUV_ext_asm_shft1(unsigned short *dst, uint8_t *usrc, uint8_t *vsrc, uint8_t *uvref, uint_t stride)
{
    unsigned short *udst;
    unsigned short *vdst;
    uint8_t cycle = 2;
    uint_t src_stride = stride / 2;
    stride = (((stride + 31) >> 5) << 5) - 16;
    udst = &dst[256];
    vdst = &dst[320];
    asm volatile(
        "0:@two cycle                               \n\t"
        "vld2.8 {d0,d2},  [%[uvref]]!               \n\t"
        "vld2.8 {d1,d3},  [%[uvref]], %[stride]     \n\t"
        "vrev32.8 q0, q0                            \n\t"
        "vrev32.8 q1, q1                            \n\t"
        "vswp q0, q1                                \n\t"
        "vext.8 d3, d2, d3, #1                      \n\t"
        "vext.8 d1, d0, d1, #1                      \n\t"
        "vld1.64 {d0}, [%[usrc]], %[src_stride]     \n\t"
        "vld1.64 {d2}, [%[vsrc]], %[src_stride]     \n\t"
        "vsubl.u8 q0,d0,d1                          \n\t"
        "vsubl.u8 q1,d2,d3                          \n\t"
        ///-----
        "vld2.8 {d4,d6},  [%[uvref]]!               \n\t"
        "vld2.8 {d5,d7},  [%[uvref]], %[stride]     \n\t"
        "vrev32.8 q2, q2                            \n\t"
        "vrev32.8 q3, q3                            \n\t"
        "vswp q2, q3                                \n\t"
        "vext.8 d7, d6, d7, #1                      \n\t"
        "vext.8 d5, d4, d5, #1                      \n\t"
        "vld1.64 {d4}, [%[usrc]], %[src_stride]      \n\t"
        "vld1.64 {d6}, [%[vsrc]], %[src_stride]      \n\t"
        "vsubl.u8 q2,d4,d5                          \n\t"
        "vsubl.u8 q3,d6,d7                          \n\t"
        ///-----
        "vld2.8 {d8,d10},  [%[uvref]]!              \n\t"
        "vld2.8 {d9,d11},  [%[uvref]], %[stride]    \n\t"
        "vrev32.8 q4, q4                            \n\t"
        "vrev32.8 q5, q5                            \n\t"
        "vswp q4, q5                                \n\t"
        "vext.8 d11, d10, d11, #1                   \n\t"
        "vext.8 d9,  d8,  d9,#1                     \n\t"
        "vld1.64 {d8}, [%[usrc]], %[src_stride]     \n\t"
        "vld1.64 {d10}, [%[vsrc]], %[src_stride]    \n\t"
        "vsubl.u8 q4,d8,d9                          \n\t"
        "vsubl.u8 q5,d10,d11                        \n\t"
        ///-----
        "vld2.8 {d12,d14},  [%[uvref]]!             \n\t"
        "vld2.8 {d13,d15},  [%[uvref]], %[stride]   \n\t"
        "vrev32.8 q6, q6                            \n\t"
        "vrev32.8 q7, q7                            \n\t"
        "vswp q6, q7                                \n\t"
        "vext.8 d15, d14, d15, #1                   \n\t"
        "vext.8 d13,  d12,  d13,#1                  \n\t"
        "vld1.64 {d12}, [%[usrc]], %[src_stride]    \n\t"
        "vld1.64 {d14}, [%[vsrc]], %[src_stride]    \n\t"
        "vsubl.u8 q6,d12,d13                        \n\t"
        "vsubl.u8 q7,d14,d15                        \n\t"
        "vst1.64 {d0}, [%[udst]]!                   \n\t"
        "vst1.64 {d4}, [%[udst]]!                   \n\t"
        "vst1.64 {d8}, [%[udst]]!                   \n\t"
        "vst1.64 {d12},[%[udst]]!                   \n\t"
        "vst1.64 {d1}, [%[udst]]!                   \n\t"
        "vst1.64 {d5}, [%[udst]]!                   \n\t"
        "vst1.64 {d9}, [%[udst]]!                   \n\t"
        "vst1.64 {d13},[%[udst]]!                   \n\t"
        "vst1.64 {d2}, [%[vdst]]!                   \n\t"
        "vst1.64 {d6}, [%[vdst]]!                   \n\t"
        "vst1.64 {d10},[%[vdst]]!                   \n\t"
        "vst1.64 {d14},[%[vdst]]!                   \n\t"
        "vst1.64 {d3}, [%[vdst]]!                   \n\t"
        "vst1.64 {d7}, [%[vdst]]!                   \n\t"
        "vst1.64 {d11},[%[vdst]]!                   \n\t"
        "vst1.64 {d15},[%[vdst]]!                   \n\t"
        "subs %[cycle], %[cycle], #1                \n\t"
        "bne 0b"
        : [udst] "+r"(udst), [vdst] "+r"(vdst), [cycle] "+r"(cycle)
        : [usrc] "r"(usrc), [vsrc] "r"(vsrc), [uvref] "r"(uvref),
        [stride] "r"(stride), [src_stride] "r"(src_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7"
    );
}

inline void YV12_pUV_ext_asm_shft2(unsigned short *dst, uint8_t *usrc, uint8_t *vsrc, uint8_t *uvref, uint_t stride)
{
    unsigned short *udst;
    unsigned short *vdst;
    uint8_t cycle = 2;
    uint_t src_stride = stride / 2;
    stride = (((stride + 31) >> 5) << 5) - 16;
    udst = &dst[256];
    vdst = &dst[320];
    asm volatile(
        "0:@two cycle                                          \n\t"
        "vld2.8 {d0,d2},  [%[uvref]]!      \n\t"
        "vld2.8 {d1,d3},  [%[uvref]], %[stride]     \n\t"
        "vrev32.8 q0, q0                            \n\t"
        "vrev32.8 q1, q1                            \n\t"
        "vswp q0, q1                                \n\t"
        "vext.8 d3, d2, d3, #2                      \n\t"
        "vext.8 d1, d0, d1, #2                      \n\t"
        "vld1.64 {d0}, [%[usrc]], %[src_stride]     \n\t"
        "vld1.64 {d2}, [%[vsrc]], %[src_stride]     \n\t"
        "vsubl.u8 q0,d0,d1                                    \n\t"
        "vsubl.u8 q1,d2,d3                                    \n\t"
        ///-----
        "vld2.8 {d4,d6},  [%[uvref]]!     \n\t"
        "vld2.8 {d5,d7},  [%[uvref]], %[stride]     \n\t"
        "vrev32.8 q2, q2                            \n\t"
        "vrev32.8 q3, q3                            \n\t"
        "vswp q2, q3                                \n\t"
        "vext.8 d7, d6, d7, #2                      \n\t"
        "vext.8 d5, d4, d5, #2                      \n\t"
        "vld1.64 {d4}, [%[usrc]], %[src_stride]      \n\t"
        "vld1.64 {d6}, [%[vsrc]], %[src_stride]      \n\t"
        "vsubl.u8 q2,d4,d5                                    \n\t"
        "vsubl.u8 q3,d6,d7                                    \n\t"
        ///-----
        "vld2.8 {d8,d10},  [%[uvref]]!    \n\t"
        "vld2.8 {d9,d11},  [%[uvref]], %[stride]    \n\t"
        "vrev32.8 q4, q4                            \n\t"
        "vrev32.8 q5, q5                            \n\t"
        "vswp q4, q5                                \n\t"
        "vext.8 d11, d10, d11, #2                      \n\t"
        "vext.8 d9,  d8,  d9,#2                      \n\t"
        "vld1.64 {d8}, [%[usrc]], %[src_stride]     \n\t"
        "vld1.64 {d10}, [%[vsrc]], %[src_stride]   \n\t"
        "vsubl.u8 q4,d8,d9                                    \n\t"
        "vsubl.u8 q5,d10,d11                                \n\t"
        ///-----
        "vld2.8 {d12,d14},  [%[uvref]]!   \n\t"
        "vld2.8 {d13,d15},  [%[uvref]], %[stride]   \n\t"
        "vrev32.8 q6, q6                            \n\t"
        "vrev32.8 q7, q7                            \n\t"
        "vswp q6, q7                                \n\t"
        "vext.8 d15, d14, d15, #2                      \n\t"
        "vext.8 d13,  d12,  d13,#2                      \n\t"
        "vld1.64 {d12}, [%[usrc]], %[src_stride]    \n\t"
        "vld1.64 {d14}, [%[vsrc]], %[src_stride]    \n\t"
        "vsubl.u8 q6,d12,d13                                 \n\t"
        "vsubl.u8 q7,d14,d15                                 \n\t"
        "vst1.64 {d0}, [%[udst]]!                   \n\t"
        "vst1.64 {d4}, [%[udst]]!                   \n\t"
        "vst1.64 {d8}, [%[udst]]!                   \n\t"
        "vst1.64 {d12},[%[udst]]!                   \n\t"
        "vst1.64 {d1}, [%[udst]]!                   \n\t"
        "vst1.64 {d5}, [%[udst]]!                   \n\t"
        "vst1.64 {d9}, [%[udst]]!                   \n\t"
        "vst1.64 {d13},[%[udst]]!                   \n\t"
        "vst1.64 {d2}, [%[vdst]]!                   \n\t"
        "vst1.64 {d6}, [%[vdst]]!                   \n\t"
        "vst1.64 {d10},[%[vdst]]!                   \n\t"
        "vst1.64 {d14},[%[vdst]]!                   \n\t"
        "vst1.64 {d3}, [%[vdst]]!                   \n\t"
        "vst1.64 {d7}, [%[vdst]]!                   \n\t"
        "vst1.64 {d11},[%[vdst]]!                   \n\t"
        "vst1.64 {d15},[%[vdst]]!                   \n\t"
        "subs %[cycle], %[cycle], #1                 \n\t"
        "bne 0b"
        : [udst] "+r"(udst), [vdst] "+r"(vdst),
        [cycle] "+r"(cycle)
        : [usrc] "r"(usrc), [vsrc] "r"(vsrc), [uvref] "r"(uvref),
        [stride] "r"(stride), [src_stride] "r"(src_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7"
    );
}

inline void YV12_pUV_ext_asm_shft3(unsigned short *dst, uint8_t *usrc, uint8_t *vsrc, uint8_t *uvref, uint_t stride)
{
    unsigned short *udst;
    unsigned short *vdst;
    uint8_t cycle = 2;
    uint_t src_stride = stride / 2;
    stride = (((stride + 31) >> 5) << 5) - 16;
    udst = &dst[256];
    vdst = &dst[320];
    asm volatile(
        "0:@two cycle                                          \n\t"
        "vld2.8 {d0,d2},  [%[uvref]]!      \n\t"
        "vld2.8 {d1,d3},  [%[uvref]], %[stride]     \n\t"
        "vrev32.8 q0, q0                            \n\t"
        "vrev32.8 q1, q1                            \n\t"
        "vswp q0, q1                                \n\t"
        "vext.8 d3, d2, d3, #3                      \n\t"
        "vext.8 d1, d0, d1, #3                      \n\t"
        "vld1.64 {d0}, [%[usrc]], %[src_stride]     \n\t"
        "vld1.64 {d2}, [%[vsrc]], %[src_stride]     \n\t"
        "vsubl.u8 q0,d0,d1                                    \n\t"
        "vsubl.u8 q1,d2,d3                                    \n\t"
        ///-----
        "vld2.8 {d4,d6},  [%[uvref]]!     \n\t"
        "vld2.8 {d5,d7},  [%[uvref]], %[stride]     \n\t"
        "vrev32.8 q2, q2                            \n\t"
        "vrev32.8 q3, q3                            \n\t"
        "vswp q2, q3                                \n\t"
        "vext.8 d7, d6, d7, #3                      \n\t"
        "vext.8 d5, d4, d5, #3                      \n\t"
        "vld1.64 {d4}, [%[usrc]], %[src_stride]      \n\t"
        "vld1.64 {d6}, [%[vsrc]], %[src_stride]      \n\t"
        "vsubl.u8 q2,d4,d5                                    \n\t"
        "vsubl.u8 q3,d6,d7                                    \n\t"
        ///-----
        "vld2.8 {d8,d10},  [%[uvref]]!    \n\t"
        "vld2.8 {d9,d11},  [%[uvref]], %[stride]    \n\t"
        "vrev32.8 q4, q4                            \n\t"
        "vrev32.8 q5, q5                            \n\t"
        "vswp q4, q5                                \n\t"
        "vext.8 d11, d10, d11, #3                      \n\t"
        "vext.8 d9,  d8,  d9,#3                      \n\t"
        "vld1.64 {d8}, [%[usrc]], %[src_stride]     \n\t"
        "vld1.64 {d10}, [%[vsrc]], %[src_stride]   \n\t"
        "vsubl.u8 q4,d8,d9                                    \n\t"
        "vsubl.u8 q5,d10,d11                                \n\t"
        ///-----
        "vld2.8 {d12,d14},  [%[uvref]]!   \n\t"
        "vld2.8 {d13,d15},  [%[uvref]], %[stride]   \n\t"
        "vrev32.8 q6, q6                            \n\t"
        "vrev32.8 q7, q7                            \n\t"
        "vswp q6, q7                                \n\t"
        "vext.8 d15, d14, d15, #3                      \n\t"
        "vext.8 d13,  d12,  d13,#3                      \n\t"
        "vld1.64 {d12}, [%[usrc]], %[src_stride]    \n\t"
        "vld1.64 {d14}, [%[vsrc]], %[src_stride]    \n\t"
        "vsubl.u8 q6,d12,d13                                 \n\t"
        "vsubl.u8 q7,d14,d15                                 \n\t"
        "vst1.64 {d0}, [%[udst]]!                   \n\t"
        "vst1.64 {d4}, [%[udst]]!                   \n\t"
        "vst1.64 {d8}, [%[udst]]!                   \n\t"
        "vst1.64 {d12},[%[udst]]!                   \n\t"
        "vst1.64 {d1}, [%[udst]]!                   \n\t"
        "vst1.64 {d5}, [%[udst]]!                   \n\t"
        "vst1.64 {d9}, [%[udst]]!                   \n\t"
        "vst1.64 {d13},[%[udst]]!                   \n\t"
        "vst1.64 {d2}, [%[vdst]]!                   \n\t"
        "vst1.64 {d6}, [%[vdst]]!                   \n\t"
        "vst1.64 {d10},[%[vdst]]!                   \n\t"
        "vst1.64 {d14},[%[vdst]]!                   \n\t"
        "vst1.64 {d3}, [%[vdst]]!                   \n\t"
        "vst1.64 {d7}, [%[vdst]]!                   \n\t"
        "vst1.64 {d11},[%[vdst]]!                   \n\t"
        "vst1.64 {d15},[%[vdst]]!                   \n\t"
        "subs %[cycle], %[cycle], #1                \n\t"
        "bne 0b"
        : [udst] "+r"(udst), [vdst] "+r"(vdst),
        [cycle] "+r"(cycle)
        : [usrc] "r"(usrc), [vsrc] "r"(vsrc), [uvref] "r"(uvref),
        [stride] "r"(stride), [src_stride] "r"(src_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7"
    );
}

void Y_line_asm(uint8_t *ydst, uint8_t *ysrc, uint_t pix_stride, uint_t mb_stride)
{
    uint8_t *cur_mb_y = ysrc;
    uint8_t *input_addr = ydst;
    unsigned int mbx_max  = mb_stride;
    unsigned int stride = pix_stride;
    asm volatile(
        "mov r0, #0                             \n\t"
        "pld [%[cur_mb_y]]                      \n\t"
        "0:@two cycle, 128 byte every cycle     \n\t"
        "mov %[ysrc], %[cur_mb_y]               \n\t"
        "add %[ydst], %[input_addr], #96        \n\t"
        "vld2.32 {d2, d4}, [%[ysrc]], %[stride] \n\t"
        "vld2.32 {d6, d8}, [%[ysrc]], %[stride] \n\t"
        "vld2.32 {d3, d5}, [%[ysrc]], %[stride] \n\t"
        "vld2.32 {d7, d9}, [%[ysrc]], %[stride] \n\t"
        "vtrn.32 q1, q3                         \n\t"
        "vtrn.32 q2, q4                         \n\t"
        "vdup.32 q0, r0                         \n\t"
        "vswp q0, q1                            \n\t"
        "vst2.8  {q0, q1}, [%[ydst]]!           \n\t"
        "vdup.32 q1, r0                         \n\t"
        "vswp q2, q1                            \n\t"
        "vst2.8  {q1, q2}, [%[ydst]]!           \n\t"
        //////
        "vld2.32 {d2, d4}, [%[ysrc]], %[stride] \n\t"
        "vld2.32 {d10,d12},[%[ysrc]], %[stride] \n\t"
        "vld2.32 {d3, d5}, [%[ysrc]], %[stride] \n\t"
        "vld2.32 {d11,d13},[%[ysrc]], %[stride] \n\t"
        "vtrn.32 q1, q5                         \n\t"
        "vtrn.32 q2, q6                         \n\t"
        "vdup.32 q0, r0                         \n\t"
        "vswp q0, q1                            \n\t"
        "vst2.8  {q0, q1}, [%[ydst]]!           \n\t"
        "vdup.32 q1, r0                         \n\t"
        "vswp q2, q1                            \n\t"
        "vst2.8  {q1, q2}, [%[ydst]]!           \n\t"
        ///////
        "vdup.32 q2, r0                         \n\t"
        "vswp q2, q3                            \n\t"
        "vst2.8  {q2, q3}, [%[ydst]]!           \n\t"
        "vdup.32 q3, r0                         \n\t"
        "vswp q4, q3                            \n\t"
        "vst2.8  {q3, q4}, [%[ydst]]!           \n\t"
        ///////
        "vdup.32 q4, r0                         \n\t"
        "vswp q4, q5                            \n\t"
        "vst2.8  {q4, q5}, [%[ydst]]!           \n\t"
        "vdup.32 q5, r0                         \n\t"
        "vswp q5, q6                            \n\t"
        "vst2.8  {q5, q6}, [%[ydst]]!           \n\t"
        /////second 128 bytes
        "vld2.32 {d2, d4}, [%[ysrc]], %[stride] \n\t"
        "vld2.32 {d6, d8}, [%[ysrc]], %[stride] \n\t"
        "vld2.32 {d3, d5}, [%[ysrc]], %[stride] \n\t"
        "vld2.32 {d7, d9}, [%[ysrc]], %[stride] \n\t"
        "vtrn.32 q1, q3                         \n\t"
        "vtrn.32 q2, q4                         \n\t"
        "vdup.32 q0, r0                         \n\t"
        "vswp q0, q1                            \n\t"
        "vst2.8  {q0, q1}, [%[ydst]]!           \n\t"
        "vdup.32 q1, r0                         \n\t"
        "vswp q2, q1                            \n\t"
        "vst2.8  {q1, q2}, [%[ydst]]!           \n\t"
        //////
        "vld2.32 {d2, d4}, [%[ysrc]], %[stride] \n\t"
        "vld2.32 {d10,d12},[%[ysrc]], %[stride] \n\t"
        "vld2.32 {d3, d5}, [%[ysrc]], %[stride] \n\t"
        "vld2.32 {d11,d13},[%[ysrc]], %[stride] \n\t"
        "vtrn.32 q1, q5                         \n\t"
        "vtrn.32 q2, q6                         \n\t"
        "vdup.32 q0, r0                         \n\t"
        "vswp q0, q1                            \n\t"
        "vst2.8  {q0, q1}, [%[ydst]]!           \n\t"
        "vdup.32 q1, r0                         \n\t"
        "vswp q2, q1                            \n\t"
        "vst2.8  {q1, q2}, [%[ydst]]!           \n\t"
        ///////
        "vdup.32 q2, r0                         \n\t"
        "vswp q2, q3                            \n\t"
        "vst2.8  {q2, q3}, [%[ydst]]!           \n\t"
        "vdup.32 q3, r0                         \n\t"
        "vswp q4, q3                            \n\t"
        "vst2.8  {q3, q4}, [%[ydst]]!           \n\t"
        ///////
        "vdup.32 q4, r0                         \n\t"
        "vswp q4, q5                            \n\t"
        "vst2.8  {q4, q5}, [%[ydst]]!           \n\t"
        "vdup.32 q5, r0                         \n\t"
        "vswp q5, q6                            \n\t"
        "vst2.8  {q5, q6}, [%[ydst]]!           \n\t"
        "1:@update                              \n\t"
        "add %[input_addr], %[input_addr], #864 \n\t"
        "add %[cur_mb_y], %[cur_mb_y], #16      \n\t"
        "subs %[cycle], %[cycle], #1            \n\t"
        "bne  0b                                \n\t"
        "2:@end                                 \n\t"
        : [ydst] "+r"(ydst), [cur_mb_y] "+r"(cur_mb_y),
        [ysrc] "+r"(ysrc), [input_addr] "+r"(input_addr), [cycle] "+r"(mbx_max)
        : [stride] "r"(stride)
        : "cc", "memory", "r0", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6"
    );
}

void nv21_uvline_asm(uint8_t *dst, uint8_t *uvsrc, uint_t pix_stride, uint_t mb_stride)
{
    uint8_t *cur_mb_uv = uvsrc;
    uint8_t *input_addr = dst;
    uint8_t *udst = NULL;
    uint8_t *vdst = NULL;
    unsigned int stride = pix_stride;
    unsigned int mbx_max = mb_stride;
    asm volatile(
        "mov r5, #0                                 \n\t"
        "pld [%[cur_mb_uv]]                         \n\t"
        "0:@begin                                   \n\t"
        "mov %[uvsrc], %[cur_mb_uv]                 \n\t"
        "add %[vdst], %[input_addr], #608           \n\t"
        "add %[udst], %[input_addr], #736           \n\t"
        "vld2.8 {d2, d4}, [%[uvsrc]], %[stride]     \n\t"
        "vld2.8 {d3, d5}, [%[uvsrc]], %[stride]     \n\t"
        "vld2.8 {d6, d8}, [%[uvsrc]], %[stride]     \n\t"
        "vld2.8 {d7, d9}, [%[uvsrc]], %[stride]     \n\t"
        "vuzp.32 q1, q3                             \n\t"
        "vuzp.32 q2, q4                             \n\t"
        "vld2.8 {d10,d12},[%[uvsrc]], %[stride]     \n\t"
        "vld2.8 {d11,d13},[%[uvsrc]], %[stride]     \n\t"
        "vld2.8 {d14,d16},[%[uvsrc]], %[stride]     \n\t"
        "vld2.8 {d15,d17},[%[uvsrc]], %[stride]     \n\t"
        "vuzp.32 q6, q8                             \n\t"
        "vuzp.32 q5, q7                             \n\t"
        "vdup.32 q0, r5                             \n\t"
        "vswp q0, q1                                \n\t"
        "vst2.8 {q0, q1}, [%[udst]]!                \n\t"
        "vdup.32 q1, r5                             \n\t"
        "vswp q1, q2                                \n\t"
        "vst2.8 {q1, q2}, [%[vdst]]!                \n\t"
        "vdup.32 q2, r5                             \n\t"
        "vswp q2, q3                                \n\t"
        "vst2.8 {q2, q3}, [%[udst]]!                \n\t"
        "vdup.32 q3, r5                             \n\t"
        "vswp q3, q4                                \n\t"
        "vst2.8 {q3, q4}, [%[vdst]]!                \n\t"
        "vdup.32 q4, r5                             \n\t"
        "vswp q4, q5                                \n\t"
        "vst2.8 {q4, q5}, [%[udst]]!                \n\t"
        "vdup.32 q5, r5                             \n\t"
        "vswp q5, q6                                \n\t"
        "vst2.8 {q5, q6}, [%[vdst]]!                \n\t"
        "vdup.32 q6, r5                             \n\t"
        "vswp q6, q7                                \n\t"
        "vst2.8 {q6, q7}, [%[udst]]!                \n\t"
        "vdup.32 q7, r5                             \n\t"
        "vswp q7, q8                                \n\t"
        "vst2.8 {q7, q8}, [%[vdst]]!                \n\t"
        "1:@update                                  \n\t"
        "add  %[cur_mb_uv], %[cur_mb_uv], #16       \n\t"
        "add  %[input_addr], %[input_addr], #864    \n\t"
        "subs %[cycle], %[cycle], #1                \n\t"
        "bne  0b                                    \n\t"
        "2:@end                                     \n\t"
        : [udst] "+r"(udst), [vdst] "+r"(vdst),
        [uvsrc] "+r"(uvsrc), [cur_mb_uv] "+r"(cur_mb_uv),
        [input_addr] "+r"(input_addr), [cycle] "+r"(mbx_max)
        : [stride] "r"(stride)
        : "cc", "memory", "r5", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7", "q8"
    );
}

void yv12_uvline_asm(uint8_t *dst, uint8_t *usrc, uint8_t *vsrc, uint_t pix_stride, uint_t mb_stride)
{
    uint8_t *cur_mb_u = usrc;
    uint8_t *cur_mb_v = vsrc;
    uint8_t *udst = NULL;
    uint8_t *vdst = NULL;
    uint8_t *input_addr = dst;
    unsigned int stride = pix_stride;
    unsigned int mbx_max = mb_stride;
    usrc = cur_mb_u;
    vsrc = cur_mb_v;
    stride = stride / 2;
    asm volatile(
        "mov r5, #0                                 \n\t"
        "pld [%[cur_mb_u]]                          \n\t"
        "0:@begin                                   \n\t"
        "mov %[usrc], %[cur_mb_u]                   \n\t"
        "mov %[vsrc], %[cur_mb_v]                   \n\t"
        "add %[udst], %[input_addr], #608           \n\t"
        "add %[vdst], %[input_addr], #736           \n\t"
        "vdup.32 q0, r5                             \n\t"
        "vld1.64 {d2},    [%[usrc]], %[stride]      \n\t"
        "vld1.64 {d4},    [%[usrc]], %[stride]      \n\t"
        "vld1.64 {d3},    [%[usrc]], %[stride]      \n\t"
        "vld1.64 {d5},    [%[usrc]], %[stride]      \n\t"
        "vld1.64 {d6},    [%[usrc]], %[stride]      \n\t"
        "vld1.64 {d8},    [%[usrc]], %[stride]      \n\t"
        "vld1.64 {d7},    [%[usrc]], %[stride]      \n\t"
        "vld1.64 {d9},    [%[usrc]], %[stride]      \n\t"
        "vtrn.32 q1, q2                             \n\t"
        "vtrn.32 q3, q4                             \n\t"
        "vswp q0, q1                                \n\t"
        "vst2.8 {q0, q1}, [%[udst]]!                \n\t"
        "vdup.32 q1, r5                             \n\t"
        "vswp q1, q2                                \n\t"
        "vst2.8 {q1, q2}, [%[udst]]!                \n\t"
        "vdup.32 q2, r5                             \n\t"
        "vswp q2, q3                                \n\t"
        "vst2.8 {q2, q3}, [%[udst]]!                \n\t"
        "vdup.32 q3, r5                             \n\t"
        "vswp q3, q4                                \n\t"
        "vst2.8 {q3, q4}, [%[udst]]!                \n\t"
        /////V
        "vdup.32 q0, r5                             \n\t"
        "vld1.64 {d2},    [%[vsrc]], %[stride]      \n\t"
        "vld1.64 {d4},    [%[vsrc]], %[stride]      \n\t"
        "vld1.64 {d3},    [%[vsrc]], %[stride]      \n\t"
        "vld1.64 {d5},    [%[vsrc]], %[stride]      \n\t"
        "vld1.64 {d6},    [%[vsrc]], %[stride]      \n\t"
        "vld1.64 {d8},    [%[vsrc]], %[stride]      \n\t"
        "vld1.64 {d7},    [%[vsrc]], %[stride]      \n\t"
        "vld1.64 {d9},    [%[vsrc]], %[stride]      \n\t"
        "vtrn.32 q1, q2                             \n\t"
        "vtrn.32 q3, q4                             \n\t"
        "vswp q0, q1                                \n\t"
        "vst2.8 {q0, q1}, [%[vdst]]!                \n\t"
        "vdup.32 q1, r5                             \n\t"
        "vswp q1, q2                                \n\t"
        "vst2.8 {q1, q2}, [%[vdst]]!                \n\t"
        "vdup.32 q2, r5                             \n\t"
        "vswp q2, q3                                \n\t"
        "vst2.8 {q2, q3}, [%[vdst]]!                \n\t"
        "vdup.32 q3, r5                             \n\t"
        "vswp q3, q4                                \n\t"
        "vst2.8 {q3, q4}, [%[vdst]]!                \n\t"
        "1:@update                                  \n\t"
        "add  %[cur_mb_u], %[cur_mb_u], #8          \n\t"
        "add  %[cur_mb_v], %[cur_mb_v], #8          \n\t"
        "add  %[input_addr], %[input_addr], #864    \n\t"
        "subs %[cycle], %[cycle], #1                \n\t"
        "bne  0b                                    \n\t"
        "2:@end                                     \n\t"
        : [udst] "+r"(udst), [vdst] "+r"(vdst),
        [usrc] "+r"(usrc), [vsrc] "+r"(vsrc),
        [cur_mb_u] "+r"(cur_mb_u), [cur_mb_v] "+r"(cur_mb_v),
        [input_addr] "+r"(input_addr), [cycle] "+r"(mbx_max)
        : [stride] "r"(stride)
        : "cc", "memory", "r5", "q0", "q1", "q2", "q3", "q4"
    );
}

static void (*pY_shft[BYTE_ALIGN])(unsigned short *ydst, uint8_t *ysrc,
                                   uint8_t *yref, uint_t stride) =
{
    pY_ext_asm_shft0,
    pY_ext_asm_shft1,
    pY_ext_asm_shft2,
    pY_ext_asm_shft3,
    pY_ext_asm_shft4,
    pY_ext_asm_shft5,
    pY_ext_asm_shft6,
    pY_ext_asm_shft7,
};

static void (*YV12_pUV_shft[BYTE_ALIGN / 2])(unsigned short *dst, uint8_t *usrc,
        uint8_t *vsrc, uint8_t *uvref, uint_t stride) =
{
    YV12_pUV_ext_asm_shft0,
    YV12_pUV_ext_asm_shft1,
    YV12_pUV_ext_asm_shft2,
    YV12_pUV_ext_asm_shft3,
};

static void (*NV21_pUV_shft[BYTE_ALIGN / 2])(unsigned short *dst, uint8_t *uvsrc,
        uint8_t *uvref, uint_t stride) =
{
    NV21_pUV_ext_asm_shft0,
    NV21_pUV_ext_asm_shft1,
    NV21_pUV_ext_asm_shft2,
    NV21_pUV_ext_asm_shft3,
};

void pY_ext_asm(unsigned short *ydst, uint8_t *ysrc, uint8_t *yref,
                uint_t stride, int offset)
{
    pY_shft[offset](ydst, ysrc, yref - offset, stride);
}

void NV21_pUV_ext_asm(unsigned short *dst, uint8_t *uvsrc, uint8_t *uvref,
                      uint_t stride, int offset)
{
    NV21_pUV_shft[offset >> 1](dst, uvsrc, uvref - offset, stride);
}

void YV12_pUV_ext_asm(unsigned short *dst, uint8_t *usrc, uint8_t *vsrc,
                      uint8_t *uvref, uint_t stride, int offset)
{
    YV12_pUV_shft[offset >> 1](dst, usrc, vsrc, uvref - offset, stride);
}

void Y_asm_direct(unsigned short *ydst, uint8_t *ysrc, uint8_t *yref, uint_t stride)
{
    uint8_t cycle = 2;
    uint_t ref_stride;
    ref_stride = 16;
    asm volatile(
        "0:@two cycle, 128 bytes every cycle    \n\t"
        ////================================
        "vld2.32 {d0,d2}, [%[ysrc]], %[stride]  \n\t"
        "vld2.32 {d1,d3}, [%[yref]], %[ref_stride]  \n\t"
        "vsubl.u8 q0, d0, d1                    \n\t"
        "vsubl.u8 q1, d2, d3                    \n\t"
        ////================================
        "vld2.32 {d4,d6}, [%[ysrc]], %[stride]  \n\t"
        "vld2.32 {d5,d7}, [%[yref]], %[ref_stride]  \n\t"
        "vsubl.u8 q2,d4, d5                     \n\t"
        "vsubl.u8 q3,d6, d7                     \n\t"
        ////================================
        "vld2.32 {d8,d10},[%[ysrc]], %[stride]  \n\t"
        "vld2.32 {d9,d11},[%[yref]], %[ref_stride]  \n\t"
        "vsubl.u8 q4,d8, d9                     \n\t"
        "vsubl.u8 q5,d10,d11                    \n\t"
        ////================================
        "vld2.32 {d12,d14},[%[ysrc]], %[stride] \n\t"
        "vld2.32 {d13,d15},[%[yref]], %[ref_stride] \n\t"
        "vsubl.u8 q6,d12,d13                    \n\t"
        "vsubl.u8 q7,d14,d15                    \n\t"
        ////================================
        "vld2.32 {d16,d18},[%[ysrc]], %[stride] \n\t"
        "vld2.32 {d17,d19},[%[yref]], %[ref_stride] \n\t"
        "vsubl.u8 q8,d16,d17                    \n\t"
        "vsubl.u8 q9,d18,d19                    \n\t"
        ////================================
        "vld2.32 {d20,d22},[%[ysrc]], %[stride] \n\t"
        "vld2.32 {d21,d23},[%[yref]], %[ref_stride] \n\t"
        "vsubl.u8 q10,d20,d21                   \n\t"
        "vsubl.u8 q11,d22,d23                   \n\t"
        ////================================
        "vld2.32 {d24,d26},[%[ysrc]], %[stride] \n\t"
        "vld2.32 {d25,d27},[%[yref]], %[ref_stride] \n\t"
        "vsubl.u8 q12,d24,d25                   \n\t"
        "vsubl.u8 q13,d26,d27                   \n\t"
        ////================================
        "vld2.32 {d28,d30},[%[ysrc]], %[stride] \n\t"
        "vld2.32 {d29,d31},[%[yref]], %[ref_stride] \n\t"
        "vsubl.u8 q14,d28,d29                   \n\t"
        "vsubl.u8 q15,d30,d31                   \n\t"
        ////================================
        "vst1.64 {d0}, [%[ydst]]!               \n\t"
        "vst1.64 {d4}, [%[ydst]]!               \n\t"
        "vst1.64 {d8}, [%[ydst]]!               \n\t"
        "vst1.64 {d12},[%[ydst]]!               \n\t"
        "vst1.64 {d2}, [%[ydst]]!               \n\t"
        "vst1.64 {d6}, [%[ydst]]!               \n\t"
        "vst1.64 {d10},[%[ydst]]!               \n\t"
        "vst1.64 {d14},[%[ydst]]!               \n\t"
        "vst1.64 {d16},[%[ydst]]!               \n\t"
        "vst1.64 {d20},[%[ydst]]!               \n\t"
        "vst1.64 {d24},[%[ydst]]!               \n\t"
        "vst1.64 {d28},[%[ydst]]!               \n\t"
        "vst1.64 {d18},[%[ydst]]!               \n\t"
        "vst1.64 {d22},[%[ydst]]!               \n\t"
        "vst1.64 {d26},[%[ydst]]!               \n\t"
        "vst1.64 {d30},[%[ydst]]!               \n\t"
        "vst1.64 {d1}, [%[ydst]]!               \n\t"
        "vst1.64 {d5}, [%[ydst]]!               \n\t"
        "vst1.64 {d9}, [%[ydst]]!               \n\t"
        "vst1.64 {d13},[%[ydst]]!               \n\t"
        "vst1.64 {d3}, [%[ydst]]!               \n\t"
        "vst1.64 {d7}, [%[ydst]]!               \n\t"
        "vst1.64 {d11},[%[ydst]]!               \n\t"
        "vst1.64 {d15},[%[ydst]]!               \n\t"
        "vst1.64 {d17},[%[ydst]]!              \n\t"
        "vst1.64 {d21},[%[ydst]]!              \n\t"
        "vst1.64 {d25},[%[ydst]]!              \n\t"
        "vst1.64 {d29},[%[ydst]]!              \n\t"
        "vst1.64 {d19},[%[ydst]]!              \n\t"
        "vst1.64 {d23},[%[ydst]]!              \n\t"
        "vst1.64 {d27},[%[ydst]]!              \n\t"
        "vst1.64 {d31},[%[ydst]]!              \n\t"
        ////================================
        "subs %[cycle], %[cycle], #1           \n\t"
        "bne 0b                                \n\t"
        : [ydst] "+r"(ydst),
        [cycle]"+r"(cycle)
        : [ysrc] "r"(ysrc),
        [yref] "r"(yref), [stride] "r"(stride),
        [ref_stride] "r"(ref_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7",
        "q8", "q9", "q10", "q11",
        "q12", "q13", "q14", "q15"
    );
}

void YV12_pUV_ext_asm_direct(unsigned short *dst, uint8_t *usrc, uint8_t *vsrc, uint8_t *uref, uint8_t *vref, uint_t stride)
{
    unsigned short *udst;
    unsigned short *vdst;
    uint8_t cycle = 2;
    uint_t src_stride = stride / 2;
    stride = 8;
    udst = &dst[256];
    vdst = &dst[320];
    asm volatile(
        "0:@two cycle                               \n\t"
        "vld1.64 {d1}, [%[uref]], %[stride]     \n\t"
        "vld1.64 {d3}, [%[vref]], %[stride]     \n\t"
        "vld1.64 {d0}, [%[usrc]], %[src_stride]     \n\t"
        "vld1.64 {d2}, [%[vsrc]], %[src_stride]     \n\t"
        "vsubl.u8 q0,d0,d1                          \n\t"
        "vsubl.u8 q1,d2,d3                          \n\t"
        "vld1.64 {d5}, [%[uref]], %[stride]     \n\t"
        "vld1.64 {d7}, [%[vref]], %[stride]     \n\t"
        "vld1.64 {d4}, [%[usrc]], %[src_stride]      \n\t"
        "vld1.64 {d6}, [%[vsrc]], %[src_stride]      \n\t"
        "vsubl.u8 q2,d4,d5                          \n\t"
        "vsubl.u8 q3,d6,d7                          \n\t"
        "vld1.64 {d9}, [%[uref]], %[stride]     \n\t"
        "vld1.64 {d11}, [%[vref]], %[stride]     \n\t"
        "vld1.64 {d8}, [%[usrc]], %[src_stride]     \n\t"
        "vld1.64 {d10}, [%[vsrc]], %[src_stride]    \n\t"
        "vsubl.u8 q4,d8,d9                          \n\t"
        "vsubl.u8 q5,d10,d11                        \n\t"
        "vld1.64 {d13}, [%[uref]], %[stride]     \n\t"
        "vld1.64 {d15}, [%[vref]], %[stride]     \n\t"
        "vld1.64 {d12}, [%[usrc]], %[src_stride]    \n\t"
        "vld1.64 {d14}, [%[vsrc]], %[src_stride]    \n\t"
        "vsubl.u8 q6,d12,d13                        \n\t"
        "vsubl.u8 q7,d14,d15                        \n\t"
        "vst1.64 {d0}, [%[udst]]!                   \n\t"
        "vst1.64 {d4}, [%[udst]]!                   \n\t"
        "vst1.64 {d8}, [%[udst]]!                   \n\t"
        "vst1.64 {d12},[%[udst]]!                   \n\t"
        "vst1.64 {d1}, [%[udst]]!                   \n\t"
        "vst1.64 {d5}, [%[udst]]!                   \n\t"
        "vst1.64 {d9}, [%[udst]]!                   \n\t"
        "vst1.64 {d13},[%[udst]]!                   \n\t"
        "vst1.64 {d2}, [%[vdst]]!                   \n\t"
        "vst1.64 {d6}, [%[vdst]]!                   \n\t"
        "vst1.64 {d10},[%[vdst]]!                   \n\t"
        "vst1.64 {d14},[%[vdst]]!                   \n\t"
        "vst1.64 {d3}, [%[vdst]]!                   \n\t"
        "vst1.64 {d7}, [%[vdst]]!                   \n\t"
        "vst1.64 {d11},[%[vdst]]!                   \n\t"
        "vst1.64 {d15},[%[vdst]]!                   \n\t"
        "subs %[cycle], %[cycle], #1                 \n\t"
        "bne 0b"
        : [udst] "+r"(udst), [vdst] "+r"(vdst),
        [cycle] "+r"(cycle)
        : [usrc] "r"(usrc), [vsrc] "r"(vsrc), [uref] "r"(uref), [vref] "r"(vref),
        [stride] "r"(stride), [src_stride] "r"(src_stride)
        : "cc", "memory", "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7"
    );
}

void NV21_pUV_ext_asm_direct(unsigned short *dst, uint8_t *uvsrc, uint8_t *uref, uint8_t *vref,  uint_t stride)
{
    unsigned short *udst;
    unsigned short *vdst;
    uint_t ref_stride = 8;
    uint8_t cycle = 2;
    udst = &dst[256];
    vdst = &dst[320];
    asm volatile(
        "0:@two cycle                               \n\t"
        "vld2.8 {d0,d2},  [%[uvsrc]], %[stride]     \n\t"
        "vld1.64 {d1}, [%[uref]], %[ref_stride]     \n\t"
        "vld1.64 {d3}, [%[vref]], %[ref_stride]     \n\t"
        "vsubl.u8 q0,d0,d1                          \n\t"
        "vsubl.u8 q1,d2,d3                          \n\t"
        "vld2.8 {d4,d6},  [%[uvsrc]], %[stride]     \n\t"
        "vld1.64 {d5}, [%[uref]], %[ref_stride]     \n\t"
        "vld1.64 {d7}, [%[vref]], %[ref_stride]     \n\t"
        "vsubl.u8 q2,d4,d5                          \n\t"
        "vsubl.u8 q3,d6,d7                          \n\t"
        "vld2.8 {d8,d10}, [%[uvsrc]], %[stride]     \n\t"
        "vld1.64 {d9}, [%[uref]], %[ref_stride]     \n\t"
        "vld1.64 {d11}, [%[vref]], %[ref_stride]     \n\t"
        "vsubl.u8 q4,d8,d9                          \n\t"
        "vsubl.u8 q5,d10,d11                        \n\t"
        "vld2.8 {d12,d14},[%[uvsrc]], %[stride]     \n\t"
        "vld1.64 {d13}, [%[uref]], %[ref_stride]     \n\t"
        "vld1.64 {d15}, [%[vref]], %[ref_stride]     \n\t"
        "vsubl.u8 q6,d12,d13                        \n\t"
        "vsubl.u8 q7,d14,d15                        \n\t"
        "vst1.64 {d0}, [%[vdst]]!                   \n\t"
        "vst1.64 {d4}, [%[vdst]]!                   \n\t"
        "vst1.64 {d8}, [%[vdst]]!                   \n\t"
        "vst1.64 {d12},[%[vdst]]!                   \n\t"
        "vst1.64 {d1}, [%[vdst]]!                   \n\t"
        "vst1.64 {d5}, [%[vdst]]!                   \n\t"
        "vst1.64 {d9}, [%[vdst]]!                   \n\t"
        "vst1.64 {d13},[%[vdst]]!                   \n\t"
        "vst1.64 {d2}, [%[udst]]!                   \n\t"
        "vst1.64 {d6}, [%[udst]]!                   \n\t"
        "vst1.64 {d10},[%[udst]]!                   \n\t"
        "vst1.64 {d14},[%[udst]]!                   \n\t"
        "vst1.64 {d3}, [%[udst]]!                   \n\t"
        "vst1.64 {d7}, [%[udst]]!                   \n\t"
        "vst1.64 {d11},[%[udst]]!                   \n\t"
        "vst1.64 {d15},[%[udst]]!                   \n\t"
        "subs %[cycle], %[cycle], #1                 \n\t"
        "bne 0b"
        : [udst] "+r"(udst), [vdst] "+r"(vdst), [cycle] "+r"(cycle)
        : [uvsrc] "r"(uvsrc), [uref] "r"(uref), [vref] "r"(vref),  [stride] "r"(stride),
        [ref_stride] "r"(ref_stride)
        : "cc",  "memory",  "q0", "q1", "q2", "q3",
        "q4", "q5", "q6", "q7"
    );
}
#endif
