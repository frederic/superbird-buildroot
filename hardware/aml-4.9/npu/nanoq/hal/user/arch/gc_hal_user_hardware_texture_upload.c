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


#include "gc_hal_user_hardware_precomp.h"

#if gcdENABLE_3D

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_HARDWARE

/*
 * SuperTileMode 2:
 * 21'{ X[21-1:6], Y[5],X[5],Y[4],X[4], Y[3],X[3],Y[2],X[2],Y[1:0],X[1:0] }
 *
 * SuperTileMode 1:
 * 21'{ X[21-1:6], Y[5:4],X[5:3],Y[3:2], X[2],Y[1:0],X[1:0] }
 *
 * SuperTileMode 0:
 * 21'{ X[21-1:6], Y[5:2],X[5:2], Y[1:0],X[1:0] }
 */
#define gcmSUPERTILE_OFFSET_X(X, Y, SuperTileMode) \
        (SuperTileMode == 2) ? \
            (((X) &  0x03) << 0) | \
            (((Y) &  0x03) << 2) | \
            (((X) &  0x04) << 2) | \
            (((Y) &  0x04) << 3) | \
            (((X) &  0x08) << 3) | \
            (((Y) &  0x08) << 4) | \
            (((X) &  0x10) << 4) | \
            (((Y) &  0x10) << 5) | \
            (((X) &  0x20) << 5) | \
            (((Y) &  0x20) << 6) | \
            (((X) & ~0x3F) << 6)   \
        : \
        (SuperTileMode == 1) ? \
            (((X) &  0x03) << 0) | \
            (((Y) &  0x03) << 2) | \
            (((X) &  0x04) << 2) | \
            (((Y) &  0x0C) << 3) | \
            (((X) &  0x38) << 4) | \
            (((Y) &  0x30) << 6) | \
            (((X) & ~0x3F) << 6)   \
        : \
            (((X) &  0x03) << 0) | \
            (((Y) &  0x03) << 2) | \
            (((X) &  0x3C) << 2) | \
            (((Y) &  0x3C) << 6) | \
            (((X) & ~0x3F) << 6)

#define gcmSUPERTILE_OFFSET_Y(X, Y) \
        ((Y) & ~0x3F)


/* 8b 'LLLLLLLL' => 32b 'AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB' */
#define L8toARGB(u) \
    ((u) | ((u) << 8) | ((u) << 16) | 0xFF000000)


/* 8b 'LLLLLLLL' => 32b 'BBBBBBBB GGGGGGGG RRRRRRRR AAAAAAAA' */
#define L8toARGBBE(u) \
    (((u) << 8) | ((u) << 16) | ((u) << 24) | 0xFF)


/* 16b 'RRRRRGGG GGGBBBBB' => 32b 'AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB' */
#define RGB565toARGB(u) \
    (0xFF000000 | ((u & 0xF800) << 8) | ((u & 0xE000) << 3) \
    | ((u & 0x7E0) << 5) | ((u & 0x600) >> 1) | ((u & 0x1F) << 3) | ((u & 0x1C) >> 2))


/* 16b 'GGGBBBBB RRRRRGGG' => 32b 'BBBBBBBB GGGGGGGG RRRRRRRR AAAAAAAA' */
#define RGB565toARGBBE(u) \
    (0xFF | ((u & 0xF8) << 8) | ((u & 0xE0) << 3) \
    | ((u & 0x7) << 21) | ((u & 0xE000) << 5) | ((u & 0x7) << 16) | ((u & 0x1F00) << 19) | ((u & 0x1C00) << 14))


/* 16b 'RRRRGGGG BBBBAAAA' to 32b 'AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB' */
#define RGBA4444toARGB(u) \
    (((u & 0xF) << 28) | ((u & 0xF) << 24) | ((u & 0xF000) << 8) | ((u & 0xF000) << 4) \
    | ((u & 0xF00) << 4) | (u & 0xF00) | (u & 0xF0) | ((u & 0xF0) >> 4))


/* 16b 'BBBBAAAA RRRRGGGG' to 32b 'BBBBBBBB GGGGGGGG RRRRRRRR AAAAAAAA' */
#define RGBA4444toARGBBE(u) \
    (((u & 0xF00) >> 4) | ((u & 0xF00) >> 8) | ((u & 0xF00) << 8) | ((u & 0xF00) << 4) \
    | ((u & 0xF) << 20) | ((u & 0xF) << 16) | ((u & 0xF000) << 16) | ((u & 0xF000) << 12))


/* 16b 'RRRRRGGG GGBBBBBA' to 32b 'AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBBB' */
#define RGBA5551toARGB(u) \
    (((u & 0x1) ? 0xFF000000 : 0) | ((u & 0xF800) << 8) | ((u & 0xE000) << 3) \
    | ((u & 0x7C0) << 5) | (u & 0x700) | ((u & 0x3E) << 2) | ((u & 0x38) >> 3))


/* 16b 'GGBBBBBA RRRRRGGG' to 32b 'BBBBBBBBB GGGGGGGG RRRRRRRR AAAAAAAA' */
#define RGBA5551toARGBBE(u) \
    (((u & 0x100) ? 0xFF : 0) | ((u & 0xF8) << 8) | ((u & 0xE0) << 3) \
    | ((u & 0x7) << 19) | ((u & 0xC000) << 5) | ((u & 0x7) << 16) | ((u & 0x3E00) << 18) | ((u & 0x3800) << 13))


static void
_UploadA8toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 1);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* A8 to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = srcLine[0] << 24;
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* A8 to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = srcLine[0] << 24;
                ((gctUINT32_PTR) trgLine)[1] = srcLine[1] << 24;
                ((gctUINT32_PTR) trgLine)[2] = srcLine[2] << 24;
                ((gctUINT32_PTR) trgLine)[3] = srcLine[3] << 24;
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* A8 to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = srcLine[SourceStride * 0] << 24;
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 1;

        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;

            /* A8 to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = src[0] << 24;
            ((gctUINT32_PTR) trgLine)[ 1] = src[1] << 24;
            ((gctUINT32_PTR) trgLine)[ 2] = src[2] << 24;
            ((gctUINT32_PTR) trgLine)[ 3] = src[3] << 24;

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = src[0] << 24;
            ((gctUINT32_PTR) trgLine)[ 5] = src[1] << 24;
            ((gctUINT32_PTR) trgLine)[ 6] = src[2] << 24;
            ((gctUINT32_PTR) trgLine)[ 7] = src[3] << 24;

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = src[0] << 24;
            ((gctUINT32_PTR) trgLine)[ 9] = src[1] << 24;
            ((gctUINT32_PTR) trgLine)[ 0] = src[2] << 24;
            ((gctUINT32_PTR) trgLine)[11] = src[3] << 24;

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = src[0] << 24;
            ((gctUINT32_PTR) trgLine)[13] = src[1] << 24;
            ((gctUINT32_PTR) trgLine)[14] = src[2] << 24;
            ((gctUINT32_PTR) trgLine)[15] = src[3] << 24;

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 1;
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadA8toARGBBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 1);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* A8 to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = (gctUINT32) srcLine[0];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* A8 to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (gctUINT32) srcLine[0];
                ((gctUINT32_PTR) trgLine)[1] = (gctUINT32) srcLine[1];
                ((gctUINT32_PTR) trgLine)[2] = (gctUINT32) srcLine[2];
                ((gctUINT32_PTR) trgLine)[3] = (gctUINT32) srcLine[3];
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* A8 to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[ 0] = (gctUINT32) srcLine[SourceStride * 0];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 1;

        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;

            /* A8 to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = (gctUINT32) src[0];
            ((gctUINT32_PTR) trgLine)[ 1] = (gctUINT32) src[1];
            ((gctUINT32_PTR) trgLine)[ 2] = (gctUINT32) src[2];
            ((gctUINT32_PTR) trgLine)[ 3] = (gctUINT32) src[3];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = (gctUINT32) src[0];
            ((gctUINT32_PTR) trgLine)[ 5] = (gctUINT32) src[1];
            ((gctUINT32_PTR) trgLine)[ 6] = (gctUINT32) src[2];
            ((gctUINT32_PTR) trgLine)[ 7] = (gctUINT32) src[3];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = (gctUINT32) src[0];
            ((gctUINT32_PTR) trgLine)[ 9] = (gctUINT32) src[1];
            ((gctUINT32_PTR) trgLine)[ 0] = (gctUINT32) src[2];
            ((gctUINT32_PTR) trgLine)[11] = (gctUINT32) src[3];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = (gctUINT32) src[0];
            ((gctUINT32_PTR) trgLine)[13] = (gctUINT32) src[1];
            ((gctUINT32_PTR) trgLine)[14] = (gctUINT32) src[2];
            ((gctUINT32_PTR) trgLine)[15] = (gctUINT32) src[3];

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 1;
        }
    }
}
#endif

static void
_UploadBGRtoARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 3);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

            /* BGR to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | (srcLine[0] << 16) | (srcLine[1] << 8) | srcLine[2];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | (srcLine[0] << 16) | (srcLine[ 1] << 8) | srcLine[ 2];
                ((gctUINT32_PTR) trgLine)[1] = 0xFF000000 | (srcLine[3] << 16) | (srcLine[ 4] << 8) | srcLine[ 5];
                ((gctUINT32_PTR) trgLine)[2] = 0xFF000000 | (srcLine[6] << 16) | (srcLine[ 7] << 8) | srcLine[ 8];
                ((gctUINT32_PTR) trgLine)[3] = 0xFF000000 | (srcLine[9] << 16) | (srcLine[10] << 8) | srcLine[11];
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[ 0] = 0xFF000000 | (srcLine[0] << 16) | (srcLine[1] << 8) | srcLine[2];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 3;

        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;

            /* BGR to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = 0xFF000000 | (src[0] << 16) | (src[ 1] << 8) | src[ 2];
            ((gctUINT32_PTR) trgLine)[ 1] = 0xFF000000 | (src[3] << 16) | (src[ 4] << 8) | src[ 5];
            ((gctUINT32_PTR) trgLine)[ 2] = 0xFF000000 | (src[6] << 16) | (src[ 7] << 8) | src[ 8];
            ((gctUINT32_PTR) trgLine)[ 3] = 0xFF000000 | (src[9] << 16) | (src[10] << 8) | src[11];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = 0xFF000000 | (src[0] << 16) | (src[ 1] << 8) | src[ 2];
            ((gctUINT32_PTR) trgLine)[ 5] = 0xFF000000 | (src[3] << 16) | (src[ 4] << 8) | src[ 5];
            ((gctUINT32_PTR) trgLine)[ 6] = 0xFF000000 | (src[6] << 16) | (src[ 7] << 8) | src[ 8];
            ((gctUINT32_PTR) trgLine)[ 7] = 0xFF000000 | (src[9] << 16) | (src[10] << 8) | src[11];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = 0xFF000000 | (src[0] << 16) | (src[ 1] << 8) | src[ 2];
            ((gctUINT32_PTR) trgLine)[ 9] = 0xFF000000 | (src[3] << 16) | (src[ 4] << 8) | src[ 5];
            ((gctUINT32_PTR) trgLine)[10] = 0xFF000000 | (src[6] << 16) | (src[ 7] << 8) | src[ 8];
            ((gctUINT32_PTR) trgLine)[11] = 0xFF000000 | (src[9] << 16) | (src[10] << 8) | src[11];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = 0xFF000000 | (src[0] << 16) | (src[ 1] << 8) | src[ 2];
            ((gctUINT32_PTR) trgLine)[13] = 0xFF000000 | (src[3] << 16) | (src[ 4] << 8) | src[ 5];
            ((gctUINT32_PTR) trgLine)[14] = 0xFF000000 | (src[6] << 16) | (src[ 7] << 8) | src[ 8];
            ((gctUINT32_PTR) trgLine)[15] = 0xFF000000 | (src[9] << 16) | (src[10] << 8) | src[11];

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 3;
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadBGRtoARGBBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 3);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

            /* BGR to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = 0xFF | (srcLine[0] << 8) | (srcLine[1] << 16) | (srcLine[2] << 24);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF | (srcLine[0] << 8) | (srcLine[ 1] << 16) | (srcLine[ 2] << 24);
                ((gctUINT32_PTR) trgLine)[1] = 0xFF | (srcLine[3] << 8) | (srcLine[ 4] << 16) | (srcLine[ 5] << 24);
                ((gctUINT32_PTR) trgLine)[2] = 0xFF | (srcLine[6] << 8) | (srcLine[ 7] << 16) | (srcLine[ 8] << 24);
                ((gctUINT32_PTR) trgLine)[3] = 0xFF | (srcLine[9] << 8) | (srcLine[10] << 16) | (srcLine[11] << 24);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF | (srcLine[0] << 8) | (srcLine[ 1] << 16) | (srcLine[ 2] << 24);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 3;

        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;

            /* BGR to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = 0xFF | (src[0] << 8) | (src[ 1] << 16) | (src[ 2] << 24);
            ((gctUINT32_PTR) trgLine)[ 1] = 0xFF | (src[3] << 8) | (src[ 4] << 16) | (src[ 5] << 24);
            ((gctUINT32_PTR) trgLine)[ 2] = 0xFF | (src[6] << 8) | (src[ 7] << 16) | (src[ 8] << 24);
            ((gctUINT32_PTR) trgLine)[ 3] = 0xFF | (src[9] << 8) | (src[10] << 16) | (src[11] << 24);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = 0xFF | (src[0] << 8) | (src[ 1] << 16) | (src[ 2] << 24);
            ((gctUINT32_PTR) trgLine)[ 5] = 0xFF | (src[3] << 8) | (src[ 4] << 16) | (src[ 5] << 24);
            ((gctUINT32_PTR) trgLine)[ 6] = 0xFF | (src[6] << 8) | (src[ 7] << 16) | (src[ 8] << 24);
            ((gctUINT32_PTR) trgLine)[ 7] = 0xFF | (src[9] << 8) | (src[10] << 16) | (src[11] << 24);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = 0xFF | (src[0] << 8) | (src[ 1] << 16) | (src[ 2] << 24);
            ((gctUINT32_PTR) trgLine)[ 9] = 0xFF | (src[3] << 8) | (src[ 4] << 16) | (src[ 5] << 24);
            ((gctUINT32_PTR) trgLine)[10] = 0xFF | (src[6] << 8) | (src[ 7] << 16) | (src[ 8] << 24);
            ((gctUINT32_PTR) trgLine)[11] = 0xFF | (src[9] << 8) | (src[10] << 16) | (src[11] << 24);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = 0xFF | (src[0] << 8) | (src[ 1] << 16) | (src[ 2] << 24);
            ((gctUINT32_PTR) trgLine)[13] = 0xFF | (src[3] << 8) | (src[ 4] << 16) | (src[ 5] << 24);
            ((gctUINT32_PTR) trgLine)[14] = 0xFF | (src[6] << 8) | (src[ 7] << 16) | (src[ 8] << 24);
            ((gctUINT32_PTR) trgLine)[15] = 0xFF | (src[9] << 8) | (src[10] << 16) | (src[11] << 24);

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 3;
        }
    }
}
#endif

static void
_UploadBGRtoABGR(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 3);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

            /* BGR to ABGR. */
            ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | srcLine[0] | (srcLine[1] << 8) | (srcLine[2] << 16);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ABGR: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | srcLine[0] | (srcLine[ 1] << 8) | (srcLine[ 2] << 16);
                ((gctUINT32_PTR) trgLine)[1] = 0xFF000000 | srcLine[3] | (srcLine[ 4] << 8) | (srcLine[ 5] << 16);
                ((gctUINT32_PTR) trgLine)[2] = 0xFF000000 | srcLine[6] | (srcLine[ 7] << 8) | (srcLine[ 8] << 16);
                ((gctUINT32_PTR) trgLine)[3] = 0xFF000000 | srcLine[9] | (srcLine[10] << 8) | (srcLine[11] << 16);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ABGR: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | srcLine[0] | (srcLine[1] << 8) | (srcLine[2] << 16);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 3;

        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;

            /* BGR to ABGR: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = 0xFF000000 | src[0] | (src[ 1] << 8) | (src[ 2] << 16);
            ((gctUINT32_PTR) trgLine)[ 1] = 0xFF000000 | src[3] | (src[ 4] << 8) | (src[ 5] << 16);
            ((gctUINT32_PTR) trgLine)[ 2] = 0xFF000000 | src[6] | (src[ 7] << 8) | (src[ 8] << 16);
            ((gctUINT32_PTR) trgLine)[ 3] = 0xFF000000 | src[9] | (src[10] << 8) | (src[11] << 16);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = 0xFF000000 | src[0] | (src[ 1] << 8) | (src[ 2] << 16);
            ((gctUINT32_PTR) trgLine)[ 5] = 0xFF000000 | src[3] | (src[ 4] << 8) | (src[ 5] << 16);
            ((gctUINT32_PTR) trgLine)[ 6] = 0xFF000000 | src[6] | (src[ 7] << 8) | (src[ 8] << 16);
            ((gctUINT32_PTR) trgLine)[ 7] = 0xFF000000 | src[9] | (src[10] << 8) | (src[11] << 16);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = 0xFF000000 | src[0] | (src[ 1] << 8) | (src[ 2] << 16);
            ((gctUINT32_PTR) trgLine)[ 9] = 0xFF000000 | src[3] | (src[ 4] << 8) | (src[ 5] << 16);
            ((gctUINT32_PTR) trgLine)[10] = 0xFF000000 | src[6] | (src[ 7] << 8) | (src[ 8] << 16);
            ((gctUINT32_PTR) trgLine)[11] = 0xFF000000 | src[9] | (src[10] << 8) | (src[11] << 16);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = 0xFF000000 | src[0] | (src[ 1] << 8) | (src[ 2] << 16);
            ((gctUINT32_PTR) trgLine)[13] = 0xFF000000 | src[3] | (src[ 4] << 8) | (src[ 5] << 16);
            ((gctUINT32_PTR) trgLine)[14] = 0xFF000000 | src[6] | (src[ 7] << 8) | (src[ 8] << 16);
            ((gctUINT32_PTR) trgLine)[15] = 0xFF000000 | src[9] | (src[10] << 8) | (src[11] << 16);

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 3;
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadBGRtoABGRBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 3);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

            /* BGR to ABGR. */
            ((gctUINT32_PTR) trgLine)[0] = 0xFF | (srcLine[0] << 24) | (srcLine[1] << 16) | (srcLine[2] << 8);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ABGR: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF | (srcLine[0] << 24) | (srcLine[ 1] << 16) | (srcLine[ 2] << 8);
                ((gctUINT32_PTR) trgLine)[1] = 0xFF | (srcLine[3] << 24) | (srcLine[ 4] << 16) | (srcLine[ 5] << 8);
                ((gctUINT32_PTR) trgLine)[2] = 0xFF | (srcLine[6] << 24) | (srcLine[ 7] << 16) | (srcLine[ 8] << 8);
                ((gctUINT32_PTR) trgLine)[3] = 0xFF | (srcLine[9] << 24) | (srcLine[10] << 16) | (srcLine[11] << 8);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ABGR: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF | (srcLine[0] << 24) | (srcLine[ 1] << 16) | (srcLine[ 2] << 8);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 3;

        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;

            /* BGR to ABGR: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = 0xFF | (src[0] << 24) | (src[ 1] << 16) | (src[ 2] << 8);
            ((gctUINT32_PTR) trgLine)[ 1] = 0xFF | (src[3] << 24) | (src[ 4] << 16) | (src[ 5] << 8);
            ((gctUINT32_PTR) trgLine)[ 2] = 0xFF | (src[6] << 24) | (src[ 7] << 16) | (src[ 8] << 8);
            ((gctUINT32_PTR) trgLine)[ 3] = 0xFF | (src[9] << 24) | (src[10] << 16) | (src[11] << 8);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = 0xFF | (src[0] << 24) | (src[ 1] << 16) | (src[ 2] << 8);
            ((gctUINT32_PTR) trgLine)[ 5] = 0xFF | (src[3] << 24) | (src[ 4] << 16) | (src[ 5] << 8);
            ((gctUINT32_PTR) trgLine)[ 6] = 0xFF | (src[6] << 24) | (src[ 7] << 16) | (src[ 8] << 8);
            ((gctUINT32_PTR) trgLine)[ 7] = 0xFF | (src[9] << 24) | (src[10] << 16) | (src[11] << 8);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = 0xFF | (src[0] << 24) | (src[ 1] << 16) | (src[ 2] << 8);
            ((gctUINT32_PTR) trgLine)[ 9] = 0xFF | (src[3] << 24) | (src[ 4] << 16) | (src[ 5] << 8);
            ((gctUINT32_PTR) trgLine)[10] = 0xFF | (src[6] << 24) | (src[ 7] << 16) | (src[ 8] << 8);
            ((gctUINT32_PTR) trgLine)[11] = 0xFF | (src[9] << 24) | (src[10] << 16) | (src[11] << 8);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = 0xFF | (src[0] << 24) | (src[ 1] << 16) | (src[ 2] << 8);
            ((gctUINT32_PTR) trgLine)[13] = 0xFF | (src[3] << 24) | (src[ 4] << 16) | (src[ 5] << 8);
            ((gctUINT32_PTR) trgLine)[14] = 0xFF | (src[6] << 24) | (src[ 7] << 16) | (src[ 8] << 8);
            ((gctUINT32_PTR) trgLine)[15] = 0xFF | (src[9] << 24) | (src[10] << 16) | (src[11] << 8);

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 3;
        }
    }
}
#endif

static void
_UploadABGRtoARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 4);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

            /* ABGR to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = (srcLine[0] << 16) | (srcLine[1] << 8) | srcLine[2] | (srcLine[3] << 24);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* ABGR to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[ 0] << 16) | (srcLine[ 1] << 8) | srcLine[ 2] | (srcLine[ 3] << 24);
                ((gctUINT32_PTR) trgLine)[1] = (srcLine[ 4] << 16) | (srcLine[ 5] << 8) | srcLine[ 6] | (srcLine[ 7] << 24);
                ((gctUINT32_PTR) trgLine)[2] = (srcLine[ 8] << 16) | (srcLine[ 9] << 8) | srcLine[10] | (srcLine[11] << 24);
                ((gctUINT32_PTR) trgLine)[3] = (srcLine[12] << 16) | (srcLine[13] << 8) | srcLine[14] | (srcLine[15] << 24);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* ABGR to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[ 0] << 16) | (srcLine[ 1] << 8) | srcLine[ 2] | (srcLine[ 3] << 24);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 4;

        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;

            /* ABGR to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = (src[ 0] << 16) | (src[ 1] << 8) | src[ 2] | (src[ 3] << 24);
            ((gctUINT32_PTR) trgLine)[ 1] = (src[ 4] << 16) | (src[ 5] << 8) | src[ 6] | (src[ 7] << 24);
            ((gctUINT32_PTR) trgLine)[ 2] = (src[ 8] << 16) | (src[ 9] << 8) | src[10] | (src[11] << 24);
            ((gctUINT32_PTR) trgLine)[ 3] = (src[12] << 16) | (src[13] << 8) | src[14] | (src[15] << 24);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = (src[ 0] << 16) | (src[ 1] << 8) | src[ 2] | (src[ 3] << 24);
            ((gctUINT32_PTR) trgLine)[ 5] = (src[ 4] << 16) | (src[ 5] << 8) | src[ 6] | (src[ 7] << 24);
            ((gctUINT32_PTR) trgLine)[ 6] = (src[ 8] << 16) | (src[ 9] << 8) | src[10] | (src[11] << 24);
            ((gctUINT32_PTR) trgLine)[ 7] = (src[12] << 16) | (src[13] << 8) | src[14] | (src[15] << 24);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = (src[ 0] << 16) | (src[ 1] << 8) | src[ 2] | (src[ 3] << 24);
            ((gctUINT32_PTR) trgLine)[ 9] = (src[ 4] << 16) | (src[ 5] << 8) | src[ 6] | (src[ 7] << 24);
            ((gctUINT32_PTR) trgLine)[10] = (src[ 8] << 16) | (src[ 9] << 8) | src[10] | (src[11] << 24);
            ((gctUINT32_PTR) trgLine)[11] = (src[12] << 16) | (src[13] << 8) | src[14] | (src[15] << 24);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = (src[ 0] << 16) | (src[ 1] << 8) | src[ 2] | (src[ 3] << 24);
            ((gctUINT32_PTR) trgLine)[13] = (src[ 4] << 16) | (src[ 5] << 8) | src[ 6] | (src[ 7] << 24);
            ((gctUINT32_PTR) trgLine)[14] = (src[ 8] << 16) | (src[ 9] << 8) | src[10] | (src[11] << 24);
            ((gctUINT32_PTR) trgLine)[15] = (src[12] << 16) | (src[13] << 8) | src[14] | (src[15] << 24);

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 4;
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadABGRtoARGBBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 4);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

            /* ABGR to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = (srcLine[0] << 8) | (srcLine[1] << 16) | (srcLine[2] << 24) | srcLine[3];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* ABGR to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[ 0] << 8) | (srcLine[ 1] << 16) | (srcLine[ 2] << 24) | srcLine[ 3];
                ((gctUINT32_PTR) trgLine)[1] = (srcLine[ 4] << 8) | (srcLine[ 5] << 16) | (srcLine[ 6] << 24) | srcLine[ 7];
                ((gctUINT32_PTR) trgLine)[2] = (srcLine[ 8] << 8) | (srcLine[ 9] << 16) | (srcLine[10] << 24) | srcLine[11];
                ((gctUINT32_PTR) trgLine)[3] = (srcLine[12] << 8) | (srcLine[13] << 16) | (srcLine[14] << 24) | srcLine[15];
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* ABGR to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[0] << 8) | (srcLine[1] << 16) | (srcLine[2] << 24) | srcLine[3];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 4;

        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;

            /* ABGR to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = (src[ 0] << 8) | (src[ 1] << 16) | (src[ 2] << 24) | src[ 3];
            ((gctUINT32_PTR) trgLine)[ 1] = (src[ 4] << 8) | (src[ 5] << 16) | (src[ 6] << 24) | src[ 7];
            ((gctUINT32_PTR) trgLine)[ 2] = (src[ 8] << 8) | (src[ 9] << 16) | (src[10] << 24) | src[11];
            ((gctUINT32_PTR) trgLine)[ 3] = (src[12] << 8) | (src[13] << 16) | (src[14] << 24) | src[15];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = (src[ 0] << 8) | (src[ 1] << 16) | (src[ 2] << 24) | src[ 3];
            ((gctUINT32_PTR) trgLine)[ 5] = (src[ 4] << 8) | (src[ 5] << 16) | (src[ 6] << 24) | src[ 7];
            ((gctUINT32_PTR) trgLine)[ 6] = (src[ 8] << 8) | (src[ 9] << 16) | (src[10] << 24) | src[11];
            ((gctUINT32_PTR) trgLine)[ 7] = (src[12] << 8) | (src[13] << 16) | (src[14] << 24) | src[15];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = (src[ 0] << 8) | (src[ 1] << 16) | (src[ 2] << 24) | src[ 3];
            ((gctUINT32_PTR) trgLine)[ 9] = (src[ 4] << 8) | (src[ 5] << 16) | (src[ 6] << 24) | src[ 7];
            ((gctUINT32_PTR) trgLine)[10] = (src[ 8] << 8) | (src[ 9] << 16) | (src[10] << 24) | src[11];
            ((gctUINT32_PTR) trgLine)[11] = (src[12] << 8) | (src[13] << 16) | (src[14] << 24) | src[15];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = (src[ 0] << 8) | (src[ 1] << 16) | (src[ 2] << 24) | src[ 3];
            ((gctUINT32_PTR) trgLine)[13] = (src[ 4] << 8) | (src[ 5] << 16) | (src[ 6] << 24) | src[ 7];
            ((gctUINT32_PTR) trgLine)[14] = (src[ 8] << 8) | (src[ 9] << 16) | (src[10] << 24) | src[11];
            ((gctUINT32_PTR) trgLine)[15] = (src[12] << 8) | (src[13] << 16) | (src[14] << 24) | src[15];

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 4;
        }
    }
}
#endif

static void
_UploadL8toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 1);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            u = *srcLine;

            /* L8 to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = L8toARGB(u);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* L8 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = L8toARGB(u);
                u = srcLine[1];
                ((gctUINT32_PTR) trgLine)[1] = L8toARGB(u);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[2] = L8toARGB(u);
                u = srcLine[3];
                ((gctUINT32_PTR) trgLine)[3] = L8toARGB(u);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* L8 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = L8toARGB(u);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 1;

        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;
            register gctUINT32 u;

            /* L8 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = L8toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 1] = L8toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 2] = L8toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 3] = L8toARGB(u);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = L8toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 5] = L8toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 6] = L8toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 7] = L8toARGB(u);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = L8toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 9] = L8toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[10] = L8toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[11] = L8toARGB(u);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = L8toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[13] = L8toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[14] = L8toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[15] = L8toARGB(u);

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 1;
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadL8toARGBBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 1);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* L8 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = L8toARGBBE(u);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* L8 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = L8toARGBBE(u);
                u = srcLine[1];
                ((gctUINT32_PTR) trgLine)[1] = L8toARGBBE(u);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[2] = L8toARGBBE(u);
                u = srcLine[3];
                ((gctUINT32_PTR) trgLine)[3] = L8toARGBBE(u);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* L8 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = L8toARGBBE(u);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 1;

        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;
            register gctUINT32 u;

            /* L8 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = L8toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 1] = L8toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 2] = L8toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 3] = L8toARGBBE(u);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = L8toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 5] = L8toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 6] = L8toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 7] = L8toARGBBE(u);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = L8toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 9] = L8toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[10] = L8toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[11] = L8toARGBBE(u);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = L8toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[13] = L8toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[14] = L8toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[15] = L8toARGBBE(u);

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 1;
        }
    }
}
#endif

static void
_UploadA8L8toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

            /* A8L8 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = u | (u << 8) | (u << 16) | (srcLine[1] << 24);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

                /* A8L8 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = u | (u << 8) | (u << 16) | (srcLine[1] << 24);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[1] = u | (u << 8) | (u << 16) | (srcLine[3] << 24);
                u = srcLine[4];
                ((gctUINT32_PTR) trgLine)[2] = u | (u << 8) | (u << 16) | (srcLine[5] << 24);
                u = srcLine[6];
                ((gctUINT32_PTR) trgLine)[3] = u | (u << 8) | (u << 16) | (srcLine[7] << 24);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

                /* A8L8 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = u | (u << 8) | (u << 16) | (srcLine[1] << 24);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 2;

        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;
            register gctUINT32 u;

            /* A8 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = u | (u << 8) | (u << 16) | (srcLine[1] << 24);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 1] = u | (u << 8) | (u << 16) | (srcLine[3] << 24);
            u = src[4];
            ((gctUINT32_PTR) trgLine)[ 2] = u | (u << 8) | (u << 16) | (srcLine[5] << 24);
            u = src[6];
            ((gctUINT32_PTR) trgLine)[ 3] = u | (u << 8) | (u << 16) | (srcLine[7] << 24);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = u | (u << 8) | (u << 16) | (srcLine[1] << 24);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 5] = u | (u << 8) | (u << 16) | (srcLine[3] << 24);
            u = src[4];
            ((gctUINT32_PTR) trgLine)[ 6] = u | (u << 8) | (u << 16) | (srcLine[5] << 24);
            u = src[6];
            ((gctUINT32_PTR) trgLine)[ 7] = u | (u << 8) | (u << 16) | (srcLine[7] << 24);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = u | (u << 8) | (u << 16) | (srcLine[1] << 24);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 9] = u | (u << 8) | (u << 16) | (srcLine[3] << 24);
            u = src[4];
            ((gctUINT32_PTR) trgLine)[10] = u | (u << 8) | (u << 16) | (srcLine[5] << 24);
            u = src[6];
            ((gctUINT32_PTR) trgLine)[11] = u | (u << 8) | (u << 16) | (srcLine[7] << 24);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = u | (u << 8) | (u << 16) | (srcLine[1] << 24);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[13] = u | (u << 8) | (u << 16) | (srcLine[3] << 24);
            u = src[4];
            ((gctUINT32_PTR) trgLine)[14] = u | (u << 8) | (u << 16) | (srcLine[5] << 24);
            u = src[6];
            ((gctUINT32_PTR) trgLine)[15] = u | (u << 8) | (u << 16) | (srcLine[7] << 24);

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 2;
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadA8L8toARGBBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

            /* A8L8 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = (u << 8) | (u << 16) | (u << 24) | srcLine[1];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

                /* A8L8 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = (u << 8) | (u << 16) | (u << 24) | srcLine[1];
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[1] = (u << 8) | (u << 16) | (u << 24) | srcLine[3];
                u = srcLine[4];
                ((gctUINT32_PTR) trgLine)[2] = (u << 8) | (u << 16) | (u << 24) | srcLine[5];
                u = srcLine[6];
                ((gctUINT32_PTR) trgLine)[3] = (u << 8) | (u << 16) | (u << 24) | srcLine[7];
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

                /* A8L8 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = (u << 8) | (u << 16) | (u << 24) | srcLine[1];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 2;

        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;
            register gctUINT32 u;

            /* A8 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = (u << 8) | (u << 16) | (u << 24) | srcLine[1];
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 1] = (u << 8) | (u << 16) | (u << 24) | srcLine[3];
            u = src[4];
            ((gctUINT32_PTR) trgLine)[ 2] = (u << 8) | (u << 16) | (u << 24) | srcLine[5];
            u = src[6];
            ((gctUINT32_PTR) trgLine)[ 3] = (u << 8) | (u << 16) | (u << 24) | srcLine[7];

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = (u << 8) | (u << 16) | (u << 24) | srcLine[1];
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 5] = (u << 8) | (u << 16) | (u << 24) | srcLine[3];
            u = src[4];
            ((gctUINT32_PTR) trgLine)[ 6] = (u << 8) | (u << 16) | (u << 24) | srcLine[5];
            u = src[6];
            ((gctUINT32_PTR) trgLine)[ 7] = (u << 8) | (u << 16) | (u << 24) | srcLine[7];

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = (u << 8) | (u << 16) | (u << 24) | srcLine[1];
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 9] = (u << 8) | (u << 16) | (u << 24) | srcLine[3];
            u = src[4];
            ((gctUINT32_PTR) trgLine)[10] = (u << 8) | (u << 16) | (u << 24) | srcLine[5];
            u = src[6];
            ((gctUINT32_PTR) trgLine)[11] = (u << 8) | (u << 16) | (u << 24) | srcLine[7];

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = (u << 8) | (u << 16) | (u << 24) | srcLine[1];
            u = src[2];
            ((gctUINT32_PTR) trgLine)[13] = (u << 8) | (u << 16) | (u << 24) | srcLine[3];
            u = src[4];
            ((gctUINT32_PTR) trgLine)[14] = (u << 8) | (u << 16) | (u << 24) | srcLine[5];
            u = src[6];
            ((gctUINT32_PTR) trgLine)[15] = (u << 8) | (u << 16) | (u << 24) | srcLine[7];

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 2;
        }
    }
}
#endif

static void
_Upload32bppto32bpp(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 4);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

            /* 32bpp to 32bpp. */
            ((gctUINT32_PTR) trgLine)[0] = srcLine[0] | (srcLine[1] << 8) | (srcLine[2] << 16) | (srcLine[3] << 24);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* 32 bpp to 32 bpp: one tile row. */
                if ((((gctUINTPTR_T)srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned source. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) srcLine)[0];
                    ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) srcLine)[1];
                    ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) srcLine)[2];
                    ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) srcLine)[3];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = srcLine[ 0] | (srcLine[ 1] << 8) | (srcLine[ 2] << 16) | (srcLine[ 3] << 24);
                    ((gctUINT32_PTR) trgLine)[1] = srcLine[ 4] | (srcLine[ 5] << 8) | (srcLine[ 6] << 16) | (srcLine[ 7] << 24);
                    ((gctUINT32_PTR) trgLine)[2] = srcLine[ 8] | (srcLine[ 9] << 8) | (srcLine[10] << 16) | (srcLine[11] << 24);
                    ((gctUINT32_PTR) trgLine)[3] = srcLine[12] | (srcLine[13] << 8) | (srcLine[14] << 16) | (srcLine[15] << 24);
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* 32 bpp to 32 bpp: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = srcLine[ 0] | (srcLine[ 1] << 8) | (srcLine[ 2] << 16) | (srcLine[ 3] << 24);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 4;

        if ((((gctUINTPTR_T)srcLine & 3) == 0) && ((SourceStride & 3) == 0))
        {
            /* Special optimization for aligned source. */
            for (x = X; x < Right; x += 4)
            {
                gctUINT8_PTR src;

                /* 32 bpp to 32 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[ 0] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 1] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[ 2] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[ 3] = ((gctUINT32_PTR) src)[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 4] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 5] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[ 6] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[ 7] = ((gctUINT32_PTR) src)[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 8] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 9] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[10] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[11] = ((gctUINT32_PTR) src)[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[12] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[13] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[14] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[15] = ((gctUINT32_PTR) src)[3];

                /* Move to next tile. */
                trgLine += 16 * 4;
                srcLine +=  4 * 4;
            }
        }
        else
        {
            for (x = X; x < Right; x += 4)
            {
                gctUINT8_PTR src;

                /* 32 bpp to 32 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[ 0] = src[ 0] | (src[ 1] << 8) | (src[ 2] << 16) | (src[ 3] << 24);
                ((gctUINT32_PTR) trgLine)[ 1] = src[ 4] | (src[ 5] << 8) | (src[ 6] << 16) | (src[ 7] << 24);
                ((gctUINT32_PTR) trgLine)[ 2] = src[ 8] | (src[ 9] << 8) | (src[10] << 16) | (src[11] << 24);
                ((gctUINT32_PTR) trgLine)[ 3] = src[12] | (src[13] << 8) | (src[14] << 16) | (src[15] << 24);

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 4] = src[ 0] | (src[ 1] << 8) | (src[ 2] << 16) | (src[ 3] << 24);
                ((gctUINT32_PTR) trgLine)[ 5] = src[ 4] | (src[ 5] << 8) | (src[ 6] << 16) | (src[ 7] << 24);
                ((gctUINT32_PTR) trgLine)[ 6] = src[ 8] | (src[ 9] << 8) | (src[10] << 16) | (src[11] << 24);
                ((gctUINT32_PTR) trgLine)[ 7] = src[12] | (src[13] << 8) | (src[14] << 16) | (src[15] << 24);

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 8] = src[ 0] | (src[ 1] << 8) | (src[ 2] << 16) | (src[ 3] << 24);
                ((gctUINT32_PTR) trgLine)[ 9] = src[ 4] | (src[ 5] << 8) | (src[ 6] << 16) | (src[ 7] << 24);
                ((gctUINT32_PTR) trgLine)[10] = src[ 8] | (src[ 9] << 8) | (src[10] << 16) | (src[11] << 24);
                ((gctUINT32_PTR) trgLine)[11] = src[12] | (src[13] << 8) | (src[14] << 16) | (src[15] << 24);

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[12] = src[ 0] | (src[ 1] << 8) | (src[ 2] << 16) | (src[ 3] << 24);
                ((gctUINT32_PTR) trgLine)[13] = src[ 4] | (src[ 5] << 8) | (src[ 6] << 16) | (src[ 7] << 24);
                ((gctUINT32_PTR) trgLine)[14] = src[ 8] | (src[ 9] << 8) | (src[10] << 16) | (src[11] << 24);
                ((gctUINT32_PTR) trgLine)[15] = src[12] | (src[13] << 8) | (src[14] << 16) | (src[15] << 24);

                /* Move to next tile. */
                trgLine += 16 * 4;
                srcLine +=  4 * 4;
            }
        }
    }
}

