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


/**
**  @file
**  Architecture independent format conversion.
**
*/

#include "gc_hal_user_precomp.h"
#if gcdENABLE_3D


#define gcdGET_FIELD(data, start, bits) (((data) >> (start)) & (~(~0U << bits)))
#define gcdUNORM_MAX(n)                 ((gctFLOAT)(((gctUINT64)1 << (n)) - 1))

#define gcdUNORM_TO_FLOAT(ub, n)        (gctFLOAT)((ub) / gcdUNORM_MAX(n))
#define gcdFLOAT_TO_UNORM(x, n)         (gctUINT32)(gcmCLAMP((x), 0.0f, 1.0f) * gcdUNORM_MAX(n) + 0.5f)

#define gcdSNORM_TO_FLOAT(b, n)         (gctFLOAT)(gcmMAX(((b) / gcdUNORM_MAX((n) - 1)), -1.0f))
#define gcdOFFSET_O_DOT_5(x)            (((x) < 0) ? ((x) - 0.5f) : ((x) + 0.5))
#define gcdFLOAT_TO_SNORM(x, n)         (gctINT32)(gcdOFFSET_O_DOT_5(gcmCLAMP((x), -1.0f, 1.0f) * gcdUNORM_MAX((n) - 1)))

#define gcdINT_MIN(n)                   (-(gctINT)((gctINT64)1 << ((n) - 1)))
#define gcdINT_MAX(n)                   ((gctINT)(((gctINT64)1 << ((n) - 1)) - 1))
#define gcdUINT_MAX(n)                  ((gctUINT)(((gctUINT64)1 << (n)) - 1))



void _ReadPixelFrom_A8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8 ub = *(gctUINT8*)inAddr[0];

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b = 0.0f;
    outPixel->color.f.a = gcdUNORM_TO_FLOAT(ub, 8);
    outPixel->d= 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A16(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 us = *(gctUINT16*)inAddr[0];

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b = 0.0f;
    outPixel->color.f.a = gcdUNORM_TO_FLOAT(us, 16);
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A16F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 sf = *(gctUINT16*)inAddr[0];
    gctUINT32 f32 = gcoMATH_Float16ToFloat(sf);

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b = 0.0f;
    outPixel->color.f.a = *(gctFLOAT *)&f32;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}


void _ReadPixelFrom_L8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8 ub = *(gctUINT8*)inAddr[0];

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(ub, 8);
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_L16(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 us = *(gctUINT16*)inAddr[0];

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(us, 16);
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_L16F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 sf = *(gctUINT16*)inAddr[0];
    gctUINT32 f32 = gcoMATH_Float16ToFloat(sf);

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b = *(gctFLOAT *)&f32;
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A8L8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8* pUB = (gctUINT8*)inAddr[0];

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(pUB[0], 8);
    outPixel->color.f.a = gcdUNORM_TO_FLOAT(pUB[1], 8);
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A8L8_1_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8 *pUB = (gctUINT8*)inAddr[0];

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(pUB[0], 8);
    outPixel->color.f.a = gcdUNORM_TO_FLOAT(pUB[3], 8);
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A16L16(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16* pUS = (gctUINT16*)inAddr[0];

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(pUS[0], 16);
    outPixel->color.f.a = gcdUNORM_TO_FLOAT(pUS[1], 16);
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A16L16F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16* pSF = (gctUINT16*)inAddr[0];
    gctUINT32 f32[2];

    f32[0] = gcoMATH_Float16ToFloat(pSF[0]);
    f32[1] = gcoMATH_Float16ToFloat(pSF[1]);
    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b = *(gctFLOAT *)&f32[0];
    outPixel->color.f.a = *(gctFLOAT *)&f32[1];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A32F(gctPOINTER inAddr[4], gcsPIXEL* outPixel)
{
    gctFLOAT sf = *(gctFLOAT*)inAddr[0];

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b = 0.0f;
    outPixel->color.f.a = sf;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A32L32F(gctPOINTER inAddr[4], gcsPIXEL* outPixel)
{
    gctFLOAT f1 = *(gctFLOAT*)inAddr[0];
    gctFLOAT f2 = *((gctFLOAT*)inAddr[0]+1);

    outPixel->color.f.r = f1;
    outPixel->color.f.g = f1;
    outPixel->color.f.b = f1;
    outPixel->color.f.a = f2;
    outPixel->d = 1.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_L32F(gctPOINTER inAddr[4], gcsPIXEL* outPixel)
{
    gctFLOAT sf = *(gctFLOAT*)inAddr[0];

    outPixel->color.f.r = sf;
    outPixel->color.f.g = sf;
    outPixel->color.f.b = sf;
    outPixel->color.f.a = 1.0f;
    outPixel->d = 1.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_R8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8 ub = *(gctUINT8*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(ub, 8);
    outPixel->color.f.g = 0.0f;
    outPixel->color.f.b = 0.0f;
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_R8_1_X8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8 *pUB = (gctUINT8*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(pUB[2], 8);
    outPixel->color.f.g = 0.0f;
    outPixel->color.f.b = 0.0f;
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_R16(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 us = *(gctUINT16*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(us, 16);
    outPixel->color.f.g = 0.0f;
    outPixel->color.f.b = 0.0f;
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_R16F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 sf = *(gctUINT16*)inAddr[0];
    gctUINT32 f32 = gcoMATH_Float16ToFloat(sf);

    outPixel->color.f.r = *(gctFLOAT*)&f32;
    outPixel->color.f.g = 0.0f;
    outPixel->color.f.b = 0.0f;
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}


void _ReadPixelFrom_R32F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctFLOAT f = *(gctFLOAT*)inAddr[0];

    outPixel->color.f.r = f;
    outPixel->color.f.g = 0.0f;
    outPixel->color.f.b = 0.0f;
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}


void _ReadPixelFrom_G8R8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8* pUB = (gctUINT8*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(pUB[0], 8);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(pUB[1], 8);
    outPixel->color.f.b = 0.0f;
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_G8R8_1_X8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8 *pUB = (gctUINT8*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(pUB[2], 8);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(pUB[1], 8);
    outPixel->color.f.b = 0.0f;
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_G16R16(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16* pUS = (gctUINT16*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(pUS[0], 16);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(pUS[1], 16);
    outPixel->color.f.b = 0.0f;
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_G16R16F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16* pSF = (gctUINT16*)inAddr[0];
    gctUINT32 f32[2];

    f32[0] = gcoMATH_Float16ToFloat(pSF[0]);
    f32[1] = gcoMATH_Float16ToFloat(pSF[1]);
    outPixel->color.f.r = *(gctFLOAT *)&f32[0];
    outPixel->color.f.g = *(gctFLOAT *)&f32[1];
    outPixel->color.f.b = 0.0f;
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}


void _ReadPixelFrom_G32R32F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctFLOAT* pF = (gctFLOAT*)inAddr[0];

    outPixel->color.f.r = pF[0];
    outPixel->color.f.g = pF[1];
    outPixel->color.f.b = 0.0f;
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_G32R32F_2_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctFLOAT* pF0 = (gctFLOAT*)inAddr[0];
    gctFLOAT* pF1 = (gctFLOAT*)inAddr[1];

    outPixel->color.f.r = pF0[0];
    outPixel->color.f.g = pF1[0];
    outPixel->color.f.b = 0.0f;
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}


void _ReadPixelFrom_A4R4G4B4(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 us = *(gctUINT16*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us,  8, 4), 4);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us,  4, 4), 4);
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us,  0, 4), 4);
    outPixel->color.f.a = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us, 12, 4), 4);
    outPixel->d = 0.0f;
    outPixel->s = 0;
}


void _ReadPixelFrom_A1R5G5B5(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 us = *(gctUINT16*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us, 10, 5), 5);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us,  5, 5), 5);
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us,  0, 5), 5);
    outPixel->color.f.a = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us, 15, 1), 1);
    outPixel->d = 0.0f;
    outPixel->s = 0;
}



void _ReadPixelFrom_R5G6B5(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 us = *(gctUINT16*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us, 11, 5), 5);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us,  5, 6), 6);
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us,  0, 5), 5);
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_B8G8R8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8 *pUB = (gctUINT8*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(pUB[0], 8);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(pUB[1], 8);
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(pUB[2], 8);
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_B16G16R16(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 *pUS = (gctUINT16*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(pUS[0], 16);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(pUS[1], 16);
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(pUS[2], 16);
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_X4R4G4B4(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 us = *(gctUINT16*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us, 8, 4), 4);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us, 4, 4), 4);
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us, 0, 4), 4);
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_R4G4B4A4(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 us = *(gctUINT16*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us, 12, 4), 4);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us,  8, 4), 4);
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us,  4, 4), 4);
    outPixel->color.f.a = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us,  0, 4), 4);
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_X1R5G5B5(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 us = *(gctUINT16*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us, 10, 5), 5);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us,  5, 5), 5);
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us,  0, 5), 5);
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_R5G5B5A1(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 us = *(gctUINT16*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us, 11, 5), 5);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us,  6, 5), 5);
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us,  1, 5), 5);
    outPixel->color.f.a = gcdUNORM_TO_FLOAT(gcdGET_FIELD(us,  0, 1), 1);
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A8B8G8R8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8 *pUB = (gctUINT8*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(pUB[0], 8);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(pUB[1], 8);
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(pUB[2], 8);
    outPixel->color.f.a = gcdUNORM_TO_FLOAT(pUB[3], 8);
    outPixel->d = 0.0f;
    outPixel->s = 0;
}


