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


#define gcmMaxInt        2147483647.0    /* 2^31 - 1 double */
#define gcmMinInt       -2147483647.0    /* -(2^32 - 1) double */
#define gcmMaxUint       4294967295.0    /* 2^32 - 1 double */
#define gcmHalf          (0.5f)

#define gcmFLoat2I(x) ((gctINT) (((x) >= 0.0f) ? ((x) * gcmMaxInt + gcmHalf) : ((x) * gcmMaxInt - gcmHalf)))
#define gcmUNORM_MAX(n)                 ((gctFLOAT)(((gctUINT64)1 << (n)) - 1))
#define gcmFLOAT_TO_UNORM(x, n)         (gctUINT32)(gcmCLAMP((x), 0.0f, 1.0f) * gcmUNORM_MAX(n))

static gctINT _GetTextureSwizzle(
    IN gcoHARDWARE Hardware,
    IN gcsTEXTURE_PTR TextureInfo,
    IN const gceTEXTURE_SWIZZLE *baseComponents
    )
{
    gctINT component;
    gceTEXTURE_SWIZZLE swizzledComponents[gcvTEXTURE_COMPONENT_NUM];

    /* If HW doesn't support the feature, no need to program the state */
    if (!Hardware->features[gcvFEATURE_TEXTURE_SWIZZLE])
    {
        return 0;
    }

    for (component = gcvTEXTURE_COMPONENT_R; component < gcvTEXTURE_COMPONENT_NUM; ++component)
    {
        switch (TextureInfo->swizzle[component])
        {
        case gcvTEXTURE_SWIZZLE_R:
            swizzledComponents[component] = baseComponents[gcvTEXTURE_COMPONENT_R];
            break;
        case gcvTEXTURE_SWIZZLE_G:
            swizzledComponents[component] = baseComponents[gcvTEXTURE_COMPONENT_G];
            break;
        case gcvTEXTURE_SWIZZLE_B:
            swizzledComponents[component] = baseComponents[gcvTEXTURE_COMPONENT_B];
            break;
        case gcvTEXTURE_SWIZZLE_A:
            swizzledComponents[component] = baseComponents[gcvTEXTURE_COMPONENT_A];
            break;
        case gcvTEXTURE_SWIZZLE_0:
            swizzledComponents[component] = gcvTEXTURE_SWIZZLE_0;
            break;
        case gcvTEXTURE_SWIZZLE_1:
            swizzledComponents[component] = gcvTEXTURE_SWIZZLE_1;
            break;
        default:
            swizzledComponents[component] = gcvTEXTURE_SWIZZLE_INVALID;
        }
        gcmASSERT(swizzledComponents[component] < gcvTEXTURE_SWIZZLE_INVALID);
    }

    return ((swizzledComponents[gcvTEXTURE_COMPONENT_R] << 20) |
            (swizzledComponents[gcvTEXTURE_COMPONENT_G] << 23) |
            (swizzledComponents[gcvTEXTURE_COMPONENT_B] << 26) |
            (swizzledComponents[gcvTEXTURE_COMPONENT_A] << 29));
}

static gctINT _GetTextureFormat(
    IN  gcoHARDWARE Hardware,
    IN  gcsSURF_FORMAT_INFO_PTR FormatInfo,
    IN  gcsTEXTURE_PTR TextureInfo,
    IN  gctBOOL needYUVAssembler,
    IN  gctBOOL unsizedDepthTexture,
    IN  gceCACHE_MODE cacheMode,
    OUT gctBOOL *IntegerFormat,
    OUT gctUINT *SampleMode2
    )
{
    if (needYUVAssembler &&
        (Hardware->features[gcvFEATURE_TEXTURE_YUV_ASSEMBLER]))
    {
        /* Use YUV Assembler module. */
        static const gceTEXTURE_SWIZZLE yuvSwizzle[] =
        {
            gcvTEXTURE_SWIZZLE_R,
            gcvTEXTURE_SWIZZLE_G,
            gcvTEXTURE_SWIZZLE_B,
            gcvTEXTURE_SWIZZLE_1
        };

        *IntegerFormat = gcvTRUE;

        return (0x13 << 12) |
               _GetTextureSwizzle(Hardware, TextureInfo, yuvSwizzle);
    }

    if (FormatInfo->txFormat == gcvINVALID_TEXTURE_FORMAT)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, _GC_OBJ_ZONE,
                      "Unknown color format.");

        /* Format is not supported. */
        return -1;
    }

    *IntegerFormat = FormatInfo->txIntFilter;

    if (FormatInfo->fmtClass == gcvFORMAT_CLASS_DEPTH)
    {
        static const gceTEXTURE_SWIZZLE baseComponents_r001[] =
        {
            gcvTEXTURE_SWIZZLE_R,
            gcvTEXTURE_SWIZZLE_0,
            gcvTEXTURE_SWIZZLE_0,
            gcvTEXTURE_SWIZZLE_1
        };

        static const gceTEXTURE_SWIZZLE baseComponents_r00a[] =
        {
            gcvTEXTURE_SWIZZLE_R,
            gcvTEXTURE_SWIZZLE_0,
            gcvTEXTURE_SWIZZLE_0,
            gcvTEXTURE_SWIZZLE_A
        };

        static const gceTEXTURE_SWIZZLE baseComponents_b001[] =
        {
            gcvTEXTURE_SWIZZLE_B,
            gcvTEXTURE_SWIZZLE_0,
            gcvTEXTURE_SWIZZLE_0,
            gcvTEXTURE_SWIZZLE_1
        };

        static const gceTEXTURE_SWIZZLE baseComponents_rgba[] =
        {
            gcvTEXTURE_SWIZZLE_R,
            gcvTEXTURE_SWIZZLE_G,
            gcvTEXTURE_SWIZZLE_B,
            gcvTEXTURE_SWIZZLE_A
        };

        static const gceTEXTURE_SWIZZLE baseComponents_rg00[] =
        {
            gcvTEXTURE_SWIZZLE_R,
            gcvTEXTURE_SWIZZLE_G,
            gcvTEXTURE_SWIZZLE_0,
            gcvTEXTURE_SWIZZLE_0
        };

        gctUINT txFormat = FormatInfo->txFormat;

        /* GL_OES_depth_texture specify depth texture with unsized internal format, the value should (d,d,d,1.0).
        ** OES3.0 core using sized internal format which produce (d,0,0,1.0).
        ** For gcvSURF_S8D32F_1_G32R32F, it's defined OES3.0, and we need clear G channel.
        */
        const gceTEXTURE_SWIZZLE *baseComponents_depth;

        if (FormatInfo->format == gcvSURF_S8D32F_1_G32R32F)
        {
            if (Hardware->features[gcvFEATURE_32BPP_COMPONENT_TEXTURE_CHANNEL_SWIZZLE])
            {
                if (TextureInfo->dsMode == gcvTEXTURE_DS_MODE_STENCIL)
                {
                    baseComponents_depth = baseComponents_rg00;
                }
                else
                {
                    baseComponents_depth = baseComponents_r001;
                }
            }
            else
            {
                if (TextureInfo->dsMode == gcvTEXTURE_DS_MODE_STENCIL)
                {
                    baseComponents_depth = baseComponents_rgba;
                }
                else
                {
                    baseComponents_depth = baseComponents_rg00;
                }
            }
        }
        else if ((FormatInfo->format == gcvSURF_D24S8) &&
                 (TextureInfo->dsMode == gcvTEXTURE_DS_MODE_STENCIL) &&
                 (!Hardware->features[gcvFEATURE_D24S8_SAMPLE_STENCIL]))
        {
            if (Hardware->features[gcvFEATURE_HALTI2])
            {
                txFormat = 0x17 << 12;
                baseComponents_depth = baseComponents_r001;
            }
            else
            {
                txFormat = 0x07;
                baseComponents_depth = baseComponents_b001;
            }
        }
        else if (unsizedDepthTexture)
        {
            baseComponents_depth = FormatInfo->txSwizzle;
        }
        else if ((FormatInfo->format != gcvSURF_S8D32F_2_A8R8G8B8) &&
                 (FormatInfo->format != gcvSURF_D24S8_1_A8R8G8B8))
        {
            baseComponents_depth = baseComponents_r00a;
        }
        else
        {
            baseComponents_depth = baseComponents_rgba;
        }

        return (txFormat | _GetTextureSwizzle(Hardware, TextureInfo, baseComponents_depth));
    }

    if (Hardware->features[gcvFEATURE_INTEGER_SIGNEXT_FIX])
    {
        if (FormatInfo->fmtDataType == gcvFORMAT_DATATYPE_SIGNED_INTEGER)
        {
            if ((FormatInfo->txFormat >> 12) == 0x15 ||
                (FormatInfo->txFormat >> 12) == 0x16 ||
                (FormatInfo->txFormat >> 12) == 0x17 ||
                (FormatInfo->txFormat >> 12) == 0x1F ||
                (FormatInfo->txFormat >> 12) == 0x20
                )
            {
                *SampleMode2 |=
                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 19:18) - (0 ?
 19:18) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:18) - (0 ?
 19:18) + 1))))))) << (0 ?
 19:18))) | (((gctUINT32) ((gctUINT32) (0x1) & ((gctUINT32) ((((1 ?
 19:18) - (0 ?
 19:18) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 19:18) - (0 ? 19:18) + 1))))))) << (0 ? 19:18)));
            }
            if ((FormatInfo->txFormat >> 12) == 0x18 ||
                (FormatInfo->txFormat >> 12) == 0x19 ||
                (FormatInfo->txFormat >> 12) == 0x1A
                )
            {
                *SampleMode2 |=
                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 19:18) - (0 ?
 19:18) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:18) - (0 ?
 19:18) + 1))))))) << (0 ?
 19:18))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ?
 19:18) - (0 ?
 19:18) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 19:18) - (0 ? 19:18) + 1))))))) << (0 ? 19:18)));
            }
        }
    }

    /*
    ** There are 2 steps of texture swizzle:
    ** 1. implicit: filter texture value to texture base color Cb. Since HW cannot support it natively, it need
    **              driver help to program the swizzle state.
    ** 2. explicit: which is specified by app, and will swizzle the component of Cb.
    ** The register programmed here should combined the 2 steps.
    */

    return FormatInfo->txFormat |
           _GetTextureSwizzle(Hardware, TextureInfo, FormatInfo->txSwizzle);
}

/* YUV-assembler for linear YUV texture. */
static gctINT _GetYUVControl(
    gceSURF_FORMAT Format,
    gctUINT_PTR YOffset,
    gctUINT_PTR UOffset,
    gctUINT_PTR VOffset
    )
{
    switch (Format)
    {
    case gcvSURF_YV12:
        /* Y... */
        *YOffset = 0;
        /* V... */
        *VOffset = 0;
        /* U... */
        *UOffset = 0;

        return ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:8) - (0 ?
 10:8) + 1))))))) << (0 ?
 10:8))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:12) - (0 ?
 14:12) + 1))))))) << (0 ?
 14:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 18:16) - (0 ?
 18:16) + 1))))))) << (0 ?
 18:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16)));

    case gcvSURF_I420:
        /* Y... */
        *YOffset = 0;
        /* U... */
        *UOffset = 0;
        /* V... */
        *VOffset = 0;

        return ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:8) - (0 ?
 10:8) + 1))))))) << (0 ?
 10:8))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:12) - (0 ?
 14:12) + 1))))))) << (0 ?
 14:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 18:16) - (0 ?
 18:16) + 1))))))) << (0 ?
 18:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16)));

    case gcvSURF_NV12:
        /* Y... */
        *YOffset = 0;
        /* UV... */
        *UOffset = 0;
        *VOffset = 1;

        return ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:8) - (0 ?
 10:8) + 1))))))) << (0 ?
 10:8))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:12) - (0 ?
 14:12) + 1))))))) << (0 ?
 14:12))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 18:16) - (0 ?
 18:16) + 1))))))) << (0 ?
 18:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16)));

    case gcvSURF_NV21:
        /* Y... */
        *YOffset = 0;
        /* VU... */
        *VOffset = 0;
        *UOffset = 1;

        return ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:8) - (0 ?
 10:8) + 1))))))) << (0 ?
 10:8))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:12) - (0 ?
 14:12) + 1))))))) << (0 ?
 14:12))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 18:16) - (0 ?
 18:16) + 1))))))) << (0 ?
 18:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16)));

    case gcvSURF_NV16:
        /* Y... */
        *YOffset = 0;
        /* UV... */
        *UOffset = 0;
        *VOffset = 1;

        return ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:8) - (0 ?
 10:8) + 1))))))) << (0 ?
 10:8))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:12) - (0 ?
 14:12) + 1))))))) << (0 ?
 14:12))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 18:16) - (0 ?
 18:16) + 1))))))) << (0 ?
 18:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16)));

    case gcvSURF_NV61:
        /* Y... */
        *YOffset = 0;
        /* VU... */
        *VOffset = 0;
        *UOffset = 1;

        return ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:8) - (0 ?
 10:8) + 1))))))) << (0 ?
 10:8))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:12) - (0 ?
 14:12) + 1))))))) << (0 ?
 14:12))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 18:16) - (0 ?
 18:16) + 1))))))) << (0 ?
 18:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16)));

    case gcvSURF_AYUV:
        /* VYUY... */
        *VOffset = 3;
        *YOffset = 1;
        *UOffset = 2;

        return ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:8) - (0 ?
 10:8) + 1))))))) << (0 ?
 10:8))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:12) - (0 ?
 14:12) + 1))))))) << (0 ?
 14:12))) | (((gctUINT32) ((gctUINT32) (4) & ((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 18:16) - (0 ?
 18:16) + 1))))))) << (0 ?
 18:16))) | (((gctUINT32) ((gctUINT32) (4) & ((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16)));

    case gcvSURF_YUV420_10_ST:
        /* Y... */
        *YOffset = 0;
        /* UV... */
        *UOffset = 0;
        *VOffset = 1;

        return ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:6) - (0 ?
 6:6) + 1))))))) << (0 ?
 6:6))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:8) - (0 ?
 10:8) + 1))))))) << (0 ?
 10:8))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:12) - (0 ?
 14:12) + 1))))))) << (0 ?
 14:12))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 18:16) - (0 ?
 18:16) + 1))))))) << (0 ?
 18:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16)));

    case gcvSURF_YUV420_TILE_ST:
        /* Y... */
        *YOffset = 0;
        /* UV... */
        *UOffset = 0;
        *VOffset = 0;

        return ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:11) - (0 ?
 11:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:11) - (0 ?
 11:11) + 1))))))) << (0 ?
 11:11))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 11:11) - (0 ?
 11:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:8) - (0 ?
 10:8) + 1))))))) << (0 ?
 10:8))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:12) - (0 ?
 14:12) + 1))))))) << (0 ?
 14:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 18:16) - (0 ?
 18:16) + 1))))))) << (0 ?
 18:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16)));

    case gcvSURF_YUV420_TILE_10_ST:
        /* Y... */
        *YOffset = 0;
        /* UV... */
        *UOffset = 0;
        *VOffset = 0;

        return ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:6) - (0 ?
 6:6) + 1))))))) << (0 ?
 6:6))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:11) - (0 ?
 11:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:11) - (0 ?
 11:11) + 1))))))) << (0 ?
 11:11))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 11:11) - (0 ?
 11:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:8) - (0 ?
 10:8) + 1))))))) << (0 ?
 10:8))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:12) - (0 ?
 14:12) + 1))))))) << (0 ?
 14:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 18:16) - (0 ?
 18:16) + 1))))))) << (0 ?
 18:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16)));

    default:
        gcmTRACE_ZONE(gcvLEVEL_WARNING, _GC_OBJ_ZONE,
                      "Unknown yuv format.");
        break;
    }

    /* Format is not supported. */
    return -1;
}


/******************************************************************************\
|*************************** Texture Management Code **************************|
\******************************************************************************/