#if gcdENDIAN_BIG
static void
_Upload32bppto32bppBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 4);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

            /* 32bpp to 32bpp. */
            ((gctUINT32_PTR) trgLine)[0] = (srcLine[0] << 24) | (srcLine[1] << 16) | (srcLine[2] << 8) | srcLine[3];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* 32 bpp to 32 bpp: one tile row. */
                if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned source. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) srcLine)[0];
                    ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) srcLine)[1];
                    ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) srcLine)[2];
                    ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) srcLine)[3];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = (srcLine[ 0] << 24) | (srcLine[ 1] << 16) | (srcLine[ 2] << 8) | srcLine[ 3];
                    ((gctUINT32_PTR) trgLine)[1] = (srcLine[ 4] << 24) | (srcLine[ 5] << 16) | (srcLine[ 6] << 8) | srcLine[ 7];
                    ((gctUINT32_PTR) trgLine)[2] = (srcLine[ 8] << 24) | (srcLine[ 9] << 16) | (srcLine[10] << 8) | srcLine[11];
                    ((gctUINT32_PTR) trgLine)[3] = (srcLine[12] << 24) | (srcLine[13] << 16) | (srcLine[14] << 8) | srcLine[15];
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* 32 bpp to 32 bpp: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[ 0] << 24) | (srcLine[ 1] << 16) | (srcLine[ 2] << 8) | srcLine[ 3];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 4;

        if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
        {
            /* Special optimization for aligned source. */
            for (x = X; x < Right; x += 4)
            {
                gctUINT8_PTR src;

                /* 32 bpp to 32 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[ 0] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 1] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[ 2] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[ 3] = ((gctUINT32_PTR) src)[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 4] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 5] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[ 6] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[ 7] = ((gctUINT32_PTR) src)[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 8] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 9] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[10] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[11] = ((gctUINT32_PTR) src)[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[12] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[13] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[14] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[15] = ((gctUINT32_PTR) src)[3];

                /* Move to next tile. */
                trgLine += 16 * 4;
                srcLine +=  4 * 4;
            }
        }
        else
        {
            for (x = X; x < Right; x += 4)
            {
                gctUINT8_PTR src;

                /* 32 bpp to 32 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[ 0] = (src[ 0] << 24) | (src[ 1] << 16) | (src[ 2] << 8) | src[ 3];
                ((gctUINT32_PTR) trgLine)[ 1] = (src[ 4] << 24) | (src[ 5] << 16) | (src[ 6] << 8) | src[ 7];
                ((gctUINT32_PTR) trgLine)[ 2] = (src[ 8] << 24) | (src[ 9] << 16) | (src[10] << 8) | src[11];
                ((gctUINT32_PTR) trgLine)[ 3] = (src[12] << 24) | (src[13] << 16) | (src[14] << 8) | src[15];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 4] = (src[ 0] << 24) | (src[ 1] << 16) | (src[ 2] << 8) | src[ 3];
                ((gctUINT32_PTR) trgLine)[ 5] = (src[ 4] << 24) | (src[ 5] << 16) | (src[ 6] << 8) | src[ 7];
                ((gctUINT32_PTR) trgLine)[ 6] = (src[ 8] << 24) | (src[ 9] << 16) | (src[10] << 8) | src[11];
                ((gctUINT32_PTR) trgLine)[ 7] = (src[12] << 24) | (src[13] << 16) | (src[14] << 8) | src[15];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 8] = (src[ 0] << 24) | (src[ 1] << 16) | (src[ 2] << 8) | src[ 3];
                ((gctUINT32_PTR) trgLine)[ 9] = (src[ 4] << 24) | (src[ 5] << 16) | (src[ 6] << 8) | src[ 7];
                ((gctUINT32_PTR) trgLine)[10] = (src[ 8] << 24) | (src[ 9] << 16) | (src[10] << 8) | src[11];
                ((gctUINT32_PTR) trgLine)[11] = (src[12] << 24) | (src[13] << 16) | (src[14] << 8) | src[15];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[12] = (src[ 0] << 24) | (src[ 1] << 16) | (src[ 2] << 8) | src[ 3];
                ((gctUINT32_PTR) trgLine)[13] = (src[ 4] << 24) | (src[ 5] << 16) | (src[ 6] << 8) | src[ 7];
                ((gctUINT32_PTR) trgLine)[14] = (src[ 8] << 24) | (src[ 9] << 16) | (src[10] << 8) | src[11];
                ((gctUINT32_PTR) trgLine)[15] = (src[12] << 24) | (src[13] << 16) | (src[14] << 8) | src[15];

                /* Move to next tile. */
                trgLine += 16 * 4;
                srcLine +=  4 * 4;
            }
        }
    }
}
#endif

static void
_Upload64bppto64bpp(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 8);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 8;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 8;

            /* 64bpp to 64bpp. */
            ((gctUINT32_PTR) trgLine)[0] = srcLine[0] | (srcLine[1] << 8) | (srcLine[2] << 16) | (srcLine[3] << 24);
            ((gctUINT32_PTR) trgLine)[1] = srcLine[4] | (srcLine[5] << 8) | (srcLine[6] << 16) | (srcLine[7] << 24);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 8;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 8;

                /* 64 bpp to 64 bpp: one tile row. */
                if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned source. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) srcLine)[0];
                    ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) srcLine)[1];
                    ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) srcLine)[2];
                    ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) srcLine)[3];
                    ((gctUINT32_PTR) trgLine)[4] = ((gctUINT32_PTR) srcLine)[4];
                    ((gctUINT32_PTR) trgLine)[5] = ((gctUINT32_PTR) srcLine)[5];
                    ((gctUINT32_PTR) trgLine)[6] = ((gctUINT32_PTR) srcLine)[6];
                    ((gctUINT32_PTR) trgLine)[7] = ((gctUINT32_PTR) srcLine)[7];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = srcLine[ 0] | (srcLine[ 1] << 8) | (srcLine[ 2] << 16) | (srcLine[ 3] << 24);
                    ((gctUINT32_PTR) trgLine)[1] = srcLine[ 4] | (srcLine[ 5] << 8) | (srcLine[ 6] << 16) | (srcLine[ 7] << 24);
                    ((gctUINT32_PTR) trgLine)[2] = srcLine[ 8] | (srcLine[ 9] << 8) | (srcLine[10] << 16) | (srcLine[11] << 24);
                    ((gctUINT32_PTR) trgLine)[3] = srcLine[12] | (srcLine[13] << 8) | (srcLine[14] << 16) | (srcLine[15] << 24);
                    ((gctUINT32_PTR) trgLine)[4] = srcLine[16] | (srcLine[17] << 8) | (srcLine[18] << 16) | (srcLine[19] << 24);
                    ((gctUINT32_PTR) trgLine)[5] = srcLine[20] | (srcLine[21] << 8) | (srcLine[22] << 16) | (srcLine[23] << 24);
                    ((gctUINT32_PTR) trgLine)[6] = srcLine[24] | (srcLine[25] << 8) | (srcLine[26] << 16) | (srcLine[27] << 24);
                    ((gctUINT32_PTR) trgLine)[7] = srcLine[28] | (srcLine[29] << 8) | (srcLine[30] << 16) | (srcLine[31] << 24);
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 8;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 8;

                /* 64 bpp to 64 bpp: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = srcLine[ 0] | (srcLine[ 1] << 8) | (srcLine[ 2] << 16) | (srcLine[ 3] << 24);
                ((gctUINT32_PTR) trgLine)[1] = srcLine[ 4] | (srcLine[ 5] << 8) | (srcLine[ 6] << 16) | (srcLine[ 7] << 24);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 8;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 8;

        if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
        {
            /* Special optimization for aligned source. */
            for (x = X; x < Right; x += 4)
            {
                gctUINT8_PTR src;

                /* 64 bpp to 64 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[ 0] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 1] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[ 2] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[ 3] = ((gctUINT32_PTR) src)[3];
                ((gctUINT32_PTR) trgLine)[ 4] = ((gctUINT32_PTR) src)[4];
                ((gctUINT32_PTR) trgLine)[ 5] = ((gctUINT32_PTR) src)[5];
                ((gctUINT32_PTR) trgLine)[ 6] = ((gctUINT32_PTR) src)[6];
                ((gctUINT32_PTR) trgLine)[ 7] = ((gctUINT32_PTR) src)[7];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 8] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 9] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[10] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[11] = ((gctUINT32_PTR) src)[3];
                ((gctUINT32_PTR) trgLine)[12] = ((gctUINT32_PTR) src)[4];
                ((gctUINT32_PTR) trgLine)[13] = ((gctUINT32_PTR) src)[5];
                ((gctUINT32_PTR) trgLine)[14] = ((gctUINT32_PTR) src)[6];
                ((gctUINT32_PTR) trgLine)[15] = ((gctUINT32_PTR) src)[7];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[16] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[17] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[18] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[19] = ((gctUINT32_PTR) src)[3];
                ((gctUINT32_PTR) trgLine)[20] = ((gctUINT32_PTR) src)[4];
                ((gctUINT32_PTR) trgLine)[21] = ((gctUINT32_PTR) src)[5];
                ((gctUINT32_PTR) trgLine)[22] = ((gctUINT32_PTR) src)[6];
                ((gctUINT32_PTR) trgLine)[23] = ((gctUINT32_PTR) src)[7];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[24] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[25] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[26] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[27] = ((gctUINT32_PTR) src)[3];
                ((gctUINT32_PTR) trgLine)[28] = ((gctUINT32_PTR) src)[4];
                ((gctUINT32_PTR) trgLine)[29] = ((gctUINT32_PTR) src)[5];
                ((gctUINT32_PTR) trgLine)[30] = ((gctUINT32_PTR) src)[6];
                ((gctUINT32_PTR) trgLine)[31] = ((gctUINT32_PTR) src)[7];

                /* Move to next tile. */
                trgLine += 16 * 8;
                srcLine +=  4 * 8;
            }
        }
        else
        {
            for (x = X; x < Right; x += 4)
            {
                gctUINT8_PTR src;

                /* 64 bpp to 64 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[ 0] = src[ 0] | (src[ 1] << 8) | (src[ 2] << 16) | (src[ 3] << 24);
                ((gctUINT32_PTR) trgLine)[ 1] = src[ 4] | (src[ 5] << 8) | (src[ 6] << 16) | (src[ 7] << 24);
                ((gctUINT32_PTR) trgLine)[ 2] = src[ 8] | (src[ 9] << 8) | (src[10] << 16) | (src[11] << 24);
                ((gctUINT32_PTR) trgLine)[ 3] = src[12] | (src[13] << 8) | (src[14] << 16) | (src[15] << 24);
                ((gctUINT32_PTR) trgLine)[ 4] = src[16] | (src[17] << 8) | (src[18] << 16) | (src[19] << 24);
                ((gctUINT32_PTR) trgLine)[ 5] = src[20] | (src[21] << 8) | (src[22] << 16) | (src[23] << 24);
                ((gctUINT32_PTR) trgLine)[ 6] = src[24] | (src[25] << 8) | (src[26] << 16) | (src[27] << 24);
                ((gctUINT32_PTR) trgLine)[ 7] = src[28] | (src[29] << 8) | (src[30] << 16) | (src[31] << 24);

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 8] = src[ 0] | (src[ 1] << 8) | (src[ 2] << 16) | (src[ 3] << 24);
                ((gctUINT32_PTR) trgLine)[ 9] = src[ 4] | (src[ 5] << 8) | (src[ 6] << 16) | (src[ 7] << 24);
                ((gctUINT32_PTR) trgLine)[10] = src[ 8] | (src[ 9] << 8) | (src[10] << 16) | (src[11] << 24);
                ((gctUINT32_PTR) trgLine)[11] = src[12] | (src[13] << 8) | (src[14] << 16) | (src[15] << 24);
                ((gctUINT32_PTR) trgLine)[12] = src[16] | (src[17] << 8) | (src[18] << 16) | (src[19] << 24);
                ((gctUINT32_PTR) trgLine)[13] = src[20] | (src[21] << 8) | (src[22] << 16) | (src[23] << 24);
                ((gctUINT32_PTR) trgLine)[14] = src[24] | (src[25] << 8) | (src[26] << 16) | (src[27] << 24);
                ((gctUINT32_PTR) trgLine)[15] = src[28] | (src[29] << 8) | (src[30] << 16) | (src[31] << 24);

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[16] = src[ 0] | (src[ 1] << 8) | (src[ 2] << 16) | (src[ 3] << 24);
                ((gctUINT32_PTR) trgLine)[17] = src[ 4] | (src[ 5] << 8) | (src[ 6] << 16) | (src[ 7] << 24);
                ((gctUINT32_PTR) trgLine)[18] = src[ 8] | (src[ 9] << 8) | (src[10] << 16) | (src[11] << 24);
                ((gctUINT32_PTR) trgLine)[19] = src[12] | (src[13] << 8) | (src[14] << 16) | (src[15] << 24);
                ((gctUINT32_PTR) trgLine)[20] = src[16] | (src[17] << 8) | (src[18] << 16) | (src[19] << 24);
                ((gctUINT32_PTR) trgLine)[21] = src[20] | (src[21] << 8) | (src[22] << 16) | (src[23] << 24);
                ((gctUINT32_PTR) trgLine)[22] = src[24] | (src[25] << 8) | (src[26] << 16) | (src[27] << 24);
                ((gctUINT32_PTR) trgLine)[23] = src[28] | (src[29] << 8) | (src[30] << 16) | (src[31] << 24);

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[24] = src[ 0] | (src[ 1] << 8) | (src[ 2] << 16) | (src[ 3] << 24);
                ((gctUINT32_PTR) trgLine)[25] = src[ 4] | (src[ 5] << 8) | (src[ 6] << 16) | (src[ 7] << 24);
                ((gctUINT32_PTR) trgLine)[26] = src[ 8] | (src[ 9] << 8) | (src[10] << 16) | (src[11] << 24);
                ((gctUINT32_PTR) trgLine)[27] = src[12] | (src[13] << 8) | (src[14] << 16) | (src[15] << 24);
                ((gctUINT32_PTR) trgLine)[28] = src[16] | (src[17] << 8) | (src[18] << 16) | (src[19] << 24);
                ((gctUINT32_PTR) trgLine)[29] = src[20] | (src[21] << 8) | (src[22] << 16) | (src[23] << 24);
                ((gctUINT32_PTR) trgLine)[30] = src[24] | (src[25] << 8) | (src[26] << 16) | (src[27] << 24);
                ((gctUINT32_PTR) trgLine)[31] = src[28] | (src[29] << 8) | (src[30] << 16) | (src[31] << 24);

                /* Move to next tile. */
                trgLine += 16 * 8;
                srcLine +=  4 * 8;
            }
        }
    }
}

#if gcdENDIAN_BIG
static void
_Upload64bppto64bppBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 8);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 8;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 8;

            /* 64bpp to 64bpp. */
            ((gctUINT32_PTR) trgLine)[0] = (srcLine[0] << 24) | (srcLine[1] << 16) | (srcLine[2] << 8) | srcLine[3];
            ((gctUINT32_PTR) trgLine)[1] = (srcLine[4] << 24) | (srcLine[5] << 16) | (srcLine[6] << 8) | srcLine[7];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 8;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 8;

                /* 64 bpp to 64 bpp: one tile row. */
                if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned source. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) srcLine)[0];
                    ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) srcLine)[1];
                    ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) srcLine)[2];
                    ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) srcLine)[3];
                    ((gctUINT32_PTR) trgLine)[4] = ((gctUINT32_PTR) srcLine)[4];
                    ((gctUINT32_PTR) trgLine)[5] = ((gctUINT32_PTR) srcLine)[5];
                    ((gctUINT32_PTR) trgLine)[6] = ((gctUINT32_PTR) srcLine)[6];
                    ((gctUINT32_PTR) trgLine)[7] = ((gctUINT32_PTR) srcLine)[7];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = (srcLine[ 0] << 24) | (srcLine[ 1] << 16) | (srcLine[ 2] << 8) | srcLine[ 3];
                    ((gctUINT32_PTR) trgLine)[1] = (srcLine[ 4] << 24) | (srcLine[ 5] << 16) | (srcLine[ 6] << 8) | srcLine[ 7];
                    ((gctUINT32_PTR) trgLine)[2] = (srcLine[ 8] << 24) | (srcLine[ 9] << 16) | (srcLine[10] << 8) | srcLine[11];
                    ((gctUINT32_PTR) trgLine)[3] = (srcLine[12] << 24) | (srcLine[13] << 16) | (srcLine[14] << 8) | srcLine[15];
                    ((gctUINT32_PTR) trgLine)[4] = (srcLine[16] << 24) | (srcLine[17] << 16) | (srcLine[18] << 8) | srcLine[19];
                    ((gctUINT32_PTR) trgLine)[5] = (srcLine[20] << 24) | (srcLine[21] << 16) | (srcLine[22] << 8) | srcLine[23];
                    ((gctUINT32_PTR) trgLine)[6] = (srcLine[24] << 24) | (srcLine[25] << 16) | (srcLine[26] << 8) | srcLine[27];
                    ((gctUINT32_PTR) trgLine)[7] = (srcLine[28] << 24) | (srcLine[29] << 16) | (srcLine[30] << 8) | srcLine[31];
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 8;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 8;

                /* 64 bpp to 64 bpp: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[ 0] << 24) | (srcLine[ 1] << 16) | (srcLine[ 2] << 8) | srcLine[ 3];
                ((gctUINT32_PTR) trgLine)[1] = (srcLine[ 4] << 24) | (srcLine[ 5] << 16) | (srcLine[ 6] << 8) | srcLine[ 7];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 8;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 8;

        if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
        {
            /* Special optimization for aligned source. */
            for (x = X; x < Right; x += 4)
            {
                gctUINT8_PTR src;

                /* 64 bpp to 64 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[ 0] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 1] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[ 2] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[ 3] = ((gctUINT32_PTR) src)[3];
                ((gctUINT32_PTR) trgLine)[ 4] = ((gctUINT32_PTR) src)[4];
                ((gctUINT32_PTR) trgLine)[ 5] = ((gctUINT32_PTR) src)[5];
                ((gctUINT32_PTR) trgLine)[ 6] = ((gctUINT32_PTR) src)[6];
                ((gctUINT32_PTR) trgLine)[ 7] = ((gctUINT32_PTR) src)[7];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 8] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 9] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[10] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[11] = ((gctUINT32_PTR) src)[3];
                ((gctUINT32_PTR) trgLine)[12] = ((gctUINT32_PTR) src)[4];
                ((gctUINT32_PTR) trgLine)[13] = ((gctUINT32_PTR) src)[5];
                ((gctUINT32_PTR) trgLine)[14] = ((gctUINT32_PTR) src)[6];
                ((gctUINT32_PTR) trgLine)[15] = ((gctUINT32_PTR) src)[7];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[16] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[17] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[18] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[19] = ((gctUINT32_PTR) src)[3];
                ((gctUINT32_PTR) trgLine)[20] = ((gctUINT32_PTR) src)[4];
                ((gctUINT32_PTR) trgLine)[21] = ((gctUINT32_PTR) src)[5];
                ((gctUINT32_PTR) trgLine)[22] = ((gctUINT32_PTR) src)[6];
                ((gctUINT32_PTR) trgLine)[23] = ((gctUINT32_PTR) src)[7];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[24] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[25] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[26] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[27] = ((gctUINT32_PTR) src)[3];
                ((gctUINT32_PTR) trgLine)[28] = ((gctUINT32_PTR) src)[4];
                ((gctUINT32_PTR) trgLine)[29] = ((gctUINT32_PTR) src)[5];
                ((gctUINT32_PTR) trgLine)[30] = ((gctUINT32_PTR) src)[6];
                ((gctUINT32_PTR) trgLine)[31] = ((gctUINT32_PTR) src)[7];

                /* Move to next tile. */
                trgLine += 16 * 8;
                srcLine +=  4 * 8;
            }
        }
        else
        {
            for (x = X; x < Right; x += 4)
            {
                gctUINT8_PTR src;

                /* 64 bpp to 64 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[0] = (src[ 0] << 24) | (src[ 1] << 16) | (src[ 2] << 8) | src[ 3];
                ((gctUINT32_PTR) trgLine)[1] = (src[ 4] << 24) | (src[ 5] << 16) | (src[ 6] << 8) | src[ 7];
                ((gctUINT32_PTR) trgLine)[2] = (src[ 8] << 24) | (src[ 9] << 16) | (src[10] << 8) | src[11];
                ((gctUINT32_PTR) trgLine)[3] = (src[12] << 24) | (src[13] << 16) | (src[14] << 8) | src[15];
                ((gctUINT32_PTR) trgLine)[4] = (src[16] << 24) | (src[17] << 16) | (src[18] << 8) | src[19];
                ((gctUINT32_PTR) trgLine)[5] = (src[20] << 24) | (src[21] << 16) | (src[22] << 8) | src[23];
                ((gctUINT32_PTR) trgLine)[6] = (src[24] << 24) | (src[25] << 16) | (src[26] << 8) | src[27];
                ((gctUINT32_PTR) trgLine)[7] = (src[28] << 24) | (src[29] << 16) | (src[30] << 8) | src[31];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 8] = (src[ 0] << 24) | (src[ 1] << 16) | (src[ 2] << 8) | src[ 3];
                ((gctUINT32_PTR) trgLine)[ 9] = (src[ 4] << 24) | (src[ 5] << 16) | (src[ 6] << 8) | src[ 7];
                ((gctUINT32_PTR) trgLine)[10] = (src[ 8] << 24) | (src[ 9] << 16) | (src[10] << 8) | src[11];
                ((gctUINT32_PTR) trgLine)[11] = (src[12] << 24) | (src[13] << 16) | (src[14] << 8) | src[15];
                ((gctUINT32_PTR) trgLine)[12] = (src[16] << 24) | (src[17] << 16) | (src[18] << 8) | src[19];
                ((gctUINT32_PTR) trgLine)[13] = (src[20] << 24) | (src[21] << 16) | (src[22] << 8) | src[23];
                ((gctUINT32_PTR) trgLine)[14] = (src[24] << 24) | (src[25] << 16) | (src[26] << 8) | src[27];
                ((gctUINT32_PTR) trgLine)[15] = (src[28] << 24) | (src[29] << 16) | (src[30] << 8) | src[31];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[16] = (src[ 0] << 24) | (src[ 1] << 16) | (src[ 2] << 8) | src[ 3];
                ((gctUINT32_PTR) trgLine)[17] = (src[ 4] << 24) | (src[ 5] << 16) | (src[ 6] << 8) | src[ 7];
                ((gctUINT32_PTR) trgLine)[18] = (src[ 8] << 24) | (src[ 9] << 16) | (src[10] << 8) | src[11];
                ((gctUINT32_PTR) trgLine)[19] = (src[12] << 24) | (src[13] << 16) | (src[14] << 8) | src[15];
                ((gctUINT32_PTR) trgLine)[20] = (src[16] << 24) | (src[17] << 16) | (src[18] << 8) | src[19];
                ((gctUINT32_PTR) trgLine)[21] = (src[20] << 24) | (src[21] << 16) | (src[22] << 8) | src[23];
                ((gctUINT32_PTR) trgLine)[22] = (src[24] << 24) | (src[25] << 16) | (src[26] << 8) | src[27];
                ((gctUINT32_PTR) trgLine)[23] = (src[28] << 24) | (src[29] << 16) | (src[30] << 8) | src[31];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[24] = (src[ 0] << 24) | (src[ 1] << 16) | (src[ 2] << 8) | src[ 3];
                ((gctUINT32_PTR) trgLine)[25] = (src[ 4] << 24) | (src[ 5] << 16) | (src[ 6] << 8) | src[ 7];
                ((gctUINT32_PTR) trgLine)[26] = (src[ 8] << 24) | (src[ 9] << 16) | (src[10] << 8) | src[11];
                ((gctUINT32_PTR) trgLine)[27] = (src[12] << 24) | (src[13] << 16) | (src[14] << 8) | src[15];
                ((gctUINT32_PTR) trgLine)[28] = (src[16] << 24) | (src[17] << 16) | (src[18] << 8) | src[19];
                ((gctUINT32_PTR) trgLine)[29] = (src[20] << 24) | (src[21] << 16) | (src[22] << 8) | src[23];
                ((gctUINT32_PTR) trgLine)[30] = (src[24] << 24) | (src[25] << 16) | (src[26] << 8) | src[27];
                ((gctUINT32_PTR) trgLine)[31] = (src[28] << 24) | (src[29] << 16) | (src[30] << 8) | src[31];

                /* Move to next tile. */
                trgLine += 16 * 8;
                srcLine +=  4 * 8;
            }
        }
    }
}
#endif