void _ReadPixelFrom_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8 *pUB = (gctUINT8*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(pUB[2], 8);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(pUB[1], 8);
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(pUB[0], 8);
    outPixel->color.f.a = gcdUNORM_TO_FLOAT(pUB[3], 8);
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_X8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8 *pUB = (gctUINT8*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(pUB[2], 8);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(pUB[1], 8);
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(pUB[0], 8);
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_X2B10G10R10(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT32 ui = *(gctUINT32*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(gcdGET_FIELD(ui,  0, 10), 10);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(gcdGET_FIELD(ui, 10, 10), 10);
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(gcdGET_FIELD(ui, 20, 10), 10);
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A2B10G10R10(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT32 ui = *(gctUINT32*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(gcdGET_FIELD(ui,  0, 10), 10);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(gcdGET_FIELD(ui, 10, 10), 10);
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(gcdGET_FIELD(ui, 20, 10), 10);
    outPixel->color.f.a = gcdUNORM_TO_FLOAT(gcdGET_FIELD(ui, 30,  2),  2);
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A8B12G12R12_2_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8 *pUB0 = (gctUINT8*)inAddr[0];
    gctUINT8 *pUB1 = (gctUINT8*)inAddr[1];

    outPixel->color.f.b = gcdUNORM_TO_FLOAT((pUB0[0] << 4) + pUB1[0], 12);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT((pUB0[1] << 4) + pUB1[1], 12);
    outPixel->color.f.r = gcdUNORM_TO_FLOAT((pUB0[2] << 4) + pUB1[2], 12);
    outPixel->color.f.a = gcdUNORM_TO_FLOAT( pUB0[3],  8);
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A16B16G16R16(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 *pUS = (gctUINT16*)inAddr[0];

    outPixel->color.f.r = gcdUNORM_TO_FLOAT(pUS[0], 16);
    outPixel->color.f.g = gcdUNORM_TO_FLOAT(pUS[1], 16);
    outPixel->color.f.b = gcdUNORM_TO_FLOAT(pUS[2], 16);
    outPixel->color.f.a = gcdUNORM_TO_FLOAT(pUS[3], 16);
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_X16B16G16R16F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 *pSF = (gctUINT16*)inAddr[0];
    gctUINT32 f32[3];

    f32[0] = gcoMATH_Float16ToFloat(pSF[0]);
    f32[1] = gcoMATH_Float16ToFloat(pSF[1]);
    f32[2] = gcoMATH_Float16ToFloat(pSF[2]);

    outPixel->color.f.r = *(gctFLOAT *)&f32[0];
    outPixel->color.f.g = *(gctFLOAT *)&f32[1];
    outPixel->color.f.b = *(gctFLOAT *)&f32[2];
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A16B16G16R16F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 *pSF = (gctUINT16*)inAddr[0];
    gctUINT32 f32[4];

    f32[0] = gcoMATH_Float16ToFloat(pSF[0]);
    f32[1] = gcoMATH_Float16ToFloat(pSF[1]);
    f32[2] = gcoMATH_Float16ToFloat(pSF[2]);
    f32[3] = gcoMATH_Float16ToFloat(pSF[3]);

    outPixel->color.f.r = *(gctFLOAT *)&f32[0];
    outPixel->color.f.g = *(gctFLOAT *)&f32[1];
    outPixel->color.f.b = *(gctFLOAT *)&f32[2];
    outPixel->color.f.a = *(gctFLOAT *)&f32[3];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A16B16G16R16F_2_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 *pSF0 = (gctUINT16*)inAddr[0];
    gctUINT16 *pSF1 = (gctUINT16*)inAddr[1];
    gctUINT32 f32[4];

    f32[0] = gcoMATH_Float16ToFloat(pSF0[0]);
    f32[1] = gcoMATH_Float16ToFloat(pSF0[1]);
    f32[2] = gcoMATH_Float16ToFloat(pSF1[0]);
    f32[3] = gcoMATH_Float16ToFloat(pSF1[1]);

    outPixel->color.f.r = *(gctFLOAT *)&f32[0];
    outPixel->color.f.g = *(gctFLOAT *)&f32[1];
    outPixel->color.f.b = *(gctFLOAT *)&f32[2];
    outPixel->color.f.a = *(gctFLOAT *)&f32[3];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A16B16G16R16F_2_G16R16F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{

    gctUINT16 *pSF0 = (gctUINT16*)inAddr[0];
    gctUINT16 *pSF1 = (gctUINT16*)inAddr[1];
    gctUINT32 f32[4];

    f32[0] = gcoMATH_Float16ToFloat(pSF0[0]);
    f32[1] = gcoMATH_Float16ToFloat(pSF0[1]);
    f32[2] = gcoMATH_Float16ToFloat(pSF1[0]);
    f32[3] = gcoMATH_Float16ToFloat(pSF1[1]);

    outPixel->color.f.r = *(gctFLOAT *)&f32[0];
    outPixel->color.f.g = *(gctFLOAT *)&f32[1];
    outPixel->color.f.b = *(gctFLOAT *)&f32[2];
    outPixel->color.f.a = *(gctFLOAT *)&f32[3];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_D16(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 us = *(gctUINT16*)inAddr[0];

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b =
    outPixel->color.f.a = 0.0f;
    outPixel->d = gcdUNORM_TO_FLOAT(us, 16);
    outPixel->s = 0;
}

void _ReadPixelFrom_D24X8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT32 ui = *(gctUINT32*)inAddr[0];

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b =
    outPixel->color.f.a = 0.0f;
    outPixel->d = gcdUNORM_TO_FLOAT(gcdGET_FIELD(ui,  8, 24), 24);
    outPixel->s = 0;
}

void _ReadPixelFrom_D24S8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT32 ui = *(gctUINT32*)inAddr[0];

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b =
    outPixel->color.f.a = 0.0f;
    outPixel->d = gcdUNORM_TO_FLOAT(gcdGET_FIELD(ui,  8, 24), 24);
    outPixel->s = (gctUINT32)(gcdGET_FIELD(ui,  0,  8));
}

void _ReadPixelFrom_X24S8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT32 ui = *(gctUINT32*)inAddr[0];

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b =
    outPixel->color.f.a = 0.0f;
    outPixel->d = 0.0f;
    outPixel->s = gcdGET_FIELD(ui,  0,  8);
}

void _ReadPixelFrom_S8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8 ui = *(gctUINT8*)inAddr[0];

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b =
    outPixel->color.f.a = 0.0f;
    outPixel->d = 0.0f;
    outPixel->s = (gctUINT32)ui;

}

void _ReadPixelFrom_D32(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT32 ui = *(gctUINT32*)inAddr[0];

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b =
    outPixel->color.f.a = 0.0f;
    outPixel->d = gcdUNORM_TO_FLOAT(ui, 32);
    outPixel->s = 0;
}

void _ReadPixelFrom_D32F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctFLOAT f = *(gctFLOAT*)inAddr[0];

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b =
    outPixel->color.f.a = 0.0f;
    outPixel->d = f;
    outPixel->s = 0;
}

void _ReadPixelFrom_S8D32F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctFLOAT f = *(gctFLOAT*)inAddr[0];
    gctUINT32 ui = *((gctINT32 *)inAddr[0] + 1);

    outPixel->color.f.r =
    outPixel->color.f.g =
    outPixel->color.f.b =
    outPixel->color.f.a = 0.0f;
    outPixel->d = f;
    outPixel->s = (ui & 0xFF);
}


/* SNORM format. */
void _ReadPixelFrom_R8_SNORM(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT8 b = *(gctINT8*)inAddr[0];

    outPixel->color.f.r = gcdSNORM_TO_FLOAT(b, 8);
    outPixel->color.f.g = 0.0f;
    outPixel->color.f.b = 0.0f;
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_G8R8_SNORM(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT8* pB = (gctINT8*)inAddr[0];

    outPixel->color.f.r = gcdSNORM_TO_FLOAT(pB[0], 8);
    outPixel->color.f.g = gcdSNORM_TO_FLOAT(pB[1], 8);
    outPixel->color.f.b = 0.0f;
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_B8G8R8_SNORM(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT8 *pB = (gctINT8*)inAddr[0];

    outPixel->color.f.r = gcdSNORM_TO_FLOAT(pB[0], 8);
    outPixel->color.f.g = gcdSNORM_TO_FLOAT(pB[1], 8);
    outPixel->color.f.b = gcdSNORM_TO_FLOAT(pB[2], 8);
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A8B8G8R8_SNORM(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT8 *pB = (gctINT8*)inAddr[0];

    outPixel->color.f.r = gcdSNORM_TO_FLOAT(pB[0], 8);
    outPixel->color.f.g = gcdSNORM_TO_FLOAT(pB[1], 8);
    outPixel->color.f.b = gcdSNORM_TO_FLOAT(pB[2], 8);
    outPixel->color.f.a = gcdSNORM_TO_FLOAT(pB[3], 8);
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_X8B8G8R8_SNORM(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT8 *pB = (gctINT8*)inAddr[0];

    outPixel->color.f.r = gcdSNORM_TO_FLOAT(pB[0], 8);
    outPixel->color.f.g = gcdSNORM_TO_FLOAT(pB[1], 8);
    outPixel->color.f.b = gcdSNORM_TO_FLOAT(pB[2], 8);
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

/* Integer format. */
void _ReadPixelFrom_R8I(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT8 i = *(gctINT8*)inAddr[0];

    outPixel->color.i.r = i;
    outPixel->color.i.g = 0;
    outPixel->color.i.b = 0;
    outPixel->color.i.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_R8UI(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8 ui = *(gctUINT8*)inAddr[0];

    outPixel->color.ui.r = ui;
    outPixel->color.ui.g = 0;
    outPixel->color.ui.b = 0;
    outPixel->color.ui.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_R16I(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT16 i = *(gctINT16*)inAddr[0];

    outPixel->color.i.r = i;
    outPixel->color.i.g = 0;
    outPixel->color.i.b = 0;
    outPixel->color.i.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_R16UI(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 i = *(gctUINT16*)inAddr[0];

    outPixel->color.ui.r = i;
    outPixel->color.ui.g = 0;
    outPixel->color.ui.b = 0;
    outPixel->color.ui.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_R32I(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT32 i = *(gctINT32*)inAddr[0];

    outPixel->color.i.r = i;
    outPixel->color.i.g = 0;
    outPixel->color.i.b = 0;
    outPixel->color.i.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_R32UI(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT32 i = *(gctUINT32*)inAddr[0];

    outPixel->color.ui.r = i;
    outPixel->color.ui.g = 0;
    outPixel->color.ui.b = 0;
    outPixel->color.ui.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_G8R8I(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT8* pI = (gctINT8*)inAddr[0];

    outPixel->color.i.r = pI[0];
    outPixel->color.i.g = pI[1];
    outPixel->color.i.b = 0;
    outPixel->color.i.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_G8R8UI(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8* pI = (gctUINT8*)inAddr[0];

    outPixel->color.ui.r = pI[0];
    outPixel->color.ui.g = pI[1];
    outPixel->color.ui.b = 0;
    outPixel->color.ui.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_G16R16I(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT16* pI = (gctINT16*)inAddr[0];

    outPixel->color.i.r = pI[0];
    outPixel->color.i.g = pI[1];
    outPixel->color.i.b = 0;
    outPixel->color.i.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_G16R16UI(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16* pI = (gctUINT16*)inAddr[0];

    outPixel->color.ui.r = pI[0];
    outPixel->color.ui.g = pI[1];
    outPixel->color.ui.b = 0;
    outPixel->color.ui.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_G32R32I(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT32* pI = (gctINT32*)inAddr[0];

    outPixel->color.i.r = pI[0];
    outPixel->color.i.g = pI[1];
    outPixel->color.i.b = 0;
    outPixel->color.i.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_G32R32I_2_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT32* pI0 = (gctINT32*)inAddr[0];
    gctINT32* pI1 = (gctINT32*)inAddr[1];


    outPixel->color.i.r = pI0[0];
    outPixel->color.i.g = pI1[0];
    outPixel->color.i.b = 0;
    outPixel->color.i.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}


void _ReadPixelFrom_G32R32UI(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT32* pI = (gctUINT32*)inAddr[0];

    outPixel->color.ui.r = pI[0];
    outPixel->color.ui.g = pI[1];
    outPixel->color.ui.b = 0;
    outPixel->color.ui.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_G32R32UI_2_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT32* pI0 = (gctUINT32*)inAddr[0];
    gctUINT32* pI1 = (gctUINT32*)inAddr[1];

    outPixel->color.ui.r = pI0[0];
    outPixel->color.ui.g = pI1[0];
    outPixel->color.ui.b = 0;
    outPixel->color.ui.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_B8G8R8I(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT8* pI = (gctINT8*)inAddr[0];

    outPixel->color.i.r = pI[0];
    outPixel->color.i.g = pI[1];
    outPixel->color.i.b = pI[2];
    outPixel->color.i.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_B8G8R8I_1_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT8* pI = (gctINT8*)inAddr[0];

    outPixel->color.i.b = pI[0];
    outPixel->color.i.g = pI[1];
    outPixel->color.i.r = pI[2];
    outPixel->color.i.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_B8G8R8UI(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8* pI = (gctUINT8*)inAddr[0];

    outPixel->color.ui.r = pI[0];
    outPixel->color.ui.g = pI[1];
    outPixel->color.ui.b = pI[2];
    outPixel->color.ui.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_B8G8R8UI_1_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8* pI = (gctUINT8*)inAddr[0];

    outPixel->color.ui.b = pI[0];
    outPixel->color.ui.g = pI[1];
    outPixel->color.ui.r = pI[2];
    outPixel->color.ui.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_B16G16R16I(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT16* pI = (gctINT16*)inAddr[0];

    outPixel->color.i.r = pI[0];
    outPixel->color.i.g = pI[1];
    outPixel->color.i.b = pI[2];
    outPixel->color.i.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_B16G16R16I_2_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT16* pI0 = (gctINT16*)inAddr[0];
    gctINT16* pI1 = (gctINT16*)inAddr[1];

    outPixel->color.i.r = pI0[0];
    outPixel->color.i.g = pI0[1];
    outPixel->color.i.b = pI1[0];
    outPixel->color.i.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_B16G16R16UI(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16* pI = (gctUINT16*)inAddr[0];

    outPixel->color.ui.r = pI[0];
    outPixel->color.ui.g = pI[1];
    outPixel->color.ui.b = pI[2];
    outPixel->color.ui.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}


void _ReadPixelFrom_B16G16R16UI_2_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16* pI0 = (gctUINT16*)inAddr[0];
    gctUINT16* pI1 = (gctUINT16*)inAddr[1];

    outPixel->color.ui.r = pI0[0];
    outPixel->color.ui.g = pI0[1];
    outPixel->color.ui.b = pI1[0];
    outPixel->color.ui.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}


void _ReadPixelFrom_B16G16R16F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16* pI = (gctUINT16*)inAddr[0];
    gctUINT32 f32[3];

    f32[0] = gcoMATH_Float16ToFloat(pI[0]);
    f32[1] = gcoMATH_Float16ToFloat(pI[1]);
    f32[2] = gcoMATH_Float16ToFloat(pI[2]);

    outPixel->color.f.r = *(gctFLOAT *)&f32[0];
    outPixel->color.f.g = *(gctFLOAT *)&f32[1];
    outPixel->color.f.b = *(gctFLOAT *)&f32[2];
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_B16G16R16F_2_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16* pI0 = (gctUINT16*)inAddr[0];
    gctUINT16* pI1 = (gctUINT16*)inAddr[1];

    gctUINT32 f32[3];

    f32[0] = gcoMATH_Float16ToFloat(pI0[0]);
    f32[1] = gcoMATH_Float16ToFloat(pI0[1]);
    f32[2] = gcoMATH_Float16ToFloat(pI1[0]);

    outPixel->color.f.r = *(gctFLOAT *)&f32[0];
    outPixel->color.f.g = *(gctFLOAT *)&f32[1];
    outPixel->color.f.b = *(gctFLOAT *)&f32[2];
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_B32G32R32I(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT32* pI = (gctINT32*)inAddr[0];

    outPixel->color.i.r = pI[0];
    outPixel->color.i.g = pI[1];
    outPixel->color.i.b = pI[2];
    outPixel->color.i.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_B32G32R32UI(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT32* pI = (gctUINT32*)inAddr[0];

    outPixel->color.ui.r = pI[0];
    outPixel->color.ui.g = pI[1];
    outPixel->color.ui.b = pI[2];
    outPixel->color.ui.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_B32G32R32I_3_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT32* pI0 = (gctINT32*)inAddr[0];
    gctINT32* pI1 = (gctINT32*)inAddr[1];
    gctINT32* pI2 = (gctINT32*)inAddr[2];

    outPixel->color.i.r = pI0[0];
    outPixel->color.i.g = pI1[0];
    outPixel->color.i.b = pI2[0];
    outPixel->color.i.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_B32G32R32UI_3_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT32* pI0 = (gctUINT32*)inAddr[0];
    gctUINT32* pI1 = (gctUINT32*)inAddr[1];
    gctUINT32* pI2 = (gctUINT32*)inAddr[2];

    outPixel->color.ui.r = pI0[0];
    outPixel->color.ui.g = pI1[0];
    outPixel->color.ui.b = pI2[0];
    outPixel->color.ui.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}


void _ReadPixelFrom_B32G32R32F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctFLOAT* pf = (gctFLOAT*)inAddr[0];

    outPixel->color.f.r = pf[0];
    outPixel->color.f.g = pf[1];
    outPixel->color.f.b = pf[2];
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}


void _ReadPixelFrom_B32G32R32F_3_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctFLOAT* pf0 = (gctFLOAT*)inAddr[0];
    gctFLOAT* pf1 = (gctFLOAT*)inAddr[1];
    gctFLOAT* pf2 = (gctFLOAT*)inAddr[2];

    outPixel->color.f.r = pf0[0];
    outPixel->color.f.g = pf1[0];
    outPixel->color.f.b = pf2[0];
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A8B8G8R8I(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT8* pI = (gctINT8*)inAddr[0];

    outPixel->color.i.r = pI[0];
    outPixel->color.i.g = pI[1];
    outPixel->color.i.b = pI[2];
    outPixel->color.i.a = pI[3];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A8B8G8R8I_1_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT8* pI = (gctINT8*)inAddr[0];

    outPixel->color.i.b = pI[0];
    outPixel->color.i.g = pI[1];
    outPixel->color.i.r = pI[2];
    outPixel->color.i.a = pI[3];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A8B8G8R8UI(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8* pI = (gctUINT8*)inAddr[0];

    outPixel->color.ui.r = pI[0];
    outPixel->color.ui.g = pI[1];
    outPixel->color.ui.b = pI[2];
    outPixel->color.ui.a = pI[3];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A8B8G8R8UI_1_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT8* pI = (gctUINT8*)inAddr[0];

    outPixel->color.ui.b = pI[0];
    outPixel->color.ui.g = pI[1];
    outPixel->color.ui.r = pI[2];
    outPixel->color.ui.a = pI[3];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_X16B16G16R16I(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT16* pI = (gctINT16*)inAddr[0];

    outPixel->color.i.r = pI[0];
    outPixel->color.i.g = pI[1];
    outPixel->color.i.b = pI[2];
    outPixel->color.i.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A16B16G16R16I(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT16* pI = (gctINT16*)inAddr[0];

    outPixel->color.i.r = pI[0];
    outPixel->color.i.g = pI[1];
    outPixel->color.i.b = pI[2];
    outPixel->color.i.a = pI[3];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_X16B16G16R16I_2_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT16* pI0 = (gctINT16*)inAddr[0];
    gctINT16* pI1 = (gctINT16*)inAddr[1];

    outPixel->color.i.r = pI0[0];
    outPixel->color.i.g = pI0[1];
    outPixel->color.i.b = pI1[0];
    outPixel->color.i.a = 1;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A16B16G16R16I_2_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT16* pI0 = (gctINT16*)inAddr[0];
    gctINT16* pI1 = (gctINT16*)inAddr[1];

    outPixel->color.i.r = pI0[0];
    outPixel->color.i.g = pI0[1];
    outPixel->color.i.b = pI1[0];
    outPixel->color.i.a = pI1[1];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_X16B16G16R16UI(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16* pI = (gctUINT16*)inAddr[0];

    outPixel->color.ui.r = pI[0];
    outPixel->color.ui.g = pI[1];
    outPixel->color.ui.b = pI[2];
    outPixel->color.ui.a = 1u;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A16B16G16R16UI(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16* pI = (gctUINT16*)inAddr[0];

    outPixel->color.ui.r = pI[0];
    outPixel->color.ui.g = pI[1];
    outPixel->color.ui.b = pI[2];
    outPixel->color.ui.a = pI[3];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_X16B16G16R16UI_2_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16* pI0 = (gctUINT16*)inAddr[0];
    gctUINT16* pI1 = (gctUINT16*)inAddr[1];

    outPixel->color.ui.r = pI0[0];
    outPixel->color.ui.g = pI0[1];
    outPixel->color.ui.b = pI1[0];
    outPixel->color.ui.a = 1u;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A16B16G16R16UI_2_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16* pI0 = (gctUINT16*)inAddr[0];
    gctUINT16* pI1 = (gctUINT16*)inAddr[1];

    outPixel->color.ui.r = pI0[0];
    outPixel->color.ui.g = pI0[1];
    outPixel->color.ui.b = pI1[0];
    outPixel->color.ui.a = pI1[1];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A32B32G32R32I(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT32* pI = (gctINT32*)inAddr[0];

    outPixel->color.i.r = pI[0];
    outPixel->color.i.g = pI[1];
    outPixel->color.i.b = pI[2];
    outPixel->color.i.a = pI[3];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A32B32G32R32I_2_G32R32X(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT32* pI0 = (gctINT32*)inAddr[0];
    gctINT32* pI1 = (gctINT32*)inAddr[1];

    outPixel->color.i.r = pI0[0];
    outPixel->color.i.g = pI0[1];
    outPixel->color.i.b = pI1[0];
    outPixel->color.i.a = pI1[1];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}


void _ReadPixelFrom_A32B32G32R32I_4_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT32* pI0 = (gctINT32*)inAddr[0];
    gctINT32* pI1 = (gctINT32*)inAddr[1];
    gctINT32* pI2 = (gctINT32*)inAddr[2];
    gctINT32* pI3 = (gctINT32*)inAddr[3];


    outPixel->color.i.r = pI0[0];
    outPixel->color.i.g = pI1[0];
    outPixel->color.i.b = pI2[0];
    outPixel->color.i.a = pI3[0];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_X32B32G32R32I_2_G32R32I(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctINT32* pI0 = (gctINT32*)inAddr[0];
    gctINT32* pI1 = (gctINT32*)inAddr[1];

    outPixel->color.i.r = pI0[0];
    outPixel->color.i.g = pI0[1];
    outPixel->color.i.b = pI1[0];
    outPixel->color.i.a = pI1[1];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A32B32G32R32UI(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT32* pI = (gctUINT32*)inAddr[0];

    outPixel->color.ui.r = pI[0];
    outPixel->color.ui.g = pI[1];
    outPixel->color.ui.b = pI[2];
    outPixel->color.ui.a = pI[3];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A32B32G32R32UI_2_G32R32X(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT32* pI0 = (gctUINT32*)inAddr[0];
    gctUINT32* pI1 = (gctUINT32*)inAddr[1];

    outPixel->color.ui.r = pI0[0];
    outPixel->color.ui.g = pI0[1];
    outPixel->color.ui.b = pI1[0];
    outPixel->color.ui.a = pI1[1];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A32B32G32R32UI_4_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT32* pI0 = (gctUINT32*)inAddr[0];
    gctUINT32* pI1 = (gctUINT32*)inAddr[1];
    gctUINT32* pI2 = (gctUINT32*)inAddr[2];
    gctUINT32* pI3 = (gctUINT32*)inAddr[3];


    outPixel->color.ui.r = pI0[0];
    outPixel->color.ui.g = pI1[0];
    outPixel->color.ui.b = pI2[0];
    outPixel->color.ui.a = pI3[0];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_X32B32G32R32UI_2_G32R32UI(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT32* pI0 = (gctUINT32*)inAddr[0];
    gctUINT32* pI1 = (gctUINT32*)inAddr[1];

    outPixel->color.ui.r = pI0[0];
    outPixel->color.ui.g = pI0[1];
    outPixel->color.ui.b = pI1[0];
    outPixel->color.ui.a = pI1[1];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}


void _ReadPixelFrom_A32B32G32R32F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctFLOAT* pf = (gctFLOAT*)inAddr[0];

    outPixel->color.f.r = pf[0];
    outPixel->color.f.g = pf[1];
    outPixel->color.f.b = pf[2];
    outPixel->color.f.a = pf[3];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A32B32G32R32F_2_G32R32F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctFLOAT* pf0 = (gctFLOAT*)inAddr[0];
    gctFLOAT* pf1 = (gctFLOAT*)inAddr[1];

    outPixel->color.f.r = pf0[0];
    outPixel->color.f.g = pf0[1];
    outPixel->color.f.b = pf1[0];
    outPixel->color.f.a = pf1[1];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_X32B32G32R32F_2_G32R32F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctFLOAT* pf0 = (gctFLOAT*)inAddr[0];
    gctFLOAT* pf1 = (gctFLOAT*)inAddr[1];

    outPixel->color.f.r = pf0[0];
    outPixel->color.f.g = pf0[1];
    outPixel->color.f.b = pf1[0];
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A32B32G32R32F_4_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctFLOAT* pf0 = (gctFLOAT*)inAddr[0];
    gctFLOAT* pf1 = (gctFLOAT*)inAddr[1];
    gctFLOAT* pf2 = (gctFLOAT*)inAddr[2];
    gctFLOAT* pf3 = (gctFLOAT*)inAddr[3];

    outPixel->color.f.r = pf0[0];
    outPixel->color.f.g = pf1[0];
    outPixel->color.f.b = pf2[0];
    outPixel->color.f.a = pf3[0];
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_X32B32G32R32F_4_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctFLOAT* pf0 = (gctFLOAT*)inAddr[0];
    gctFLOAT* pf1 = (gctFLOAT*)inAddr[1];
    gctFLOAT* pf2 = (gctFLOAT*)inAddr[2];

    outPixel->color.f.r = pf0[0];
    outPixel->color.f.g = pf1[0];
    outPixel->color.f.b = pf2[0];
    outPixel->color.f.a = 1.0;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_A2B10G10R10UI(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT32 ui = *(gctUINT32*)inAddr[0];

    outPixel->color.ui.r = gcdGET_FIELD(ui,  0, 10);
    outPixel->color.ui.g = gcdGET_FIELD(ui, 10, 10);
    outPixel->color.ui.b = gcdGET_FIELD(ui, 20, 10);
    outPixel->color.ui.a = gcdGET_FIELD(ui, 30,  2);
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_E5B9G9R9(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    const gctUINT32 mBits = 9;      /* mantissa bits */
    const gctUINT32 eBias = 15;     /* max allowed bias exponent */
    gctUINT32 ui = *(gctUINT32*)inAddr[0];
    gctFLOAT  scale = gcoMATH_Power(2.0f, (gctFLOAT)gcdGET_FIELD(ui, 27, 5) - eBias - mBits);

    outPixel->color.f.r = (gctFLOAT)gcdGET_FIELD(ui, 0,  9) * scale;
    outPixel->color.f.g = (gctFLOAT)gcdGET_FIELD(ui, 9,  9) * scale;
    outPixel->color.f.b = (gctFLOAT)gcdGET_FIELD(ui, 18, 9) * scale;
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}

void _ReadPixelFrom_B10G11R11(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctUINT16 r, g, b;
    gctUINT32 or, og, ob;
    gctUINT32 input = *(gctUINT32*) inAddr[0];
    r = input & 0x7ff;
    g = (input >> 11) & 0x7ff;
    b = (input >> 22) & 0x3ff;

    or = gcoMATH_Float11ToFloat(r);
    og = gcoMATH_Float11ToFloat(g);
    ob = gcoMATH_Float10ToFloat(b);

    outPixel->color.f.r = *(gctFLOAT*)&or;
    outPixel->color.f.g = *(gctFLOAT*)&og;
    outPixel->color.f.b = *(gctFLOAT*)&ob;
    outPixel->color.f.a = 1.0f;
    outPixel->d = 0.0f;
    outPixel->s = 0;
}
void _ReadPixelFrom_S8D32F_1_G32R32F(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctFLOAT* pF = (gctFLOAT*)inAddr[0];
    outPixel->d = pF[0];
    outPixel->s = ((gctUINT32)pF[1] & 0xFF);
}

void _ReadPixelFrom_S8D32F_2_A8R8G8B8(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel)
{
    gctFLOAT* pF = (gctFLOAT*)inAddr[0];
    gctUINT32* pUI = (gctUINT32*)inAddr[1];

    outPixel->d = pF[0];
    outPixel->s = (pUI[0] & 0xFF);
}

_PFNreadPixel gcoSURF_GetReadPixelFunc(gcoSURF surf)
{
    switch (surf->format)
    {
    case gcvSURF_A8:
        return _ReadPixelFrom_A8;
    case gcvSURF_A16:
        return _ReadPixelFrom_A16;
    case gcvSURF_A16F:
        return _ReadPixelFrom_A16F;
    case gcvSURF_A32F:
    case gcvSURF_A32F_1_R32F:
        return _ReadPixelFrom_A32F;

    case gcvSURF_L8:
        return _ReadPixelFrom_L8;
    case gcvSURF_L16:
        return _ReadPixelFrom_L16;
/*
    case gcvSURF_L32:
        return _ReadPixelFrom_L32;
*/
    case gcvSURF_L16F:
        return _ReadPixelFrom_L16F;
    case gcvSURF_L32F:
    case gcvSURF_L32F_1_R32F:
        return _ReadPixelFrom_L32F;

    case gcvSURF_A8L8:
        return _ReadPixelFrom_A8L8;
    case gcvSURF_A8L8_1_A8R8G8B8:
        return _ReadPixelFrom_A8L8_1_A8R8G8B8;
    case gcvSURF_A16L16:
        return _ReadPixelFrom_A16L16;
    case gcvSURF_A16L16F:
        return _ReadPixelFrom_A16L16F;
    case gcvSURF_A32L32F:
    case gcvSURF_A32L32F_1_G32R32F:
        return _ReadPixelFrom_A32L32F;


    case gcvSURF_R8:
        return _ReadPixelFrom_R8;
    case gcvSURF_R8_1_X8R8G8B8:
        return _ReadPixelFrom_R8_1_X8R8G8B8;
    case gcvSURF_R8_SNORM:
        return _ReadPixelFrom_R8_SNORM;
    case gcvSURF_R16:
        return _ReadPixelFrom_R16;

    case gcvSURF_R16F:
    case gcvSURF_R16F_1_A4R4G4B4:
        return _ReadPixelFrom_R16F;

    case gcvSURF_R32F:
    case gcvSURF_R32F_1_A8R8G8B8:
        return _ReadPixelFrom_R32F;

    case gcvSURF_G8R8:
        return _ReadPixelFrom_G8R8;
    case gcvSURF_G8R8_1_X8R8G8B8:
        return _ReadPixelFrom_G8R8_1_X8R8G8B8;
    case gcvSURF_G8R8_SNORM:
        return _ReadPixelFrom_G8R8_SNORM;
    case gcvSURF_G16R16:
        return _ReadPixelFrom_G16R16;

    case gcvSURF_G16R16F:
    case gcvSURF_G16R16F_1_A8R8G8B8:
        return _ReadPixelFrom_G16R16F;

    case gcvSURF_G32R32F:
        return _ReadPixelFrom_G32R32F;

    case gcvSURF_G32R32F_2_A8R8G8B8:
        return _ReadPixelFrom_G32R32F_2_A8R8G8B8;

    case gcvSURF_A4R4G4B4:
        return _ReadPixelFrom_A4R4G4B4;

    case gcvSURF_A1R5G5B5:
        return _ReadPixelFrom_A1R5G5B5;

    case gcvSURF_R5G6B5:
        return _ReadPixelFrom_R5G6B5;
    case gcvSURF_B8G8R8:
        return _ReadPixelFrom_B8G8R8;
    case gcvSURF_B8G8R8_SNORM:
        return _ReadPixelFrom_B8G8R8_SNORM;
    case gcvSURF_B16G16R16:
        return _ReadPixelFrom_B16G16R16;
    case gcvSURF_X4R4G4B4:
        return _ReadPixelFrom_X4R4G4B4;
    case gcvSURF_R4G4B4A4:
        return _ReadPixelFrom_R4G4B4A4;
    case gcvSURF_X1R5G5B5:
        return _ReadPixelFrom_X1R5G5B5;
    case gcvSURF_R5G5B5A1:
        return _ReadPixelFrom_R5G5B5A1;
    case gcvSURF_A8B8G8R8:
        return _ReadPixelFrom_A8B8G8R8;
    case gcvSURF_A8B8G8R8_SNORM:
        return _ReadPixelFrom_A8B8G8R8_SNORM;
    case gcvSURF_A8R8G8B8:
        return _ReadPixelFrom_A8R8G8B8;
    case gcvSURF_X8R8G8B8:
        return _ReadPixelFrom_X8R8G8B8;
    case gcvSURF_X8B8G8R8_SNORM:
        return _ReadPixelFrom_X8B8G8R8_SNORM;
    case gcvSURF_X2B10G10R10:
        return _ReadPixelFrom_X2B10G10R10;
    case gcvSURF_A2B10G10R10:
        return _ReadPixelFrom_A2B10G10R10;
    case gcvSURF_A16B16G16R16:
        return _ReadPixelFrom_A16B16G16R16;
    case gcvSURF_X16B16G16R16F:
        return _ReadPixelFrom_X16B16G16R16F;
    case gcvSURF_A16B16G16R16F:
        return _ReadPixelFrom_A16B16G16R16F;
    case gcvSURF_B16G16R16F:
        return _ReadPixelFrom_B16G16R16F;
    case gcvSURF_X16B16G16R16F_2_A8R8G8B8:
    case gcvSURF_B16G16R16F_2_A8R8G8B8:
        return _ReadPixelFrom_B16G16R16F_2_A8R8G8B8;
    case gcvSURF_A16B16G16R16F_2_A8R8G8B8:
        return _ReadPixelFrom_A16B16G16R16F_2_A8R8G8B8;
    case gcvSURF_A16B16G16R16F_2_G16R16F:
        return _ReadPixelFrom_A16B16G16R16F_2_G16R16F;

    case gcvSURF_B32G32R32F:
        return _ReadPixelFrom_B32G32R32F;
    case gcvSURF_B32G32R32F_3_A8R8G8B8:
        return _ReadPixelFrom_B32G32R32F_3_A8R8G8B8;

    case gcvSURF_A32B32G32R32F:
        return _ReadPixelFrom_A32B32G32R32F;

    case gcvSURF_A32B32G32R32F_2_G32R32F:
        return _ReadPixelFrom_A32B32G32R32F_2_G32R32F;
    case gcvSURF_X32B32G32R32F_2_G32R32F:
        return _ReadPixelFrom_X32B32G32R32F_2_G32R32F;
    case gcvSURF_A32B32G32R32F_4_A8R8G8B8:
        return _ReadPixelFrom_A32B32G32R32F_4_A8R8G8B8;
    case gcvSURF_X32B32G32R32F_4_A8R8G8B8:
        return _ReadPixelFrom_X32B32G32R32F_4_A8R8G8B8;

    case gcvSURF_D16:
        return _ReadPixelFrom_D16;
    case gcvSURF_D24X8:
        return _ReadPixelFrom_D24X8;
    case gcvSURF_D24S8:
        return _ReadPixelFrom_D24S8;
    case gcvSURF_X24S8:
        return _ReadPixelFrom_X24S8;
    case gcvSURF_D32:
        return _ReadPixelFrom_D32;
    case gcvSURF_D32F:
        return _ReadPixelFrom_D32F;

    case gcvSURF_S8D32F:
        return _ReadPixelFrom_S8D32F;

    case gcvSURF_S8:
        return _ReadPixelFrom_S8;

    case gcvSURF_S8D32F_1_G32R32F:
        return _ReadPixelFrom_S8D32F_1_G32R32F;
    case gcvSURF_S8D32F_2_A8R8G8B8:
        return _ReadPixelFrom_S8D32F_2_A8R8G8B8;
    case gcvSURF_D24S8_1_A8R8G8B8:
        return _ReadPixelFrom_D24S8;

    case gcvSURF_X24S8_1_A8R8G8B8:
        return _ReadPixelFrom_X24S8;

    case gcvSURF_A8B12G12R12_2_A8R8G8B8:
        return _ReadPixelFrom_A8B12G12R12_2_A8R8G8B8;

    /* Integer format. */
    case gcvSURF_R8I:
    case gcvSURF_R8I_1_A4R4G4B4:
        return _ReadPixelFrom_R8I;

    case gcvSURF_R8UI:
    case gcvSURF_R8UI_1_A4R4G4B4:
        return _ReadPixelFrom_R8UI;

    case gcvSURF_R16I:
    case gcvSURF_R16I_1_A4R4G4B4:
        return _ReadPixelFrom_R16I;

    case gcvSURF_R16UI:
    case gcvSURF_R16UI_1_A4R4G4B4:
        return _ReadPixelFrom_R16UI;

    case gcvSURF_R32I:
    case gcvSURF_R32I_1_A8R8G8B8:
        return _ReadPixelFrom_R32I;

    case gcvSURF_R32UI:
    case gcvSURF_R32UI_1_A8R8G8B8:
        return _ReadPixelFrom_R32UI;

    case gcvSURF_G8R8I:
    case gcvSURF_G8R8I_1_A4R4G4B4:
        return _ReadPixelFrom_G8R8I;

    case gcvSURF_G8R8UI:
    case gcvSURF_G8R8UI_1_A4R4G4B4:
        return _ReadPixelFrom_G8R8UI;

    case gcvSURF_G16R16I:
    case gcvSURF_G16R16I_1_A8R8G8B8:
        return _ReadPixelFrom_G16R16I;

    case gcvSURF_G16R16UI:
    case gcvSURF_G16R16UI_1_A8R8G8B8:
        return _ReadPixelFrom_G16R16UI;

    case gcvSURF_G32R32I:
    case gcvSURF_G32R32I_1_G32R32F:
        return _ReadPixelFrom_G32R32I;

    case gcvSURF_G32R32I_2_A8R8G8B8:
        return _ReadPixelFrom_G32R32I_2_A8R8G8B8;

    case gcvSURF_G32R32UI:
    case gcvSURF_G32R32UI_1_G32R32F:
        return _ReadPixelFrom_G32R32UI;

    case gcvSURF_G32R32UI_2_A8R8G8B8:
        return _ReadPixelFrom_G32R32UI_2_A8R8G8B8;

    case gcvSURF_B8G8R8I:
        return _ReadPixelFrom_B8G8R8I;
    case gcvSURF_B8G8R8I_1_A8R8G8B8:
        return _ReadPixelFrom_B8G8R8I_1_A8R8G8B8;

    case gcvSURF_B8G8R8UI:
        return _ReadPixelFrom_B8G8R8UI;
    case gcvSURF_B8G8R8UI_1_A8R8G8B8:
        return _ReadPixelFrom_B8G8R8UI_1_A8R8G8B8;

    case gcvSURF_B16G16R16I:
    case gcvSURF_B16G16R16I_1_G32R32F:
        return _ReadPixelFrom_B16G16R16I;

    case gcvSURF_B16G16R16I_2_A8R8G8B8:
        return _ReadPixelFrom_B16G16R16I_2_A8R8G8B8;

    case gcvSURF_B16G16R16UI:
    case gcvSURF_B16G16R16UI_1_G32R32F:
        return _ReadPixelFrom_B16G16R16UI;

    case gcvSURF_B16G16R16UI_2_A8R8G8B8:
        return _ReadPixelFrom_B16G16R16UI_2_A8R8G8B8;

    case gcvSURF_B32G32R32I:
        return _ReadPixelFrom_B32G32R32I;

    case gcvSURF_B32G32R32I_3_A8R8G8B8:
        return _ReadPixelFrom_B32G32R32I_3_A8R8G8B8;

    case gcvSURF_B32G32R32UI:
        return _ReadPixelFrom_B32G32R32UI;

    case gcvSURF_B32G32R32UI_3_A8R8G8B8:
        return _ReadPixelFrom_B32G32R32UI_3_A8R8G8B8;

    case gcvSURF_A8B8G8R8I:
        return _ReadPixelFrom_A8B8G8R8I;
    case gcvSURF_A8B8G8R8I_1_A8R8G8B8:
        return _ReadPixelFrom_A8B8G8R8I_1_A8R8G8B8;

    case gcvSURF_A8B8G8R8UI:
        return _ReadPixelFrom_A8B8G8R8UI;
    case gcvSURF_A8B8G8R8UI_1_A8R8G8B8:
        return _ReadPixelFrom_A8B8G8R8UI_1_A8R8G8B8;


    case gcvSURF_X16B16G16R16I:
    case gcvSURF_X16B16G16R16I_1_G32R32F:
        return _ReadPixelFrom_X16B16G16R16I;

    case gcvSURF_X16B16G16R16I_2_A8R8G8B8:
        return _ReadPixelFrom_X16B16G16R16I_2_A8R8G8B8;

    case gcvSURF_A16B16G16R16I:
    case gcvSURF_A16B16G16R16I_1_G32R32F:
        return _ReadPixelFrom_A16B16G16R16I;

    case gcvSURF_A16B16G16R16I_2_A8R8G8B8:
        return _ReadPixelFrom_A16B16G16R16I_2_A8R8G8B8;

    case gcvSURF_X16B16G16R16UI:
    case gcvSURF_X16B16G16R16UI_1_G32R32F:
        return _ReadPixelFrom_X16B16G16R16UI;

    case gcvSURF_X16B16G16R16UI_2_A8R8G8B8:
        return _ReadPixelFrom_X16B16G16R16UI_2_A8R8G8B8;

    case gcvSURF_A16B16G16R16UI:
    case gcvSURF_A16B16G16R16UI_1_G32R32F:
        return _ReadPixelFrom_A16B16G16R16UI;

    case gcvSURF_A16B16G16R16UI_2_A8R8G8B8:
        return _ReadPixelFrom_A16B16G16R16UI_2_A8R8G8B8;

    case gcvSURF_A32B32G32R32I:
        return _ReadPixelFrom_A32B32G32R32I;
    case gcvSURF_A32B32G32R32I_2_G32R32I:
    case gcvSURF_A32B32G32R32I_2_G32R32F:
        return _ReadPixelFrom_A32B32G32R32I_2_G32R32X;
    case gcvSURF_A32B32G32R32I_4_A8R8G8B8:
        return _ReadPixelFrom_A32B32G32R32I_4_A8R8G8B8;

    case gcvSURF_X32B32G32R32I_2_G32R32I:
        return _ReadPixelFrom_X32B32G32R32I_2_G32R32I;

    case gcvSURF_A32B32G32R32UI:
        return _ReadPixelFrom_A32B32G32R32UI;
    case gcvSURF_A32B32G32R32UI_2_G32R32UI:
    case gcvSURF_A32B32G32R32UI_2_G32R32F:
        return _ReadPixelFrom_A32B32G32R32UI_2_G32R32X;
    case gcvSURF_A32B32G32R32UI_4_A8R8G8B8:
        return _ReadPixelFrom_A32B32G32R32UI_4_A8R8G8B8;

    case gcvSURF_X32B32G32R32UI_2_G32R32UI:
        return _ReadPixelFrom_X32B32G32R32UI_2_G32R32UI;

    case gcvSURF_A2B10G10R10UI:
    case gcvSURF_A2B10G10R10UI_1_A8R8G8B8:
        return _ReadPixelFrom_A2B10G10R10UI;

    case gcvSURF_SBGR8:
        return _ReadPixelFrom_B8G8R8;

    case gcvSURF_A8_SBGR8:
        return _ReadPixelFrom_A8B8G8R8;

    case gcvSURF_A8_SRGB8:
        return _ReadPixelFrom_A8R8G8B8;

    case gcvSURF_E5B9G9R9:
        return _ReadPixelFrom_E5B9G9R9;

    case gcvSURF_B10G11R11F:
    case gcvSURF_B10G11R11F_1_A8R8G8B8:
        return _ReadPixelFrom_B10G11R11;

    case gcvSURF_X8_SRGB8:
        return _ReadPixelFrom_X8R8G8B8;

    default:
        gcmASSERT(0);
    }

    return gcvNULL;
}


void _WritePixelTo_A8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctUINT8*)outAddr[0] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.a, 8);
}

void _WritePixelTo_L8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctUINT8*)outAddr[0] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.r, 8);
}

void _WritePixelTo_R8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctUINT8*)outAddr[0] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.r, 8);
}

void _WritePixelTo_R8_1_X8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT8 *pUB = (gctUINT8*)outAddr[0];

    pUB[0] = (gctUINT8)gcdFLOAT_TO_UNORM(0.0f, 8);
    pUB[1] = (gctUINT8)gcdFLOAT_TO_UNORM(0.0f, 8);
    pUB[2] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.r, 8);
    pUB[3] = (gctUINT8)gcdFLOAT_TO_UNORM(1.0f, 8);
}

void _WritePixelTo_A8L8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT8 *pUB = (gctUINT8*)outAddr[0];

    pUB[0] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.r, 8);
    pUB[1] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.a, 8);
}

void _WritePixelTo_A8L8_1_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT8 *pUB = (gctUINT8*)outAddr[0];

    pUB[0] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.r, 8);
    pUB[1] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.r, 8);
    pUB[2] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.r, 8);
    pUB[3] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.a, 8);
}

void _WritePixelTo_G8R8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT8 *pUB = (gctUINT8*)outAddr[0];

    pUB[0] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.r, 8);
    pUB[1] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.g, 8);
}

void _WritePixelTo_G8R8_1_X8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT8 *pUB = (gctUINT8*)outAddr[0];

    pUB[0] = (gctUINT8)gcdFLOAT_TO_UNORM(0.0, 8);
    pUB[1] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.g, 8);
    pUB[2] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.r, 8);
    pUB[3] = (gctUINT8)gcdFLOAT_TO_UNORM(1.0f, 8);
}

void _WritePixelTo_R5G6B5(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctUINT16*)outAddr[0] = (gctUINT16)(gcdFLOAT_TO_UNORM(inPixel->color.f.r, 5) << 11 |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.g, 6) << 5  |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.b, 5));
}

void _WritePixelTo_X4R4G4B4(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctUINT16*)outAddr[0] = (gctUINT16)(gcdFLOAT_TO_UNORM(inPixel->color.f.r, 4) << 8 |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.g, 4) << 4 |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.b, 4));
}