/*******************************************************************************
**
**  gcoHARDWARE_QueryTexture
**
**  Query if the requested texture is supported.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gceSURF_FORMAT Format
**          Format of texture.
**
**      gceTILING Tiling
**          Tiling of texture, specify linear or tile texture.
**
**      gctUINT Level
**          Level of texture map.
**
**      gctUINT Width
**          Width of texture.
**
**      gctUINT Height
**          Height of texture.
**
**      gctUINT Depth
**          Depth of texture.
**
**      gctUINT Faces
**          Number of faces for texture.
**
**  OUTPUT:
**
**      gctUINT * BlockWidth
**          Pointer to a variable receiving the width of a block of texels.
**
**      gctUINT * BlockHeight
**          Pointer to a variable receiving the height of a block of texels.
**
*/
gceSTATUS
gcoHARDWARE_QueryTexture(
    IN gcoHARDWARE Hardware,
    IN gceSURF_FORMAT Format,
    IN gceTILING Tiling,
    IN gctUINT Level,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth,
    IN gctUINT Faces,
    OUT gctUINT * BlockWidth,
    OUT gctUINT * BlockHeight
    )
{
    gceSTATUS status;
    gcsSURF_FORMAT_INFO_PTR formatInfo;

    gcmHEADER_ARG("Hardware=0x%x Format=%d Tiling=%d Level=%u Width=%u Height=%u "
                  "Depth=%u Faces=%u",
                  Hardware, Format, Tiling, Level, Width, Height, Depth, Faces);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(BlockWidth != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(BlockHeight != gcvNULL);

    gcmGETHARDWARE(Hardware);

    /* Non-power of two support. */
#if !gcdUSE_NPOT_PATCH
    if (!Hardware->features[gcvFEATURE_NON_POWER_OF_TWO])
    {
        /* We don't support non-power of two textures beyond level 0. */
        if ((Level > 0)
        &&  ((Width  & (Width  - 1))
            || (Height & (Height - 1))
            )
        )
        {
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }
#endif

    /* Check maximum width and height. */
    if (Hardware->features[gcvFEATURE_TEXTURE_8K])
    {
        if ((Width > 8192) || (Height > 8192))
        {
            gcmONERROR(gcvSTATUS_MIPMAP_TOO_LARGE);
        }
    }
    else
    {
        if ((Width > 2048) || (Height > 2048))
        {
            gcmONERROR(gcvSTATUS_MIPMAP_TOO_LARGE);
        }
    }

    /* Check linear texture. */
    if (Tiling == gcvLINEAR)
    {
        /* Is HW support linear texture? */
        if (!Hardware->features[gcvFEATURE_TEXTURE_LINEAR] && (Format != gcvSURF_L8_RAW) && (Format != gcvSURF_A8L8_RAW) )
        {
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }

        /* Linear YUV texture need YUV-assembler support in HW. */
        if (((Format == gcvSURF_YV12) ||
              (Format == gcvSURF_I420) ||
              (Format == gcvSURF_NV12) ||
              (Format == gcvSURF_NV21) ||
              (Format == gcvSURF_NV16) ||
              (Format == gcvSURF_NV61)) &&
            (!Hardware->features[gcvFEATURE_TEXTURE_YUV_ASSEMBLER]))
        {
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }

    /* Find the format record. */
    gcmONERROR(gcoHARDWARE_QueryFormat(Format, &formatInfo));

    /* Set the output. */
    *BlockWidth  = formatInfo->blockWidth;
    *BlockHeight = formatInfo->blockHeight;

    /* Success. */
    gcmFOOTER_ARG("*BlockWidth=%u *BlockHeight=%u",
                  *BlockWidth, *BlockHeight);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_DisableTextureSampler
**
**  Disable a specific sampler.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gctINT Sampler
**          Sampler number.
**
**      gctBOOL DefaultInteger
**          Default value is integer or not.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_DisableTextureSampler(
    IN gcoHARDWARE Hardware,
    IN gctINT Sampler,
    IN gctBOOL DefaultInteger
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT totalCount = 0;

    gcmHEADER_ARG("Hardware=0x%x Sampler=%d DefaultInteger=%d", Hardware, Sampler, DefaultInteger);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    gcmONERROR(gcoHARDWARE_QuerySamplerBase(gcvNULL,
                                            gcvNULL,
                                            gcvNULL,
                                            &totalCount
                                            ));

    /* Sometimes, sampler index may exceed HW supported range, just skip. */
    if (Sampler >= 0 && Sampler < (gctINT)(totalCount))
    {
        if (Hardware->features[gcvFEATURE_TX_DESCRIPTOR])
        {
            gctUINT32 physical;
            gctUINT index = DefaultInteger ? 1 : 0;

            if (gcvNULL == Hardware->TXStates->nullTxDescNode[index])
            {
                gcsTEXTUREDESCRIPTORREGS *nullTxDescPointer;
                gcoOS_Allocate(gcvNULL, gcmSIZEOF(gcsSURF_NODE), (gctPOINTER *)&Hardware->TXStates->nullTxDescNode[index]);
                gcoOS_ZeroMemory((gctPOINTER)Hardware->TXStates->nullTxDescNode[index], gcmSIZEOF(gcsSURF_NODE));
                gcmONERROR(gcsSURF_NODE_Construct(Hardware->TXStates->nullTxDescNode[index],
                                                  256,
                                                  64,
                                                  gcvSURF_TXDESC,
                                                  0,
                                                  gcvPOOL_DEFAULT
                                                  ));
                gcmONERROR(gcoSURF_LockNode(Hardware->TXStates->nullTxDescNode[index], gcvNULL, (gctPOINTER*)&nullTxDescPointer));

                gcoOS_ZeroMemory(nullTxDescPointer, gcmSIZEOF(gcsTEXTUREDESCRIPTORREGS));

                if (index)
                {
                    nullTxDescPointer->gcregTXConfig = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:0) - (0 ?
 2:0) + 1))))))) << (0 ?
 2:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                                                     | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 17:13) - (0 ?
 17:13) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 17:13) - (0 ?
 17:13) + 1))))))) << (0 ?
 17:13))) | (((gctUINT32) ((gctUINT32) (0x01) & ((gctUINT32) ((((1 ?
 17:13) - (0 ?
 17:13) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 17:13) - (0 ? 17:13) + 1))))))) << (0 ? 17:13)));
                }
                else
                {
                    nullTxDescPointer->gcregTXConfig = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:0) - (0 ?
 2:0) + 1))))))) << (0 ?
 2:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                                                     | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 17:13) - (0 ?
 17:13) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 17:13) - (0 ?
 17:13) + 1))))))) << (0 ?
 17:13))) | (((gctUINT32) ((gctUINT32) (0x07) & ((gctUINT32) ((((1 ?
 17:13) - (0 ?
 17:13) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 17:13) - (0 ? 17:13) + 1))))))) << (0 ? 17:13)));
                }


                gcmONERROR(gcoSURF_UnLockNode(Hardware->TXStates->nullTxDescNode[index], gcvSURF_TXDESC));
            }

            if (!Hardware->TXStates->nullTxDescLocked[index])
            {
                gcmONERROR(gcoSURF_LockNode(Hardware->TXStates->nullTxDescNode[index], gcvNULL, &Hardware->TXStates->nullTxDescLocked[index]));
            }
            gcmGETHARDWAREADDRESSP(Hardware->TXStates->nullTxDescNode[index], physical);

            gcmDUMP_BUFFER(gcvNULL, gcvDUMP_BUFFER_MEMORY, physical, Hardware->TXStates->nullTxDescLocked[index], 0, 256);

            if (Hardware->TXStates->hwTxDescAddress[Sampler] != physical)
            {
                Hardware->TXStates->hwTxDescAddress[Sampler] = physical;
                gcsBITMASK_Set(&Hardware->TXDirty->hwTxDescAddressDirty, Sampler);
                gcsBITMASK_Set(&Hardware->TXDirty->hwTxDescDirty, Sampler);
                Hardware->TXDirty->textureDirty = gcvTRUE;
            }
        }
        else
        {
            /* Setting the Texture Sampler mode to 0, shall disable the sampler. */
            gctUINT32 hwMode = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:0) - (0 ?
 2:0) + 1))))))) << (0 ?
 2:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                             | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 17:13) - (0 ?
 17:13) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 17:13) - (0 ?
 17:13) + 1))))))) << (0 ?
 17:13))) | (((gctUINT32) ((gctUINT32) (!DefaultInteger ?
 0x07 : 0x01) & ((gctUINT32) ((((1 ?
 17:13) - (0 ?
 17:13) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 17:13) - (0 ? 17:13) + 1))))))) << (0 ? 17:13)));

            if (hwMode != Hardware->TXStates->hwTxSamplerMode[Sampler])
            {
                /* Save dirty sampler mode register. */
                Hardware->TXStates->hwTxSamplerMode[Sampler] = hwMode;
                Hardware->TXDirty->hwTxSamplerModeDirty    |= 1 << Sampler;

                Hardware->TXDirty->hwTxSamplerDirty |= Hardware->TXDirty->hwTxSamplerModeDirty ;
                Hardware->TXDirty->textureDirty = gcvTRUE;
            }
        }

        if (Hardware->MCStates->texHasTileStatus[Sampler] != gcvFALSE)
        {
            gctINT tsSlotIndex = Hardware->MCStates->texTileStatusSlotIndex[Sampler];
            Hardware->MCStates->texHasTileStatus[Sampler] = gcvFALSE;
            if (tsSlotIndex != -1)
            {
                gcmASSERT(tsSlotIndex < gcdSAMPLER_TS);
                Hardware->MCStates->texTileStatusSlotUser[tsSlotIndex] = -1;
                Hardware->MCStates->texTileStatusSlotIndex[Sampler] = -1;
                Hardware->MCDirty->texTileStatusSlotDirty |= 1 << tsSlotIndex;
            }
        }
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gcoHARDWARE_FlushTexture
**
**  Flish the texture cache.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gctBOOL vsUsed
**          Used by vertex shader or pixel shader.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_FlushTexture(
    IN gcoHARDWARE Hardware,
    IN gctBOOL vsUsed
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Set texture flush flag. */
    if (vsUsed)
    {
        Hardware->TXDirty->hwTxFlushVS = gcvTRUE;
    }
    else
    {
        Hardware->TXDirty->hwTxFlushPS = gcvTRUE;
    }

    Hardware->TXDirty->textureDirty = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


/***************************************************************
* convert a input floating point number to an fixed point
* n.m format. The value will be clamped if out-of-range.
****************************************************************/
gctFIXED_POINT _Float2SignedFixed(gctFLOAT x, gctINT8 truncBits, gctINT8 fracBits)
{
    gctFIXED_POINT outInteger;

    gctUINT32 in = *((gctUINT32*)&x);

    const gctUINT16 fixedMask = (1 << (truncBits + fracBits)) - 1;
    const gctUINT16 fixedMax  = (1 << (truncBits + fracBits - 1)) - 1;
    const gctUINT16 fixedMin  = 1 << (truncBits + fracBits - 1);

    gctBOOL   signIn = (in & 0x80000000) ? gcvTRUE : gcvFALSE;
    /* minus 7F to get signed exponent */
    gctINT16  expIn  = ((in >> 23) & 0xFF) - 0x7F;
    /* There is implicit "1" before the 23 mantissa bits */
    gctUINT32 manIn  = (in & 0x7FFFFF) | 0x800000;

    if (expIn < -fracBits)
    {
        outInteger = 0;
    }
    else if (expIn >= truncBits - 1)
    {
        outInteger = signIn ? fixedMin : fixedMax;
    }
    else
    {
        gctFIXED_POINT manShift = 23 - (expIn + fracBits);
        outInteger = manIn >> manShift;
        if (signIn)
        {
            outInteger = (~outInteger + 1) & fixedMask;
        }
    }

    return outInteger;
}


gctUINT32 _Log2inXdot8(IN gctINT X)
{
    gctUINT32 res = 0;
    gctUINT32 scale = (1 << 8);

    if (X <= 1 )
    {
        return 0;
    }

    if (!(X & 0xFF))
    {
        X >>= 8;
        res += 8 * scale;
    }
    if (!(X & 0xF))
    {
        X >>= 4;
        res += 4 * scale;
    }
    if (!(X & 0x3))
    {
        X >>= 2;
        res += 2 * scale;
    }
    if (!(X & 0x1))
    {
        X >>= 1;
        res += scale;
    }

    if (X > 1)
    {
        res += (gctUINT32)(gcoMATH_Log2((gctFLOAT)X) * scale);
    }

    return res;
}

static gctINT _GetSamplerType(gceTEXTURE_TYPE textureType, gctBOOL *isArray)
{
    gctINT type;
    gctBOOL isTexArray = gcvFALSE;

    switch (textureType)
    {
    case gcvTEXTURE_2D:
    case gcvTEXTURE_2D_MS:
        type = 0x2;
        break;

    case gcvTEXTURE_3D:
        type = 0x3;
        break;

    case gcvTEXTURE_2D_ARRAY:
    case gcvTEXTURE_2D_MS_ARRAY:
        type = 0x3;
        isTexArray = gcvTRUE;
        break;

    case gcvTEXTURE_CUBEMAP:
        type = 0x5;
        break;

    case gcvTEXTURE_CUBEMAP_ARRAY:
        type = 0x5;
        isTexArray = gcvTRUE;
        break;

    case gcvTEXTURE_1D:
        type = 0x1;
        break;

    case gcvTEXTURE_1D_ARRAY:
        type = 0x2;
        isTexArray = gcvTRUE;
        break;

    case gcvTEXTURE_EXTERNAL:
        type = 0x2;
        break;

    case gcvTEXTURE_UNKNOWN:
    default:
        type = 0x2;
        break;

    }

    if (isArray)
    {
        *isArray = isTexArray;
    }

    return type;
}

static gctUINT32 _GetEndianSetting(gcoHARDWARE Hardware, gctBOOL Hint)
{
    gctUINT32 endian;
    switch (Hint)
    {
    case gcvENDIAN_SWAP_WORD:
        endian = Hardware->bigEndian
               ? 0x1
               : 0x0;
        break;

    case gcvENDIAN_SWAP_DWORD:
        endian = Hardware->bigEndian
               ? 0x2
               : 0x0;
        break;

    default:
        endian = 0x0;
        break;
    }
    return endian;
}

static void _GetMinAndmaxValue(
    gceFORMAT_DATATYPE DataType,
    gctUINT BitNum,
    gctPOINTER MinValue,
    gctPOINTER MaxValue
    )
{
    union value{
        gctINT    intValue;
        gctUINT   uintValue;
    } *minValue, *maxValue;

    minValue = (union value *)MinValue;
    maxValue = (union value *)MaxValue;

    if (DataType == gcvFORMAT_DATATYPE_SIGNED_INTEGER)
    {
        minValue->intValue = (-((gctINT)1 << (BitNum - 1)));
        maxValue->intValue = (((gctINT)1 << (BitNum - 1)) - 1);
    }
    else
    {
        minValue->uintValue = 0;
        maxValue->uintValue = (gctUINT)((((gctINT64)1) << BitNum)-1);
    }
}

static void _EncodeRGB9E5(
    gctFLOAT_PTR borderColor32
    )
{
    const gctINT    mBits   = 9;
    const gctINT    eBits   = 5;
    const gctINT    eBias   = 15;
    const gctINT    eMax    = (1<<eBits)-1;
    const gctFLOAT  maxVal  = (gctFLOAT)(((1<<mBits) - 1) * (1<<(eMax-eBias))) / (gctFLOAT)(1<<mBits);
    gctFLOAT *      color   = borderColor32;

    gctFLOAT    rc      = gcmCLAMP(color[0], 0.0f, maxVal);
    gctFLOAT    gc      = gcmCLAMP(color[1], 0.0f, maxVal);
    gctFLOAT    bc      = gcmCLAMP(color[2], 0.0f, maxVal);
    gctFLOAT    maxc    = gcmMAX(rc, gcmMAX(gc, bc));
    gctINT      expp    = gcmMAX(-eBias - 1, (gctINT)gcoMATH_Floor(gcoMATH_Log2(maxc))) + 1 + eBias;
    gctFLOAT    e       = gcoMATH_Exp2((gctFLOAT)(expp-eBias-mBits));
    gctINT      maxs    = (gctINT)gcoMATH_Floor(maxc / e + 0.5f);

    gctUINT32   exps    = maxs == (1<<mBits) ? expp+1 : expp;
    color[0]    = (gctFLOAT)gcmCLAMP((gctINT)gcoMATH_Floor(rc / e + 0.5f), 0, (1<<9)-1);
    color[1]    = (gctFLOAT)gcmCLAMP((gctINT)gcoMATH_Floor(gc / e + 0.5f), 0, (1<<9)-1);
    color[2]    = (gctFLOAT)gcmCLAMP((gctINT)gcoMATH_Floor(bc / e + 0.5f), 0, (1<<9)-1);

    e = gcoMATH_Exp2((gctFLOAT)((gctINT)exps - eBias - mBits));
    color[0] *= e;
    color[1] *= e;
    color[2] *= e;
    color[3] = 1.0;
}

#define gcmConvertFloatToInt(Value)             \
    (((FormatInfo->fmtDataType == gcvFORMAT_DATATYPE_SIGNED_INTEGER) ? ((gctUINT)(tempValue.int##Value)) : (tempValue.uint##Value)))

#define gcmClampInt(Value, MinValue, MaxValue)  \
    ((FormatInfo->fmtDataType == gcvFORMAT_DATATYPE_SIGNED_INTEGER) ? ((gctUINT)(gcmCLAMP((gctINT)Value, MinValue.intValue, MaxValue.intValue))) : (gcmCLAMP((gctUINT)Value, MinValue.uintValue, MaxValue.uintValue)))

static void _GetBorderColor(
    gcoHARDWARE Hardware,
    gcsTEXTURE *TexParams,
    gcsSURF_FORMAT_INFO_PTR FormatInfo,
    gctFLOAT_PTR borderColor32,
    gcsTEXTUREDESCRIPTORREGS *Desc
    )
{
    gctUINT * resultColor32 = (gctUINT *)borderColor32;
    gctUINT8 resultColor[4] = {0, 0, 0, 1};
    gceSURF_FORMAT format = FormatInfo->format;
    gctINT i;
    union {
        gctFLOAT  fv[4];
        gctINT    intborderColor32[4];
        gctUINT   uintborderColor32[4];
    } tempValue;
    union {
        gctINT    intValue;
        gctUINT   uintValue;
    } minValue, maxValue;

    gcoOS_ZeroMemory((gctPOINTER)&tempValue, gcmSIZEOF(tempValue));
    for (i = 0; i < 4; i++)
    {
        tempValue.fv[i] = borderColor32[i];
    }

    switch(FormatInfo->txFormat)
    {
    /* For unsigned normalized format texture, clamping it to [0.0, 1.0f]. */
    case 0x01:
    case 0x12:
        {
            resultColor[3] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[3], 8);

            borderColor32[3] = gcmFLOATCLAMP_0_TO_1(borderColor32[3]);
            borderColor32[0] = borderColor32[1] = borderColor32[2] = 0;
        }
        break;

    case 0x02:
        {
            resultColor[0] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[0], 8);
            resultColor[2] = resultColor[1] = resultColor[0];
            resultColor[3] = (gctUINT8)gcmFLOAT_TO_UNORM(1.0, 8);

            borderColor32[0] = gcmFLOATCLAMP_0_TO_1(borderColor32[0]);
            borderColor32[2] = borderColor32[1] = borderColor32[0];
            borderColor32[3] = 1;
        }
        break;

    case 0x03:
        break;

    case 0x04:
        {
            resultColor[3] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[3], 8);
            resultColor[0] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[0], 8);
            resultColor[2] = resultColor[1] = resultColor[0];

            borderColor32[3] = gcmFLOATCLAMP_0_TO_1(borderColor32[3]);
            borderColor32[0] = gcmFLOATCLAMP_0_TO_1(borderColor32[0]);
            borderColor32[2] = borderColor32[1] = borderColor32[0];

        }
        break;

    case 0x05:
    case 0x07:
        {
            resultColor[0] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[0], 8);
            resultColor[1] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[1], 8);
            resultColor[2] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[2], 8);
            resultColor[3] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[3], 8);

            borderColor32[0] = gcmFLOATCLAMP_0_TO_1(borderColor32[0]);
            borderColor32[1] = gcmFLOATCLAMP_0_TO_1(borderColor32[1]);
            borderColor32[2] = gcmFLOATCLAMP_0_TO_1(borderColor32[2]);
            borderColor32[3] = gcmFLOATCLAMP_0_TO_1(borderColor32[3]);
        }
        break;

    case 0x06:
    case 0x08:
        {
            resultColor[0] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[0], 8);
            resultColor[1] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[1], 8);
            resultColor[2] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[2], 8);
            resultColor[3] = (gctUINT8)gcmFLOAT_TO_UNORM(1.0, 8);

            borderColor32[0] = gcmFLOATCLAMP_0_TO_1(borderColor32[0]);
            borderColor32[1] = gcmFLOATCLAMP_0_TO_1(borderColor32[1]);
            borderColor32[2] = gcmFLOATCLAMP_0_TO_1(borderColor32[2]);
            borderColor32[3] = 1;
        }
        break;

    case 0x0B:
        {
            resultColor[0] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[0], 8);
            resultColor[1] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[1], 8);
            resultColor[2] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[2], 8);
            resultColor[3] = (gctUINT8)gcmFLOAT_TO_UNORM(1.0, 8);

            borderColor32[0] = gcmFLOATCLAMP_0_TO_1(borderColor32[0]);
            borderColor32[1] = gcmFLOATCLAMP_0_TO_1(borderColor32[1]);
            borderColor32[2] = gcmFLOATCLAMP_0_TO_1(borderColor32[2]);
            borderColor32[3] = 1;
        }
        break;

    case 0x0C:
        {
            resultColor[0] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[0], 8);
            resultColor[1] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[1], 8);
            resultColor[2] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[2], 8);
            resultColor[3] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[3], 8);

            borderColor32[0] = gcmFLOATCLAMP_0_TO_1(borderColor32[0]);
            borderColor32[1] = gcmFLOATCLAMP_0_TO_1(borderColor32[1]);
            borderColor32[2] = gcmFLOATCLAMP_0_TO_1(borderColor32[2]);
            borderColor32[3] = gcmFLOATCLAMP_0_TO_1(borderColor32[3]);
        }
        break;

    case 0x0D:
        {
            resultColor[0] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[0], 8);
            resultColor[1] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[1], 8);
            resultColor[2] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[2], 8);
            resultColor[3] = (gctUINT8)gcmFLOAT_TO_UNORM(1.0, 8);

            borderColor32[0] = gcmFLOATCLAMP_0_TO_1(borderColor32[0]);
            borderColor32[1] = gcmFLOATCLAMP_0_TO_1(borderColor32[1]);
            borderColor32[2] = gcmFLOATCLAMP_0_TO_1(borderColor32[2]);
            borderColor32[3] = 1;
        }
        break;

    /* For D16, deNormalize float R / A channel to U16 and store at R/A, GB keep to  zero. */
    case 0x10:
        {
            resultColor32[0] = gcmFLOAT_TO_UNORM(borderColor32[0], 16);
            resultColor32[3] = gcmFLOAT_TO_UNORM(borderColor32[3], 16);
            resultColor32[1] = resultColor32[2] = 0;
        }
        break;

    /* For D24X8, deNormalize float R/A channel to U24 and store at R/A, GB keep to zero.
       For D24S8, if DSMode is depth, deNormal float R/A to U24 and store at R/A, GB keep to zero,
       if DSMode is S8, interpret data in texParams as integer, and copy R/A to R/A channel, keep GB to zero.
    */
    case 0x11:
    case 0x22 << 12:
        {
            if (TexParams->dsMode != gcvTEXTURE_DS_MODE_STENCIL)
            {
                resultColor32[0] = gcmFLOAT_TO_UNORM(borderColor32[0], 24);
                resultColor32[3] = gcmFLOAT_TO_UNORM(borderColor32[3], 24);
                resultColor32[1] = resultColor32[2] = 0;
            }
            else
            {
                _GetMinAndmaxValue(FormatInfo->fmtDataType, 8, &minValue, &maxValue);
                resultColor32[0] = gcmClampInt(gcmConvertFloatToInt(borderColor32[0]), minValue, maxValue);
                resultColor32[3] = gcmClampInt(gcmConvertFloatToInt(borderColor32[3]), minValue, maxValue);
                resultColor32[1] = resultColor32[2] = 0;
            }
        }
        break;

    /* For unsigned normalized format texture, clamping it to [0.0, 1.0f] before copy them to tex descriptor.*/
    case 0x0E:
    case 0x0F:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1E:
    case 0x14 << 12:
    case 0x13 << 12:
        {
            resultColor[0] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[0], 8);
            resultColor[1] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[1], 8);
            resultColor[2] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[2], 8);
            resultColor[3] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[3], 8);

            borderColor32[0] = gcmFLOATCLAMP_0_TO_1(borderColor32[0]);
            borderColor32[1] = gcmFLOATCLAMP_0_TO_1(borderColor32[1]);
            borderColor32[2] = gcmFLOATCLAMP_0_TO_1(borderColor32[2]);
            borderColor32[3] = gcmFLOATCLAMP_0_TO_1(borderColor32[3]);
        }
        break;

    case 0x1C:
    case 0x1D:
        _EncodeRGB9E5(borderColor32);
        break;

    /* Extended format of texture. */
    case 0x00 << 12:
        {
            resultColor[0] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[0], 8);
            resultColor[1] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[1], 8);
            resultColor[2] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[2], 8);
            resultColor[3] = (gctUINT8)gcmFLOAT_TO_UNORM(1.0, 8);

            borderColor32[0] = gcmFLOATCLAMP_0_TO_1(borderColor32[0]);
            borderColor32[1] = gcmFLOATCLAMP_0_TO_1(borderColor32[1]);
            borderColor32[2] = gcmFLOATCLAMP_0_TO_1(borderColor32[2]);
            borderColor32[3] = 1;
        }
        break;

    case 0x01 << 12:
    case 0x02 << 12:
        {
            resultColor[0] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[0], 8);
            resultColor[1] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[1], 8);
            resultColor[2] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[2], 8);
            resultColor[3] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[3], 8);

            borderColor32[0] = gcmFLOATCLAMP_0_TO_1(borderColor32[0]);
            borderColor32[1] = gcmFLOATCLAMP_0_TO_1(borderColor32[1]);
            borderColor32[2] = gcmFLOATCLAMP_0_TO_1(borderColor32[2]);
            borderColor32[3] = gcmFLOATCLAMP_0_TO_1(borderColor32[3]);
        }
        break;

    case 0x03 << 12:
        {
            borderColor32[0] = gcmFLOATCLAMP_0_TO_1(borderColor32[0]);
            borderColor32[1] = 0;
            borderColor32[2] = 0;
            borderColor32[3] = 1;
        }
        break;

    case 0x04 << 12:
        {
            borderColor32[0] = gcmFLOATCLAMP_0_TO_1(borderColor32[0]);
            borderColor32[1] = gcmFLOATCLAMP_0_TO_1(borderColor32[1]);
            borderColor32[2] = 0;
            borderColor32[3] = 1;
        }
        break;

    case 0x05 << 12:
        {
            borderColor32[0] = gcmFLOATCLAMP_NEG1_TO_1(borderColor32[0]);
            borderColor32[1] = gcmFLOATCLAMP_NEG1_TO_1(borderColor32[1]);
            borderColor32[2] = 0;
            borderColor32[3] = 1;
        }
        break;

    case 0x07 << 12:
    case 0x0A << 12:
        {
            borderColor32[1] = borderColor32[2] = 0;
            borderColor32[3] = 1;
        }
        break;

    case 0x08 << 12 :
        {
            borderColor32[2] = 0;
            borderColor32[3] = 1;
        }
        break;

    case 0x0B << 12:
        {
            if (FormatInfo->format == gcvSURF_S8D32F_1_G32R32F)
            {
                if (TexParams->dsMode == gcvTEXTURE_DS_MODE_STENCIL)
                {
                    _GetMinAndmaxValue(FormatInfo->fmtDataType, 8, &minValue, &maxValue);
                    borderColor32[1] = (gctFLOAT) (gcmClampInt(resultColor32[0], minValue, maxValue));
                    borderColor32[0] = 0;
                    borderColor32[2] = 0;
                    borderColor32[3] = 1;
                }
                else
                {
                    borderColor32[1] = 0;
                    borderColor32[2] = 0;
                    borderColor32[3] = 1;
                }
            }
            else
            {
                borderColor32[2] = 0;
                borderColor32[3] = 1;
            }
        }
        break;

    case 0x1B << 12:
        {
            borderColor32[3] = 1;
        }
        break;

    case 0x09 << 12:
    case 0x0C << 12:
            break;


    /* For signed normalized format, clamping it to [-1.0f, 1.0f] before copying them to tex descriptor.*/
    case 0x0D << 12:
    case 0x0E << 12:
        {
            borderColor32[0] = gcmFLOATCLAMP_NEG1_TO_1(borderColor32[0]);
            borderColor32[1] = 0;
            borderColor32[2] = 0;
            borderColor32[3] = 1;
        }
        break;

    case 0x0F << 12:
        {
            borderColor32[0] = gcmFLOATCLAMP_NEG1_TO_1(borderColor32[0]);
            borderColor32[1] = gcmFLOATCLAMP_NEG1_TO_1(borderColor32[1]);
            borderColor32[2] = 0;
            borderColor32[3] = 1;
        }
        break;

    case 0x10 << 12:
        {
            borderColor32[0] = gcmFLOATCLAMP_NEG1_TO_1(borderColor32[0]);
            borderColor32[1] = gcmFLOATCLAMP_NEG1_TO_1(borderColor32[1]);
            borderColor32[2] = gcmFLOATCLAMP_NEG1_TO_1(borderColor32[2]);
            borderColor32[3] = 1;
        }
        break;

    case 0x11 << 12:
        {
            borderColor32[0] = gcmFLOATCLAMP_NEG1_TO_1(borderColor32[0]);
            borderColor32[1] = gcmFLOATCLAMP_NEG1_TO_1(borderColor32[1]);
            borderColor32[2] = gcmFLOATCLAMP_NEG1_TO_1(borderColor32[2]);
            borderColor32[3] = gcmFLOATCLAMP_NEG1_TO_1(borderColor32[3]);
        }
        break;

    case 0x15 << 12:
        {
            _GetMinAndmaxValue(FormatInfo->fmtDataType, 8, &minValue, &maxValue);
            resultColor32[0] = gcmClampInt(gcmConvertFloatToInt(borderColor32[0]), minValue, maxValue);
            resultColor32[1] = resultColor32[2] = 0;
            resultColor32[3] = 1;
        }
        break;

    case 0x18 << 12:
        {
            _GetMinAndmaxValue(FormatInfo->fmtDataType, 16, &minValue, &maxValue);
            resultColor32[0] = gcmClampInt(gcmConvertFloatToInt(borderColor32[0]), minValue, maxValue);
            resultColor32[1] = resultColor32[2] = 0;
            resultColor32[3] = 1;
        }
        break;

    case 0x23 << 12:
        {
            resultColor32[0] = (gctUINT)gcmConvertFloatToInt(borderColor32[0]);
            resultColor32[1] = resultColor32[2] = 0;
            resultColor32[3] = 1;
        }
        break;

    case 0x16 << 12:
        {
            _GetMinAndmaxValue(FormatInfo->fmtDataType, 8, &minValue, &maxValue);
            resultColor32[0] = gcmClampInt(gcmConvertFloatToInt(borderColor32[0]), minValue, maxValue);
            resultColor32[1] = gcmClampInt(gcmConvertFloatToInt(borderColor32[1]), minValue, maxValue);
            resultColor32[2] = 0;
            resultColor32[3] = 1;
        }
        break;

    case 0x19 << 12:
        {
            _GetMinAndmaxValue(FormatInfo->fmtDataType, 16, &minValue, &maxValue);
            resultColor32[0] = gcmClampInt(gcmConvertFloatToInt(borderColor32[0]), minValue, maxValue);
            resultColor32[1] = gcmClampInt(gcmConvertFloatToInt(borderColor32[1]), minValue, maxValue);
            resultColor32[2] = 0;
            resultColor32[3] = 1;
        }
        break;

    case 0x24 << 12:
        {
            _GetMinAndmaxValue(FormatInfo->fmtDataType, 32, &minValue, &maxValue);
            resultColor32[0] = gcmClampInt(gcmConvertFloatToInt(borderColor32[0]), minValue, maxValue);
            resultColor32[1] = gcmClampInt(gcmConvertFloatToInt(borderColor32[1]), minValue, maxValue);
            resultColor32[2] = 0;
            resultColor32[3] = 1;
        }
        break;

    case 0x1F << 12:
    case 0x20 << 12:
        {
            _GetMinAndmaxValue(FormatInfo->fmtDataType, 8, &minValue, &maxValue);
            resultColor32[0] = gcmClampInt(gcmConvertFloatToInt(borderColor32[0]), minValue, maxValue);
            resultColor32[1] = gcmClampInt(gcmConvertFloatToInt(borderColor32[1]), minValue, maxValue);
            resultColor32[2] = gcmClampInt(gcmConvertFloatToInt(borderColor32[2]), minValue, maxValue);
            resultColor32[3] = 1;
        }
        break;

    case 0x17 << 12:
        {
            _GetMinAndmaxValue(FormatInfo->fmtDataType, 8, &minValue, &maxValue);
            resultColor32[0] = gcmClampInt(gcmConvertFloatToInt(borderColor32[0]), minValue, maxValue);
            resultColor32[1] = gcmClampInt(gcmConvertFloatToInt(borderColor32[1]), minValue, maxValue);
            resultColor32[2] = gcmClampInt(gcmConvertFloatToInt(borderColor32[2]), minValue, maxValue);
            if (format != gcvSURF_X8B8G8R8I && format != gcvSURF_X8B8G8R8UI)
            {
                resultColor32[3] = gcmClampInt(gcmConvertFloatToInt(borderColor32[3]), minValue, maxValue);
            }
            else
            {
                resultColor32[3] = 1;
            }
        }
        break;

    case 0x1A << 12:
        {
            _GetMinAndmaxValue(FormatInfo->fmtDataType, 16, &minValue, &maxValue);
            resultColor32[0] = gcmClampInt(gcmConvertFloatToInt(borderColor32[0]), minValue, maxValue);
            resultColor32[1] = gcmClampInt(gcmConvertFloatToInt(borderColor32[1]), minValue, maxValue);
            resultColor32[2] = gcmClampInt(gcmConvertFloatToInt(borderColor32[2]), minValue, maxValue);
            if (format != gcvSURF_X16B16G16R16I && format != gcvSURF_X16B16G16R16UI)
            {
                resultColor32[3] = gcmClampInt(gcmConvertFloatToInt(borderColor32[3]), minValue, maxValue);
            }
            else
            {
                resultColor32[3] = 1;
            }
        }
        break;

    case 0x1C << 12:
        {
            _GetMinAndmaxValue(FormatInfo->fmtDataType, 10, &minValue, &maxValue);
            resultColor32[0] = gcmClampInt(gcmConvertFloatToInt(borderColor32[0]), minValue, maxValue);
            resultColor32[1] = gcmClampInt(gcmConvertFloatToInt(borderColor32[1]), minValue, maxValue);
            resultColor32[2] = gcmClampInt(gcmConvertFloatToInt(borderColor32[2]), minValue, maxValue);
            _GetMinAndmaxValue(FormatInfo->fmtDataType, 2, &minValue, &maxValue);
            resultColor32[3] = gcmClampInt(gcmConvertFloatToInt(borderColor32[3]), minValue, maxValue);
        }
        break;

    case 0x21 << 12:
        {
            /* Just make VS12 compiler happy */
            gctUINT8 zero = 0;

            resultColor[0] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[0], 8);
            resultColor[1] = resultColor[2] = zero;
            resultColor[3] = (gctUINT8)gcmFLOAT_TO_UNORM(1.0, 8);

            borderColor32[0] = gcmFLOATCLAMP_0_TO_1(borderColor32[0]);
            borderColor32[1] = borderColor32[2] = 0;
            borderColor32[3] = 1;
        }
        break;

    case 0x06 << 12:
        {
            /* Just make VS12 compiler happy */
            gctUINT8 zero = 0;

            resultColor[0] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[0], 8);
            resultColor[1] = (gctUINT8)gcmFLOAT_TO_UNORM(borderColor32[1], 8);
            resultColor[2] = zero;
            resultColor[3] = (gctUINT8)gcmFLOAT_TO_UNORM(1.0, 8);

            borderColor32[0] = gcmFLOATCLAMP_0_TO_1(borderColor32[0]);
            borderColor32[1] = gcmFLOATCLAMP_0_TO_1(borderColor32[1]);
            borderColor32[2] = 0;
            borderColor32[3] = 1;
        }
        break;

    case (0x00 << 12) | (1 << 18):
        {
            borderColor32[0] = gcmFLOATCLAMP_0_TO_1(borderColor32[0]);
            borderColor32[1] = gcmFLOATCLAMP_0_TO_1(borderColor32[1]);
            borderColor32[2] = gcmFLOATCLAMP_0_TO_1(borderColor32[2]);
            borderColor32[3] = 1;
        }
        break;

    case (0x07) | (1 << 18):
        {
            borderColor32[0] = gcmFLOATCLAMP_0_TO_1(borderColor32[0]);
            borderColor32[1] = gcmFLOATCLAMP_0_TO_1(borderColor32[1]);
            borderColor32[2] = gcmFLOATCLAMP_0_TO_1(borderColor32[2]);
            borderColor32[3] = gcmFLOATCLAMP_0_TO_1(borderColor32[3]);
        }
        break;

    default :
            break;
    }

    Desc->gcregTXBorderColorRed32   = resultColor32[0];
    Desc->gcregTXBorderColorGreen32 = resultColor32[1];
    Desc->gcregTXBorderColorBlue32  = resultColor32[2];
    Desc->gcregTXBorderColorAlpha32 = resultColor32[3];

    Desc->gcregTXBorderColor = (resultColor[3] << 24) | (resultColor[0] << 16) | (resultColor[1] << 8) | (resultColor[2]);
}