static void
_UploadRGB565toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory + y * SourceStride + x * 2);

            /* RGB565 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = RGB565toARGB(u);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory + y * SourceStride + x * 2);

                /* RGB565 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGB565toARGB(u);
                u = srcLine[1];
                ((gctUINT32_PTR) trgLine)[1] = RGB565toARGB(u);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[2] = RGB565toARGB(u);
                u = srcLine[3];
                ((gctUINT32_PTR) trgLine)[3] = RGB565toARGB(u);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory + y * SourceStride + x * 2);

                /* RGB565 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGB565toARGB(u);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory + y * SourceStride + X * 2);

        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;
            register gctUINT32 u;

            /* RGB565 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = RGB565toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 1] = RGB565toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 2] = RGB565toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 3] = RGB565toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = RGB565toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 5] = RGB565toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 6] = RGB565toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 7] = RGB565toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = RGB565toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 9] = RGB565toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[10] = RGB565toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[11] = RGB565toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = RGB565toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[13] = RGB565toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[14] = RGB565toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[15] = RGB565toARGB(u);

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 1; /* srcLine is in 16 bits. */
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadRGB565toARGBBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2);

            /* RGB565 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = RGB565toARGBBE(u);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2);

                /* RGB565 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGB565toARGBBE(u);
                u = srcLine[1];
                ((gctUINT32_PTR) trgLine)[1] = RGB565toARGBBE(u);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[2] = RGB565toARGBBE(u);
                u = srcLine[3];
                ((gctUINT32_PTR) trgLine)[3] = RGB565toARGBBE(u);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2);

                /* RGB565 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGB565toARGBBE(u);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory + y * SourceStride + X * 2);

        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;
            register gctUINT32 u;

            /* RGB565 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = RGB565toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 1] = RGB565toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 2] = RGB565toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 3] = RGB565toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = RGB565toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 5] = RGB565toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 6] = RGB565toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 7] = RGB565toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = RGB565toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 9] = RGB565toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[10] = RGB565toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[11] = RGB565toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = RGB565toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[13] = RGB565toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[14] = RGB565toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[15] = RGB565toARGBBE(u);

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 1; /* srcLine is in 16 bits. */
        }
    }
}
#endif

static void
_UploadRGBA4444toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA4444 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = RGBA4444toARGB(u);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGBA4444toARGB(u);
                u = srcLine[1];
                ((gctUINT32_PTR) trgLine)[1] = RGBA4444toARGB(u);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[2] = RGBA4444toARGB(u);
                u = srcLine[3];
                ((gctUINT32_PTR) trgLine)[3] = RGBA4444toARGB(u);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGBA4444toARGB(u);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + X * 2);

        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;
            register gctUINT32 u;

            /* RGBA4444 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = RGBA4444toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 1] = RGBA4444toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 2] = RGBA4444toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 3] = RGBA4444toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = RGBA4444toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 5] = RGBA4444toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 6] = RGBA4444toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 7] = RGBA4444toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = RGBA4444toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 9] = RGBA4444toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[10] = RGBA4444toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[11] = RGBA4444toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = RGBA4444toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[13] = RGBA4444toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[14] = RGBA4444toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[15] = RGBA4444toARGB(u);

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 1; /* srcLine is in 16 bits. */
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadRGBA4444toARGBBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA4444 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = RGBA4444toARGBBE(u);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;

                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGBA4444toARGBBE(u);
                u = srcLine[1];
                ((gctUINT32_PTR) trgLine)[1] = RGBA4444toARGBBE(u);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[2] = RGBA4444toARGBBE(u);
                u = srcLine[3];
                ((gctUINT32_PTR) trgLine)[3] = RGBA4444toARGBBE(u);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;

                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGBA4444toARGBBE(u);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + X * 2);

        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;
            register gctUINT32 u;

            /* RGBA4444 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = RGBA4444toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 1] = RGBA4444toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 2] = RGBA4444toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 3] = RGBA4444toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = RGBA4444toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 5] = RGBA4444toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 6] = RGBA4444toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 7] = RGBA4444toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = RGBA4444toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 9] = RGBA4444toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[10] = RGBA4444toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[11] = RGBA4444toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = RGBA4444toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[13] = RGBA4444toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[14] = RGBA4444toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[15] = RGBA4444toARGBBE(u);

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 1; /* srcLine is in 16 bits. */
        }
    }
}
#endif

static void
_UploadRGBA5551toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA5551 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = RGBA5551toARGB(u);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGBA5551toARGB(u);
                u = srcLine[1];
                ((gctUINT32_PTR) trgLine)[1] = RGBA5551toARGB(u);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[2] = RGBA5551toARGB(u);
                u = srcLine[3];
                ((gctUINT32_PTR) trgLine)[3] = RGBA5551toARGB(u);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGBA5551toARGB(u);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + X * 2);

        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;
            register gctUINT32 u;

            /* RGBA5551 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = RGBA5551toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 1] = RGBA5551toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 2] = RGBA5551toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 3] = RGBA5551toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = RGBA5551toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 5] = RGBA5551toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 6] = RGBA5551toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 7] = RGBA5551toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = RGBA5551toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 9] = RGBA5551toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[10] = RGBA5551toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[11] = RGBA5551toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = RGBA5551toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[13] = RGBA5551toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[14] = RGBA5551toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[15] = RGBA5551toARGB(u);

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 1; /* srcLine is in 16 bits. */
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadRGBA5551toARGBBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA5551 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = RGBA5551toARGBBE(u);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGBA5551toARGBBE(u);
                u = srcLine[1];
                ((gctUINT32_PTR) trgLine)[1] = RGBA5551toARGBBE(u);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[2] = RGBA5551toARGBBE(u);
                u = srcLine[3];
                ((gctUINT32_PTR) trgLine)[3] = RGBA5551toARGBBE(u);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGBA5551toARGBBE(u);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + X * 2);

        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;
            register gctUINT32 u;

            /* RGBA5551 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = RGBA5551toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 1] = RGBA5551toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 2] = RGBA5551toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 3] = RGBA5551toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = RGBA5551toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 5] = RGBA5551toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 6] = RGBA5551toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 7] = RGBA5551toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = RGBA5551toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 9] = RGBA5551toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[10] = RGBA5551toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[11] = RGBA5551toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = RGBA5551toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[13] = RGBA5551toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[14] = RGBA5551toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[15] = RGBA5551toARGBBE(u);

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 1; /* srcLine is in 16 bits. */
        }
    }
}
#endif

static void
_Upload8bppto8bpp(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 1);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* 8 bpp to 8 bpp. */
            trgLine[0] = srcLine[0];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* 8 bpp to 8 bpp: one tile row. */
                if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned source. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR)srcLine)[0];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = srcLine[0] | (srcLine[1] << 8) | (srcLine[2] << 16) | (srcLine[3] << 24);
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* 8 bpp to 8 bpp: part tile row. */
                trgLine[0] = srcLine[0];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 1;

        if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
        {
            /* Special optimization for aligned source. */
            for (x = X; x < Right; x += 4)
            {
                ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) (srcLine + SourceStride * 0))[0];
                ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) (srcLine + SourceStride * 1))[0];
                ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) (srcLine + SourceStride * 2))[0];
                ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) (srcLine + SourceStride * 3))[0];

                /* Move to next tile. */
                trgLine += 16 * 1;
                srcLine +=  4 * 1;
            }
        }
        else
        {
            for (x = X; x < Right; x += 4)
            {
                gctUINT8_PTR src;

                /* 8 bpp to 8 bpp: one tile. */
                src  = srcLine;
                ((gctUINT32_PTR) trgLine)[0] = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[1] = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[2] = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[3] = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);

                /* Move to next tile. */
                trgLine += 16 * 1;
                srcLine +=  4 * 1;
            }
        }
    }
}

#if gcdENDIAN_BIG
static void
_Upload8bppto8bppBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 1);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* 8 bpp to 8 bpp. */
            trgLine[0] = srcLine[0];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* 8 bpp to 8 bpp: one tile row. */
                if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned source. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR)srcLine)[0];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = (srcLine[0] << 24) | (srcLine[1] << 16) | (srcLine[2] << 8) | srcLine[3];
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* 8 bpp to 8 bpp: part tile row. */
                trgLine[0] = srcLine[0];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 1;

        if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
        {
            /* Special optimization for aligned source. */
            for (x = X; x < Right; x += 4)
            {
                ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) (srcLine + SourceStride * 0))[0];
                ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) (srcLine + SourceStride * 1))[0];
                ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) (srcLine + SourceStride * 2))[0];
                ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) (srcLine + SourceStride * 3))[0];

                /* Move to next tile. */
                trgLine += 16 * 1;
                srcLine +=  4 * 1;
            }
        }
        else
        {
            for (x = X; x < Right; x += 4)
            {
                gctUINT8_PTR src;

                /* 8 bpp to 8 bpp: one tile. */
                src  = srcLine;
                ((gctUINT32_PTR) trgLine)[0] = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[1] = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[2] = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[3] = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];

                /* Move to next tile. */
                trgLine += 16 * 1;
                srcLine +=  4 * 1;
            }
        }
    }
}
#endif

static void
_Upload16bppto16bpp(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* 16 bpp to 16 bpp. */
            ((gctUINT16_PTR) trgLine)[0] = srcLine[0];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* 16 bpp to 16 bpp: one tile row. */
                if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned case. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) srcLine)[0];
                    ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) srcLine)[1];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = srcLine[0] | (srcLine[1] << 16);
                    ((gctUINT32_PTR) trgLine)[1] = srcLine[2] | (srcLine[3] << 16);
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* 16 bpp to 16 bpp: part tile row. */
                ((gctUINT16_PTR) trgLine)[0] = srcLine[0];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
        srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + X * 2);

        if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
        {
            /* Special optimization for aligned case. */
            for (x = X; x < Right; x += 4)
            {
                gctUINT16_PTR src;

                /* 16 bpp to 16 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) src)[1];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) src)[1];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[4] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[5] = ((gctUINT32_PTR) src)[1];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[6] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[7] = ((gctUINT32_PTR) src)[1];

                /* Move to next tile. */
                trgLine += 16 * 2;
                srcLine +=  4 * 1; /* srcLine is in 16 bits. */
            }
        }
        else
        {
            for (x = X; x < Right; x += 4)
            {
                gctUINT16_PTR src;

                /* 16 bpp to 16 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[0] = src[0] | (src[1] << 16);
                ((gctUINT32_PTR) trgLine)[1] = src[2] | (src[3] << 16);

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[2] = src[0] | (src[1] << 16);
                ((gctUINT32_PTR) trgLine)[3] = src[2] | (src[3] << 16);

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[4] = src[0] | (src[1] << 16);
                ((gctUINT32_PTR) trgLine)[5] = src[2] | (src[3] << 16);

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[6] = src[0] | (src[1] << 16);
                ((gctUINT32_PTR) trgLine)[7] = src[2] | (src[3] << 16);

                /* Move to next tile. */
                trgLine += 16 * 2;
                srcLine +=  4 * 1; /* srcLine is in 16 bits. */
            }
        }
    }
}

#if gcdENDIAN_BIG
static void
_Upload16bppto16bppBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

            /* 16 bpp to 16 bpp. */
            ((gctUINT16_PTR) trgLine)[0] = ((gctUINT16_PTR) srcLine)[0];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

                /* 16 bpp to 16 bpp: one tile row. */
                if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned case. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) srcLine)[0];
                    ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) srcLine)[1];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = (srcLine[0] << 24) | (srcLine[1] << 16) | (srcLine[2] << 8) | srcLine[3];
                    ((gctUINT32_PTR) trgLine)[1] = (srcLine[4] << 24) | (srcLine[5] << 16) | (srcLine[6] << 8) | srcLine[7];
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

                /* 16 bpp to 16 bpp: part tile row. */
                ((gctUINT16_PTR) trgLine)[0] = ((gctUINT16_PTR) srcLine)[0];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 2;

        if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
        {
            /* Special optimization for aligned case. */
            for (x = X; x < Right; x += 4)
            {
                gctUINT16_PTR src;

                /* 16 bpp to 16 bpp: one tile. */
                src = (gctUINT16_PTR) srcLine;
                ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) src)[1];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) src)[1];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[4] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[5] = ((gctUINT32_PTR) src)[1];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[6] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[7] = ((gctUINT32_PTR) src)[1];

                /* Move to next tile. */
                trgLine += 16 * 2;
                srcLine +=  4 * 2;
            }
        }
        else
        {
            for (x = X; x < Right; x += 4)
            {
                gctUINT8_PTR src;

                /* 16 bpp to 16 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[0] = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
                ((gctUINT32_PTR) trgLine)[1] = (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[2] = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
                ((gctUINT32_PTR) trgLine)[3] = (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[4] = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
                ((gctUINT32_PTR) trgLine)[5] = (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[6] = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
                ((gctUINT32_PTR) trgLine)[7] = (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];

                /* Move to next tile. */
                trgLine += 16 * 2;
                srcLine +=  4 * 2;
            }
        }
    }
}
#endif

static void
_UploadRGBA4444toARGB4444(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA4444 to ARGB4444. */
            ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 4) | ((srcLine[0] & 0xF) << 12);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB4444: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[0] >> 4) | ((srcLine[0] & 0xF) << 12) | ((srcLine[1] & 0xFFF0) << 12) | ((srcLine[1] & 0xF) << 28) ;
                ((gctUINT32_PTR) trgLine)[1] = (srcLine[2] >> 4) | ((srcLine[2] & 0xF) << 12) | ((srcLine[3] & 0xFFF0) << 12) | ((srcLine[3] & 0xF) << 28) ;
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB4444: part tile row. */
                ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 4) | ((srcLine[0] & 0xF) << 12);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
        srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + X * 2);

        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;

            /* RGBA4444 to ARGB4444: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[0] = (src[0] >> 4) | ((src[0] & 0xF) << 12) | ((src[1] & 0xFFF0) << 12) | ((src[1] & 0xF) << 28) ;
            ((gctUINT32_PTR) trgLine)[1] = (src[2] >> 4) | ((src[2] & 0xF) << 12) | ((src[3] & 0xFFF0) << 12) | ((src[3] & 0xF) << 28) ;

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[2] = (src[0] >> 4) | ((src[0] & 0xF) << 12) | ((src[1] & 0xFFF0) << 12) | ((src[1] & 0xF) << 28) ;
            ((gctUINT32_PTR) trgLine)[3] = (src[2] >> 4) | ((src[2] & 0xF) << 12) | ((src[3] & 0xFFF0) << 12) | ((src[3] & 0xF) << 28) ;

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[4] = (src[0] >> 4) | ((src[0] & 0xF) << 12) | ((src[1] & 0xFFF0) << 12) | ((src[1] & 0xF) << 28) ;
            ((gctUINT32_PTR) trgLine)[5] = (src[2] >> 4) | ((src[2] & 0xF) << 12) | ((src[3] & 0xFFF0) << 12) | ((src[3] & 0xF) << 28) ;

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[6] = (src[0] >> 4) | ((src[0] & 0xF) << 12) | ((src[1] & 0xFFF0) << 12) | ((src[1] & 0xF) << 28) ;
            ((gctUINT32_PTR) trgLine)[7] = (src[2] >> 4) | ((src[2] & 0xF) << 12) | ((src[3] & 0xFFF0) << 12) | ((src[3] & 0xF) << 28) ;

            /* Move to next tile. */
            trgLine += 16 * 2;
            srcLine +=  4 * 1; /* srcLine is in 16 bits. */
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadRGBA4444toARGB4444BE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA4444 to ARGB4444. */
            ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 4) | ((srcLine[0] & 0xF) << 12);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB4444: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = ((srcLine[0] & 0xFFF0) << 12) | ((srcLine[0] & 0xF) << 28) | (srcLine[1] >> 4) | ((srcLine[1] & 0xF) << 12) ;
                ((gctUINT32_PTR) trgLine)[1] = ((srcLine[2] & 0xFFF0) << 12) | ((srcLine[2] & 0xF) << 28) | (srcLine[3] >> 4) | ((srcLine[3] & 0xF) << 12) ;
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB4444: part tile row. */
                ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 4) | ((srcLine[0] & 0xF) << 12);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
        srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + X * 2);

        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;

            /* RGBA4444 to ARGB4444: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[0] = ((src[0] & 0xFFF0) << 12) | ((src[0] & 0xF) << 28) | (src[1] >> 4) | ((src[1] & 0xF) << 12) ;
            ((gctUINT32_PTR) trgLine)[1] = ((src[2] & 0xFFF0) << 12) | ((src[2] & 0xF) << 28) | (src[3] >> 4) | ((src[3] & 0xF) << 12) ;

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[2] = ((src[0] & 0xFFF0) << 12) | ((src[0] & 0xF) << 28) | (src[1] >> 4) | ((src[1] & 0xF) << 12) ;
            ((gctUINT32_PTR) trgLine)[3] = ((src[2] & 0xFFF0) << 12) | ((src[2] & 0xF) << 28) | (src[3] >> 4) | ((src[3] & 0xF) << 12) ;

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[4] = ((src[0] & 0xFFF0) << 12) | ((src[0] & 0xF) << 28) | (src[1] >> 4) | ((src[1] & 0xF) << 12) ;
            ((gctUINT32_PTR) trgLine)[5] = ((src[2] & 0xFFF0) << 12) | ((src[2] & 0xF) << 28) | (src[3] >> 4) | ((src[3] & 0xF) << 12) ;

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[6] = ((src[0] & 0xFFF0) << 12) | ((src[0] & 0xF) << 28) | (src[1] >> 4) | ((src[1] & 0xF) << 12) ;
            ((gctUINT32_PTR) trgLine)[7] = ((src[2] & 0xFFF0) << 12) | ((src[2] & 0xF) << 28) | (src[3] >> 4) | ((src[3] & 0xF) << 12) ;

            /* Move to next tile. */
            trgLine += 16 * 2;
            srcLine +=  4 * 1; /* srcLine is in 16 bits. */
        }
    }
}
#endif

static void
_UploadRGBA5551toARGB1555(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA5551 to ARGB1555. */
            ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 1) | ((srcLine[0] & 0x1) << 15);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB1555: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[0] >> 1) | ((srcLine[0] & 0x1) << 15) | ((srcLine[1] & 0xFFFE) << 15) | ((srcLine[1] & 0x1) << 31);
                ((gctUINT32_PTR) trgLine)[1] = (srcLine[2] >> 1) | ((srcLine[2] & 0x1) << 15) | ((srcLine[3] & 0xFFFE) << 15) | ((srcLine[3] & 0x1) << 31) ;
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB1555: part tile row. */
                ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 1) | ((srcLine[0] & 0x1) << 15);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
        srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + X * 2);

        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;

            /* RGBA5551 to ARGB1555: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[0] = (src[0] >> 1) | ((src[0] & 0x1) << 15) | ((src[1] & 0xFFFE) << 15) | ((src[1] & 0x1) << 31);
            ((gctUINT32_PTR) trgLine)[1] = (src[2] >> 1) | ((src[2] & 0x1) << 15) | ((src[3] & 0xFFFE) << 15) | ((src[3] & 0x1) << 31);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[2] = (src[0] >> 1) | ((src[0] & 0x1) << 15) | ((src[1] & 0xFFFE) << 15) | ((src[1] & 0x1) << 31);
            ((gctUINT32_PTR) trgLine)[3] = (src[2] >> 1) | ((src[2] & 0x1) << 15) | ((src[3] & 0xFFFE) << 15) | ((src[3] & 0x1) << 31);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[4] = (src[0] >> 1) | ((src[0] & 0x1) << 15) | ((src[1] & 0xFFFE) << 15) | ((src[1] & 0x1) << 31);
            ((gctUINT32_PTR) trgLine)[5] = (src[2] >> 1) | ((src[2] & 0x1) << 15) | ((src[3] & 0xFFFE) << 15) | ((src[3] & 0x1) << 31);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[6] = (src[0] >> 1) | ((src[0] & 0x1) << 15) | ((src[1] & 0xFFFE) << 15) | ((src[1] & 0x1) << 31);
            ((gctUINT32_PTR) trgLine)[7] = (src[2] >> 1) | ((src[2] & 0x1) << 15) | ((src[3] & 0xFFFE) << 15) | ((src[3] & 0x1) << 31);

            /* Move to next tile. */
            trgLine += 16 * 2;
            srcLine +=  4 * 1; /* srcLine is in 16 bits. */
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadRGBA5551toARGB1555BE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA5551 to ARGB1555. */
            ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 1) | ((srcLine[0] & 0x1) << 15);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB1555: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = ((srcLine[0] & 0xFFFE) << 15) | ((srcLine[0] & 0x1) << 31) | (srcLine[1] >> 1) | ((srcLine[1] & 0x1) << 15);
                ((gctUINT32_PTR) trgLine)[1] = ((srcLine[2] & 0xFFFE) << 15) | ((srcLine[2] & 0x1) << 31) | (srcLine[3] >> 1) | ((srcLine[3] & 0x1) << 15);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB1555: part tile row. */
                ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 1) | ((srcLine[0] & 0x1) << 15);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
        srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + X * 2);

        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;

            /* RGBA5551 to ARGB1555: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[0] = ((src[0] & 0xFFFE) << 15) | ((src[0] & 0x1) << 31) | (src[1] >> 1) | ((src[1] & 0x1) << 15);
            ((gctUINT32_PTR) trgLine)[1] = ((src[2] & 0xFFFE) << 15) | ((src[2] & 0x1) << 31) | (src[3] >> 1) | ((src[3] & 0x1) << 15);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[2] = ((src[0] & 0xFFFE) << 15) | ((src[0] & 0x1) << 31) | (src[1] >> 1) | ((src[1] & 0x1) << 15);
            ((gctUINT32_PTR) trgLine)[3] = ((src[2] & 0xFFFE) << 15) | ((src[2] & 0x1) << 31) | (src[3] >> 1) | ((src[3] & 0x1) << 15);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[4] = ((src[0] & 0xFFFE) << 15) | ((src[0] & 0x1) << 31) | (src[1] >> 1) | ((src[1] & 0x1) << 15);
            ((gctUINT32_PTR) trgLine)[5] = ((src[2] & 0xFFFE) << 15) | ((src[2] & 0x1) << 31) | (src[3] >> 1) | ((src[3] & 0x1) << 15);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[6] = ((src[0] & 0xFFFE) << 15) | ((src[0] & 0x1) << 31) | (src[1] >> 1) | ((src[1] & 0x1) << 15);
            ((gctUINT32_PTR) trgLine)[7] = ((src[2] & 0xFFFE) << 15) | ((src[2] & 0x1) << 31) | (src[3] >> 1) | ((src[3] & 0x1) << 15);

            /* Move to next tile. */
            trgLine += 16 * 2;
            srcLine +=  4 * 1; /* srcLine is in 16 bits. */
        }
    }
}
#endif