void _WritePixelTo_A4R4G4B4(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctUINT16*)outAddr[0] = (gctUINT16)(gcdFLOAT_TO_UNORM(inPixel->color.f.a, 4) << 12 |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.r, 4) << 8  |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.g, 4) << 4  |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.b, 4));
}

void _WritePixelTo_R4G4B4A4(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctUINT16*)outAddr[0] = (gctUINT16)(gcdFLOAT_TO_UNORM(inPixel->color.f.r, 4) << 12 |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.g, 4) << 8  |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.b, 4) << 4  |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.a, 4));
}

void _WritePixelTo_X1R5G5B5(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctUINT16*)outAddr[0] = (gctUINT16)(gcdFLOAT_TO_UNORM(inPixel->color.f.r, 5) << 10 |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.g, 5) << 5  |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.b, 5));
}

void _WritePixelTo_A1R5G5B5(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctUINT16*)outAddr[0] = (gctUINT16)(gcdFLOAT_TO_UNORM(inPixel->color.f.a, 1) << 15 |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.r, 5) << 10 |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.g, 5) << 5  |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.b, 5));
}

void _WritePixelTo_X8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT8 *pUB = (gctUINT8*)outAddr[0];

    pUB[0] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.b, 8);
    pUB[1] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.g, 8);
    pUB[2] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.r, 8);
    pUB[3] = 0xFF;
}

