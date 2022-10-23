/****************************************************************************
*
*    Copyright (c) 2005 - 2019 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/


/*
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * except that you don't need to include two pages of legalese
 * with every copy.
 */

/**
**  @file
**  Architecture independent hash generate.
**
*/

#include "gc_hal_user_precomp.h"


static void __hash_swapUINT(gctUINT8_PTR data, gctSIZE_T uints)
{
    do
    {
        gctUINT32 swapped = ((gctUINT32)data[3]) << 24
                          | ((gctUINT32)data[2]) << 16
                          | ((gctUINT32)data[1]) << 8
                          | ((gctUINT32)data[0]);
        *(gctUINT32_PTR)data = swapped;

        data += 4;
    } while (--uints);
}

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */
static void __hash_MD5Transform(
    gcsHASH_MD5CTX *ctx,
    gctUINT32 data[16]
    )
{
    gctUINT32_PTR p = data;
    register gctUINT32 a, b, c, d;

    a = ctx->states[0];
    b = ctx->states[1];
    c = ctx->states[2];
    d = ctx->states[3];

    if (ctx->bigEndian)
    {
        /* Input data need to be copied to scratch before byte swap. */
        if (p != (gctUINT32_PTR)ctx->buffer)
        {
            static gctUINT32 scratch[16];
            gcoOS_MemCopy(scratch, data, sizeof(gctUINT32) * 16);
            p = scratch;
        }
        __hash_swapUINT((gctUINT8_PTR)p, 16);
    }

#define gcdHASH_MD5STEP(f, w, x, y, z, data, s) \
  (w += f(x, y, z) + data,  w = w << s | w >> (32 - s),  w += x )

#define gcdHASH_F1(x, y, z) (z ^ (x & (y ^ z)))
    gcdHASH_MD5STEP(gcdHASH_F1, a, b, c, d, p[ 0] + 0xd76aa478,  7);
    gcdHASH_MD5STEP(gcdHASH_F1, d, a, b, c, p[ 1] + 0xe8c7b756, 12);
    gcdHASH_MD5STEP(gcdHASH_F1, c, d, a, b, p[ 2] + 0x242070db, 17);
    gcdHASH_MD5STEP(gcdHASH_F1, b, c, d, a, p[ 3] + 0xc1bdceee, 22);
    gcdHASH_MD5STEP(gcdHASH_F1, a, b, c, d, p[ 4] + 0xf57c0faf,  7);
    gcdHASH_MD5STEP(gcdHASH_F1, d, a, b, c, p[ 5] + 0x4787c62a, 12);
    gcdHASH_MD5STEP(gcdHASH_F1, c, d, a, b, p[ 6] + 0xa8304613, 17);
    gcdHASH_MD5STEP(gcdHASH_F1, b, c, d, a, p[ 7] + 0xfd469501, 22);
    gcdHASH_MD5STEP(gcdHASH_F1, a, b, c, d, p[ 8] + 0x698098d8,  7);
    gcdHASH_MD5STEP(gcdHASH_F1, d, a, b, c, p[ 9] + 0x8b44f7af, 12);
    gcdHASH_MD5STEP(gcdHASH_F1, c, d, a, b, p[10] + 0xffff5bb1, 17);
    gcdHASH_MD5STEP(gcdHASH_F1, b, c, d, a, p[11] + 0x895cd7be, 22);
    gcdHASH_MD5STEP(gcdHASH_F1, a, b, c, d, p[12] + 0x6b901122,  7);
    gcdHASH_MD5STEP(gcdHASH_F1, d, a, b, c, p[13] + 0xfd987193, 12);
    gcdHASH_MD5STEP(gcdHASH_F1, c, d, a, b, p[14] + 0xa679438e, 17);
    gcdHASH_MD5STEP(gcdHASH_F1, b, c, d, a, p[15] + 0x49b40821, 22);
#undef gcdHASH_F1

#define gcdHASH_F2(x, y, z) (y ^ (z & (x ^ y)))
    gcdHASH_MD5STEP(gcdHASH_F2, a, b, c, d, p[ 1] + 0xf61e2562,  5);
    gcdHASH_MD5STEP(gcdHASH_F2, d, a, b, c, p[ 6] + 0xc040b340,  9);
    gcdHASH_MD5STEP(gcdHASH_F2, c, d, a, b, p[11] + 0x265e5a51, 14);
    gcdHASH_MD5STEP(gcdHASH_F2, b, c, d, a, p[ 0] + 0xe9b6c7aa, 20);
    gcdHASH_MD5STEP(gcdHASH_F2, a, b, c, d, p[ 5] + 0xd62f105d,  5);
    gcdHASH_MD5STEP(gcdHASH_F2, d, a, b, c, p[10] + 0x02441453,  9);
    gcdHASH_MD5STEP(gcdHASH_F2, c, d, a, b, p[15] + 0xd8a1e681, 14);
    gcdHASH_MD5STEP(gcdHASH_F2, b, c, d, a, p[ 4] + 0xe7d3fbc8, 20);
    gcdHASH_MD5STEP(gcdHASH_F2, a, b, c, d, p[ 9] + 0x21e1cde6,  5);
    gcdHASH_MD5STEP(gcdHASH_F2, d, a, b, c, p[14] + 0xc33707d6,  9);
    gcdHASH_MD5STEP(gcdHASH_F2, c, d, a, b, p[ 3] + 0xf4d50d87, 14);
    gcdHASH_MD5STEP(gcdHASH_F2, b, c, d, a, p[ 8] + 0x455a14ed, 20);
    gcdHASH_MD5STEP(gcdHASH_F2, a, b, c, d, p[13] + 0xa9e3e905,  5);
    gcdHASH_MD5STEP(gcdHASH_F2, d, a, b, c, p[ 2] + 0xfcefa3f8,  9);
    gcdHASH_MD5STEP(gcdHASH_F2, c, d, a, b, p[ 7] + 0x676f02d9, 14);
    gcdHASH_MD5STEP(gcdHASH_F2, b, c, d, a, p[12] + 0x8d2a4c8a, 20);
#undef gcdHASH_F2

#define gcdHASH_F3(x, y, z) (x ^ y ^ z)
    gcdHASH_MD5STEP(gcdHASH_F3, a, b, c, d, p[ 5] + 0xfffa3942,  4);
    gcdHASH_MD5STEP(gcdHASH_F3, d, a, b, c, p[ 8] + 0x8771f681, 11);
    gcdHASH_MD5STEP(gcdHASH_F3, c, d, a, b, p[11] + 0x6d9d6122, 16);
    gcdHASH_MD5STEP(gcdHASH_F3, b, c, d, a, p[14] + 0xfde5380c, 23);
    gcdHASH_MD5STEP(gcdHASH_F3, a, b, c, d, p[ 1] + 0xa4beea44,  4);
    gcdHASH_MD5STEP(gcdHASH_F3, d, a, b, c, p[ 4] + 0x4bdecfa9, 11);
    gcdHASH_MD5STEP(gcdHASH_F3, c, d, a, b, p[ 7] + 0xf6bb4b60, 16);
    gcdHASH_MD5STEP(gcdHASH_F3, b, c, d, a, p[10] + 0xbebfbc70, 23);
    gcdHASH_MD5STEP(gcdHASH_F3, a, b, c, d, p[13] + 0x289b7ec6,  4);
    gcdHASH_MD5STEP(gcdHASH_F3, d, a, b, c, p[ 0] + 0xeaa127fa, 11);
    gcdHASH_MD5STEP(gcdHASH_F3, c, d, a, b, p[ 3] + 0xd4ef3085, 16);
    gcdHASH_MD5STEP(gcdHASH_F3, b, c, d, a, p[ 6] + 0x04881d05, 23);
    gcdHASH_MD5STEP(gcdHASH_F3, a, b, c, d, p[ 9] + 0xd9d4d039,  4);
    gcdHASH_MD5STEP(gcdHASH_F3, d, a, b, c, p[12] + 0xe6db99e5, 11);
    gcdHASH_MD5STEP(gcdHASH_F3, c, d, a, b, p[15] + 0x1fa27cf8, 16);
    gcdHASH_MD5STEP(gcdHASH_F3, b, c, d, a, p[ 2] + 0xc4ac5665, 23);
#undef gcdHASH_F3

#define gcdHASH_F4(x, y, z) (y ^ (x | ~z))
    gcdHASH_MD5STEP(gcdHASH_F4, a, b, c, d, p[ 0] + 0xf4292244,  6);
    gcdHASH_MD5STEP(gcdHASH_F4, d, a, b, c, p[ 7] + 0x432aff97, 10);
    gcdHASH_MD5STEP(gcdHASH_F4, c, d, a, b, p[14] + 0xab9423a7, 15);
    gcdHASH_MD5STEP(gcdHASH_F4, b, c, d, a, p[ 5] + 0xfc93a039, 21);
    gcdHASH_MD5STEP(gcdHASH_F4, a, b, c, d, p[12] + 0x655b59c3,  6);
    gcdHASH_MD5STEP(gcdHASH_F4, d, a, b, c, p[ 3] + 0x8f0ccc92, 10);
    gcdHASH_MD5STEP(gcdHASH_F4, c, d, a, b, p[10] + 0xffeff47d, 15);
    gcdHASH_MD5STEP(gcdHASH_F4, b, c, d, a, p[ 1] + 0x85845dd1, 21);
    gcdHASH_MD5STEP(gcdHASH_F4, a, b, c, d, p[ 8] + 0x6fa87e4f,  6);
    gcdHASH_MD5STEP(gcdHASH_F4, d, a, b, c, p[15] + 0xfe2ce6e0, 10);
    gcdHASH_MD5STEP(gcdHASH_F4, c, d, a, b, p[ 6] + 0xa3014314, 15);
    gcdHASH_MD5STEP(gcdHASH_F4, b, c, d, a, p[13] + 0x4e0811a1, 21);
    gcdHASH_MD5STEP(gcdHASH_F4, a, b, c, d, p[ 4] + 0xf7537e82,  6);
    gcdHASH_MD5STEP(gcdHASH_F4, d, a, b, c, p[11] + 0xbd3af235, 10);
    gcdHASH_MD5STEP(gcdHASH_F4, c, d, a, b, p[ 2] + 0x2ad7d2bb, 15);
    gcdHASH_MD5STEP(gcdHASH_F4, b, c, d, a, p[ 9] + 0xeb86d391, 21);
#undef gcdHASH_F4

#undef gcdHASH_MD5STEP

    ctx->states[0] += a;
    ctx->states[1] += b;
    ctx->states[2] += c;
    ctx->states[3] += d;
}