static gceSTATUS
_UploadTextureTiled(
    IN gcoHARDWARE Hardware,
    IN gcoSURF TexSurf,
    IN gctUINT32 Offset,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride,
    IN gceSURF_FORMAT SourceFormat
    )
{
    gceSTATUS status;
    gctUINT x, y;
    gctUINT edgeX[6];
    gctUINT edgeY[6];
    gctUINT countX = 0;
    gctUINT countY = 0;
    gctUINT32 bitsPerPixel;
    gctUINT32 bytesPerLine;
    gctUINT32 bytesPerTile;
    gctBOOL srcOdd, trgOdd;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
    gctUINT srcBit;
    gctUINT srcBitStep;
    gctINT srcStride;
    gctUINT8* srcPixel;
    gctUINT8* srcLine;
    gcsSURF_FORMAT_INFO_PTR srcFormatInfo;
    gcsSURF_FORMAT_INFO_PTR trgFormatInfo;
    gctUINT trgBit;
    gctUINT trgBitStep;
    gctUINT8* trgPixel;
    gctUINT8* trgLine;
    gctUINT8* trgTile;
    /*gctUINT32 trgAddress;*/
    gctPOINTER trgLogical;
    gctINT trgStride;
    gceSURF_FORMAT trgFormat;
    gctBOOL oclSrcFormat;

    gcmHEADER_ARG("Hardware=0x%x TexSurf=0x%x Offset=%u "
                  "X=%u Y=%u Width=%u Height=%u "
                  "Memory=0x%x SourceStride=%d SourceFormat=%d",
                  Hardware, TexSurf, Offset, X, Y,
                  Width, Height, Memory, SourceStride, SourceFormat);

    gcmGETHARDWARE(Hardware);

    /* Handle the OCL bit. */
    oclSrcFormat = (SourceFormat & gcvSURF_FORMAT_OCL) != 0;
    SourceFormat = (gceSURF_FORMAT) (SourceFormat & ~gcvSURF_FORMAT_OCL);

    trgFormat  = TexSurf->format;
    /*trgAddress = TexSurf->node.physical;*/
    trgLogical = TexSurf->node.logical;
    trgStride  = TexSurf->stride;

    /* Compute dest logical. */
    trgLogical = (gctPOINTER) ((gctUINT8_PTR) trgLogical + Offset);

    /* Compute edge coordinates. */
    if (Width < 4)
    {
        for (x = X; x < right; x++)
        {
           edgeX[countX++] = x;
        }
    }
    else
    {
        for (x = X; x < ((X + 3) & ~3); x++)
        {
            edgeX[countX++] = x;
        }

        for (x = (right & ~3); x < right; x++)
        {
            edgeX[countX++] = x;
        }
    }

    if (Height < 4)
    {
       for (y = Y; y < bottom; y++)
       {
           edgeY[countY++] = y;
       }
    }
    else
    {
        for (y = Y; y < ((Y + 3) & ~3); y++)
        {
           edgeY[countY++] = y;
        }

        for (y = (bottom & ~3); y < bottom; y++)
        {
           edgeY[countY++] = y;
        }
    }

    /* Fast path(s) for all common OpenGL ES formats. */
    if ((trgFormat == gcvSURF_A8R8G8B8)
    ||  (trgFormat == gcvSURF_X8R8G8B8)
    )
    {
        switch (SourceFormat)
        {
        case gcvSURF_A8:
#if gcdENDIAN_BIG
            _UploadA8toARGBBE(trgLogical,
                              trgStride,
                              X, Y,
                              right, bottom,
                              edgeX, edgeY,
                              countX, countY,
                              Memory,
                              (SourceStride == 0)
                              ? (gctINT) Width
                              : SourceStride);
#else
            _UploadA8toARGB(trgLogical,
                            trgStride,
                            X, Y,
                            right, bottom,
                            edgeX, edgeY,
                            countX, countY,
                            Memory,
                            (SourceStride == 0)
                            ? (gctINT) Width
                            : SourceStride);
#endif

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case gcvSURF_B8G8R8:
#if gcdENDIAN_BIG
            _UploadBGRtoARGBBE(trgLogical,
                               trgStride,
                               X, Y,
                               right, bottom,
                               edgeX, edgeY,
                               countX, countY,
                               Memory,
                               (SourceStride == 0)
                               ? (gctINT) Width * 3
                               : SourceStride);
#else
            _UploadBGRtoARGB(trgLogical,
                             trgStride,
                             X, Y,
                             right, bottom,
                             edgeX, edgeY,
                             countX, countY,
                             Memory,
                             (SourceStride == 0)
                             ? (gctINT) Width * 3
                             : SourceStride);
#endif

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case gcvSURF_A8B8G8R8:
#if gcdENDIAN_BIG
            _UploadABGRtoARGBBE(trgLogical,
                                trgStride,
                                X, Y,
                                right, bottom,
                                edgeX, edgeY,
                                countX, countY,
                                Memory,
                                (SourceStride == 0)
                                ? (gctINT) Width * 4
                                : SourceStride);
#else
            _UploadABGRtoARGB(trgLogical,
                              trgStride,
                              X, Y,
                              right, bottom,
                              edgeX, edgeY,
                              countX, countY,
                              Memory,
                              (SourceStride == 0)
                              ? (gctINT) Width * 4
                              : SourceStride);
#endif

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case gcvSURF_L8:
#if gcdENDIAN_BIG
            _UploadL8toARGBBE(trgLogical,
                              trgStride,
                              X, Y,
                              right, bottom,
                              edgeX, edgeY,
                              countX, countY,
                              Memory,
                              (SourceStride == 0)
                              ? (gctINT) Width
                              : SourceStride);
#else
            _UploadL8toARGB(trgLogical,
                            trgStride,
                            X, Y,
                            right, bottom,
                            edgeX, edgeY,
                            countX, countY,
                            Memory,
                            (SourceStride == 0)
                            ? (gctINT) Width
                            : SourceStride);
#endif

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case gcvSURF_A8L8:
#if gcdENDIAN_BIG
            _UploadA8L8toARGBBE(trgLogical,
                                trgStride,
                                X, Y,
                                right, bottom,
                                edgeX, edgeY,
                                countX, countY,
                                Memory,
                                (SourceStride == 0)
                                ? (gctINT) Width * 2
                                : SourceStride);
#else
            _UploadA8L8toARGB(trgLogical,
                              trgStride,
                              X, Y,
                              right, bottom,
                              edgeX, edgeY,
                              countX, countY,
                              Memory,
                              (SourceStride == 0)
                              ? (gctINT) Width * 2
                              : SourceStride);
#endif

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case gcvSURF_A8R8G8B8:
#if gcdENDIAN_BIG
            _Upload32bppto32bppBE(trgLogical,
                                  trgStride,
                                  X, Y,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 4
                                  : SourceStride);
#else
            _Upload32bppto32bpp(trgLogical,
                                trgStride,
                                X, Y,
                                right, bottom,
                                edgeX, edgeY,
                                countX, countY,
                                Memory,
                                (SourceStride == 0)
                                ? (gctINT) Width * 4
                                : SourceStride);
#endif

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case gcvSURF_R5G6B5:
#if gcdENDIAN_BIG
                _UploadRGB565toARGBBE(trgLogical,
                                      trgStride,
                                      X, Y,
                                      right, bottom,
                                      edgeX, edgeY,
                                      countX, countY,
                                      Memory,
                                      (SourceStride == 0)
                                      ? (gctINT) Width * 2
                                      : SourceStride);
#else
                _UploadRGB565toARGB(trgLogical,
                                    trgStride,
                                    X, Y,
                                    right, bottom,
                                    edgeX, edgeY,
                                    countX, countY,
                                    Memory,
                                    (SourceStride == 0)
                                    ? (gctINT) Width * 2
                                    : SourceStride);
#endif

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case gcvSURF_R4G4B4A4:
#if gcdENDIAN_BIG
                 _UploadRGBA4444toARGBBE(trgLogical,
                                        trgStride,
                                        X, Y,
                                        right, bottom,
                                        edgeX, edgeY,
                                        countX, countY,
                                        Memory,
                                        (SourceStride == 0)
                                        ? (gctINT) Width * 2
                                        : SourceStride);
#else
                 _UploadRGBA4444toARGB(trgLogical,
                                      trgStride,
                                      X, Y,
                                      right, bottom,
                                      edgeX, edgeY,
                                      countX, countY,
                                      Memory,
                                      (SourceStride == 0)
                                      ? (gctINT) Width * 2
                                      : SourceStride);
#endif

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case gcvSURF_R5G5B5A1:
#if gcdENDIAN_BIG
            _UploadRGBA5551toARGBBE(trgLogical,
                                    trgStride,
                                    X, Y,
                                    right, bottom,
                                    edgeX, edgeY,
                                    countX, countY,
                                    Memory,
                                    (SourceStride == 0)
                                    ? (gctINT) Width * 2
                                    : SourceStride);
#else
            _UploadRGBA5551toARGB(trgLogical,
                                  trgStride,
                                  X, Y,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);
#endif

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        default:
            break;
        }
    }

    else
    {
        switch (SourceFormat)
        {
        case gcvSURF_A8:
            if (trgFormat == gcvSURF_A8)
            {
#if gcdENDIAN_BIG
                _Upload8bppto8bppBE(trgLogical,
                                    trgStride,
                                    X, Y,
                                    right, bottom,
                                    edgeX, edgeY,
                                    countX, countY,
                                    Memory,
                                    (SourceStride == 0)
                                    ? (gctINT) Width
                                    : SourceStride);
#else
                _Upload8bppto8bpp(trgLogical,
                                  trgStride,
                                  X, Y,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width
                                  : SourceStride);
#endif

                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
            break;

        case gcvSURF_B8G8R8:
            if (trgFormat == gcvSURF_X8R8G8B8)
            {
                /* Same as BGR to ARGB. */
#if gcdENDIAN_BIG
                _UploadBGRtoARGBBE(trgLogical,
                                   trgStride,
                                   X, Y,
                                   right, bottom,
                                   edgeX, edgeY,
                                   countX, countY,
                                   Memory,
                                   (SourceStride == 0)
                                   ? (gctINT) Width * 3
                                   : SourceStride);
#else
                _UploadBGRtoARGB(trgLogical,
                                 trgStride,
                                 X, Y,
                                 right, bottom,
                                 edgeX, edgeY,
                                 countX, countY,
                                 Memory,
                                 (SourceStride == 0)
                                 ? (gctINT) Width * 3
                                 : SourceStride);
#endif

                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
            else if (trgFormat == gcvSURF_X8B8G8R8)
            {
#if gcdENDIAN_BIG
                _UploadBGRtoABGRBE(trgLogical,
                                   trgStride,
                                   X, Y,
                                   right, bottom,
                                   edgeX, edgeY,
                                   countX, countY,
                                   Memory,
                                   (SourceStride == 0)
                                   ? (gctINT) Width * 3
                                   : SourceStride);
#else
                _UploadBGRtoABGR(trgLogical,
                                 trgStride,
                                 X, Y,
                                 right, bottom,
                                 edgeX, edgeY,
                                 countX, countY,
                                 Memory,
                                 (SourceStride == 0)
                                 ? (gctINT) Width * 3
                                 : SourceStride);
#endif

                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }

            break;

        /* case gcvSURF_A8B8G8R8: */

        case gcvSURF_L8:
            if (trgFormat == gcvSURF_L8)
            {
#if gcdENDIAN_BIG
                _Upload8bppto8bppBE(trgLogical,
                                    trgStride,
                                    X, Y,
                                    right, bottom,
                                    edgeX, edgeY,
                                    countX, countY,
                                    Memory,
                                    (SourceStride == 0)
                                    ? (gctINT) Width
                                    : SourceStride);
#else
                _Upload8bppto8bpp(trgLogical,
                                  trgStride,
                                  X, Y,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width
                                  : SourceStride);
#endif

                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
            break;

        case gcvSURF_A8L8:
            if (trgFormat == gcvSURF_A8L8)
            {
#if gcdENDIAN_BIG
                _Upload16bppto16bppBE(trgLogical,
                                      trgStride,
                                      X, Y,
                                      right, bottom,
                                      edgeX, edgeY,
                                      countX, countY,
                                      Memory,
                                      (SourceStride == 0)
                                      ? (gctINT) Width * 2
                                      : SourceStride);
#else
                _Upload16bppto16bpp(trgLogical,
                                    trgStride,
                                    X, Y,
                                    right, bottom,
                                    edgeX, edgeY,
                                    countX, countY,
                                    Memory,
                                    (SourceStride == 0)
                                    ? (gctINT) Width * 2
                                    : SourceStride);
#endif

                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
            break;

        /* case gcvSURF_A8R8G8B8: */

        case gcvSURF_R5G6B5:
            if (trgFormat == gcvSURF_R5G6B5)
            {
#if gcdENDIAN_BIG
                _Upload16bppto16bppBE(trgLogical,
                                      trgStride,
                                      X, Y,
                                      right, bottom,
                                      edgeX, edgeY,
                                      countX, countY,
                                      Memory,
                                      (SourceStride == 0)
                                      ? (gctINT) Width * 2
                                      : SourceStride);
#else
                _Upload16bppto16bpp(trgLogical,
                                    trgStride,
                                    X, Y,
                                    right, bottom,
                                    edgeX, edgeY,
                                    countX, countY,
                                    Memory,
                                    (SourceStride == 0)
                                    ? (gctINT) Width * 2
                                    : SourceStride);
#endif

                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
            break;

        case gcvSURF_R4G4B4A4:
            if (trgFormat == gcvSURF_A4R4G4B4)
            {
#if gcdENDIAN_BIG
                _UploadRGBA4444toARGB4444BE(trgLogical,
                                            trgStride,
                                            X, Y,
                                            right, bottom,
                                            edgeX, edgeY,
                                            countX, countY,
                                            Memory,
                                            (SourceStride == 0)
                                            ? (gctINT) Width * 2
                                            : SourceStride);
#else
                _UploadRGBA4444toARGB4444(trgLogical,
                                          trgStride,
                                          X, Y,
                                          right, bottom,
                                          edgeX, edgeY,
                                          countX, countY,
                                          Memory,
                                          (SourceStride == 0)
                                          ? (gctINT) Width * 2
                                          : SourceStride);
#endif

                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
            break;

        case gcvSURF_R5G5B5A1:
            if (trgFormat == gcvSURF_A1R5G5B5)
            {
#if gcdENDIAN_BIG
                _UploadRGBA5551toARGB1555BE(trgLogical,
                                            trgStride,
                                            X, Y,
                                            right, bottom,
                                            edgeX, edgeY,
                                            countX, countY,
                                            Memory,
                                            (SourceStride == 0)
                                            ? (gctINT) Width * 2
                                            : SourceStride);
#else
                _UploadRGBA5551toARGB1555(trgLogical,
                                          trgStride,
                                          X, Y,
                                          right, bottom,
                                          edgeX, edgeY,
                                          countX, countY,
                                          Memory,
                                          (SourceStride == 0)
                                          ? (gctINT) Width * 2
                                          : SourceStride);
#endif

                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
            break;

        default:
            break;
        }
    }

    /* Get number of bytes per pixel and tile for the format. */
    gcmONERROR(gcoHARDWARE_ConvertFormat(trgFormat,
                                         &bitsPerPixel,
                                         &bytesPerTile));

    /* Check for OpenCL image. */
    if (oclSrcFormat)
    {
        /* OpenCL image has to be linear without alignment. */
        /* There is no format conversion dur to OpenCL Map API. */
        gctUINT32 bytesPerPixel = bitsPerPixel / 8;

        /* Compute proper memory stride. */
        srcStride = (SourceStride == 0)
                  ? (gctINT) (Width * bytesPerPixel)
                  : SourceStride;

        srcLine = (gctUINT8_PTR) Memory + bytesPerPixel * X;
        trgLine = (gctUINT8_PTR) trgLogical + Offset + bytesPerPixel * X;
        for (y = Y; y < Height; y++)
        {
            gcoOS_MemCopy(trgLine, srcLine, bytesPerPixel * Width);

            srcLine += SourceStride;
            trgLine += SourceStride;
        }

        /* Success. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    /* Compute number of bytes per tile line. */
    bytesPerLine = bitsPerPixel * 4 / 8;

    /* See if the memory format equals the texture format (easy case). */
    if ((SourceFormat == trgFormat)
    ||  ((SourceFormat == gcvSURF_A8R8G8B8) && (trgFormat == gcvSURF_X8R8G8B8))
    ||  ((SourceFormat == gcvSURF_A8B8G8R8) && (trgFormat == gcvSURF_X8B8G8R8)))
    {
        switch (bitsPerPixel)
        {
        case 8:
#if gcdENDIAN_BIG
            _Upload8bppto8bppBE(trgLogical,
                                trgStride,
                                X, Y,
                                right, bottom,
                                edgeX, edgeY,
                                countX, countY,
                                Memory,
                                (SourceStride == 0)
                                ? (gctINT) Width
                                : SourceStride);
#else
            _Upload8bppto8bpp(trgLogical,
                              trgStride,
                              X, Y,
                              right, bottom,
                              edgeX, edgeY,
                              countX, countY,
                              Memory,
                              (SourceStride == 0)
                              ? (gctINT) Width
                              : SourceStride);
#endif

            /* Success. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case 16:
#if gcdENDIAN_BIG
            _Upload16bppto16bppBE(trgLogical,
                                  trgStride,
                                  X, Y,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);
#else
            _Upload16bppto16bpp(trgLogical,
                                trgStride,
                                X, Y,
                                right, bottom,
                                edgeX, edgeY,
                                countX, countY,
                                Memory,
                                (SourceStride == 0)
                                ? (gctINT) Width * 2
                                : SourceStride);
#endif

            /* Success. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case 32:
#if gcdENDIAN_BIG
            _Upload32bppto32bppBE(trgLogical,
                                  trgStride,
                                  X, Y,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 4
                                  : SourceStride);
#else
            _Upload32bppto32bpp(trgLogical,
                                trgStride,
                                X, Y,
                                right, bottom,
                                edgeX, edgeY,
                                countX, countY,
                                Memory,
                                (SourceStride == 0)
                                ? (gctINT) Width * 4
                                : SourceStride);
#endif

            /* Success. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case 64:
#if gcdENDIAN_BIG
            _Upload64bppto64bppBE(trgLogical,
                                  trgStride,
                                  X, Y,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 8
                                  : SourceStride);
#else
            _Upload64bppto64bpp(trgLogical,
                                trgStride,
                                X, Y,
                                right, bottom,
                                edgeX, edgeY,
                                countX, countY,
                                Memory,
                                (SourceStride == 0)
                                ? (gctINT) Width * 8
                                : SourceStride);
#endif

            /* Success. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        default:
            /* Invalid format. */
            gcmASSERT(gcvTRUE);
        }

        /* Compute proper memory stride. */
        srcStride = (SourceStride == 0)
                  ? (gctINT) (Width * bitsPerPixel / 8)
                  : SourceStride;

        if (SourceFormat == gcvSURF_DXT1)
        {
            gcmASSERT((X == 0) && (Y == 0));

            srcLine      = (gctUINT8_PTR) Memory;
            trgLine      = (gctUINT8_PTR) trgLogical;
            bytesPerLine = Width * bytesPerTile / 4;

            for (y = Y; y < Height; y += 4)
            {
                gcoOS_MemCopy(trgLine, srcLine, bytesPerLine);

                trgLine += trgStride;
                srcLine += srcStride * 4;
            }

            /* Success. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }

        /* Success. */
        gcmFOOTER_ARG("bitsPerPixel=%d not supported", bitsPerPixel);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    gcmTRACE_ZONE(
        gcvLEVEL_WARNING, _GC_OBJ_ZONE,
        "Slow path: SourceFormat=%d trgFormat=%d",
        SourceFormat, trgFormat
        );

    /* Query format specifics. */
    trgFormatInfo = &TexSurf->formatInfo;
    gcmONERROR(gcoHARDWARE_QueryFormat(SourceFormat, &srcFormatInfo));

    /* Determine bit step. */
    srcBitStep = srcFormatInfo->interleaved
               ? srcFormatInfo->bitsPerPixel * 2
               : srcFormatInfo->bitsPerPixel;

    trgBitStep = trgFormatInfo->interleaved
               ? trgFormatInfo->bitsPerPixel * 2
               : trgFormatInfo->bitsPerPixel;

    /* Compute proper memory stride. */
    srcStride = (SourceStride == 0)
              ? (gctINT) Width * ((srcFormatInfo->bitsPerPixel + 7) >> 3)
              : SourceStride;

    /* Compute the starting source and target addresses. */
    srcLine = (gctUINT8_PTR) Memory;

    /* Compute Address of line inside the tile in which X,Y reside. */
    trgLine = (gctUINT8_PTR) trgLogical
            + (Y & ~3) * trgStride + (Y & 3) * bytesPerLine
            + (X >> 2) * bytesPerTile;

    /* Walk all vertical lines. */
    for (y = Y; y < bottom; y++)
    {
        /* Get starting source and target addresses. */
        srcPixel = srcLine;
        trgTile  = trgLine;
        trgPixel = trgTile + (X & 3) * bitsPerPixel / 8;

        /* Reset bit positions. */
        srcBit = 0;
        trgBit = 0;

        /* Walk all horizontal pixels. */
        for (x = X; x < right; x++)
        {
            /* Determine whether to use odd format descriptor for the current
            ** pixel. */
            srcOdd = x & srcFormatInfo->interleaved;
            trgOdd = x & trgFormatInfo->interleaved;

            /* Convert the current pixel. */
            gcmONERROR(gcoHARDWARE_ConvertPixel(srcPixel,
                                                trgPixel,
                                                srcBit,
                                                trgBit,
                                                srcFormatInfo,
                                                trgFormatInfo,
                                                gcvNULL,
                                                gcvNULL,
                                                srcOdd,
                                                trgOdd));

            /* Move to the next pixel in the source. */
            if (!srcFormatInfo->interleaved || srcOdd)
            {
                srcBit   += srcBitStep;
                srcPixel += srcBit >> 3;
                srcBit   &= 7;
            }

            /* Move to the next pixel in the target. */
            if ((x & 3) < 3)
            {
                /* Still within a tile, update to next pixel. */
                if (!trgFormatInfo->interleaved || trgOdd)
                {
                    trgBit   += trgBitStep;
                    trgPixel += trgBit >> 3;
                    trgBit   &= 7;
                }
            }
            else
            {
                /* Move to next tiled address in target. */
                trgTile += bytesPerTile;
                trgPixel = trgTile;
                trgBit = 0;
            }
        }

        /* Move to next line. */
        srcLine += srcStride;

        if ((y & 3) < 3)
        {
            /* Still within a tile, update to next line inside the tile. */
            trgLine += bytesPerLine;
        }
        else
        {
            /* New tile, move to beginning of new tile. */
            trgLine += trgStride * 4 - 3 * bytesPerLine;
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#if !defined(UNDER_CE)
static void
_UploadSuperTiledL8toARGB(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 1);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* L8 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = L8toARGB(u);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* L8 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = L8toARGB(u);
                u = srcLine[1];
                ((gctUINT32_PTR) trgLine)[1] = L8toARGB(u);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[2] = L8toARGB(u);
                u = srcLine[3];
                ((gctUINT32_PTR) trgLine)[3] = L8toARGB(u);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* L8 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = L8toARGB(u);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;
            register gctUINT32 u;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* L8 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = L8toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 1] = L8toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 2] = L8toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 3] = L8toARGB(u);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = L8toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 5] = L8toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 6] = L8toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 7] = L8toARGB(u);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = L8toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 9] = L8toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[10] = L8toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[11] = L8toARGB(u);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = L8toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[13] = L8toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[14] = L8toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[15] = L8toARGB(u);
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadSuperTiledL8toARGBBE(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 1);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* L8 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = L8toARGBBE(u);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* L8 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = L8toARGBBE(u);
                u = srcLine[1];
                ((gctUINT32_PTR) trgLine)[1] = L8toARGBBE(u);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[2] = L8toARGBBE(u);
                u = srcLine[3];
                ((gctUINT32_PTR) trgLine)[3] = L8toARGBBE(u);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* L8 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = L8toARGBBE(u);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;
            register gctUINT32 u;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* L8 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = L8toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 1] = L8toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 2] = L8toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 3] = L8toARGBBE(u);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = L8toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 5] = L8toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 6] = L8toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 7] = L8toARGBBE(u);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = L8toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 9] = L8toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[10] = L8toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[11] = L8toARGBBE(u);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = L8toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[13] = L8toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[14] = L8toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[15] = L8toARGBBE(u);
        }
    }
}
#endif


static void
_UploadSuperTiledA8L8toARGB(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        register gctUINT32 u;
        y  = EdgeY[j];

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

            /* A8L8 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = u | (u << 8) | (u << 16) | (srcLine[1] << 24);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

                /* A8L8 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = u | (u << 8) | (u << 16) | (srcLine[1] << 24);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[1] = u | (u << 8) | (u << 16) | (srcLine[3] << 24);
                u = srcLine[4];
                ((gctUINT32_PTR) trgLine)[2] = u | (u << 8) | (u << 16) | (srcLine[5] << 24);
                u = srcLine[6];
                ((gctUINT32_PTR) trgLine)[3] = u | (u << 8) | (u << 16) | (srcLine[7] << 24);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

                /* A8L8 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = u | (u << 8) | (u << 16) | (srcLine[1] << 24);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;
            register gctUINT32 u;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

            /* A8 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = u | (u << 8) | (u << 16) | (srcLine[1] << 24);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 1] = u | (u << 8) | (u << 16) | (srcLine[3] << 24);
            u = src[4];
            ((gctUINT32_PTR) trgLine)[ 2] = u | (u << 8) | (u << 16) | (srcLine[5] << 24);
            u = src[6];
            ((gctUINT32_PTR) trgLine)[ 3] = u | (u << 8) | (u << 16) | (srcLine[7] << 24);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = u | (u << 8) | (u << 16) | (srcLine[1] << 24);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 5] = u | (u << 8) | (u << 16) | (srcLine[3] << 24);
            u = src[4];
            ((gctUINT32_PTR) trgLine)[ 6] = u | (u << 8) | (u << 16) | (srcLine[5] << 24);
            u = src[6];
            ((gctUINT32_PTR) trgLine)[ 7] = u | (u << 8) | (u << 16) | (srcLine[7] << 24);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = u | (u << 8) | (u << 16) | (srcLine[1] << 24);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 9] = u | (u << 8) | (u << 16) | (srcLine[3] << 24);
            u = src[4];
            ((gctUINT32_PTR) trgLine)[10] = u | (u << 8) | (u << 16) | (srcLine[5] << 24);
            u = src[6];
            ((gctUINT32_PTR) trgLine)[11] = u | (u << 8) | (u << 16) | (srcLine[7] << 24);

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = u | (u << 8) | (u << 16) | (srcLine[1] << 24);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[13] = u | (u << 8) | (u << 16) | (srcLine[3] << 24);
            u = src[4];
            ((gctUINT32_PTR) trgLine)[14] = u | (u << 8) | (u << 16) | (srcLine[5] << 24);
            u = src[6];
            ((gctUINT32_PTR) trgLine)[15] = u | (u << 8) | (u << 16) | (srcLine[7] << 24);
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadSuperTiledA8L8toARGBBE(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

            /* A8L8 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = (u << 8) | (u << 16) | (u << 24) | srcLine[1];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

                /* A8L8 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = (u << 8) | (u << 16) | (u << 24) | srcLine[1];
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[1] = (u << 8) | (u << 16) | (u << 24) | srcLine[3];
                u = srcLine[4];
                ((gctUINT32_PTR) trgLine)[2] = (u << 8) | (u << 16) | (u << 24) | srcLine[5];
                u = srcLine[6];
                ((gctUINT32_PTR) trgLine)[3] = (u << 8) | (u << 16) | (u << 24) | srcLine[7];
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

                /* A8L8 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = (u << 8) | (u << 16) | (u << 24) | srcLine[1];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;
            register gctUINT32 u;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

            /* A8 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = (u << 8) | (u << 16) | (u << 24) | srcLine[1];
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 1] = (u << 8) | (u << 16) | (u << 24) | srcLine[3];
            u = src[4];
            ((gctUINT32_PTR) trgLine)[ 2] = (u << 8) | (u << 16) | (u << 24) | srcLine[5];
            u = src[6];
            ((gctUINT32_PTR) trgLine)[ 3] = (u << 8) | (u << 16) | (u << 24) | srcLine[7];

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = (u << 8) | (u << 16) | (u << 24) | srcLine[1];
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 5] = (u << 8) | (u << 16) | (u << 24) | srcLine[3];
            u = src[4];
            ((gctUINT32_PTR) trgLine)[ 6] = (u << 8) | (u << 16) | (u << 24) | srcLine[5];
            u = src[6];
            ((gctUINT32_PTR) trgLine)[ 7] = (u << 8) | (u << 16) | (u << 24) | srcLine[7];

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = (u << 8) | (u << 16) | (u << 24) | srcLine[1];
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 9] = (u << 8) | (u << 16) | (u << 24) | srcLine[3];
            u = src[4];
            ((gctUINT32_PTR) trgLine)[10] = (u << 8) | (u << 16) | (u << 24) | srcLine[5];
            u = src[6];
            ((gctUINT32_PTR) trgLine)[11] = (u << 8) | (u << 16) | (u << 24) | srcLine[7];

            src += SourceStride;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = (u << 8) | (u << 16) | (u << 24) | srcLine[1];
            u = src[2];
            ((gctUINT32_PTR) trgLine)[13] = (u << 8) | (u << 16) | (u << 24) | srcLine[3];
            u = src[4];
            ((gctUINT32_PTR) trgLine)[14] = (u << 8) | (u << 16) | (u << 24) | srcLine[5];
            u = src[6];
            ((gctUINT32_PTR) trgLine)[15] = (u << 8) | (u << 16) | (u << 24) | srcLine[7];
        }
    }
}
#endif

static void
_UploadSuperTiledA8toARGB(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT32 x, y;
    gctUINT32 xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;
    gctUINT i, j;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 1);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* A8 to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = srcLine[0] << 24;
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* A8 to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = srcLine[0] << 24;
                ((gctUINT32_PTR) trgLine)[1] = srcLine[1] << 24;
                ((gctUINT32_PTR) trgLine)[2] = srcLine[2] << 24;
                ((gctUINT32_PTR) trgLine)[3] = srcLine[3] << 24;
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* A8 to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[ 0] = srcLine[0] << 24;
             }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;
            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* A8 to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = src[0] << 24;
            ((gctUINT32_PTR) trgLine)[ 1] = src[1] << 24;
            ((gctUINT32_PTR) trgLine)[ 2] = src[2] << 24;
            ((gctUINT32_PTR) trgLine)[ 3] = src[3] << 24;

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = src[0] << 24;
            ((gctUINT32_PTR) trgLine)[ 5] = src[1] << 24;
            ((gctUINT32_PTR) trgLine)[ 6] = src[2] << 24;
            ((gctUINT32_PTR) trgLine)[ 7] = src[3] << 24;

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = src[0] << 24;
            ((gctUINT32_PTR) trgLine)[ 9] = src[1] << 24;
            ((gctUINT32_PTR) trgLine)[ 0] = src[2] << 24;
            ((gctUINT32_PTR) trgLine)[11] = src[3] << 24;

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = src[0] << 24;
            ((gctUINT32_PTR) trgLine)[13] = src[1] << 24;
            ((gctUINT32_PTR) trgLine)[14] = src[2] << 24;
            ((gctUINT32_PTR) trgLine)[15] = src[3] << 24;
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadSuperTiledA8toARGBBE(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT32 x, y;
    gctUINT32 xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;
    gctUINT i, j;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 1);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* A8 to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = (gctUINT32) srcLine[0];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* A8 to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (gctUINT32) srcLine[0];
                ((gctUINT32_PTR) trgLine)[1] = (gctUINT32) srcLine[1];
                ((gctUINT32_PTR) trgLine)[2] = (gctUINT32) srcLine[2];
                ((gctUINT32_PTR) trgLine)[3] = (gctUINT32) srcLine[3];
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* A8 to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (gctUINT32) srcLine[0];
             }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;
            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* A8 to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = (gctUINT32) src[0];
            ((gctUINT32_PTR) trgLine)[ 1] = (gctUINT32) src[1];
            ((gctUINT32_PTR) trgLine)[ 2] = (gctUINT32) src[2];
            ((gctUINT32_PTR) trgLine)[ 3] = (gctUINT32) src[3];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = (gctUINT32) src[0];
            ((gctUINT32_PTR) trgLine)[ 5] = (gctUINT32) src[1];
            ((gctUINT32_PTR) trgLine)[ 6] = (gctUINT32) src[2];
            ((gctUINT32_PTR) trgLine)[ 7] = (gctUINT32) src[3];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = (gctUINT32) src[0];
            ((gctUINT32_PTR) trgLine)[ 9] = (gctUINT32) src[1];
            ((gctUINT32_PTR) trgLine)[ 0] = (gctUINT32) src[2];
            ((gctUINT32_PTR) trgLine)[11] = (gctUINT32) src[3];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = (gctUINT32) src[0];
            ((gctUINT32_PTR) trgLine)[13] = (gctUINT32) src[1];
            ((gctUINT32_PTR) trgLine)[14] = (gctUINT32) src[2];
            ((gctUINT32_PTR) trgLine)[15] = (gctUINT32) src[3];
        }
    }
}
#endif

static void
_UploadSuperTiledRGB565toARGB(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];

        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGB565 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = RGB565toARGB(u);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGB565 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGB565toARGB(u);
                u = srcLine[1];
                ((gctUINT32_PTR) trgLine)[1] = RGB565toARGB(u);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[2] = RGB565toARGB(u);
                u = srcLine[3];
                ((gctUINT32_PTR) trgLine)[3] = RGB565toARGB(u);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGB565 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGB565toARGB(u);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;
            register gctUINT32 u;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGB565 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = RGB565toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 1] = RGB565toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 2] = RGB565toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 3] = RGB565toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = RGB565toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 5] = RGB565toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 6] = RGB565toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 7] = RGB565toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = RGB565toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 9] = RGB565toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[10] = RGB565toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[11] = RGB565toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = RGB565toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[13] = RGB565toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[14] = RGB565toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[15] = RGB565toARGB(u);
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadSuperTiledRGB565toARGBBE(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2);

            /* RGB565 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = RGB565toARGBBE(u);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2);

                /* RGB565 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGB565toARGBBE(u);
                u = srcLine[1];
                ((gctUINT32_PTR) trgLine)[1] = RGB565toARGBBE(u);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[2] = RGB565toARGBBE(u);
                u = srcLine[3];
                ((gctUINT32_PTR) trgLine)[3] = RGB565toARGBBE(u);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2);

                /* RGB565 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGB565toARGBBE(u);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;
            register gctUINT32 u;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2);

            /* RGB565 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = RGB565toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 1] = RGB565toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 2] = RGB565toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 3] = RGB565toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = RGB565toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 5] = RGB565toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 6] = RGB565toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 7] = RGB565toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = RGB565toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 9] = RGB565toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[10] = RGB565toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[11] = RGB565toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = RGB565toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[13] = RGB565toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[14] = RGB565toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[15] = RGB565toARGBBE(u);
        }
    }
}
#endif

static void
_UploadSuperTiledRGBA4444toARGB(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];

        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA4444 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = RGBA4444toARGB(u);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGBA4444toARGB(u);
                u = srcLine[1];
                ((gctUINT32_PTR) trgLine)[1] = RGBA4444toARGB(u);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[2] = RGBA4444toARGB(u);
                u = srcLine[3];
                ((gctUINT32_PTR) trgLine)[3] = RGBA4444toARGB(u);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                 yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGBA4444toARGB(u);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;
            register gctUINT32 u;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA4444 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = RGBA4444toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 1] = RGBA4444toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 2] = RGBA4444toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 3] = RGBA4444toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = RGBA4444toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 5] = RGBA4444toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 6] = RGBA4444toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 7] = RGBA4444toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = RGBA4444toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 9] = RGBA4444toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[10] = RGBA4444toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[11] = RGBA4444toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = RGBA4444toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[13] = RGBA4444toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[14] = RGBA4444toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[15] = RGBA4444toARGB(u);
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadSuperTiledRGBA4444toARGBBE(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA4444 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = RGBA4444toARGBBE(u);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                 yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGBA4444toARGBBE(u);
                u = srcLine[1];
                ((gctUINT32_PTR) trgLine)[1] = RGBA4444toARGBBE(u);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[2] = RGBA4444toARGBBE(u);
                u = srcLine[3];
                ((gctUINT32_PTR) trgLine)[3] = RGBA4444toARGBBE(u);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;

                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                 yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGBA4444toARGBBE(u);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;
            register gctUINT32 u;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA4444 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = RGBA4444toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 1] = RGBA4444toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 2] = RGBA4444toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 3] = RGBA4444toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = RGBA4444toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 5] = RGBA4444toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 6] = RGBA4444toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 7] = RGBA4444toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = RGBA4444toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 9] = RGBA4444toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[10] = RGBA4444toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[11] = RGBA4444toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = RGBA4444toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[13] = RGBA4444toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[14] = RGBA4444toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[15] = RGBA4444toARGBBE(u);
        }
    }
}
#endif

static void
_UploadSuperTiledRGBA5551toARGB(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA5551 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = RGBA5551toARGB(u);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGBA5551toARGB(u);
                u = srcLine[1];
                ((gctUINT32_PTR) trgLine)[1] = RGBA5551toARGB(u);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[2] = RGBA5551toARGB(u);
                u = srcLine[3];
                ((gctUINT32_PTR) trgLine)[3] = RGBA5551toARGB(u);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGBA5551toARGB(u);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;
            register gctUINT32 u;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA5551 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = RGBA5551toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 1] = RGBA5551toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 2] = RGBA5551toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 3] = RGBA5551toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = RGBA5551toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 5] = RGBA5551toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 6] = RGBA5551toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 7] = RGBA5551toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = RGBA5551toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 9] = RGBA5551toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[10] = RGBA5551toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[11] = RGBA5551toARGB(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = RGBA5551toARGB(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[13] = RGBA5551toARGB(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[14] = RGBA5551toARGB(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[15] = RGBA5551toARGB(u);
         }
    }
}

#if gcdENDIAN_BIG
static void
_UploadSuperTiledRGBA5551toARGBBE(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        for (i = 0; i < CountX; i++)
        {
            register gctUINT32 u;
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA5551 to ARGB. */
            u = srcLine[0];
            ((gctUINT32_PTR) trgLine)[0] = RGBA5551toARGBBE(u);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                register gctUINT32 u;
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB: one tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGBA5551toARGBBE(u);
                u = srcLine[1];
                ((gctUINT32_PTR) trgLine)[1] = RGBA5551toARGBBE(u);
                u = srcLine[2];
                ((gctUINT32_PTR) trgLine)[2] = RGBA5551toARGBBE(u);
                u = srcLine[3];
                ((gctUINT32_PTR) trgLine)[3] = RGBA5551toARGBBE(u);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                register gctUINT32 u;
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB: part tile row. */
                u = srcLine[0];
                ((gctUINT32_PTR) trgLine)[0] = RGBA5551toARGBBE(u);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;
            register gctUINT32 u;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA5551 to ARGB: one tile. */
            src = srcLine;
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 0] = RGBA5551toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 1] = RGBA5551toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 2] = RGBA5551toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 3] = RGBA5551toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 4] = RGBA5551toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 5] = RGBA5551toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[ 6] = RGBA5551toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[ 7] = RGBA5551toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[ 8] = RGBA5551toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[ 9] = RGBA5551toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[10] = RGBA5551toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[11] = RGBA5551toARGBBE(u);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            u = src[0];
            ((gctUINT32_PTR) trgLine)[12] = RGBA5551toARGBBE(u);
            u = src[1];
            ((gctUINT32_PTR) trgLine)[13] = RGBA5551toARGBBE(u);
            u = src[2];
            ((gctUINT32_PTR) trgLine)[14] = RGBA5551toARGBBE(u);
            u = src[3];
            ((gctUINT32_PTR) trgLine)[15] = RGBA5551toARGBBE(u);
         }
    }
}
#endif