/*******************************************************************************
**
**  gcoHARDWARE_BindTexture
**
**  Bind texture info to a sampler.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gctINT Sampler
**          Sampler number.
**
**      gcsSAMPLER_PTR SamplerInfo
**          Pointer to a gcsSAMPLER structure.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_BindTexture(
    IN gcoHARDWARE Hardware,
    IN gctINT Sampler,
    IN gcsSAMPLER_PTR SamplerInfo
    )
{
    gceSTATUS status;
    gctBOOL   type, prevType;
    gctUINT32 width, height, depth;
    gctUINT32 endian;
    gctFIXED_POINT bias = 0, lodmin = 0, lodmax = 0;
    gctINT    format;
    gctUINT32 i, anisoLog;
    gctUINT32 sampleMode, sampleModeEx;
    gctUINT32 sampleMode2 =  0x00030000;
    gctUINT32 sampleWH, sampleLogWH, sample3D, sampleLOD, baseLOD;
    gctUINT32 sampleLinearStride;
    gctUINT32 sampleYUVControl, sampleYUVStride;
    gctUINT32 yOffset = 0, uOffset = 0, vOffset = 0;
    gctUINT32 samplerWidth, samplerHeight;
    gctUINT32 samplerDepth;
    gctUINT32 level0Width, level0Height, level0Depth;
    gceTEXTURE_FILTER minFilter;
    gceTEXTURE_FILTER mipFilter;
    gceTEXTURE_FILTER magFilter;
    gctUINT32 filterConstant;
    gctINT swapUV;
    gctINT roundUV;
    gctBOOL integerFormat = gcvFALSE;
    /*Always enable NPOT as either hw support it or not, it can't be dynamically switched as cmodel indicated*/
    gctUINT32 npot = 1;
    gctBOOL supportGCREGTX;
    gctBOOL isTexArray = gcvFALSE;
    gctBOOL isASTCtex = (SamplerInfo->formatInfo->fmtClass == gcvFORMAT_CLASS_ASTC);
    gctINT tsSlotIndex;
    gctUINT32 sliceSize = 0x00000000;
    gctUINT32 txConfig3 = 0x00000000;
    gctBOOL sampleStencilAvail;
    gctBOOL msaaTextureAvail;
    gctUINT samples;
    gctBOOL sRGBEnabled;
    gceSURF_ALIGNMENT hAlignment;

#if gcdDEBUG_OPTION && gcdDEBUG_OPTION_NONE_TEXTURE
    gcePATCH_ID patchId = gcvPATCH_INVALID;
#endif

    static const gctINT32 addressXlate[] =
    {
        /* gcvTEXTURE_INVALID */
        -1,
        /* gcvTEXTURE_CLAMP */
        0x2,
        /* gcvTEXTURE_WRAP */
        0x0,
        /* gcvTEXTURE_MIRROR */
        0x1,
        /* gcvTEXTURE_BORDER */
        0x3,
        /* gcvTEXTURE_MIRROR_ONCE */
        -1,
    };

    static const gctINT32 magXlate[] =
    {
        /* gcvTEXTURE_NONE */
        0x0,
        /* gcvTEXTURE_POINT */
        0x1,
        /* gcvTEXTURE_LINEAR */
        0x2,
        /* gcvTEXTURE_ANISOTROPIC */
        0x3,
    };

    static const gctINT32 minXlate[] =
    {
        /* gcvTEXTURE_NONE */
        0x0,
        /* gcvTEXTURE_POINT */
        0x1,
        /* gcvTEXTURE_LINEAR */
        0x2,
        /* gcvTEXTURE_ANISOTROPIC */
        0x3,
    };

    static const gctINT32 mipXlate[] =
    {
        /* gcvTEXTURE_NONE */
        0x0,
        /* gcvTEXTURE_POINT */
        0x1,
        /* gcvTEXTURE_LINEAR */
        0x2,
        /* gcvTEXTURE_ANISOTROPIC */
        0x3,
    };

    static const gctINT32 alignXlate[] =
    {
        /* gcvSURF_FOUR */
        0x0,
        /* gcvSURF_SIXTEEN */
        0x1,
        /* gcvSURF_SUPER_TILED */
        0x2,
        /* gcvSURF_SPLIT_TILED */
        0x3,
        /* gcvSURF_SPLIT_SUPER_TILED */
        0x4,
    };

    static const gctINT32 addressingXlate[] =
    {
        /* gcvSURF_NO_STRIDE_TILED. */
        0x0,
        /* gcvSURF_NO_STRIDE_LINEAR. */
        0x1,
        /* gcvSURF_STRIDE_TILED. */
        0x2,
        /* gcvSURF_STRIDE_LINEAR. */
        0x3,
    };


    static const gctINT32 modeXlate[] =
    {
        -1, /* gcvTEXTURE_COMPARE_MODE_INVALID */
        0x0, /* gcvTEXTURE_COMPARE_MODE_NONE */
        0x1, /* gcvTEXTURE_COMPARE_MODE_REF */
    };

    static const gctINT32 funcXlate[] =
    {
        -1, /* gcvCOMPARE_INVALID */
        0x7, /* gcvCOMPARE_NEVER */
        0x5, /* gcvCOMPARE_NOT_EQUAL */
        0x2, /* gcvCOMPARE_LESS */
        0x0, /* gcvCOMPARE_LESS_OR_EQUAL */
        0x4, /* gcvCOMPARE_EQUAL */
        0x3, /* gcvCOMPARE_GREATER */
        0x1, /* gcvCOMPARE_GREATER_OR_EQUAL */
        0x6, /* gcvCOMPARE_ALWAYS */
    };

    gcmHEADER_ARG("Hardware=0x%x Sampler=%d SamplerInfo=0x%x",
                  Hardware, Sampler, SamplerInfo);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    supportGCREGTX = Hardware->features[gcvFEATURE_SUPPORT_GCREGTX];

    sampleStencilAvail = Hardware->features[gcvFEATURE_D24S8_SAMPLE_STENCIL];
    msaaTextureAvail = Hardware->features[gcvFEATURE_MSAA_TEXTURE];

    if (supportGCREGTX)
    {
        gcmDEBUG_VERIFY_ARGUMENT((Sampler >= 0) && (Sampler < gcdSAMPLERS));
    }
    else
    {
        gcmDEBUG_VERIFY_ARGUMENT((Sampler >= 0) && (Sampler < 16));
    }
    gcmDEBUG_VERIFY_ARGUMENT(SamplerInfo != gcvNULL);

    samplerDepth  = SamplerInfo->depth;
    minFilter     = SamplerInfo->textureInfo->minFilter;
    mipFilter     = SamplerInfo->textureInfo->mipFilter;
    magFilter     = SamplerInfo->textureInfo->magFilter;
    samples       = SamplerInfo->baseLevelSurf->sampleInfo.product;
    samplerWidth  = SamplerInfo->width  * SamplerInfo->baseLevelSurf->sampleInfo.x;
    samplerHeight = SamplerInfo->height * SamplerInfo->baseLevelSurf->sampleInfo.y;


    if (!SamplerInfo->filterable)
    {
        if ((minFilter == gcvTEXTURE_LINEAR) || (minFilter == gcvTEXTURE_ANISOTROPIC))
        {
            minFilter = gcvTEXTURE_POINT;
        }

        if ((magFilter == gcvTEXTURE_LINEAR) || (magFilter == gcvTEXTURE_ANISOTROPIC))
        {
            magFilter = gcvTEXTURE_POINT;
        }

        if ((mipFilter == gcvTEXTURE_LINEAR) || (mipFilter == gcvTEXTURE_ANISOTROPIC))
        {
            mipFilter = gcvTEXTURE_POINT;
        }
    }

    if (mipFilter == gcvTEXTURE_LINEAR &&
        SamplerInfo->formatInfo->fmtClass == gcvFORMAT_CLASS_DEPTH)
    {
        mipFilter = gcvTEXTURE_POINT;
    }

#if gcdDEBUG_OPTION && gcdDEBUG_OPTION_NONE_TEXTURE
    gcoHAL_GetPatchID(gcvNULL, &patchId);

    if (patchId == gcvPATCH_DEBUG)
    {
        type = 0x0;
    }
    else