void _WritePixelTo_A8B8G8R8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT8 *pUB = (gctUINT8*)outAddr[0];

    pUB[0] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.r, 8);
    pUB[1] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.g, 8);
    pUB[2] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.b, 8);
    pUB[3] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.a, 8);
}

void _WritePixelTo_X8B8G8R8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT8 *pUB = (gctUINT8*)outAddr[0];

    pUB[0] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.r, 8);
    pUB[1] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.g, 8);
    pUB[2] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.b, 8);
    pUB[3] = (gctUINT8)gcdFLOAT_TO_UNORM(1.0, 8);
}

void  _WritePixelTo_A32L32F(gcsPIXEL* inPixel, gctPOINTER outAddr[4], gctUINT flags)
{
    gctFLOAT* pI = (gctFLOAT*)outAddr[0];

    pI[0] = (gctFLOAT)(inPixel->color.f.r);
    pI[1] = (gctFLOAT)(inPixel->color.f.a);
}
void  _WritePixelTo_L32F(gcsPIXEL* inPixel, gctPOINTER outAddr[4], gctUINT flags)
{
    gctFLOAT* pI = (gctFLOAT*)outAddr[0];
    pI[0] = (gctFLOAT)(inPixel->color.f.r);
}
void  _WritePixelTo_A32F(gcsPIXEL* inPixel, gctPOINTER outAddr[4], gctUINT flags)
{
   gctFLOAT* pI = (gctFLOAT*)outAddr[0];
    pI[0] = (gctFLOAT)(inPixel->color.f.a);
}