static void
_UploadSuperTiledBGRtoABGR(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT32 x, y;
    gctUINT32 xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;
    gctUINT i, j;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 3);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

            /* BGR to ABGR. */
            ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | srcLine[0] | (srcLine[ 1] << 8) | (srcLine[ 2] << 16);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ABGR: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | srcLine[0] | (srcLine[ 1] << 8) | (srcLine[ 2] << 16);
                ((gctUINT32_PTR) trgLine)[1] = 0xFF000000 | srcLine[3] | (srcLine[ 4] << 8) | (srcLine[ 5] << 16);
                ((gctUINT32_PTR) trgLine)[2] = 0xFF000000 | srcLine[6] | (srcLine[ 7] << 8) | (srcLine[ 8] << 16);
                ((gctUINT32_PTR) trgLine)[3] = 0xFF000000 | srcLine[9] | (srcLine[10] << 8) | (srcLine[11] << 16);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ABGR: part tile row. */
            ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | srcLine[0] | (srcLine[ 1] << 8) | (srcLine[ 2] << 16);
             }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

            /* BGR to ABGR: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = 0xFF000000 | src[0] | (src[ 1] << 8) | (src[ 2] << 16);
            ((gctUINT32_PTR) trgLine)[ 1] = 0xFF000000 | src[3] | (src[ 4] << 8) | (src[ 5] << 16);
            ((gctUINT32_PTR) trgLine)[ 2] = 0xFF000000 | src[6] | (src[ 7] << 8) | (src[ 8] << 16);
            ((gctUINT32_PTR) trgLine)[ 3] = 0xFF000000 | src[9] | (src[10] << 8) | (src[11] << 16);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = 0xFF000000 | src[0] | (src[ 1] << 8) | (src[ 2] << 16);
            ((gctUINT32_PTR) trgLine)[ 5] = 0xFF000000 | src[3] | (src[ 4] << 8) | (src[ 5] << 16);
            ((gctUINT32_PTR) trgLine)[ 6] = 0xFF000000 | src[6] | (src[ 7] << 8) | (src[ 8] << 16);
            ((gctUINT32_PTR) trgLine)[ 7] = 0xFF000000 | src[9] | (src[10] << 8) | (src[11] << 16);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = 0xFF000000 | src[0] | (src[ 1] << 8) | (src[ 2] << 16);
            ((gctUINT32_PTR) trgLine)[ 9] = 0xFF000000 | src[3] | (src[ 4] << 8) | (src[ 5] << 16);
            ((gctUINT32_PTR) trgLine)[10] = 0xFF000000 | src[6] | (src[ 7] << 8) | (src[ 8] << 16);
            ((gctUINT32_PTR) trgLine)[11] = 0xFF000000 | src[9] | (src[10] << 8) | (src[11] << 16);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = 0xFF000000 | src[0] | (src[ 1] << 8) | (src[ 2] << 16);
            ((gctUINT32_PTR) trgLine)[13] = 0xFF000000 | src[3] | (src[ 4] << 8) | (src[ 5] << 16);
            ((gctUINT32_PTR) trgLine)[14] = 0xFF000000 | src[6] | (src[ 7] << 8) | (src[ 8] << 16);
            ((gctUINT32_PTR) trgLine)[15] = 0xFF000000 | src[9] | (src[10] << 8) | (src[11] << 16);
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadSuperTiledBGRtoABGRBE(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT32 x, y;
    gctUINT32 xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;
    gctUINT i, j;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 3);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

            /* BGR to ABGR. */
            ((gctUINT32_PTR) trgLine)[0] = 0xFF | (srcLine[0] << 24) | (srcLine[1] << 16) | (srcLine[2] << 8);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ABGR: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF | (srcLine[0] << 24) | (srcLine[ 1] << 16) | (srcLine[ 2] << 8);
                ((gctUINT32_PTR) trgLine)[1] = 0xFF | (srcLine[3] << 24) | (srcLine[ 4] << 16) | (srcLine[ 5] << 8);
                ((gctUINT32_PTR) trgLine)[2] = 0xFF | (srcLine[6] << 24) | (srcLine[ 7] << 16) | (srcLine[ 8] << 8);
                ((gctUINT32_PTR) trgLine)[3] = 0xFF | (srcLine[9] << 24) | (srcLine[10] << 16) | (srcLine[11] << 8);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ABGR: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF | (srcLine[0] << 24) | (srcLine[1] << 16) | (srcLine[2] << 8);
             }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

            /* BGR to ABGR: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = 0xFF | (src[0] << 24) | (src[ 1] << 16) | (src[ 2] << 8);
            ((gctUINT32_PTR) trgLine)[ 1] = 0xFF | (src[3] << 24) | (src[ 4] << 16) | (src[ 5] << 8);
            ((gctUINT32_PTR) trgLine)[ 2] = 0xFF | (src[6] << 24) | (src[ 7] << 16) | (src[ 8] << 8);
            ((gctUINT32_PTR) trgLine)[ 3] = 0xFF | (src[9] << 24) | (src[10] << 16) | (src[11] << 8);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = 0xFF | (src[0] << 24) | (src[ 1] << 16) | (src[ 2] << 8);
            ((gctUINT32_PTR) trgLine)[ 5] = 0xFF | (src[3] << 24) | (src[ 4] << 16) | (src[ 5] << 8);
            ((gctUINT32_PTR) trgLine)[ 6] = 0xFF | (src[6] << 24) | (src[ 7] << 16) | (src[ 8] << 8);
            ((gctUINT32_PTR) trgLine)[ 7] = 0xFF | (src[9] << 24) | (src[10] << 16) | (src[11] << 8);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = 0xFF | (src[0] << 24) | (src[ 1] << 16) | (src[ 2] << 8);
            ((gctUINT32_PTR) trgLine)[ 9] = 0xFF | (src[3] << 24) | (src[ 4] << 16) | (src[ 5] << 8);
            ((gctUINT32_PTR) trgLine)[10] = 0xFF | (src[6] << 24) | (src[ 7] << 16) | (src[ 8] << 8);
            ((gctUINT32_PTR) trgLine)[11] = 0xFF | (src[9] << 24) | (src[10] << 16) | (src[11] << 8);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = 0xFF | (src[0] << 24) | (src[ 1] << 16) | (src[ 2] << 8);
            ((gctUINT32_PTR) trgLine)[13] = 0xFF | (src[3] << 24) | (src[ 4] << 16) | (src[ 5] << 8);
            ((gctUINT32_PTR) trgLine)[14] = 0xFF | (src[6] << 24) | (src[ 7] << 16) | (src[ 8] << 8);
            ((gctUINT32_PTR) trgLine)[15] = 0xFF | (src[9] << 24) | (src[10] << 16) | (src[11] << 8);
        }
    }
}
#endif

static void
_UploadSuperTiledBGRtoARGB(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT32 x, y;
    gctUINT32 xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;
    gctUINT i, j;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 3);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

            /* BGR to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | (srcLine[0] << 16) | (srcLine[1] << 8) | srcLine[2];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | (srcLine[0] << 16) | (srcLine[ 1] << 8) | srcLine[ 2];
                ((gctUINT32_PTR) trgLine)[1] = 0xFF000000 | (srcLine[3] << 16) | (srcLine[ 4] << 8) | srcLine[ 5];
                ((gctUINT32_PTR) trgLine)[2] = 0xFF000000 | (srcLine[6] << 16) | (srcLine[ 7] << 8) | srcLine[ 8];
                ((gctUINT32_PTR) trgLine)[3] = 0xFF000000 | (srcLine[9] << 16) | (srcLine[10] << 8) | srcLine[11];
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[ 0] = 0xFF000000 | (srcLine[0] << 16) | (srcLine[1] << 8) | srcLine[2];
             }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

            /* BGR to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = 0xFF000000 | (src[0] << 16) | (src[ 1] << 8) | src[ 2];
            ((gctUINT32_PTR) trgLine)[ 1] = 0xFF000000 | (src[3] << 16) | (src[ 4] << 8) | src[ 5];
            ((gctUINT32_PTR) trgLine)[ 2] = 0xFF000000 | (src[6] << 16) | (src[ 7] << 8) | src[ 8];
            ((gctUINT32_PTR) trgLine)[ 3] = 0xFF000000 | (src[9] << 16) | (src[10] << 8) | src[11];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = 0xFF000000 | (src[0] << 16) | (src[ 1] << 8) | src[ 2];
            ((gctUINT32_PTR) trgLine)[ 5] = 0xFF000000 | (src[3] << 16) | (src[ 4] << 8) | src[ 5];
            ((gctUINT32_PTR) trgLine)[ 6] = 0xFF000000 | (src[6] << 16) | (src[ 7] << 8) | src[ 8];
            ((gctUINT32_PTR) trgLine)[ 7] = 0xFF000000 | (src[9] << 16) | (src[10] << 8) | src[11];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = 0xFF000000 | (src[0] << 16) | (src[ 1] << 8) | src[ 2];
            ((gctUINT32_PTR) trgLine)[ 9] = 0xFF000000 | (src[3] << 16) | (src[ 4] << 8) | src[ 5];
            ((gctUINT32_PTR) trgLine)[10] = 0xFF000000 | (src[6] << 16) | (src[ 7] << 8) | src[ 8];
            ((gctUINT32_PTR) trgLine)[11] = 0xFF000000 | (src[9] << 16) | (src[10] << 8) | src[11];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = 0xFF000000 | (src[0] << 16) | (src[ 1] << 8) | src[ 2];
            ((gctUINT32_PTR) trgLine)[13] = 0xFF000000 | (src[3] << 16) | (src[ 4] << 8) | src[ 5];
            ((gctUINT32_PTR) trgLine)[14] = 0xFF000000 | (src[6] << 16) | (src[ 7] << 8) | src[ 8];
            ((gctUINT32_PTR) trgLine)[15] = 0xFF000000 | (src[9] << 16) | (src[10] << 8) | src[11];
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadSuperTiledBGRtoARGBBE(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT32 x, y;
    gctUINT32 xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;
    gctUINT i, j;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 3);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

            /* BGR to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = 0xFF | (srcLine[0] << 8) | (srcLine[1] << 16) | (srcLine[2] << 24);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];
                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF | (srcLine[0] << 8) | (srcLine[ 1] << 16) | (srcLine[ 2] << 24);
                ((gctUINT32_PTR) trgLine)[1] = 0xFF | (srcLine[3] << 8) | (srcLine[ 4] << 16) | (srcLine[ 5] << 24);
                ((gctUINT32_PTR) trgLine)[2] = 0xFF | (srcLine[6] << 8) | (srcLine[ 7] << 16) | (srcLine[ 8] << 24);
                ((gctUINT32_PTR) trgLine)[3] = 0xFF | (srcLine[9] << 8) | (srcLine[10] << 16) | (srcLine[11] << 24);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF | (srcLine[0] << 8) | (srcLine[ 1] << 16) | (srcLine[ 2] << 24);
             }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

            /* BGR to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = 0xFF | (src[0] << 8) | (src[ 1] << 16) | (src[ 2] << 24);
            ((gctUINT32_PTR) trgLine)[ 1] = 0xFF | (src[3] << 8) | (src[ 4] << 16) | (src[ 5] << 24);
            ((gctUINT32_PTR) trgLine)[ 2] = 0xFF | (src[6] << 8) | (src[ 7] << 16) | (src[ 8] << 24);
            ((gctUINT32_PTR) trgLine)[ 3] = 0xFF | (src[9] << 8) | (src[10] << 16) | (src[11] << 24);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = 0xFF | (src[0] << 8) | (src[ 1] << 16) | (src[ 2] << 24);
            ((gctUINT32_PTR) trgLine)[ 5] = 0xFF | (src[3] << 8) | (src[ 4] << 16) | (src[ 5] << 24);
            ((gctUINT32_PTR) trgLine)[ 6] = 0xFF | (src[6] << 8) | (src[ 7] << 16) | (src[ 8] << 24);
            ((gctUINT32_PTR) trgLine)[ 7] = 0xFF | (src[9] << 8) | (src[10] << 16) | (src[11] << 24);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = 0xFF | (src[0] << 8) | (src[ 1] << 16) | (src[ 2] << 24);
            ((gctUINT32_PTR) trgLine)[ 9] = 0xFF | (src[3] << 8) | (src[ 4] << 16) | (src[ 5] << 24);
            ((gctUINT32_PTR) trgLine)[10] = 0xFF | (src[6] << 8) | (src[ 7] << 16) | (src[ 8] << 24);
            ((gctUINT32_PTR) trgLine)[11] = 0xFF | (src[9] << 8) | (src[10] << 16) | (src[11] << 24);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = 0xFF | (src[0] << 8) | (src[ 1] << 16) | (src[ 2] << 24);
            ((gctUINT32_PTR) trgLine)[13] = 0xFF | (src[3] << 8) | (src[ 4] << 16) | (src[ 5] << 24);
            ((gctUINT32_PTR) trgLine)[14] = 0xFF | (src[6] << 8) | (src[ 7] << 16) | (src[ 8] << 24);
            ((gctUINT32_PTR) trgLine)[15] = 0xFF | (src[9] << 8) | (src[10] << 16) | (src[11] << 24);
        }
    }
}
#endif

static void
_UploadSuperTiledABGRtoARGB(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT32 x, y;
    gctUINT32 xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;
    gctUINT i, j;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 4);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

            /* ABGR to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = (srcLine[0] << 16) | (srcLine[1] << 8) | srcLine[2] | (srcLine[3] << 24);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];
                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* ABGR to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[ 0] << 16) | (srcLine[ 1] << 8) | srcLine[ 2] | (srcLine[ 3] << 24);
                ((gctUINT32_PTR) trgLine)[1] = (srcLine[ 4] << 16) | (srcLine[ 5] << 8) | srcLine[ 6] | (srcLine[ 7] << 24);
                ((gctUINT32_PTR) trgLine)[2] = (srcLine[ 8] << 16) | (srcLine[ 9] << 8) | srcLine[10] | (srcLine[11] << 24);
                ((gctUINT32_PTR) trgLine)[3] = (srcLine[12] << 16) | (srcLine[13] << 8) | srcLine[14] | (srcLine[15] << 24);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* ABGR to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[ 0] << 16) | (srcLine[ 1] << 8) | srcLine[ 2] | (srcLine[ 3] << 24);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

            /* ABGR to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = (src[ 0] << 16) | (src[ 1] << 8) | src[ 2] | (src[ 3] << 24);
            ((gctUINT32_PTR) trgLine)[ 1] = (src[ 4] << 16) | (src[ 5] << 8) | src[ 6] | (src[ 7] << 24);
            ((gctUINT32_PTR) trgLine)[ 2] = (src[ 8] << 16) | (src[ 9] << 8) | src[10] | (src[11] << 24);
            ((gctUINT32_PTR) trgLine)[ 3] = (src[12] << 16) | (src[13] << 8) | src[14] | (src[15] << 24);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = (src[ 0] << 16) | (src[ 1] << 8) | src[ 2] | (src[ 3] << 24);
            ((gctUINT32_PTR) trgLine)[ 5] = (src[ 4] << 16) | (src[ 5] << 8) | src[ 6] | (src[ 7] << 24);
            ((gctUINT32_PTR) trgLine)[ 6] = (src[ 8] << 16) | (src[ 9] << 8) | src[10] | (src[11] << 24);
            ((gctUINT32_PTR) trgLine)[ 7] = (src[12] << 16) | (src[13] << 8) | src[14] | (src[15] << 24);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = (src[ 0] << 16) | (src[ 1] << 8) | src[ 2] | (src[ 3] << 24);
            ((gctUINT32_PTR) trgLine)[ 9] = (src[ 4] << 16) | (src[ 5] << 8) | src[ 6] | (src[ 7] << 24);
            ((gctUINT32_PTR) trgLine)[10] = (src[ 8] << 16) | (src[ 9] << 8) | src[10] | (src[11] << 24);
            ((gctUINT32_PTR) trgLine)[11] = (src[12] << 16) | (src[13] << 8) | src[14] | (src[15] << 24);

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = (src[ 0] << 16) | (src[ 1] << 8) | src[ 2] | (src[ 3] << 24);
            ((gctUINT32_PTR) trgLine)[13] = (src[ 4] << 16) | (src[ 5] << 8) | src[ 6] | (src[ 7] << 24);
            ((gctUINT32_PTR) trgLine)[14] = (src[ 8] << 16) | (src[ 9] << 8) | src[10] | (src[11] << 24);
            ((gctUINT32_PTR) trgLine)[15] = (src[12] << 16) | (src[13] << 8) | src[14] | (src[15] << 24);
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadSuperTiledABGRtoARGBBE(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctSIZE_T x, y;
    gctUINT i, j;
    gctSIZE_T xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 4);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

            /* ABGR to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = (srcLine[0] << 8) | (srcLine[1] << 16) | (srcLine[2] << 24) | srcLine[3];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* ABGR to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[ 0] << 8) | (srcLine[ 1] << 16) | (srcLine[ 2] << 24) | srcLine[ 3];
                ((gctUINT32_PTR) trgLine)[1] = (srcLine[ 4] << 8) | (srcLine[ 5] << 16) | (srcLine[ 6] << 24) | srcLine[ 7];
                ((gctUINT32_PTR) trgLine)[2] = (srcLine[ 8] << 8) | (srcLine[ 9] << 16) | (srcLine[10] << 24) | srcLine[11];
                ((gctUINT32_PTR) trgLine)[3] = (srcLine[12] << 8) | (srcLine[13] << 16) | (srcLine[14] << 24) | srcLine[15];
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* ABGR to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[ 0] << 8) | (srcLine[ 1] << 16) | (srcLine[ 2] << 24) | srcLine[ 3];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR src;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

            /* ABGR to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = (src[ 0] << 8) | (src[ 1] << 16) | (src[ 2] << 24) | src[ 3];
            ((gctUINT32_PTR) trgLine)[ 1] = (src[ 4] << 8) | (src[ 5] << 16) | (src[ 6] << 24) | src[ 7];
            ((gctUINT32_PTR) trgLine)[ 2] = (src[ 8] << 8) | (src[ 9] << 16) | (src[10] << 24) | src[11];
            ((gctUINT32_PTR) trgLine)[ 3] = (src[12] << 8) | (src[13] << 16) | (src[14] << 24) | src[15];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = (src[ 0] << 8) | (src[ 1] << 16) | (src[ 2] << 24) | src[ 3];
            ((gctUINT32_PTR) trgLine)[ 5] = (src[ 4] << 8) | (src[ 5] << 16) | (src[ 6] << 24) | src[ 7];
            ((gctUINT32_PTR) trgLine)[ 6] = (src[ 8] << 8) | (src[ 9] << 16) | (src[10] << 24) | src[11];
            ((gctUINT32_PTR) trgLine)[ 7] = (src[12] << 8) | (src[13] << 16) | (src[14] << 24) | src[15];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = (src[ 0] << 8) | (src[ 1] << 16) | (src[ 2] << 24) | src[ 3];
            ((gctUINT32_PTR) trgLine)[ 9] = (src[ 4] << 8) | (src[ 5] << 16) | (src[ 6] << 24) | src[ 7];
            ((gctUINT32_PTR) trgLine)[10] = (src[ 8] << 8) | (src[ 9] << 16) | (src[10] << 24) | src[11];
            ((gctUINT32_PTR) trgLine)[11] = (src[12] << 8) | (src[13] << 16) | (src[14] << 24) | src[15];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = (src[ 0] << 8) | (src[ 1] << 16) | (src[ 2] << 24) | src[ 3];
            ((gctUINT32_PTR) trgLine)[13] = (src[ 4] << 8) | (src[ 5] << 16) | (src[ 6] << 24) | src[ 7];
            ((gctUINT32_PTR) trgLine)[14] = (src[ 8] << 8) | (src[ 9] << 16) | (src[10] << 24) | src[11];
            ((gctUINT32_PTR) trgLine)[15] = (src[12] << 8) | (src[13] << 16) | (src[14] << 24) | src[15];
        }
    }
}
#endif
#endif

static gceSTATUS
_UploadSuperTiledABGR32ToABGR32_2Layer(
    IN gcoSURF TexSurf,
    IN gctUINT8_PTR Target,
    IN gctUINT XOffset,
    IN gctUINT YOffset,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT8_PTR Source,
    IN gctINT SourceStride,
    IN gceSURF_FORMAT SourceFormat
    )
{
    gctUINT x, y;
    gctUINT8_PTR target2, source;
    gctUINT32 offset = 0;

    target2 = Target + TexSurf->layerSize;

    for (y = YOffset; y < Height + YOffset; y++)
    {
        source = Source + (y - YOffset) * SourceStride;

        for (x = XOffset; x < Width + XOffset; x++)
        {
            gcoHARDWARE_ComputeOffset(gcvNULL,
                                      x,
                                      y,
                                      TexSurf->stride,
                                      8,
                                      TexSurf->tiling,
                                      &offset);

            /* Copy the GR pixel to layer 0. */
            gcoOS_MemCopy(Target + offset, source, 8);

            source += 8;

            /* Copy the AB pixel to layer 1. */
            gcoOS_MemCopy(target2 + offset, source, 8);

            source += 8;
        }
    }

    return gcvSTATUS_OK;
}

static gceSTATUS
_UploadSuperTiledBGR32ToABGR32_2Layer(
    IN gcoSURF TexSurf,
    IN gctUINT8_PTR Target,
    IN gctUINT XOffset,
    IN gctUINT YOffset,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT8_PTR Source,
    IN gctINT SourceStride,
    IN gceSURF_FORMAT SourceFormat
    )
{
    gctUINT x, y;
    gctUINT8_PTR target2, source;
    gctUINT32 offset = 0;

    /* Note: Not assigning anything to alpha channel,
             as not needed. If needed, it needs to be set
             based on target format info type. */

    target2 = Target + TexSurf->layerSize;

    for (y = YOffset; y < Height + YOffset; y++)
    {
        source = Source + (y - YOffset) * SourceStride;

        for (x = XOffset; x < Width + XOffset; x++)
        {
            gcoHARDWARE_ComputeOffset(gcvNULL,
                                      x,
                                      y,
                                      TexSurf->stride,
                                      8,
                                      TexSurf->tiling,
                                      &offset);

            /* Copy the GR pixel to layer 0. */
            gcoOS_MemCopy(Target + offset, source, 8);

            source += 8;

            /* Copy the B pixel to layer 1 as XB. */
            gcoOS_MemCopy(target2 + offset, source, 4);

            source += 4;
        }
    }

    return gcvSTATUS_OK;
}

#if !defined(UNDER_CE)
static void
_UploadSuperTiled8bppto8bpp(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 1);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* 8 bpp to 8 bpp. */
            trgLine[0] = srcLine[0];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* 8 bpp to 8 bpp: one tile row. */
                if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned source. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR)srcLine)[0];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = srcLine[0] | (srcLine[1] << 8) | (srcLine[2] << 16) | (srcLine[3] << 24);
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* 8 bpp to 8 bpp: part tile row. */
                trgLine[0] = srcLine[0];
            }
        }
    }

    /* Y middle - X middle. */
    if ((((gctUINTPTR_T) Memory & 3) == 0) && ((SourceStride & 3) == 0))
    {
        for (y = Y; y < Bottom; y += 4)
        {
            for (x = X; x < Right; x += 4)
            {
                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* Special optimization for aligned source. */
                ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) (srcLine + SourceStride * 0))[0];
                ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) (srcLine + SourceStride * 1))[0];
                ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) (srcLine + SourceStride * 2))[0];
                ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) (srcLine + SourceStride * 3))[0];
            }
        }
    }
    else
    {
        for (y = Y; y < Bottom; y += 4)
        {
            for (x = X; x < Right; x += 4)
            {
                gctUINT8_PTR src;

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* 8 bpp to 8 bpp: one tile. */
                src  = srcLine;
                ((gctUINT32_PTR) trgLine)[0] = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[1] = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[2] = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[3] = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
            }
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadSuperTiled8bppto8bppBE(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 1);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* 8 bpp to 8 bpp. */
            trgLine[0] = srcLine[0];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* 8 bpp to 8 bpp: one tile row. */
                if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned source. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR)srcLine)[0];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = (srcLine[0] << 24) | (srcLine[1] << 16) | (srcLine[2] << 8) | srcLine[3];
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* 8 bpp to 8 bpp: part tile row. */
                trgLine[0] = srcLine[0];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            if ((((gctUINTPTR_T)srcLine & 3) == 0) && ((SourceStride & 3) == 0))
            {
                /* Special optimization for aligned source. */
                /* 8 bpp to 8 bpp: one tile. */
                ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) (srcLine + SourceStride * 0))[0];
                ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) (srcLine + SourceStride * 1))[0];
                ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) (srcLine + SourceStride * 2))[0];
                ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) (srcLine + SourceStride * 3))[0];
            }
            else
            {
                gctUINT8_PTR src;

                /* 8 bpp to 8 bpp: one tile. */
                src  = srcLine;
                ((gctUINT32_PTR) trgLine)[0] = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[1] = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[2] = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[3] = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
            }
        }
    }
}
#endif