#endif

    if (samplerWidth == 0)
    {
        type = 0x0;
    }
    else
    {
        if (samplerHeight <= 1)
        {
            samplerHeight = 1;
        }
        else if (Hardware->specialHint & gcvHINT_BIT_0)
        {
            if (mipFilter == gcvTEXTURE_LINEAR
                && (--Hardware->specialHintData < 0)
                )
            {
#if gcdFORCE_BILINEAR
                mipFilter = gcvTEXTURE_POINT;
#endif
            }
        }

        type = _GetSamplerType(SamplerInfo->texType, &isTexArray);
    }

    if (Hardware->features[gcvFEATURE_LOD_FIX_FOR_BASELEVEL])
    {
        level0Width   = samplerWidth;
        level0Height  = samplerHeight;
        level0Depth   = samplerDepth;
    }
    else
    {
        level0Width   = samplerWidth << SamplerInfo->baseLod;
        level0Height  = samplerHeight << SamplerInfo->baseLod;
        level0Depth   = isTexArray ? samplerDepth : (samplerDepth << SamplerInfo->baseLod);
    }

    endian = _GetEndianSetting(Hardware, SamplerInfo->endianHint);

    /* Convert the format. */
    format = _GetTextureFormat(Hardware,
                               SamplerInfo->formatInfo,
                               SamplerInfo->textureInfo,
                               SamplerInfo->needYUVAssembler,
                               SamplerInfo->unsizedDepthTexture,
                               SamplerInfo->cacheMode,
                               &integerFormat,
                               &sampleMode2);


    if (Hardware->features[gcvFEATURE_TX_LOD_GUARDBAND])
    {
        sampleMode2 |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 20:20) - (0 ?
 20:20) + 1))))))) << (0 ?
 20:20))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxLODGuardbandEnable) & ((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20)));
        sampleMode2 |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:24) - (0 ?
 25:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:24) - (0 ?
 25:24) + 1))))))) << (0 ?
 25:24))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxLODGuardbandIndex) & ((gctUINT32) ((((1 ?
 25:24) - (0 ?
 25:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24)));
    }

    if (format == -1)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Swap U,V for YVYU and VYUY. */
    swapUV = SamplerInfo->format == gcvSURF_YVYU
          || SamplerInfo->format == gcvSURF_VYUY;

    if (Hardware->features[gcvFEATURE_TX_8bit_UVFrac])
    {
        roundUV = 0;
    }
    else if (Hardware->features[gcvFEATURE_TX_FRAC_PRECISION_6BIT])
    {
        roundUV = 1;
    }
    else
    {
        if (Hardware->currentApi == gcvAPI_OPENVG)
        {
            roundUV = 0;
        }
        else if (Hardware->patchID == gcvPATCH_CTGL20 || Hardware->patchID == gcvPATCH_CTGL11)
        {
            roundUV = (SamplerInfo->textureInfo->magFilter == gcvTEXTURE_POINT &&
                       samplerWidth <= 32 && samplerHeight <= 32 ) ? 0 : 1;
        }
        else if (Hardware->patchID == gcvPATCH_NETFLIX)
        {
            roundUV = 1;
        }
        else
        {
            roundUV = (SamplerInfo->textureInfo->minFilter == gcvTEXTURE_POINT ||
                       SamplerInfo->textureInfo->magFilter == gcvTEXTURE_POINT) ? 0 : 1;
        }
    }

    if ((samplerWidth  & (samplerWidth  - 1)) ||
        (samplerHeight & (samplerHeight - 1)) ||
        (samplerDepth & (samplerDepth  - 1)))
    {
        /* HW sampling non-power of two textures only work correct
           under clamp to edge mode if HW didn't support NPOT feature */
#if !gcdUSE_NPOT_PATCH
        if(Hardware->features[gcvFEATURE_NON_POWER_OF_TWO] != gcvSTATUS_TRUE)
        {
            SamplerInfo->textureInfo->s = gcvTEXTURE_CLAMP;
            SamplerInfo->textureInfo->t = gcvTEXTURE_CLAMP;
            SamplerInfo->textureInfo->r = gcvTEXTURE_CLAMP;
        }
#endif
    }

    anisoLog = (SamplerInfo->textureInfo->anisoFilter == 1) ?
               0 : gcoMATH_Log2in5dot5(SamplerInfo->textureInfo->anisoFilter);

    if (0x5 == type
    ||  0x3 == type)
    {
        anisoLog = 0;
    }

    filterConstant = 0;

    if (SamplerInfo->hasTileStatus
        && (samplerWidth >= 1024 && samplerHeight >= 1024)
        && (Hardware->specialHintData < 0)
        && (Hardware->patchID == gcvPATCH_GLBM25 ||
            Hardware->patchID == gcvPATCH_GLBM27 ||
            Hardware->patchID == gcvPATCH_GFXBENCH)
        )
    {
        minFilter = magFilter = gcvTEXTURE_POINT;
    }

    /* Set the gcregTXConfig register value. */
    sampleMode =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:0) - (0 ?
 2:0) + 1))))))) << (0 ?
 2:0))) | (((gctUINT32) ((gctUINT32) (type) & ((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 17:13) - (0 ?
 17:13) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 17:13) - (0 ?
 17:13) + 1))))))) << (0 ?
 17:13))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ?
 17:13) - (0 ?
 17:13) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 17:13) - (0 ? 17:13) + 1))))))) << (0 ? 17:13)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:3) - (0 ?
 4:3) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:3) - (0 ?
 4:3) + 1))))))) << (0 ?
 4:3))) | (((gctUINT32) ((gctUINT32) (addressXlate[SamplerInfo->textureInfo->s]) & ((gctUINT32) ((((1 ?
 4:3) - (0 ?
 4:3) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:3) - (0 ? 4:3) + 1))))))) << (0 ? 4:3)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 6:5) - (0 ?
 6:5) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:5) - (0 ?
 6:5) + 1))))))) << (0 ?
 6:5))) | (((gctUINT32) ((gctUINT32) (addressXlate[SamplerInfo->textureInfo->t]) & ((gctUINT32) ((((1 ?
 6:5) - (0 ?
 6:5) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 6:5) - (0 ? 6:5) + 1))))))) << (0 ? 6:5)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:7) - (0 ?
 8:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:7) - (0 ?
 8:7) + 1))))))) << (0 ?
 8:7))) | (((gctUINT32) ((gctUINT32) (minXlate[minFilter]) & ((gctUINT32) ((((1 ?
 8:7) - (0 ?
 8:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:7) - (0 ? 8:7) + 1))))))) << (0 ? 8:7)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:11) - (0 ?
 12:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:11) - (0 ?
 12:11) + 1))))))) << (0 ?
 12:11))) | (((gctUINT32) ((gctUINT32) (magXlate[magFilter]) & ((gctUINT32) ((((1 ?
 12:11) - (0 ?
 12:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:11) - (0 ? 12:11) + 1))))))) << (0 ? 12:11)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:9) - (0 ?
 10:9) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:9) - (0 ?
 10:9) + 1))))))) << (0 ?
 10:9))) | (((gctUINT32) ((gctUINT32) (mipXlate[mipFilter]) & ((gctUINT32) ((((1 ?
 10:9) - (0 ?
 10:9) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:9) - (0 ? 10:9) + 1))))))) << (0 ? 10:9)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 18:18) - (0 ?
 18:18) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 18:18) - (0 ?
 18:18) + 1))))))) << (0 ?
 18:18))) | (((gctUINT32) ((gctUINT32) (filterConstant) & ((gctUINT32) ((((1 ?
 18:18) - (0 ?
 18:18) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 18:18) - (0 ? 18:18) + 1))))))) << (0 ? 18:18)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 19:19) - (0 ?
 19:19) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:19) - (0 ?
 19:19) + 1))))))) << (0 ?
 19:19))) | (((gctUINT32) ((gctUINT32) (roundUV) & ((gctUINT32) ((((1 ?
 19:19) - (0 ?
 19:19) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 19:19) - (0 ? 19:19) + 1))))))) << (0 ? 19:19)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 21:20) - (0 ?
 21:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 21:20) - (0 ?
 21:20) + 1))))))) << (0 ?
 21:20))) | (((gctUINT32) ((gctUINT32) (addressingXlate[SamplerInfo->addressing]) & ((gctUINT32) ((((1 ?
 21:20) - (0 ?
 21:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20)))
        /* Compute log2 in 5.5 format. */
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:24) - (0 ?
 31:24) + 1))))))) << (0 ?
 31:24))) | (((gctUINT32) ((gctUINT32) (anisoLog) & ((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 23:22) - (0 ?
 23:22) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:22) - (0 ?
 23:22) + 1))))))) << (0 ?
 23:22))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ?
 23:22) - (0 ?
 23:22) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 23:22) - (0 ? 23:22) + 1))))))) << (0 ? 23:22)));

    hAlignment = alignXlate[SamplerInfo->hAlignment];

    if (Hardware->features[gcvFEATURE_128BTILE] && SamplerInfo->tiling == gcvYMAJOR_SUPERTILED)
    {
        hAlignment = 0x7;
    }

    /* Set the gcregTXExtConfig register value. */
    sampleModeEx =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:0) - (0 ?
 5:0) + 1))))))) << (0 ?
 5:0))) | (((gctUINT32) ((gctUINT32) ((format >> 12)) & ((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:8) - (0 ?
 10:8) + 1))))))) << (0 ?
 10:8))) | (((gctUINT32) ((gctUINT32) ((format >> 20)) & ((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:12) - (0 ?
 14:12) + 1))))))) << (0 ?
 14:12))) | (((gctUINT32) ((gctUINT32) ((format >> 23)) & ((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 18:16) - (0 ?
 18:16) + 1))))))) << (0 ?
 18:16))) | (((gctUINT32) ((gctUINT32) ((format >> 26)) & ((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 22:20) - (0 ?
 22:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 22:20) - (0 ?
 22:20) + 1))))))) << (0 ?
 22:20))) | (((gctUINT32) ((gctUINT32) ((format >> 29)) & ((gctUINT32) ((((1 ?
 22:20) - (0 ?
 22:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 22:20) - (0 ? 22:20) + 1))))))) << (0 ? 22:20)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:15) - (0 ?
 15:15) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:15) - (0 ?
 15:15) + 1))))))) << (0 ?
 15:15))) | (((gctUINT32) ((gctUINT32) (swapUV) & ((gctUINT32) ((((1 ?
 15:15) - (0 ?
 15:15) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:15) - (0 ? 15:15) + 1))))))) << (0 ? 15:15)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 30:30) - (0 ?
 30:30) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 30:30) - (0 ?
 30:30) + 1))))))) << (0 ?
 30:30))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 30:30) - (0 ?
 30:30) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 24:24) - (0 ?
 24:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 24:24) - (0 ?
 24:24) + 1))))))) << (0 ?
 24:24))) | (((gctUINT32) ((gctUINT32) (isTexArray) & ((gctUINT32) ((((1 ?
 24:24) - (0 ?
 24:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 24:24) - (0 ? 24:24) + 1))))))) << (0 ? 24:24)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 28:26) - (0 ?
 28:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 28:26) - (0 ?
 28:26) + 1))))))) << (0 ?
 28:26))) | (((gctUINT32) ((gctUINT32) (hAlignment) & ((gctUINT32) ((((1 ?
 28:26) - (0 ?
 28:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 28:26) - (0 ? 28:26) + 1))))))) << (0 ? 28:26)));

    /* Always enable seamless cubemap for es30 requires that. */
    if (Hardware->features[gcvFEATURE_TEX_SEAMLESS_CUBE])
    {
        sampleModeEx |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:25) - (0 ?
 25:25) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:25) - (0 ?
 25:25) + 1))))))) << (0 ?
 25:25))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 25:25) - (0 ?
 25:25) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25)));
    }

    /* Set the gcregTXSize register value. */
    sampleWH =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:0) - (0 ?
 14:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:0) - (0 ?
 14:0) + 1))))))) << (0 ?
 14:0))) | (((gctUINT32) ((gctUINT32) (level0Width) & ((gctUINT32) ((((1 ?
 14:0) - (0 ?
 14:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:0) - (0 ? 14:0) + 1))))))) << (0 ? 14:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 30:16) - (0 ?
 30:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 30:16) - (0 ?
 30:16) + 1))))))) << (0 ?
 30:16))) | (((gctUINT32) ((gctUINT32) (level0Height) & ((gctUINT32) ((((1 ?
 30:16) - (0 ?
 30:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 30:16) - (0 ? 30:16) + 1))))))) << (0 ? 30:16)));

    /* Compute log2 in 5.5 format for width and height. */
    width  = gcoMATH_Log2in5dot5(samplerWidth);
    height = gcoMATH_Log2in5dot5(samplerHeight);
    depth  = gcoMATH_Log2in5dot5(samplerDepth);

    /* set sRGB Decode information. */
    sRGBEnabled = (SamplerInfo->textureInfo->sRGB != gcvTEXTURE_SKIP_DECODE) && ((format >> 18) & 1);

    /* Can integer filter be used? */
    integerFormat = integerFormat
                    && (type != 0x3)
                    && (!sRGBEnabled)
                    && (anisoLog == 0);


    /* Set the gcregTXLogSize register value. */
    sampleLogWH =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:0) - (0 ?
 9:0) + 1))))))) << (0 ?
 9:0))) | (((gctUINT32) ((gctUINT32) (width) & ((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 19:10) - (0 ?
 19:10) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:10) - (0 ?
 19:10) + 1))))))) << (0 ?
 19:10))) | (((gctUINT32) ((gctUINT32) (height) & ((gctUINT32) ((((1 ?
 19:10) - (0 ?
 19:10) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 19:10) - (0 ? 19:10) + 1))))))) << (0 ? 19:10)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 29:29) - (0 ?
 29:29) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 29:29) - (0 ?
 29:29) + 1))))))) << (0 ?
 29:29))) | (((gctUINT32) ((gctUINT32) (integerFormat) & ((gctUINT32) ((((1 ?
 29:29) - (0 ?
 29:29) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 29:29) - (0 ? 29:29) + 1))))))) << (0 ? 29:29)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 30:30) - (0 ?
 30:30) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 30:30) - (0 ?
 30:30) + 1))))))) << (0 ?
 30:30))) | (((gctUINT32) ((gctUINT32) (!npot) & ((gctUINT32) ((((1 ?
 30:30) - (0 ?
 30:30) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 28:27) - (0 ?
 28:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 28:27) - (0 ?
 28:27) + 1))))))) << (0 ?
 28:27))) | (((gctUINT32) ((gctUINT32) ((isASTCtex ?
 2 : 0)) & ((gctUINT32) ((((1 ?
 28:27) - (0 ?
 28:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 28:27) - (0 ? 28:27) + 1))))))) << (0 ? 28:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:31) - (0 ?
 31:31) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:31) - (0 ?
 31:31) + 1))))))) << (0 ?
 31:31))) | (((gctUINT32) ((gctUINT32) (sRGBEnabled) & ((gctUINT32) ((((1 ?
 31:31) - (0 ?
 31:31) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31)));

    sample3D = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:0) - (0 ?
 13:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:0) - (0 ?
 13:0) + 1))))))) << (0 ?
 13:0))) | (((gctUINT32) ((gctUINT32) (level0Depth) & ((gctUINT32) ((((1 ?
 13:0) - (0 ?
 13:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0)))
             | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (depth) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
             | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 29:28) - (0 ?
 29:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 29:28) - (0 ?
 29:28) + 1))))))) << (0 ?
 29:28))) | (((gctUINT32) ((gctUINT32) (addressXlate[SamplerInfo->textureInfo->r]) & ((gctUINT32) ((((1 ?
 29:28) - (0 ?
 29:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 29:28) - (0 ? 29:28) + 1))))))) << (0 ? 29:28)));

    bias   = _Float2SignedFixed(SamplerInfo->textureInfo->lodBias, 5, 5);
    lodmax = _Float2SignedFixed(SamplerInfo->textureInfo->lodMax, 5, 5);
    lodmin = _Float2SignedFixed(SamplerInfo->textureInfo->lodMin, 5, 5);

    /* Set gcregTXLod register value. */
    sampleLOD = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) ((gctUINT32) ((bias == 0 ?
 0 : 1)) & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:1) - (0 ?
 10:1) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:1) - (0 ?
 10:1) + 1))))))) << (0 ?
 10:1))) | (((gctUINT32) ((gctUINT32) (lodmax) & ((gctUINT32) ((((1 ?
 10:1) - (0 ?
 10:1) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:1) - (0 ? 10:1) + 1))))))) << (0 ? 10:1)))
              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 20:11) - (0 ?
 20:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 20:11) - (0 ?
 20:11) + 1))))))) << (0 ?
 20:11))) | (((gctUINT32) ((gctUINT32) (lodmin) & ((gctUINT32) ((((1 ?
 20:11) - (0 ?
 20:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 20:11) - (0 ? 20:11) + 1))))))) << (0 ? 20:11)))
              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 30:21) - (0 ?
 30:21) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 30:21) - (0 ?
 30:21) + 1))))))) << (0 ?
 30:21))) | (((gctUINT32) ((gctUINT32) (bias) & ((gctUINT32) ((((1 ?
 30:21) - (0 ?
 30:21) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 30:21) - (0 ? 30:21) + 1))))))) << (0 ? 30:21)));


    if (Hardware->features[gcvFEATURE_TEX_BASELOD])
    {
        baseLOD = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (SamplerInfo->textureInfo->baseLevel) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (SamplerInfo->textureInfo->maxLevel) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 16:16) - (0 ?
 16:16) + 1))))))) << (0 ?
 16:16))) | (((gctUINT32) ((gctUINT32) (modeXlate[SamplerInfo->textureInfo->compareMode]) & ((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 22:20) - (0 ?
 22:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 22:20) - (0 ?
 22:20) + 1))))))) << (0 ?
 22:20))) | (((gctUINT32) ((gctUINT32) (funcXlate[SamplerInfo->textureInfo->compareFunc]) & ((gctUINT32) ((((1 ?
 22:20) - (0 ?
 22:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 22:20) - (0 ? 22:20) + 1))))))) << (0 ? 22:20)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 24:23) - (0 ?
 24:23) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 24:23) - (0 ?
 24:23) + 1))))))) << (0 ?
 24:23))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 24:23) - (0 ?
 24:23) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 24:23) - (0 ? 24:23) + 1))))))) << (0 ? 24:23)));
    }
    else
    {
        baseLOD = 0;
    }

    if (sampleStencilAvail)
    {
        txConfig3 |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) ((gctUINT32) ((SamplerInfo->textureInfo->dsMode == gcvTEXTURE_DS_MODE_STENCIL)) & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)));
    }

    if (msaaTextureAvail)
    {
        if (samples > 1)
        {
            txConfig3 |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:3) - (0 ?
 3:3) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:3) - (0 ?
 3:3) + 1))))))) << (0 ?
 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 3:3) - (0 ?
 3:3) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)));

            /* explicitly set slice size for array texture */
            if (SamplerInfo->texType == gcvTEXTURE_2D_MS_ARRAY)
            {
                sliceSize = SamplerInfo->baseLevelSurf->sliceSize;
            }
        }
    }

    Hardware->MCStates->texHasTileStatus[Sampler] = SamplerInfo->hasTileStatus;
    Hardware->MCStates->texBaseLevelSurfInfoWithTS[Sampler] = SamplerInfo->baseLevelSurf;

    tsSlotIndex = Hardware->MCStates->texTileStatusSlotIndex[Sampler];

    if (SamplerInfo->hasTileStatus)
    {
        if (tsSlotIndex != -1)
        {
            Hardware->MCDirty->texTileStatusSlotDirty |= 1 << tsSlotIndex;
        }
        else
        {
            /* Mark sampler to let later BindTextureTS() assign new tsSlotIndex */
            Hardware->TXDirty->hwTxSamplerDirty |= 1 << Sampler;
        }
    }
    else
    {
        if (tsSlotIndex != -1)
        {
            gcmASSERT(tsSlotIndex < gcdSAMPLER_TS);
            Hardware->MCStates->texTileStatusSlotUser[tsSlotIndex] = -1;
            Hardware->MCStates->texTileStatusSlotIndex[Sampler] = -1;
            Hardware->MCDirty->texTileStatusSlotDirty |= 1 << tsSlotIndex;
        }
    }

    /* Program stride for linear or msaa texture. */
    if ((SamplerInfo->addressing == gcvSURF_STRIDE_LINEAR) ||
        (samples > 1))
    {
        /* explicitely set texture stride. */
        sampleLinearStride = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 17:0) - (0 ?
 17:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 17:0) - (0 ?
 17:0) + 1))))))) << (0 ?
 17:0))) | (((gctUINT32) ((gctUINT32) (SamplerInfo->lodStride[SamplerInfo->baseLod]) & ((gctUINT32) ((((1 ?
 17:0) - (0 ?
 17:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 17:0) - (0 ? 17:0) + 1))))))) << (0 ? 17:0)));
    }
    else
    {
        sampleLinearStride = 0;
    }

    /* Program YUV-assembler information. */
    if (SamplerInfo->needYUVAssembler)
    {
        /* YUV-assembler can only use LOD 0.
         * Y plane address is from LOD 0,
         * U plane address is from LOD 1,
         * V plane address is from LOD 2,
         * Set stride from lodStride. */
        sampleYUVControl = _GetYUVControl(
            SamplerInfo->format,
            &yOffset,
            &uOffset,
            &vOffset
            );

        /* YUV strides. */
        sampleLinearStride = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 17:0) - (0 ?
 17:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 17:0) - (0 ?
 17:0) + 1))))))) << (0 ?
 17:0))) | (((gctUINT32) ((gctUINT32) (SamplerInfo->lodStride[0]) & ((gctUINT32) ((((1 ?
 17:0) - (0 ?
 17:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 17:0) - (0 ? 17:0) + 1))))))) << (0 ? 17:0)));

        sampleYUVStride  =
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (SamplerInfo->lodStride[1]) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (SamplerInfo->lodStride[2]) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)));

    }
    else
    {
        sampleYUVControl = 0;
        sampleYUVStride  = 0;
    }

    prevType = (((((gctUINT32) (Hardware->TXStates->hwTxSamplerMode[Sampler])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0U : (~(~0U << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) );

#if gcdSECURITY
    Hardware->TXStates->txLodNums[Sampler] = SamplerInfo->lodNum;
#endif

    /* Save changed sampler mode register. */
    if (Hardware->TXStates->hwTxSamplerMode[Sampler] != sampleMode)
    {
        Hardware->TXStates->hwTxSamplerMode[Sampler] = sampleMode;
        Hardware->TXDirty->hwTxSamplerModeDirty    |= 1 << Sampler;
    }

    if (Hardware->TXStates->hwTxSamplerModeEx[Sampler] != sampleModeEx)
    {
        Hardware->TXStates->hwTxSamplerModeEx[Sampler] = sampleModeEx;
        Hardware->TXDirty->hwTxSamplerModeDirty      |= 1 << Sampler;
    }

    if (Hardware->TXStates->hwTxSamplerMode2[Sampler] != sampleMode2)
    {
        Hardware->TXStates->hwTxSamplerMode2[Sampler] = sampleMode2;
        Hardware->TXDirty->hwTxSamplerModeDirty      |= 1 << Sampler;
    }

    /* Save changed sampler size register. */
    if (Hardware->TXStates->hwTxSamplerSize[Sampler] != sampleWH)
    {
        Hardware->TXStates->hwTxSamplerSize[Sampler] = sampleWH;
        Hardware->TXDirty->hwTxSamplerSizeDirty    |= 1 << Sampler;
    }

    /* Save changed sampler log2 size register. */
    if (Hardware->TXStates->hwTxSamplerSizeLog[Sampler] != sampleLogWH)
    {
        Hardware->TXStates->hwTxSamplerSizeLog[Sampler] = sampleLogWH;
        Hardware->TXDirty->hwTxSamplerSizeLogDirty    |= 1 << Sampler;
    }

    /* Save changed sampler 3D register. */
    /* GCREG_TX3_D did not take effect when texture type is not 3D, need to reprogram if switched. */
    if ((Hardware->TXStates->hwTxSampler3D[Sampler] != sample3D) ||
        (type == 0x3 && prevType != 0x3))
    {
        Hardware->TXStates->hwTxSampler3D[Sampler]  = sample3D;
        Hardware->TXDirty->hwTxSampler3DDirty     |= 1 << Sampler;
    }

    /* Save changed sampler LOD register. */
    if (Hardware->TXStates->hwTxSamplerLOD[Sampler] != sampleLOD)
    {
        Hardware->TXStates->hwTxSamplerLOD[Sampler] = sampleLOD;
        Hardware->TXDirty->hwTxSamplerLODDirty    |= 1 << Sampler;
    }

    /* Save changed base LOD register. */
    if (Hardware->TXStates->hwTxSamplerBaseLOD[Sampler] != baseLOD)
    {
        Hardware->TXStates->hwTxSamplerBaseLOD[Sampler]  = baseLOD;
        Hardware->TXDirty->hwTxSamplerBaseLODDirty     |= 1 << Sampler;
    }

    /* Save changed sampler linear stride register. */
    if (Hardware->TXStates->hwTxSamplerLinearStride[Sampler] != sampleLinearStride)
    {
        Hardware->TXStates->hwTxSamplerLinearStride[Sampler] = sampleLinearStride;
        Hardware->TXDirty->hwTxSamplerLinearStrideDirty    |= 1 << Sampler;
    }

    /* Save changed sampler YUV-assembler control register. */
    if (Hardware->TXStates->hwTxSamplerYUVControl[Sampler] != sampleYUVControl)
    {
        Hardware->TXStates->hwTxSamplerYUVControl[Sampler] = sampleYUVControl;
        Hardware->TXDirty->hwTxSamplerYUVControlDirty    |= 1 << Sampler;
    }

    /* Save changed sampler YUV stride register. */
    if (Hardware->TXStates->hwTxSamplerYUVStride[Sampler] != sampleYUVStride)
    {
        Hardware->TXStates->hwTxSamplerYUVStride[Sampler] = sampleYUVStride;
        Hardware->TXDirty->hwTxSamplerYUVStrideDirty    |= 1 << Sampler;
    }

    /* Set each LOD. */
    for (i = SamplerInfo->baseLod; i < SamplerInfo->lodNum; i++)
    {
        if (Hardware->TXStates->hwTxSamplerAddress[i][Sampler] != SamplerInfo->lodAddr[i])
        {
            Hardware->TXStates->hwTxSamplerAddress[i][Sampler] = SamplerInfo->lodAddr[i];
            Hardware->TXDirty->hwTxSamplerAddressDirty[i]    |= 1 << Sampler;

            if (!supportGCREGTX)
            {
                /* we should always set stride if address changes. Otherwise, hw doesn't use
                ** correct stride to get texture data. This fix is for bug7155. please refer to CL6130.
                */
                Hardware->TXDirty->hwTxSamplerLinearStrideDirty  |= 1 << Sampler;
            }
        }
    }

    /* Need adjust LOD addresses for YUV-assembler. */
    if (SamplerInfo->needYUVAssembler)
    {
        Hardware->TXStates->hwTxSamplerAddress[0][Sampler] += yOffset;
        Hardware->TXStates->hwTxSamplerAddress[1][Sampler] += uOffset;
        Hardware->TXStates->hwTxSamplerAddress[2][Sampler] += vOffset;
    }

    /* ASTC textures. */
    if (Hardware->features[gcvFEATURE_TEXTURE_ASTC])
    {
        gctUINT baseLOD = SamplerInfo->baseLod;

        for (i = baseLOD; i < SamplerInfo->lodNum; ++i)
        {
            if ((Hardware->TXStates->hwTxSamplerASTCSize[i][Sampler] != SamplerInfo->astcSize[i])
            ||  (Hardware->TXStates->hwTxSamplerASTCSRGB[i][Sampler] != SamplerInfo->astcSRGB[i]))
            {
                Hardware->TXStates->hwTxSamplerASTCSize[i][Sampler] = SamplerInfo->astcSize[i];
                Hardware->TXStates->hwTxSamplerASTCSRGB[i][Sampler] = SamplerInfo->astcSRGB[i];
                Hardware->TXDirty->hwTxSamplerASTCDirty[i / 4]     |= 1 << Sampler;
            }
        }

        if (!Hardware->features[gcvFEATURE_TEXTURE_ASTC_BASE_LOD_FIX] &&
            SamplerInfo->formatInfo->fmtClass == gcvFORMAT_CLASS_ASTC &&
            baseLOD > 0)
        {
            for (i = 0; i < baseLOD; ++i)
            {
                if ((Hardware->TXStates->hwTxSamplerASTCSize[i][Sampler] != SamplerInfo->astcSize[baseLOD])
                ||  (Hardware->TXStates->hwTxSamplerASTCSRGB[i][Sampler] != SamplerInfo->astcSRGB[baseLOD]))
                {
                    Hardware->TXStates->hwTxSamplerASTCSize[i][Sampler] = SamplerInfo->astcSize[baseLOD];
                    Hardware->TXStates->hwTxSamplerASTCSRGB[i][Sampler] = SamplerInfo->astcSRGB[baseLOD];
                    Hardware->TXDirty->hwTxSamplerASTCDirty[i / 4]     |= 1 << Sampler;
                }
            }
        }
    }

    if (Hardware->features[gcvFEATURE_TX_FRAC_PRECISION_6BIT])
    {
        gctUINT32 sampleLogWHExt, sample3DExt, sampleLodExt, sampleLodBiasExt, anisoCtrl;

        /* Compute log2 in 5.8 format for dimensions. */
        width  = _Log2inXdot8(samplerWidth);
        height = _Log2inXdot8(samplerHeight);
        depth  = _Log2inXdot8(samplerDepth);

        anisoLog = (SamplerInfo->textureInfo->anisoFilter == 1) ?
                   0 : _Log2inXdot8(SamplerInfo->textureInfo->anisoFilter);

        if (0x5 == type
        ||  0x3 == type)
        {
            anisoLog = 0;
        }

        /* Compute 5.8 format values */
        bias   = _Float2SignedFixed(SamplerInfo->textureInfo->lodBias, 8, 8);
        lodmax = _Float2SignedFixed(SamplerInfo->textureInfo->lodMax, 5, 8);
        lodmin = _Float2SignedFixed(SamplerInfo->textureInfo->lodMin, 5, 8);

        sampleLogWHExt   = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (width) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
                         | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (height) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)));

        sample3DExt      = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (depth) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

        sampleLodExt     = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:0) - (0 ?
 12:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:0) - (0 ?
 12:0) + 1))))))) << (0 ?
 12:0))) | (((gctUINT32) ((gctUINT32) (lodmax) & ((gctUINT32) ((((1 ?
 12:0) - (0 ?
 12:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0)))
                         | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 28:16) - (0 ?
 28:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 28:16) - (0 ?
 28:16) + 1))))))) << (0 ?
 28:16))) | (((gctUINT32) ((gctUINT32) (lodmin) & ((gctUINT32) ((((1 ?
 28:16) - (0 ?
 28:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16)));

        sampleLodBiasExt = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (bias) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
                         | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 16:16) - (0 ?
 16:16) + 1))))))) << (0 ?
 16:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)));

        anisoCtrl        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:0) - (0 ?
 10:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:0) - (0 ?
 10:0) + 1))))))) << (0 ?
 10:0))) | (((gctUINT32) ((gctUINT32) (anisoLog) & ((gctUINT32) ((((1 ?
 10:0) - (0 ?
 10:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:0) - (0 ? 10:0) + 1))))))) << (0 ? 10:0)));

        if ((Hardware->TXStates->hwTxSamplerSizeLogExt[Sampler] != sampleLogWHExt) ||
            (Hardware->TXDirty->hwTxSamplerSizeLogDirty & (1 << Sampler)))
        {
            Hardware->TXStates->hwTxSamplerSizeLogExt[Sampler]  = sampleLogWHExt;
            Hardware->TXDirty->hwTxSamplerSizeLogExtDirty     |= 1 << Sampler;
        }

        if ((Hardware->TXStates->hwTxSampler3DExt[Sampler]      != sample3DExt) ||
            (Hardware->TXDirty->hwTxSampler3DDirty & (1 << Sampler)))
        {
            Hardware->TXStates->hwTxSampler3DExt[Sampler]       = sample3DExt;
            Hardware->TXDirty->hwTxSampler3DExtDirty          |= 1 << Sampler;
        }

        if ((Hardware->TXStates->hwTxSamplerLodExt[Sampler]     != sampleLodExt) ||
            (Hardware->TXDirty->hwTxSamplerLODDirty & (1 << Sampler)))
        {
            Hardware->TXStates->hwTxSamplerLodExt[Sampler]      = sampleLodExt;
            Hardware->TXDirty->hwTxSamplerLodExtDirty         |= 1 << Sampler;
        }

        if ((Hardware->TXStates->hwTxSamplerLodBiasExt[Sampler] != sampleLodBiasExt) ||
            (Hardware->TXDirty->hwTxSamplerLODDirty & (1 << Sampler)))
        {
            Hardware->TXStates->hwTxSamplerLodBiasExt[Sampler]  = sampleLodBiasExt;
            Hardware->TXDirty->hwTxSamplerLodBiasExtDirty     |= 1 << Sampler;
        }

        if ((Hardware->TXStates->hwTxSamplerAnisoCtrl[Sampler] != anisoCtrl) ||
            (Hardware->TXDirty->hwTxSamplerModeDirty & (1 << Sampler)))
        {
            Hardware->TXStates->hwTxSamplerAnisoCtrl[Sampler]  = anisoCtrl;
            Hardware->TXDirty->hwTxSamplerAnisoCtrlDirty     |= 1 << Sampler;
        }

    }

    if (sampleStencilAvail || msaaTextureAvail)
    {
        if (Hardware->TXStates->hwTxSamplerConfig3[Sampler] != txConfig3)
        {
            Hardware->TXStates->hwTxSamplerConfig3[Sampler] = txConfig3;
            Hardware->TXDirty->hwTxSamplerConfig3Dirty    |= 1 << Sampler;
        }

        if (Hardware->TXStates->hwTxSamplerSliceSize[Sampler] != sliceSize)
        {
            Hardware->TXStates->hwTxSamplerSliceSize[Sampler] = sliceSize;
            Hardware->TXDirty->hwTxSamplerConfig3Dirty    |= 1 << Sampler;
        }
    }

    if (Hardware->features[gcvFEATURE_TX_FLUSH_L1CACHE])
    {
        Hardware->TXDirty->hwTxFlushL1Cache = (SamplerInfo->needYUVAssembler || isASTCtex) ? gcvTRUE : gcvFALSE;
    }

    Hardware->TXDirty->hwTxSamplerDirty |=
                   Hardware->TXDirty->hwTxSamplerModeDirty         |
                   Hardware->TXDirty->hwTxSamplerSizeDirty         |
                   Hardware->TXDirty->hwTxSamplerSizeLogDirty      |
                   Hardware->TXDirty->hwTxSampler3DDirty           |
                   Hardware->TXDirty->hwTxSamplerLODDirty          |
                   Hardware->TXDirty->hwTxSamplerBaseLODDirty      |
                   Hardware->TXDirty->hwTxSamplerYUVControlDirty   |
                   Hardware->TXDirty->hwTxSamplerYUVStrideDirty    |
                   Hardware->TXDirty->hwTxSamplerLinearStrideDirty |
                   Hardware->TXDirty->hwTxSamplerSizeLogExtDirty   |
                   Hardware->TXDirty->hwTxSampler3DExtDirty        |
                   Hardware->TXDirty->hwTxSamplerLodExtDirty       |
                   Hardware->TXDirty->hwTxSamplerLodBiasExtDirty   |
                   Hardware->TXDirty->hwTxSamplerAnisoCtrlDirty    |
                   Hardware->TXDirty->hwTxSamplerConfig3Dirty;

    for (i = 0; i < gcdLOD_LEVELS; i++)
    {
        Hardware->TXDirty->hwTxSamplerDirty |= Hardware->TXDirty->hwTxSamplerAddressDirty[i] |
                                      Hardware->TXDirty->hwTxSamplerASTCDirty[i];
    }

    if (Hardware->TXDirty->hwTxSamplerDirty)
    {
        Hardware->TXDirty->textureDirty = gcvTRUE;
    }


    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_BindTextureTS