void _WritePixelTo_B8G8R8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT8 *pUB = (gctUINT8*)outAddr[0];

    pUB[0] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.r, 8);
    pUB[1] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.g, 8);
    pUB[2] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.b, 8);
}


void _WritePixelTo_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT8 *pUB = (gctUINT8*)outAddr[0];

    pUB[0] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.b, 8);
    pUB[1] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.g, 8);
    pUB[2] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.r, 8);
    pUB[3] = (gctUINT8)gcdFLOAT_TO_UNORM(inPixel->color.f.a, 8);
}

void _WritePixelTo_X2B10G10R10(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctUINT32*)outAddr[0] = (gctUINT32)(gcdFLOAT_TO_UNORM(inPixel->color.f.b, 10) << 20 |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.g, 10) << 10 |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.r, 10));
}

void _WritePixelTo_A2B10G10R10(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctUINT32*)outAddr[0] = (gctUINT32)(gcdFLOAT_TO_UNORM(inPixel->color.f.a,  2) << 30 |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.b, 10) << 20 |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.g, 10) << 10 |
                                          gcdFLOAT_TO_UNORM(inPixel->color.f.r, 10));
}

void _WritePixelTo_A8B12G12R12_2_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT8 *pUB0 = (gctUINT8*)outAddr[0];
    gctUINT8 *pUB1 = (gctUINT8*)outAddr[1];
    gctUINT32 r = gcdFLOAT_TO_UNORM(inPixel->color.f.r, 12);
    gctUINT32 g = gcdFLOAT_TO_UNORM(inPixel->color.f.g, 12);
    gctUINT32 b = gcdFLOAT_TO_UNORM(inPixel->color.f.b, 12);
    gctUINT32 a = gcdFLOAT_TO_UNORM(inPixel->color.f.a, 8);

    pUB0[0] = (gctUINT8) ((b & 0xF00) >> 4);
    pUB0[1] = (gctUINT8) ((g & 0xF00) >> 4);
    pUB0[2] = (gctUINT8) ((r & 0xF00) >> 4);
    pUB0[3] = (gctUINT8) a;

    pUB1[0] = (gctUINT8) (b & 0xFF);
    pUB1[1] = (gctUINT8) (g & 0xFF);
    pUB1[2] = (gctUINT8) (r & 0xFF);
    pUB1[3] = (gctUINT8) a;
}

void _WritePixelTo_D16(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctUINT16*)outAddr[0] = (gctUINT16)gcdFLOAT_TO_UNORM(inPixel->d, 16);
}

void _WritePixelTo_D24X8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT32 depth = gcdFLOAT_TO_UNORM(inPixel->d, 24);

    /* Float cannot hold 24bit precision, and the nearest rounding sometimes
    ** would cause the value larger than maximum, especially on ARM platform.
    */
    if (depth > (gctUINT32)gcdUNORM_MAX(24))
    {
        depth = (gctUINT32)gcdUNORM_MAX(24);
    }

    *(gctUINT32*)outAddr[0] = (gctUINT32)(depth << 8);
}

void _WritePixelTo_D24S8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT32 depth, stencil;

    if (flags & gcvBLIT_FLAG_SKIP_DEPTH_WRITE)
    {
        depth = (*(gctUINT32*)outAddr[0]) >> 8;
    }
    else
    {
        depth = gcdFLOAT_TO_UNORM(inPixel->d, 24);

        /* Float cannot hold 24bit precision, and the nearest rounding sometimes
        ** would cause the value larger than maximum, especially on ARM platform.
        */
        if (depth > (gctUINT32)gcdUNORM_MAX(24))
        {
            depth = (gctUINT32)gcdUNORM_MAX(24);
        }
    }

    if (flags & gcvBLIT_FLAG_SKIP_STENCIL_WRITE)
    {
        stencil = (*(gctUINT32*)outAddr[0]) & 0xFF;
    }
    else
    {
        stencil = (inPixel->s);
    }

    *(gctUINT32*)outAddr[0] = (gctUINT32)(depth << 8 | stencil);
}

void _WritePixelTo_X24S8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT32 depth , stencil;
    depth = 0;

    if (flags & gcvBLIT_FLAG_SKIP_STENCIL_WRITE)
    {
        stencil = (*(gctUINT32*)outAddr[0]) & 0xFF;
    }
    else
    {
        stencil = (gctUINT32)gcmMIN(inPixel->s, gcdUINT_MAX(8));
    }

    *(gctUINT32*)outAddr[0] = (gctUINT32)(depth << 8 | stencil);
}

void _WritePixelTo_D32(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT64 depth = (gctUINT64)(inPixel->d * gcdUNORM_MAX(32) + 0.5f);

    /* Float cannot hold 32bit precision, and the nearest rounding sometimes
    ** would cause the value larger than maximum, especially on ARM platform.
    */
    if (depth > (gctUINT64)gcdUNORM_MAX(32))
    {
        depth = (gctUINT64)gcdUNORM_MAX(32);
    }

    *(gctUINT32*)outAddr[0] = (gctUINT32)gcdFLOAT_TO_UNORM(inPixel->d, 32);
}

void _WritePixelTo_D32F(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctFLOAT*)outAddr[0] = gcmCLAMP(inPixel->d, 0.0f, 1.0f);
}

void _WritePixelTo_S8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctUINT8*)outAddr[0] = (gctUINT8)gcmMIN(inPixel->s, gcdUINT_MAX(8));
}


/* SNORM format. */
void _WritePixelTo_R8_SNORM(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctINT8*)outAddr[0] = (gctINT8)gcdFLOAT_TO_SNORM(inPixel->color.f.r, 8);
}

void _WritePixelTo_G8R8_SNORM(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT8 *pB = (gctINT8*)outAddr[0];

    pB[0] = (gctINT8)gcdFLOAT_TO_SNORM(inPixel->color.f.r, 8);
    pB[1] = (gctINT8)gcdFLOAT_TO_SNORM(inPixel->color.f.g, 8);
}

void _WritePixelTo_B8G8R8_SNORM(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT8 *pB = (gctINT8*)outAddr[0];

    pB[0] = (gctINT8)gcdFLOAT_TO_SNORM(inPixel->color.f.r, 8);
    pB[1] = (gctINT8)gcdFLOAT_TO_SNORM(inPixel->color.f.g, 8);
    pB[2] = (gctINT8)gcdFLOAT_TO_SNORM(inPixel->color.f.b, 8);
}

void _WritePixelTo_A8B8G8R8_SNORM(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT8 *pB = (gctINT8*)outAddr[0];

    pB[0] = (gctINT8)gcdFLOAT_TO_SNORM(inPixel->color.f.r, 8);
    pB[1] = (gctINT8)gcdFLOAT_TO_SNORM(inPixel->color.f.g, 8);
    pB[2] = (gctINT8)gcdFLOAT_TO_SNORM(inPixel->color.f.b, 8);
    pB[3] = (gctINT8)gcdFLOAT_TO_SNORM(inPixel->color.f.a, 8);
}

void _WritePixelTo_X8B8G8R8_SNORM(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT8 *pB = (gctINT8*)outAddr[0];

    pB[0] = (gctINT8)gcdFLOAT_TO_SNORM(inPixel->color.f.r, 8);
    pB[1] = (gctINT8)gcdFLOAT_TO_SNORM(inPixel->color.f.g, 8);
    pB[2] = (gctINT8)gcdFLOAT_TO_SNORM(inPixel->color.f.b, 8);
    pB[3] = (gctINT8)gcdFLOAT_TO_SNORM(1.0f, 8);
}


/* Integer format. */
void _WritePixelTo_R8I(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctINT8*)outAddr[0] = (gctINT8)gcmCLAMP(inPixel->color.i.r, gcdINT_MIN(8), gcdINT_MAX(8));
}

void _WritePixelTo_R8UI(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctUINT8*)outAddr[0] = (gctUINT8)gcmMIN(inPixel->color.ui.r, gcdUINT_MAX(8));
}

void _WritePixelTo_R16I(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctINT16*)outAddr[0] = (gctINT16)gcmCLAMP(inPixel->color.i.r, gcdINT_MIN(16), gcdINT_MAX(16));
}

void _WritePixelTo_R16UI(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctUINT16*)outAddr[0] = (gctUINT16)gcmMIN(inPixel->color.ui.r, gcdUINT_MAX(16));
}

void _WritePixelTo_R16F(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctUINT16*)outAddr[0] = gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.r);
}

void _WritePixelTo_R32I(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctINT32*)outAddr[0] = (gctINT32)(inPixel->color.i.r);
}

void _WritePixelTo_R32UI(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctUINT32*)outAddr[0] = (gctUINT32)(inPixel->color.ui.r);
}

void _WritePixelTo_R32F(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    *(gctFLOAT*)outAddr[0] = (gctFLOAT)(inPixel->color.f.r);
}

void _WritePixelTo_G8R8I(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT8* pI = (gctINT8*)outAddr[0];

    pI[0] = (gctINT8)gcmCLAMP(inPixel->color.i.r, gcdINT_MIN(8), gcdINT_MAX(8));
    pI[1] = (gctINT8)gcmCLAMP(inPixel->color.i.g, gcdINT_MIN(8), gcdINT_MAX(8));
}

void _WritePixelTo_G8R8UI(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT8* pI = (gctUINT8*)outAddr[0];

    pI[0] = (gctUINT8)gcmMIN(inPixel->color.ui.r, gcdUINT_MAX(8));
    pI[1] = (gctUINT8)gcmMIN(inPixel->color.ui.g, gcdUINT_MAX(8));
}

void _WritePixelTo_G16R16I(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT16* pI = (gctINT16*)outAddr[0];

    pI[0] = (gctINT16)gcmCLAMP(inPixel->color.i.r, gcdINT_MIN(16), gcdINT_MAX(16));
    pI[1] = (gctINT16)gcmCLAMP(inPixel->color.i.g, gcdINT_MIN(16), gcdINT_MAX(16));
}

void _WritePixelTo_G16R16UI(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT16* pI = (gctUINT16*)outAddr[0];

    pI[0] = (gctUINT16)gcmMIN(inPixel->color.ui.r, gcdUINT_MAX(16));
    pI[1] = (gctUINT16)gcmMIN(inPixel->color.ui.g, gcdUINT_MAX(16));
}

void _WritePixelTo_G16R16F(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT16* pI = (gctUINT16*)outAddr[0];

    pI[0] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.r);
    pI[1] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.g);
}

void _WritePixelTo_G32R32I(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT32* pI = (gctINT32*)outAddr[0];

    pI[0] = (gctINT32)(inPixel->color.i.r);
    pI[1] = (gctINT32)(inPixel->color.i.g);
}

void _WritePixelTo_G32R32I_2_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT32* pI0 = (gctINT32*)outAddr[0];
    gctINT32* pI1 = (gctINT32*)outAddr[1];

    pI0[0] = (gctINT32)(inPixel->color.i.r);
    pI1[0] = (gctINT32)(inPixel->color.i.g);
}

void _WritePixelTo_G32R32UI(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT32* pI = (gctUINT32*)outAddr[0];

    pI[0] = (gctUINT32)(inPixel->color.ui.r);
    pI[1] = (gctUINT32)(inPixel->color.ui.g);
}

void _WritePixelTo_G32R32UI_2_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT32* pI0 = (gctUINT32*)outAddr[0];
    gctUINT32* pI1 = (gctUINT32*)outAddr[1];

    pI0[0] = (gctUINT32)(inPixel->color.ui.r);
    pI1[0] = (gctUINT32)(inPixel->color.ui.g);
}

void _WritePixelTo_G32R32F(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctFLOAT* pI = (gctFLOAT*)outAddr[0];

    pI[0] = (gctFLOAT)(inPixel->color.f.r);
    pI[1] = (gctFLOAT)(inPixel->color.f.g);
}

void _WritePixelTo_G32R32F_2_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctFLOAT* pI0 = (gctFLOAT*)outAddr[0];
    gctFLOAT* pI1 = (gctFLOAT*)outAddr[1];

    pI0[0] = (gctFLOAT)(inPixel->color.f.r);
    pI1[0] = (gctFLOAT)(inPixel->color.f.g);
}

void _WritePixelTo_B8G8R8I(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT8* pI = (gctINT8*)outAddr[0];

    pI[0] = (gctINT8)gcmCLAMP(inPixel->color.i.r, gcdINT_MIN(8), gcdINT_MAX(8));
    pI[1] = (gctINT8)gcmCLAMP(inPixel->color.i.g, gcdINT_MIN(8), gcdINT_MAX(8));
    pI[2] = (gctINT8)gcmCLAMP(inPixel->color.i.b, gcdINT_MIN(8), gcdINT_MAX(8));
}

void _WritePixelTo_B8G8R8I_1_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT8* pI = (gctINT8*)outAddr[0];

    pI[0] = (gctINT8)gcmCLAMP(inPixel->color.i.b, gcdINT_MIN(8), gcdINT_MAX(8));
    pI[1] = (gctINT8)gcmCLAMP(inPixel->color.i.g, gcdINT_MIN(8), gcdINT_MAX(8));
    pI[2] = (gctINT8)gcmCLAMP(inPixel->color.i.r, gcdINT_MIN(8), gcdINT_MAX(8));
}

void _WritePixelTo_B8G8R8UI(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT8* pI = (gctUINT8*)outAddr[0];

    pI[0] = (gctUINT8)gcmMIN(inPixel->color.ui.r, gcdUINT_MAX(8));
    pI[1] = (gctUINT8)gcmMIN(inPixel->color.ui.g, gcdUINT_MAX(8));
    pI[2] = (gctUINT8)gcmMIN(inPixel->color.ui.b, gcdUINT_MAX(8));
}