static void
_UploadSuperTiled16bppto16bpp(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* 16 bpp to 16 bpp. */
            ((gctUINT16_PTR) trgLine)[0] = srcLine[0];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* 16 bpp to 16 bpp: one tile row. */
                if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned case. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) srcLine)[0];
                    ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) srcLine)[1];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = srcLine[0] | (srcLine[1] << 16);
                    ((gctUINT32_PTR) trgLine)[1] = srcLine[2] | (srcLine[3] << 16);
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* 16 bpp to 16 bpp: part tile row. */
                ((gctUINT16_PTR) trgLine)[0] = srcLine[0];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            if ((((gctUINTPTR_T)srcLine & 3) == 0) && ((SourceStride & 3) == 0))
            {
                /* Special optimization for aligned source. */
                gctUINT16_PTR src;

                /* 16 bpp to 16 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) src)[1];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) src)[1];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[4] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[5] = ((gctUINT32_PTR) src)[1];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[6] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[7] = ((gctUINT32_PTR) src)[1];
            }
            else
            {
                gctUINT16_PTR src;

                /* 16 bpp to 16 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[0] = src[0] | (src[1] << 16);
                ((gctUINT32_PTR) trgLine)[1] = src[2] | (src[3] << 16);

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[2] = src[0] | (src[1] << 16);
                ((gctUINT32_PTR) trgLine)[3] = src[2] | (src[3] << 16);

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[4] = src[0] | (src[1] << 16);
                ((gctUINT32_PTR) trgLine)[5] = src[2] | (src[3] << 16);

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[6] = src[0] | (src[1] << 16);
                ((gctUINT32_PTR) trgLine)[7] = src[2] | (src[3] << 16);
            }
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadSuperTiled16bppto16bppBE(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

            /* 16 bpp to 16 bpp. */
            ((gctUINT16_PTR) trgLine)[0] = ((gctUINT16_PTR) srcLine)[0];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

                /* 16 bpp to 16 bpp: one tile row. */
                if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned case. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) srcLine)[0];
                    ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) srcLine)[1];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = (srcLine[0] << 24) | (srcLine[1] << 16) | (srcLine[2] << 8) | srcLine[3];
                    ((gctUINT32_PTR) trgLine)[1] = (srcLine[4] << 24) | (srcLine[5] << 16) | (srcLine[6] << 8) | srcLine[7];
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

                /* 16 bpp to 16 bpp: part tile row. */
                ((gctUINT16_PTR) trgLine)[0] = ((gctUINT16_PTR) srcLine)[0];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + X  * 2;

            if ((((gctUINTPTR_T)srcLine & 3) == 0) && ((SourceStride & 3) == 0))
            {
                /* Special optimization for aligned source. */
                gctUINT16_PTR src;

                /* 16 bpp to 16 bpp: one tile. */
                src = (gctUINT16_PTR) srcLine;
                ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) src)[1];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) src)[1];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[4] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[5] = ((gctUINT32_PTR) src)[1];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[6] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[7] = ((gctUINT32_PTR) src)[1];
            }
            else
            {
                gctUINT8_PTR src;

                /* 16 bpp to 16 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[0] = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
                ((gctUINT32_PTR) trgLine)[1] = (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[2] = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
                ((gctUINT32_PTR) trgLine)[3] = (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[4] = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
                ((gctUINT32_PTR) trgLine)[5] = (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[6] = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
                ((gctUINT32_PTR) trgLine)[7] = (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];
            }
        }
    }
}
#endif

static void
_UploadSuperTiled32bppto32bpp(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 4);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

            /* 32bpp to 32bpp. */
            ((gctUINT32_PTR) trgLine)[0] = srcLine[0] | (srcLine[1] << 8) | (srcLine[2] << 16) | (srcLine[3] << 24);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* 32 bpp to 32 bpp: one tile row. */
                if ((((gctUINTPTR_T)srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned source. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) srcLine)[0];
                    ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) srcLine)[1];
                    ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) srcLine)[2];
                    ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) srcLine)[3];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = srcLine[ 0] | (srcLine[ 1] << 8) | (srcLine[ 2] << 16) | (srcLine[ 3] << 24);
                    ((gctUINT32_PTR) trgLine)[1] = srcLine[ 4] | (srcLine[ 5] << 8) | (srcLine[ 6] << 16) | (srcLine[ 7] << 24);
                    ((gctUINT32_PTR) trgLine)[2] = srcLine[ 8] | (srcLine[ 9] << 8) | (srcLine[10] << 16) | (srcLine[11] << 24);
                    ((gctUINT32_PTR) trgLine)[3] = srcLine[12] | (srcLine[13] << 8) | (srcLine[14] << 16) | (srcLine[15] << 24);
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* 32 bpp to 32 bpp: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = srcLine[ 0] | (srcLine[ 1] << 8) | (srcLine[ 2] << 16) | (srcLine[ 3] << 24);
            }
        }
    }


    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

            if ((((gctUINTPTR_T)srcLine & 3) == 0) && ((SourceStride & 3) == 0))
            {
                /* Special optimization for aligned source. */
                gctUINT8_PTR src;

                /* 32 bpp to 32 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[ 0] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 1] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[ 2] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[ 3] = ((gctUINT32_PTR) src)[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 4] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 5] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[ 6] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[ 7] = ((gctUINT32_PTR) src)[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 8] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 9] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[10] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[11] = ((gctUINT32_PTR) src)[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[12] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[13] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[14] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[15] = ((gctUINT32_PTR) src)[3];
            }
            else
            {
                gctUINT8_PTR src;

                /* 32 bpp to 32 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[ 0] = src[ 0] | (src[ 1] << 8) | (src[ 2] << 16) | (src[ 3] << 24);
                ((gctUINT32_PTR) trgLine)[ 1] = src[ 4] | (src[ 5] << 8) | (src[ 6] << 16) | (src[ 7] << 24);
                ((gctUINT32_PTR) trgLine)[ 2] = src[ 8] | (src[ 9] << 8) | (src[10] << 16) | (src[11] << 24);
                ((gctUINT32_PTR) trgLine)[ 3] = src[12] | (src[13] << 8) | (src[14] << 16) | (src[15] << 24);

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 4] = src[ 0] | (src[ 1] << 8) | (src[ 2] << 16) | (src[ 3] << 24);
                ((gctUINT32_PTR) trgLine)[ 5] = src[ 4] | (src[ 5] << 8) | (src[ 6] << 16) | (src[ 7] << 24);
                ((gctUINT32_PTR) trgLine)[ 6] = src[ 8] | (src[ 9] << 8) | (src[10] << 16) | (src[11] << 24);
                ((gctUINT32_PTR) trgLine)[ 7] = src[12] | (src[13] << 8) | (src[14] << 16) | (src[15] << 24);

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 8] = src[ 0] | (src[ 1] << 8) | (src[ 2] << 16) | (src[ 3] << 24);
                ((gctUINT32_PTR) trgLine)[ 9] = src[ 4] | (src[ 5] << 8) | (src[ 6] << 16) | (src[ 7] << 24);
                ((gctUINT32_PTR) trgLine)[10] = src[ 8] | (src[ 9] << 8) | (src[10] << 16) | (src[11] << 24);
                ((gctUINT32_PTR) trgLine)[11] = src[12] | (src[13] << 8) | (src[14] << 16) | (src[15] << 24);

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[12] = src[ 0] | (src[ 1] << 8) | (src[ 2] << 16) | (src[ 3] << 24);
                ((gctUINT32_PTR) trgLine)[13] = src[ 4] | (src[ 5] << 8) | (src[ 6] << 16) | (src[ 7] << 24);
                ((gctUINT32_PTR) trgLine)[14] = src[ 8] | (src[ 9] << 8) | (src[10] << 16) | (src[11] << 24);
                ((gctUINT32_PTR) trgLine)[15] = src[12] | (src[13] << 8) | (src[14] << 16) | (src[15] << 24);
            }
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadSuperTiled32bppto32bppBE(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 4);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

            /* 32bpp to 32bpp. */
            ((gctUINT32_PTR) trgLine)[0] = (srcLine[0] << 24) | (srcLine[1] << 16) | (srcLine[2] << 8) | srcLine[3];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* 32 bpp to 32 bpp: one tile row. */
                if ((((gctUINTPTR_T) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned source. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) srcLine)[0];
                    ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) srcLine)[1];
                    ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) srcLine)[2];
                    ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) srcLine)[3];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = (srcLine[ 0] << 24) | (srcLine[ 1] << 16) | (srcLine[ 2] << 8) | srcLine[ 3];
                    ((gctUINT32_PTR) trgLine)[1] = (srcLine[ 4] << 24) | (srcLine[ 5] << 16) | (srcLine[ 6] << 8) | srcLine[ 7];
                    ((gctUINT32_PTR) trgLine)[2] = (srcLine[ 8] << 24) | (srcLine[ 9] << 16) | (srcLine[10] << 8) | srcLine[11];
                    ((gctUINT32_PTR) trgLine)[3] = (srcLine[12] << 24) | (srcLine[13] << 16) | (srcLine[14] << 8) | srcLine[15];
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* 32 bpp to 32 bpp: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[ 0] << 24) | (srcLine[ 1] << 16) | (srcLine[ 2] << 8) | srcLine[ 3];
            }
        }
    }


    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

            if ((((gctUINTPTR_T)srcLine & 3) == 0) && ((SourceStride & 3) == 0))
            {
                /* Special optimization for aligned source. */
                gctUINT8_PTR src;

                /* 32 bpp to 32 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[ 0] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 1] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[ 2] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[ 3] = ((gctUINT32_PTR) src)[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 4] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 5] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[ 6] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[ 7] = ((gctUINT32_PTR) src)[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 8] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 9] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[10] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[11] = ((gctUINT32_PTR) src)[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[12] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[13] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[14] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[15] = ((gctUINT32_PTR) src)[3];
            }
            else
            {
                gctUINT8_PTR src;

                /* 32 bpp to 32 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[ 0] = (src[ 0] << 24) | (src[ 1] << 16) | (src[ 2] << 8) | src[ 3];
                ((gctUINT32_PTR) trgLine)[ 1] = (src[ 4] << 24) | (src[ 5] << 16) | (src[ 6] << 8) | src[ 7];
                ((gctUINT32_PTR) trgLine)[ 2] = (src[ 8] << 24) | (src[ 9] << 16) | (src[10] << 8) | src[11];
                ((gctUINT32_PTR) trgLine)[ 3] = (src[12] << 24) | (src[13] << 16) | (src[14] << 8) | src[15];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 4] = (src[ 0] << 24) | (src[ 1] << 16) | (src[ 2] << 8) | src[ 3];
                ((gctUINT32_PTR) trgLine)[ 5] = (src[ 4] << 24) | (src[ 5] << 16) | (src[ 6] << 8) | src[ 7];
                ((gctUINT32_PTR) trgLine)[ 6] = (src[ 8] << 24) | (src[ 9] << 16) | (src[10] << 8) | src[11];
                ((gctUINT32_PTR) trgLine)[ 7] = (src[12] << 24) | (src[13] << 16) | (src[14] << 8) | src[15];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 8] = (src[ 0] << 24) | (src[ 1] << 16) | (src[ 2] << 8) | src[ 3];
                ((gctUINT32_PTR) trgLine)[ 9] = (src[ 4] << 24) | (src[ 5] << 16) | (src[ 6] << 8) | src[ 7];
                ((gctUINT32_PTR) trgLine)[10] = (src[ 8] << 24) | (src[ 9] << 16) | (src[10] << 8) | src[11];
                ((gctUINT32_PTR) trgLine)[11] = (src[12] << 24) | (src[13] << 16) | (src[14] << 8) | src[15];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[12] = (src[ 0] << 24) | (src[ 1] << 16) | (src[ 2] << 8) | src[ 3];
                ((gctUINT32_PTR) trgLine)[13] = (src[ 4] << 24) | (src[ 5] << 16) | (src[ 6] << 8) | src[ 7];
                ((gctUINT32_PTR) trgLine)[14] = (src[ 8] << 24) | (src[ 9] << 16) | (src[10] << 8) | src[11];
                ((gctUINT32_PTR) trgLine)[15] = (src[12] << 24) | (src[13] << 16) | (src[14] << 8) | src[15];
            }
        }
    }
}
#endif

static void
_UploadSuperTiledRGBA4444toARGB4444(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA4444 to ARGB4444. */
            ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 4) | ((srcLine[0] & 0xF) << 12);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB4444: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[0] >> 4) | ((srcLine[0] & 0xF) << 12) | ((srcLine[1] & 0xFFF0) << 12) | ((srcLine[1] & 0xF) << 28) ;
                ((gctUINT32_PTR) trgLine)[1] = (srcLine[2] >> 4) | ((srcLine[2] & 0xF) << 12) | ((srcLine[3] & 0xFFF0) << 12) | ((srcLine[3] & 0xF) << 28) ;
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB4444: part tile row. */
                ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 4) | ((srcLine[0] & 0xF) << 12);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA4444 to ARGB4444: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[0] = (src[0] >> 4) | ((src[0] & 0xF) << 12) | ((src[1] & 0xFFF0) << 12) | ((src[1] & 0xF) << 28) ;
            ((gctUINT32_PTR) trgLine)[1] = (src[2] >> 4) | ((src[2] & 0xF) << 12) | ((src[3] & 0xFFF0) << 12) | ((src[3] & 0xF) << 28) ;

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[2] = (src[0] >> 4) | ((src[0] & 0xF) << 12) | ((src[1] & 0xFFF0) << 12) | ((src[1] & 0xF) << 28) ;
            ((gctUINT32_PTR) trgLine)[3] = (src[2] >> 4) | ((src[2] & 0xF) << 12) | ((src[3] & 0xFFF0) << 12) | ((src[3] & 0xF) << 28) ;

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[4] = (src[0] >> 4) | ((src[0] & 0xF) << 12) | ((src[1] & 0xFFF0) << 12) | ((src[1] & 0xF) << 28) ;
            ((gctUINT32_PTR) trgLine)[5] = (src[2] >> 4) | ((src[2] & 0xF) << 12) | ((src[3] & 0xFFF0) << 12) | ((src[3] & 0xF) << 28) ;

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[6] = (src[0] >> 4) | ((src[0] & 0xF) << 12) | ((src[1] & 0xFFF0) << 12) | ((src[1] & 0xF) << 28) ;
            ((gctUINT32_PTR) trgLine)[7] = (src[2] >> 4) | ((src[2] & 0xF) << 12) | ((src[3] & 0xFFF0) << 12) | ((src[3] & 0xF) << 28) ;
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadSuperTiledRGBA4444toARGB4444BE(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA4444 to ARGB4444. */
            ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 4) | ((srcLine[0] & 0xF) << 12);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB4444: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = ((srcLine[0] & 0xFFF0) << 12) | ((srcLine[0] & 0xF) << 28) | (srcLine[1] >> 4) | ((srcLine[1] & 0xF) << 12) ;
                ((gctUINT32_PTR) trgLine)[1] = ((srcLine[2] & 0xFFF0) << 12) | ((srcLine[2] & 0xF) << 28) | (srcLine[3] >> 4) | ((srcLine[3] & 0xF) << 12) ;
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB4444: part tile row. */
                ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 4) | ((srcLine[0] & 0xF) << 12);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA4444 to ARGB4444: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[0] = ((src[0] & 0xFFF0) << 12) | ((src[0] & 0xF) << 28) | (src[1] >> 4) | ((src[1] & 0xF) << 12) ;
            ((gctUINT32_PTR) trgLine)[1] = ((src[2] & 0xFFF0) << 12) | ((src[2] & 0xF) << 28) | (src[3] >> 4) | ((src[3] & 0xF) << 12) ;

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[2] = ((src[0] & 0xFFF0) << 12) | ((src[0] & 0xF) << 28) | (src[1] >> 4) | ((src[1] & 0xF) << 12) ;
            ((gctUINT32_PTR) trgLine)[3] = ((src[2] & 0xFFF0) << 12) | ((src[2] & 0xF) << 28) | (src[3] >> 4) | ((src[3] & 0xF) << 12) ;

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[4] = ((src[0] & 0xFFF0) << 12) | ((src[0] & 0xF) << 28) | (src[1] >> 4) | ((src[1] & 0xF) << 12) ;
            ((gctUINT32_PTR) trgLine)[5] = ((src[2] & 0xFFF0) << 12) | ((src[2] & 0xF) << 28) | (src[3] >> 4) | ((src[3] & 0xF) << 12) ;

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[6] = ((src[0] & 0xFFF0) << 12) | ((src[0] & 0xF) << 28) | (src[1] >> 4) | ((src[1] & 0xF) << 12) ;
            ((gctUINT32_PTR) trgLine)[7] = ((src[2] & 0xFFF0) << 12) | ((src[2] & 0xF) << 28) | (src[3] >> 4) | ((src[3] & 0xF) << 12) ;
        }
    }
}
#endif

static void
_UploadSuperTiledRGBA5551toARGB1555(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
             xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA5551 to ARGB1555. */
            ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 1) | ((srcLine[0] & 0x1) << 15);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB1555: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[0] >> 1) | ((srcLine[0] & 0x1) << 15) | ((srcLine[1] & 0xFFFE) << 15) | ((srcLine[1] & 0x1) << 31);
                ((gctUINT32_PTR) trgLine)[1] = (srcLine[2] >> 1) | ((srcLine[2] & 0x1) << 15) | ((srcLine[3] & 0xFFFE) << 15) | ((srcLine[3] & 0x1) << 31) ;
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB1555: part tile row. */
                ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 1) | ((srcLine[0] & 0x1) << 15);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA5551 to ARGB1555: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[0] = (src[0] >> 1) | ((src[0] & 0x1) << 15) | ((src[1] & 0xFFFE) << 15) | ((src[1] & 0x1) << 31);
            ((gctUINT32_PTR) trgLine)[1] = (src[2] >> 1) | ((src[2] & 0x1) << 15) | ((src[3] & 0xFFFE) << 15) | ((src[3] & 0x1) << 31);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[2] = (src[0] >> 1) | ((src[0] & 0x1) << 15) | ((src[1] & 0xFFFE) << 15) | ((src[1] & 0x1) << 31);
            ((gctUINT32_PTR) trgLine)[3] = (src[2] >> 1) | ((src[2] & 0x1) << 15) | ((src[3] & 0xFFFE) << 15) | ((src[3] & 0x1) << 31);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[4] = (src[0] >> 1) | ((src[0] & 0x1) << 15) | ((src[1] & 0xFFFE) << 15) | ((src[1] & 0x1) << 31);
            ((gctUINT32_PTR) trgLine)[5] = (src[2] >> 1) | ((src[2] & 0x1) << 15) | ((src[3] & 0xFFFE) << 15) | ((src[3] & 0x1) << 31);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[6] = (src[0] >> 1) | ((src[0] & 0x1) << 15) | ((src[1] & 0xFFFE) << 15) | ((src[1] & 0x1) << 31);
            ((gctUINT32_PTR) trgLine)[7] = (src[2] >> 1) | ((src[2] & 0x1) << 15) | ((src[3] & 0xFFFE) << 15) | ((src[3] & 0x1) << 31);
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadSuperTiledRGBA5551toARGB1555BE(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;
    xt = 0;
    yt = 0;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
             xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA5551 to ARGB1555. */
            ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 1) | ((srcLine[0] & 0x1) << 15);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB1555: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = ((srcLine[0] & 0xFFFE) << 15) | ((srcLine[0] & 0x1) << 31) | (srcLine[1] >> 1) | ((srcLine[1] & 0x1) << 15);
                ((gctUINT32_PTR) trgLine)[1] = ((srcLine[2] & 0xFFFE) << 15) | ((srcLine[2] & 0x1) << 31) | (srcLine[3] >> 1) | ((srcLine[3] & 0x1) << 15);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
                yt = gcmSUPERTILE_OFFSET_Y(x, y);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB1555: part tile row. */
            ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 1) | ((srcLine[0] & 0x1) << 15);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        for (x = X; x < Right; x += 4)
        {
            gctUINT16_PTR src;

            xt = gcmSUPERTILE_OFFSET_X(x, y, Hardware->config->superTileMode);
            yt = gcmSUPERTILE_OFFSET_Y(x, y);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA5551 to ARGB1555: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[0] = ((src[0] & 0xFFFE) << 15) | ((src[0] & 0x1) << 31) | (src[1] >> 1) | ((src[1] & 0x1) << 15);
            ((gctUINT32_PTR) trgLine)[1] = ((src[2] & 0xFFFE) << 15) | ((src[2] & 0x1) << 31) | (src[3] >> 1) | ((src[3] & 0x1) << 15);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[2] = ((src[0] & 0xFFFE) << 15) | ((src[0] & 0x1) << 31) | (src[1] >> 1) | ((src[1] & 0x1) << 15);
            ((gctUINT32_PTR) trgLine)[3] = ((src[2] & 0xFFFE) << 15) | ((src[2] & 0x1) << 31) | (src[3] >> 1) | ((src[3] & 0x1) << 15);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[4] = ((src[0] & 0xFFFE) << 15) | ((src[0] & 0x1) << 31) | (src[1] >> 1) | ((src[1] & 0x1) << 15);
            ((gctUINT32_PTR) trgLine)[5] = ((src[2] & 0xFFFE) << 15) | ((src[2] & 0x1) << 31) | (src[3] >> 1) | ((src[3] & 0x1) << 15);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[6] = ((src[0] & 0xFFFE) << 15) | ((src[0] & 0x1) << 31) | (src[1] >> 1) | ((src[1] & 0x1) << 15);
            ((gctUINT32_PTR) trgLine)[7] = ((src[2] & 0xFFFE) << 15) | ((src[2] & 0x1) << 31) | (src[3] >> 1) | ((src[3] & 0x1) << 15);
        }
    }
}
#endif
#endif

static gceSTATUS
_UploadTextureSuperTiled(
    IN gcoSURF TexSurf,
    IN gctUINT32 Offset,
    IN gctUINT XOffset,
    IN gctUINT YOffset,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride,
    IN gceSURF_FORMAT SourceFormat
    )
{
    gctUINT8_PTR target                   = gcvNULL;
    gctUINT8_PTR source                   = gcvNULL;
    gctUINT32 offset                      = 0;
    gctUINT32 srcBytesPerPixel            = 0;
    gctUINT32 trgBytesPerPixel            = 0;
    gctUINT x                             = 0;
    gctUINT y                             = 0;
    gceSTATUS status;
    gcsSURF_FORMAT_INFO_PTR srcFormatInfo = gcvNULL;

    gcoHARDWARE Hardware                  = gcvNULL;
    gctUINT trgStride                     = 0;
    gctPOINTER trgLogical                 = gcvNULL;
    gctUINT edgeX[6];
    gctUINT edgeY[6];
    gctUINT countX = 0;
    gctUINT countY = 0;
    gctUINT right  = XOffset + Width;
    gctUINT bottom = YOffset + Height;

    gcmGETHARDWARE(Hardware);

    target = TexSurf->node.logical + Offset;

    trgStride  = TexSurf->stride;
    trgLogical = TexSurf->node.logical;

    /* Compute dest logical. */
    trgLogical = (gctPOINTER) ((gctUINT8_PTR) trgLogical + Offset);

    /* Compute edge coordinates. */
    if (Width < 4)
    {
        for (x = XOffset; x < right; x++)
        {
           edgeX[countX++] = x;
        }
    }
    else
    {
        for (x = XOffset; x < ((XOffset + 3) & ~3); x++)
        {
            edgeX[countX++] = x;
        }

        for (x = (right & ~3); x < right; x++)
        {
            edgeX[countX++] = x;
        }
    }

    if (Height < 4)
    {
       for (y = YOffset; y < bottom; y++)
       {
           edgeY[countY++] = y;
       }
    }
    else
    {
        for (y = YOffset; y < ((YOffset + 3) & ~3); y++)
        {
           edgeY[countY++] = y;
        }

        for (y = (bottom & ~3); y < bottom; y++)
        {
           edgeY[countY++] = y;
        }
    }

#if !defined(UNDER_CE)
    /* Fast path(s) for all common OpenGL ES formats. */
    if ((TexSurf->format == gcvSURF_A8R8G8B8)
    ||  (TexSurf->format == gcvSURF_X8R8G8B8)
    )
    {
        switch (SourceFormat)
        {
        case gcvSURF_A8:
#if gcdENDIAN_BIG
            _UploadSuperTiledA8toARGBBE(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width
                                  : SourceStride);
#else
            _UploadSuperTiledA8toARGB(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width
                                  : SourceStride);
#endif
            return gcvSTATUS_OK;

        case gcvSURF_B8G8R8:
#if gcdENDIAN_BIG
            _UploadSuperTiledBGRtoARGBBE(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 3
                                  : SourceStride);
#else
            _UploadSuperTiledBGRtoARGB(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 3
                                  : SourceStride);
#endif
            return gcvSTATUS_OK;

        case gcvSURF_A8B8G8R8:
#if gcdENDIAN_BIG
            _UploadSuperTiledABGRtoARGBBE(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 4
                                  : SourceStride);
#else
            _UploadSuperTiledABGRtoARGB(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 3
                                  : SourceStride);
#endif
            return gcvSTATUS_OK;

        case gcvSURF_L8:
#if gcdENDIAN_BIG
            _UploadSuperTiledL8toARGBBE(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width
                                  : SourceStride);
#else
            _UploadSuperTiledL8toARGB(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width
                                  : SourceStride);
#endif
            return gcvSTATUS_OK;

        case gcvSURF_A8L8:
#if gcdENDIAN_BIG
            _UploadSuperTiledA8L8toARGBBE(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);
#else
            _UploadSuperTiledA8L8toARGB(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 3
                                  : SourceStride);
#endif
            return gcvSTATUS_OK;

        case gcvSURF_A8R8G8B8:
#if gcdENDIAN_BIG
            _UploadSuperTiled32bppto32bppBE(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 4
                                  : SourceStride);
#else
            _UploadSuperTiled32bppto32bpp(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 4
                                  : SourceStride);
#endif
            return gcvSTATUS_OK;

        case gcvSURF_R5G6B5:
#if gcdENDIAN_BIG
            _UploadSuperTiledRGB565toARGBBE(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);
#else
            _UploadSuperTiledRGB565toARGB(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);
#endif
            return gcvSTATUS_OK;

        case gcvSURF_R4G4B4A4:
#if gcdENDIAN_BIG
             _UploadSuperTiledRGBA4444toARGBBE(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);
#else
             _UploadSuperTiledRGBA4444toARGB(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);
#endif
            return gcvSTATUS_OK;

        case gcvSURF_R5G5B5A1:
#if gcdENDIAN_BIG
            _UploadSuperTiledRGBA5551toARGBBE(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);
#else
            _UploadSuperTiledRGBA5551toARGB(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);
#endif
            return gcvSTATUS_OK;

        default:
            break;
        }
    }

    else
    {
        switch (SourceFormat)
        {
        case gcvSURF_A8:
            if (TexSurf->format == gcvSURF_A8)
            {
#if gcdENDIAN_BIG
                _UploadSuperTiled8bppto8bppBE(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width
                                  : SourceStride);
#else
                _UploadSuperTiled8bppto8bpp(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width
                                  : SourceStride);
#endif
                return gcvSTATUS_OK;
            }
            break;

        case gcvSURF_B8G8R8:
            if (TexSurf->format == gcvSURF_X8R8G8B8)
            {
               /* Same as BGR to ARGB. */
#if gcdENDIAN_BIG
                _UploadSuperTiledBGRtoARGBBE(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 3
                                  : SourceStride);
#else
                _UploadSuperTiledBGRtoARGB(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 3
                                  : SourceStride);
#endif
                return gcvSTATUS_OK;
            }
            else if (TexSurf->format == gcvSURF_X8B8G8R8)
            {
#if gcdENDIAN_BIG
                _UploadSuperTiledBGRtoABGRBE(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 3
                                  : SourceStride);
#else
                _UploadSuperTiledBGRtoABGR(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 3
                                  : SourceStride);
#endif
                return gcvSTATUS_OK;
            }

            break;

        /* case gcvSURF_A8B8G8R8: */

        case gcvSURF_L8:
            if (TexSurf->format == gcvSURF_L8)
            {
#if gcdENDIAN_BIG
                _UploadSuperTiled8bppto8bppBE(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width
                                  : SourceStride);
#else
                _UploadSuperTiled8bppto8bpp(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width
                                  : SourceStride);
#endif
                return gcvSTATUS_OK;
            }
            break;

        case gcvSURF_A8L8:
            if (TexSurf->format == gcvSURF_A8L8)
            {
#if gcdENDIAN_BIG
                _UploadSuperTiled16bppto16bppBE(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);
#else
                _UploadSuperTiled16bppto16bpp(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);
#endif
                return gcvSTATUS_OK;
            }
            break;

        /* case gcvSURF_A8R8G8B8: */

        case gcvSURF_R5G6B5:
            if (TexSurf->format == gcvSURF_R5G6B5)
            {
#if gcdENDIAN_BIG
                _UploadSuperTiled16bppto16bppBE(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);
#else
                _UploadSuperTiled16bppto16bpp(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);
#endif
                return gcvSTATUS_OK;
            }
            break;

        case gcvSURF_R4G4B4A4:
            if (TexSurf->format == gcvSURF_A4R4G4B4)
            {
#if gcdENDIAN_BIG
                _UploadSuperTiledRGBA4444toARGB4444BE(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);
#else
                _UploadSuperTiledRGBA4444toARGB4444(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);
#endif
                return gcvSTATUS_OK;
            }
            break;

        case gcvSURF_R5G5B5A1:
            if (TexSurf->format == gcvSURF_A1R5G5B5)
            {
#if gcdENDIAN_BIG
                _UploadSuperTiledRGBA5551toARGB1555BE(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);
#else
                _UploadSuperTiledRGBA5551toARGB1555(Hardware,
                                  trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  right, bottom,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);
#endif
                return gcvSTATUS_OK;
            }
            break;

        default:
            break;
        }
    }

    /* More fast path(s) for same source/dest format. */
    if (SourceFormat == TexSurf->format)
    {
        gcsSURF_FORMAT_INFO_PTR formatInfo;
        gcmONERROR(gcoHARDWARE_QueryFormat(SourceFormat, &formatInfo));

        if ((formatInfo->fmtClass == gcvFORMAT_CLASS_RGBA) &&
            (formatInfo->layers == 1))
        {
            switch (formatInfo->bitsPerPixel)
            {
            case 8:
#if gcdENDIAN_BIG
                _UploadSuperTiled8bppto8bppBE(Hardware,
                                              trgLogical,
                                              trgStride,
                                              XOffset, YOffset,
                                              right, bottom,
                                              edgeX, edgeY,
                                              countX, countY,
                                              Memory,
                                              (SourceStride == 0)
                                              ? (gctINT) Width
                                              : SourceStride);
#else
                _UploadSuperTiled8bppto8bpp(Hardware,
                                            trgLogical,
                                            trgStride,
                                            XOffset, YOffset,
                                            right, bottom,
                                            edgeX, edgeY,
                                            countX, countY,
                                            Memory,
                                            (SourceStride == 0)
                                            ? (gctINT) Width
                                            : SourceStride);
#endif
                return gcvSTATUS_OK;

            case 16:
#if gcdENDIAN_BIG
                _UploadSuperTiled16bppto16bppBE(Hardware,
                                                trgLogical,
                                                trgStride,
                                                XOffset, YOffset,
                                                right, bottom,
                                                edgeX, edgeY,
                                                countX, countY,
                                                Memory,
                                                (SourceStride == 0)
                                                ? (gctINT) Width * 2
                                                : SourceStride);
#else
                _UploadSuperTiled16bppto16bpp(Hardware,
                                              trgLogical,
                                              trgStride,
                                              XOffset, YOffset,
                                              right, bottom,
                                              edgeX, edgeY,
                                              countX, countY,
                                              Memory,
                                              (SourceStride == 0)
                                              ? (gctINT) Width * 2
                                              : SourceStride);
#endif
                return gcvSTATUS_OK;

            case 32:
#if gcdENDIAN_BIG
                _UploadSuperTiled32bppto32bppBE(Hardware,
                                                trgLogical,
                                                trgStride,
                                                XOffset, YOffset,
                                                right, bottom,
                                                edgeX, edgeY,
                                                countX, countY,
                                                Memory,
                                                (SourceStride == 0)
                                                ? (gctINT) Width * 4
                                                : SourceStride);
#else
                _UploadSuperTiled32bppto32bpp(Hardware,
                                              trgLogical,
                                              trgStride,
                                              XOffset, YOffset,
                                              right, bottom,
                                              edgeX, edgeY,
                                              countX, countY,
                                              Memory,
                                              (SourceStride == 0)
                                              ? (gctINT) Width * 4
                                              : SourceStride);
#endif
                return gcvSTATUS_OK;

            case 64:
            default:
                break;
            }
        }
    }

#endif
    /* Get number of bytes per pixel and tile for the format. */
    gcmONERROR(gcoHARDWARE_ConvertFormat(SourceFormat,
                                         &srcBytesPerPixel,
                                         gcvNULL));
    srcBytesPerPixel /= 8;

    trgBytesPerPixel = TexSurf->bitsPerPixel / 8;

    /* Auto-stride. */
    if (SourceStride == 0)
    {
        SourceStride = srcBytesPerPixel * Width;
    }

    /* Query format specifics. */
    gcmONERROR(gcoHARDWARE_QueryFormat(SourceFormat, &srcFormatInfo));

    if (TexSurf->formatInfo.layers > 1)
    {
        if ((TexSurf->formatInfo.layers == 2)
            && (srcBytesPerPixel == 12) && (trgBytesPerPixel == 16))
        {
            _UploadSuperTiledBGR32ToABGR32_2Layer(TexSurf,
                                                   target,
                                                   XOffset,
                                                   YOffset,
                                                   Width,
                                                   Height,
                                                   (gctUINT8_PTR)Memory,
                                                   SourceStride,
                                                   SourceFormat);
        }
        else if ((TexSurf->formatInfo.layers == 2)
            && (srcBytesPerPixel == 16) && (trgBytesPerPixel == 16))
        {
            _UploadSuperTiledABGR32ToABGR32_2Layer(TexSurf,
                                                   target,
                                                   XOffset,
                                                   YOffset,
                                                   Width,
                                                   Height,
                                                   (gctUINT8_PTR)Memory,
                                                   SourceStride,
                                                   SourceFormat);
        }
        else
        {
            gcmASSERT(0);
        }
    }
    else
    {
        for (y = YOffset; y < Height + YOffset; y++)
        {
            source = (gctUINT8_PTR)Memory + (y - YOffset) * SourceStride;

            for (x = XOffset; x < Width + XOffset; x++)
            {
                gcoHARDWARE_ComputeOffset(gcvNULL,
                                          x,
                                          y,
                                          TexSurf->stride,
                                          trgBytesPerPixel,
                                          TexSurf->tiling,
                                          &offset);

                /* Copy the pixel. */
                /* Convert the current pixel. */
                gcmONERROR(gcoHARDWARE_ConvertPixel(source,
                                                    (target + offset),
                                                    0, 0,
                                                    srcFormatInfo,
                                                    &TexSurf->formatInfo,
                                                    gcvNULL, gcvNULL,
                                                    0, 0));

                source += srcBytesPerPixel;
            }
        }
    }

    return gcvSTATUS_OK;
OnError:
    return status;
}