void gcsHASH_MD5Init(
    gcsHASH_MD5CTX *ctx
    )
{
    const gctUINT16 data = 0xff00;

    gcoOS_ZeroMemory(ctx, gcmSIZEOF(gcsHASH_MD5CTX));

    ctx->bigEndian = (*(gctUINT8*)&data == 0xff);
    ctx->bytes = 0;

    /* mysterious initial constants */
    ctx->states[0] = 0x67452301;
    ctx->states[1] = 0xefcdab89;
    ctx->states[2] = 0x98badcfe;
    ctx->states[3] = 0x10325476;
}

/* Update context to reflect the concatenation of another buffer full of bytes */
void gcsHASH_MD5Update(
    gcsHASH_MD5CTX *ctx,
    const void *data,
    gctSIZE_T bytes
    )
{
    const gctUINT8 *buf = (const gctUINT8*)data;
    gctSIZE_T left = ctx->bytes & 0x3F;
    gctSIZE_T fill = 64 - left;

    ctx->bytes += bytes;

    /* Handle any leading odd-sized chunks */
    if (left > 0 && fill <= bytes)
    {
        gcoOS_MemCopy(&ctx->buffer[left], buf, fill);

        __hash_MD5Transform(ctx, (gctUINT32_PTR)ctx->buffer);

        buf   += fill;
        bytes -= fill;
        left = 0;

    }

    /* Process data in 64-bytes chunks */
    while (bytes >= 64)
    {
        __hash_MD5Transform(ctx, (gctUINT32_PTR)buf);
        buf   += 64;
        bytes -= 64;
    }

    /* Copy any remaining bytes for next process */
    if (bytes > 0)
    {
        gcoOS_MemCopy(&ctx->buffer[left], buf, bytes);
    }
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
void gcsHASH_MD5Final(
    gcsHASH_MD5CTX *ctx,
    gctUINT8 digest[16]
    )
{
    gctSIZE_T left, fill;
    gctUINT64 bits = 0;

    left = ctx->bytes & 0x3F;
    ctx->buffer[left++] = 0x80;     /* padding 0x80*/
    fill = 64 - left;

    if (fill > 0)
    {
        gcoOS_ZeroMemory(&ctx->buffer[left], fill);
    }

    if (fill < sizeof(bits))
    {
        __hash_MD5Transform(ctx, (gctUINT32_PTR)ctx->buffer);
        gcoOS_ZeroMemory(ctx->buffer, 64);
        left = 0;
    }

    /* Append length in bits and transform */
    bits = ctx->bytes << 3;
    gcoOS_MemCopy(&ctx->buffer[left], &bits, sizeof(bits));

    __hash_MD5Transform(ctx, (gctUINT32_PTR)ctx->buffer);
    if (ctx->bigEndian)
    {
        __hash_swapUINT((gctUINT8_PTR)ctx->states, 4);
    }
    gcoOS_MemCopy(digest, ctx->states, 16);
}