void _WritePixelTo_B8G8R8UI_1_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT8* pI = (gctUINT8*)outAddr[0];

    pI[0] = (gctUINT8)gcmMIN(inPixel->color.ui.b, gcdUINT_MAX(8));
    pI[1] = (gctUINT8)gcmMIN(inPixel->color.ui.g, gcdUINT_MAX(8));
    pI[2] = (gctUINT8)gcmMIN(inPixel->color.ui.r, gcdUINT_MAX(8));
}

void _WritePixelTo_B16G16R16I(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT16* pI = (gctINT16*)outAddr[0];

    pI[0] = (gctINT16)gcmCLAMP(inPixel->color.i.r, gcdINT_MIN(16), gcdINT_MAX(16));
    pI[1] = (gctINT16)gcmCLAMP(inPixel->color.i.g, gcdINT_MIN(16), gcdINT_MAX(16));
    pI[2] = (gctINT16)gcmCLAMP(inPixel->color.i.b, gcdINT_MIN(16), gcdINT_MAX(16));
}

void _WritePixelTo_B16G16R16I_2_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT16* pI0 = (gctINT16*)outAddr[0];
    gctINT16* pI1 = (gctINT16*)outAddr[1];

    pI0[0] = (gctINT16)gcmCLAMP(inPixel->color.i.r, gcdINT_MIN(16), gcdINT_MAX(16));
    pI0[1] = (gctINT16)gcmCLAMP(inPixel->color.i.g, gcdINT_MIN(16), gcdINT_MAX(16));
    pI1[0] = (gctINT16)gcmCLAMP(inPixel->color.i.b, gcdINT_MIN(16), gcdINT_MAX(16));
}

void _WritePixelTo_B16G16R16UI(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT16* pI = (gctUINT16*)outAddr[0];

    pI[0] = (gctUINT16)gcmMIN(inPixel->color.ui.r, gcdUINT_MAX(16));
    pI[1] = (gctUINT16)gcmMIN(inPixel->color.ui.g, gcdUINT_MAX(16));
    pI[2] = (gctUINT16)gcmMIN(inPixel->color.ui.b, gcdUINT_MAX(16));
}

void _WritePixelTo_B16G16R16UI_2_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT16* pI0 = (gctUINT16*)outAddr[0];
    gctUINT16* pI1 = (gctUINT16*)outAddr[1];

    pI0[0] = (gctUINT16)gcmMIN(inPixel->color.ui.r, gcdUINT_MAX(16));
    pI0[1] = (gctUINT16)gcmMIN(inPixel->color.ui.g, gcdUINT_MAX(16));
    pI1[0] = (gctUINT16)gcmMIN(inPixel->color.ui.b, gcdUINT_MAX(16));
}

void _WritePixelTo_B32G32R32I(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT32* pI = (gctINT32*)outAddr[0];

    pI[0] = (gctINT32)(inPixel->color.i.r);
    pI[1] = (gctINT32)(inPixel->color.i.g);
    pI[2] = (gctINT32)(inPixel->color.i.b);
}

void _WritePixelTo_B32G32R32I_3_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT32* pI0 = (gctINT32*)outAddr[0];
    gctINT32* pI1 = (gctINT32*)outAddr[1];
    gctINT32* pI2 = (gctINT32*)outAddr[2];

    pI0[0] = (gctINT32)(inPixel->color.i.r);
    pI1[0] = (gctINT32)(inPixel->color.i.g);
    pI2[0] = (gctINT32)(inPixel->color.i.b);
}

void _WritePixelTo_B32G32R32UI(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT32* pI = (gctUINT32*)outAddr[0];

    pI[0] = (gctUINT32)(inPixel->color.ui.r);
    pI[1] = (gctUINT32)(inPixel->color.ui.g);
    pI[2] = (gctUINT32)(inPixel->color.ui.b);
}

void _WritePixelTo_B32G32R32UI_3_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT32* pI0 = (gctUINT32*)outAddr[0];
    gctUINT32* pI1 = (gctUINT32*)outAddr[1];
    gctUINT32* pI2 = (gctUINT32*)outAddr[2];

    pI0[0] = (gctUINT32)(inPixel->color.ui.r);
    pI1[0] = (gctUINT32)(inPixel->color.ui.g);
    pI2[0] = (gctUINT32)(inPixel->color.ui.b);
}

void _WritePixelTo_A8B8G8R8I(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT8* pI = (gctINT8*)outAddr[0];

    pI[0] = (gctINT8)gcmCLAMP(inPixel->color.i.r, gcdINT_MIN(8), gcdINT_MAX(8));
    pI[1] = (gctINT8)gcmCLAMP(inPixel->color.i.g, gcdINT_MIN(8), gcdINT_MAX(8));
    pI[2] = (gctINT8)gcmCLAMP(inPixel->color.i.b, gcdINT_MIN(8), gcdINT_MAX(8));
    pI[3] = (gctINT8)gcmCLAMP(inPixel->color.i.a, gcdINT_MIN(8), gcdINT_MAX(8));
}

void _WritePixelTo_A8B8G8R8I_1_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT8* pI = (gctINT8*)outAddr[0];

    pI[0] = (gctINT8)gcmCLAMP(inPixel->color.i.b, gcdINT_MIN(8), gcdINT_MAX(8));
    pI[1] = (gctINT8)gcmCLAMP(inPixel->color.i.g, gcdINT_MIN(8), gcdINT_MAX(8));
    pI[2] = (gctINT8)gcmCLAMP(inPixel->color.i.r, gcdINT_MIN(8), gcdINT_MAX(8));
    pI[3] = (gctINT8)gcmCLAMP(inPixel->color.i.a, gcdINT_MIN(8), gcdINT_MAX(8));
}

void _WritePixelTo_X8B8G8R8I(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT8* pI = (gctINT8*)outAddr[0];

    pI[0] = (gctINT8)gcmCLAMP(inPixel->color.i.r, gcdINT_MIN(8), gcdINT_MAX(8));
    pI[1] = (gctINT8)gcmCLAMP(inPixel->color.i.g, gcdINT_MIN(8), gcdINT_MAX(8));
    pI[2] = (gctINT8)gcmCLAMP(inPixel->color.i.b, gcdINT_MIN(8), gcdINT_MAX(8));
    pI[3] = (gctINT8)1;
}

void _WritePixelTo_A8B8G8R8UI(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT8* pI = (gctUINT8*)outAddr[0];

    pI[0] = (gctUINT8)gcmMIN(inPixel->color.ui.r, gcdUINT_MAX(8));
    pI[1] = (gctUINT8)gcmMIN(inPixel->color.ui.g, gcdUINT_MAX(8));
    pI[2] = (gctUINT8)gcmMIN(inPixel->color.ui.b, gcdUINT_MAX(8));
    pI[3] = (gctUINT8)gcmMIN(inPixel->color.ui.a, gcdUINT_MAX(8));
}

void _WritePixelTo_A8B8G8R8UI_1_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT8* pI = (gctUINT8*)outAddr[0];

    pI[0] = (gctUINT8)gcmMIN(inPixel->color.ui.b, gcdUINT_MAX(8));
    pI[1] = (gctUINT8)gcmMIN(inPixel->color.ui.g, gcdUINT_MAX(8));
    pI[2] = (gctUINT8)gcmMIN(inPixel->color.ui.r, gcdUINT_MAX(8));
    pI[3] = (gctUINT8)gcmMIN(inPixel->color.ui.a, gcdUINT_MAX(8));
}

void _WritePixelTo_X8B8G8R8UI(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT8* pI = (gctUINT8*)outAddr[0];

    pI[0] = (gctUINT8)gcmMIN(inPixel->color.ui.r, gcdUINT_MAX(8));
    pI[1] = (gctUINT8)gcmMIN(inPixel->color.ui.g, gcdUINT_MAX(8));
    pI[2] = (gctUINT8)gcmMIN(inPixel->color.ui.b, gcdUINT_MAX(8));
    pI[3] = (gctUINT8)1;
}

void _WritePixelTo_A16B16G16R16I(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT16* pI = (gctINT16*)outAddr[0];

    pI[0] = (gctINT16)gcmCLAMP(inPixel->color.i.r, gcdINT_MIN(16), gcdINT_MAX(16));
    pI[1] = (gctINT16)gcmCLAMP(inPixel->color.i.g, gcdINT_MIN(16), gcdINT_MAX(16));
    pI[2] = (gctINT16)gcmCLAMP(inPixel->color.i.b, gcdINT_MIN(16), gcdINT_MAX(16));
    pI[3] = (gctINT16)gcmCLAMP(inPixel->color.i.a, gcdINT_MIN(16), gcdINT_MAX(16));
}

void _WritePixelTo_A16B16G16R16I_2_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT16* pI0 = (gctINT16*)outAddr[0];
    gctINT16* pI1 = (gctINT16*)outAddr[1];

    pI0[0] = (gctINT16)gcmCLAMP(inPixel->color.i.r, gcdINT_MIN(16), gcdINT_MAX(16));
    pI0[1] = (gctINT16)gcmCLAMP(inPixel->color.i.g, gcdINT_MIN(16), gcdINT_MAX(16));
    pI1[0] = (gctINT16)gcmCLAMP(inPixel->color.i.b, gcdINT_MIN(16), gcdINT_MAX(16));
    pI1[1] = (gctINT16)gcmCLAMP(inPixel->color.i.a, gcdINT_MIN(16), gcdINT_MAX(16));
}

void _WritePixelTo_X16B16G16R16I(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT16* pI = (gctINT16*)outAddr[0];

    pI[0] = (gctINT16)gcmCLAMP(inPixel->color.i.r, gcdINT_MIN(16), gcdINT_MAX(16));
    pI[1] = (gctINT16)gcmCLAMP(inPixel->color.i.g, gcdINT_MIN(16), gcdINT_MAX(16));
    pI[2] = (gctINT16)gcmCLAMP(inPixel->color.i.b, gcdINT_MIN(16), gcdINT_MAX(16));
    pI[3] = (gctINT16)1;
}

void _WritePixelTo_X16B16G16R16I_2_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT16* pI0 = (gctINT16*)outAddr[0];
    gctINT16* pI1 = (gctINT16*)outAddr[1];

    pI0[0] = (gctINT16)gcmCLAMP(inPixel->color.i.r, gcdINT_MIN(16), gcdINT_MAX(16));
    pI0[1] = (gctINT16)gcmCLAMP(inPixel->color.i.g, gcdINT_MIN(16), gcdINT_MAX(16));
    pI1[0] = (gctINT16)gcmCLAMP(inPixel->color.i.b, gcdINT_MIN(16), gcdINT_MAX(16));
    pI1[1] = (gctINT16)1;
}

void _WritePixelTo_A16B16G16R16UI(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT16* pI = (gctUINT16*)outAddr[0];

    pI[0] = (gctUINT16)gcmMIN(inPixel->color.ui.r, gcdUINT_MAX(16));
    pI[1] = (gctUINT16)gcmMIN(inPixel->color.ui.g, gcdUINT_MAX(16));
    pI[2] = (gctUINT16)gcmMIN(inPixel->color.ui.b, gcdUINT_MAX(16));
    pI[3] = (gctUINT16)gcmMIN(inPixel->color.ui.a, gcdUINT_MAX(16));
}

void _WritePixelTo_A16B16G16R16UI_2_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT16* pI0 = (gctUINT16*)outAddr[0];
    gctUINT16* pI1 = (gctUINT16*)outAddr[1];

    pI0[0] = (gctUINT16)gcmMIN(inPixel->color.ui.r, gcdUINT_MAX(16));
    pI0[1] = (gctUINT16)gcmMIN(inPixel->color.ui.g, gcdUINT_MAX(16));
    pI1[0] = (gctUINT16)gcmMIN(inPixel->color.ui.b, gcdUINT_MAX(16));
    pI1[1] = (gctUINT16)gcmMIN(inPixel->color.ui.a, gcdUINT_MAX(16));
}

void _WritePixelTo_X16B16G16R16UI(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT16* pI = (gctUINT16*)outAddr[0];

    pI[0] = (gctUINT16)gcmMIN(inPixel->color.ui.r, gcdUINT_MAX(16));
    pI[1] = (gctUINT16)gcmMIN(inPixel->color.ui.g, gcdUINT_MAX(16));
    pI[2] = (gctUINT16)gcmMIN(inPixel->color.ui.b, gcdUINT_MAX(16));
    pI[3] = (gctUINT16)1;
}

void _WritePixelTo_X16B16G16R16UI_2_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT16* pI0 = (gctUINT16*)outAddr[0];
    gctUINT16* pI1 = (gctUINT16*)outAddr[1];

    pI0[0] = (gctUINT16)gcmMIN(inPixel->color.ui.r, gcdUINT_MAX(16));
    pI0[1] = (gctUINT16)gcmMIN(inPixel->color.ui.g, gcdUINT_MAX(16));
    pI1[0] = (gctUINT16)gcmMIN(inPixel->color.ui.b, gcdUINT_MAX(16));
    pI1[1] = (gctUINT16)1;
}

void _WritePixelTo_A16B16G16R16F(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT16* pI = (gctUINT16*)outAddr[0];

    pI[0] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.r);
    pI[1] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.g);
    pI[2] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.b);
    pI[3] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.a);
}

void _WritePixelTo_B16G16R16F(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT16* pI = (gctUINT16*)outAddr[0];

    pI[0] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.r);
    pI[1] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.g);
    pI[2] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.b);
}

void _WritePixelTo_A16B16G16R16F_2_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT16* pI0 = (gctUINT16*)outAddr[0];
    gctUINT16* pI1 = (gctUINT16*)outAddr[1];

    pI0[0] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.r);
    pI0[1] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.g);
    pI1[0] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.b);
    pI1[1] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.a);
}

void _WritePixelTo_A16B16G16R16F_2_G16R16F(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT16* pI0 = (gctUINT16*)outAddr[0];
    gctUINT16* pI1 = (gctUINT16*)outAddr[1];

    pI0[0] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.r);
    pI0[1] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.g);
    pI1[0] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.b);
    pI1[1] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.a);
}

void _WritePixelTo_B16G16R16F_2_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT16* pI0 = (gctUINT16*)outAddr[0];
    gctUINT16* pI1 = (gctUINT16*)outAddr[1];

    pI0[0] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.r);
    pI0[1] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.g);
    pI1[0] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.b);
    pI1[1] = (gctUINT16)1;
}