/* linear texture. */

static void
_UploadLinearA8toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT8_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT8_PTR) Memory + y * SourceStride;
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            *trg++ = (gctUINT32) (*src++) << 24;
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadLinearA8toARGBBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT8_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT8_PTR) Memory + y * SourceStride;
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            *trg++ = (gctUINT32) (*src++);
        }
    }
}
#endif

static void
_UploadLinearBGRtoARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT8_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT8_PTR) Memory + y * SourceStride;
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            *trg++ = 0xFF000000 | (src[0] << 16) | (src[1] << 8) | src[2];
            src += 3;
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadLinearBGRtoARGBBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT8_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT8_PTR) Memory + y * SourceStride;
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            *trg++ = 0xFF | (src[0] << 8) | (src[1] << 16) | src[2] << 24;
            src += 3;
        }
    }
}
#endif

static void
_UploadLinearBGRtoABGR(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT8_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT8_PTR) Memory + y * SourceStride;
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            *trg++ = 0xFF000000 | src[0] | (src[1] << 8) | (src[2] << 16);
            src += 3;
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadLinearBGRtoABGRBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT8_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT8_PTR) Memory + y * SourceStride;
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            *trg++ = 0xFF | (src[0] << 24) | (src[1] << 16) | (src[2] << 8);
            src += 3;
        }
    }
}
#endif

static void
_UploadLinearABGRtoARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT8_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT8_PTR) Memory + y * SourceStride;
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            *trg++ = (src[0] << 16) | (src[1] << 8) | (src[2]) | (src[3] << 24);
            src += 4;
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadLinearABGRtoARGBBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT8_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT8_PTR) Memory + y * SourceStride;
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            *trg++ = (src[0] << 8) | (src[1] << 16) | (src[2] << 24) | (src[3]);
            src += 4;
        }
    }
}
#endif

static void
_UploadLinearL8toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT8_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT8_PTR) Memory + y * SourceStride;
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            register gctUINT32 u = *src++;

            *trg++ = L8toARGB(u);
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadLinearL8toARGBBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT8_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT8_PTR) Memory + y * SourceStride;
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            register gctUINT32 u = *src++;

            *trg++ = L8toARGBBE(u);
        }
    }
}
#endif

static void
_UploadLinearA8L8toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT8_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT8_PTR) Memory + y * SourceStride;
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            register gctUINT32 u = *src++;

            u |= (u << 8) | (u << 16) | (*src++ << 24);
            *trg++ = u;
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadLinearA8L8toARGBBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT8_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT8_PTR) Memory + y * SourceStride;
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            register gctUINT32 u = *src++;

            u |= (u << 8);
            u |= (u << 16) | *src++;
            *trg++ = u;
        }
    }
}
#endif

static void
_UploadLinear32bppto32bpp(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT y;
    gctUINT8_PTR src;
    gctUINT8_PTR trg;

    src = (gctUINT8_PTR) Memory;
    trg = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        gcoOS_MemCopy(trg, src, Width * 4);

        /* Advance to next line. */
        src += SourceStride;
        trg += TargetStride;
    }
}

static void
_UploadLinear64bppto64bpp(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT y;
    gctUINT8_PTR src;
    gctUINT8_PTR trg;

    src = (gctUINT8_PTR) Memory;
    trg = (gctUINT8_PTR) Logical + Y * TargetStride + X * 8;

    for (y = 0; y < Height; y++)
    {
        gcoOS_MemCopy(trg, src, Width * 4);

        /* Advance to next line. */
        src += SourceStride;
        trg += TargetStride;
    }
}

static void
_UploadLinearRGB565toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT16_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT16_PTR) ((gctUINT8_PTR) Memory + y * SourceStride);
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            register gctUINT32 u = (gctUINT32) (*src++);
            *trg++ = RGB565toARGB(u);
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadLinearRGB565toARGBBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT16_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT16_PTR) ((gctUINT8_PTR) Memory + y * SourceStride);
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            register gctUINT32 u = (gctUINT32) (*src++);
            *trg++ = RGB565toARGBBE(u);
        }
    }
}
#endif

static void
_UploadLinearRGBA4444toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT16_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT16_PTR) ((gctUINT8_PTR) Memory + y * SourceStride);
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            register gctUINT32 u = (gctUINT32) (*src++);
            *trg++ = RGBA4444toARGB(u);
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadLinearRGBA4444toARGBBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT16_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT16_PTR) ((gctUINT8_PTR) Memory + y * SourceStride);
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            register gctUINT32 u = (gctUINT32) (*src++);
            *trg++ = RGBA4444toARGBBE(u);
        }
    }
}
#endif

static void
_UploadLinearRGBA5551toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT16_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT16_PTR) ((gctUINT8_PTR) Memory + y * SourceStride);
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            register gctUINT32 u = (gctUINT32) (*src++);
            *trg++ = RGBA5551toARGB(u);
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadLinearRGBA5551toARGBBE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT16_PTR src;
    gctUINT32_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT16_PTR) ((gctUINT8_PTR) Memory + y * SourceStride);
        trg = (gctUINT32_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            register gctUINT32 u = (gctUINT32) (*src++);
            *trg++ = RGBA5551toARGBBE(u);
        }
    }
}
#endif

static void
_UploadLinear8bppto8bpp(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT y;
    gctUINT8_PTR src;
    gctUINT8_PTR trg;

    src = (gctUINT8_PTR) Memory;
    trg = (gctUINT8_PTR) Logical + Y * TargetStride + X;

    for (y = 0; y < Height; y++)
    {
        gcoOS_MemCopy(trg, src, Width);

        /* Advance to next line. */
        src += SourceStride;
        trg += TargetStride;
    }
}

static void
_UploadLinear16bppto16bpp(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT y;
    gctUINT8_PTR src;
    gctUINT8_PTR trg;

    src = (gctUINT8_PTR) Memory;
    trg = (gctUINT8_PTR) Logical + Y * TargetStride + X * 2;

    for (y = 0; y < Height; y++)
    {
        gcoOS_MemCopy(trg, src, Width * 2);

        /* Advance to next line. */
        src += SourceStride;
        trg += TargetStride;
    }
}

static void
_UploadLinearRGBA4444toARGB4444(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT16_PTR src;
    gctUINT16_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 2;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT16_PTR) ((gctUINT8_PTR) Memory + y * SourceStride);
        trg = (gctUINT16_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            register gctUINT16 u = (gctUINT16) (*src++);
            *trg++ = (u >> 4) | ((u & 0xF) << 12);
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadLinearRGBA4444toARGB4444BE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT16_PTR src;
    gctUINT16_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 2;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT16_PTR) ((gctUINT8_PTR) Memory + y * SourceStride);
        trg = (gctUINT16_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            register gctUINT16 u = (gctUINT16) (*src++);
            *trg++ = (u >> 4) | ((u & 0xF) << 12);
        }
    }
}
#endif

static void
_UploadLinearRGBA5551toARGB1555(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT16_PTR src;
    gctUINT16_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 2;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT16_PTR) ((gctUINT8_PTR) Memory + y * SourceStride);
        trg = (gctUINT16_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            register gctUINT16 u = (gctUINT16) (*src++);
            *trg++ = (u >> 1) | ((u & 0x1) << 15);
        }
    }
}

#if gcdENDIAN_BIG
static void
_UploadLinearRGBA5551toARGB1555BE(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT16_PTR src;
    gctUINT16_PTR trg;

    Logical = (gctUINT8_PTR) Logical + Y * TargetStride + X * 4;

    for (y = 0; y < Height; y++)
    {
        /* Point to line start. */
        src = (gctUINT16_PTR) ((gctUINT8_PTR) Memory + y * SourceStride);
        trg = (gctUINT16_PTR) ((gctUINT8_PTR) Logical + y * TargetStride);

        for (x = 0; x < Width; x++)
        {
            register gctUINT16 u = (gctUINT16) (*src++);
            *trg++ = (u >> 1) | ((u & 0x1) << 15);
        }
    }
}
#endif

static gceSTATUS
_UploadTextureLinear(
    IN gcoSURF TexSurf,
    IN gctUINT32 Offset,
    IN gctUINT XOffset,
    IN gctUINT YOffset,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride,
    IN gceSURF_FORMAT SourceFormat
    )
{
    gctUINT8_PTR target                   = gcvNULL;
    gctUINT8_PTR source                   = gcvNULL;
    gctUINT x;
    gctUINT y;
    gctUINT32 srcBytesPerPixel            = 0;
    gctUINT32 trgBytesPerPixel            = 0;
    gceSTATUS status;
    gcsSURF_FORMAT_INFO_PTR srcFormatInfo = gcvNULL;

    gcoHARDWARE Hardware                  = gcvNULL;
    gctUINT trgStride                     = 0;
    gctPOINTER trgLogical                 = gcvNULL;

    gcmGETHARDWARE(Hardware);

    target = TexSurf->node.logical + Offset;

    trgStride  = TexSurf->stride;
    trgLogical = TexSurf->node.logical;

    /* Compute dest logical. */
    trgLogical = (gctPOINTER) ((gctUINT8_PTR) trgLogical + Offset);

    /* Fast path(s) for all common OpenGL ES formats. */
    if ((TexSurf->format == gcvSURF_A8R8G8B8)
    ||  (TexSurf->format == gcvSURF_X8R8G8B8)
    )
    {
        switch (SourceFormat)
        {
        case gcvSURF_A8:
#if gcdENDIAN_BIG
            _UploadLinearA8toARGBBE(trgLogical,
                                    trgStride,
                                    XOffset, YOffset,
                                    Width, Height,
                                    Memory,
                                    (SourceStride == 0)
                                    ? (gctINT) Width
                                    : SourceStride);
#else
            _UploadLinearA8toARGB(trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  Width, Height,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width
                                  : SourceStride);
#endif
            return gcvSTATUS_OK;

        case gcvSURF_B8G8R8:
#if gcdENDIAN_BIG
            _UploadLinearBGRtoARGBBE(trgLogical,
                                    trgStride,
                                    XOffset, YOffset,
                                    Width, Height,
                                    Memory,
                                    (SourceStride == 0)
                                    ? (gctINT) Width * 3
                                    : SourceStride);
#else
            _UploadLinearBGRtoARGB(trgLogical,
                                   trgStride,
                                   XOffset, YOffset,
                                   Width, Height,
                                   Memory,
                                   (SourceStride == 0)
                                   ? (gctINT) Width * 3
                                   : SourceStride);
#endif
            return gcvSTATUS_OK;

        case gcvSURF_A8B8G8R8:
#if gcdENDIAN_BIG
            _UploadLinearABGRtoARGBBE(trgLogical,
                                      trgStride,
                                      XOffset, YOffset,
                                      Width, Height,
                                      Memory,
                                      (SourceStride == 0)
                                      ? (gctINT) Width * 4
                                      : SourceStride);
#else
            _UploadLinearABGRtoARGB(trgLogical,
                                    trgStride,
                                    XOffset, YOffset,
                                    Width, Height,
                                    Memory,
                                    (SourceStride == 0)
                                    ? (gctINT) Width * 3
                                    : SourceStride);
#endif
            return gcvSTATUS_OK;

        case gcvSURF_L8:
#if gcdENDIAN_BIG
            _UploadLinearL8toARGBBE(trgLogical,
                                    trgStride,
                                    XOffset, YOffset,
                                    Width, Height,
                                    Memory,
                                    (SourceStride == 0)
                                    ? (gctINT) Width
                                    : SourceStride);
#else
            _UploadLinearL8toARGB(trgLogical,
                                  trgStride,
                                  XOffset, YOffset,
                                  Width, Height,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width
                                  : SourceStride);
#endif
            return gcvSTATUS_OK;

        case gcvSURF_A8L8:
#if gcdENDIAN_BIG
            _UploadLinearA8L8toARGBBE(trgLogical,
                                      trgStride,
                                      XOffset, YOffset,
                                      Width, Height,
                                      Memory,
                                      (SourceStride == 0)
                                      ? (gctINT) Width * 2
                                      : SourceStride);
#else
            _UploadLinearA8L8toARGB(trgLogical,
                                    trgStride,
                                    XOffset, YOffset,
                                    Width, Height,
                                    Memory,
                                    (SourceStride == 0)
                                    ? (gctINT) Width * 3
                                    : SourceStride);
#endif
            return gcvSTATUS_OK;

        case gcvSURF_A8R8G8B8:
            _UploadLinear32bppto32bpp(trgLogical,
                                      trgStride,
                                      XOffset, YOffset,
                                      Width, Height,
                                      Memory,
                                      (SourceStride == 0)
                                      ? (gctINT) Width * 4
                                      : SourceStride);
            return gcvSTATUS_OK;

        case gcvSURF_R5G6B5:
#if gcdENDIAN_BIG
            _UploadLinearRGB565toARGBBE(trgLogical,
                                       trgStride,
                                       XOffset, YOffset,
                                       Width, Height,
                                       Memory,
                                       (SourceStride == 0)
                                       ? (gctINT) Width * 2
                                       : SourceStride);
#else
            _UploadLinearRGB565toARGB(trgLogical,
                                      trgStride,
                                      XOffset, YOffset,
                                      Width, Height,
                                      Memory,
                                      (SourceStride == 0)
                                      ? (gctINT) Width * 2
                                      : SourceStride);
#endif
            return gcvSTATUS_OK;

        case gcvSURF_R4G4B4A4:
#if gcdENDIAN_BIG
             _UploadLinearRGBA4444toARGBBE(trgLogical,
                                           trgStride,
                                           XOffset, YOffset,
                                           Width, Height,
                                           Memory,
                                           (SourceStride == 0)
                                           ? (gctINT) Width * 2
                                           : SourceStride);
#else
             _UploadLinearRGBA4444toARGB(trgLogical,
                                         trgStride,
                                         XOffset, YOffset,
                                         Width, Height,
                                         Memory,
                                         (SourceStride == 0)
                                         ? (gctINT) Width * 2
                                         : SourceStride);
#endif
            return gcvSTATUS_OK;

        case gcvSURF_R5G5B5A1:
#if gcdENDIAN_BIG
            _UploadLinearRGBA5551toARGBBE(trgLogical,
                                          trgStride,
                                          XOffset, YOffset,
                                          Width, Height,
                                          Memory,
                                          (SourceStride == 0)
                                          ? (gctINT) Width * 2
                                          : SourceStride);
#else
            _UploadLinearRGBA5551toARGB(trgLogical,
                                        trgStride,
                                        XOffset, YOffset,
                                        Width, Height,
                                        Memory,
                                        (SourceStride == 0)
                                        ? (gctINT) Width * 2
                                        : SourceStride);
#endif
            return gcvSTATUS_OK;

        default:
            break;
        }
    }

    else
    {
        switch (SourceFormat)
        {
        case gcvSURF_A8:
            if (TexSurf->format == gcvSURF_A8)
            {
                _UploadLinear8bppto8bpp(trgLogical,
                                        trgStride,
                                        XOffset, YOffset,
                                        Width, Height,
                                        Memory,
                                        (SourceStride == 0)
                                        ? (gctINT) Width
                                        : SourceStride);
                return gcvSTATUS_OK;
            }
            break;

        case gcvSURF_B8G8R8:
            if (TexSurf->format == gcvSURF_X8R8G8B8)
            {
               /* Same as BGR to ARGB. */
#if gcdENDIAN_BIG
                _UploadLinearBGRtoARGBBE(trgLogical,
                                         trgStride,
                                         XOffset, YOffset,
                                         Width, Height,
                                         Memory,
                                         (SourceStride == 0)
                                         ? (gctINT) Width * 3
                                         : SourceStride);
#else
                _UploadLinearBGRtoARGB(trgLogical,
                                       trgStride,
                                       XOffset, YOffset,
                                       Width, Height,
                                       Memory,
                                       (SourceStride == 0)
                                       ? (gctINT) Width * 3
                                       : SourceStride);
#endif
                return gcvSTATUS_OK;
            }
            else if (TexSurf->format == gcvSURF_X8B8G8R8)
            {
#if gcdENDIAN_BIG
                _UploadLinearBGRtoABGRBE(trgLogical,
                                         trgStride,
                                         XOffset, YOffset,
                                         Width, Height,
                                         Memory,
                                         (SourceStride == 0)
                                         ? (gctINT) Width * 3
                                         : SourceStride);
#else
                _UploadLinearBGRtoABGR(trgLogical,
                                       trgStride,
                                       XOffset, YOffset,
                                       Width, Height,
                                       Memory,
                                       (SourceStride == 0)
                                       ? (gctINT) Width * 3
                                       : SourceStride);
#endif
                return gcvSTATUS_OK;
            }

            break;

        /* case gcvSURF_A8B8G8R8: */

        case gcvSURF_L8:
            if (TexSurf->format == gcvSURF_L8)
            {
                _UploadLinear8bppto8bpp(trgLogical,
                                        trgStride,
                                        XOffset, YOffset,
                                        Width, Height,
                                        Memory,
                                        (SourceStride == 0)
                                        ? (gctINT) Width
                                        : SourceStride);
                return gcvSTATUS_OK;
            }
            break;

        case gcvSURF_A8L8:
            if (TexSurf->format == gcvSURF_A8L8)
            {
                _UploadLinear16bppto16bpp(trgLogical,
                                          trgStride,
                                          XOffset, YOffset,
                                          Width, Height,
                                          Memory,
                                          (SourceStride == 0)
                                          ? (gctINT) Width * 2
                                          : SourceStride);
                return gcvSTATUS_OK;
            }
            break;

        /* case gcvSURF_A8R8G8B8: */

        case gcvSURF_R5G6B5:
            if (TexSurf->format == gcvSURF_R5G6B5)
            {
                _UploadLinear16bppto16bpp(trgLogical,
                                          trgStride,
                                          XOffset, YOffset,
                                          Width, Height,
                                          Memory,
                                          (SourceStride == 0)
                                          ? (gctINT) Width * 2
                                          : SourceStride);
                return gcvSTATUS_OK;
            }
            break;

        case gcvSURF_R4G4B4A4:
            if (TexSurf->format == gcvSURF_A4R4G4B4)
            {
#if gcdENDIAN_BIG
                _UploadLinearRGBA4444toARGB4444BE(trgLogical,
                                                  trgStride,
                                                  XOffset, YOffset,
                                                  Width, Height,
                                                  Memory,
                                                  (SourceStride == 0)
                                                  ? (gctINT) Width * 2
                                                  : SourceStride);
#else
                _UploadLinearRGBA4444toARGB4444(trgLogical,
                                                trgStride,
                                                XOffset, YOffset,
                                                Width, Height,
                                                Memory,
                                                (SourceStride == 0)
                                                ? (gctINT) Width * 2
                                                : SourceStride);
#endif
                return gcvSTATUS_OK;
            }
            break;

        case gcvSURF_R5G5B5A1:
            if (TexSurf->format == gcvSURF_A1R5G5B5)
            {
#if gcdENDIAN_BIG
                _UploadLinearRGBA5551toARGB1555BE(trgLogical,
                                                  trgStride,
                                                  XOffset, YOffset,
                                                  Width, Height,
                                                  Memory,
                                                  (SourceStride == 0)
                                                  ? (gctINT) Width * 2
                                                  : SourceStride);
#else
                _UploadLinearRGBA5551toARGB1555(trgLogical,
                                                trgStride,
                                                XOffset, YOffset,
                                                Width, Height,
                                                Memory,
                                                (SourceStride == 0)
                                                ? (gctINT) Width * 2
                                                : SourceStride);
#endif
                return gcvSTATUS_OK;
            }
            break;

        default:
            break;
        }
    }

    /* More fast path(s) for same source/dest format. */
    if (SourceFormat == TexSurf->format)
    {
        gcsSURF_FORMAT_INFO_PTR formatInfo;
        gcmONERROR(gcoHARDWARE_QueryFormat(SourceFormat, &formatInfo));

        if ((formatInfo->fmtClass == gcvFORMAT_CLASS_RGBA) &&
            (formatInfo->layers == 1))
        {
            switch (formatInfo->bitsPerPixel)
            {
            case 8:
                _UploadLinear8bppto8bpp(trgLogical,
                                        trgStride,
                                        XOffset, YOffset,
                                        Width, Height,
                                        Memory,
                                        (SourceStride == 0)
                                        ? (gctINT) Width
                                        : SourceStride);
                return gcvSTATUS_OK;

            case 16:
                _UploadLinear16bppto16bpp(trgLogical,
                                          trgStride,
                                          XOffset, YOffset,
                                          Width, Height,
                                          Memory,
                                          (SourceStride == 0)
                                          ? (gctINT) Width * 2
                                          : SourceStride);
                return gcvSTATUS_OK;

            case 32:
                _UploadLinear32bppto32bpp(trgLogical,
                                          trgStride,
                                          XOffset, YOffset,
                                          Width, Height,
                                          Memory,
                                          (SourceStride == 0)
                                          ? (gctINT) Width * 4
                                          : SourceStride);
                return gcvSTATUS_OK;

            case 64:
                _UploadLinear64bppto64bpp(trgLogical,
                                          trgStride,
                                          XOffset, YOffset,
                                          Width, Height,
                                          Memory,
                                          (SourceStride == 0)
                                          ? (gctINT) Width * 8
                                          : SourceStride);

                return gcvSTATUS_OK;
            default:
                /* Invalid format. */
                gcmASSERT(gcvFALSE);
                break;
            }
        }
    }

    /* Get number of bytes per pixel and tile for the format. */
    gcmONERROR(gcoHARDWARE_ConvertFormat(SourceFormat,
                                         &srcBytesPerPixel,
                                         gcvNULL));
    srcBytesPerPixel /= 8;

    trgBytesPerPixel = TexSurf->bitsPerPixel / 8;

    /* Auto-stride. */
    if (SourceStride == 0)
    {
        SourceStride = srcBytesPerPixel * Width;
    }

    /* Query format specifics. */
    gcmONERROR(gcoHARDWARE_QueryFormat(SourceFormat, &srcFormatInfo));

    gcmASSERT(TexSurf->formatInfo.layers == 1);

    for (y = YOffset; y < Height + YOffset; y++)
    {
        source = (gctUINT8_PTR) Memory + (y - YOffset) * SourceStride;
        target = (gctUINT8_PTR) trgLogical + y * trgStride + XOffset * trgBytesPerPixel;

        for (x = 0; x < Width; x++)
        {
            /* Convert the current pixel. */
            gcmONERROR(gcoHARDWARE_ConvertPixel(source,
                                                target,
                                                0, 0,
                                                srcFormatInfo,
                                                &TexSurf->formatInfo,
                                                gcvNULL, gcvNULL,
                                                0, 0));

            source += srcBytesPerPixel;
            target += trgBytesPerPixel;
        }
    }

    return gcvSTATUS_OK;
OnError:
    return status;
}