**
**  Bind all sampler with TS if it's available
**  This function must be call after all samplers are settle down.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_BindTextureTS(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctINT sampler;
    gctUINT samplerTSIndex;
    gctINT32 samplerTSSlotDirty;
    gcoSURF surface;
    gctBOOL texTSMapping;
    gcsBITMASK samplerDirty;
    gcsSURF_VIEW texView = {gcvNULL, 0, 1};

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    texTSMapping = Hardware->features[gcvFEATURE_TX_TILE_STATUS_MAPPING];

    if (Hardware->features[gcvFEATURE_TX_DESCRIPTOR])
    {
        samplerDirty = Hardware->TXDirty->hwTxDescDirty;
    }
    else
    {
        gcsBITMASK_InitWithValue(&samplerDirty, Hardware->TXDirty->hwTxSamplerDirty);
    }

    for (sampler = 0;
         !gcsBITMASK_IsAllZero(&samplerDirty);
         sampler++)
    {
        gctBOOL enableTexTS = Hardware->MCStates->texHasTileStatus[sampler];
        gctINT texTSSlot = -1;
        gctINT j;

        if (!gcsBITMASK_TestAndClear(&samplerDirty, sampler))
        {
            continue;
        }

        if (enableTexTS)
        {
            surface = Hardware->MCStates->texBaseLevelSurfInfoWithTS[sampler];
            gcmASSERT(surface);
            gcmASSERT(surface->formatInfo.layers <= 1);

            texView.surf = surface;
            texView.numSlices = surface->requestD;

            if (Hardware->MCStates->texTileStatusSlotIndex[sampler] == -1)
            {
                if (texTSMapping)
                {
                    for (j = 0; j < gcdSAMPLER_TS; j++)
                    {
                        if (Hardware->MCStates->texTileStatusSlotUser[j] == -1)
                        {
                            texTSSlot = j;
                            break;
                        }
                    }
                }
                else if (sampler < gcdSAMPLER_TS)
                {
                    texTSSlot = sampler;
                }
                /* Can't find the slot */
                if (texTSSlot == -1)
                {
#if gcdDEBUG_OPTION
                    gcmPRINT("texture TS has to be disabled as out of slots");
#endif
                    gcmONERROR(gcoHARDWARE_DisableTileStatus(Hardware, &texView, gcvTRUE));
                    enableTexTS = gcvFALSE;
                }

                if (texTSSlot != -1)
                {
                    Hardware->MCStates->texTileStatusSlotUser[texTSSlot] = sampler;
                    Hardware->MCStates->texTileStatusSlotIndex[sampler] = texTSSlot;
                    Hardware->MCDirty->texTileStatusSlotDirty |= 1 << texTSSlot;
                }
            }
        }
    }

    samplerTSSlotDirty = Hardware->MCDirty->texTileStatusSlotDirty;

    for (samplerTSIndex = 0;
         samplerTSSlotDirty;
         samplerTSIndex++, samplerTSSlotDirty >>= 1)
    {
        gctUINT32 sampleTSConfig = 0, sampleTSBuffer = 0, sampleTSClearValue = 0, sampleTSClearValueUpper = 0;
        gctUINT32 sampleTexBaseBuffer = 0;

        if (!(samplerTSSlotDirty & 0x1))
        {
            continue;
        }

        sampler = Hardware->MCStates->texTileStatusSlotUser[samplerTSIndex];
        /* Program tile status information. */
        if (sampler != -1)
        {
            gctUINT control;
            gctBOOL isDecFormat;
            gctBOOL msaaTexture;

            surface = Hardware->MCStates->texBaseLevelSurfInfoWithTS[sampler];

            if (!surface->compressed)
            {
                control = 1; /* Enable */
            }
            else
            {
                /*
                ** 1, invalid compression format to tx decoding.
                ** 2, v1 tx can't support z16 decompression.
                */
                if ((surface->compressFormat == -1) ||
                    ((surface->compressFormat == 0x8) &&
                     (Hardware->features[gcvFEATURE_COMPRESSION_V1])))
                {
                    gcmFATAL("tx decoder is taking invalid compression format, surface format=%d",
                              surface->format);
                }

                /* Enabled and compressed */
                control = 3;
            }

            isDecFormat = surface->compressDecFormat != -1;

            msaaTexture = surface->isMsaa;

            sampleTSConfig     = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) ((gctUINT32) (control) & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
                               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 7:4) - (0 ?
 7:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 7:4) - (0 ?
 7:4) + 1))))))) << (0 ?
 7:4))) | (((gctUINT32) ((gctUINT32) (surface->compressFormat) & ((gctUINT32) ((((1 ?
 7:4) - (0 ?
 7:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4)))
                               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 9:9) - (0 ?
 9:9) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:9) - (0 ?
 9:9) + 1))))))) << (0 ?
 9:9))) | (((gctUINT32) ((gctUINT32) (surface->formatInfo.bitsPerPixel == 64) & ((gctUINT32) ((((1 ?
 9:9) - (0 ?
 9:9) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9)))
                               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:11) - (0 ?
 13:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:11) - (0 ?
 13:11) + 1))))))) << (0 ?
 13:11))) | (((gctUINT32) ((gctUINT32) (surface->compressDecFormat) & ((gctUINT32) ((((1 ?
 13:11) - (0 ?
 13:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:11) - (0 ? 13:11) + 1))))))) << (0 ? 13:11)))
                               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:10) - (0 ?
 10:10) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:10) - (0 ?
 10:10) + 1))))))) << (0 ?
 10:10))) | (((gctUINT32) ((gctUINT32) (isDecFormat ?
 0x1 : 0x0) & ((gctUINT32) ((((1 ?
 10:10) - (0 ?
 10:10) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:10) - (0 ? 10:10) + 1))))))) << (0 ? 10:10)));

            if (Hardware->features[gcvFEATURE_MSAA_TEXTURE] && surface->isMsaa)
            {
                sampleTSConfig |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 16:16) - (0 ?
 16:16) + 1))))))) << (0 ?
 16:16))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)));
            }

            if ((Hardware->features[gcvFEATURE_COMPRESSION_V4] ||
                 (gcoHAL_GetOption(gcvNULL, gcvOPTION_COMPRESSION_DEC400) && Hardware->features[gcvFEATURE_COMPRESSION_DEC400]))
            &&  surface->isMsaa)
            {
                sampleTSConfig |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 18:18) - (0 ?
 18:18) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 18:18) - (0 ?
 18:18) + 1))))))) << (0 ?
 18:18))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 18:18) - (0 ?
 18:18) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 18:18) - (0 ? 18:18) + 1))))))) << (0 ? 18:18)));
            }

            sampleTSClearValue = surface->fcValue[0];
            sampleTSClearValueUpper = surface->fcValueUpper[0];
            if (surface->formatInfo.endian != gcvENDIAN_NO_SWAP)
            {
                sampleTSClearValue = gcmBSWAP32(sampleTSClearValue);
                sampleTSClearValueUpper = gcmBSWAP32(sampleTSClearValueUpper);
            }

            gcmGETHARDWAREADDRESS(surface->tileStatusNode, sampleTSBuffer);
            gcmGETHARDWAREADDRESS(surface->node, sampleTexBaseBuffer);

            if (Hardware->features[gcvFEATURE_TX_DESCRIPTOR])
            {
                gctUINT32 textureControl = 0;

                if (Hardware->features[gcvFEATURE_128BTILE])
                {
                    textureControl =
                            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) ((gctUINT32) (surface->cacheMode == gcvCACHE_256) & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:1) - (0 ?
 1:1) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:1) - (0 ?
 1:1) + 1))))))) << (0 ?
 1:1))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 1:1) - (0 ?
 1:1) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:2) - (0 ?
 4:2) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:2) - (0 ?
 4:2) + 1))))))) << (0 ?
 4:2))) | (((gctUINT32) ((gctUINT32) (samplerTSIndex) & ((gctUINT32) ((((1 ?
 4:2) - (0 ?
 4:2) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:2) - (0 ? 4:2) + 1))))))) << (0 ? 4:2)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:5) - (0 ?
 5:5) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:5) - (0 ?
 5:5) + 1))))))) << (0 ?
 5:5))) | (((gctUINT32) ((gctUINT32) ((isDecFormat ?
 1 : 0)) & ((gctUINT32) ((((1 ?
 5:5) - (0 ?
 5:5) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:6) - (0 ?
 6:6) + 1))))))) << (0 ?
 6:6))) | (((gctUINT32) ((gctUINT32) (surface->cacheMode == gcvCACHE_128) & ((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
                }
                else
                {
                    textureControl =
                        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) ((gctUINT32) (msaaTexture ?
 1 : 0) & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:1) - (0 ?
 1:1) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:1) - (0 ?
 1:1) + 1))))))) << (0 ?
 1:1))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 1:1) - (0 ?
 1:1) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:2) - (0 ?
 4:2) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:2) - (0 ?
 4:2) + 1))))))) << (0 ?
 4:2))) | (((gctUINT32) ((gctUINT32) (samplerTSIndex) & ((gctUINT32) ((((1 ?
 4:2) - (0 ?
 4:2) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:2) - (0 ? 4:2) + 1))))))) << (0 ? 4:2)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:5) - (0 ?
 5:5) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:5) - (0 ?
 5:5) + 1))))))) << (0 ?
 5:5))) | (((gctUINT32) ((gctUINT32) ((isDecFormat ?
 1 : 0)) & ((gctUINT32) ((((1 ?
 5:5) - (0 ?
 5:5) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5)));
                }

                if (textureControl != Hardware->TXStates->hwTextureControl[sampler])
                {
                    Hardware->TXStates->hwTextureControl[sampler] = textureControl;
                    gcsBITMASK_Set(&Hardware->TXDirty->hwTextureControlDirty, sampler);
                    gcsBITMASK_Set(&Hardware->TXDirty->hwTxDescDirty, sampler);
                }
            }
            else
            {
                gctUINT sampleModeEx = Hardware->TXStates->hwTxSamplerModeEx[sampler];
                gctUINT sampleMode2 = Hardware->TXStates->hwTxSamplerMode2[sampler];
                sampleModeEx |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 30:30) - (0 ?
 30:30) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 30:30) - (0 ?
 30:30) + 1))))))) << (0 ?
 30:30))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 30:30) - (0 ?
 30:30) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30)));

                if (Hardware->features[gcvFEATURE_128BTILE] &&
                    surface->cacheMode == gcvCACHE_256)
                {
                    sampleModeEx |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 23:23) - (0 ?
 23:23) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:23) - (0 ?
 23:23) + 1))))))) << (0 ?
 23:23))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 23:23) - (0 ?
 23:23) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 23:23) - (0 ? 23:23) + 1))))))) << (0 ? 23:23)));
                }
                else if (msaaTexture)
                {
                    sampleModeEx |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 23:23) - (0 ?
 23:23) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:23) - (0 ?
 23:23) + 1))))))) << (0 ?
 23:23))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 23:23) - (0 ?
 23:23) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 23:23) - (0 ? 23:23) + 1))))))) << (0 ? 23:23)));
                }

                sampleMode2 |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 30:28) - (0 ?
 30:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 30:28) - (0 ?
 30:28) + 1))))))) << (0 ?
 30:28))) | (((gctUINT32) ((gctUINT32) (samplerTSIndex ) & ((gctUINT32) ((((1 ?
 30:28) - (0 ?
 30:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

                if (isDecFormat)
                {
                    sampleMode2 |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:31) - (0 ?
 31:31) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:31) - (0 ?
 31:31) + 1))))))) << (0 ?
 31:31))) | (((gctUINT32) ((gctUINT32) (0x1) & ((gctUINT32) ((((1 ?
 31:31) - (0 ?
 31:31) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31)));
                }

                if ((Hardware->TXStates->hwTxSamplerMode2[sampler] != sampleMode2) ||
                    (Hardware->TXStates->hwTxSamplerModeEx[sampler] != sampleModeEx))
                {
                    Hardware->TXStates->hwTxSamplerModeEx[sampler] = sampleModeEx;
                    Hardware->TXStates->hwTxSamplerMode2[sampler] = sampleMode2;
                    Hardware->TXDirty->hwTxSamplerModeDirty |= 1 << sampler;
                    Hardware->TXDirty->hwTxSamplerDirty |= 1 << sampler;
                }
            }
        }
        else
        {
            sampleTSConfig     = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)));
            sampleTSClearValue = 0x0;
            sampleTSClearValueUpper = 0x0;
            sampleTSBuffer     = 0x00C0FFEE;
            sampleTexBaseBuffer = 0x00C0FFEF;
        }

        if ((Hardware->MCStates->hwTXSampleTSConfig[samplerTSIndex]     != sampleTSConfig)
        ||  (Hardware->MCStates->hwTXSampleTSBuffer[samplerTSIndex]     != sampleTSBuffer)
        ||  (Hardware->MCStates->hwTXSampleTSClearValue[samplerTSIndex] != sampleTSClearValue)
        ||  (Hardware->MCStates->hwTXSampleTSClearValueUpper[samplerTSIndex] != sampleTSClearValueUpper)
        ||  (texTSMapping && Hardware->MCStates->hwTxSamplerTxBaseBuffer[samplerTSIndex] != sampleTexBaseBuffer))
        {
            Hardware->MCStates->hwTXSampleTSConfig[samplerTSIndex]     = sampleTSConfig;
            Hardware->MCStates->hwTXSampleTSBuffer[samplerTSIndex]     = sampleTSBuffer;
            Hardware->MCStates->hwTXSampleTSClearValue[samplerTSIndex] = sampleTSClearValue;
            Hardware->MCStates->hwTXSampleTSClearValueUpper[samplerTSIndex] = sampleTSClearValueUpper;
            Hardware->MCStates->hwTxSamplerTxBaseBuffer[samplerTSIndex] = sampleTexBaseBuffer;
            Hardware->MCDirty->hwTxSamplerTSDirty             |= 1 << samplerTSIndex;
        }
    }

    /* clear dirty bit now */
    Hardware->MCDirty->texTileStatusSlotDirty = 0;


    if ((Hardware->MCDirty->hwTxSamplerTSDirty) ||
        (Hardware->TXDirty->hwTxSamplerDirty)   ||
        (Hardware->features[gcvFEATURE_TX_DESCRIPTOR] && !gcsBITMASK_IsAllZero(&Hardware->TXDirty->hwTxDescDirty))
       )
    {
        Hardware->TXDirty->textureDirty = gcvTRUE;
    }


OnError:
    gcmFOOTER_NO();
    return status;

}