void _WritePixelTo_A16B16G16R16F_2_A8R8B8G8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT16* pI0 = (gctUINT16*)outAddr[0];
    gctUINT16* pI1 = (gctUINT16*)outAddr[1];

    pI0[0] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.r);
    pI0[1] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.g);
    pI1[0] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.b);
    pI1[1] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.a);
}

void _WritePixelTo_X16B16G16R16F(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT16* pI = (gctUINT16*)outAddr[0];
    gctFLOAT alpha = 1.0f;

    pI[0] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.r);
    pI[1] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.g);
    pI[2] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&inPixel->color.f.b);
    pI[3] = (gctUINT16)gcoMATH_FloatToFloat16(*(gctUINT32*)&alpha);
}

void _WritePixelTo_A32B32G32R32I(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT32* pI = (gctINT32*)outAddr[0];

    pI[0] = (gctINT32)(inPixel->color.i.r);
    pI[1] = (gctINT32)(inPixel->color.i.g);
    pI[2] = (gctINT32)(inPixel->color.i.b);
    pI[3] = (gctINT32)(inPixel->color.i.a);
}

void _WritePixelTo_X32B32G32R32I(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT32* pI = (gctINT32*)outAddr[0];

    pI[0] = (gctINT32)(inPixel->color.i.r);
    pI[1] = (gctINT32)(inPixel->color.i.g);
    pI[2] = (gctINT32)(inPixel->color.i.b);
    pI[3] = (gctINT32)(1);
}

void _WritePixelTo_A32B32G32R32I_2_G32R32X(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT32* pI0 = (gctINT32*)outAddr[0];
    gctINT32* pI1 = (gctINT32*)outAddr[1];

    pI0[0] = (gctINT32)(inPixel->color.i.r);
    pI0[1] = (gctINT32)(inPixel->color.i.g);
    pI1[0] = (gctINT32)(inPixel->color.i.b);
    pI1[1] = (gctINT32)(inPixel->color.i.a);
}

void _WritePixelTo_A32B32G32R32I_4_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT32* pI0 = (gctINT32*)outAddr[0];
    gctINT32* pI1 = (gctINT32*)outAddr[1];
    gctINT32* pI2 = (gctINT32*)outAddr[2];
    gctINT32* pI3 = (gctINT32*)outAddr[3];

    pI0[0] = (gctINT32)(inPixel->color.i.r);
    pI1[0] = (gctINT32)(inPixel->color.i.g);
    pI2[0] = (gctINT32)(inPixel->color.i.b);
    pI3[0] = (gctINT32)(inPixel->color.i.a);
}

void _WritePixelTo_X32B32G32R32I_2_G32R32I(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctINT32* pI0 = (gctINT32*)outAddr[0];
    gctINT32* pI1 = (gctINT32*)outAddr[1];

    pI0[0] = (gctINT32)(inPixel->color.i.r);
    pI0[1] = (gctINT32)(inPixel->color.i.g);
    pI1[0] = (gctINT32)(inPixel->color.i.b);
    pI1[1] = (gctINT32)(1);
}

void _WritePixelTo_A32B32G32R32UI(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT32* pI = (gctUINT32*)outAddr[0];

    pI[0] = (gctUINT32)(inPixel->color.ui.r);
    pI[1] = (gctUINT32)(inPixel->color.ui.g);
    pI[2] = (gctUINT32)(inPixel->color.ui.b);
    pI[3] = (gctUINT32)(inPixel->color.ui.a);
}

void _WritePixelTo_X32B32G32R32UI(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT32* pI = (gctUINT32*)outAddr[0];

    pI[0] = (gctUINT32)(inPixel->color.ui.r);
    pI[1] = (gctUINT32)(inPixel->color.ui.g);
    pI[2] = (gctUINT32)(inPixel->color.ui.b);
    pI[3] = (gctUINT32)(1);
}

void _WritePixelTo_A32B32G32R32UI_2_G32R32X(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT32* pI0 = (gctUINT32*)outAddr[0];
    gctUINT32* pI1 = (gctUINT32*)outAddr[1];

    pI0[0] = (gctUINT32)(inPixel->color.ui.r);
    pI0[1] = (gctUINT32)(inPixel->color.ui.g);
    pI1[0] = (gctUINT32)(inPixel->color.ui.b);
    pI1[1] = (gctUINT32)(inPixel->color.ui.a);
}

void _WritePixelTo_A32B32G32R32UI_4_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT32* pI0 = (gctUINT32*)outAddr[0];
    gctUINT32* pI1 = (gctUINT32*)outAddr[1];
    gctUINT32* pI2 = (gctUINT32*)outAddr[2];
    gctUINT32* pI3 = (gctUINT32*)outAddr[3];

    pI0[0] = (gctUINT32)(inPixel->color.ui.r);
    pI1[0] = (gctUINT32)(inPixel->color.ui.g);
    pI2[0] = (gctUINT32)(inPixel->color.ui.b);
    pI3[0] = (gctUINT32)(inPixel->color.ui.a);
}

void _WritePixelTo_X32B32G32R32UI_2_G32R32UI(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT32* pI0 = (gctUINT32*)outAddr[0];
    gctUINT32* pI1 = (gctUINT32*)outAddr[1];

    pI0[0] = (gctUINT32)(inPixel->color.ui.r);
    pI0[1] = (gctUINT32)(inPixel->color.ui.g);
    pI1[0] = (gctUINT32)(inPixel->color.ui.b);
    pI1[1] = (gctUINT32)(1);
}

void _WritePixelTo_A32B32G32R32F(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctFLOAT* pI = (gctFLOAT*)outAddr[0];

    pI[0] = (gctFLOAT)(inPixel->color.f.r);
    pI[1] = (gctFLOAT)(inPixel->color.f.g);
    pI[2] = (gctFLOAT)(inPixel->color.f.b);
    pI[3] = (gctFLOAT)(inPixel->color.f.a);
}

void _WritePixelTo_A32B32G32R32F_2_G32R32F(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctFLOAT* pI0 = (gctFLOAT*)outAddr[0];
    gctFLOAT* pI1 = (gctFLOAT*)outAddr[1];

    pI0[0] = (gctFLOAT)(inPixel->color.f.r);
    pI0[1] = (gctFLOAT)(inPixel->color.f.g);
    pI1[0] = (gctFLOAT)(inPixel->color.f.b);
    pI1[1] = (gctFLOAT)(inPixel->color.f.a);
}

void _WritePixelTo_A32B32G32R32F_4_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctFLOAT* pI0 = (gctFLOAT*)outAddr[0];
    gctFLOAT* pI1 = (gctFLOAT*)outAddr[1];
    gctFLOAT* pI2 = (gctFLOAT*)outAddr[2];
    gctFLOAT* pI3 = (gctFLOAT*)outAddr[3];

    pI0[0] = (gctFLOAT)(inPixel->color.f.r);
    pI1[0] = (gctFLOAT)(inPixel->color.f.g);
    pI2[0] = (gctFLOAT)(inPixel->color.f.b);
    pI3[0] = (gctFLOAT)(inPixel->color.f.a);
}

void _WritePixelTo_X32B32G32R32F_2_G32R32F(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctFLOAT* pI0 = (gctFLOAT*)outAddr[0];
    gctFLOAT* pI1 = (gctFLOAT*)outAddr[1];

    pI0[0] = (gctFLOAT)(inPixel->color.f.r);
    pI0[1] = (gctFLOAT)(inPixel->color.f.g);
    pI1[0] = (gctFLOAT)(inPixel->color.f.b);
    pI1[1] = (gctFLOAT)(1.0f);
}

void _WritePixelTo_X32B32G32R32F_4_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctFLOAT* pI0 = (gctFLOAT*)outAddr[0];
    gctFLOAT* pI1 = (gctFLOAT*)outAddr[1];
    gctFLOAT* pI2 = (gctFLOAT*)outAddr[2];
    gctFLOAT* pI3 = (gctFLOAT*)outAddr[3];

    pI0[0] = (gctFLOAT)(inPixel->color.f.r);
    pI1[0] = (gctFLOAT)(inPixel->color.f.g);
    pI2[0] = (gctFLOAT)(inPixel->color.f.b);
    pI3[0] = (gctFLOAT)(1.0f);
}

void _WritePixelTo_A2B10G10R10UI(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT32 r = gcmMIN(inPixel->color.ui.r, gcdUINT_MAX(10));
    gctUINT32 g = gcmMIN(inPixel->color.ui.g, gcdUINT_MAX(10));
    gctUINT32 b = gcmMIN(inPixel->color.ui.b, gcdUINT_MAX(10));
    gctUINT32 a = gcmMIN(inPixel->color.ui.a, gcdUINT_MAX(2));

    *(gctUINT32*)outAddr[0] = (gctUINT32)((a << 30) | (b << 20) | (g << 10) | r);
}

void _WritePixelTo_B10G11R11F(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctUINT32* pI = (gctUINT32*)outAddr[0];
    gctUINT16 r,g,b;

    r = (gctUINT16)gcoMATH_FloatToFloat11(*(gctUINT32*)&inPixel->color.f.r);
    g = (gctUINT16)gcoMATH_FloatToFloat11(*(gctUINT32*)&inPixel->color.f.g);
    b = (gctUINT16)gcoMATH_FloatToFloat10(*(gctUINT32*)&inPixel->color.f.b);

    pI[0] = (b << 22) | (g << 11) | r;
}

/* (gctINT32)gcoMATH_Log2() can achieve same func, but has precision issue.
** e.g. gcoMATH_Log2(32768) = 14.999999... and become 14 after floor.
** Actually it should be 15.
*/
gctINT32 _FloorLog2(gctFLOAT val)
{
    gctINT32 exp = 0;
    gctUINT64 base = 1;

    while (val >= (gctFLOAT)base)
    {
        base = (gctUINT64)1 << (++exp);
    }

    return exp - 1;
}

void _WritePixelTo_E5B9G9R9(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    const gctINT32 mBits = 9;
    const gctINT32 eBits = 5;
    const gctINT32 eBias = 15;
    const gctINT32 eMax  = (1 << eBits) - 1;
    const gctFLOAT sharedExpMax = ((1 << mBits) - 1) * (1 << (eMax - eBias)) / (gctFLOAT)(1 << mBits);

    gctFLOAT rc   = gcmCLAMP(inPixel->color.f.r, 0.0f, sharedExpMax);
    gctFLOAT gc   = gcmCLAMP(inPixel->color.f.g, 0.0f, sharedExpMax);
    gctFLOAT bc   = gcmCLAMP(inPixel->color.f.b, 0.0f, sharedExpMax);
    gctFLOAT maxc = gcoMATH_MAX(rc, gcoMATH_MAX(gc, bc));

    gctINT32 expp  = gcoMATH_MAX(-eBias - 1, _FloorLog2(maxc)) + 1 + eBias;
    gctFLOAT scale = (gctFLOAT)gcoMATH_Power(2.0f, (gctFLOAT)(expp - eBias - mBits));
    gctINT32 maxs  = (gctINT32)(maxc / scale + 0.5f);

    gctUINT32 exps = (maxs == (1 << mBits)) ? (gctUINT32)(expp + 1) : (gctUINT32)expp;
    gctUINT32 rs = gcmMIN((gctUINT32)(rc / scale + 0.5f), gcdUINT_MAX(9));
    gctUINT32 gs = gcmMIN((gctUINT32)(gc / scale + 0.5f), gcdUINT_MAX(9));
    gctUINT32 bs = gcmMIN((gctUINT32)(bc / scale + 0.5f), gcdUINT_MAX(9));

    *(gctUINT32*)outAddr[0] = rs | (gs << 9) | (bs << 18) | (exps << 27);
}

void _WritePixelTo_S8D32F_1_G32R32F(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctFLOAT *pF = (gctFLOAT *)outAddr[0];
    pF[0] = gcmCLAMP(inPixel->d, 0.0f, 1.0f);
    pF[1] = (gctFLOAT)inPixel->s;
}

void _WritePixelTo_S8D32F_2_A8R8G8B8(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags)
{
    gctFLOAT *pF = (gctFLOAT *)outAddr[0];
    gctUINT32 *pUI = (gctUINT32 *)outAddr[1];
    pF[0] = gcmCLAMP(inPixel->d, 0.0f, 1.0f);
    pUI[0] = inPixel->s;
}