/*******************************************************************************
**
**  gcoHARDWARE_UploadTexture
**
**  Upload one slice into a texture.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gceSURF_FORMAT TargetFormat
**          Destination texture format.
**
**      gctUINT32 Address
**          Hardware specific base address for mip map.
**
**      gctPOINTER Logical
**          Logical base address for mip map.
**
**      gctUINT32 Offset
**          Offset into mip map to upload.
**
**      gctINT TargetStride
**          Stride of the destination texture.
**
**      gctUINT X
**          X origin of data to upload.
**
**      gctUINT Y
**          Y origin of data to upload.
**
**      gctUINT Width
**          Width of texture to upload.
**
**      gctUINT Height
**          Height of texture to upload.
**
**      gctCONST_POINTER Memory
**          Pointer to user buffer containing the data to upload.
**
**      gctINT SourceStride
**          Stride of user buffer.
**
**      gceSURF_FORMAT SourceFormat
**          Format of the source texture to upload.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_UploadTexture(
    IN gcsSURF_VIEW *TexView,
    IN gctUINT32 Offset,
    IN gctUINT XOffset,
    IN gctUINT YOffset,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride,
    IN gceSURF_FORMAT SourceFormat
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcoSURF TexSurf = TexView->surf;

    gcmHEADER_ARG("TexView=%x  Offset=%u "
                  "X=%u Y=%u Width=%u Height=%u "
                  "Memory=0x%x SourceStride=%d SourceFormat=%d",
                  TexView, Offset, XOffset, YOffset, Width, Height,
                  Memory, SourceStride, SourceFormat);

    /* No multiSlice support when tile status enabled for now.*/
    gcmASSERT(TexView->numSlices == 1);

    if (!TexSurf->tileStatusDisabled[TexView->firstSlice] &&
         TexSurf->tileStatusNode.pool != gcvPOOL_UNKNOWN)
    {
        /* Disable tile status buffer in case it's there */
        gcmONERROR(gcoHARDWARE_DisableTileStatus(gcvNULL, TexView, gcvTRUE));
        gcmONERROR(gcoHARDWARE_FlushPipe(gcvNULL, gcvNULL));
        gcmONERROR(gcoHARDWARE_Commit(gcvNULL));
        gcmONERROR(gcoHARDWARE_Stall(gcvNULL));
    }

    if (TexSurf->superTiled && ((SourceFormat & gcvSURF_FORMAT_OCL) == 0))
    {
        gcmONERROR(_UploadTextureSuperTiled(TexSurf, Offset, XOffset, YOffset,
                                            Width, Height, Memory, SourceStride, SourceFormat));
    }
    else if (TexSurf->tiling == gcvLINEAR)
    {
        gcmONERROR(_UploadTextureLinear(TexSurf, Offset, XOffset, YOffset,
                                        Width, Height, Memory, SourceStride, SourceFormat));
    }
    else
    {
        gcmONERROR(_UploadTextureTiled(gcvNULL, TexSurf, Offset, XOffset, YOffset,
                                       Width, Height, Memory, SourceStride, SourceFormat));
    }

    /* If dst surface was fully overwritten, reset the garbagePadded flag. */
    if (gcmIS_SUCCESS(status) && TexSurf->paddingFormat &&
        XOffset == 0 && Width  >= TexSurf->requestW &&
        YOffset == 0 && Height >= TexSurf->requestH)
    {
        TexSurf->garbagePadded = gcvFALSE;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static void
_UploadNV16toYUY2(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctPOINTER Memory[3],
    IN gctINT SourceStride[3]
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT yStride, uvStride;
    gctUINT8_PTR yPlane  = gcvNULL;
    gctUINT8_PTR uvPlane = gcvNULL;
    gctUINT8_PTR yLine   = gcvNULL;
    gctUINT8_PTR uvLine  = gcvNULL;
    gctUINT8_PTR trgLine = gcvNULL;

    if (SourceStride != 0)
    {
        yStride  = SourceStride[0];
        uvStride = SourceStride[1];
    }
    else
    {
        yStride  = Right - X;
        uvStride = Right - X;
    }

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Move linear Memory to [0,0] origin. */
    yPlane  = (gctUINT8_PTR) Memory[0] -  yStride * Y - X * 1;
    uvPlane = (gctUINT8_PTR) Memory[1] - uvStride * Y - X * 1;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            /* Skip odd x. */
            if (x & 1) continue;

            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            yLine   = yPlane  + y * yStride  + x * 1;
            uvLine  = uvPlane + y * uvStride + x * 1;

            /* NV16 to YUY2: a package - 2 pixels. */
            ((gctUINT32_PTR) trgLine)[0] =  (yLine[0]) | (uvLine[0] << 8) | (yLine[1] << 16) | (uvLine[1] << 24);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                yLine   = yPlane  + y * yStride  + x * 1;
                uvLine  = uvPlane + y * uvStride + x * 1;

                /* NV16 to YUY2: one tile row - 2 packages - 4 pixels. */
                ((gctUINT32_PTR) trgLine)[0] =  (yLine[0]) | (uvLine[0] << 8) | (yLine[1] << 16) | (uvLine[1] << 24);
                ((gctUINT32_PTR) trgLine)[1] =  (yLine[2]) | (uvLine[2] << 8) | (yLine[3] << 16) | (uvLine[3] << 24);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];

                /* Skip odd x. */
                if (x & 1) continue;

                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                yLine   = yPlane  + y * yStride  + x * 1;
                uvLine  = uvPlane + y * uvStride + x * 1;

                ((gctUINT32_PTR) trgLine)[0] =  (yLine[0]) | (uvLine[0] << 8) | (yLine[1] << 16) | (uvLine[1] << 24);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
        yLine   = yPlane  + y * yStride  + X * 1;
        uvLine  = uvPlane + y * uvStride + X * 1;

        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR srcY, srcUV;

            /* NV16 to YUY2: one block - 4 pixels. */
            srcY  = yLine;
            srcUV = uvLine;

            ((gctUINT32_PTR) trgLine)[0] =  (srcY[0]) | (srcUV[0] << 8) | (srcY[1] << 16) | (srcUV[1] << 24);
            ((gctUINT32_PTR) trgLine)[1] =  (srcY[2]) | (srcUV[2] << 8) | (srcY[3] << 16) | (srcUV[3] << 24);

            srcY  += yStride;
            srcUV += uvStride;
            ((gctUINT32_PTR) trgLine)[2] =  (srcY[0]) | (srcUV[0] << 8) | (srcY[1] << 16) | (srcUV[1] << 24);
            ((gctUINT32_PTR) trgLine)[3] =  (srcY[2]) | (srcUV[2] << 8) | (srcY[3] << 16) | (srcUV[3] << 24);

            srcY  += yStride;
            srcUV += uvStride;
            ((gctUINT32_PTR) trgLine)[4] =  (srcY[0]) | (srcUV[0] << 8) | (srcY[1] << 16) | (srcUV[1] << 24);
            ((gctUINT32_PTR) trgLine)[5] =  (srcY[2]) | (srcUV[2] << 8) | (srcY[3] << 16) | (srcUV[3] << 24);

            srcY  += yStride;
            srcUV += uvStride;
            ((gctUINT32_PTR) trgLine)[6] =  (srcY[0]) | (srcUV[0] << 8) | (srcY[1] << 16) | (srcUV[1] << 24);
            ((gctUINT32_PTR) trgLine)[7] =  (srcY[2]) | (srcUV[2] << 8) | (srcY[3] << 16) | (srcUV[3] << 24);

            /* Move to next tile. */
            trgLine += 16 * 2;
            yLine   +=  4 * 1;
            uvLine  +=  4 * 1;
        }
    }
}

static void
_UploadNV61toYUY2(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Right,
    IN gctUINT Bottom,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctPOINTER Memory[3],
    IN gctINT SourceStride[3]
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT yStride, uvStride;
    gctUINT8_PTR yPlane  = gcvNULL;
    gctUINT8_PTR uvPlane = gcvNULL;
    gctUINT8_PTR yLine   = gcvNULL;
    gctUINT8_PTR uvLine  = gcvNULL;
    gctUINT8_PTR trgLine = gcvNULL;

    if (SourceStride != 0)
    {
        yStride  = SourceStride[0];
        uvStride = SourceStride[1];
    }
    else
    {
        yStride  = Right - X;
        uvStride = Right - X;
    }

    /* Compute aligned area. */
    X       = (X + 3) & ~0x03;
    Y       = (Y + 3) & ~0x03;
    Right  &= ~0x03;
    Bottom &= ~0x03;

    /* Move linear Memory to [0,0] origin. */
    yPlane  = (gctUINT8_PTR) Memory[0] -  yStride * Y - X * 1;
    uvPlane = (gctUINT8_PTR) Memory[1] - uvStride * Y - X * 1;

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];

            /* Skip odd x. */
            if (x & 1) continue;

            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            yLine   = yPlane  + y * yStride  + x * 1;
            uvLine  = uvPlane + y * uvStride + x * 1;

            /* NV61 to YUY2: a package - 2 pixels. */
            ((gctUINT32_PTR) trgLine)[0] =  (yLine[0]) | (uvLine[1] << 8) | (yLine[1] << 16) | (uvLine[0] << 24);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = X; x < Right; x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((y & 0x03) << 2) + (x << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                yLine   = yPlane  + y * yStride  + x * 1;
                uvLine  = uvPlane + y * uvStride + x * 1;

                /* NV61 to YUY2: one tile row - 2 packages - 4 pixels. */
                ((gctUINT32_PTR) trgLine)[0] =  (yLine[0]) | (uvLine[1] << 8) | (yLine[1] << 16) | (uvLine[0] << 24);
                ((gctUINT32_PTR) trgLine)[1] =  (yLine[2]) | (uvLine[3] << 8) | (yLine[3] << 16) | (uvLine[2] << 24);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = Y; y < Bottom; y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];

                /* Skip odd x. */
                if (x & 1) continue;

                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                yLine   = yPlane  + y * yStride  + x * 1;
                uvLine  = uvPlane + y * uvStride + x * 1;

                ((gctUINT32_PTR) trgLine)[0] =  (yLine[0]) | (uvLine[1] << 8) | (yLine[1] << 16) | (uvLine[0] << 24);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = Y; y < Bottom; y += 4)
    {
        xt = X << 2;
        yt = y;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
        yLine   = yPlane  + y * yStride  + X * 1;
        uvLine  = uvPlane + y * uvStride + X * 1;

        for (x = X; x < Right; x += 4)
        {
            gctUINT8_PTR srcY, srcUV;

            /* NV61 to YUY2: one block - 4 pixels. */
            srcY  = yLine;
            srcUV = uvLine;

            ((gctUINT32_PTR) trgLine)[0] =  (srcY[0]) | (srcUV[1] << 8) | (srcY[1] << 16) | (srcUV[0] << 24);
            ((gctUINT32_PTR) trgLine)[1] =  (srcY[2]) | (srcUV[3] << 8) | (srcY[3] << 16) | (srcUV[2] << 24);

            srcY  += yStride;
            srcUV += uvStride;
            ((gctUINT32_PTR) trgLine)[2] =  (srcY[0]) | (srcUV[1] << 8) | (srcY[1] << 16) | (srcUV[0] << 24);
            ((gctUINT32_PTR) trgLine)[3] =  (srcY[2]) | (srcUV[3] << 8) | (srcY[3] << 16) | (srcUV[2] << 24);

            srcY  += yStride;
            srcUV += uvStride;
            ((gctUINT32_PTR) trgLine)[4] =  (srcY[0]) | (srcUV[1] << 8) | (srcY[1] << 16) | (srcUV[0] << 24);
            ((gctUINT32_PTR) trgLine)[5] =  (srcY[2]) | (srcUV[3] << 8) | (srcY[3] << 16) | (srcUV[2] << 24);

            srcY  += yStride;
            srcUV += uvStride;
            ((gctUINT32_PTR) trgLine)[6] =  (srcY[0]) | (srcUV[1] << 8) | (srcY[1] << 16) | (srcUV[0] << 24);
            ((gctUINT32_PTR) trgLine)[7] =  (srcY[2]) | (srcUV[3] << 8) | (srcY[3] << 16) | (srcUV[2] << 24);

            /* Move to next tile. */
            trgLine += 16 * 2;
            yLine   +=  4 * 1;
            uvLine  +=  4 * 1;
        }
    }
}

/*******************************************************************************
**
**  gcoHARDWARE_UploadTextureYUV
**
**  Upload one slice into a texture.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gceSURF_FORMAT TargetFormat
**          Destination texture format. MUST be YUY2.
**
**      gctUINT32 Address
**          Hardware specific base address for mip map.
**
**      gctPOINTER Logical
**          Logical base address for mip map.
**
**      gctUINT32 Offset
**          Offset into mip map to upload.
**
**      gctINT TargetStride
**          Stride of the destination texture.
**
**      gctUINT X
**          X origin of data to upload.
**
**      gctUINT Y
**          Y origin of data to upload.
**
**      gctUINT Width
**          Width of texture to upload.
**
**      gctUINT Height
**          Height of texture to upload.
**
**      gctCONST_POINTER Memory[3]
**          Pointer to user buffer containing the data to upload.
**
**      gctINT SourceStride[3]
**          Stride of user buffer.
**
**      gceSURF_FORMAT SourceFormat
**          Format of the source texture to upload.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_UploadTextureYUV(
    IN gceSURF_FORMAT TargetFormat,
    IN gctUINT32 Address,
    IN gctPOINTER Logical,
    IN gctUINT32 Offset,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctPOINTER Memory[3],
    IN gctINT SourceStride[3],
    IN gceSURF_FORMAT SourceFormat
    )
{
    gceSTATUS status = gcvSTATUS_OK;
#if !gcdENDIAN_BIG
    gcoHARDWARE Hardware = gcvNULL;
    gctUINT x, y;
    gctUINT edgeX[6];
    gctUINT edgeY[6];
    gctUINT countX = 0;
    gctUINT countY = 0;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
#endif

    gcmHEADER_ARG("TargetFormat=%d Address=%08x Logical=0x%x "
                  "Offset=%u TargetStride=%d X=%u Y=%u Width=%u Height=%u "
                  "Memory=0x%x SourceStride=%d SourceFormat=%d",
                  TargetFormat, Address, Logical, Offset,
                  TargetStride, X, Y, Width, Height, Memory, SourceStride,
                  SourceFormat);

#if gcdENDIAN_BIG
    gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
#else
    gcmGETHARDWARE(Hardware);

    (void) TargetFormat;

    /* Compute dest logical. */
    Logical = (gctPOINTER) ((gctUINT8_PTR) Logical + Offset);

    /* Compute edge coordinates. */
    if (Width < 4)
    {
        for (x = X; x < right; x++)
        {
           edgeX[countX++] = x;
        }
    }
    else
    {
        for (x = X; x < ((X + 3) & ~3); x++)
        {
            edgeX[countX++] = x;
        }

        for (x = (right & ~3); x < right; x++)
        {
            edgeX[countX++] = x;
        }
    }

    if (Height < 4)
    {
       for (y = Y; y < bottom; y++)
       {
           edgeY[countY++] = y;
       }
    }
    else
    {
        for (y = Y; y < ((Y + 3) & ~3); y++)
        {
           edgeY[countY++] = y;
        }

        for (y = (bottom & ~3); y < bottom; y++)
        {
           edgeY[countY++] = y;
        }
    }

    switch (SourceFormat)
    {
    case gcvSURF_NV16:
        _UploadNV16toYUY2(Logical,
                        TargetStride,
                        X, Y,
                        right, bottom,
                        edgeX, edgeY,
                        countX, countY,
                        Memory,
                        SourceStride);
        break;

    case gcvSURF_NV61:
        _UploadNV61toYUY2(Logical,
                        TargetStride,
                        X, Y,
                        right, bottom,
                        edgeX, edgeY,
                        countX, countY,
                        Memory,
                        SourceStride);
        break;

    default:
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }
#endif

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gctUINT
_ComputeBlockOffset(
    gctUINT X,
    gctUINT Y
    )
{
    /* Compute offset in blocks, in a supertiled target, for source x,y.
       Target is compressed 4x4 blocks. */
    gctUINT x = X >> 2;
    gctUINT y = Y >> 2;

    return (x & 0x1)
         + ((y & 0x1) << 1)
         + ((x & 0x2) << 1)
         + ((y & 0x2) << 2)
         + ((x & 0x4) << 2)
         + ((y & 0x4) << 3)
         + ((x & 0x8) << 3)
         + ((y & 0x8) << 4);
}

/* Assuming 4x4 block size. */
static gceSTATUS
_UploadCompressedSuperTiled(
    gctCONST_POINTER Target,
    gctCONST_POINTER Source,
    IN gctUINT32 TrgWidth,
    IN gctUINT32 BytesPerTile,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT XOffset,
    IN gctUINT YOffset
    )
{
    gctUINT32_PTR source = gcvNULL;
    gctUINT32 sourceStride, targetStride;
    gctUINT32 offset, rowOffset;
    gctUINT x, y;

    /* 4 bits/pixel. */
    sourceStride = gcmALIGN(Width, 4) * BytesPerTile / 16;

    targetStride = gcmALIGN(TrgWidth, 64) * BytesPerTile / 16;

    gcmASSERT(sourceStride != 0);
    gcmASSERT(targetStride != 0);

    for (y = YOffset; y < Height + YOffset; y+=4)
    {
        rowOffset = (y & ~63) * targetStride;
        source = (gctUINT32_PTR)((gctUINT8_PTR) Source + (y - YOffset) * sourceStride);

        for (x = XOffset; x < Width + XOffset; x+=4)
        {
            offset = _ComputeBlockOffset(x & 63, y & 63) * BytesPerTile;

            /* Move to this supertile row start. */
            offset += rowOffset;

            /* Move to this supertile. */
            offset += (x & ~63) * (4 * BytesPerTile);

            /* Copy the 4x4 compressed block. */
            gcoOS_MemCopy(
                (gctPOINTER)((gctUINT8_PTR)Target + offset),
                source,
                BytesPerTile);
            source += BytesPerTile / 4;
        }
    }

    return gcvSTATUS_OK;
}

void _ConvertETC2(
    gctCONST_POINTER Target,
    gctCONST_POINTER Source,
    IN gctUINT32 BytesPerTile,
    IN gctBOOL PunchThrough
    )
{
    gctUINT8_PTR source = (gctUINT8_PTR)Source;
    gctUINT8_PTR target = (gctUINT8_PTR)Target;
    gctUINT8 index;
    gctINT8  baseColR1, baseColR2d;
    static gctUINT8 lookupTable[16] = { 0x4, 0x4, 0x4, 0x4,
                                        0x4, 0x4, 0x4, 0xE0,
                                        0x4, 0x4, 0xE0, 0xE0,
                                        0x4, 0xE0, 0xE0, 0xE0 };

    gcmASSERT(BytesPerTile == 8);

    /* Skip individual mode. */
    if (PunchThrough || (source[3] & 0x2))
    {
        /* Detect T-mode in big-endian. */
        baseColR1  = source[0] >> 3;
        baseColR2d = source[0] & 0x7;
        baseColR2d = (baseColR2d << 5);

        /* Sign extension. */
        baseColR2d = (baseColR2d >> 5);
        baseColR1 += baseColR2d;

        if (baseColR1 & 0x20)
        {
            /* Swap C0/C1 from below while copying source to target,
               and fill in the // bits with lookup table bit so as
               to ensure T mode decoding.

              63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48 47 46 45 44 43 42 41 40 39 38 37 36 35 34 33 32
              -----------------------------------------------------------------------------------------------
              // // //|R0a  |//|R0b  |G0         |B0         |R1         |G1         |B1          |da  |df|db|
            */

            index = source[2] >> 4;

            target[0] = lookupTable[index]
                      | ((index & 0x0C) << 1)
                      | ((index & 0x03) << 0);

            target[1] = ((source[2] & 0x0F) << 4)
                      | ((source[3] & 0xF0) >> 4);

            target[2] = ((source[0] & 0x18) << 3)
                      | ((source[0] & 0x03) << 4)
                      | ((source[1] & 0xF0) >> 4);

            target[3] = ((source[1] & 0x0F) << 4)
                      |  (source[3] & 0x0F);


            /* Detect T-mode in big-endian. */
            baseColR1  = target[0] >> 3;
            baseColR2d = target[0] & 0x7;
            baseColR2d = (baseColR2d << 5);

            /* Sign extension. */
            baseColR2d = (baseColR2d >> 5);
            baseColR1 += baseColR2d;
            if (!(baseColR1 & 0x20))
            {
                /* BUG. */
                baseColR1 = 0;
            }
        }
        else
        {
            target[0] = source[0];
            target[1] = source[1];
            target[2] = source[2];
            target[3] = source[3];
        }
    }
    else
    {
        target[0] = source[0];
        target[1] = source[1];
        target[2] = source[2];
        target[3] = source[3];
    }

    target[4] = source[4];
    target[5] = source[5];
    target[6] = source[6];
    target[7] = source[7];
}

static gceSTATUS
_UploadCompressedSuperTiledETC2(
    IN gctCONST_POINTER Target,
    IN gctCONST_POINTER Source,
    IN gctUINT32 TrgWidth,
    IN gctUINT32 BytesPerTile,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT XOffset,
    IN gctUINT YOffset,
    IN gctBOOL PatchEvenDword,
    IN gctBOOL PunchThrough
    )
{
    gctPOINTER source;
    gctUINT32 sourceStride, targetStride;
    gctUINT32 offset, rowOffset;
    gctUINT x, y;

    /* 4 bits/pixel. */
    sourceStride = gcmALIGN(Width, 4) * BytesPerTile / 16;

    targetStride = gcmALIGN(TrgWidth, 64) * BytesPerTile / 16;

    for (y = YOffset; y < Height + YOffset; y+=4)
    {
        rowOffset = (y & ~63) * targetStride;
        source = (gctPOINTER)((gctUINTPTR_T) Source + (y - YOffset) * sourceStride);

        for (x = XOffset; x < Width + XOffset; x+=4)
        {
            offset = _ComputeBlockOffset(x & 63, y & 63) * BytesPerTile;

            /* Move to this supertile row start. */
            offset += rowOffset;

            /* Move to this supertile. */
            offset += (x & ~63) * (4 * BytesPerTile);

            /* Copy the 4x4 compressed block. */
            if (PatchEvenDword && (BytesPerTile == 0x10))
            {
                gcoOS_MemCopy(
                    (gctPOINTER)((gctUINTPTR_T)Target + offset),
                    (gctPOINTER)((gctUINTPTR_T)source),
                    8);

                _ConvertETC2(
                    (gctPOINTER)((gctUINTPTR_T)Target + offset + 8),
                    (gctPOINTER)((gctUINTPTR_T)source + 8),
                    8,
                    PunchThrough);
            }
            else
            {
                _ConvertETC2(
                    (gctPOINTER)((gctUINTPTR_T)Target + offset),
                    source,
                    BytesPerTile,
                    PunchThrough);
            }

            source = (gctPOINTER) ((gctUINTPTR_T)source + BytesPerTile);
        }
    }

    return gcvSTATUS_OK;
}

static gceSTATUS
_UploadCompressedETC2(
    IN gctCONST_POINTER Target,
    IN gctCONST_POINTER Source,
    IN gctUINT ImageSize,
    IN gctBOOL PatchEvenDword,
    IN gctBOOL PunchThrough
    )
{
    gctUINT offset;

    for (offset = 0; offset < ImageSize; offset+=8)
    {
        if (PatchEvenDword && !(offset & 0x8))
        {
            gcoOS_MemCopy(
                (gctPOINTER)((gctUINTPTR_T)Target + offset),
                (gctPOINTER)((gctUINTPTR_T)Source + offset),
                8);
        }
        else
        {
            /* Copy the 4x4 compressed block. */
            _ConvertETC2(
                (gctPOINTER)((gctUINTPTR_T)Target + offset),
                (gctPOINTER)((gctUINTPTR_T)Source + offset),
                8,
                PunchThrough);
        }
    }

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHARDWARE_UploadCompressedTexture
**
**  Upload one slice into a texture.
**
**  INPUT:
**
**      gcoSURF TexSurf
**          Destination texture surface.
**
**      gctPOINTER Logical
**          Logical base address for mip map.
**
**      gctUINT32 Offset
**          Offset into mip map to upload.
**
**      gctUINT Width
**          Width of texture to upload.
**
**      gctUINT Height
**          Height of texture to upload.
**
**      gctINT SourceStride
**          Stride of user buffer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_UploadCompressedTexture(
    IN gcoSURF TexSurf,
    IN gctCONST_POINTER Logical,
    IN gctUINT32 Offset,
    IN gctUINT32 XOffset,
    IN gctUINT32 YOffset,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT ImageSize
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctBOOL needFlushCPUCache = gcvTRUE;
    gctBOOL patchEvenDword = gcvFALSE;

    gcmHEADER_ARG("TexSurf=0x%x Logical=0x%x "
                  "Offset=%u XOffset=%u YOffset=%u Width=%u Height=%u "
                  "ImageSize=%u",
                  TexSurf, Logical, Offset, XOffset, YOffset,
                  Width, Height,
                  ImageSize);

    switch (TexSurf->format)
    {
    case gcvSURF_RGBA8_ETC2_EAC:
        /* Fall through */
    case gcvSURF_SRGB8_ALPHA8_ETC2_EAC:
        /* Fall through */
        patchEvenDword = gcvTRUE;

    case gcvSURF_RGB8_ETC2:
        /* Fall through */
    case gcvSURF_SRGB8_ETC2:
        /* Fall through */
    case gcvSURF_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        /* Fall through */
    case gcvSURF_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        /* Fall through */

        if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_TEX_ETC2) == gcvFALSE)
        {
            gctBOOL punchThrough = gcvFALSE;

            /* Verify that the surface is locked. */
            gcmVERIFY_NODE_LOCK(&TexSurf->node);

            if ((TexSurf->format == gcvSURF_RGB8_PUNCHTHROUGH_ALPHA1_ETC2)
             || (TexSurf->format == gcvSURF_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2))
            {
                punchThrough = gcvTRUE;
            }

            if (TexSurf->superTiled)
            {
                gcmONERROR(
                    _UploadCompressedSuperTiledETC2(
                        TexSurf->node.logical + Offset,
                        Logical,
                        TexSurf->alignedW,
                        /* 4x4 block. */
                        (TexSurf->bitsPerPixel * 4 * 4) / 8,
                        Width,
                        Height,
                        XOffset,
                        YOffset,
                        patchEvenDword,
                        punchThrough));
            }
            else
            {
                gcmONERROR(
                    _UploadCompressedETC2(
                        TexSurf->node.logical + Offset,
                        Logical,
                        ImageSize,
                        patchEvenDword,
                        punchThrough));
            }
            break;
        }
        /* else case, fall through. */


    case gcvSURF_DXT1:
        /* Fall through */
    case gcvSURF_ETC1:
        /* Fall through */
    case gcvSURF_DXT2:
        /* Fall through */
    case gcvSURF_DXT3:
        /* Fall through */
    case gcvSURF_DXT4:
        /* Fall through */
    case gcvSURF_DXT5:
        /* Fall through */
    case gcvSURF_R11_EAC:
        /* Fall through */
    case gcvSURF_SIGNED_R11_EAC:
        /* Fall through */
    case gcvSURF_RG11_EAC:
        /* Fall through */
    case gcvSURF_SIGNED_RG11_EAC:
        /* Fall through */

    case gcvSURF_ASTC4x4:
    case gcvSURF_ASTC5x4:
    case gcvSURF_ASTC5x5:
    case gcvSURF_ASTC6x5:
    case gcvSURF_ASTC6x6:
    case gcvSURF_ASTC8x5:
    case gcvSURF_ASTC8x6:
    case gcvSURF_ASTC8x8:
    case gcvSURF_ASTC10x5:
    case gcvSURF_ASTC10x6:
    case gcvSURF_ASTC10x8:
    case gcvSURF_ASTC10x10:
    case gcvSURF_ASTC12x10:
    case gcvSURF_ASTC12x12:
        /* Fall through */

    case gcvSURF_ASTC4x4_SRGB:
    case gcvSURF_ASTC5x4_SRGB:
    case gcvSURF_ASTC5x5_SRGB:
    case gcvSURF_ASTC6x5_SRGB:
    case gcvSURF_ASTC6x6_SRGB:
    case gcvSURF_ASTC8x5_SRGB:
    case gcvSURF_ASTC8x6_SRGB:
    case gcvSURF_ASTC8x8_SRGB:
    case gcvSURF_ASTC10x5_SRGB:
    case gcvSURF_ASTC10x6_SRGB:
    case gcvSURF_ASTC10x8_SRGB:
    case gcvSURF_ASTC10x10_SRGB:
    case gcvSURF_ASTC12x10_SRGB:
    case gcvSURF_ASTC12x12_SRGB:
        /* Fall through */

        /* Verify that the surface is locked. */
        gcmVERIFY_NODE_LOCK(&TexSurf->node);

        if (TexSurf->superTiled)
        {
            gcmONERROR(
                _UploadCompressedSuperTiled(
                    TexSurf->node.logical + Offset,
                    Logical,
                    TexSurf->alignedW,
                    /* 4x4 block. */
                    (TexSurf->bitsPerPixel * 4 * 4) / 8,
                    Width,
                    Height,
                    XOffset,
                    YOffset));
        }
        else
        {
            switch (TexSurf->format)
            {
            case gcvSURF_DXT1:
                /* Fall through */
            case gcvSURF_DXT2:
                /* Fall through */
            case gcvSURF_DXT3:
                /* Fall through */
            case gcvSURF_DXT4:
                /* Fall through */
            case gcvSURF_DXT5:
                /* Fall through */
                {
                    /* Uploaded texture in linear fashion. */
                    gctUINT height, width, leftOffset, bottomOffset, destOffset, srcOffset;
                    gctUINT blockSize =  (TexSurf->bitsPerPixel * 4 * 4) / 8;

                    leftOffset = (XOffset + 3) / 4;
                    bottomOffset = ((YOffset + 3) / 4) * ((TexSurf->alignedW + 3) / 4);

                    for (height = 0; height < ((Height + 3) / 4); height++)
                    {
                        for (width = 0; width < ((Width + 3) / 4); width++)
                        {
                            destOffset = (bottomOffset + leftOffset + height *  ((TexSurf->alignedW + 3) / 4) + width) * blockSize;
                            srcOffset = (height * ((Width + 3) / 4) + width)* blockSize;
                            gcoOS_MemCopy(
                                (gctPOINTER)((gctUINT8_PTR)TexSurf->node.logical + destOffset + Offset),
                                (gctPOINTER)((gctUINT8_PTR)Logical + srcOffset),
                                blockSize);
                        }
                    }
                }
                break;

            default:
                /* Uploaded texture in linear fashion. */
                gcmONERROR(
                           gcoHARDWARE_CopyData(&TexSurf->node,
                                                Offset,
                                                Logical,
                                                ImageSize));
                /* gcoHARDWARE_CopyData already flushed the CPU cache */
                needFlushCPUCache = gcvFALSE;
                break;
            }
        }
        break;

    default:
        /* Non compressed textures cannot be uploaded supertiled here. */
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    if (needFlushCPUCache)
    {
        /* Flush the CPU cache. */
        gcmONERROR(gcoSURF_NODE_Cache(&TexSurf->node,
                                      TexSurf->node.logical,
                                      TexSurf->node.size,
                                      gcvCACHE_CLEAN));
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#endif