/*******************************************************************************
**
**  gcoHARDWARE_ProgramTexture
**
**  Program all dirty texture states.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_ProgramTexture(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    )
{
    static const gctUINT32 address[gcdLOD_LEVELS] =
    {
        0x0900,
        0x0910,
        0x0920,
        0x0930,
        0x0940,
        0x0950,
        0x0960,
        0x0970,
        0x0980,
        0x0990,
        0x09A0,
        0x09B0,
        0x09C0,
        0x09D0
    };

    gceSTATUS status = gcvSTATUS_OK;
    gctUINT dirty, sampler, i, index, semaStall;
    gctBOOL supportGCREGTX;
    gctBOOL halti2Avail;
    gctBOOL halti3Avail;
    gctBOOL unifiedSampler;
    gctUINT32 vsSamplerMask;
    gctUINT32 psSamplerMask;
    gctINT GPipeBase;
    gctBOOL texTSMapping;
    gctBOOL texDecSupported;
    gctUINT32 samplerDirty;
    gctUINT32 shaderConfigData = 0;
    gctINT32 samplerTSDirty;
    gctUINT  samplerTSIndex;
    gctBOOL separateSteeringBit;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETHARDWARE(Hardware);

    gcmASSERT(!Hardware->features[gcvFEATURE_SMALL_BATCH]);
    gcmASSERT(!Hardware->features[gcvFEATURE_MULTI_CLUSTER]);

    supportGCREGTX = Hardware->features[gcvFEATURE_SUPPORT_GCREGTX];
    halti2Avail = Hardware->features[gcvFEATURE_HALTI2];
    halti3Avail = Hardware->features[gcvFEATURE_HALTI3];
    unifiedSampler = Hardware->features[gcvFEATURE_UNIFIED_SAMPLERS];
    texTSMapping = Hardware->features[gcvFEATURE_TX_TILE_STATUS_MAPPING];
    texDecSupported = Hardware->features[gcvFEATURE_TX_SUPPORT_DEC];

    separateSteeringBit = Hardware->features[gcvFEATURE_NEW_STEERING_AND_ICACHE_FLUSH];

    /* For non-unified mode, GPipeStart is the vsSamplerBase. */
    GPipeBase = Hardware->SHStates->programState.hints->unifiedStatus.samplerGPipeStart;

    vsSamplerMask =(~0) << GPipeBase;
    psSamplerMask = ~vsSamplerMask;

    samplerDirty = Hardware->TXDirty->hwTxSamplerDirty;
    samplerTSDirty = Hardware->MCDirty->hwTxSamplerTSDirty;

    vsSamplerMask &= samplerDirty;
    psSamplerMask &= samplerDirty;

    shaderConfigData = Hardware->SHStates->programState.hints->shaderConfigData;

    if (Hardware->MCDirty->hwTxSamplerTSDirty)
    {
        semaStall = 1;
    }
    else
    {
        semaStall = 0;
    }

    {        /* Reserve space in the command buffer. */
        gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, Memory);

        if (Hardware->TXDirty->hwTxFlushVS)
        {
            if (!Hardware->features[gcvFEATURE_TEX_CACHE_FLUSH_FIX])
            {
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E02) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0E02, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


                /* Send FE-PE stall token. */
                *memory++ =
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

                *memory++ =
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)));

                /* Dump the stall. */
                gcmDUMP(gcvNULL, "#[stall 0x%08X 0x%08X]",
                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))),
                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))));
            }

            /* Flush the texture unit. */
            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0E03, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:1) - (0 ?
 1:1) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:1) - (0 ?
 1:1) + 1))))))) << (0 ?
 1:1))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:1) - (0 ?
 1:1) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:1) - (0 ?
 1:1) + 1))))))) << (0 ?
 1:1))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            Hardware->TXDirty->hwTxFlushVS = gcvFALSE;
        }

        if (Hardware->TXDirty->hwTxFlushL1Cache)
        {
            /* Flush the texture L1 cache through Gpipe or pixel shader*/
            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0E03, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:12) - (0 ?
 12:12) + 1))))))) << (0 ?
 12:12))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:12) - (0 ?
 12:12) + 1))))))) << (0 ?
 12:12))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:13) - (0 ?
 13:13) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:13) - (0 ?
 13:13) + 1))))))) << (0 ?
 13:13))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 13:13) - (0 ?
 13:13) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:13) - (0 ? 13:13) + 1))))))) << (0 ? 13:13))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            Hardware->TXDirty->hwTxFlushL1Cache = gcvFALSE;
        }

        if (supportGCREGTX)
        {
            for (dirty = 1, sampler = 0;
                 samplerDirty != 0;
                 sampler++, dirty <<= 1, samplerDirty >>=1)
            {
                if (!(samplerDirty & 0x1))
                {
                    continue;
                }

                if (unifiedSampler)
                {
                    if (vsSamplerMask && (sampler >= (gctUINT)Hardware->SHStates->programState.hints->unifiedStatus.samplerGPipeStart))
                    {
                        if (separateSteeringBit)
                        {
                            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x022D) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x022D, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                        }
                        else
                        {
                            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0218) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0218, ((((gctUINT32) (shaderConfigData)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 16:16) - (0 ?
 16:16) + 1))))))) << (0 ?
 16:16))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                        }
                        vsSamplerMask = 0;
                    }
                    else if (psSamplerMask && sampler < (gctUINT)GPipeBase)
                    {
                        if (separateSteeringBit)
                        {
                             {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x022D) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x022D, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                        }
                        else
                        {
                            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0218) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0218, ((((gctUINT32) (shaderConfigData)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 16:16) - (0 ?
 16:16) + 1))))))) << (0 ?
 16:16))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                        }
                        psSamplerMask = 0;
                    }
                }

                /* Check for dirty sampler mode registers. */
                if (dirty & Hardware->TXDirty->hwTxSamplerModeDirty)
                {
#if gcdENABLE_APPCTXT_BLITDRAW
                    if (Hardware->TXStates->hwTxSamplerMode[sampler] != gcvINVALID_VALUE)
#endif
                    {
                        /* Save dirty state in state buffer. */
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x4000 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x4000 + sampler, Hardware->TXStates->hwTxSamplerMode[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }
#if gcdENABLE_APPCTXT_BLITDRAW
                    if (Hardware->TXStates->hwTxSamplerModeEx[sampler] != gcvINVALID_VALUE)
#endif
                    {
                        /* Extra state to specify texture alignment. */
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x40E0 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x40E0 + sampler, Hardware->TXStates->hwTxSamplerModeEx[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }

                    if ((halti3Avail || texDecSupported)
#if gcdENABLE_APPCTXT_BLITDRAW
                        && (Hardware->TXStates->hwTxSamplerMode2[sampler] != gcvINVALID_VALUE)
#endif
                        )
                    {
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x41E0 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x41E0 + sampler, Hardware->TXStates->hwTxSamplerMode2[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }
                }

                /* Check for dirty sampler size registers. */
                if ((dirty & Hardware->TXDirty->hwTxSamplerSizeDirty)
#if gcdENABLE_APPCTXT_BLITDRAW
                    && (Hardware->TXStates->hwTxSamplerSize[sampler] != gcvINVALID_VALUE)
#endif
                    )
                {
                    /* Save dirty state in state buffer. */
                    {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x4020 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x4020 + sampler, Hardware->TXStates->hwTxSamplerSize[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                }

                /* Check for dirty sampler log2 size registers. */
                if ((dirty & Hardware->TXDirty->hwTxSamplerSizeLogDirty)
#if gcdENABLE_APPCTXT_BLITDRAW
                    && (Hardware->TXStates->hwTxSamplerSizeLog[sampler] != gcvINVALID_VALUE)
#endif
                    )
                {
                    /* Save dirty state in state buffer. */
                    {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x4040 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x4040 + sampler, Hardware->TXStates->hwTxSamplerSizeLog[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                }

                /* Check for dirty sampler 3D size registers. */
                if ((dirty & Hardware->TXDirty->hwTxSampler3DDirty)
#if gcdENABLE_APPCTXT_BLITDRAW
                    && (Hardware->TXStates->hwTxSampler3D[sampler] != gcvINVALID_VALUE)
#endif
                    )
                {
                    /* Save dirty state in state buffer. */
                    {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x40C0 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x40C0 + sampler, Hardware->TXStates->hwTxSampler3D[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                }

                /* Check for dirty sampler LOD registers. */
                if ((dirty & Hardware->TXDirty->hwTxSamplerLODDirty)
#if gcdENABLE_APPCTXT_BLITDRAW
                    && (Hardware->TXStates->hwTxSamplerLOD[sampler] != gcvINVALID_VALUE)
#endif
                    )
                {
                    {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x4060 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x4060 + sampler, Hardware->TXStates->hwTxSamplerLOD[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                }

                if (Hardware->features[gcvFEATURE_TEX_BASELOD])
                {
                    /* Check for dirty base LOD registers. */
                    if ((dirty & Hardware->TXDirty->hwTxSamplerBaseLODDirty)
#if gcdENABLE_APPCTXT_BLITDRAW
                        && (Hardware->TXStates->hwTxSamplerBaseLOD[sampler] != gcvINVALID_VALUE)
#endif
                        )
                    {
                        /* Save dirty state in state buffer. */
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x41C0 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x41C0 + sampler, Hardware->TXStates->hwTxSamplerBaseLOD[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }
                }

#if gcdSECURITY
                {
                    {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E02) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0E02, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


                    /* Send FE-PE stall token. */
                    *memory++ =
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

                    *memory++ =
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)));

                    /* Dump the stall. */
                    gcmDUMP(gcvNULL, "#[stall 0x%08X 0x%08X]",
                        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))),
                        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))));
                }
#endif

                /* Walk all LOD levels. */
                for (i = 0; i < gcdLOD_LEVELS; ++i)
                {
                    /* Check for dirty sampler address registers. */
                    if ((dirty & Hardware->TXDirty->hwTxSamplerAddressDirty[i])
#if gcdENABLE_APPCTXT_BLITDRAW
                        && (Hardware->TXStates->hwTxSamplerAddress[i][sampler] != gcvINVALID_VALUE)
#endif
                        )
                    {
                        gctUINT32 lodAddr = Hardware->TXStates->hwTxSamplerAddress[i][sampler];

#if gcdSECURITY
                        /* If the sampler or lod was not used by shader */
                        if (!(Hardware->SHStates->programState.hints->usedSamplerMask & (1 << sampler)) ||
                            i >= Hardware->TXStates->txLodNums[sampler])
                        {
                            lodAddr = gcvINVALID_ADDRESS;
                        }
#endif

                        /* Save dirty state in state buffer. */
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x4200 + (sampler << 4) + i) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x4200 + (sampler << 4) + i, lodAddr );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }
                }

                if (Hardware->features[gcvFEATURE_TEXTURE_LINEAR] ||
                    Hardware->features[gcvFEATURE_MSAA_TEXTURE])
                {
                    /* Check for dirty linear stride registers. */
                    if ((dirty & Hardware->TXDirty->hwTxSamplerLinearStrideDirty)
#if gcdENABLE_APPCTXT_BLITDRAW
                        && (Hardware->TXStates->hwTxSamplerLinearStride[sampler] != gcvINVALID_VALUE)
#endif
                        )
                    {
                        /* Save dirty state in state buffer. */
                        /* Program level 0 of the sampler. */
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x40A0+ sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x40A0+ sampler, Hardware->TXStates->hwTxSamplerLinearStride[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }
                }

                /* Check for dirty YUV-assembler control registers. */
                if ((dirty & Hardware->TXDirty->hwTxSamplerYUVControlDirty)
#if gcdENABLE_APPCTXT_BLITDRAW
                    && (Hardware->TXStates->hwTxSamplerYUVControl[sampler] != gcvINVALID_VALUE)
#endif
                    )
                {
                    /* Save dirty state in state buffer. */
                    {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x4100 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x4100 + sampler, Hardware->TXStates->hwTxSamplerYUVControl[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                }

                /* Check for dirty YUV stride registers. */
                if ((dirty & Hardware->TXDirty->hwTxSamplerYUVStrideDirty)
#if gcdENABLE_APPCTXT_BLITDRAW
                    && (Hardware->TXStates->hwTxSamplerYUVStride[sampler] != gcvINVALID_VALUE)
#endif
                    )
                {
                    /* Save dirty state in state buffer. */
                    {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x4120 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x4120 + sampler, Hardware->TXStates->hwTxSamplerYUVStride[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                }

                /* Walk LODs. */
                for (i = 0; i < (gcdLOD_LEVELS) / 4; ++i)
                {
                    if ((dirty & Hardware->TXDirty->hwTxSamplerASTCDirty[i])
#if gcdENABLE_APPCTXT_BLITDRAW
                        && ((Hardware->TXStates->hwTxSamplerASTCSize[i * 4][sampler] != gcvINVALID_VALUE)
                        || (Hardware->TXStates->hwTxSamplerASTCSRGB[i * 4][sampler] != gcvINVALID_VALUE))
#endif
                        )
                    {
                        index = i * 4;

                        if (i < 3)
                        {
                            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x4140 + i * 16 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x4140 + i * 16 + sampler, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSize[index + 0][sampler]) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSRGB[index + 0][sampler]) & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSize[index + 1][sampler]) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:12) - (0 ?
 12:12) + 1))))))) << (0 ?
 12:12))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSRGB[index + 1][sampler]) & ((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:12) - (0 ?
 12:12) + 1))))))) << (0 ?
 12:12))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:16) - (0 ?
 19:16) + 1))))))) << (0 ?
 19:16))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSize[index + 2][sampler]) & ((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:16) - (0 ?
 19:16) + 1))))))) << (0 ?
 19:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 20:20) - (0 ?
 20:20) + 1))))))) << (0 ?
 20:20))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSRGB[index + 2][sampler]) & ((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 20:20) - (0 ?
 20:20) + 1))))))) << (0 ?
 20:20))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 27:24) - (0 ?
 27:24) + 1))))))) << (0 ?
 27:24))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSize[index + 3][sampler]) & ((gctUINT32) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 27:24) - (0 ?
 27:24) + 1))))))) << (0 ?
 27:24))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 28:28) - (0 ?
 28:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 28:28) - (0 ?
 28:28) + 1))))))) << (0 ?
 28:28))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSRGB[index + 3][sampler]) & ((gctUINT32) ((((1 ?
 28:28) - (0 ?
 28:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 28:28) - (0 ? 28:28) + 1))))))) << (0 ? 28:28))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                        }
                        else
                        {
                            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x41A0 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x41A0 + sampler, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSize[12][sampler]) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSRGB[12][sampler]) & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSize[13][sampler]) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:12) - (0 ?
 12:12) + 1))))))) << (0 ?
 12:12))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSRGB[13][sampler]) & ((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                        }
                    }
                }

                if (Hardware->features[gcvFEATURE_TX_FRAC_PRECISION_6BIT])
                {
                    if ((dirty & Hardware->TXDirty->hwTxSamplerSizeLogExtDirty)
#if gcdENABLE_APPCTXT_BLITDRAW
                        && (Hardware->TXStates->hwTxSamplerSizeLogExt[sampler] != gcvINVALID_VALUE)
#endif
                        )
                    {
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x4400 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x4400 + sampler, Hardware->TXStates->hwTxSamplerSizeLogExt[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }

                    if ((dirty & Hardware->TXDirty->hwTxSampler3DExtDirty)
#if gcdENABLE_APPCTXT_BLITDRAW
                        && (Hardware->TXStates->hwTxSampler3DExt[sampler] != gcvINVALID_VALUE)
#endif
                        )
                    {
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x4420 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x4420 + sampler, Hardware->TXStates->hwTxSampler3DExt[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }

                    if ((dirty & Hardware->TXDirty->hwTxSamplerLodExtDirty)
#if gcdENABLE_APPCTXT_BLITDRAW
                        && (Hardware->TXStates->hwTxSamplerLodExt[sampler] != gcvINVALID_VALUE)
#endif
                        )
                    {
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x4440 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x4440 + sampler, Hardware->TXStates->hwTxSamplerLodExt[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }

                    if ((dirty & Hardware->TXDirty->hwTxSamplerLodBiasExtDirty)
#if gcdENABLE_APPCTXT_BLITDRAW
                        && (Hardware->TXStates->hwTxSamplerLodBiasExt[sampler] != gcvINVALID_VALUE)
#endif
                        )
                    {
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x4460 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x4460 + sampler, Hardware->TXStates->hwTxSamplerLodBiasExt[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }

                    if ((dirty & Hardware->TXDirty->hwTxSamplerAnisoCtrlDirty)
#if gcdENABLE_APPCTXT_BLITDRAW
                        && (Hardware->TXStates->hwTxSamplerAnisoCtrl[sampler] != gcvINVALID_VALUE)
#endif
                        )
                    {
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x44C0 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x44C0 + sampler, Hardware->TXStates->hwTxSamplerAnisoCtrl[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }
                }

                /* Check for dirty YUV stride registers. */
                if (dirty & Hardware->TXDirty->hwTxSamplerConfig3Dirty)
                {
#if gcdENABLE_APPCTXT_BLITDRAW
                    if (Hardware->TXStates->hwTxSamplerConfig3[sampler] != gcvINVALID_VALUE)
#endif
                    {
                        /* Save dirty state in state buffer. */
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x44A0 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x44A0 + sampler, Hardware->TXStates->hwTxSamplerConfig3[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }
#if gcdENABLE_APPCTXT_BLITDRAW
                    if (Hardware->TXStates->hwTxSamplerSliceSize[sampler] != gcvINVALID_VALUE)
#endif
                    {
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x4480 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x4480 + sampler, Hardware->TXStates->hwTxSamplerSliceSize[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }
                }
            }
        }
        else
        {
            for (dirty = 1, sampler = 0;
                 samplerDirty != 0;
                 sampler++, dirty <<= 1, samplerDirty >>=1)
            {
                if (!(samplerDirty & 0x1))
                {
                    continue;
                }

                gcmASSERT(!separateSteeringBit);

                if (unifiedSampler)
                {
                    if (vsSamplerMask && (sampler >= (gctUINT)GPipeBase))
                    {
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0218) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0218, ((((gctUINT32) (shaderConfigData)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 16:16) - (0 ?
 16:16) + 1))))))) << (0 ?
 16:16))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                        vsSamplerMask = 0;
                    }
                    else if (psSamplerMask && sampler < (gctUINT)GPipeBase)
                    {
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0218) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0218, ((((gctUINT32) (shaderConfigData)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 16:16) - (0 ?
 16:16) + 1))))))) << (0 ?
 16:16))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                        psSamplerMask = 0;
                    }
                }

                /* Check for dirty sampler mode registers. */
                if ((dirty & Hardware->TXDirty->hwTxSamplerModeDirty)
                    &&
                    (sampler < 16))
                {
#if gcdENABLE_APPCTXT_BLITDRAW
                    if (Hardware->TXStates->hwTxSamplerMode[sampler] != gcvINVALID_VALUE)
#endif
                    {
                        /* Save dirty state in state buffer. */
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0800 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0800 + sampler, Hardware->TXStates->hwTxSamplerMode[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }
#if gcdENABLE_APPCTXT_BLITDRAW
                    if (Hardware->TXStates->hwTxSamplerModeEx[sampler] != gcvINVALID_VALUE)
#endif
                    {
                        /* Extra state to specify texture alignment. */
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0870 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0870 + sampler, Hardware->TXStates->hwTxSamplerModeEx[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }
                }

                /* Check for dirty sampler size registers. */
                if ((dirty & Hardware->TXDirty->hwTxSamplerSizeDirty)
                    &&
                    (sampler < 16)
#if gcdENABLE_APPCTXT_BLITDRAW
                    && (Hardware->TXStates->hwTxSamplerSize[sampler] != gcvINVALID_VALUE)
#endif
                    )
                {
                    /* Save dirty state in state buffer. */
                    {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0810 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0810 + sampler, Hardware->TXStates->hwTxSamplerSize[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                }

                /* Check for dirty sampler log2 size registers. */
                if ((dirty & Hardware->TXDirty->hwTxSamplerSizeLogDirty)
                    &&
                    (sampler < 12)
#if gcdENABLE_APPCTXT_BLITDRAW
                    && (Hardware->TXStates->hwTxSamplerSizeLog[sampler] != gcvINVALID_VALUE)
#endif
                    )
                {
                    /* Save dirty state in state buffer. */
                    {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0820 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0820 + sampler, Hardware->TXStates->hwTxSamplerSizeLog[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                }

                /* Check for dirty sampler 3D size registers. */
                if ((dirty & Hardware->TXDirty->hwTxSampler3DDirty)
                    &&
                    (sampler < 16)
#if gcdENABLE_APPCTXT_BLITDRAW
                    && (Hardware->TXStates->hwTxSampler3D[sampler] != gcvINVALID_VALUE)
#endif
                    )
                {
                    /* Save dirty state in state buffer. */
                    {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0860 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0860 + sampler, Hardware->TXStates->hwTxSampler3D[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                }

                /* Check for dirty sampler LOD registers. */
                if ((dirty & Hardware->TXDirty->hwTxSamplerLODDirty)
                    &&
                    (sampler < 12)
#if gcdENABLE_APPCTXT_BLITDRAW
                    && (Hardware->TXStates->hwTxSamplerLOD[sampler] != gcvINVALID_VALUE)
#endif
                    )
                {
                    /* Save dirty state in state buffer. */
                    {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0830 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0830 + sampler, Hardware->TXStates->hwTxSamplerLOD[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                }

#if gcdSECURITY
                {
                    {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E02) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0E02, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


                    /* Send FE-PE stall token. */
                    *memory++ =
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

                    *memory++ =
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)));

                    /* Dump the stall. */
                    gcmDUMP(gcvNULL, "#[stall 0x%08X 0x%08X]",
                        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))),
                        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))));
                }
#endif

                /* Walk all LOD levels. */
                for (i = 0; i < gcdLOD_LEVELS; ++i)
                {
                    if (dirty & Hardware->TXDirty->hwTxSamplerAddressDirty[i]
#if gcdENABLE_APPCTXT_BLITDRAW
                    && (Hardware->TXStates->hwTxSamplerAddress[i][sampler] != gcvINVALID_VALUE)
#endif
                        )
                    {

                        gctUINT32 lodAddr = Hardware->TXStates->hwTxSamplerAddress[i][sampler];

#if gcdSECURITY
                        /* If the sampler or lod was not used by shader */
                        if (!(Hardware->SHStates->programState.hints->usedSamplerMask & (1 << sampler)) ||
                            i >= Hardware->TXStates->txLodNums[sampler])
                        {
                            lodAddr = gcvINVALID_ADDRESS;
                        }
#endif

                        /* Save dirty state in state buffer. */
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (address[i] + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, address[i] + sampler, lodAddr );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }
                }

                if (Hardware->features[gcvFEATURE_TEXTURE_LINEAR])
                {
                    /* Check for dirty linear stride registers. */
                    if ((dirty & Hardware->TXDirty->hwTxSamplerLinearStrideDirty)
#if gcdENABLE_APPCTXT_BLITDRAW
                        && (Hardware->TXStates->hwTxSamplerLinearStride[sampler] != gcvINVALID_VALUE)
#endif
                        )
                    {
                        /* Save dirty state in state buffer. */
                        /* Program level 0 of the sampler. */
                        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0B00 + (sampler * 16)) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0B00 + (sampler * 16), Hardware->TXStates->hwTxSamplerLinearStride[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }
                }

                /* Walk LODs. */
                for (i = 0; i < (gcdLOD_LEVELS + 3) / 4; ++i)
                {
                    if ((dirty & Hardware->TXDirty->hwTxSamplerASTCDirty[i])
#if gcdENABLE_APPCTXT_BLITDRAW
                        && ((Hardware->TXStates->hwTxSamplerASTCSize[i * 4][sampler] != gcvINVALID_VALUE)
                        ||  (Hardware->TXStates->hwTxSamplerASTCSRGB[i * 4][sampler] != gcvINVALID_VALUE))
#endif
                        )
                    {
                        index = i * 4;
                        if (i < 3)
                        {
                            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x08A0 + i * 16 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x08A0 + i * 16 + sampler, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSize[index + 0][sampler]) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSRGB[index + 0][sampler]) & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSize[index + 1][sampler]) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:12) - (0 ?
 12:12) + 1))))))) << (0 ?
 12:12))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSRGB[index + 1][sampler]) & ((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:12) - (0 ?
 12:12) + 1))))))) << (0 ?
 12:12))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:16) - (0 ?
 19:16) + 1))))))) << (0 ?
 19:16))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSize[index + 2][sampler]) & ((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:16) - (0 ?
 19:16) + 1))))))) << (0 ?
 19:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 20:20) - (0 ?
 20:20) + 1))))))) << (0 ?
 20:20))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSRGB[index + 2][sampler]) & ((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 20:20) - (0 ?
 20:20) + 1))))))) << (0 ?
 20:20))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 27:24) - (0 ?
 27:24) + 1))))))) << (0 ?
 27:24))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSize[index + 3][sampler]) & ((gctUINT32) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 27:24) - (0 ?
 27:24) + 1))))))) << (0 ?
 27:24))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 28:28) - (0 ?
 28:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 28:28) - (0 ?
 28:28) + 1))))))) << (0 ?
 28:28))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSRGB[index + 3][sampler]) & ((gctUINT32) ((((1 ?
 28:28) - (0 ?
 28:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 28:28) - (0 ? 28:28) + 1))))))) << (0 ? 28:28))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                        }
                        else
                        {
                            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x08D0 + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x08D0 + sampler, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSize[12][sampler]) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSRGB[12][sampler]) & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSize[13][sampler]) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:12) - (0 ?
 12:12) + 1))))))) << (0 ?
 12:12))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxSamplerASTCSRGB[13][sampler]) & ((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                        }
                    }
                }
            }
        }


        for (samplerTSIndex = 0;
             samplerTSDirty;
             samplerTSIndex++, samplerTSDirty >>=1)
        {
            if (!(samplerTSDirty & 0x1))
            {
                continue;
            }

            /* Save dirty state in state buffer. */
            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05C8 + samplerTSIndex) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x05C8 + samplerTSIndex, Hardware->MCStates->hwTXSampleTSConfig[samplerTSIndex] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05D0 + samplerTSIndex) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x05D0 + samplerTSIndex, Hardware->MCStates->hwTXSampleTSBuffer[samplerTSIndex] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05D8 + samplerTSIndex) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x05D8 + samplerTSIndex, Hardware->MCStates->hwTXSampleTSClearValue[samplerTSIndex] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            if (halti2Avail)
            {
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05E0 + samplerTSIndex) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x05E0 + samplerTSIndex, Hardware->MCStates->hwTXSampleTSClearValueUpper[samplerTSIndex] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            }

            if (texTSMapping)
            {
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x06A0 + samplerTSIndex) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x06A0 + samplerTSIndex, Hardware->MCStates->hwTxSamplerTxBaseBuffer[samplerTSIndex] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            }
        }
        /* Reset dirty flags. */
        Hardware->TXDirty->hwTxSamplerModeDirty = 0;
        Hardware->TXDirty->hwTxSamplerSizeDirty = 0;
        Hardware->TXDirty->hwTxSamplerSizeLogDirty = 0;
        Hardware->TXDirty->hwTxSampler3DDirty = 0;
        Hardware->TXDirty->hwTxSamplerLODDirty = 0;
        Hardware->TXDirty->hwTxSamplerBaseLODDirty = 0;
        Hardware->TXDirty->hwTxSamplerYUVControlDirty = 0;
        Hardware->TXDirty->hwTxSamplerYUVStrideDirty = 0;
        Hardware->TXDirty->hwTxSamplerLinearStrideDirty = 0;
        Hardware->TXDirty->hwTxSamplerSizeLogExtDirty = 0;
        Hardware->TXDirty->hwTxSampler3DExtDirty = 0;
        Hardware->TXDirty->hwTxSamplerLodExtDirty = 0;
        Hardware->TXDirty->hwTxSamplerLodBiasExtDirty = 0;
        Hardware->TXDirty->hwTxSamplerAnisoCtrlDirty = 0;
        Hardware->TXDirty->hwTxSamplerConfig3Dirty = 0;

        for (i = 0; i < gcdLOD_LEVELS; ++i)
        {
            Hardware->TXDirty->hwTxSamplerAddressDirty[i] = 0;
            Hardware->TXDirty->hwTxSamplerASTCDirty[i] = 0;
        }

        /* Check for texture cache flush. */
        if (Hardware->TXDirty->hwTxFlushPS)
        {
            /* Flush the texture unit. */
            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0E03, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 2:2) - (0 ?
 2:2) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:2) - (0 ?
 2:2) + 1))))))) << (0 ?
 2:2))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 2:2) - (0 ?
 2:2) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:2) - (0 ?
 2:2) + 1))))))) << (0 ?
 2:2))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:1) - (0 ?
 1:1) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:1) - (0 ?
 1:1) + 1))))))) << (0 ?
 1:1))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:1) - (0 ?
 1:1) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:1) - (0 ?
 1:1) + 1))))))) << (0 ?
 1:1))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            if (Hardware->config->chipModel == gcv700
                || Hardware->config->gpuCoreCount > 1
                )
            {
                /* Flush the L2 cache as well. */
                Hardware->flushL2 = gcvTRUE;
            }

            /* Flushed the texture cache. */
            Hardware->TXDirty->hwTxFlushPS = gcvFALSE;
        }

        if (semaStall)
        {
            gcmASSERT(!Hardware->features[gcvFEATURE_BLT_ENGINE]
                      || Hardware->features[gcvFEATURE_PE_TILE_CACHE_FLUSH_FIX]);

            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0594) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0594, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E02) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0E02, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            /* Send FE-PE stall token. */
            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)));

            /* Dump the stall. */
            gcmDUMP(gcvNULL, "#[stall 0x%08X 0x%08X]",
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))),
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))));
        }

        gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, Memory);
    }

    /* Mark texture as updated. */
    Hardware->TXDirty->textureDirty = gcvFALSE;
    Hardware->TXDirty->hwTxSamplerDirty = 0;
    Hardware->MCDirty->hwTxSamplerTSDirty = 0;


OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_UpdateTextureDesc(
    gcoHARDWARE Hardware,
    gcsTEXTURE_PTR TexParam,
    gcsTXDESC_UPDATE_INFO_PTR UpdateInfo
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsTEXTUREDESCRIPTORREGS * pDesc = (gcsTEXTUREDESCRIPTORREGS *)UpdateInfo->desc;
    gctINT baseLevel = gcmMAX(0, TexParam->baseLevel);
    gctINT maxLevel = gcmMIN(TexParam->maxLevel, UpdateInfo->levels - 1);
    gctBOOL isTexArray;
    gctINT type;
    gcoSURF baseSurf = UpdateInfo->baseLevelSurf;
    gctINT format;
    gctBOOL needYUVAssembler = gcvFALSE;
    gctBOOL integerFormat;
    gctUINT signExtFix = 0x00030000;
    gctUINT32 yOffset = 0, uOffset = 0, vOffset = 0;
    gctUINT32 logWidth, logHeight, logDepth;
    gctBOOL swapUV;
    gctUINT32 addressing;
    gctUINT32 hAlignment;
    gctUINT endian;
    gctBOOL msaaTexture;
    gctBOOL astcTexture;
    gctINT hAlignmentCfg;
    static const gctINT32 alignXlate[] =
    {
        /* gcvSURF_FOUR */
        0x0,
        /* gcvSURF_SIXTEEN */
        0x1,
        /* gcvSURF_SUPER_TILED */
        0x2,
        /* gcvSURF_SPLIT_TILED */
        0x3,
        /* gcvSURF_SPLIT_SUPER_TILED */
        0x4,
    };

    gcmHEADER_ARG("Hardware=0x%x TexParam=0x%x UpdateInfo=0x%x", Hardware, TexParam, UpdateInfo);

    gcmGETHARDWARE(Hardware);

    if (!Hardware->features[gcvFEATURE_TX_DESCRIPTOR])
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    switch (baseSurf->format)
    {
        case gcvSURF_YV12:
        case gcvSURF_I420:
        case gcvSURF_NV12:
        case gcvSURF_NV21:
        case gcvSURF_NV16:
        case gcvSURF_NV61:
        case gcvSURF_YUV420_10_ST:
        case gcvSURF_YUV420_TILE_ST:
        case gcvSURF_YUV420_TILE_10_ST:
            /* Need YUV assembler. */
            needYUVAssembler = gcvTRUE;
        default:
            break;
    }

    if ((baseSurf->format == gcvSURF_YUV420_TILE_ST) ||
        (baseSurf->format == gcvSURF_YUV420_TILE_10_ST))
    {
        /* Special format requires tiled addressing. */
        addressing = 0x0;
    }
    else if (baseSurf->tiling == gcvLINEAR)
    {
        addressing = 0x3;
    }
    else
    {
        addressing = 0x0;
    }

    astcTexture = (baseSurf->formatInfo.fmtClass == gcvFORMAT_CLASS_ASTC);
    /* Zero the descriptor */
    gcoOS_ZeroMemory((gctPOINTER)pDesc, gcmSIZEOF(gcsTEXTUREDESCRIPTORREGS));

    gcoOS_MemCopy((gctPOINTER)pDesc->gcregTXAddress, UpdateInfo->lodAddr, 16 * gcmSIZEOF(gctUINT32));

    type = _GetSamplerType(UpdateInfo->type, &isTexArray);

    format = _GetTextureFormat(Hardware,
                               &baseSurf->formatInfo,
                               TexParam,
                               needYUVAssembler,
                               UpdateInfo->unsizedDepthTexture,
                               baseSurf->cacheMode,
                               &integerFormat,
                               &signExtFix);

    endian = _GetEndianSetting(Hardware, UpdateInfo->endianHint);

    /* Swap U,V for YVYU and VYUY. */
    swapUV = baseSurf->format == gcvSURF_YVYU
          || baseSurf->format == gcvSURF_VYUY;

    /* Update horizontal alignment to the surface's alignment. */
    switch (baseSurf->tiling)
    {
    case gcvLINEAR:
        hAlignment = gcvSURF_SIXTEEN;
        break;

    case gcvTILED:
        hAlignment = baseSurf->hAlignment;
        break;

    case gcvSUPERTILED:
        hAlignment = gcvSURF_SUPER_TILED;
        break;

    case gcvMULTI_TILED:
        hAlignment = gcvSURF_SPLIT_TILED;
        break;

    case gcvMULTI_SUPERTILED:
        hAlignment = gcvSURF_SPLIT_SUPER_TILED;
        break;

    default:
        hAlignment = gcvSURF_SUPER_TILED;
        gcmASSERT(gcvFALSE);
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        break;
    }

    msaaTexture = baseSurf->isMsaa;

    logWidth  = _Log2inXdot8(UpdateInfo->baseLevelWidth);
    logHeight = _Log2inXdot8(UpdateInfo->baseLevelHeight);
    logDepth  = _Log2inXdot8(UpdateInfo->baseLevelDepth);

    if ((UpdateInfo->baseLevelWidth * UpdateInfo->baseLevelHeight) < 1)
    {
        type = 0x0;
    }

    pDesc->gcregTXConfig =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:0) - (0 ?
 2:0) + 1))))))) << (0 ?
 2:0))) | (((gctUINT32) ((gctUINT32) (type) & ((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 17:13) - (0 ?
 17:13) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 17:13) - (0 ?
 17:13) + 1))))))) << (0 ?
 17:13))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ?
 17:13) - (0 ?
 17:13) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 17:13) - (0 ? 17:13) + 1))))))) << (0 ? 17:13)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 21:20) - (0 ?
 21:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 21:20) - (0 ?
 21:20) + 1))))))) << (0 ?
 21:20))) | (((gctUINT32) ((gctUINT32) (addressing) & ((gctUINT32) ((((1 ?
 21:20) - (0 ?
 21:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 23:22) - (0 ?
 23:22) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:22) - (0 ?
 23:22) + 1))))))) << (0 ?
 23:22))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ?
 23:22) - (0 ?
 23:22) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 23:22) - (0 ? 23:22) + 1))))))) << (0 ? 23:22)));

    pDesc->gcregTXSize =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:0) - (0 ?
 14:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:0) - (0 ?
 14:0) + 1))))))) << (0 ?
 14:0))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->baseLevelWidth) & ((gctUINT32) ((((1 ?
 14:0) - (0 ?
 14:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:0) - (0 ? 14:0) + 1))))))) << (0 ? 14:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 30:16) - (0 ?
 30:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 30:16) - (0 ?
 30:16) + 1))))))) << (0 ?
 30:16))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->baseLevelHeight) & ((gctUINT32) ((((1 ?
 30:16) - (0 ?
 30:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 30:16) - (0 ? 30:16) + 1))))))) << (0 ? 30:16)));

    pDesc->gcregTX3D = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:0) - (0 ?
 13:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:0) - (0 ?
 13:0) + 1))))))) << (0 ?
 13:0))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->baseLevelDepth) & ((gctUINT32) ((((1 ?
 13:0) - (0 ?
 13:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0)));

    pDesc->gcregTXLinearStride =
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 17:0) - (0 ?
 17:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 17:0) - (0 ?
 17:0) + 1))))))) << (0 ?
 17:0))) | (((gctUINT32) ((gctUINT32) (baseSurf->stride) & ((gctUINT32) ((((1 ?
 17:0) - (0 ?
 17:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 17:0) - (0 ? 17:0) + 1))))))) << (0 ? 17:0)));

    hAlignmentCfg = alignXlate[hAlignment];

    if (Hardware->features[gcvFEATURE_128BTILE])
    {
        if (hAlignment == gcvSURF_SUPER_TILED &&
            baseSurf->tiling == gcvYMAJOR_SUPERTILED)
        {
            hAlignmentCfg = 0x7;
        }
    }

    pDesc->gcregTXExtConfig =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:0) - (0 ?
 5:0) + 1))))))) << (0 ?
 5:0))) | (((gctUINT32) ((gctUINT32) ((format >> 12)) & ((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:8) - (0 ?
 10:8) + 1))))))) << (0 ?
 10:8))) | (((gctUINT32) ((gctUINT32) ((format >> 20)) & ((gctUINT32) ((((1 ?
 10:8) - (0 ?
 10:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:12) - (0 ?
 14:12) + 1))))))) << (0 ?
 14:12))) | (((gctUINT32) ((gctUINT32) ((format >> 23)) & ((gctUINT32) ((((1 ?
 14:12) - (0 ?
 14:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 18:16) - (0 ?
 18:16) + 1))))))) << (0 ?
 18:16))) | (((gctUINT32) ((gctUINT32) ((format >> 26)) & ((gctUINT32) ((((1 ?
 18:16) - (0 ?
 18:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 22:20) - (0 ?
 22:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 22:20) - (0 ?
 22:20) + 1))))))) << (0 ?
 22:20))) | (((gctUINT32) ((gctUINT32) ((format >> 29)) & ((gctUINT32) ((((1 ?
 22:20) - (0 ?
 22:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 22:20) - (0 ? 22:20) + 1))))))) << (0 ? 22:20)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:15) - (0 ?
 15:15) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:15) - (0 ?
 15:15) + 1))))))) << (0 ?
 15:15))) | (((gctUINT32) ((gctUINT32) (swapUV) & ((gctUINT32) ((((1 ?
 15:15) - (0 ?
 15:15) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:15) - (0 ? 15:15) + 1))))))) << (0 ? 15:15)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 24:24) - (0 ?
 24:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 24:24) - (0 ?
 24:24) + 1))))))) << (0 ?
 24:24))) | (((gctUINT32) ((gctUINT32) (isTexArray) & ((gctUINT32) ((((1 ?
 24:24) - (0 ?
 24:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 24:24) - (0 ? 24:24) + 1))))))) << (0 ? 24:24)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 28:26) - (0 ?
 28:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 28:26) - (0 ?
 28:26) + 1))))))) << (0 ?
 28:26))) | (((gctUINT32) ((gctUINT32) (hAlignmentCfg) & ((gctUINT32) ((((1 ?
 28:26) - (0 ?
 28:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 28:26) - (0 ? 28:26) + 1))))))) << (0 ? 28:26)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:25) - (0 ?
 25:25) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:25) - (0 ?
 25:25) + 1))))))) << (0 ?
 25:25))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 25:25) - (0 ?
 25:25) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 29:29) - (0 ?
 29:29) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 29:29) - (0 ?
 29:29) + 1))))))) << (0 ?
 29:29))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 29:29) - (0 ?
 29:29) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 29:29) - (0 ? 29:29) + 1))))))) << (0 ? 29:29)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:31) - (0 ?
 31:31) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:31) - (0 ?
 31:31) + 1))))))) << (0 ?
 31:31))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 31:31) - (0 ?
 31:31) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31)))
        ;


    pDesc->gcregTXBaseLOD =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (baseLevel) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (maxLevel) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)));

    pDesc->gcregTXConfig2 = signExtFix;

    pDesc->gcregTXConfig3 =
        ((((gctUINT32) (0x00000000)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:3) - (0 ?
 3:3) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:3) - (0 ?
 3:3) + 1))))))) << (0 ?
 3:3))) | (((gctUINT32) ((gctUINT32) (msaaTexture) & ((gctUINT32) ((((1 ?
 3:3) - (0 ?
 3:3) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)));

    pDesc->gcregTXSizeExt =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (logWidth) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (logHeight) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)));

    pDesc->gcregTXVolumeExt =
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (logDepth) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    pDesc->gcregTXSlice = baseSurf->sliceSize;

    pDesc->gcregTXLogSize =
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 28:27) - (0 ?
 28:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 28:27) - (0 ?
 28:27) + 1))))))) << (0 ?
 28:27))) | (((gctUINT32) ((gctUINT32) (astcTexture ?
 2 : 0) & ((gctUINT32) ((((1 ?
 28:27) - (0 ?
 28:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 28:27) - (0 ? 28:27) + 1))))))) << (0 ? 28:27)));

    /* YUV-assembler can only use LOD 0.
     * Y plane address is from LOD 0,
     * U plane address is from LOD 1,
     * V plane address is from LOD 2,
     * Set stride from lodStride.
     */
    pDesc->gcregTXControlYUVEx = needYUVAssembler ?
                                 _GetYUVControl(baseSurf->format, &yOffset, &uOffset, &vOffset) : 0;

    pDesc->gcregTXStrideYUVEx  =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (baseSurf->uStride) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (baseSurf->vStride) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)));

    if (needYUVAssembler)
    {
        if (baseSurf->flags & gcvSURF_FLAG_MULTI_NODE)
        {
            /* Get u,v channel addresses from other nodes for multi-node surface. */
            gcmGETHARDWAREADDRESS(baseSurf->node2, pDesc->gcregTXAddress[1]);
            gcmGETHARDWAREADDRESS(baseSurf->node3, pDesc->gcregTXAddress[2]);
        }
        else
        {
            pDesc->gcregTXAddress[1] = pDesc->gcregTXAddress[0] + baseSurf->uOffset;
            pDesc->gcregTXAddress[2] = pDesc->gcregTXAddress[0] + baseSurf->vOffset;
        }

        pDesc->gcregTXAddress[0] += yOffset;
        pDesc->gcregTXAddress[1] += uOffset;
        pDesc->gcregTXAddress[2] += vOffset;
    }

    if (astcTexture)
    {
        pDesc->gcregTXASTC0Ex =
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSize[0]) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSRGB[0]) & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSize[1]) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:12) - (0 ?
 12:12) + 1))))))) << (0 ?
 12:12))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSRGB[1]) & ((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:16) - (0 ?
 19:16) + 1))))))) << (0 ?
 19:16))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSize[2]) & ((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 20:20) - (0 ?
 20:20) + 1))))))) << (0 ?
 20:20))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSRGB[2]) & ((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 27:24) - (0 ?
 27:24) + 1))))))) << (0 ?
 27:24))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSize[3]) & ((gctUINT32) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 28:28) - (0 ?
 28:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 28:28) - (0 ?
 28:28) + 1))))))) << (0 ?
 28:28))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSRGB[3]) & ((gctUINT32) ((((1 ?
 28:28) - (0 ?
 28:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 28:28) - (0 ? 28:28) + 1))))))) << (0 ? 28:28)));


        pDesc->gcregTXASTC1Ex =
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSize[4]) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSRGB[4]) & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSize[5]) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:12) - (0 ?
 12:12) + 1))))))) << (0 ?
 12:12))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSRGB[5]) & ((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:16) - (0 ?
 19:16) + 1))))))) << (0 ?
 19:16))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSize[6]) & ((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 20:20) - (0 ?
 20:20) + 1))))))) << (0 ?
 20:20))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSRGB[6]) & ((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 27:24) - (0 ?
 27:24) + 1))))))) << (0 ?
 27:24))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSize[7]) & ((gctUINT32) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 28:28) - (0 ?
 28:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 28:28) - (0 ?
 28:28) + 1))))))) << (0 ?
 28:28))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSRGB[7]) & ((gctUINT32) ((((1 ?
 28:28) - (0 ?
 28:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 28:28) - (0 ? 28:28) + 1))))))) << (0 ? 28:28)));

        pDesc->gcregTXASTC2Ex =
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSize[8]) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSRGB[8]) & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSize[9]) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:12) - (0 ?
 12:12) + 1))))))) << (0 ?
 12:12))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSRGB[9]) & ((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:16) - (0 ?
 19:16) + 1))))))) << (0 ?
 19:16))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSize[10]) & ((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 20:20) - (0 ?
 20:20) + 1))))))) << (0 ?
 20:20))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSRGB[10]) & ((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 27:24) - (0 ?
 27:24) + 1))))))) << (0 ?
 27:24))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSize[11]) & ((gctUINT32) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 28:28) - (0 ?
 28:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 28:28) - (0 ?
 28:28) + 1))))))) << (0 ?
 28:28))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSRGB[11]) & ((gctUINT32) ((((1 ?
 28:28) - (0 ?
 28:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 28:28) - (0 ? 28:28) + 1))))))) << (0 ? 28:28)));


        pDesc->gcregTXASTC3Ex =
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSize[12]) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSRGB[12]) & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSize[13]) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:12) - (0 ?
 12:12) + 1))))))) << (0 ?
 12:12))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSRGB[13]) & ((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:16) - (0 ?
 19:16) + 1))))))) << (0 ?
 19:16))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSize[14]) & ((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 20:20) - (0 ?
 20:20) + 1))))))) << (0 ?
 20:20))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSRGB[14]) & ((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 27:24) - (0 ?
 27:24) + 1))))))) << (0 ?
 27:24))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSize[15]) & ((gctUINT32) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 28:28) - (0 ?
 28:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 28:28) - (0 ?
 28:28) + 1))))))) << (0 ?
 28:28))) | (((gctUINT32) ((gctUINT32) (UpdateInfo->astcSRGB[15]) & ((gctUINT32) ((((1 ?
 28:28) - (0 ?
 28:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 28:28) - (0 ? 28:28) + 1))))))) << (0 ? 28:28)));
    }

    /* BC */
    _GetBorderColor(Hardware, TexParam, &baseSurf->formatInfo, UpdateInfo->borderColor32, pDesc);

#if gcdENDIAN_BIG
    {
        gctSIZE_T i;
        gctUINT32_PTR pUI = (gctUINT32_PTR)pDesc;

        for (i = 0; i < sizeof(gcsTEXTUREDESCRIPTORREGS) / sizeof(gctUINT32); ++i)
        {
            pUI[i] = gcmBSWAP32(pUI[i]);
        }
    }