_PFNwritePixel gcoSURF_GetWritePixelFunc(gcoSURF surf)
{
    switch (surf->format)
    {
    case gcvSURF_A8:
        return _WritePixelTo_A8;
    case gcvSURF_L8:
        return _WritePixelTo_L8;
    case gcvSURF_R8:
        return _WritePixelTo_R8;
    case gcvSURF_R8_1_X8R8G8B8:
        return _WritePixelTo_R8_1_X8R8G8B8;
    case gcvSURF_A8L8:
        return _WritePixelTo_A8L8;
    case gcvSURF_A8L8_1_A8R8G8B8:
        return _WritePixelTo_A8L8_1_A8R8G8B8;
    case gcvSURF_G8R8:
        return _WritePixelTo_G8R8;
    case gcvSURF_G8R8_1_X8R8G8B8:
        return _WritePixelTo_G8R8_1_X8R8G8B8;
    case gcvSURF_R5G6B5:
        return _WritePixelTo_R5G6B5;
    case gcvSURF_X4R4G4B4:
        return _WritePixelTo_X4R4G4B4;
    case gcvSURF_A4R4G4B4:
        return _WritePixelTo_A4R4G4B4;
    case gcvSURF_R4G4B4A4:
        return _WritePixelTo_R4G4B4A4;
    case gcvSURF_X1R5G5B5:
        return _WritePixelTo_X1R5G5B5;
    case gcvSURF_A1R5G5B5:
        return _WritePixelTo_A1R5G5B5;
    case gcvSURF_X8R8G8B8:
        return _WritePixelTo_X8R8G8B8;
    case gcvSURF_A8B8G8R8:
        return _WritePixelTo_A8B8G8R8;
    case gcvSURF_X8B8G8R8:
        return _WritePixelTo_X8B8G8R8;
    case gcvSURF_A8R8G8B8:
        return _WritePixelTo_A8R8G8B8;
    case gcvSURF_X2B10G10R10:
        return _WritePixelTo_X2B10G10R10;
    case gcvSURF_A2B10G10R10:
        return _WritePixelTo_A2B10G10R10;
    case gcvSURF_D16:
        return _WritePixelTo_D16;
    case gcvSURF_D24X8:
        return _WritePixelTo_D24X8;
    case gcvSURF_D24S8:
        return _WritePixelTo_D24S8;
    case gcvSURF_X24S8:
        return _WritePixelTo_X24S8;
    case gcvSURF_D32:
        return _WritePixelTo_D32;
    case gcvSURF_D32F:
        return _WritePixelTo_D32F;

    case gcvSURF_D24S8_1_A8R8G8B8:
        return _WritePixelTo_D24S8;

    case gcvSURF_X24S8_1_A8R8G8B8:
        return _WritePixelTo_X24S8;

    case gcvSURF_S8:
        return _WritePixelTo_S8;

    /* SNORM format. */
    case gcvSURF_R8_SNORM:
        return _WritePixelTo_R8_SNORM;
    case gcvSURF_G8R8_SNORM:
        return _WritePixelTo_G8R8_SNORM;
    case gcvSURF_B8G8R8_SNORM:
        return _WritePixelTo_B8G8R8_SNORM;
    case gcvSURF_A8B8G8R8_SNORM:
        return _WritePixelTo_A8B8G8R8_SNORM;
    case gcvSURF_X8B8G8R8_SNORM:
        return _WritePixelTo_X8B8G8R8_SNORM;


    /* Integer format. */
    case gcvSURF_R8I:
    case gcvSURF_R8I_1_A4R4G4B4:
        return _WritePixelTo_R8I;

    case gcvSURF_R8UI:
    case gcvSURF_R8UI_1_A4R4G4B4:
        return _WritePixelTo_R8UI;

    case gcvSURF_R16I:
    case gcvSURF_R16I_1_A4R4G4B4:
        return _WritePixelTo_R16I;

    case gcvSURF_R16UI:
    case gcvSURF_R16UI_1_A4R4G4B4:
        return _WritePixelTo_R16UI;

    case gcvSURF_R32I:
    case gcvSURF_R32I_1_A8R8G8B8:
        return _WritePixelTo_R32I;

    case gcvSURF_R32UI:
    case gcvSURF_R32UI_1_A8R8G8B8:
        return _WritePixelTo_R32UI;

    case gcvSURF_G8R8I:
    case gcvSURF_G8R8I_1_A4R4G4B4:
        return _WritePixelTo_G8R8I;

    case gcvSURF_G8R8UI:
    case gcvSURF_G8R8UI_1_A4R4G4B4:
        return _WritePixelTo_G8R8UI;

    case gcvSURF_G16R16I:
    case gcvSURF_G16R16I_1_A8R8G8B8:
        return _WritePixelTo_G16R16I;

    case gcvSURF_G16R16UI:
    case gcvSURF_G16R16UI_1_A8R8G8B8:
        return _WritePixelTo_G16R16UI;

    case gcvSURF_G32R32I:
    case gcvSURF_G32R32I_1_G32R32F:
        return _WritePixelTo_G32R32I;

    case gcvSURF_G32R32I_2_A8R8G8B8:
        return _WritePixelTo_G32R32I_2_A8R8G8B8;

    case gcvSURF_G32R32UI:
    case gcvSURF_G32R32UI_1_G32R32F:
        return _WritePixelTo_G32R32UI;

    case gcvSURF_G32R32UI_2_A8R8G8B8:
        return _WritePixelTo_G32R32UI_2_A8R8G8B8;

    case gcvSURF_B8G8R8I:
        return _WritePixelTo_B8G8R8I;
    case gcvSURF_B8G8R8I_1_A8R8G8B8:
        return _WritePixelTo_B8G8R8I_1_A8R8G8B8;

    case gcvSURF_B8G8R8UI:
        return _WritePixelTo_B8G8R8UI;
    case gcvSURF_B8G8R8UI_1_A8R8G8B8:
        return _WritePixelTo_B8G8R8UI_1_A8R8G8B8;

    case gcvSURF_B16G16R16I:
    case gcvSURF_B16G16R16I_1_G32R32F:
        return _WritePixelTo_B16G16R16I;

    case gcvSURF_B16G16R16I_2_A8R8G8B8:
        return _WritePixelTo_B16G16R16I_2_A8R8G8B8;

    case gcvSURF_B16G16R16UI:
    case gcvSURF_B16G16R16UI_1_G32R32F:
        return _WritePixelTo_B16G16R16UI;

    case gcvSURF_B16G16R16UI_2_A8R8G8B8:
        return _WritePixelTo_B16G16R16UI_2_A8R8G8B8;

    case gcvSURF_B32G32R32I:
        return _WritePixelTo_B32G32R32I;

    case gcvSURF_B32G32R32I_3_A8R8G8B8:
        return _WritePixelTo_B32G32R32I_3_A8R8G8B8;

    case gcvSURF_B32G32R32UI:
        return _WritePixelTo_B32G32R32UI;

    case gcvSURF_B32G32R32UI_3_A8R8G8B8:
        return _WritePixelTo_B32G32R32UI_3_A8R8G8B8;

    case gcvSURF_A8B8G8R8I:
        return _WritePixelTo_A8B8G8R8I;
    case gcvSURF_A8B8G8R8I_1_A8R8G8B8:
        return _WritePixelTo_A8B8G8R8I_1_A8R8G8B8;

    case gcvSURF_X8B8G8R8I:
        return _WritePixelTo_X8B8G8R8I;

    case gcvSURF_A8B8G8R8UI:
        return _WritePixelTo_A8B8G8R8UI;
    case gcvSURF_A8B8G8R8UI_1_A8R8G8B8:
        return _WritePixelTo_A8B8G8R8UI_1_A8R8G8B8;

    case gcvSURF_X8B8G8R8UI:
        return _WritePixelTo_X8B8G8R8UI;

    case gcvSURF_A16B16G16R16I:
    case gcvSURF_A16B16G16R16I_1_G32R32F:
        return _WritePixelTo_A16B16G16R16I;

    case gcvSURF_A16B16G16R16I_2_A8R8G8B8:
        return _WritePixelTo_A16B16G16R16I_2_A8R8G8B8;

    case gcvSURF_X16B16G16R16I:
    case gcvSURF_X16B16G16R16I_1_G32R32F:
        return _WritePixelTo_X16B16G16R16I;

    case gcvSURF_X16B16G16R16I_2_A8R8G8B8:
        return _WritePixelTo_X16B16G16R16I_2_A8R8G8B8;

    case gcvSURF_A16B16G16R16UI:
    case gcvSURF_A16B16G16R16UI_1_G32R32F:
        return _WritePixelTo_A16B16G16R16UI;

    case gcvSURF_A16B16G16R16UI_2_A8R8G8B8:
        return _WritePixelTo_A16B16G16R16UI_2_A8R8G8B8;

    case gcvSURF_X16B16G16R16UI:
    case gcvSURF_X16B16G16R16UI_1_G32R32F:
        return _WritePixelTo_X16B16G16R16UI;

    case gcvSURF_X16B16G16R16UI_2_A8R8G8B8:
        return _WritePixelTo_X16B16G16R16UI_2_A8R8G8B8;

    case gcvSURF_A32B32G32R32I:
        return _WritePixelTo_A32B32G32R32I;

    case gcvSURF_A32B32G32R32I_2_G32R32I:
    case gcvSURF_A32B32G32R32I_2_G32R32F:
        return _WritePixelTo_A32B32G32R32I_2_G32R32X;

    case gcvSURF_A32B32G32R32I_4_A8R8G8B8:
        return _WritePixelTo_A32B32G32R32I_4_A8R8G8B8;

    case gcvSURF_X32B32G32R32I:
        return _WritePixelTo_X32B32G32R32I;
    case gcvSURF_X32B32G32R32I_2_G32R32I:
        return _WritePixelTo_X32B32G32R32I_2_G32R32I;

    case gcvSURF_A32B32G32R32UI:
        return _WritePixelTo_A32B32G32R32UI;
    case gcvSURF_A32B32G32R32UI_2_G32R32UI:
    case gcvSURF_A32B32G32R32UI_2_G32R32F:
        return _WritePixelTo_A32B32G32R32UI_2_G32R32X;
    case gcvSURF_A32B32G32R32UI_4_A8R8G8B8:
        return _WritePixelTo_A32B32G32R32UI_4_A8R8G8B8;

    case gcvSURF_X32B32G32R32UI:
        return _WritePixelTo_X32B32G32R32UI;
    case gcvSURF_X32B32G32R32UI_2_G32R32UI:
        return _WritePixelTo_X32B32G32R32UI_2_G32R32UI;

    case gcvSURF_A2B10G10R10UI:
    case gcvSURF_A2B10G10R10UI_1_A8R8G8B8:
        return _WritePixelTo_A2B10G10R10UI;

    case gcvSURF_SBGR8:
        return _WritePixelTo_B8G8R8;
    case gcvSURF_A8_SBGR8:
        return _WritePixelTo_A8B8G8R8;

    case gcvSURF_X8_SBGR8:
        return _WritePixelTo_X8B8G8R8;

    case gcvSURF_A8_SRGB8:
        return _WritePixelTo_A8R8G8B8;

    case gcvSURF_X8_SRGB8:
        return _WritePixelTo_X8R8G8B8;


    /* Floating format */
    case gcvSURF_R16F:
    case gcvSURF_R16F_1_A4R4G4B4:
        return _WritePixelTo_R16F;

    case gcvSURF_G16R16F:
    case gcvSURF_G16R16F_1_A8R8G8B8:
        return _WritePixelTo_G16R16F;

    case gcvSURF_A16B16G16R16F:
        return _WritePixelTo_A16B16G16R16F;

    case gcvSURF_B16G16R16F:
        return _WritePixelTo_B16G16R16F;

    case gcvSURF_A16B16G16R16F_2_A8R8G8B8:
        return _WritePixelTo_A16B16G16R16F_2_A8R8G8B8;

    case gcvSURF_A16B16G16R16F_2_G16R16F:
        return _WritePixelTo_A16B16G16R16F_2_G16R16F;

    case gcvSURF_X16B16G16R16F_2_A8R8G8B8:
    case gcvSURF_B16G16R16F_2_A8R8G8B8:
        return _WritePixelTo_B16G16R16F_2_A8R8G8B8;

    case gcvSURF_X16B16G16R16F:
        return _WritePixelTo_X16B16G16R16F;

    case gcvSURF_R32F:
    case gcvSURF_R32F_1_A8R8G8B8:
        return _WritePixelTo_R32F;

    case gcvSURF_G32R32F:
        return _WritePixelTo_G32R32F;

    case gcvSURF_G32R32F_2_A8R8G8B8:
        return _WritePixelTo_G32R32F_2_A8R8G8B8;

    case gcvSURF_A32B32G32R32F:
        return _WritePixelTo_A32B32G32R32F;

    case gcvSURF_A32B32G32R32F_2_G32R32F:
        return _WritePixelTo_A32B32G32R32F_2_G32R32F;

    case gcvSURF_A32B32G32R32F_4_A8R8G8B8:
        return _WritePixelTo_A32B32G32R32F_4_A8R8G8B8;

    case gcvSURF_X32B32G32R32F_2_G32R32F:
        return _WritePixelTo_X32B32G32R32F_2_G32R32F;

    case gcvSURF_X32B32G32R32F_4_A8R8G8B8:
        return _WritePixelTo_X32B32G32R32F_4_A8R8G8B8;

    case gcvSURF_B10G11R11F:
    case gcvSURF_B10G11R11F_1_A8R8G8B8:
        return _WritePixelTo_B10G11R11F;

    case gcvSURF_E5B9G9R9:
        return _WritePixelTo_E5B9G9R9;

    case gcvSURF_S8D32F_1_G32R32F:
        return _WritePixelTo_S8D32F_1_G32R32F;
    case gcvSURF_S8D32F_2_A8R8G8B8:
        return _WritePixelTo_S8D32F_2_A8R8G8B8;

    case gcvSURF_A8B12G12R12_2_A8R8G8B8:
        return _WritePixelTo_A8B12G12R12_2_A8R8G8B8;
    case gcvSURF_A32L32F:
    case gcvSURF_A32L32F_1_G32R32F:
        return _WritePixelTo_A32L32F;
    case gcvSURF_A32F:
    case gcvSURF_A32F_1_R32F:
        return _WritePixelTo_A32F;

    case gcvSURF_L32F:
    case gcvSURF_L32F_1_R32F:
        return _WritePixelTo_L32F;


    default:
        gcmASSERT(0);
    }

    return gcvNULL;
}


/*
** Linear -> Non-linear conversion for input pixel
*/

gctFLOAT _LinearToNonLinearConv(gctFLOAT lFloat)
{
    gctFLOAT sFloat;
    if (lFloat <= 0.0f)
    {
        sFloat = 0.0f;
    }
    else if (lFloat < 0.0031308f)
    {
        sFloat = 12.92f * lFloat;
    }
    else if (lFloat < 1.0)
    {
        sFloat = 1.055f * gcoMATH_Power(lFloat, 0.41666f) -0.055f;
    }
    else
    {
        sFloat = 1.0f;
    }

    return sFloat;
}

void gcoSURF_PixelToNonLinear(gcsPIXEL* inPixel)
{
    gctFLOAT sR, sG, sB, sA;
    sR = _LinearToNonLinearConv(inPixel->color.f.r);
    sG = _LinearToNonLinearConv(inPixel->color.f.g);
    sB = _LinearToNonLinearConv(inPixel->color.f.b);
    sA = inPixel->color.f.a;

    inPixel->color.f.r = gcmCLAMP(sR, 0.0f, 1.0f);
    inPixel->color.f.g = gcmCLAMP(sG, 0.0f, 1.0f);
    inPixel->color.f.b = gcmCLAMP(sB, 0.0f, 1.0f);
    inPixel->color.f.a = gcmCLAMP(sA, 0.0f, 1.0f);
}

/*
** Non-linear to Linear conversion for input pixel
*/
void gcoSURF_PixelToLinear(gcsPIXEL* inPixel)
{
    gctFLOAT sR, sG, sB, sA;

    sR = gcmCLAMP(inPixel->color.f.r, 0.0f, 1.0f);
    sG = gcmCLAMP(inPixel->color.f.g, 0.0f, 1.0f);
    sB = gcmCLAMP(inPixel->color.f.b, 0.0f, 1.0f);
    sA = gcmCLAMP(inPixel->color.f.a, 0.0f, 1.0f);

    inPixel->color.f.r = (sR <= 0.04045f) ? (sR / 12.92f) : gcoMATH_Power((sR + 0.055f) / 1.055f, 2.4f);
    inPixel->color.f.g = (sG <= 0.04045f) ? (sG / 12.92f) : gcoMATH_Power((sG + 0.055f) / 1.055f, 2.4f);
    inPixel->color.f.b = (sB <= 0.04045f) ? (sB / 12.92f) : gcoMATH_Power((sB + 0.055f) / 1.055f, 2.4f);
    inPixel->color.f.a = sA;
}

#endif /* gcdENABLE_3D */