#endif

    gcmDUMP_BUFFER(gcvNULL, gcvDUMP_BUFFER_MEMORY, UpdateInfo->physical, pDesc, 0, 256);

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_BindTextureDesc(
    IN gcoHARDWARE Hardware,
    IN gctINT Sampler,
    IN gcsSAMPLER_PTR SamplerInfo
    )
{
    gceSTATUS status;
    gctBOOL   type;
    gctFIXED_POINT bias = 0, lodmin = 0, lodmax = 0;
    gctUINT32 anisoLog;
    gceTEXTURE_FILTER minFilter;
    gceTEXTURE_FILTER mipFilter;
    gceTEXTURE_FILTER magFilter;
    gctINT roundUV;
    gctBOOL integerFormat = gcvFALSE;
    gctINT tsSlotIndex;
    gctUINT32 samplerCtrl0, samplerCtrl1, samplerLODMinMax, samplerLODBias, samplerAnisCtrl;
    gctBOOL sRGB;
    gcsTEXTURE_PTR textureInfo = SamplerInfo->textureInfo;
    gctUINT32 baseLevelWidth, baseLevelHeight;
    gctUINT32 txDescAddress;
    gcsBITMASK_PTR maskArray[10] = {gcvNULL};
    gctUINT32 count = 0;
    gctBOOL smallBatch = gcvFALSE;

#if gcdDEBUG_OPTION && gcdDEBUG_OPTION_NONE_TEXTURE
    gcePATCH_ID patchId = gcvPATCH_INVALID;
#endif

    static const gctINT32 addressXlate[] =
    {
        /* gcvTEXTURE_INVALID */
        -1,
        /* gcvTEXTURE_CLAMP */
        0x2,
        /* gcvTEXTURE_WRAP */
        0x0,
        /* gcvTEXTURE_MIRROR */
        0x1,
        /* gcvTEXTURE_BORDER */
        0x3,
        /* gcvTEXTURE_MIRROR_ONCE */
        -1,
    };

    static const gctINT32 magXlate[] =
    {
        /* gcvTEXTURE_NONE */
        0x0,
        /* gcvTEXTURE_POINT */
        0x1,
        /* gcvTEXTURE_LINEAR */
        0x2,
        /* gcvTEXTURE_ANISOTROPIC */
        0x3,
    };

    static const gctINT32 minXlate[] =
    {
        /* gcvTEXTURE_NONE */
        0x0,
        /* gcvTEXTURE_POINT */
        0x1,
        /* gcvTEXTURE_LINEAR */
        0x2,
        /* gcvTEXTURE_ANISOTROPIC */
        0x3,
    };

    static const gctINT32 mipXlate[] =
    {
        /* gcvTEXTURE_NONE */
        0x0,
        /* gcvTEXTURE_POINT */
        0x1,
        /* gcvTEXTURE_LINEAR */
        0x2,
        /* gcvTEXTURE_ANISOTROPIC */
        0x3,
    };

    static const gctINT32 modeXlate[] =
    {
        -1, /* gcvTEXTURE_COMPARE_MODE_INVALID */
        0x0, /* gcvTEXTURE_COMPARE_MODE_NONE */
        0x1, /* gcvTEXTURE_COMPARE_MODE_REF */
    };

    static const gctINT32 funcXlate[] =
    {
        -1, /* gcvCOMPARE_INVALID */
        0x7, /* gcvCOMPARE_NEVER */
        0x5, /* gcvCOMPARE_NOT_EQUAL */
        0x2, /* gcvCOMPARE_LESS */
        0x0, /* gcvCOMPARE_LESS_OR_EQUAL */
        0x4, /* gcvCOMPARE_EQUAL */
        0x3, /* gcvCOMPARE_GREATER */
        0x1, /* gcvCOMPARE_GREATER_OR_EQUAL */
        0x6, /* gcvCOMPARE_ALWAYS */
    };

    gcmHEADER_ARG("Hardware=0x%x Sampler=%d SamplerInfo=0x%x",
                  Hardware, Sampler, SamplerInfo);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    gcmDEBUG_VERIFY_ARGUMENT((Sampler >= 0) && (Sampler < gcdTXDESCRIPTORS));

    minFilter     = textureInfo->minFilter;
    mipFilter     = textureInfo->mipFilter;
    magFilter     = textureInfo->magFilter;

    baseLevelWidth  = SamplerInfo->baseLevelSurf->allocedW;
    baseLevelHeight = SamplerInfo->baseLevelSurf->allocedH;

    smallBatch = Hardware->features[gcvFEATURE_SMALL_BATCH] && Hardware->options.smallBatch;

    if (!SamplerInfo->filterable)
    {
        if ((minFilter == gcvTEXTURE_LINEAR) || (minFilter == gcvTEXTURE_ANISOTROPIC))
        {
            minFilter = gcvTEXTURE_POINT;
        }

        if ((magFilter == gcvTEXTURE_LINEAR) || (magFilter == gcvTEXTURE_ANISOTROPIC))
        {
            magFilter = gcvTEXTURE_POINT;
        }

        if ((mipFilter == gcvTEXTURE_LINEAR) || (mipFilter == gcvTEXTURE_ANISOTROPIC))
        {
            mipFilter = gcvTEXTURE_POINT;
        }
    }
    if (mipFilter == gcvTEXTURE_LINEAR &&
        SamplerInfo->formatInfo->fmtClass == gcvFORMAT_CLASS_DEPTH)
    {
        mipFilter = gcvTEXTURE_POINT;
    }

#if gcdDEBUG_OPTION && gcdDEBUG_OPTION_NONE_TEXTURE
    gcoHAL_GetPatchID(gcvNULL, &patchId);

    if (patchId == gcvPATCH_DEBUG)
    {
        type = 0x0;
    }
    else
#endif

    if (Hardware->specialHint & gcvHINT_BIT_0)
    {
        if ((mipFilter == gcvTEXTURE_LINEAR) &&
            (--Hardware->specialHintData < 0))
        {
#if gcdFORCE_BILINEAR
            mipFilter = gcvTEXTURE_POINT;
#endif
        }
    }

    if (Hardware->features[gcvFEATURE_TX_8bit_UVFrac])
    {
        roundUV = 0;
    }
    else if (Hardware->features[gcvFEATURE_TX_FRAC_PRECISION_6BIT])
    {
        roundUV = 1;
    }
    else
    {
        if (Hardware->currentApi == gcvAPI_OPENVG)
        {
            roundUV = 0;
        }
        else if (Hardware->patchID == gcvPATCH_CTGL20 || Hardware->patchID == gcvPATCH_CTGL11)
        {
            roundUV = (textureInfo->magFilter == gcvTEXTURE_POINT &&
                       baseLevelWidth <= 32 && baseLevelHeight <= 32 ) ? 0 : 1;
        }
        else if (Hardware->patchID == gcvPATCH_NETFLIX)
        {
            roundUV = 1;
        }
        else
        {
            roundUV = (textureInfo->minFilter == gcvTEXTURE_POINT ||
                       textureInfo->magFilter == gcvTEXTURE_POINT) ? 0 : 1;
        }
    }

    type = _GetSamplerType(SamplerInfo->texType, gcvNULL);

    anisoLog = (textureInfo->anisoFilter == 1) ?
               0 : _Log2inXdot8(textureInfo->anisoFilter);

    if (0x5 == type
    ||  0x3 == type)
    {
        anisoLog = 0;
    }

    /* Compute 5.8 format values */
    bias   = _Float2SignedFixed(textureInfo->lodBias, 8, 8);
    lodmax = _Float2SignedFixed(textureInfo->lodMax, 5, 8);
    lodmin = _Float2SignedFixed(textureInfo->lodMin, 5, 8);

    sRGB = (textureInfo->sRGB != gcvTEXTURE_SKIP_DECODE) && ((SamplerInfo->formatInfo->txFormat >> 18) & 1);

    if  ((SamplerInfo->formatInfo->format == gcvSURF_YUV420_10_ST) ||
         (SamplerInfo->formatInfo->format == gcvSURF_YUV420_TILE_10_ST))
    {
        integerFormat = gcvFALSE;
    }
    else if (SamplerInfo->formatInfo->fmtClass == gcvFORMAT_CLASS_YUV)
    {
        integerFormat = gcvTRUE;
    }
    else
    {
        integerFormat = SamplerInfo->formatInfo->txIntFilter;
    }

    /* Can integer filter be used? */
    integerFormat = integerFormat
                    && (type != 0x3)
                    && (!sRGB)
                    && (anisoLog == 0);
    if (SamplerInfo->hasTileStatus
        && (baseLevelWidth >= 1024 && baseLevelHeight >= 1024)
        && (Hardware->specialHintData < 0)
        && (Hardware->patchID == gcvPATCH_GLBM25 ||
            Hardware->patchID == gcvPATCH_GLBM27 ||
            Hardware->patchID == gcvPATCH_GFXBENCH)
        )
    {
        minFilter = magFilter = gcvTEXTURE_POINT;
    }

    gcmGETHARDWAREADDRESSP(SamplerInfo->descNode, txDescAddress);

    samplerCtrl0 =
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:0) - (0 ?
 2:0) + 1))))))) << (0 ?
 2:0))) | (((gctUINT32) ((gctUINT32) (addressXlate[textureInfo->s]) & ((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:3) - (0 ?
 5:3) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:3) - (0 ?
 5:3) + 1))))))) << (0 ?
 5:3))) | (((gctUINT32) ((gctUINT32) (addressXlate[textureInfo->t]) & ((gctUINT32) ((((1 ?
 5:3) - (0 ?
 5:3) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))
      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:6) - (0 ?
 8:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:6) - (0 ?
 8:6) + 1))))))) << (0 ?
 8:6))) | (((gctUINT32) ((gctUINT32) (addressXlate[textureInfo->r]) & ((gctUINT32) ((((1 ?
 8:6) - (0 ?
 8:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:6) - (0 ? 8:6) + 1))))))) << (0 ? 8:6)))
      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:9) - (0 ?
 10:9) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:9) - (0 ?
 10:9) + 1))))))) << (0 ?
 10:9))) | (((gctUINT32) ((gctUINT32) (minXlate[minFilter]) & ((gctUINT32) ((((1 ?
 10:9) - (0 ?
 10:9) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:9) - (0 ? 10:9) + 1))))))) << (0 ? 10:9)))
      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:11) - (0 ?
 12:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:11) - (0 ?
 12:11) + 1))))))) << (0 ?
 12:11))) | (((gctUINT32) ((gctUINT32) (mipXlate[mipFilter]) & ((gctUINT32) ((((1 ?
 12:11) - (0 ?
 12:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:11) - (0 ? 12:11) + 1))))))) << (0 ? 12:11)))
      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:13) - (0 ?
 14:13) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:13) - (0 ?
 14:13) + 1))))))) << (0 ?
 14:13))) | (((gctUINT32) ((gctUINT32) (magXlate[magFilter]) & ((gctUINT32) ((((1 ?
 14:13) - (0 ?
 14:13) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:13) - (0 ? 14:13) + 1))))))) << (0 ? 14:13)))
      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:15) - (0 ?
 15:15) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:15) - (0 ?
 15:15) + 1))))))) << (0 ?
 15:15))) | (((gctUINT32) ((gctUINT32) (roundUV) & ((gctUINT32) ((((1 ?
 15:15) - (0 ?
 15:15) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:15) - (0 ? 15:15) + 1))))))) << (0 ? 15:15)))
      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 16:16) - (0 ?
 16:16) + 1))))))) << (0 ?
 16:16))) | (((gctUINT32) ((gctUINT32) ((textureInfo->dsMode == gcvTEXTURE_DS_MODE_STENCIL)) & ((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)))
      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 17:17) - (0 ?
 17:17) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 17:17) - (0 ?
 17:17) + 1))))))) << (0 ?
 17:17))) | (((gctUINT32) ((gctUINT32) (modeXlate[textureInfo->compareMode]) & ((gctUINT32) ((((1 ?
 17:17) - (0 ?
 17:17) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 17:17) - (0 ? 17:17) + 1))))))) << (0 ? 17:17)))
      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 20:18) - (0 ?
 20:18) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 20:18) - (0 ?
 20:18) + 1))))))) << (0 ?
 20:18))) | (((gctUINT32) ((gctUINT32) (funcXlate[textureInfo->compareFunc]) & ((gctUINT32) ((((1 ?
 20:18) - (0 ?
 20:18) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 20:18) - (0 ? 20:18) + 1))))))) << (0 ? 20:18)))
      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 22:21) - (0 ?
 22:21) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 22:21) - (0 ?
 22:21) + 1))))))) << (0 ?
 22:21))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 22:21) - (0 ?
 22:21) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 22:21) - (0 ? 22:21) + 1))))))) << (0 ? 22:21)))
      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 23:23) - (0 ?
 23:23) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:23) - (0 ?
 23:23) + 1))))))) << (0 ?
 23:23))) | (((gctUINT32) ((gctUINT32) (integerFormat) & ((gctUINT32) ((((1 ?
 23:23) - (0 ?
 23:23) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 23:23) - (0 ? 23:23) + 1))))))) << (0 ? 23:23)));

    samplerCtrl1 =
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 2:2) - (0 ?
 2:2) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:2) - (0 ?
 2:2) + 1))))))) << (0 ?
 2:2))) | (((gctUINT32) ((gctUINT32) (sRGB) & ((gctUINT32) ((((1 ?
 2:2) - (0 ?
 2:2) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 2:2) - (0 ? 2:2) + 1))))))) << (0 ? 2:2)))
      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:3) - (0 ?
 3:3) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:3) - (0 ?
 3:3) + 1))))))) << (0 ?
 3:3))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxLODGuardbandEnable) & ((gctUINT32) ((((1 ?
 3:3) - (0 ?
 3:3) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) ((gctUINT32) (Hardware->TXStates->hwTxLODGuardbandIndex) & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)));

    samplerLODMinMax =
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:0) - (0 ?
 12:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:0) - (0 ?
 12:0) + 1))))))) << (0 ?
 12:0))) | (((gctUINT32) ((gctUINT32) (lodmax) & ((gctUINT32) ((((1 ?
 12:0) - (0 ?
 12:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0)))
      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 28:16) - (0 ?
 28:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 28:16) - (0 ?
 28:16) + 1))))))) << (0 ?
 28:16))) | (((gctUINT32) ((gctUINT32) (lodmin) & ((gctUINT32) ((((1 ?
 28:16) - (0 ?
 28:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16)));


    /* Enable LOD_BIAS when textureInfo->lodBias is not 0.0, else, disalbe LOD_BIAS */
    if (0x0 != bias)
    {
        samplerLODBias =
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (bias) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 16:16) - (0 ?
 16:16) + 1))))))) << (0 ?
 16:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)));
    }
    else
    {
        samplerLODBias =
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (bias) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 16:16) - (0 ?
 16:16) + 1))))))) << (0 ?
 16:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)));
    }


    samplerAnisCtrl =
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:0) - (0 ?
 10:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:0) - (0 ?
 10:0) + 1))))))) << (0 ?
 10:0))) | (((gctUINT32) ((gctUINT32) (anisoLog) & ((gctUINT32) ((((1 ?
 10:0) - (0 ?
 10:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:0) - (0 ? 10:0) + 1))))))) << (0 ? 10:0)));


    Hardware->MCStates->texHasTileStatus[Sampler] = SamplerInfo->hasTileStatus;
    Hardware->MCStates->texBaseLevelSurfInfoWithTS[Sampler] = SamplerInfo->baseLevelSurf;

    tsSlotIndex = Hardware->MCStates->texTileStatusSlotIndex[Sampler];

    if (SamplerInfo->hasTileStatus)
    {
        if (tsSlotIndex != -1)
        {
            Hardware->MCDirty->texTileStatusSlotDirty |= 1 << tsSlotIndex;
        }
        else
        {
            /* Mark sampler to let later BindTextureTS() assign new tsSlotIndex */
            gcsBITMASK_Set(&Hardware->TXDirty->hwTxDescDirty, Sampler);
        }
    }
    else
    {
        if (tsSlotIndex != -1)
        {
            gcmASSERT(tsSlotIndex < gcdSAMPLER_TS);
            Hardware->MCStates->texTileStatusSlotUser[tsSlotIndex] = -1;
            Hardware->MCStates->texTileStatusSlotIndex[Sampler] = -1;
            Hardware->MCDirty->texTileStatusSlotDirty |= 1 << tsSlotIndex;
        }
    }

    if (Hardware->features[gcvFEATURE_TX_FLUSH_L1CACHE])
    {
        Hardware->TXDirty->hwTxFlushL1Cache =
            (SamplerInfo->needYUVAssembler || (SamplerInfo->formatInfo->fmtClass == gcvFORMAT_CLASS_ASTC))
            ? gcvTRUE : gcvFALSE;
    }

    /*set sampler bind dirty for use it on small batch later*/
    gcsBITMASK_Set(&Hardware->TXDirty->hwSamplerBindDirty, Sampler);

    /* Save changed sampler mode register. */
    if (Hardware->TXStates->hwSamplerControl0[Sampler] != samplerCtrl0)
    {
        Hardware->TXStates->hwSamplerControl0[Sampler] = samplerCtrl0;
        gcsBITMASK_Set(&Hardware->TXDirty->hwSamplerControl0Dirty, Sampler);
        maskArray[count++] = &Hardware->TXDirty->hwSamplerControl0Dirty;
    }

    if (Hardware->TXStates->hwSamplerControl1[Sampler] != samplerCtrl1)
    {
        Hardware->TXStates->hwSamplerControl1[Sampler] = samplerCtrl1;
        gcsBITMASK_Set(&Hardware->TXDirty->hwSamplerControl1Dirty, Sampler);
        maskArray[count++] = &Hardware->TXDirty->hwSamplerControl1Dirty;
    }

    if (Hardware->TXStates->hwSamplerLodMinMax[Sampler] != samplerLODMinMax)
    {
        Hardware->TXStates->hwSamplerLodMinMax[Sampler] = samplerLODMinMax;
        gcsBITMASK_Set(&Hardware->TXDirty->hwSamplerLodMinMaxDirty, Sampler);
        maskArray[count++] = &Hardware->TXDirty->hwSamplerLodMinMaxDirty;
    }

    /* Save changed sampler size register. */
    if (Hardware->TXStates->hwSamplerLODBias[Sampler] != samplerLODBias)
    {
        Hardware->TXStates->hwSamplerLODBias[Sampler] = samplerLODBias;
        gcsBITMASK_Set(&Hardware->TXDirty->hwSamplerLODBiasDirty, Sampler);
        maskArray[count++] = &Hardware->TXDirty->hwSamplerLODBiasDirty;
    }

    /* Save changed sampler log2 size register. */
    if (Hardware->TXStates->hwSamplerAnisCtrl[Sampler] != samplerAnisCtrl)
    {
        Hardware->TXStates->hwSamplerAnisCtrl[Sampler] = samplerAnisCtrl;
        gcsBITMASK_Set(&Hardware->TXDirty->hwSamplerAnisCtrlDirty, Sampler);
        maskArray[count++] = &Hardware->TXDirty->hwSamplerAnisCtrlDirty;
    }

    /* Save changed sampler LOD register. */
    if (Hardware->TXStates->hwTxDescAddress[Sampler] != txDescAddress)
    {
        Hardware->TXStates->hwTxDescAddress[Sampler] = txDescAddress;
        gcsBITMASK_Set(&Hardware->TXDirty->hwTxDescAddressDirty, Sampler);
        maskArray[count++] = &Hardware->TXDirty->hwTxDescAddressDirty;
    }

    if (Hardware->TXStates->hwTextureControl[Sampler] != 0)
    {
        Hardware->TXStates->hwTextureControl[Sampler] = 0;
        gcsBITMASK_Set(&Hardware->TXDirty->hwTextureControlDirty, Sampler);
        maskArray[count++] = &Hardware->TXDirty->hwTextureControlDirty;
    }

    gcmASSERT(count <= 10);

    gcsBITMASK_MergeBitMaskArray(&Hardware->TXDirty->hwTxDescDirty, maskArray, count);

    if (!gcsBITMASK_IsAllZero(&Hardware->TXDirty->hwTxDescDirty) ||
        (smallBatch && !gcsBITMASK_IsAllZero(&Hardware->TXDirty->hwSamplerBindDirty)))
    {
        Hardware->TXDirty->textureDirty = gcvTRUE;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_ProgramTextureDesc(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctINT  sampler;
    gctBOOL semaStall;
    gcsBITMASK txDescDirty;
    gctINT32 samplerTSDirty;
    gctUINT  samplerTSIndex;
    gctUINT32 samplerCtrl0Reg;
    gctUINT32 samplerCtrl1Reg;
    gctUINT32 samplerLODMaxMinReg;
    gctUINT32 samplerLODBiasReg;
    gctUINT32 samplerAnisCtrlReg;
    gctUINT32 texDescAddrReg;
    gctUINT32 textureControlAddrReg;
    gctUINT32 texCommandReg;
    gctINT prefetchSamplers[80];
    gctINT prefetchCount = 0;
    gctINT i;
    gctBOOL smallBatch = gcvFALSE;
    gcsHINT_PTR hints = gcvNULL;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETHARDWARE(Hardware);

    if (Hardware->MCDirty->hwTxSamplerTSDirty)
    {
        semaStall = gcvTRUE;
    }
    else
    {
        semaStall = gcvFALSE;
    }

    txDescDirty    = Hardware->TXDirty->hwTxDescDirty;
    samplerTSDirty = Hardware->MCDirty->hwTxSamplerTSDirty;
    smallBatch     = Hardware->features[gcvFEATURE_SMALL_BATCH] && Hardware->options.smallBatch;
    hints          = Hardware->SHStates->programState.hints;

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, Memory);

    /* before program uniforms, add "allocation".*/
    if (smallBatch &&
        hints->unifiedStatus.samplerCount != 0 &&
        hints->unifiedStatus.samplerUnifiedMode != gcvUNIFORM_ALLOC_NONE_UNIFIED)
    {
        /* now, always copy.*/
        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x042C) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x042C, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 6:0) - (0 ?
 6:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:0) - (0 ?
 6:0) + 1))))))) << (0 ?
 6:0))) | (((gctUINT32) ((gctUINT32) (hints->unifiedStatus.samplerCount) & ((gctUINT32) ((((1 ?
 6:0) - (0 ?
 6:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:0) - (0 ?
 6:0) + 1))))))) << (0 ?
 6:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:31) - (0 ?
 31:31) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:31) - (0 ?
 31:31) + 1))))))) << (0 ?
 31:31))) | (((gctUINT32) ((gctUINT32) (gcvTRUE) & ((gctUINT32) ((((1 ?
 31:31) - (0 ?
 31:31) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};


        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x042D) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x042D, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 6:0) - (0 ?
 6:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:0) - (0 ?
 6:0) + 1))))))) << (0 ?
 6:0))) | (((gctUINT32) ((gctUINT32) (hints->unifiedStatus.samplerCount) & ((gctUINT32) ((((1 ?
 6:0) - (0 ?
 6:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:0) - (0 ?
 6:0) + 1))))))) << (0 ?
 6:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:31) - (0 ?
 31:31) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:31) - (0 ?
 31:31) + 1))))))) << (0 ?
 31:31))) | (((gctUINT32) ((gctUINT32) (gcvTRUE) & ((gctUINT32) ((((1 ?
 31:31) - (0 ?
 31:31) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};


        /*for those sampler which is bind before and sampler id is larger than the copy size,
        should dirty on this sampler.*/
        for (sampler = 0; !gcsBITMASK_IsAllZero(&Hardware->TXDirty->hwSamplerBindDirty); sampler++)
        {
            if (gcsBITMASK_TestAndClear(&Hardware->TXDirty->hwSamplerBindDirty, sampler) &&
                sampler >= (gctINT)Hardware->SHStates->samplerCount)
            {
                gcsBITMASK_Set(&Hardware->TXDirty->hwSamplerControl0Dirty, sampler);
                gcsBITMASK_Set(&Hardware->TXDirty->hwSamplerControl1Dirty, sampler);
                gcsBITMASK_Set(&Hardware->TXDirty->hwSamplerLODBiasDirty, sampler);
                gcsBITMASK_Set(&Hardware->TXDirty->hwSamplerLodMinMaxDirty, sampler);
                gcsBITMASK_Set(&Hardware->TXDirty->hwSamplerAnisCtrlDirty, sampler);
                gcsBITMASK_Set(&Hardware->TXDirty->hwTxDescAddressDirty, sampler);
                gcsBITMASK_Set(&Hardware->TXDirty->hwTextureControlDirty, sampler);
                gcsBITMASK_Set(&txDescDirty, sampler);
            }
        }

        Hardware->SHStates->samplerCount = hints->unifiedStatus.samplerCount;
    }

    if (Hardware->TXDirty->hwTxFlushVS)
    {
        if (!Hardware->features[gcvFEATURE_TEX_CACHE_FLUSH_FIX])
        {
            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E02) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0E02, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            /* Send FE-PE stall token. */
            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)));

            /* Dump the stall. */
            gcmDUMP(gcvNULL, "#[stall 0x%08X 0x%08X]",
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))),
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))));
        }

        /* Flush the texture unit. */
        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0E03, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:1) - (0 ?
 1:1) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:1) - (0 ?
 1:1) + 1))))))) << (0 ?
 1:1))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:1) - (0 ?
 1:1) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:1) - (0 ?
 1:1) + 1))))))) << (0 ?
 1:1))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


        Hardware->TXDirty->hwTxFlushVS = gcvFALSE;
    }

    if (Hardware->TXDirty->hwTxFlushL1Cache)
    {
        /* Flush the texture L1 cache through Gpipe or pixel shader*/
        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0E03, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:12) - (0 ?
 12:12) + 1))))))) << (0 ?
 12:12))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 12:12) - (0 ?
 12:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:12) - (0 ?
 12:12) + 1))))))) << (0 ?
 12:12))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:13) - (0 ?
 13:13) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:13) - (0 ?
 13:13) + 1))))))) << (0 ?
 13:13))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 13:13) - (0 ?
 13:13) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:13) - (0 ? 13:13) + 1))))))) << (0 ? 13:13))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


        Hardware->TXDirty->hwTxFlushL1Cache = gcvFALSE;
    }

    for (sampler = 0; !gcsBITMASK_IsAllZero(&txDescDirty); sampler++)
    {

        if (gcsBITMASK_TestAndClear(&txDescDirty, sampler))
        {
            if (smallBatch ||
                sampler >= Hardware->SHStates->programState.hints->unifiedStatus.samplerGPipeStart)
            {
                samplerCtrl0Reg = 0x5800;
                samplerCtrl1Reg = 0x5880;
                samplerLODMaxMinReg = 0x5900;
                samplerLODBiasReg = 0x5980;
                samplerAnisCtrlReg = 0x5A00;
                texDescAddrReg = 0x5600;
                textureControlAddrReg = 0x5680;
            }
            else
            {
                samplerCtrl0Reg = 0x5B00;
                samplerCtrl1Reg = 0x5B80;
                samplerLODMaxMinReg = 0x5C00;
                samplerLODBiasReg = 0x5C80;
                samplerAnisCtrlReg = 0x5D00;
                texDescAddrReg = 0x5700;
                textureControlAddrReg = 0x5780;
            }

            if ((gcsBITMASK_TestAndClear(&Hardware->TXDirty->hwSamplerControl0Dirty, sampler))
#if gcdENABLE_APPCTXT_BLITDRAW
                && (Hardware->TXStates->hwSamplerControl0[sampler] != gcvINVALID_VALUE)
#endif
                )
            {
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (samplerCtrl0Reg + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, samplerCtrl0Reg + sampler, Hardware->TXStates->hwSamplerControl0[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            }

            if ((gcsBITMASK_TestAndClear(&Hardware->TXDirty->hwSamplerControl1Dirty, sampler))
#if gcdENABLE_APPCTXT_BLITDRAW
                && (Hardware->TXStates->hwSamplerControl1[sampler] != gcvINVALID_VALUE)
#endif
                )
            {
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (samplerCtrl1Reg + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, samplerCtrl1Reg + sampler, Hardware->TXStates->hwSamplerControl1[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            }

            if ((gcsBITMASK_TestAndClear(&Hardware->TXDirty->hwSamplerLodMinMaxDirty, sampler))
#if gcdENABLE_APPCTXT_BLITDRAW
                && (Hardware->TXStates->hwSamplerLodMinMax[sampler] != gcvINVALID_VALUE)
#endif
                )
            {
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (samplerLODMaxMinReg + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, samplerLODMaxMinReg + sampler, Hardware->TXStates->hwSamplerLodMinMax[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            }

            if ((gcsBITMASK_TestAndClear(&Hardware->TXDirty->hwSamplerLODBiasDirty, sampler))
#if gcdENABLE_APPCTXT_BLITDRAW
                && (Hardware->TXStates->hwSamplerLODBias[sampler] != gcvINVALID_VALUE)
#endif
                )
            {
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (samplerLODBiasReg + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, samplerLODBiasReg + sampler, Hardware->TXStates->hwSamplerLODBias[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            }

            if ((gcsBITMASK_TestAndClear(&Hardware->TXDirty->hwSamplerAnisCtrlDirty, sampler))
#if gcdENABLE_APPCTXT_BLITDRAW
                && (Hardware->TXStates->hwSamplerAnisCtrl[sampler] != gcvINVALID_VALUE)
#endif
                )
            {
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (samplerAnisCtrlReg + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, samplerAnisCtrlReg + sampler, Hardware->TXStates->hwSamplerAnisCtrl[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            }

            if (gcsBITMASK_TestAndClear(&Hardware->TXDirty->hwTxDescAddressDirty, sampler)
#if gcdENABLE_APPCTXT_BLITDRAW
                && (Hardware->TXStates->hwTxDescAddress[sampler] != gcvINVALID_ADDRESS)
#endif
                )
            {
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (texDescAddrReg + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, texDescAddrReg + sampler, Hardware->TXStates->hwTxDescAddress[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


                if ((prefetchCount <= 31) && (sampler < 64))
                {
                    prefetchSamplers[prefetchCount++] = sampler;
                }
            }

            if (gcsBITMASK_TestAndClear(&Hardware->TXDirty->hwTextureControlDirty, sampler)
#if gcdENABLE_APPCTXT_BLITDRAW
                && (Hardware->TXStates->hwTextureControl[sampler] != gcvINVALID_ADDRESS)
#endif
                )
            {
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (textureControlAddrReg + sampler) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, textureControlAddrReg + sampler, Hardware->TXStates->hwTextureControl[sampler] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            }
        }
    }

    for (i = 0; i < prefetchCount; i++)
    {
        if (smallBatch ||
            prefetchSamplers[i] >= Hardware->SHStates->programState.hints->unifiedStatus.samplerGPipeStart)
        {
            texCommandReg = 0x5311;
        }
        else
        {
            texCommandReg = 0x5312;
        }

        if (Hardware->features[gcvFEATURE_MULTI_CLUSTER])
        {
            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x5310) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x5310, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};

        }

        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (texCommandReg) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, texCommandReg, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:28) - (0 ?
 31:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:28) - (0 ?
 31:28) + 1))))))) << (0 ?
 31:28))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ?
 31:28) - (0 ?
 31:28) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:28) - (0 ?
 31:28) + 1))))))) << (0 ?
 31:28))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 6:0) - (0 ?
 6:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:0) - (0 ?
 6:0) + 1))))))) << (0 ?
 6:0))) | (((gctUINT32) ((gctUINT32) (prefetchSamplers[i]) & ((gctUINT32) ((((1 ?
 6:0) - (0 ?
 6:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 6:0) - (0 ? 6:0) + 1))))))) << (0 ? 6:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

    }

#if gcdSECURITY
    {
        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E02) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0E02, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


        /* Send FE-PE stall token. */
        *memory++ =
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

        *memory++ =
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)));

        /* Dump the stall. */
        gcmDUMP(gcvNULL, "#[stall 0x%08X 0x%08X]",
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))),
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))));
    }
#endif

    for (samplerTSIndex = 0;
         samplerTSDirty;
         samplerTSIndex++, samplerTSDirty >>=1)
    {
        if (!(samplerTSDirty & 0x1))
        {
            continue;
        }

        /* Save dirty state in state buffer. */
        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05C8 + samplerTSIndex) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x05C8 + samplerTSIndex, Hardware->MCStates->hwTXSampleTSConfig[samplerTSIndex] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05D0 + samplerTSIndex) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x05D0 + samplerTSIndex, Hardware->MCStates->hwTXSampleTSBuffer[samplerTSIndex] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05D8 + samplerTSIndex) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x05D8 + samplerTSIndex, Hardware->MCStates->hwTXSampleTSClearValue[samplerTSIndex] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05E0 + samplerTSIndex) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x05E0 + samplerTSIndex, Hardware->MCStates->hwTXSampleTSClearValueUpper[samplerTSIndex] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


         {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x06A0 + samplerTSIndex) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x06A0 + samplerTSIndex, Hardware->MCStates->hwTxSamplerTxBaseBuffer[samplerTSIndex] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


    }
    /* Check for texture cache flush. */
    if (Hardware->TXDirty->hwTxFlushPS)
    {
        /* Flush the texture unit. */
        {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0E03, Hardware->features[gcvFEATURE_MULTI_CLUSTER] ?
 ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) : ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 2:2) - (0 ?
 2:2) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:2) - (0 ?
 2:2) + 1))))))) << (0 ?
 2:2))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 2:2) - (0 ?
 2:2) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:2) - (0 ?
 2:2) + 1))))))) << (0 ?
 2:2))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:1) - (0 ?
 1:1) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:1) - (0 ?
 1:1) + 1))))))) << (0 ?
 1:1))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:1) - (0 ?
 1:1) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:1) - (0 ?
 1:1) + 1))))))) << (0 ?
 1:1))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


        /* Flushed the texture cache. */
        Hardware->TXDirty->hwTxFlushPS = gcvFALSE;
    }

    if (semaStall)
    {
        if (!Hardware->features[gcvFEATURE_PE_TILE_CACHE_FLUSH_FIX] &&
            Hardware->features[gcvFEATURE_BLT_ENGINE])
        {
            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x502E) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x502E, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x502B) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x502B, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E02) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0E02, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x10 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            /* Send FE-BLT stall token. */
            *memory++ =
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

            *memory++ =
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x10 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)));

            /* Dump the stall. */
            gcmDUMP(gcvNULL, "#[stall 0x%08X 0x%08X]",
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))),
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x10 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))));

            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x502E) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x502E, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};

        }
        else
        {
            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0594) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0594, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E02) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0E02, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            /* Send FE-PE stall token. */
            *memory++ =
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

            *memory++ =
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)));

            /* Dump the stall. */
            gcmDUMP(gcvNULL, "#[stall 0x%08X 0x%08X]",
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))),
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))));
        }
    }

    gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, Memory);

    /* Mark texture as updated. */
    Hardware->TXDirty->textureDirty = gcvFALSE;
    Hardware->TXDirty->hwTxDescDirty = txDescDirty;
    Hardware->MCDirty->hwTxSamplerTSDirty = 0;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


#endif

