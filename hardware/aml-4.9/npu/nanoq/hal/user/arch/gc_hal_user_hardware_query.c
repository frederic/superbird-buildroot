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

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_HARDWARE

/******************************************************************************\
****************************** Format Definitions ******************************
\******************************************************************************/

#define gcmDUMMY_FORMAT_ENTRY() \
    { gcvSURF_UNKNOWN }

static const gceTEXTURE_SWIZZLE baseComponents_rgba[] =
{
    gcvTEXTURE_SWIZZLE_R,
    gcvTEXTURE_SWIZZLE_G,
    gcvTEXTURE_SWIZZLE_B,
    gcvTEXTURE_SWIZZLE_A
};

static const gceTEXTURE_SWIZZLE baseComponents_rgb1[] =
{
    gcvTEXTURE_SWIZZLE_R,
    gcvTEXTURE_SWIZZLE_G,
    gcvTEXTURE_SWIZZLE_B,
    gcvTEXTURE_SWIZZLE_1
};


static const gceTEXTURE_SWIZZLE baseComponents_r001[] =
{
    gcvTEXTURE_SWIZZLE_R,
    gcvTEXTURE_SWIZZLE_0,
    gcvTEXTURE_SWIZZLE_0,
    gcvTEXTURE_SWIZZLE_1
};

static const gceTEXTURE_SWIZZLE baseComponents_rg01[] =
{
    gcvTEXTURE_SWIZZLE_R,
    gcvTEXTURE_SWIZZLE_G,
    gcvTEXTURE_SWIZZLE_0,
    gcvTEXTURE_SWIZZLE_1
};


static const gceTEXTURE_SWIZZLE baseComponents_rg00[] =
{
    gcvTEXTURE_SWIZZLE_R,
    gcvTEXTURE_SWIZZLE_G,
    gcvTEXTURE_SWIZZLE_0,
    gcvTEXTURE_SWIZZLE_0
};


static const gceTEXTURE_SWIZZLE baseComponents_000r[] =
{
    gcvTEXTURE_SWIZZLE_0,
    gcvTEXTURE_SWIZZLE_0,
    gcvTEXTURE_SWIZZLE_0,
    gcvTEXTURE_SWIZZLE_R
};

static const gceTEXTURE_SWIZZLE baseComponents_rrrg[] =
{
    gcvTEXTURE_SWIZZLE_R,
    gcvTEXTURE_SWIZZLE_R,
    gcvTEXTURE_SWIZZLE_R,
    gcvTEXTURE_SWIZZLE_G
};

static const gceTEXTURE_SWIZZLE baseComponents_rrra[] =
{
    gcvTEXTURE_SWIZZLE_R,
    gcvTEXTURE_SWIZZLE_R,
    gcvTEXTURE_SWIZZLE_R,
    gcvTEXTURE_SWIZZLE_A
};

static const gceTEXTURE_SWIZZLE baseComponents_r00g[] =
{
    gcvTEXTURE_SWIZZLE_R,
    gcvTEXTURE_SWIZZLE_0,
    gcvTEXTURE_SWIZZLE_0,
    gcvTEXTURE_SWIZZLE_G
};

static const gceTEXTURE_SWIZZLE baseComponents_rrr1[] =
{
    gcvTEXTURE_SWIZZLE_R,
    gcvTEXTURE_SWIZZLE_R,
    gcvTEXTURE_SWIZZLE_R,
    gcvTEXTURE_SWIZZLE_1
};

static const gceTEXTURE_SWIZZLE baseComponents_bgra[] =
{
    gcvTEXTURE_SWIZZLE_B,
    gcvTEXTURE_SWIZZLE_G,
    gcvTEXTURE_SWIZZLE_R,
    gcvTEXTURE_SWIZZLE_A
};

static const gceTEXTURE_SWIZZLE baseComponents_bgr1[] =
{
    gcvTEXTURE_SWIZZLE_B,
    gcvTEXTURE_SWIZZLE_G,
    gcvTEXTURE_SWIZZLE_R,
    gcvTEXTURE_SWIZZLE_1
};


#define gcmNameFormat(f)  #f, gcvSURF_##f

#define gcmINVALID_RENDER_FORMAT_ENTRY \
    gcvINVALID_RENDER_FORMAT, baseComponents_rgba

#define gcmINVALID_TEXTURE_FORMAT_ENTRY \
    gcvINVALID_TEXTURE_FORMAT, baseComponents_rgba, gcvFALSE

#define gcmNON_COMPRESSED_BPP_ENTRY(b) \
    b, 1, 1, b

#define gcmINVALID_PIXEL_FORMAT_CLASS \
    {{{0},{0},{0},{0},{0},{0}}}

/* NOTE: Only DEFAULT (pre halti) Entries initialized in the tables below. */

/* Format value range: 100-199
 * Class: palettized formats (gcsFORMAT_CLASS_TYPE_INDEX)
 * Component encoding: (index) */
static struct _gcsSURF_FORMAT_INFO formatPalettized[] =
{
    {
        gcmNameFormat(INDEX1), gcvFORMAT_CLASS_INDEX, gcvFORMAT_DATATYPE_INDEX, gcmNON_COMPRESSED_BPP_ENTRY(1),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 1 }, {0}, {0}, {0}, {0}, {0}}}, {{{ 0, 1 }, {0}, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_INDEX8, gcmINVALID_TEXTURE_FORMAT_ENTRY,
    },

    {
        gcmNameFormat(INDEX4), gcvFORMAT_CLASS_INDEX, gcvFORMAT_DATATYPE_INDEX, gcmNON_COMPRESSED_BPP_ENTRY(4),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 4 }, {0}, {0}, {0}, {0}, {0}}}, {{{ 0, 4 }, {0}, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_INDEX8, gcmINVALID_TEXTURE_FORMAT_ENTRY,
    },

    {
        gcmNameFormat(INDEX8), gcvFORMAT_CLASS_INDEX, gcvFORMAT_DATATYPE_INDEX, gcmNON_COMPRESSED_BPP_ENTRY(8),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 }, {0}, {0}, {0}, {0}, {0}}}, {{{ 0, 8 }, {0}, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY,
    },

#if gcdVG_ONLY
    {
        gcmNameFormat(INDEX2), gcvFORMAT_CLASS_INDEX, gcvFORMAT_DATATYPE_INDEX, gcmNON_COMPRESSED_BPP_ENTRY(2),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 2 }, {0}, {0}, {0}, {0}, {0}}}, {{{ 0, 2 }, {0}, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_INDEX8, gcmINVALID_TEXTURE_FORMAT_ENTRY,
    },
#endif
};

/* Format value range: 200-299
 * Class: RGB formats (gcsFORMAT_CLASS_TYPE_RGBA)
 * Component encoding: (A, R, G, B) */
static struct _gcsSURF_FORMAT_INFO formatRGB[] =
{
    {
        gcmNameFormat(A2R2G2B2), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(8),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 6, 2 }, { 4, 2 }, { 2, 2 }, { 0, 2 }, {0}, {0}}},
        {{{ 6, 2 }, { 4, 2 }, { 2, 2 }, { 0, 2 }, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY,
    },

    {
        gcmNameFormat(R3G3B2), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(8),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 5, 3 }, { 2, 3 }, { 0, 2 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 5, 3 }, { 2, 3 }, { 0, 2 }, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_R5G6B5, gcmINVALID_TEXTURE_FORMAT_ENTRY,
    },

    {
        gcmNameFormat(A8R3G3B2), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 8, 8 }, { 5, 3 }, { 2, 3 }, { 0, 2 }, {0}, {0}}},
        {{{ 8, 8 }, { 5, 3 }, { 2, 3 }, { 0, 2 }, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY,
    },

    {
        gcmNameFormat(X4R4G4B4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 12, 4 | gcvCOMPONENT_DONTCARE }, { 8, 4 }, { 4, 4 }, { 0, 4 }, {0}, {0}}},
        {{{ 12, 4 | gcvCOMPONENT_DONTCARE }, { 8, 4 }, { 4, 4 }, { 0, 4 }, {0}, {0}}},
        gcvSURF_X4R4G4B4, 0x00, baseComponents_rgba,
        gcvSURF_X4R4G4B4, 0x06, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(A4R4G4B4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 12, 4 }, { 8, 4 }, { 4, 4 }, { 0, 4 }, {0}, {0}}},
        {{{ 12, 4 }, { 8, 4 }, { 4, 4 }, { 0, 4 }, {0}, {0}}},
        gcvSURF_A4R4G4B4, 0x01, baseComponents_rgba,
        gcvSURF_A4R4G4B4, 0x05, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(R4G4B4A4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 4 }, { 12, 4 }, { 8, 4 }, { 4, 4 }, {0}, {0}}},
        {{{ 0, 4 }, { 12, 4 }, { 8, 4 }, { 4, 4 }, {0}, {0}}},
        gcvSURF_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X1R5G5B5), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 15, 1 | gcvCOMPONENT_DONTCARE }, { 10, 5 }, { 5, 5 }, { 0, 5 }, {0}, {0}}},
        {{{ 15, 1 | gcvCOMPONENT_DONTCARE }, { 10, 5 }, { 5, 5 }, { 0, 5 }, {0}, {0}}},
        gcvSURF_X1R5G5B5, 0x02, baseComponents_rgba,
        gcvSURF_X1R5G5B5, 0x0D, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(A1R5G5B5), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 15, 1 }, { 10, 5 }, { 5, 5 }, { 0, 5 }, {0}, {0}}},
        {{{ 15, 1 }, { 10, 5 }, { 5, 5 }, { 0, 5 }, {0}, {0}}},
        gcvSURF_A1R5G5B5, 0x03, baseComponents_rgba,
        gcvSURF_A1R5G5B5, 0x0C, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(R5G5B5A1), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 1 }, { 11, 5 }, { 6, 5 }, { 1, 5 }, {0}, {0}}},
        {{{ 0, 1 }, { 11, 5 }, { 6, 5 }, { 1, 5 }, {0}, {0}}},
        gcvSURF_A1R5G5B5, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A1R5G5B5, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(R5G6B5), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 11, 5 }, { 5, 6 }, { 0, 5 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 11, 5 }, { 5, 6 }, { 0, 5 }, {0}, {0}}},
        gcvSURF_R5G6B5, 0x04, baseComponents_rgba,
        gcvSURF_R5G6B5, 0x0B, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(24),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 16, 8 }, { 8, 8 }, { 0, 8 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 16, 8 }, { 8, 8 }, { 0, 8 }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 | gcvCOMPONENT_DONTCARE }, { 16, 8 }, { 8, 8 }, { 0, 8 }, {0}, {0}}},
        {{{ 24, 8 | gcvCOMPONENT_DONTCARE }, { 16, 8 }, { 8, 8 }, { 0, 8 }, {0}, {0}}},
        gcvSURF_X8R8G8B8, 0x05, baseComponents_rgba,
        gcvSURF_X8R8G8B8, 0x08, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 }, { 16, 8 }, { 8, 8 }, { 0, 8 }, {0}, {0}}},
        {{{ 24, 8 }, { 16, 8 }, { 8, 8 }, { 0, 8 }, {0}, {0}}},
        gcvSURF_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(R8G8B8A8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 }, { 24, 8 }, { 16, 8 }, { 8, 8 }, {0}, {0}}},
        {{{ 0, 8 }, { 24, 8 }, { 16, 8 }, { 8, 8 }, {0}, {0}}},
        gcvSURF_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(G8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 16, 8 }, { 8, 8 }, { 0, 8 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 16, 8 }, { 24, 8 }, { 0, 8 }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(R8G8B8G8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 16, 8 }, { 8, 8 }, { 0, 8 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 16, 8 }, { 24, 8 }, { 0, 8 }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X2R10G10B10), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 30, 2 | gcvCOMPONENT_DONTCARE}, { 20, 10 }, { 10, 10 }, { 0, 10 }, {0}, {0}}},
        {{{ 30, 2 | gcvCOMPONENT_DONTCARE}, { 20, 10 }, { 10, 10 }, { 0, 10 }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A2R10G10B10), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 30, 2 }, { 20, 10 }, { 10, 10 }, { 0, 10 }, {0}, {0}}},
        {{{ 30, 2 }, { 20, 10 }, { 10, 10 }, { 0, 10 }, {0}, {0}}},
        gcvSURF_A2R10G10B10, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A2R10G10B10, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(R10G10B10A2), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, 2 }, { 22, 10 }, { 12, 10 }, { 2, 10 }, {0}, {0}}},
        {{{ 0, 2 }, { 22, 10 }, { 12, 10 }, { 2, 10 }, {0}, {0}}},
        gcvSURF_R10G10B10A2, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_R10G10B10A2, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X12R12G12B12), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(48),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_QWORD,
        {{{ 36, 12 | gcvCOMPONENT_DONTCARE }, { 24, 12 }, { 12, 12 }, { 0, 12 }, {0}, {0}}},
        {{{ 36, 12 | gcvCOMPONENT_DONTCARE }, { 24, 12 }, { 12, 12 }, { 0, 12 }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A12R12G12B12), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(48),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_QWORD,
        {{{ 36, 12 }, { 24, 12 }, { 12, 12 }, { 0, 12 }, {0}, {0}}},
        {{{ 36, 12 }, { 24, 12 }, { 12, 12 }, { 0, 12 }, {0}, {0}}},
        gcvSURF_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X16R16G16B16), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 32, 16 }, { 16, 16 }, { 0, 16 }, {0}, {0}}},
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 32, 16 }, { 16, 16 }, { 0, 16 }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A16R16G16B16), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 48, 16 }, { 32, 16 }, { 16, 16 }, { 0, 16 }, {0}, {0}}},
        {{{ 48, 16 }, { 32, 16 }, { 16, 16 }, { 0, 16 }, {0}, {0}}},
        gcvSURF_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A32R32G32B32), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(128),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 96, 32 }, { 64, 32 }, { 32, 32 }, { 0, 32 }, {0}, {0}}},
        {{{ 96, 32 }, { 64, 32 }, { 32, 32 }, { 0, 32 }, {0}, {0}}},
        gcvSURF_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(R8G8B8X8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 | gcvCOMPONENT_DONTCARE }, { 24, 8}, { 16, 8}, { 8, 8}, {0}, {0}}},
        {{{ 0, 8 | gcvCOMPONENT_DONTCARE }, { 24, 8}, { 16, 8}, { 8, 8}, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(R5G5B5X1), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 1 | gcvCOMPONENT_DONTCARE }, { 11, 5 }, { 6, 5 }, { 1, 5 }, {0}, {0}}},
        {{{ 0, 1 | gcvCOMPONENT_DONTCARE }, { 11, 5 }, { 6, 5 }, { 1, 5 }, {0}, {0}}},
        gcvSURF_A1R5G5B5, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A1R5G5B5, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(R4G4B4X4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 4 | gcvCOMPONENT_DONTCARE }, { 12, 4 }, { 8, 4 }, { 4, 4 }, {0}, {0}}},
        {{{ 0, 4 | gcvCOMPONENT_DONTCARE }, { 12, 4 }, { 8, 4 }, { 4, 4 }, {0}, {0}}},
        gcvSURF_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },
};

/* Format value range: 300-399
 * Class: BGR formats (gcsFORMAT_CLASS_TYPE_RGBA)
 * Component encoding: (A, R, G, B) */
static struct _gcsSURF_FORMAT_INFO formatBGR[] =
{
    {
        gcmNameFormat(A4B4G4R4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 12, 4 }, { 0, 4 }, { 4, 4 }, { 8, 4 }, {0}, {0}}},
        {{{ 12, 4 }, { 0, 4 }, { 4, 4 }, { 8, 4 }, {0}, {0}}},
        gcvSURF_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A1B5G5R5), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 15, 1 }, { 0, 5 }, { 5, 5 }, { 10, 5 }, {0}, {0}}},
        {{{ 15, 1 }, { 0, 5 }, { 5, 5 }, { 10, 5 }, {0}, {0}}},
        gcvSURF_A1R5G5B5, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A1R5G5B5, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B5G6R5), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 5 }, { 5, 6 }, { 11, 5 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 5 }, { 5, 6 }, { 11, 5 }, {0}, {0}}},
        gcvSURF_R5G6B5, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_R5G6B5, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B8G8R8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(24),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B16G16R16), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(48),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X8B8G8R8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 24, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A8B8G8R8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 24, 8 }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvSURF_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A2B10G10R10), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 30, 2 }, { 0, 10 }, { 10, 10 }, { 20, 10 }, {0}, {0}}},
        {{{ 30, 2 }, { 0, 10 }, { 10, 10 }, { 20, 10 }, {0}, {0}}},
        gcvSURF_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X16B16G16R16), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A16B16G16R16), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B32G32R32), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(96),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X32B32G32R32), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(128),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A32B32G32R32), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(128),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B4G4R4A4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 4 }, { 4, 4 }, { 8, 4 }, { 12, 4 }, {0}, {0}}},
        {{{ 0, 4 }, { 4, 4 }, { 8, 4 }, { 12, 4 }, {0}, {0}}},
        gcvSURF_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B5G5R5A1), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 1 }, { 1, 5 }, { 6, 5 }, { 11, 5 }, {0}, {0}}},
        {{{ 0, 1 }, { 1, 5 }, { 6, 5 }, { 11, 5 }, {0}, {0}}},
        gcvSURF_A1R5G5B5, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A1R5G5B5, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B8G8R8X8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 | gcvCOMPONENT_DONTCARE }, { 8, 8 }, { 16, 8 }, { 24, 8 }, {0}, {0}}},
        {{{ 0, 8 | gcvCOMPONENT_DONTCARE }, { 8, 8 }, { 16, 8 }, { 24, 8 }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B8G8R8A8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 }, { 8, 8 }, { 16, 8 }, { 24, 8 }, {0}, {0}}},
        {{{ 0, 8 }, { 8, 8 }, { 16, 8 }, { 24, 8 }, {0}, {0}}},
        gcvSURF_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B10G10R10A2), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, 2 }, { 2, 10 }, { 12, 10 }, { 22, 10 }, {0}, {0}}},
        {{{ 0, 2 }, { 2, 10 }, { 12, 10 }, { 22, 10 }, {0}, {0}}},
        gcvSURF_B10G10R10A2, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_B10G10R10A2, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X4B4G4R4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 12, 4 | gcvCOMPONENT_DONTCARE }, { 0, 4 }, { 4, 4 }, { 8, 4 }, {0}, {0}}},
        {{{ 12, 4 | gcvCOMPONENT_DONTCARE }, { 0, 4 }, { 4, 4 }, { 8, 4 }, {0}, {0}}},
        gcvSURF_X4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X1B5G5R5), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 15, 1 | gcvCOMPONENT_DONTCARE }, { 0, 5 }, { 5, 5 }, { 10, 5 }, {0}, {0}}},
        {{{ 15, 1 | gcvCOMPONENT_DONTCARE }, { 0, 5 }, { 5, 5 }, { 10, 5 }, {0}, {0}}},
        gcvSURF_X1R5G5B5, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X1R5G5B5, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B4G4R4X4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 4 | gcvCOMPONENT_DONTCARE }, { 4, 4 }, { 8, 4 }, { 12, 4 }, {0}, {0}}},
        {{{ 0, 4 | gcvCOMPONENT_DONTCARE }, { 4, 4 }, { 8, 4 }, { 12, 4 }, {0}, {0}}},
        gcvSURF_X4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B5G5R5X1), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 1 | gcvCOMPONENT_DONTCARE }, { 1, 5 }, { 6, 5 }, { 11, 5 }, {0}, {0}}},
        {{{ 0, 1 | gcvCOMPONENT_DONTCARE }, { 1, 5 }, { 6, 5 }, { 11, 5 }, {0}, {0}}},
        gcvSURF_X1R5G5B5, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X1R5G5B5, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X2B10G10R10), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 30, 2 | gcvCOMPONENT_DONTCARE }, { 0, 10 }, { 10, 10 }, { 20, 10 }, {0}, {0}}},
        {{{ 30, 2 | gcvCOMPONENT_DONTCARE }, { 0, 10 }, { 10, 10 }, { 20, 10 }, {0}, {0}}},
        gcvSURF_X2B10G10R10, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X2B10G10R10, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B8G8R8_SNORM), gcvFORMAT_CLASS_RGBA,gcvFORMAT_DATATYPE_SIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(24),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8B8G8R8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X8B8G8R8_SNORM), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 24, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8B8G8R8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A8B8G8R8_SNORM), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 24, 8 }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8B8G8R8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    /* patch format for sRGB rendering to get better precision on cores without half float rendering*/
    {
        gcmNameFormat(A8B12G12R12_2_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(64),
        2, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 48, 8 }, { 0, 12 }, { 12, 12 }, { 32, 12 }, {0}, {0}}},
        {{{ 48, 8 }, { 0, 12 }, { 12, 12 }, { 32, 12 }, {0}, {0}}},
        gcvSURF_A8B12G12R12_2_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_A8B12G12R12_2_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

};

/* Format value range: 400-499
 * Class: compressed formats
 * Component encoding: (A, R, G, B) */
static struct _gcsSURF_FORMAT_INFO formatCompressed[] =
{
    {
        gcmNameFormat(DXT1), gcvFORMAT_CLASS_COMPRESSED, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 4, 4, 4, 64,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A1R5G5B5, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(DXT2), gcvFORMAT_CLASS_COMPRESSED, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 8, 4, 4, 64,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(DXT3), gcvFORMAT_CLASS_COMPRESSED, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 8, 4, 4, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(DXT4), gcvFORMAT_CLASS_COMPRESSED, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 8, 4, 4, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(DXT5), gcvFORMAT_CLASS_COMPRESSED, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 8, 4, 4, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(CXV8U8), gcvFORMAT_CLASS_COMPRESSED, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 16, 1, 1, 16,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ETC1), gcvFORMAT_CLASS_COMPRESSED, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 4, 4, 4, 64,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(R11_EAC), gcvFORMAT_CLASS_COMPRESSED, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 4, 4, 4, 64,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_R11_EAC, 0x03 << 12, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(SIGNED_R11_EAC), gcvFORMAT_CLASS_COMPRESSED, gcvFORMAT_DATATYPE_SIGNED_NORMALIZED, 4, 4, 4, 64,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_SIGNED_R11_EAC, 0x0D << 12, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(RG11_EAC), gcvFORMAT_CLASS_COMPRESSED, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 8, 4, 4, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_RG11_EAC, 0x04 << 12, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(SIGNED_RG11_EAC), gcvFORMAT_CLASS_COMPRESSED, gcvFORMAT_DATATYPE_SIGNED_NORMALIZED, 8, 4, 4, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_SIGNED_RG11_EAC, 0x05 << 12, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(RGB8_ETC2), gcvFORMAT_CLASS_COMPRESSED, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 4, 4, 4, 64,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_RGB8_ETC2, 0x00 << 12, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(SRGB8_ETC2), gcvFORMAT_CLASS_COMPRESSED, gcvFORMAT_DATATYPE_SRGB, 4, 4, 4, 64,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_SRGB8_ETC2, (0x00 << 12) | (1 << 18), baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(RGB8_PUNCHTHROUGH_ALPHA1_ETC2), gcvFORMAT_CLASS_COMPRESSED, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 4, 4, 4, 64,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, 0x01 << 12, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(SRGB8_PUNCHTHROUGH_ALPHA1_ETC2), gcvFORMAT_CLASS_COMPRESSED, gcvFORMAT_DATATYPE_SRGB, 4, 4, 4, 64,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, (0x01 << 12) | (1 << 18), baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(RGBA8_ETC2_EAC), gcvFORMAT_CLASS_COMPRESSED, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 8, 4, 4, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_RGBA8_ETC2_EAC, 0x02 << 12, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(SRGB8_ALPHA8_ETC2_EAC), gcvFORMAT_CLASS_COMPRESSED, gcvFORMAT_DATATYPE_SRGB, 8, 4, 4, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_SRGB8_ALPHA8_ETC2_EAC, (0x02 << 12) | (1 << 18), baseComponents_rgba, gcvTRUE
    },
};

/* Format value range: 500-599
 * Class: YUV formats (gcsFORMAT_CLASS_TYPE_YUV)
 * Component encoding: (Y, U, V) */
static struct _gcsSURF_FORMAT_INFO formatYUV[] =
{
    {
        gcmNameFormat(YUY2), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 16, 2, 1, 32,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 }, { 8, 8 }, { 24, 8 }, {0}, {0}, {0}}},
        {{{ 16, 8 }, { 8, 8 }, { 24, 8 }, {0}, {0}, {0}}},
        gcvSURF_YUY2, 0x07, baseComponents_rgba,
        gcvSURF_YUY2, 0x0E, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(UYVY), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 16, 2, 1, 32,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 8, 8 }, { 0, 8 }, { 16, 8 }, {0}, {0}, {0}}},
        {{{ 24, 8 }, { 0, 8 }, { 16, 8 }, {0}, {0}, {0}}},
        gcvSURF_UYVY, 0x07, baseComponents_rgba,
        gcvSURF_UYVY, 0x0F, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(YV12), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 12, 2, 2, 48,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 }, { 0, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        {{{ 0, 8 }, { 0, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(I420), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 12, 2, 2, 48,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 }, { 0, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        {{{ 0, 8 }, { 0, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(NV12), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 12, 2, 2, 48,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 }, { 0, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        {{{ 0, 8 }, { 0, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(NV21), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 12, 2, 2, 48,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 }, { 0, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        {{{ 0, 8 }, { 0, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(NV16), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 16, 2, 1, 32,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 }, { 0, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        {{{ 0, 8 }, { 0, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(NV61), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 16, 2, 1, 32,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 }, { 0, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        {{{ 0, 8 }, { 0, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(YVYU), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 16, 2, 1, 32,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 }, { 24, 8 }, { 8, 8 }, {0}, {0}, {0}}},
        {{{ 16, 8 }, { 24, 8 }, { 8, 8 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_YUY2, 0x0E, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(VYUY), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 16, 2, 1, 32,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 8, 8 }, { 16, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        {{{ 24, 8 }, { 16, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_UYVY, 0x0F, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(AYUV), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 32, 1, 1, 32,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 8, 8 }, { 16, 8 }, { 24, 8 }, {0}, {0}, {0}}},
        {{{ 8, 8 }, { 16, 8 }, { 24, 8 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_AYUV, 0x25 << 12, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(YUV420_10_ST), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 15, 8, 2, 240,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 10 }, { 0, 10 }, { 0, 10 }, {0}, {0}, {0}}},
        {{{ 0, 10 }, { 0, 10 }, { 0, 10 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(YUV420_TILE_ST), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 12, 64, 64, 6144,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 }, { 0, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        {{{ 0, 8 }, { 0, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(YUV420_TILE_10_ST), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 15, 64, 64, 7680,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 10 }, { 0, 10 }, { 0, 10 }, {0}, {0}, {0}}},
        {{{ 0, 10 }, { 0, 10 }, { 0, 10 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(NV12_10BIT), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 15, 8, 2, 240,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 10 }, { 0, 10 }, { 0, 10 }, {0}, {0}, {0}}},
        {{{ 0, 10 }, { 0, 10 }, { 0, 10 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(NV21_10BIT), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 15, 8, 2, 240,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 10 }, { 0, 10 }, { 0, 10 }, {0}, {0}, {0}}},
        {{{ 0, 10 }, { 0, 10 }, { 0, 10 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(NV16_10BIT), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 20, 8, 2, 240,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 10 }, { 0, 10 }, { 0, 10 }, {0}, {0}, {0}}},
        {{{ 0, 10 }, { 0, 10 }, { 0, 10 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(NV61_10BIT), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 20, 8, 2, 240,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 10 }, { 0, 10 }, { 0, 10 }, {0}, {0}, {0}}},
        {{{ 0, 10 }, { 0, 10 }, { 0, 10 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(P010), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 32, 8, 2, 240,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 16 }, { 0, 16 }, { 0, 16 }, {0}, {0}, {0}}},
        {{{ 0, 16 }, { 0, 16 }, { 0, 16 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

#if gcdVG_ONLY
    /* Just for format query in new 355VG core, not actually rendering usage for 3D core. */
    {
        gcmNameFormat(AYUY2), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 32, 1, 1, 32,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 8, 8 }, { 16, 8 }, { 24, 8 }, {0}, {0}, {0}}},
        {{{ 8, 8 }, { 16, 8 }, { 24, 8 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_AYUY2, 0x25 << 12, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(ANV12), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 32, 1, 1, 32,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 8, 8 }, { 16, 8 }, { 24, 8 }, {0}, {0}, {0}}},
        {{{ 8, 8 }, { 16, 8 }, { 24, 8 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_ANV12, 0x25 << 12, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(ANV16), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 32, 1, 1, 32,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 8, 8 }, { 16, 8 }, { 24, 8 }, {0}, {0}, {0}}},
        {{{ 8, 8 }, { 16, 8 }, { 24, 8 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_ANV16, 0x25 << 12, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(AUYVY), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 32, 1, 1, 32,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 8, 8 }, { 16, 8 }, { 24, 8 }, {0}, {0}, {0}}},
        {{{ 8, 8 }, { 16, 8 }, { 24, 8 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_AUYVY, 0x25 << 12, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(YV16), gcvFORMAT_CLASS_YUV, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 16, 2, 1, 32,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 }, { 0, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        {{{ 0, 8 }, { 0, 8 }, { 0, 8 }, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },
#endif
};

/* Format value range: 600-699
 * Class: depth and stencil formats (gcsFORMAT_CLASS_TYPE_DEPTH)
 * Component encoding: (D, S) */
static struct _gcsSURF_FORMAT_INFO formatDepth[] =
{
    {
        gcmNameFormat(D16), gcvFORMAT_CLASS_DEPTH, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}, {0}, {0}}},
        {{{ 0, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}, {0}, {0}}},
        gcvSURF_D16, 0x0, baseComponents_rgba,
        gcvSURF_D16, 0x10, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(D24S8), gcvFORMAT_CLASS_DEPTH, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 8, 24 }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        {{{ 8, 24 }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        gcvSURF_D24S8, 0x1, baseComponents_rgba,
        gcvSURF_D24S8, 0x11, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(D32), gcvFORMAT_CLASS_DEPTH, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}, {0}, {0}}},
        {{{ 0, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}, {0}, {0}}},
        gcvSURF_D24X8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_D24X8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(D24X8), gcvFORMAT_CLASS_DEPTH, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 8, 24 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}, {0}, {0}}},
        {{{ 8, 24 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}, {0}, {0}}},
        gcvSURF_D24X8, 0x1, baseComponents_rgba,
        gcvSURF_D24X8, 0x11, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(D32F), gcvFORMAT_CLASS_DEPTH, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}, {0}, {0}}},
        {{{ 0, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}, {0}, {0}}},
        gcvSURF_D24X8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_S8D32F_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(S8D32F), gcvFORMAT_CLASS_DEPTH, gcvFORMAT_DATATYPE_FLOAT32_UINT, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, 32 }, { 32, 8 }, {0}, {0}, {0}, {0}}},
        {{{ 0, 32 }, { 32, 8 }, {0}, {0}, {0}, {0}}},
        gcvSURF_D24S8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_S8D32F_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(S8D32F_1_G32R32F), gcvFORMAT_CLASS_DEPTH, gcvFORMAT_DATATYPE_FLOAT32_UINT, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 1, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, 32 }, { 32, 32 }, {0}, {0}, {0}, {0}}},
        {{{ 0, 32 }, { 32, 32 }, {0}, {0}, {0}, {0}}},
        gcvSURF_S8D32F_1_G32R32F, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_S8D32F_1_G32R32F, 0x0B << 12, baseComponents_rgba, gcvFALSE,
    },

    {
        gcmNameFormat(S8D32F_2_A8R8G8B8), gcvFORMAT_CLASS_DEPTH, gcvFORMAT_DATATYPE_FLOAT32_UINT, gcmNON_COMPRESSED_BPP_ENTRY(64),
        2, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 32 }, { 32, 32 }, {0}, {0}, {0}, {0}}},
        {{{ 0, 32 }, { 32, 32 }, {0}, {0}, {0}, {0}}},
        gcvSURF_S8D32F_2_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_S8D32F_2_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE,
    },

    {
        gcmNameFormat(D24S8_1_A8R8G8B8), gcvFORMAT_CLASS_DEPTH, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 8, 24 }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        {{{ 8, 24 }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        gcvSURF_D24S8_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_D24S8_1_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(S8), gcvFORMAT_CLASS_DEPTH, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(8),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 8, 24 }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        {{{ 8, 24 }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        gcvSURF_X24S8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X24S8, gcmINVALID_TEXTURE_FORMAT_ENTRY,
    },

    {
        gcmNameFormat(X24S8), gcvFORMAT_CLASS_DEPTH, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 8, 24 }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        {{{ 8, 24 }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        gcvSURF_X24S8, 0x1, baseComponents_rgba,
        gcvSURF_X24S8_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY,
    },

    {
        gcmNameFormat(X24S8_1_A8R8G8B8), gcvFORMAT_CLASS_DEPTH, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 8, 24 }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        {{{ 8, 24 }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        gcvSURF_D24S8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X24S8_1_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE
    },

};

/* Format value range: 700-799
 * Class: alpha formats (gcsFORMAT_CLASS_TYPE_RGBA)
 * Component encoding: (A, R, G, B) */
static struct _gcsSURF_FORMAT_INFO formatAlpha[] =
{
    {
        gcmNameFormat(A4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(4),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 4 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 4 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(8),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 8 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8, 0x01, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(A12), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(12),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 12 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 12 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A16), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 16 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 16 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A32), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, 32 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 32 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A1), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(1),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 1 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 1 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },
};

/* Format value range: 800-899
 * Class: luminance formats (gcsFORMAT_CLASS_TYPE_LUMINANCE)
 * Component encoding: (A, L) */
static struct _gcsSURF_FORMAT_INFO formatLuminance[] =
{
    {
        gcmNameFormat(L4), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(4),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 4 }, {0}, {0}, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 4 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_L8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(L8), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(8),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_L8, 0x02, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(L12), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(12),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 12 }, {0}, {0}, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 12 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_L8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(L16), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, {0}, {0}, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_L8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(L32), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, {0}, {0}, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_L8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(L1), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(1),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 1 }, {0}, {0}, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 1 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_L8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(L8_RAW), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(8),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_L8, 0x02, baseComponents_rgba, gcvTRUE
    },
};

/* Format value range: 900-999
 * Class: luminance formats (gcsFORMAT_CLASS_TYPE_LUMINANCE)
 * Component encoding: (A, L) */
static struct _gcsSURF_FORMAT_INFO formatLuminanceAlpha[] =
{
    {
        gcmNameFormat(A4L4), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(8),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 4, 4 }, { 0, 4 }, {0}, {0}, {0}, {0}}},
        {{{ 4, 4 }, { 0, 4 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8L8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A2L6), gcvFORMAT_CLASS_LUMINANCE,gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(8),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 6, 2 }, { 0, 6 }, {0}, {0}, {0}, {0}}},
        {{{ 6, 2 }, { 0, 6 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8L8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A8L8), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 8, 8 }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        {{{ 8, 8 }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8L8, 0x04, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(A4L12), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 12, 4 }, { 0, 12 }, {0}, {0}, {0}, {0}}},
        {{{ 12, 4 }, { 0, 12 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8L8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A12L12), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(24),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 12, 12 }, { 0, 12 }, {0}, {0}, {0}, {0}}},
        {{{ 12, 12 }, { 0, 12 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8L8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A16L16), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 16, 16 }, { 0, 16 }, {0}, {0}, {0}, {0}}},
        {{{ 16, 16 }, { 0, 16 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8L8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A8L8_1_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, gcvTRUE, gcvFALSE, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 }, { 16, 8}, { 8, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 | gcvCOMPONENT_DONTCARE }, {0}, {0}}},
        {{{ 24, 8 }, { 16, 8}, { 8, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 | gcvCOMPONENT_DONTCARE }, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8L8_1_A8R8G8B8, 0x07, baseComponents_rrra, gcvTRUE
    },

    {
        gcmNameFormat(A8L8_RAW), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 8, 8 }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        {{{ 8, 8 }, { 0, 8 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8L8, 0x04, baseComponents_rgba, gcvTRUE
    },

};

/* Format value range: 1000-1099
 * Class: bump formats (gcsFORMAT_CLASS_TYPE_BUMP)
 * Component encoding: (A, L, V, U, Q, W) */
static struct _gcsSURF_FORMAT_INFO formatBump[] =
{
    {
        gcmNameFormat(L6V5U5), gcvFORMAT_CLASS_BUMP, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 10, 6 }, { 5, 5 }, { 0, 5 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 10, 6 }, { 5, 5 }, { 0, 5 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(V8U8), gcvFORMAT_CLASS_BUMP, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, { 8, 8 }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, { 8, 8 }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X8L8V8U8), gcvFORMAT_CLASS_BUMP, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 | gcvCOMPONENT_DONTCARE }, { 16, 8 }, { 8, 8 }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }}},
        {{{ 24, 8 | gcvCOMPONENT_DONTCARE }, { 16, 8 }, { 8, 8 }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(Q8W8V8U8), gcvFORMAT_CLASS_BUMP, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, { 8, 8 }, { 0, 8 },
          { 24, 8 }, { 16, 8 }}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, { 8, 8 }, { 0, 8 },
          { 24, 8 }, { 16, 8 }}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A2W10V10U10), gcvFORMAT_CLASS_BUMP, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 30, 2 }, { 0, gcvCOMPONENT_NOTPRESENT }, { 10, 10 }, { 0, 10 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 20, 10 }}},
        {{{ 30, 2 }, { 0, gcvCOMPONENT_NOTPRESENT }, { 10, 10 }, { 0, 10 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 20, 10 }}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(V16U16), gcvFORMAT_CLASS_BUMP, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, { 16, 16 }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, { 16, 16 }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(Q16W16V16U16), gcvFORMAT_CLASS_BUMP, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, { 16, 16 }, { 0, 16 },
          { 48, 16 }, { 32, 16 }}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, { 16, 16 }, { 0, 16 },
          { 48, 16 }, { 32, 16 }}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvINVALID_TEXTURE_FORMAT, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },
};

/* Format value range: 1100-1199
 * Class: R/RG/RA formats (gcsFORMAT_CLASS_TYPE_RGBA)
 * Component encoding: (A, R, G, B) */
static struct _gcsSURF_FORMAT_INFO formatRGB2[] =
{
    {
        gcmNameFormat(R8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(8),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_R8_1_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_R8_1_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X8R8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 8, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 8, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(G8R8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G8R8_1_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_G8R8_1_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X8G8R8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 | gcvCOMPONENT_DONTCARE }, { 8, 8 }, { 16, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 8 | gcvCOMPONENT_DONTCARE }, { 8, 8 }, { 16, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A8R8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 8, 8 }, { 0, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 8, 8 }, { 0, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(R16), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X16R16), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 16, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 16, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(G16R16), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X16G16R16), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(48),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 16 | gcvCOMPONENT_DONTCARE }, { 16, 16 }, { 32, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 16 | gcvCOMPONENT_DONTCARE }, { 16, 16 }, { 32, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A16R16), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 16, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 16, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(R32), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X32R32), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 32, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 32, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(G32R32), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X32G32R32), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(96),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, 32 | gcvCOMPONENT_DONTCARE }, { 32, 32 }, { 64, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 32 | gcvCOMPONENT_DONTCARE }, { 32, 32 }, { 64, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A32R32), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 32, 32 }, { 0, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 32, 32 }, { 0, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(RG16), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 8, 8 }, { 0, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 8, 8 }, { 0, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(R8_SNORM), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(8),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(G8R8_SNORM), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(R8_1_X8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 | gcvCOMPONENT_DONTCARE }, { 16, 8 }, { 8, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 | gcvCOMPONENT_DONTCARE }, {0}, {0}}},
        {{{ 24, 8 | gcvCOMPONENT_DONTCARE }, { 16, 8 }, { 8, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 | gcvCOMPONENT_DONTCARE }, {0}, {0}}},
        gcvSURF_R8_1_X8R8G8B8, 0x05, baseComponents_r001,
        gcvSURF_R8_1_X8R8G8B8, 0x08, baseComponents_r001, gcvTRUE
    },

    {
        gcmNameFormat(G8R8_1_X8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 | gcvCOMPONENT_DONTCARE }, { 16, 8 }, { 8, 8 }, { 0, 8 | gcvCOMPONENT_DONTCARE }, {0}, {0}}},
        {{{ 24, 8 | gcvCOMPONENT_DONTCARE }, { 16, 8 }, { 8, 8 }, { 0, 8 | gcvCOMPONENT_DONTCARE }, {0}, {0}}},
        gcvSURF_G8R8_1_X8R8G8B8, 0x05, baseComponents_rg01,
        gcvSURF_G8R8_1_X8R8G8B8, 0x08, baseComponents_rg01, gcvTRUE
    },
};

/* Format value range: 1200-1299
 * Class: floating point formats
 * Component encoding: (A, R, G, B) */
static struct _gcsSURF_FORMAT_INFO formatFP[] =
{
    {
        gcmNameFormat(R16F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT16, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_R16F_1_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_R16F_1_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X16R16F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT16, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 16, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 16, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_R16F_1_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_R16F_1_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(G16R16F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT16, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G16R16F_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_G16R16F_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X16G16R16F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT16, gcmNON_COMPRESSED_BPP_ENTRY(48),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 16 | gcvCOMPONENT_DONTCARE }, { 16, 16 }, { 32, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 16 | gcvCOMPONENT_DONTCARE }, { 16, 16 }, { 32, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G16R16F_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_G16R16F_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B16G16R16F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT16, gcmNON_COMPRESSED_BPP_ENTRY(48),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_B16G16R16F_2_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_B16G16R16F_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X16B16G16R16F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT16, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_B16G16R16F_2_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_B16G16R16F_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A16B16G16R16F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT16, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_A16B16G16R16F_2_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A16B16G16R16F_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_R32F_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_R32F_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 32, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 32, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_R32F_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_R32F_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(G32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G32R32F_2_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_G32R32F_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X32G32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(96),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, 32 | gcvCOMPONENT_DONTCARE }, { 32, 32 }, { 64, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 32 | gcvCOMPONENT_DONTCARE }, { 32, 32 }, { 64, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G32R32F_2_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_G32R32F_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B32G32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(96),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_B32G32R32F_3_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_B32G32R32F_3_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X32B32G32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(128),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_B32G32R32F_3_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_B32G32R32F_3_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A32B32G32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(128),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_A32B32G32R32F_4_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A32B32G32R32F_4_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A16F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT16, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 16 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 16 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_A16B16G16R16F_2_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A16B16G16R16F_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(L16F), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_FLOAT16, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, {0}, {0}, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A16B16G16R16F_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A16L16F), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_FLOAT16, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 16, 16 }, { 0, 16 }, {0}, {0}, {0}, {0}}},
        {{{ 16, 16 }, { 0, 16 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A16B16G16R16F_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A16R16F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT16, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 16, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 16, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_A16B16G16R16F_2_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A16B16G16R16F_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, 32 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 32 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_A32B32G32R32F_4_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A32B32G32R32F_4_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(L32F), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, {0}, {0}, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A32B32G32R32F_4_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

     {
        gcmNameFormat(A32L32F), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 32, 32 }, { 0, 32 }, {0}, {0}, {0}, {0}}},
        {{{ 32, 32 }, { 0, 32 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A32B32G32R32F_4_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 32, 32 }, { 0, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 32, 32 }, { 0, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_A32B32G32R32F_4_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A32B32G32R32F_4_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(E5B9G9R9), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT_E5B9G9R9, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 27, 5 }, { 18, 9 }, { 9, 9 }, { 0, 9 }, {0}, {0}}},
        {{{ 27, 5 }, { 18, 9 }, { 9, 9 }, { 0, 9 }, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_B16G16R16F_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
         gcmNameFormat(B10G11R11F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT_B10G11R11F, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 22, 10 }, { 11, 11 }, { 0, 11 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 22, 10 }, { 11, 11 }, { 0, 11 }, {0}, {0}}},
        gcvSURF_B10G11R11F_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_B10G11R11F_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    /*
    ** Mutli-layer floating surface definition, we will move them to separate table later.
    */
    {
        gcmNameFormat(X16B16G16R16F_2_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT16, gcmNON_COMPRESSED_BPP_ENTRY(64),
        2, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_X16B16G16R16F_2_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_X16B16G16R16F_2_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(A16B16G16R16F_2_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT16, gcmNON_COMPRESSED_BPP_ENTRY(64),
        2, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_A16B16G16R16F_2_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_A16B16G16R16F_2_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(A16B16G16R16F_2_G16R16F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT16, gcmNON_COMPRESSED_BPP_ENTRY(64),
        2, 1, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_A16B16G16R16F_2_G16R16F, 0x12, baseComponents_rgba,
        gcvSURF_A16B16G16R16F_2_G16R16F, 0x08 << 12, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(G32R32F_2_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(64),
        2, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G32R32F_2_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_G32R32F_2_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(X32B32G32R32F_2_G32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(128),
        2, 1, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_X32B32G32R32F_2_G32R32F, 0x15, baseComponents_rgba,
        gcvSURF_X32B32G32R32F_2_G32R32F, 0x0B << 12, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(A32B32G32R32F_2_G32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(128),
        2, 1, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_A32B32G32R32F_2_G32R32F, 0x15, baseComponents_rgba,
        gcvSURF_A32B32G32R32F_2_G32R32F, 0x0B << 12, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(X32B32G32R32F_4_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(128),
        4, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_X32B32G32R32F_4_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_X32B32G32R32F_4_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(A32B32G32R32F_4_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(128),
        4, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_A32B32G32R32F_4_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_A32B32G32R32F_4_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(R16F_1_A4R4G4B4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT16, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 1, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_R16F_1_A4R4G4B4, 0x01, baseComponents_rgba,
        gcvSURF_R16F_1_A4R4G4B4, 0x05, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(G16R16F_1_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT16, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G16R16F_1_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_G16R16F_1_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },


    {
        gcmNameFormat(B16G16R16F_2_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT16, gcmNON_COMPRESSED_BPP_ENTRY(64),
        2, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_B16G16R16F_2_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_B16G16R16F_2_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(R32F_1_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_R32F_1_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_R32F_1_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(B32G32R32F_3_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(96),
        3, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_B32G32R32F_3_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_B32G32R32F_3_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
         gcmNameFormat(B10G11R11F_1_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_FLOAT_B10G11R11F, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 22, 10 }, { 11, 11 }, { 0, 11 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 22, 10 }, { 11, 11 }, { 0, 11 }, {0}, {0}}},
        gcvSURF_B10G11R11F_1_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_B10G11R11F_1_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(A32F_1_R32F), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, 32 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 32 }, { 0, gcvCOMPONENT_NOTPRESENT },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A32F_1_R32F, 0x0A<< 12, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(L32F_1_R32F), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, {0}, {0}, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_L32F_1_R32F, 0x0A << 12, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(A32L32F_1_G32R32F), gcvFORMAT_CLASS_LUMINANCE, gcvFORMAT_DATATYPE_FLOAT32, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 1, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 32, 32 }, { 0, 32 }, {0}, {0}, {0}, {0}}},
        {{{ 32, 32 }, { 0, 32 }, {0}, {0}, {0}, {0}}},
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A32L32F_1_G32R32F, 0x0B << 12, baseComponents_rgba, gcvFALSE
    },


};

/* Format value range: 1400-1499
 * Class: sRGB formats
 * Component encoding: N/A */
static struct _gcsSURF_FORMAT_INFO formatSRGB[] =
{
    {
        gcmNameFormat(SBGR8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SRGB, gcmNON_COMPRESSED_BPP_ENTRY(24),
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvSURF_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A16B16G16R16F_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {

        gcmNameFormat(A8_SBGR8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SRGB, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 24, 8 }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvSURF_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A16B16G16R16F_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X8_SBGR8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SRGB, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 24, 8 }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvSURF_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A16B16G16R16F_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },
    {
        gcmNameFormat(A8_SRGB8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SRGB, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 }, { 16, 8 }, { 8, 8 }, { 0, 8 }, {0}, {0}}},
        {{{ 24, 8 }, { 16, 8 }, { 8, 8 }, { 0, 8 }, {0}, {0}}},
        gcvSURF_A8_SRGB8, 0x06, baseComponents_rgba,
        gcvSURF_A8_SRGB8, (0x07) | (1 << 18), baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(X8_SRGB8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SRGB, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 }, { 16, 8 }, { 8, 8 }, { 0, 8 }, {0}, {0}}},
        {{{ 24, 8 }, { 16, 8 }, { 8, 8 }, { 0, 8 }, {0}, {0}}},
        gcvSURF_X8_SRGB8, 0x05, baseComponents_rgb1,
        gcvSURF_X8_SRGB8, (0x08) | (1 << 18), baseComponents_rgb1, gcvTRUE
    },
};

/* Format value range: 1500-1599
 * Class: Integer formats (gcsFORMAT_CLASS_TYPE_RGBA)
 * Component encoding: (A, R, G, B) */
static struct _gcsSURF_FORMAT_INFO formatINT[] =
{
    {
        gcmNameFormat(R8I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(8),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_R8I_1_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_R8I_1_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(R8UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(8),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_R8UI_1_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_R8UI_1_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(R16I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_R16I_1_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_R16I_1_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(R16UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_R16UI_1_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_R16UI_1_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(R32I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_R32I_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_R32I_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(R32UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_R32UI_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_R32UI_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X8R8I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 8, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 8, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8R8I_1_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8I_1_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(G8R8I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G8R8I_1_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_G8R8I_1_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X8R8UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 8, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 8, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8R8UI_1_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8R8UI_1_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(G8R8UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G8R8UI_1_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_G8R8UI_1_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X16R16I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 16, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 16, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X16R16I_1_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X16R16I_1_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(G16R16I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G16R16I_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_G16R16I_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X16R16UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 16, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 16, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X16R16UI_1_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X16R16UI_1_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(G16R16UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G16R16UI_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_G16R16UI_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X32R32I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 32, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 32, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X32R32I_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X32R32I_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(G32R32I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G32R32I_2_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_G32R32I_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X32R32UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 32, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 32, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X32R32UI_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X32R32UI_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(G32R32UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G32R32UI_2_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_G32R32UI_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X8G8R8I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(24),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 | gcvCOMPONENT_DONTCARE }, { 8, 8 }, { 16, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 8 | gcvCOMPONENT_DONTCARE }, { 8, 8 }, { 16, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8G8R8I_1_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8G8R8I_1_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B8G8R8I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(24),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvSURF_B8G8R8I_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_B8G8R8I_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X8G8R8UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(24),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, 8 | gcvCOMPONENT_DONTCARE }, { 8, 8 }, { 16, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 8 | gcvCOMPONENT_DONTCARE }, { 8, 8 }, { 16, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8G8R8UI_1_A4R4G4B4, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X8G8R8UI_1_A4R4G4B4, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B8G8R8UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(24),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvSURF_B8G8R8UI_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_B8G8R8UI_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X16G16R16I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(48),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 16 | gcvCOMPONENT_DONTCARE }, { 16, 16 }, { 32, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 16 | gcvCOMPONENT_DONTCARE }, { 16, 16 }, { 32, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G16R16I_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_G16R16I_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B16G16R16I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(48),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 32, 16 }, { 16, 16 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 32, 16 }, { 16, 16 }, {0}, {0}}},
        gcvSURF_B16G16R16I_2_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_B16G16R16I_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X16G16R16UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(48),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 16 | gcvCOMPONENT_DONTCARE }, { 16, 16 }, { 32, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 16 | gcvCOMPONENT_DONTCARE }, { 16, 16 }, { 32, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G16R16UI_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_G16R16UI_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B16G16R16UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(48),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 32, 16 }, { 16, 16 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 32, 16 }, { 16, 16 }, {0}, {0}}},
        gcvSURF_B16G16R16UI_2_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_B16G16R16UI_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X32G32R32I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(96),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, 32 | gcvCOMPONENT_DONTCARE }, { 32, 32 }, { 64, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 32 | gcvCOMPONENT_DONTCARE }, { 32, 32 }, { 64, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G32R32I_2_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_G32R32I_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B32G32R32I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(96),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 64, 32 }, { 32, 32 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 64, 32 }, { 32, 32 }, {0}, {0}}},
        gcvSURF_B32G32R32I_3_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_B32G32R32I_3_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X32G32R32UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(96),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, 32 | gcvCOMPONENT_DONTCARE }, { 32, 32 }, { 64, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 32 | gcvCOMPONENT_DONTCARE }, { 32, 32 }, { 64, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G32R32UI_2_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_G32R32UI_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(B32G32R32UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(96),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 64, 32 }, { 32, 32 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 64, 32 }, { 32, 32 }, {0}, {0}}},
        gcvSURF_B32G32R32UI_3_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_B32G32R32UI_3_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X8B8G8R8I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 24, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvSURF_B8G8R8I_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_B8G8R8I_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A8B8G8R8I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 24, 8 }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvSURF_A8B8G8R8I_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8B8G8R8I_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X8B8G8R8UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 24, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvSURF_B8G8R8UI_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_B8G8R8UI_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A8B8G8R8UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 24, 8 }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvSURF_A8B8G8R8UI_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8B8G8R8UI_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X16B16G16R16I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_B16G16R16I_2_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_B16G16R16I_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A16B16G16R16I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_A16B16G16R16I_2_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A16B16G16R16I_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X16B16G16R16UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_X16B16G16R16UI_2_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X16B16G16R16UI_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A16B16G16R16UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_A16B16G16R16UI_2_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A16B16G16R16UI_2_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X32B32G32R32I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(128),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_X32B32G32R32I_3_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X32B32G32R32I_3_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A32B32G32R32I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(128),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_A32B32G32R32I_4_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A32B32G32R32I_4_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(X32B32G32R32UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(128),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_X32B32G32R32UI_3_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_X32B32G32R32UI_3_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A32B32G32R32UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(128),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_A32B32G32R32UI_4_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A32B32G32R32UI_4_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(A2B10G10R10UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 0, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 30, 2 }, { 0, 10 }, { 10, 10 }, { 20, 10 }, {0}, {0}}},
        {{{ 30, 2 }, { 0, 10 }, { 10, 10 }, { 20, 10 }, {0}, {0}}},
        gcvSURF_A2B10G10R10UI_1_A8R8G8B8, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A2B10G10R10UI_1_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    /*Multi-layer surface definition, we will move to a separate table later*/
    {
        gcmNameFormat(G32R32I_2_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        2, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G32R32I_2_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_G32R32I_2_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(G32R32I_1_G32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, gcvTRUE, gcvFALSE, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G32R32I_1_G32R32F, 0x15, baseComponents_rgba,
        gcvSURF_G32R32I_1_G32R32F, (0x0B << 12), baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(G32R32UI_2_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        2, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G32R32UI_2_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_G32R32UI_2_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(G32R32UI_1_G32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, gcvTRUE, gcvFALSE, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 32, 32 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G32R32UI_1_G32R32F, 0x15, baseComponents_rgba,
        gcvSURF_G32R32UI_1_G32R32F, (0x0B << 12), baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(X16B16G16R16I_2_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        2, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_X16B16G16R16I_2_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_X16B16G16R16I_2_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(X16B16G16R16I_1_G32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, gcvTRUE, gcvFALSE, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_X16B16G16R16I_1_G32R32F, 0x15, baseComponents_rgba,
        gcvSURF_X16B16G16R16I_1_G32R32F, (0x0B << 12), baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(A16B16G16R16I_2_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        2, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_A16B16G16R16I_2_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_A16B16G16R16I_2_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(A16B16G16R16I_1_G32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, gcvTRUE, gcvFALSE, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_A16B16G16R16I_1_G32R32F, 0x15, baseComponents_rgba,
        gcvSURF_A16B16G16R16I_1_G32R32F, (0x0B << 12), baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(X16B16G16R16UI_2_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        2, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_X16B16G16R16UI_2_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_X16B16G16R16UI_2_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(X16B16G16R16UI_1_G32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, gcvTRUE, gcvFALSE, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_X16B16G16R16UI_1_G32R32F, 0x15, baseComponents_rgba,
        gcvSURF_X16B16G16R16UI_1_G32R32F, (0x0B << 12), baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(A16B16G16R16UI_2_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        2, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_A16B16G16R16UI_2_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_A16B16G16R16UI_2_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(A16B16G16R16UI_1_G32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, gcvTRUE, gcvFALSE, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        {{{ 48, 16 }, { 0, 16 }, { 16, 16 }, { 32, 16 }, {0}, {0}}},
        gcvSURF_A16B16G16R16UI_1_G32R32F, 0x15, baseComponents_rgba,
        gcvSURF_A16B16G16R16UI_1_G32R32F, (0x0B << 12), baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(X32B32G32R32I_2_G32R32I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(128),
        2, 1, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_X32B32G32R32I_2_G32R32I, 0x15, baseComponents_rgba,
        gcvSURF_X32B32G32R32I_2_G32R32I, 0x0B << 12, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(A32B32G32R32I_2_G32R32I), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(128),
        2, 1, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_A32B32G32R32I_2_G32R32I, 0x15, baseComponents_rgba,
        gcvSURF_A32B32G32R32I_2_G32R32I, 0x0B << 12, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(A32B32G32R32I_2_G32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(128),
        2, gcvTRUE, gcvFALSE, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_A32B32G32R32I_2_G32R32F, 0x15, baseComponents_rgba,
        gcvSURF_A32B32G32R32I_2_G32R32F, (0x0B << 12), baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(X32B32G32R32I_3_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(96),
        3, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_X32B32G32R32I_3_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_X32B32G32R32I_3_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(A32B32G32R32I_4_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(128),
        4, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_A32B32G32R32I_4_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_A32B32G32R32I_4_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(X32B32G32R32UI_2_G32R32UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(128),
        2, 1, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_X32B32G32R32UI_2_G32R32UI, 0x15, baseComponents_rgba,
        gcvSURF_X32B32G32R32UI_2_G32R32UI, 0x0B << 12, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(A32B32G32R32UI_2_G32R32UI), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(128),
        2, 1, 0, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_A32B32G32R32UI_2_G32R32UI, 0x15, baseComponents_rgba,
        gcvSURF_A32B32G32R32UI_2_G32R32UI, 0x0B << 12, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(A32B32G32R32UI_2_G32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(128),
        2, gcvTRUE, gcvFALSE, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_A32B32G32R32UI_2_G32R32F, 0x15, baseComponents_rgba,
        gcvSURF_A32B32G32R32UI_2_G32R32F, (0x0B << 12), baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(X32B32G32R32UI_3_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(96),
        3, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_X32B32G32R32UI_3_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_X32B32G32R32UI_3_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(A32B32G32R32UI_4_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(128),
        4, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        {{{ 96, 32 }, { 0, 32 }, { 32, 32 }, { 64, 32 }, {0}, {0}}},
        gcvSURF_A32B32G32R32UI_4_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_A32B32G32R32UI_4_A8R8G8B8, 0x07, baseComponents_rgba, gcvTRUE
    },

    {
        gcmNameFormat(A2B10G10R10UI_1_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 30, 2 }, { 0, 10 }, { 10, 10 }, { 20, 10 }, {0}, {0}}},
        {{{ 30, 2 }, { 0, 10 }, { 10, 10 }, { 20, 10 }, {0}, {0}}},
        gcvSURF_A2B10G10R10UI_1_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_A2B10G10R10UI_1_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(A8B8G8R8I_1_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 24, 8 }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvSURF_A8B8G8R8I_1_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_A8B8G8R8I_1_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(A8B8G8R8UI_1_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 24, 8 }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 24, 8 }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvSURF_A8B8G8R8UI_1_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_A8B8G8R8UI_1_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(R8I_1_A4R4G4B4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 1, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_R8I_1_A4R4G4B4, 0x01, baseComponents_rgba,
        gcvSURF_R8I_1_A4R4G4B4, 0x05, baseComponents_rgba, gcvFALSE
    },


    {
        gcmNameFormat(R8UI_1_A4R4G4B4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 1, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_R8UI_1_A4R4G4B4, 0x01, baseComponents_rgba,
        gcvSURF_R8UI_1_A4R4G4B4, 0x05, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(R16I_1_A4R4G4B4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 1, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_R16I_1_A4R4G4B4, 0x01, baseComponents_rgba,
        gcvSURF_R16I_1_A4R4G4B4, 0x05, baseComponents_rgba, gcvFALSE
    },

     {
         gcmNameFormat(R16UI_1_A4R4G4B4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(16),
         1, 1, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
         {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 },
           { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
         {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 },
           { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
         gcvSURF_R16UI_1_A4R4G4B4, 0x01, baseComponents_rgba,
         gcvSURF_R16UI_1_A4R4G4B4, 0x05, baseComponents_rgba, gcvFALSE
     },

     {
         gcmNameFormat(R32I_1_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
         1, gcvTRUE, gcvFALSE, gcvFALSE, gcvENDIAN_NO_SWAP,
         {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 },
           { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
         {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 },
           { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
         gcvSURF_R32I_1_A8R8G8B8, 0x06, baseComponents_rgba,
         gcvSURF_R32I_1_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE
     },

     {
         gcmNameFormat(R32UI_1_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
         1, gcvTRUE, gcvFALSE, gcvFALSE, gcvENDIAN_NO_SWAP,
         {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 },
           { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
         {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 },
           { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
         gcvSURF_R32UI_1_A8R8G8B8, 0x06, baseComponents_rgba,
         gcvSURF_R32UI_1_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE
     },

     {
         gcmNameFormat(X8R8I_1_A4R4G4B4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(16),
         1, 1, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
         {{{ 8, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 },
           { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
         {{{ 8, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 },
           { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
         gcvSURF_X8R8I_1_A4R4G4B4, 0x01, baseComponents_rgba,
         gcvSURF_X8R8I_1_A4R4G4B4, 0x05, baseComponents_rgba, gcvFALSE
     },

     {
         gcmNameFormat(X8R8UI_1_A4R4G4B4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(16),
         1, 1, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
         {{{ 8, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 },
           { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
         {{{ 8, 8 | gcvCOMPONENT_DONTCARE }, { 0, 8 },
           { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
         gcvSURF_X8R8UI_1_A4R4G4B4, 0x01, baseComponents_rgba,
         gcvSURF_X8R8UI_1_A4R4G4B4, 0x05, baseComponents_rgba, gcvFALSE
     },

     {
         gcmNameFormat(G8R8I_1_A4R4G4B4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(16),
         1, gcvTRUE, gcvFALSE, gcvFALSE, gcvENDIAN_SWAP_WORD,
         {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
         {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
         gcvSURF_G8R8I_1_A4R4G4B4, 0x01, baseComponents_rgba,
         gcvSURF_G8R8I_1_A4R4G4B4, 0x05, baseComponents_rgba, gcvFALSE
     },

    {
        gcmNameFormat(G8R8UI_1_A4R4G4B4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, gcvTRUE, gcvFALSE, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G8R8UI_1_A4R4G4B4, 0x01, baseComponents_rgba,
        gcvSURF_G8R8UI_1_A4R4G4B4, 0x05, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(X16R16I_1_A4R4G4B4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 1, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 16, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 16, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X16R16I_1_A4R4G4B4, 0x01, baseComponents_rgba,
        gcvSURF_X16R16I_1_A4R4G4B4, 0x05, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(X16R16UI_1_A4R4G4B4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 1, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 16, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 16, 16 | gcvCOMPONENT_DONTCARE }, { 0, 16 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X16R16UI_1_A4R4G4B4, 0x01, baseComponents_rgba,
        gcvSURF_X16R16UI_1_A4R4G4B4, 0x05, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(G16R16I_1_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G16R16I_1_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_G16R16I_1_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(G16R16UI_1_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 16, 16 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_G16R16UI_1_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_G16R16UI_1_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(X32R32I_1_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 32, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 32, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X32R32I_1_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_X32R32I_1_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(X32R32UI_1_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 32, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 32, 32 | gcvCOMPONENT_DONTCARE }, { 0, 32 },
          { 0, gcvCOMPONENT_NOTPRESENT }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X32R32UI_1_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_X32R32UI_1_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(X8G8R8I_1_A4R4G4B4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(24),
        1, 1, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 8 | gcvCOMPONENT_DONTCARE }, { 8, 8 }, { 16, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 8 | gcvCOMPONENT_DONTCARE }, { 8, 8 }, { 16, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8G8R8I_1_A4R4G4B4, 0x01, baseComponents_rgba,
        gcvSURF_X8G8R8I_1_A4R4G4B4, 0x05, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(X8G8R8UI_1_A4R4G4B4), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(16),
        1, 1, 0, gcvFALSE, gcvENDIAN_SWAP_WORD,
        {{{ 0, 8 | gcvCOMPONENT_DONTCARE }, { 8, 8 }, { 16, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        {{{ 0, 8 | gcvCOMPONENT_DONTCARE }, { 8, 8 }, { 16, 8 }, { 0, gcvCOMPONENT_NOTPRESENT }, {0}, {0}}},
        gcvSURF_X8G8R8UI_1_A4R4G4B4, 0x01, baseComponents_rgba,
        gcvSURF_X8G8R8UI_1_A4R4G4B4, 0x05, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(B8G8R8I_1_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvSURF_B8G8R8I_1_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_B8G8R8I_1_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(B8G8R8UI_1_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(32),
        1, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 8 }, { 8, 8 }, { 16, 8 }, {0}, {0}}},
        gcvSURF_B8G8R8UI_1_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_B8G8R8UI_1_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(B16G16R16I_2_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        2, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 32, 16 }, { 16, 16 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 32, 16 }, { 16, 16 }, {0}, {0}}},
        gcvSURF_B16G16R16I_2_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_B16G16R16I_2_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(B16G16R16I_1_G32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, gcvTRUE, gcvFALSE, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 32, 16 }, { 16, 16 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 32, 16 }, { 16, 16 }, {0}, {0}}},
        gcvSURF_B16G16R16I_1_G32R32F, 0x15, baseComponents_rgba,
        gcvSURF_B16G16R16I_1_G32R32F, (0x0B << 12), baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(B16G16R16UI_2_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        2, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 32, 16 }, { 16, 16 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 32, 16 }, { 16, 16 }, {0}, {0}}},
        gcvSURF_B16G16R16UI_2_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_B16G16R16UI_2_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(B16G16R16UI_1_G32R32F), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(64),
        1, gcvTRUE, gcvFALSE, gcvFALSE, gcvENDIAN_SWAP_DWORD,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 32, 16 }, { 16, 16 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 16 }, { 32, 16 }, { 16, 16 }, {0}, {0}}},
        gcvSURF_B16G16R16UI_1_G32R32F, 0x15, baseComponents_rgba,
        gcvSURF_B16G16R16UI_1_G32R32F, (0x0B << 12), baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(B32G32R32I_3_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_SIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(96),
        3, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 64, 32 }, { 32, 32 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 64, 32 }, { 32, 32 }, {0}, {0}}},
        gcvSURF_B32G32R32I_3_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_B32G32R32I_3_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE
    },

    {
        gcmNameFormat(B32G32R32UI_3_A8R8G8B8), gcvFORMAT_CLASS_RGBA, gcvFORMAT_DATATYPE_UNSIGNED_INTEGER, gcmNON_COMPRESSED_BPP_ENTRY(96),
        3, 1, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 64, 32 }, { 32, 32 }, {0}, {0}}},
        {{{ 0, gcvCOMPONENT_NOTPRESENT }, { 0, 32 }, { 64, 32 }, { 32, 32 }, {0}, {0}}},
        gcvSURF_B32G32R32UI_3_A8R8G8B8, 0x06, baseComponents_rgba,
        gcvSURF_B32G32R32UI_3_A8R8G8B8, 0x07, baseComponents_rgba, gcvFALSE
    },
};

/* Format value range: 1600-1699
 * Class: ASTC formats
 * Component encoding: N/A */
static struct _gcsSURF_FORMAT_INFO formatASTC[] =
{
    {
        gcmNameFormat(ASTC4x4), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 0, 4, 4, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC5x4), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 0, 5, 4, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC5x5), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 0, 5, 5, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC6x5), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 0, 6, 5, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC6x6), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 0, 6, 6, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC8x5), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 0, 8, 5, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC8x6), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 0, 8, 6, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC8x8), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 0, 8, 8, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC10x5), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 0, 10, 5, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC10x6), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 0, 10, 6, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC10x8), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 0, 10, 8, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC10x10), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 0, 10, 10, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC12x10), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 0, 12, 10, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC12x12), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED, 0, 12, 12, 128,
        1, 0, 0, gcvFALSE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC4x4_SRGB), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_SRGB, 0, 4, 4, 128,
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC5x4_SRGB), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_SRGB, 0, 5, 4, 128,
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC5x5_SRGB), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_SRGB, 0, 5, 5, 128,
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC6x5_SRGB), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_SRGB, 0, 6, 5, 128,
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC6x6_SRGB), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_SRGB, 0, 6, 6, 128,
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC8x5_SRGB), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_SRGB, 0, 8, 5, 128,
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC8x6_SRGB), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_SRGB, 0, 8, 6, 128,
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC8x8_SRGB), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_SRGB, 0, 8, 8, 128,
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC10x5_SRGB), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_SRGB, 0, 10, 5, 128,
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC10x6_SRGB), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_SRGB, 0, 10, 6, 128,
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC10x8_SRGB), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_SRGB, 0, 10, 8, 128,
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC10x10_SRGB), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_SRGB, 0, 10, 10, 128,
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC12x10_SRGB), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_SRGB, 0, 12, 10, 128,
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    },

    {
        gcmNameFormat(ASTC12x12_SRGB), gcvFORMAT_CLASS_ASTC, gcvFORMAT_DATATYPE_SRGB, 0, 12, 12, 128,
        1, 0, 0, gcvTRUE, gcvENDIAN_NO_SWAP,
        gcmINVALID_PIXEL_FORMAT_CLASS, gcmINVALID_PIXEL_FORMAT_CLASS,
        gcvINVALID_RENDER_FORMAT, gcmINVALID_RENDER_FORMAT_ENTRY,
        gcvSURF_A8R8G8B8, gcmINVALID_TEXTURE_FORMAT_ENTRY
    }
};

/******************************************************************************\
********************************* Format Array *********************************
\******************************************************************************/

#define gcmDUMMY_FORMAT_ARRAY_ENTRY() \
    { gcvNULL, 0 }

struct gcsFORMAT_ARRAY
{
    struct _gcsSURF_FORMAT_INFO   * formats;
    gctUINT                         count;
};

static struct gcsFORMAT_ARRAY formatArray[] =
{
    /* 0-99 */
    gcmDUMMY_FORMAT_ARRAY_ENTRY(),

    /* 100-199 */
    { formatPalettized, gcmCOUNTOF(formatPalettized) },

    /* 200-299 */
    { formatRGB, gcmCOUNTOF(formatRGB) },

    /* 300-399 */
    { formatBGR, gcmCOUNTOF(formatBGR) },

    /* 400-499 */
    { formatCompressed, gcmCOUNTOF(formatCompressed) },

    /* 500-599 */
    { formatYUV, gcmCOUNTOF(formatYUV) },

    /* 600-699 */
    { formatDepth, gcmCOUNTOF(formatDepth) },

    /* 700-799 */
    { formatAlpha, gcmCOUNTOF(formatAlpha) },

    /* 800-899 */
    { formatLuminance, gcmCOUNTOF(formatLuminance) },

    /* 900-999 */
    { formatLuminanceAlpha, gcmCOUNTOF(formatLuminanceAlpha) },

    /* 1000-1099 */
    { formatBump, gcmCOUNTOF(formatBump) },

    /* 1100-1199 */
    { formatRGB2, gcmCOUNTOF(formatRGB2) },

    /* 1200-1299 */
    { formatFP, gcmCOUNTOF(formatFP) },

    /* 1300-1399 */
    gcmDUMMY_FORMAT_ARRAY_ENTRY(),

    /* 1400-1499 */
    { formatSRGB, gcmCOUNTOF(formatSRGB) },

    /* 1500-1599 */
    { formatINT, gcmCOUNTOF(formatINT) },

    /* 1600-1699 */
    { formatASTC, gcmCOUNTOF(formatASTC) },
};

#define gcmGET_SURF_FORMAT_INFO(Format) \
    &formatArray[((Format) / 100)].formats[(Format) % 100]


gceSTATUS
gcoHARDWARE_InitializeFormatArrayTable(
    IN gcoHARDWARE Hardware
    )
{
    gcsSURF_FORMAT_INFO * info;

    gcmHEADER();

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Change all endian mode to NO_SWAP for little endian mode */
    if (!Hardware->bigEndian)
    {
        gctSIZE_T i;

        for (i = 0; i < gcmCOUNTOF(formatArray); ++i)
        {
            gctUINT j;
            struct gcsFORMAT_ARRAY *pFmtArray = &formatArray[i];

            for (j = 0; j < pFmtArray->count; ++j)
            {
                pFmtArray->formats[j].endian = gcvENDIAN_NO_SWAP;
            }
        }
    }

#if gcdENABLE_3D
    if (Hardware->patchID == gcvPATCH_GTFES30)
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X8R8G8B8);
        info->txIntFilter = gcvFALSE;
    }
#endif

    /* gcvSURF_X8R8G8B8 */
    if ((Hardware->config->chipModel == gcv4000) &&
        (Hardware->config->chipRevision == 0x5208))
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X8R8G8B8);
        info->txFormat  = 0x07;
        info->txSwizzle = baseComponents_rgb1;
    }

    /* gcvSURF_A2B10G10R10 */
    if (Hardware->features[gcvFEATURE_HALTI0]
        || Hardware->features[gcvFEATURE_HALTI1]
        || Hardware->features[gcvFEATURE_HALTI2])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A2B10G10R10);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_A2B10G10R10;
        info->renderFormat        = 0x16;
        info->txFormat            = 0x0C << 12;
        info->pixelSwizzle        = baseComponents_rgba;
        info->txSwizzle           = baseComponents_rgba;
    }

    /* gcvSURF_X2B10G10R10 */
    if (Hardware->features[gcvFEATURE_HALF_FLOAT_PIPE])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X2B10G10R10);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X2B10G10R10;
        info->renderFormat        = 0x16;
        info->txFormat            = 0x0C << 12;
        info->pixelSwizzle        = baseComponents_rgb1;
        info->txSwizzle           = baseComponents_rgb1;
    }

    /* SNORM TEXTURE formats. */
    if (Hardware->features[gcvFEATURE_TX_SNORM_SUPPORT])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_R8_SNORM);
        info->closestTXFormat     = gcvSURF_R8_SNORM;
        info->txFormat            = 0x0E << 12;
        info->txSwizzle           = baseComponents_r001;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_G8R8_SNORM);
        info->closestTXFormat     = gcvSURF_G8R8_SNORM;
        info->txFormat            = 0x0F << 12;
        info->txSwizzle           = baseComponents_rg01;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_B8G8R8_SNORM);
        info->closestTXFormat     = gcvSURF_X8B8G8R8_SNORM;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X8B8G8R8_SNORM);
        info->closestTXFormat     = gcvSURF_X8B8G8R8_SNORM;
        info->txFormat            = 0x10 << 12;
        info->txSwizzle           = baseComponents_rgb1;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A8B8G8R8_SNORM);
        info->closestTXFormat     = gcvSURF_A8B8G8R8_SNORM;
        info->txFormat            = 0x11 << 12;
        info->txSwizzle           = baseComponents_rgba;
    }

    /* YUV420 */
    if (Hardware->features[gcvFEATURE_TEXTURE_YUV_ASSEMBLER])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_YV12);
        info->closestTXFormat     = gcvSURF_YV12;
        info->txFormat            = 0x13 << 12;
        info->txSwizzle           = baseComponents_rgb1;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_I420);
        info->closestTXFormat     = gcvSURF_I420;
        info->txFormat            = 0x13 << 12;
        info->txSwizzle           = baseComponents_rgb1;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_NV12);
        info->closestTXFormat     = gcvSURF_NV12;
        info->txFormat            = 0x13 << 12;
        info->txSwizzle           = baseComponents_rgb1;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_NV21);
        info->closestTXFormat     = gcvSURF_NV21;
        info->txFormat            = 0x13 << 12;
        info->txSwizzle           = baseComponents_rgb1;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_NV16);
        info->closestTXFormat     = gcvSURF_NV16;
        info->txFormat            = 0x13 << 12;
        info->txSwizzle           = baseComponents_rgb1;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_NV61);
        info->closestTXFormat     = gcvSURF_NV61;
        info->txFormat            = 0x13 << 12;
        info->txSwizzle           = baseComponents_rgb1;

        if (Hardware->features[gcvFEATURE_TX_YUV_ASSEMBLER_10BIT])
        {
            info = gcmGET_SURF_FORMAT_INFO(gcvSURF_YUV420_TILE_ST);
            info->closestTXFormat = gcvSURF_YUV420_TILE_ST;
            info->txFormat        = 0x13 << 12;
            info->txSwizzle       = baseComponents_rgb1;

            info = gcmGET_SURF_FORMAT_INFO(gcvSURF_YUV420_10_ST);
            info->closestTXFormat = gcvSURF_YUV420_10_ST;
            info->txFormat        = 0x13 << 12;
            info->txSwizzle       = baseComponents_rgb1;

            info = gcmGET_SURF_FORMAT_INFO(gcvSURF_YUV420_TILE_10_ST);
            info->closestTXFormat = gcvSURF_YUV420_TILE_10_ST;
            info->txFormat        = 0x13 << 12;
            info->txSwizzle       = baseComponents_rgb1;
        }
    }
    else if (Hardware->features[gcvFEATURE_YUV420_TILER])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_YV12);
        info->closestTXFormat     = gcvSURF_YUY2;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_I420);
        info->closestTXFormat     = gcvSURF_YUY2;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_NV12);
        info->closestTXFormat     = gcvSURF_YUY2;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_NV21);
        info->closestTXFormat     = gcvSURF_YUY2;
    }

    /* DXT textures. */
    if (Hardware->features[gcvFEATURE_DXT_TEXTURE_COMPRESSION])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_DXT1);
        info->closestTXFormat     = gcvSURF_DXT1;
        info->txFormat            = 0x13;
        info->txIntFilter         = gcvTRUE;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_DXT2);
        info->closestTXFormat     = gcvSURF_DXT2;
        info->txFormat            = 0x14;
        info->txIntFilter         = gcvTRUE;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_DXT3);
        info->closestTXFormat     = gcvSURF_DXT3;
        info->txFormat            = 0x14;
        info->txIntFilter         = gcvTRUE;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_DXT4);
        info->closestTXFormat     = gcvSURF_DXT4;
        info->txFormat            = 0x15;
        info->txIntFilter         = gcvTRUE;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_DXT5);
        info->closestTXFormat     = gcvSURF_DXT5;
        info->txFormat            = 0x15;
        info->txIntFilter         = gcvTRUE;
    }

    if (Hardware->features[gcvFEATURE_HALF_FLOAT_PIPE])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_R16F);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_R16F;
        info->txFormat            = 0x07 << 12;
        info->renderFormat        = 0x11;
        info->txIntFilter         = gcvFALSE;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X16R16F);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X16R16F;
        info->txFormat            = 0x08 << 12;
        info->txSwizzle           = baseComponents_r001;
        info->renderFormat        = 0x12;
        info->txIntFilter         = gcvFALSE;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_G16R16F);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_G16R16F;
        info->txFormat            = 0x08 << 12;
        info->renderFormat        = 0x12;
        info->txIntFilter         = gcvFALSE;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X16G16R16F);
        /* 48 bit, not natively readable. */
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X16B16G16R16F;
        info->txIntFilter         = gcvFALSE;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_B16G16R16F);
        /* 48 bit, not natively readable. */
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X16B16G16R16F;
        info->txIntFilter         = gcvFALSE;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X16B16G16R16F);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X16B16G16R16F;
        info->txFormat            = 0x09 << 12;
        info->renderFormat        = 0x13;
        info->txSwizzle           = baseComponents_rgb1;
        info->txIntFilter         = gcvFALSE;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A16B16G16R16F);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_A16B16G16R16F;
        info->txFormat            = 0x09 << 12;
        info->renderFormat        = 0x13;
        info->txIntFilter         = gcvFALSE;
        info->txSwizzle           = baseComponents_rgba;

        /* Before 32bpp TX swizzle support,
        ** 32bit per-channel occupy two U16 channels, so rg for R, ba for G
        ** For B, A channel, they are always default value no matter its swizzle setting
        */
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_R32F);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_R32F;
        info->txFormat            = 0x0A << 12;
        info->renderFormat        = 0x14;
        info->txIntFilter         = gcvFALSE;
        if (Hardware->features[gcvFEATURE_32BPP_COMPONENT_TEXTURE_CHANNEL_SWIZZLE])
        {
            info->txSwizzle           = baseComponents_rgba;
        }
        else
        {
            info->txSwizzle           = baseComponents_rg00;
        }

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X32R32F);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X32R32F;
        info->txFormat            = 0x0B << 12;
        info->renderFormat        = 0x15;
        info->txSwizzle           = baseComponents_rg00;
        if (Hardware->features[gcvFEATURE_32BPP_COMPONENT_TEXTURE_CHANNEL_SWIZZLE])
        {
            info->txSwizzle           = baseComponents_rgba;
        }
        else
        {
            info->txSwizzle           = baseComponents_rg00;
        }

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_G32R32F);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_G32R32F;
        info->txFormat            = 0x0B << 12;
        info->renderFormat        = 0x15;
        info->txSwizzle           = baseComponents_rgba;
        info->txIntFilter         = gcvFALSE;

        /* Multi-layer formats. */
        /* Don't support gcvSURF_X32G32R32F. Doesn't map to GL formats. */
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_B32G32R32F);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X32B32G32R32F_2_G32R32F;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X32B32G32R32F);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X32B32G32R32F_2_G32R32F;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A32B32G32R32F);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_A32B32G32R32F_2_G32R32F;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A16F);
        info->closestTXFormat     = gcvSURF_A16F;
        info->txFormat            = 0x07 << 12;
        info->txIntFilter         = gcvFALSE;
        info->txSwizzle           = baseComponents_000r;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_L16F);
        info->closestTXFormat     = gcvSURF_L16F;
        info->txFormat            = 0x07 << 12;
        info->txIntFilter         = gcvFALSE;
        info->txSwizzle           = baseComponents_rrr1;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A16L16F);
        info->closestTXFormat     = gcvSURF_A16L16F;
        info->txFormat            = 0x08 << 12;
        info->txIntFilter         = gcvFALSE;
        info->txSwizzle           = baseComponents_rrrg;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A16R16F);
        info->closestTXFormat     = gcvSURF_A16R16F;
        info->txFormat            = 0x08 << 12;
        info->txIntFilter         = gcvFALSE;
        info->txSwizzle           = baseComponents_r00g;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A32F);
        if(Hardware->features[gcvFEATURE_32BPP_COMPONENT_TEXTURE_CHANNEL_SWIZZLE])
        {
            info->closestTXFormat     = gcvSURF_A32F;
        }
        else
        {
            info->closestTXFormat     = gcvSURF_A32F_1_R32F;
        }
        info->txFormat            = 0x0A << 12;
        info->txIntFilter         = gcvFALSE;
        info->txSwizzle           = baseComponents_000r;

        /*FIXME: below, the swizzle can't work for L32F, A32L32F, A32R32F. Need support in HW*/
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_L32F);
        if(Hardware->features[gcvFEATURE_32BPP_COMPONENT_TEXTURE_CHANNEL_SWIZZLE])
        {
            info->closestTXFormat = gcvSURF_L32F;
        }
        else
        {
            info->closestTXFormat = gcvSURF_L32F_1_R32F;
        }

        info->txFormat            = 0x0A << 12;
        info->txIntFilter         = gcvFALSE;
        info->txSwizzle           = baseComponents_rrr1;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A32L32F);

        if(Hardware->features[gcvFEATURE_32BPP_COMPONENT_TEXTURE_CHANNEL_SWIZZLE])
        {
            info->closestTXFormat     = gcvSURF_A32L32F;
        }
        else
        {
            info->closestTXFormat     = gcvSURF_A32L32F_1_G32R32F;
        }

        info->txFormat            = 0x0B << 12;
        info->txIntFilter         = gcvFALSE;
        info->txSwizzle           = baseComponents_rrrg;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A32R32F);
        info->closestTXFormat     = gcvSURF_A32R32F;
        info->txFormat            = 0x0B << 12;
        info->txIntFilter         = gcvFALSE;
        info->txSwizzle           = baseComponents_r00g;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_D32F);
        info->closestTXFormat     = gcvSURF_S8D32F_1_G32R32F;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_S8D32F);
        info->closestTXFormat     = gcvSURF_S8D32F_1_G32R32F;
    }
    else
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_B32G32R32F);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X32B32G32R32F_4_A8R8G8B8;
    }

    /* Integer formats. */
    if (Hardware->features[gcvFEATURE_HALTI2])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_R8I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_R8I;
        info->renderFormat        = 0x17;
        info->txFormat            = 0x15 << 12;
        info->pixelSwizzle        = baseComponents_r001;
        info->txSwizzle           = baseComponents_r001;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_R8UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_R8UI;
        info->renderFormat        = 0x17;
        info->txFormat            = 0x15 << 12;
        info->pixelSwizzle        = baseComponents_r001;
        info->txSwizzle           = baseComponents_r001;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_R16I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_R16I;
        info->renderFormat        = 0x1A;
        info->txFormat            = 0x18 << 12;
        info->pixelSwizzle        = baseComponents_r001;
        info->txSwizzle           = baseComponents_r001;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_R16UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_R16UI;
        info->renderFormat        = 0x1A;
        info->txFormat            = 0x18 << 12;
        info->pixelSwizzle        = baseComponents_r001;
        info->txSwizzle           = baseComponents_r001;

        /* Before 32bpp TX swizzle support,
        ** 32bit per-channel occupy two U16 channels, so rg for R, ba for G
        ** For B, A channel, they are always default value no matter its swizzle setting
        */
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_R32I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_R32I;
        info->renderFormat        = 0x14;
        info->txFormat            = 0x0A << 12;
        info->pixelSwizzle        = baseComponents_rgba;
        if (Hardware->features[gcvFEATURE_32BPP_COMPONENT_TEXTURE_CHANNEL_SWIZZLE])
        {
            info->txSwizzle           = baseComponents_rgba;
        }
        else
        {
            info->txSwizzle           = baseComponents_rg00;
        }

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_R32UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_R32UI;
        info->renderFormat        = 0x14;
        info->txFormat            = 0x0A << 12;
        info->pixelSwizzle        = baseComponents_rgba;
        if (Hardware->features[gcvFEATURE_32BPP_COMPONENT_TEXTURE_CHANNEL_SWIZZLE])
        {
            info->txSwizzle           = baseComponents_rgba;
        }
        else
        {
            info->txSwizzle           = baseComponents_rg00;
        }

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X8R8I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X8R8I;
        info->renderFormat        = 0x18;
        info->txFormat            = 0x16 << 12;
        info->pixelSwizzle        = baseComponents_r001;
        info->txSwizzle           = baseComponents_r001;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_G8R8I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_G8R8I;
        info->renderFormat        = 0x18;
        info->txFormat            = 0x16 << 12;
        info->pixelSwizzle        = baseComponents_rg01;
        info->txSwizzle           = baseComponents_rg01;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X8R8UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X8R8UI;
        info->renderFormat        = 0x18;
        info->txFormat            = 0x16 << 12;
        info->pixelSwizzle        = baseComponents_r001;
        info->txSwizzle           = baseComponents_r001;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_G8R8UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_G8R8UI;
        info->renderFormat        = 0x18;
        info->txFormat            = 0x16 << 12;
        info->pixelSwizzle        = baseComponents_rg01;
        info->txSwizzle           = baseComponents_rg01;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X16R16I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X16R16I;
        info->renderFormat        = 0x1B;
        info->txFormat            = 0x19 << 12;
        info->pixelSwizzle        = baseComponents_r001;
        info->txSwizzle           = baseComponents_r001;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_G16R16I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_G16R16I;
        info->renderFormat        = 0x1B;
        info->txFormat            = 0x19 << 12;
        info->pixelSwizzle        = baseComponents_rg01;
        info->txSwizzle           = baseComponents_rg01;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X16R16UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X16R16UI;
        info->renderFormat        = 0x1B;
        info->txFormat            = 0x19 << 12;
        info->pixelSwizzle        = baseComponents_r001;
        info->txSwizzle           = baseComponents_r001;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_G16R16UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_G16R16UI;
        info->renderFormat        = 0x1B;
        info->txFormat            = 0x19 << 12;
        info->pixelSwizzle        = baseComponents_rg01;
        info->txSwizzle           = baseComponents_rg01;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_G32R32I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_G32R32I;
        info->renderFormat        = 0x15;
        info->txFormat            = 0x0B << 12;
        info->pixelSwizzle        = baseComponents_rgba;
        info->txSwizzle           = baseComponents_rgba;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_G32R32UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_G32R32UI;
        info->renderFormat        = 0x15;
        info->txFormat            = 0x0B << 12;
        info->pixelSwizzle        = baseComponents_rgba;
        info->txSwizzle           = baseComponents_rgba;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X8G8R8I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X8B8G8R8I;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_B8G8R8I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X8B8G8R8I;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X8G8R8UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X8B8G8R8UI;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_B8G8R8UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X8B8G8R8UI;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X16G16R16I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X16B16G16R16I;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_B16G16R16I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X16B16G16R16I;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X16G16R16UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X16B16G16R16UI;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_B16G16R16UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X16B16G16R16UI;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X32G32R32I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X32B32G32R32I_2_G32R32I;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_B32G32R32I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X32B32G32R32I_2_G32R32I;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X32G32R32UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X32B32G32R32UI_2_G32R32UI;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_B32G32R32UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X32B32G32R32UI_2_G32R32UI;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X8B8G8R8I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X8B8G8R8I;
        info->renderFormat        = 0x19;
        info->txFormat            = 0x17 << 12;
        info->pixelSwizzle        = baseComponents_rgb1;
        info->txSwizzle           = baseComponents_rgb1;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A8B8G8R8I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_A8B8G8R8I;
        info->renderFormat        = 0x19;
        info->txFormat            = 0x17 << 12;
        info->pixelSwizzle        = baseComponents_rgba;
        info->txSwizzle           = baseComponents_rgba;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X8B8G8R8UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X8B8G8R8UI;
        info->renderFormat        = 0x19;
        info->txFormat            = 0x17 << 12;
        info->pixelSwizzle        = baseComponents_rgb1;
        info->txSwizzle           = baseComponents_rgb1;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A8B8G8R8UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_A8B8G8R8UI;
        info->renderFormat        = 0x19;
        info->txFormat            = 0x17 << 12;
        info->pixelSwizzle        = baseComponents_rgba;
        info->txSwizzle           = baseComponents_rgba;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X16B16G16R16I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X16B16G16R16I;
        info->renderFormat        = 0x1C;
        info->txFormat            = 0x1A << 12;
        info->pixelSwizzle        = baseComponents_rgb1;
        info->txSwizzle           = baseComponents_rgb1;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A16B16G16R16I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_A16B16G16R16I;
        info->renderFormat        = 0x1C;
        info->txFormat            = 0x1A << 12;
        info->pixelSwizzle        = baseComponents_rgba;
        info->txSwizzle           = baseComponents_rgba;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X16B16G16R16UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X16B16G16R16UI;
        info->renderFormat        = 0x1C;
        info->txFormat            = 0x1A << 12;
        info->pixelSwizzle        = baseComponents_rgb1;
        info->txSwizzle           = baseComponents_rgb1;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A16B16G16R16UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_A16B16G16R16UI;
        info->renderFormat        = 0x1C;
        info->txFormat            = 0x1A << 12;
        info->pixelSwizzle        = baseComponents_rgba;
        info->txSwizzle           = baseComponents_rgba;

        /* Cannot have only one fix */
        gcmASSERT(!(Hardware->features[gcvFEATURE_PE_RGBA16I_FIX] ^ Hardware->features[gcvFEATURE_32F_COLORMASK_FIX]));

        if (!Hardware->features[gcvFEATURE_PE_RGBA16I_FIX])
        {
            info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X16B16G16R16I);
            info->closestRenderFormat = gcvSURF_X16B16G16R16I_1_G32R32F;
            info->renderFormat = gcvINVALID_RENDER_FORMAT;

            info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A16B16G16R16I);
            info->closestRenderFormat = gcvSURF_A16B16G16R16I_1_G32R32F;
            info->renderFormat        = gcvINVALID_RENDER_FORMAT;

            info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X16B16G16R16UI);
            info->closestRenderFormat = gcvSURF_X16B16G16R16UI_1_G32R32F;
            info->renderFormat        = gcvINVALID_RENDER_FORMAT;

            info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A16B16G16R16UI);
            info->closestRenderFormat = gcvSURF_A16B16G16R16UI_1_G32R32F;
            info->renderFormat        = gcvINVALID_RENDER_FORMAT;
        }

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X32B32G32R32I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X32B32G32R32I_2_G32R32I;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A32B32G32R32I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_A32B32G32R32I_2_G32R32I;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X32B32G32R32UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X32B32G32R32UI_2_G32R32UI;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A32B32G32R32UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_A32B32G32R32UI_2_G32R32UI;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A2B10G10R10UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_A2B10G10R10UI;
        info->renderFormat        = 0x1E;
        info->txFormat            = 0x1C << 12;
        info->pixelSwizzle        = baseComponents_rgba;
        info->txSwizzle           = baseComponents_rgba;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_B10G11R11F);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_B10G11R11F;
        info->txFormat            = 0x1B << 12;
        info->renderFormat        = 0x1D;
        info->txIntFilter         = gcvFALSE;
        info->txSwizzle           = baseComponents_rgb1;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_E5B9G9R9);
        info->closestTXFormat     = gcvSURF_E5B9G9R9;
        info->txFormat            = 0x1D;
        info->txIntFilter         = gcvFALSE;
    }

    if (Hardware->config->chipModel == gcv900 && Hardware->config->chipRevision == 0x5250)
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_B10G11R11F);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_B10G11R11F;
        info->txFormat            = 0x1B << 12;
        info->renderFormat        = 0x1D;
        info->txIntFilter         = gcvFALSE;
        info->txSwizzle           = baseComponents_rgb1;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_E5B9G9R9);
        info->closestTXFormat     = gcvSURF_E5B9G9R9;
        info->txFormat            = 0x1D;
        info->txIntFilter         = gcvFALSE;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_G32R32I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_G32R32I_1_G32R32F;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_G32R32UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_G32R32UI_1_G32R32F;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_B16G16R16I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_B16G16R16I_1_G32R32F;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_B16G16R16UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_B16G16R16UI_1_G32R32F;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X16B16G16R16I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X16B16G16R16I_1_G32R32F;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X16B16G16R16UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_X16B16G16R16UI_1_G32R32F;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A16B16G16R16I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_A16B16G16R16I_1_G32R32F;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A16B16G16R16UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_A16B16G16R16UI_1_G32R32F;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A32B32G32R32I);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_A32B32G32R32I_2_G32R32F;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A32B32G32R32UI);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_A32B32G32R32UI_2_G32R32F;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_B10G11R11F);
        info->closestRenderFormat =
        info->closestTXFormat     = gcvSURF_B10G11R11F;
        info->txFormat            = 0x1B << 12;
        info->renderFormat        = 0x1D;
        info->txIntFilter         = gcvFALSE;
        info->txSwizzle           = baseComponents_rgb1;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_E5B9G9R9);
        info->closestTXFormat     = gcvSURF_E5B9G9R9;
        info->txFormat            = 0x1D;
        info->txIntFilter         = gcvFALSE;
    }

    /* ETC textures. */
    if (Hardware->features[gcvFEATURE_ETC1_TEXTURE_COMPRESSION])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ETC1);
        info->closestTXFormat     = gcvSURF_ETC1;
        info->txFormat            = 0x1E;
        info->txIntFilter         = gcvTRUE;
    }
    else if (Hardware->features[gcvFEATURE_TEX_ETC2])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ETC1);
        info->closestTXFormat     = gcvSURF_ETC1;
        info->txFormat            = 0x00 << 12;
        info->txIntFilter         = gcvTRUE;
    }

    /* ETC2 textures. */
    if (Hardware->features[gcvFEATURE_HALTI0])
    {
        /* No 11 bit support. */
        if ((Hardware->config->chipModel == gcv880 && Hardware->config->chipRevision == 0x5106) ||
            (Hardware->config->chipModel == gcv880 && Hardware->config->chipRevision == 0x5121) ||
            (Hardware->config->chipModel == gcv880 && Hardware->config->chipRevision == 0x5122) ||
            (Hardware->config->chipModel == gcv880 && Hardware->config->chipRevision == 0x5124) ||
            ((Hardware->config->chipModel == gcv2000 || Hardware->config->chipModel == gcv2100) && Hardware->config->chipRevision == 0x5108))
        {
            info = gcmGET_SURF_FORMAT_INFO(gcvSURF_R11_EAC);
            info->closestTXFormat     = gcvSURF_R16F_1_A4R4G4B4;
            info->txFormat            = 0x03 << 12;

            info = gcmGET_SURF_FORMAT_INFO(gcvSURF_SIGNED_R11_EAC);
            info->closestTXFormat     = gcvSURF_R16F_1_A4R4G4B4;

            info = gcmGET_SURF_FORMAT_INFO(gcvSURF_RG11_EAC);
            info->closestTXFormat     = gcvSURF_G16R16F_1_A8R8G8B8;

            info = gcmGET_SURF_FORMAT_INFO(gcvSURF_SIGNED_RG11_EAC);
            info->closestTXFormat     = gcvSURF_G16R16F_1_A8R8G8B8;
        }
        else
        {
            info = gcmGET_SURF_FORMAT_INFO(gcvSURF_R11_EAC);
            info->closestTXFormat     = gcvSURF_R11_EAC;
            info->txFormat            = 0x03 << 12;

            info = gcmGET_SURF_FORMAT_INFO(gcvSURF_SIGNED_R11_EAC);
            info->closestTXFormat     = gcvSURF_SIGNED_R11_EAC;
            info->txFormat            = 0x0D << 12;

            info = gcmGET_SURF_FORMAT_INFO(gcvSURF_RG11_EAC);
            info->closestTXFormat     = gcvSURF_RG11_EAC;
            info->txFormat            = 0x04 << 12;

            info = gcmGET_SURF_FORMAT_INFO(gcvSURF_SIGNED_RG11_EAC);
            info->closestTXFormat     = gcvSURF_SIGNED_RG11_EAC;
            info->txFormat            = 0x05 << 12;
        }

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_RGB8_ETC2);
        info->closestTXFormat     = gcvSURF_RGB8_ETC2;
        info->txFormat            = 0x00 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_SRGB8_ETC2);
        info->closestTXFormat     = gcvSURF_SRGB8_ETC2;
        info->txFormat            = (0x00 << 12) | (1 << 18);

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_RGB8_PUNCHTHROUGH_ALPHA1_ETC2);
        info->closestTXFormat     = gcvSURF_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
        info->txFormat            = 0x01 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2);
        info->closestTXFormat     = gcvSURF_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2;
        info->txFormat            = (0x01 << 12) | (1 << 18);

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_RGBA8_ETC2_EAC);
        info->closestTXFormat     = gcvSURF_RGBA8_ETC2_EAC;
        info->txFormat            = 0x02 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_SRGB8_ALPHA8_ETC2_EAC);
        info->closestTXFormat     = gcvSURF_SRGB8_ALPHA8_ETC2_EAC;
        info->txFormat            = (0x02 << 12) | (1 << 18);
    }

    /* sRGB tx formats native support */
    if (Hardware->features[gcvFEATURE_HALTI0])
    {
        /* SRGB8 is texture only format. */
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_SBGR8);
        info->closestTXFormat     = gcvSURF_X8_SBGR8;

        /* SRGB8_X8 is closest texture format for sRGB8. */
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X8_SBGR8);
        info->closestTXFormat     = gcvSURF_X8_SBGR8;
        info->txFormat            = (0x0A) | (1 << 18);
        info->txSwizzle           = baseComponents_rgb1;

        /* SRGB8_A8 is texture format.*/
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A8_SBGR8);
        info->closestTXFormat     = gcvSURF_A8_SBGR8;
        info->txFormat            = (0x09) | (1 << 18);
        info->txSwizzle           = baseComponents_rgba;
    }

    if (Hardware->features[gcvFEATURE_SRGB_RT_SUPPORT])
    {
        /* SRGB8 is texture only format. */
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_SBGR8);
        info->closestRenderFormat = gcvSURF_X8_SRGB8;
        info->closestTXFormat     = gcvSURF_X8_SRGB8;

        /* X8_SRGB8 is the closest texture format for X8_sBGR8. */
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X8_SBGR8);
        info->closestRenderFormat = gcvSURF_X8_SRGB8;
        info->closestTXFormat     = gcvSURF_X8_SRGB8;

        /* SRGB8_A8 is texture format.*/
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A8_SBGR8);
        info->closestRenderFormat = gcvSURF_A8_SRGB8;
        info->closestTXFormat     = gcvSURF_A8_SRGB8;
    }
    else if (Hardware->features[gcvFEATURE_HALF_FLOAT_PIPE])
    {
        /* SRGB8_X8 is closest texture format for sRGB8. */
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X8_SBGR8);
        info->closestRenderFormat = gcvSURF_A16B16G16R16F;

        /* SRGB8_A8 is texture format.*/
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A8_SBGR8);
        info->closestRenderFormat = gcvSURF_A16B16G16R16F;
    }

    /* R/RG/RA formats. */
    if (Hardware->features[gcvFEATURE_HALTI0])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_G8R8);
        info->closestTXFormat     = gcvSURF_G8R8;
        info->txFormat            = 0x06 << 12;
        info->txSwizzle           = baseComponents_rg01;
        info->txIntFilter         = gcvTRUE;
        if (Hardware->features[gcvFEATURE_HALTI2])
        {
            info->closestRenderFormat = gcvSURF_G8R8;
            info->renderFormat        = 0x1F;
            info->pixelSwizzle        = baseComponents_rgba;

            info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A8);
            info->closestRenderFormat = gcvSURF_A8;
            info->renderFormat = 0x10;
            info->pixelSwizzle = baseComponents_rgba;
        }
    }
    else if ((Hardware->config->chipModel == gcv1000) &&
             (Hardware->config->chipRevision >= 0x5039))
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_G8R8);

        info->closestTXFormat     = gcvSURF_G8R8;
        info->txFormat            = 0x06 << 12;
        info->txSwizzle           = baseComponents_rg01;
        info->txIntFilter         = gcvTRUE;

        info->closestRenderFormat = gcvSURF_G8R8;
        info->renderFormat        = 0x1F;
        info->pixelSwizzle        = baseComponents_rgba;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A8);
        info->closestRenderFormat = gcvSURF_A8;
        info->renderFormat = 0x10;
        info->pixelSwizzle = baseComponents_rgba;
    }

    /* ASTC textures */
    if (Hardware->features[gcvFEATURE_TEXTURE_ASTC])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC4x4);
        info->closestTXFormat     = gcvSURF_ASTC4x4;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC5x4);
        info->closestTXFormat     = gcvSURF_ASTC5x4;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC5x5);
        info->closestTXFormat     = gcvSURF_ASTC5x5;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC6x5);
        info->closestTXFormat     = gcvSURF_ASTC6x5;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC6x6);
        info->closestTXFormat     = gcvSURF_ASTC6x6;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC8x5);
        info->closestTXFormat     = gcvSURF_ASTC8x5;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC8x6);
        info->closestTXFormat     = gcvSURF_ASTC8x6;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC8x8);
        info->closestTXFormat     = gcvSURF_ASTC8x8;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC10x5);
        info->closestTXFormat     = gcvSURF_ASTC10x5;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC10x6);
        info->closestTXFormat     = gcvSURF_ASTC10x6;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC10x8);
        info->closestTXFormat     = gcvSURF_ASTC10x8;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC10x10);
        info->closestTXFormat     = gcvSURF_ASTC10x10;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC12x10);
        info->closestTXFormat     = gcvSURF_ASTC12x10;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC12x12);
        info->closestTXFormat     = gcvSURF_ASTC12x12;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC4x4_SRGB);
        info->closestTXFormat     = gcvSURF_ASTC4x4_SRGB;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC5x4_SRGB);
        info->closestTXFormat     = gcvSURF_ASTC5x4_SRGB;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC5x5_SRGB);
        info->closestTXFormat     = gcvSURF_ASTC5x5_SRGB;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC6x5_SRGB);
        info->closestTXFormat     = gcvSURF_ASTC6x5_SRGB;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC6x6_SRGB);
        info->closestTXFormat     = gcvSURF_ASTC6x6_SRGB;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC8x5_SRGB);
        info->closestTXFormat     = gcvSURF_ASTC8x5_SRGB;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC8x6_SRGB);
        info->closestTXFormat     = gcvSURF_ASTC8x6_SRGB;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC8x8_SRGB);
        info->closestTXFormat     = gcvSURF_ASTC8x8_SRGB;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC10x5_SRGB);
        info->closestTXFormat     = gcvSURF_ASTC10x5_SRGB;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC10x6_SRGB);
        info->closestTXFormat     = gcvSURF_ASTC10x6_SRGB;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC10x8_SRGB);
        info->closestTXFormat     = gcvSURF_ASTC10x8_SRGB;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC10x10_SRGB);
        info->closestTXFormat     = gcvSURF_ASTC10x10_SRGB;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC12x10_SRGB);
        info->closestTXFormat     = gcvSURF_ASTC12x10_SRGB;
        info->txFormat            = 0x14 << 12;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_ASTC12x12_SRGB);
        info->closestTXFormat     = gcvSURF_ASTC12x12_SRGB;
        info->txFormat            = 0x14 << 12;
    }

    if (Hardware->features[gcvFEATURE_STENCIL_TEXTURE])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_S8);
        info->closestTXFormat     = gcvSURF_S8;
        info->txFormat            = 0x15 << 12;
        info->txSwizzle           = baseComponents_r001;
    }

    if (Hardware->features[gcvFEATURE_S8_ONLY_RENDERING])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_S8);
        info->closestRenderFormat = gcvSURF_S8;
        info->renderFormat        = 0x17;
        info->pixelSwizzle        = baseComponents_r001;
    }

    if (Hardware->features[gcvFEATURE_HALTI2])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X24S8);
        info->closestTXFormat = gcvSURF_X24S8;
        info->txFormat        = 0x17 << 12;
        info->txSwizzle       = baseComponents_rgba;
    }

    if (Hardware->features[gcvFEATURE_D24S8_SAMPLE_STENCIL])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_D24S8);
        info->closestTXFormat = gcvSURF_D24S8;
        info->txFormat        = 0x22 << 12;
        info->txSwizzle       = baseComponents_rgba;
    }

    if (Hardware->features[gcvFEATURE_INTEGER32_FIX])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_R32I);
        info->closestTXFormat = gcvSURF_R32I;
        info->txFormat        = 0x23 << 12;
        info->txSwizzle        = baseComponents_r001;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_R32UI);
        info->closestTXFormat = gcvSURF_R32UI;
        info->txFormat        = 0x23 << 12;
        info->txSwizzle        = baseComponents_r001;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_G32R32I);
        info->closestTXFormat     = gcvSURF_G32R32I;
        info->txFormat            = 0x24 << 12;
        info->txSwizzle           = baseComponents_rg01;

        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_G32R32UI);
        info->closestTXFormat     = gcvSURF_G32R32UI;
        info->txFormat            = 0x24 << 12;
        info->txSwizzle           = baseComponents_rg01;
    }

    if (Hardware->features[gcvFEATURE_TEXTURE_SWIZZLE])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_X8B8G8R8);
        info->closestTXFormat = gcvSURF_X8B8G8R8;
        info->txFormat        = 0x08;
        info->txSwizzle       = baseComponents_bgr1;
        info->txIntFilter     = gcvTRUE;


        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_A8B8G8R8);
        info->closestTXFormat = gcvSURF_A8B8G8R8;
        info->txFormat        = 0x07;
        info->txSwizzle       = baseComponents_bgra;
        info->txIntFilter     = gcvTRUE;
    }

    if (Hardware->features[gcvFEATURE_R8_UNORM])
    {
        info = gcmGET_SURF_FORMAT_INFO(gcvSURF_R8);
        info->closestTXFormat = gcvSURF_R8;
        info->txFormat        = 0x21 << 12;
        info->txSwizzle       = baseComponents_r001;
        info->txIntFilter     = gcvTRUE;

        info->closestRenderFormat = gcvSURF_R8;
        info->renderFormat        = 0x23;
        info->pixelSwizzle        = baseComponents_r001;
    }

    if (!Hardware->features[gcvFEATURE_BLT_ENGINE] && (Hardware->patchID != gcvPATCH_WESTON))
    {
       gctSIZE_T i;

       for (i = 0; i < gcmCOUNTOF(formatLuminanceAlpha); ++i)
       {
           /* change all closestTXFormat=gcvSURF_A8L8 to gcvSURF_A8L8_1_A8R8G8B8 */
           if (formatLuminanceAlpha[i].closestTXFormat == gcvSURF_A8L8 &&
               formatLuminanceAlpha[i].format != gcvSURF_A8L8_RAW)
           {
              formatLuminanceAlpha[i].closestTXFormat = gcvSURF_A8L8_1_A8R8G8B8;
              formatLuminanceAlpha[i].txFormat        = gcvINVALID_TEXTURE_FORMAT;
              formatLuminanceAlpha[i].txIntFilter     = gcvFALSE;
           }
        }
    }
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoHARDWARE_SwitchFormatArrayTable(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER();

    gcmGETHARDWARE(Hardware);
    gcmONERROR(gcoHARDWARE_InitializeFormatArrayTable(Hardware));

OnError:
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_GetClosestTextureFormat
**
**  Returns the closest supported texture format to the one specified in the call.
**
**  INPUT:
**
**      gceSURF_FORMAT APIValue
**          API value.
**
**
**  INPUT:
**      gceTEXTURE_TYPE TextureType
**          Texture type
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Closest supported API value.
*/
gceSTATUS
gcoHARDWARE_GetClosestTextureFormat(
    IN gcoHARDWARE  Hardware,
    IN gceSURF_FORMAT InFormat,
    IN gceTEXTURE_TYPE TextureType,
    OUT gceSURF_FORMAT* OutFormat
    )
{
    gceSTATUS status;
    gcsSURF_FORMAT_INFO_PTR formatInfo;
    gceSURF_FORMAT closestTXFormat;

    gcmHEADER_ARG("Hardware=0x%x InFormat=%d TextureType=%d", Hardware, InFormat, TextureType);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Find the format record. */
    gcmONERROR(gcoHARDWARE_QueryFormat(InFormat, &formatInfo));

    closestTXFormat = formatInfo->closestTXFormat;

    if ((gctUINT) closestTXFormat == gcvINVALID_TEXTURE_FORMAT)
    {
        closestTXFormat  = gcvSURF_UNKNOWN;
    }

    /*
    ** Special handling for particular texture type
    */
    switch (TextureType)
    {
    case gcvTEXTURE_2D_ARRAY:
    case gcvTEXTURE_3D:
        if (Hardware->config->chipRevision >= 0x5220 && Hardware->config->chipRevision < 0x5420 &&
            !(Hardware->config->chipModel == gcv900 && Hardware->config->chipRevision == 0x5250)
           )
        {
            switch (closestTXFormat)
            {
            case gcvSURF_D24S8:
            case gcvSURF_D24X8:
            case gcvSURF_D16:
                closestTXFormat = gcvSURF_D24S8_1_A8R8G8B8;
                break;
            case gcvSURF_S8D32F:
            case gcvSURF_D32F:
            case gcvSURF_S8D32F_1_G32R32F:
                closestTXFormat = gcvSURF_S8D32F_2_A8R8G8B8;
                break;
            case gcvSURF_R32F:
                closestTXFormat = gcvSURF_R32F_1_A8R8G8B8;
                break;
            case gcvSURF_G32R32F:
                closestTXFormat = gcvSURF_G32R32F_2_A8R8G8B8;
                break;
            case gcvSURF_X32B32G32R32F_2_G32R32F:
                closestTXFormat = gcvSURF_X32B32G32R32F_4_A8R8G8B8;
                break;
            case gcvSURF_A32B32G32R32F_2_G32R32F:
                closestTXFormat = gcvSURF_A32B32G32R32F_4_A8R8G8B8;
                break;
            case gcvSURF_E5B9G9R9:
                gcmASSERT(0);
                break;
            default:
                break;
            }
        }
        break;

    default:
        break;
    }

    *OutFormat = closestTXFormat;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gcoHARDWARE_GetClosestRenderFormat
**
**  Returns the closest supported render format to the one specified in the call.
**
**  INPUT:
**
**      gceSURF_FORMAT APIValue
**          API value.
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Closest supported API value.
*/
gceSTATUS
gcoHARDWARE_GetClosestRenderFormat(
    IN gceSURF_FORMAT InFormat,
    OUT gceSURF_FORMAT* OutFormat
    )
{
    gceSTATUS               status;
    gcoHARDWARE             Hardware = gcvNULL;
    gcsSURF_FORMAT_INFO_PTR formatInfo;
    gceSURF_FORMAT          closestRenderFormat;

    gcmHEADER_ARG("InFormat=%d", InFormat);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Find the format record. */
    gcmONERROR(gcoHARDWARE_QueryFormat(InFormat, &formatInfo));

    closestRenderFormat = formatInfo->closestRenderFormat;

    if ((gctUINT) closestRenderFormat == gcvINVALID_RENDER_FORMAT)
    {
        closestRenderFormat = gcvSURF_UNKNOWN;
    }

    *OutFormat = closestRenderFormat;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/******************************************************************************\
******************************* gcoSURF API Code *******************************
\******************************************************************************/

/*******************************************************************************
**
**  gcoHARDWARE_QueryFormat
**
**  Return pixel format parameters.
**
**  INPUT:
**
**      gceSURF_FORMAT Format
**          API format.
**
**  OUTPUT:
**
**      gcsSURF_FORMAT_INFO_PTR * Info
**          Pointer to a variable that will hold the format description entry.
**          If the format in question is interleaved, two pointers will be
**          returned stored in an array fashion.
**
*/
gceSTATUS
gcoHARDWARE_QueryFormat(
    IN gceSURF_FORMAT Format,
    OUT gcsSURF_FORMAT_INFO_PTR * Info
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;
    struct gcsFORMAT_ARRAY * classTable;
    gcsSURF_FORMAT_INFO * info0;
    gctUINT classIndex;
    gctUINT formatIndex;

    gcmHEADER_ARG("Format=0x%08X (%d)", Format, Format);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Info != gcvNULL);

    /* Get the class record. */
    classIndex = Format / 100;
    if (classIndex >= gcmCOUNTOF(formatArray))
        goto OnError;

    classTable = &formatArray[classIndex];

    /* Get the format record. */
    formatIndex = Format % 100;

    if (formatIndex >= classTable->count)
        goto OnError;

    info0 = &classTable->formats[formatIndex];
    if (info0->format != Format)
        goto OnError;

    /* Success. */
    *Info = info0;
    status = gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_QueryBPP
**
**  Return pixel format parameters.
**
**  INPUT:
**
**      gceSURF_FORMAT Format
**          API format.
**
**  OUTPUT:
**
**      gctFLOAT_PTR BppArray
**          Pointer to a array that will hold the bytes per pixel for each planar.
**
*/
gceSTATUS
gcoHARDWARE_QueryBPP(
    IN gceSURF_FORMAT Format,
    OUT gctFLOAT_PTR BppArray
    )
{
    gctUINT32 bpp;
    gctFLOAT bpps[3] = {1.0};
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Format=0x%08X (%d)", Format, Format);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(BppArray != gcvNULL);

    gcmONERROR(gcoHARDWARE_ConvertFormat(Format, &bpp, gcvNULL));

    /* Palettized formats. */
    if (Format >= 100 && Format < 200)
    {
        bpps[0] = 1.0;
    }
    /* RGB formats. */
    else if (Format >= 200 && Format < 400)
    {
        bpps[0] = (gctFLOAT)(bpp / 8);
    }
    /* Alpha formats & R/RG/RA formats. */
    else if ((Format >=  700 && Format <  800) ||
             (Format >= 1100 && Format < 1200))
    {
        bpps[0] = bpp < 8 ? (gctFLOAT)1.0 : (gctFLOAT)(bpp / 8);
    }
    /* YUV formats. */
    else if (Format >= 500 && Format < 600)
    {
        switch(Format)
        {
            case gcvSURF_YUY2:
            case gcvSURF_UYVY:
            case gcvSURF_YVYU:
            case gcvSURF_VYUY:
            case gcvSURF_AYUV:
                bpps[0] = 2.0;
                break;

            case gcvSURF_NV12:
            case gcvSURF_NV21:
                bpps[0] = 1.0;
                bpps[1] = 1.0;
                break;

            case gcvSURF_NV16:
            case gcvSURF_NV61:
                bpps[0] = 1.0;
                bpps[1] = 1.0;
                break;

            case gcvSURF_YV12:
            case gcvSURF_I420:
                bpps[0] = 1.0;
                bpps[1] = 1.0;
                bpps[2] = 1.0;
                break;

            case gcvSURF_NV12_10BIT:
            case gcvSURF_NV21_10BIT:
                bpps[0] = 1.25;
                bpps[1] = 1.25;
                break;

            case gcvSURF_NV16_10BIT:
            case gcvSURF_NV61_10BIT:
                bpps[0] = 1.25;
                bpps[1] = 2.5;
                break;

            case gcvSURF_P010:
                bpps[0] = 2.0;
                bpps[1] = 2.0;
                break;

            default:
                status = gcvSTATUS_NOT_SUPPORTED;
                break;
        }
    }
    else
    {
        status = gcvSTATUS_NOT_SUPPORTED;
    }

    if (BppArray != gcvNULL)
    {
        BppArray[0] = bpps[0];
        BppArray[1] = bpps[1];
        BppArray[2] = bpps[2];
    }

OnError:
    gcmFOOTER();
    return status;
}

/******************************************************************************\
****************************** gcoHARDWARE API Code *****************************
\******************************************************************************/

/*******************************************************************************
**
**  gcoHARDWARE_ConvertFormat
**
**  Convert an API format to hardware parameters.
**
**  INPUT:
**
**      gceSURF_FORMAT Format
**          API format to convert.
**
**  OUTPUT:
**
**      gctUINT32_PTR BitsPerPixel
**          Pointer to a variable that will hold the number of bits per pixel.
**
**      gctUINT32_PTR BytesPerTile
**          Pointer to a variable that will hold the number of bytes per tile.
*/
gceSTATUS gcoHARDWARE_ConvertFormat(
    IN gceSURF_FORMAT Format,
    OUT gctUINT32_PTR BitsPerPixel,
    OUT gctUINT32_PTR BytesPerTile
    )
{
    gceSTATUS status;
    gcsSURF_FORMAT_INFO_PTR formatInfo;

    gcmHEADER_ARG("Format=%d BitsPerPixel=0x%x BytesPerTile=0x%x",
                  Format, BitsPerPixel, BytesPerTile);

    gcmONERROR(gcoHARDWARE_QueryFormat(Format, &formatInfo));

    if (BitsPerPixel != gcvNULL)
    {
        * BitsPerPixel = formatInfo->bitsPerPixel;
    }

    if (BytesPerTile != gcvNULL)
    {
        * BytesPerTile = (formatInfo->bitsPerPixel * 4 * 4) / 8;
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
**  gcoHARDWARE_QueryChipIdentity
**
**  Query the identity of the hardware.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**  OUTPUT:
**
**      gceCHIPMODEL* ChipModel
**          If 'ChipModel' is not gcvNULL, the variable it points to will
**          receive the model of the chip.
**
**      gctUINT32* ChipRevision
**          If 'ChipRevision' is not gcvNULL, the variable it points to will
**          receive the revision of the chip.
**
*/
gceSTATUS gcoHARDWARE_QueryChipIdentity(
    IN  gcoHARDWARE Hardware,
    OUT gceCHIPMODEL* ChipModel,
    OUT gctUINT32* ChipRevision
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hardware=0x%x ChipModel=0x%x ChipRevision=0x%x ",
                    Hardware, ChipModel, ChipRevision);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Return chip model. */
    if (ChipModel != gcvNULL)
    {
        *ChipModel = Hardware->config->chipModel;
    }

    /* Return revision number. */
    if (ChipRevision != gcvNULL)
    {
        *ChipRevision = Hardware->config->chipRevision;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gcoHARDWARE_QueryChipIdentityEx
**
**  Query the identity of the hardware.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gctUINT32  SizeOfParam
**          Size of Parameter structure memory.
**
**  OUTPUT:
**      gcsHAL_CHIPIDENTITY *ChipIdentity
**          Pointer to memory buffer to have chip identity.
*/
gceSTATUS gcoHARDWARE_QueryChipIdentityEx(
    IN  gcoHARDWARE Hardware,
    IN  gctUINT32 SizeOfParam,
    OUT gcsHAL_CHIPIDENTITY *ChipIdentity
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hardware=0x%x SizeOfParam=0x%x ChipIdentity=%p ",
                    Hardware, SizeOfParam, ChipIdentity);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (gcmSIZEOF(gcsHAL_CHIPIDENTITY) == SizeOfParam)
    {
        gcmASSERT(ChipIdentity);
        ChipIdentity->chipModel = Hardware->config->chipModel;
        ChipIdentity->chipRevision = Hardware->config->chipRevision;
        ChipIdentity->customerID = Hardware->config->customerID;
        ChipIdentity->ecoID = Hardware->config->ecoID;
        ChipIdentity->productID = Hardware->config->productID;
        ChipIdentity->platformFlagBits = Hardware->config->platformFlagBits;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gcoHARDWARE_QueryCommandBuffer
**
**  Query the command buffer alignment and number of reserved bytes.
**
**  INPUT:
**
**      gcoHARDWARE Harwdare
**          Pointer to an gcoHARDWARE object.
**
**  OUTPUT:
**
**      gctSIZE_T * Alignment
**          Pointer to a variable receiving the alignment for each command.
**
**      gctSIZE_T * ReservedHead
**          Pointer to a variable receiving the number of reserved bytes at the
**          head of each command buffer.
**
**      gctSIZE_T * ReservedTail
**          Pointer to a variable receiving the number of bytes reserved at the
**          tail of each command buffer.
*/
gceSTATUS
gcoHARDWARE_QueryCommandBuffer(
    IN gcoHARDWARE Hardware,
    IN gceENGINE Engine,
    OUT gctUINT32 * Alignment,
    OUT gctUINT32 * ReservedHead,
    OUT gctUINT32 * ReservedTail,
    OUT gctUINT32 * ReservedUser,
    OUT gctUINT32 * MGPUModeSwitchBytes
    )
{
    gceSTATUS status;
    gctUINT32 gpuFlushBytes = gcdRESERVED_FLUSHCACHE_LENGTH;
    gctUINT32 mGpuSyncBytes = 0;
    gctUINT32 mGpuModeSwitchBytes = 0;

    gcmHEADER_ARG("Alignment=0x%x ReservedHead=0x%x ReservedTail=0x%x",
                  Alignment, ReservedHead, ReservedTail);

    if (Alignment != gcvNULL)
    {
        /* Align every 8 bytes. */
        *Alignment = 8;
    }

    if (ReservedHead != gcvNULL)
    {
        /* BLT engine possible need flush MMU */
        if (Engine == gcvENGINE_BLT)
        {
            *ReservedHead = 40;
        }
        else
        {
            /* Only render engine need select pipe */
            *ReservedHead = 32;
        }
    }

    if (ReservedTail != gcvNULL)
    {
        if (Engine == gcvENGINE_RENDER)
        {
            gcmGETHARDWARE(Hardware);

            if (Hardware->config->gpuCoreCount == 1)
            {
                /* Reserve space for Link(). */
                *ReservedTail = 8;
            }
            else
            {
                /* Reserve space for ChipEnable, Link for each core. */
                *ReservedTail = (8 + 8) * Hardware->config->gpuCoreCount;
            }

            if (Hardware->features[gcvFEATURE_FENCE_64BIT])
            {
                *ReservedTail += gcdRENDER_FENCE_LENGTH;
            }

            if (Hardware->features[gcvFEATURE_MCFE])
            {
                /* 16 byte alignment for MCFE. */
                *ReservedTail = gcmALIGN(*ReservedTail, 16);
            }
        }
        else
        {
            *ReservedTail = gcdBLT_FENCE_LENGTH;
        }
    }

    mGpuModeSwitchBytes = Hardware->config->gpuCoreCount > 1 ?
                          4 * gcmSIZEOF(gctUINT32) : 0;

    if (ReservedUser != gcvNULL)
    {
        *ReservedUser = 0;

        if (Engine == gcvENGINE_BLT)
        {
            /* BLT engine will flush within command, so, don't need flush here
               All other semphore/stall/flush/sync command is unknown to Async 3DBLT pipe
            */
            if (Hardware->features[gcvFEATURE_FENCE_64BIT])
            {
                *ReservedUser = gcdRESERVED_HW_FENCE_64BIT;
            }
            else if (Hardware->features[gcvFEATURE_FENCE_32BIT])
            {
                *ReservedUser = gcdRESERVED_HW_FENCE_32BIT;
            }
        }
        else
        {
            if (Hardware->config->gpuCoreCount > 1)
            {
                gcoHARDWARE_QueryMultiGPUSyncLength(Hardware, &mGpuSyncBytes);
                *ReservedUser += mGpuSyncBytes;
            }

            if (!gcoHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_COMPUTE_ONLY))
            {
                if (Hardware->config->gpuCoreCount > 1)
                {
                    gcoHARDWARE_QueryMultiGPUCacheFlushLength(Hardware, &gpuFlushBytes);
                }

                *ReservedUser += gpuFlushBytes;
                *ReservedUser += gcdRESERVED_PAUSE_OQ_LENGTH;

                if (Hardware->features[gcvFEATURE_HW_TFB])
                {
                    *ReservedUser += (gcdRESERVED_PAUSE_XFBWRITTEN_QUERY_LENGTH + mGpuModeSwitchBytes);
                    *ReservedUser += (gcdRESERVED_PAUSE_PRIMGEN_QUERY_LENGTH    + mGpuModeSwitchBytes);
                    *ReservedUser += (gcdRESERVED_PAUSE_XFB_LENGTH              + mGpuModeSwitchBytes);
                }
            }

            if (Hardware->features[gcvFEATURE_PROBE])
            {
                /*only reserved more size for probe when enable profile*/
                gctSTRING profileGLmode = gcvNULL;
                gctSTRING profileVXmode = gcvNULL;
                gctSTRING profileCLmode = gcvNULL;
                gctSTRING probeDisalbed = gcvNULL;
                gcmONERROR(gcoOS_GetEnv(gcvNULL, "VIV_PROFILE", &profileGLmode));
                gcmONERROR(gcoOS_GetEnv(gcvNULL, "VIV_VX_PROFILE", &profileVXmode));
                gcmONERROR(gcoOS_GetEnv(gcvNULL, "VIV_CL_PROFILE", &profileCLmode));
                gcmONERROR(gcoOS_GetEnv(gcvNULL, "VP_DISABLE_PROBE", &probeDisalbed));
                if (!((probeDisalbed != gcvNULL) && gcmIS_SUCCESS(gcoOS_StrCmp(probeDisalbed, "1"))) &&
                    ((profileGLmode != gcvNULL && gcoOS_StrCmp(profileGLmode, "0") == gcvSTATUS_LARGER) ||
                    (profileVXmode != gcvNULL && gcoOS_StrCmp(profileVXmode, "0") == gcvSTATUS_LARGER) ||
                    (profileCLmode != gcvNULL && gcoOS_StrCmp(profileCLmode, "0") == gcvSTATUS_LARGER)))
                {
                    if (Hardware->config->gpuCoreCount > 1)
                    {
                        *ReservedUser += (Hardware->config->gpuCoreCount * (2 * gcmSIZEOF(gctUINT32) + gcdRESERVED_PAUSE_PROBE_LENGTH)
                                         + 2 * gcmSIZEOF(gctUINT32));
                    }
                    else
                    {
                        *ReservedUser += gcdRESERVED_PAUSE_PROBE_LENGTH;
                    }
                }
            }

            if (Hardware->features[gcvFEATURE_FENCE_64BIT])
            {
                *ReservedUser += (gcdRESERVED_HW_FENCE_64BIT + mGpuModeSwitchBytes);
            }
            else if (Hardware->features[gcvFEATURE_FENCE_32BIT])
            {
                *ReservedUser += (gcdRESERVED_HW_FENCE_32BIT + mGpuModeSwitchBytes);
            }
        }
    }

    if (MGPUModeSwitchBytes)
    {
        *MGPUModeSwitchBytes = mGpuModeSwitchBytes;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_AlignToTile
**
**  Align the specified width and height to tile boundaries.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gceSURF_TYPE Type
**          Type of alignment.
**
**      gctUINT32 * Width
**          Pointer to the width to be aligned.  If 'Width' is gcvNULL, no width
**          will be aligned.
**
**      gctUINT32 * Height
**          Pointer to the height to be aligned.  If 'Height' is gcvNULL, no height
**          will be aligned.
**
**  OUTPUT:
**
**      gctUINT32 * Width
**          Pointer to a variable that will receive the aligned width.
**
**      gctUINT32 * Height
**          Pointer to a variable that will receive the aligned height.
**
**      gctBOOL_PTR SuperTiled
**          Pointer to a variable that receives the super-tiling flag for the
**          surface.
**
**      gceSURF_ALIGNMENT *hAlignment
**          Pointer to a variable that receives the hAlignment for the
**          surface.

*/
gceSTATUS
gcoHARDWARE_AlignToTile(
    IN gcoHARDWARE Hardware,
    IN gceSURF_TYPE Type,
    IN gceSURF_TYPE Hint,
    IN gceSURF_FORMAT Format,
    IN OUT gctUINT32_PTR Width,
    IN OUT gctUINT32_PTR Height,
    IN gctUINT32 Depth,
    OUT gceTILING * Tiling,
    OUT gctBOOL_PTR SuperTiled,
    OUT gceSURF_ALIGNMENT *hAlignment
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gceTILING tiling;
    gctUINT32 xAlignment, yAlignment;
    gctBOOL superTiled;
    gceSURF_FORMAT format;
    gcsSURF_FORMAT_INFO_PTR formatInfo;
    gctBOOL superTileOnly = gcvFALSE;
    gceSURF_ALIGNMENT horizontalAlign = gcvSURF_FOUR;

    gcmHEADER_ARG("Hardware=0x%x Type=%d Format=0x%x Width=0x%x Height=0x%x hAlignment=0x%x",
                  Type, Format, Width, Height, hAlignment);

    gcmVERIFY_ARGUMENT((gctUINT32)Type < 0xFF);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    format = (gceSURF_FORMAT) (Format & ~(gcvSURF_FORMAT_OCL | gcvSURF_FORMAT_PATCH_BORDER));

    gcmONERROR(gcoHARDWARE_QueryFormat(format, &formatInfo));

    if (Hardware->features[gcvFEATURE_128BTILE] &&
       (Type == gcvSURF_RENDER_TARGET|| Type == gcvSURF_DEPTH))
    {
        if (Type == gcvSURF_DEPTH)
        {
            superTileOnly = gcvTRUE;
        }
        else if (Hint & gcvSURF_LINEAR)
        {
            superTileOnly = gcvFALSE;
        }
        else if (formatInfo->bitsPerPixel == 8 || formatInfo->bitsPerPixel == 16)
        {
            superTileOnly = gcvTRUE;
        }
    }

    switch (Type)
    {
#if gcdENABLE_3D
    case gcvSURF_RENDER_TARGET:
    case gcvSURF_DEPTH:
        if (Hint & gcvSURF_LINEAR)
        {
            /* Liner render target. */
            superTiled = gcvFALSE;
            tiling = gcvLINEAR;

            /* Fit resolve alignment. */
            xAlignment = Hardware->resolveAlignmentX;
            yAlignment = Hardware->resolveAlignmentY;
        }
        else if (Hint & gcvSURF_NO_TILE_STATUS)
        {
            if (Hardware->features[gcvFEATURE_MULTI_PIXELPIPES])
            {
                superTiled = gcvTRUE;
            }
            else
            {
                superTiled = Hardware->features[gcvFEATURE_SUPER_TILED] &&
                             Hardware->features[gcvFEATURE_SUPERTILED_TEXTURE];
            }

            if (Hint & gcvSURF_CREATE_AS_DISPLAYBUFFER)
            {
                /* Override for display buffer. */
                if (gcoHAL_GetOption(gcvNULL, gcvOPTION_PREFER_TILED_DISPLAY_BUFFER))
                {
                    superTiled = gcvFALSE;
                }
                else
                {
                    superTiled = Hardware->features[gcvFEATURE_SUPER_TILED] ||
                                 Hardware->features[gcvFEATURE_MULTI_PIXELPIPES];
                }
            }

            tiling = superTiled ? gcvSUPERTILED : gcvTILED;

            xAlignment = superTiled ? 64 : 16;
            yAlignment = superTiled ? 64 : 4;

            if (Hardware->features[gcvFEATURE_MULTI_PIXELPIPES] &&
                !Hardware->features[gcvFEATURE_SINGLE_BUFFER])
            {
                /*
                 * Split buffer only if we have multiple pixel pipe, and
                 * PE_ENHANCEMENTS3 is not available.
                 */
                tiling |= gcvTILING_SPLIT_BUFFER;
                yAlignment *= Hardware->config->resolvePipes;
            }
        }
        else if ((Hint & gcvSURF_CREATE_AS_DISPLAYBUFFER) &&
                 (gcoHAL_GetOption(gcvNULL, gcvOPTION_PREFER_TILED_DISPLAY_BUFFER) &&
                 !superTileOnly))
        {
            /*
             * special path for platform which perfer to have tiled display
             * buffer
             */
            tiling = gcvTILED;
            xAlignment = 16;
            yAlignment = 4;
            superTiled = gcvFALSE;

            if (Hardware->features[gcvFEATURE_MULTI_PIXELPIPES] &&
                !Hardware->features[gcvFEATURE_SINGLE_BUFFER])
            {
                /*
                 * Split buffer only if we have multiple pixel pipe, and
                 * PE_ENHANCEMENTS3 is not available.
                 */
                tiling |= gcvTILING_SPLIT_BUFFER;
                yAlignment *= Hardware->config->resolvePipes;
            }
        }
        else
        {
            superTiled = Hardware->features[gcvFEATURE_SUPER_TILED];
            tiling = superTiled ? gcvSUPERTILED : gcvTILED;

            xAlignment = superTiled ? 64 : 16;
            yAlignment = superTiled ? 64 : 4;

            if (Hardware->features[gcvFEATURE_MULTI_PIXELPIPES]
                && !Hardware->features[gcvFEATURE_SINGLE_BUFFER])
            {
                /*
                 * Split buffer only if we have multiple pixel pipe, and
                 * PE_ENHANCEMENTS3 is not available.
                 */
                tiling |= gcvTILING_SPLIT_BUFFER;
                yAlignment *= Hardware->config->resolvePipes;
            }
        }
        break;

    case gcvSURF_TEXTURE:
        switch (format)
        {
        case gcvSURF_ETC1:
        case gcvSURF_DXT1:
        case gcvSURF_DXT2:
        case gcvSURF_DXT3:
        case gcvSURF_DXT4:
        case gcvSURF_DXT5:
        case gcvSURF_R11_EAC:
        case gcvSURF_SIGNED_R11_EAC:
        case gcvSURF_RG11_EAC:
        case gcvSURF_SIGNED_RG11_EAC:
        case gcvSURF_RGB8_ETC2:
        case gcvSURF_SRGB8_ETC2:
        case gcvSURF_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        case gcvSURF_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        case gcvSURF_RGBA8_ETC2_EAC:
        case gcvSURF_SRGB8_ALPHA8_ETC2_EAC:
            /* Of course, hardware needs to support compressed super tiles. */
            superTiled = ((Format & gcvSURF_FORMAT_OCL) != 0)
                       ? gcvFALSE
                       : Hardware->features[gcvFEATURE_TEX_COMPRRESSION_SUPERTILED];

            xAlignment = superTiled ? 64 : 4;
            yAlignment = superTiled ? 64 : 4;
            tiling     = superTiled ? gcvSUPERTILED : gcvTILED;
            break;

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
            {
                gctUINT index;

                superTiled = gcvFALSE;
                index = format - gcvSURF_ASTC4x4;
                xAlignment = formatASTC[index].blockWidth;
                yAlignment = formatASTC[index].blockHeight;
                tiling = gcvTILED;
            }
            break;

        default:
            if ((format >= gcvSURF_YUY2) && (format <= gcvSURF_AYUV))
            {
                superTiled = gcvFALSE;
            }
            else
            {
                superTiled = (Hardware->features[gcvFEATURE_SINGLE_BUFFER] ||
                              Hardware->features[gcvFEATURE_SINGLE_PIPE_HALTI1]
                             ) &&
                             Hardware->features[gcvFEATURE_SUPER_TILED] &&
                             Hardware->features[gcvFEATURE_SUPERTILED_TEXTURE];
            }

            if (Hint & gcvSURF_TILE_RLV_FENCE)
            {
                superTiled = gcvFALSE;
            }

            if (Hint & gcvSURF_LINEAR)
            {
                superTiled = gcvFALSE;
                xAlignment = 16;
                yAlignment = 4;
                tiling     = gcvLINEAR;
            }
            else
            {
                xAlignment = superTiled ? 64
                           : Hardware->features[gcvFEATURE_TX_HOR_ALIGN_SEL] ? 16
                           : (Hardware->features[gcvFEATURE_128BTILE] ? 8 : 4);
                yAlignment = superTiled ? 64 : 4;
                tiling     = superTiled ? gcvSUPERTILED : gcvTILED;
            }
            break;
        }

        /* Need to be aligned for resolve. */
        if (Hardware->features[gcvFEATURE_MULTI_PIXELPIPES] &&
            !Hardware->features[gcvFEATURE_SINGLE_BUFFER])
        {
            if (!superTiled && (1 == Depth))
            {
                yAlignment *= Hardware->config->resolvePipes;
            }
        }
        break;
#endif

    default:
        switch (Format)
        {
        case gcvSURF_YUV420_TILE_ST:
        case gcvSURF_YUV420_TILE_10_ST:
            superTiled = gcvFALSE;
            /* Y is 64x64 pixel in one block. */
            xAlignment = 64;
            yAlignment = 64;
            break;

        default:
            superTiled = gcvFALSE;
            xAlignment = 16;
            yAlignment = 4;
        }

        if (Hardware->features[gcvFEATURE_MULTI_PIXELPIPES] &&
            !Hardware->features[gcvFEATURE_SINGLE_BUFFER])
        {
            yAlignment *= Hardware->config->resolvePipes;
        }

        tiling     = gcvLINEAR;
    }

    if (superTileOnly && !superTiled)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Only for gcvTILED, hAlignment is useful,
    ** for other cases, hAlign will be assigned in gcoTEXTURE_BindTextureEx
    ** base on tiling.
    */
    switch (tiling)
    {
    case gcvLINEAR:
        horizontalAlign = gcvSURF_SIXTEEN;
        break;

    case gcvTILED:
        if (xAlignment == 16)
        {
            horizontalAlign = gcvSURF_SIXTEEN;
        }
        else
        {
            horizontalAlign = gcvSURF_FOUR;
        }
        break;

    case gcvSUPERTILED:
        horizontalAlign = gcvSURF_SUPER_TILED;
        break;

    case gcvMULTI_SUPERTILED:
        horizontalAlign = gcvSURF_SPLIT_SUPER_TILED;
        break;

    case gcvMULTI_TILED:
        horizontalAlign = gcvSURF_SPLIT_TILED;
        break;

    case gcvYMAJOR_SUPERTILED:
        /* YMjor super tile still horizontalAlign to super tile,
        there are some other enum not list in  alignXlate[] of gcoHARDWARE_BindTexture*/
        horizontalAlign = gcvSURF_SUPER_TILED;
        break;

    default:
        horizontalAlign = gcvSURF_FOUR;
        break;
    }

    gcmTRACE_ZONE(
        gcvLEVEL_INFO, _GC_OBJ_ZONE,
        "%s: type=%x hint=%x format=%d => "
        "tiling=%x superTiled=%d alignment=%d,%d",
        __FUNCTION__,
        Type, Hint, Format,
        tiling, superTiled, xAlignment, yAlignment
        );

    if (Width != gcvNULL)
    {
        /* Align the width. */
        *Width = gcmALIGN_NP2(*Width, xAlignment);
    }

    if (Height != gcvNULL)
    {
        /* Align the height. */
        *Height = gcmALIGN_NP2(*Height, yAlignment);
    }

    if (Tiling != gcvNULL)
    {
        /* Copy the tiling. */
        *Tiling = tiling;
    }

    if (SuperTiled != gcvNULL)
    {
        /* Copy the super tiling. */
        *SuperTiled = superTiled;
    }

    if (hAlignment != gcvNULL)
    {
        *hAlignment = horizontalAlign;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_AlignToTileCompatible
**
**  Align the specified width and height to compatible size for all cores exist
**  in this hardware.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gceSURF_TYPE Type
**          Type of alignment.
**
**      gctUINT32 * Width
**          Pointer to the width to be aligned.  If 'Width' is gcvNULL, no width
**          will be aligned.
**
**      gctUINT32 * Height
**          Pointer to the height to be aligned.  If 'Height' is gcvNULL, no height
**          will be aligned.
**
**  OUTPUT:
**
**      gctUINT32 * Width
**          Pointer to a variable that will receive the aligned width.
**
**      gctUINT32 * Height
**          Pointer to a variable that will receive the aligned height.
**
**      gceTILING * Tiling
**          Pointer to a variable that receives the detailed tiling info.
**
**      gctBOOL_PTR SuperTiled
**          Pointer to a variable that receives the super-tiling flag for the
**          surface.
**
**      gceSURF_ALIGNMENT *hAlignment
**          Pointer to a variable that receives the hAlignment for the
**          surface.

*/
gceSTATUS
gcoHARDWARE_AlignToTileCompatible(
    IN gcoHARDWARE Hardware,
    IN gceSURF_TYPE Type,
    IN gceSURF_TYPE Hint,
    IN gceSURF_FORMAT Format,
    IN OUT gctUINT32_PTR Width,
    IN OUT gctUINT32_PTR Height,
    IN gctUINT32 Depth,
    OUT gceTILING * Tiling,
    OUT gctBOOL_PTR SuperTiled,
    OUT gceSURF_ALIGNMENT *hAlignment
    )
{
    gceSTATUS status;
    gcsTLS_PTR tls;
    gceHARDWARE_TYPE prevType;

    gcmHEADER_ARG("Hardware=0x%x Type=%d Format=0x%x Width=0x%x Height=0x%x hAlignment=0x%x",
                  Hardware, Type, Format, Width, Height, hAlignment);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    gcmONERROR(gcoOS_GetTLS(&tls));

    /* Set to 3D hardware. */
    prevType = tls->currentType;
    tls->currentType = gcvHARDWARE_3D;

    status = gcoHARDWARE_AlignToTile(gcvNULL, Type, Hint, Format, Width, Height,
                                     Depth, Tiling, SuperTiled, hAlignment);

    /* Set back to previous type. */
    tls->currentType = prevType;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


#if gcdENABLE_3D

/*******************************************************************************
**
**  gcoHARDWARE_GetSurfaceTileSize
**
**  Query the tile size of the given surface.
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      gctINT32 * TileWidth
**          Pointer to a variable receiving the width in pixels per tile.
**
**      gctINT32 * TileHeight
**          Pointer to a variable receiving the height in pixels per tile.
*/
gceSTATUS gcoHARDWARE_GetSurfaceTileSize(
    IN gcoSURF Surface,
    OUT gctINT32 * TileWidth,
    OUT gctINT32 * TileHeight
    )
{
    gcmHEADER_ARG("Surface=0x%x TileWidth=0x%x TileHeight=0x%x",
                    Surface, TileWidth, TileHeight);

    if (Surface->type == gcvSURF_BITMAP)
    {
        if (TileWidth != gcvNULL)
        {
            /* 1 pixel per 2D tile (linear). */
            *TileWidth = 1;
        }

        if (TileHeight != gcvNULL)
        {
            /* 1 pixel per 2D tile (linear). */
            *TileHeight = 1;
        }
    }
    else
    {
        if (TileWidth != gcvNULL)
        {
            /* 4 pixels per 3D tile for now. */
            *TileWidth = 4;
        }

        if (TileHeight != gcvNULL)
        {
            /* 4 lines per 3D tile. */
            *TileHeight = 4;
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHARDWARE_GetSurfaceResolveAlignment
**
**  Query the resolve alignemnt requirement to resolve this surface
**
**  INPUT:
**
**     gcoHARDWARE Hardware,
*           Pointer to hardware object
**     gcoSURF Surface,
**
**
**  OUTPUT:
**
**      gctUINT *originX
**          Pointer to a variable receiving X direction origin alignemnt requirement
**
**      gctUINT *originY
**          Pointer to a variable receiving Y direction origin alignemnt requirement
**
**      gctUINT *sizeX
**          Pointer to a variable receiving X direction size alignemnt requirement
**
**      gctUINT *sizeY
**          Pointer to a variable receiving Y direction size alignemnt requirement
**
*/
gceSTATUS gcoHARDWARE_GetSurfaceResolveAlignment(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface,
    OUT gctUINT *originX,
    OUT gctUINT *originY,
    OUT gctUINT *sizeX,
    OUT gctUINT *sizeY
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gctUINT x, y, width, height;

    gcmHEADER_ARG("Hardware=0x%x Surface=0x%x", Hardware, Surface);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Surface->superTiled)
    {
        x = y = 64;
    }
    else if (Surface->tiling == gcvLINEAR)
    {
        /* Requires 16,4 origin alignment for linear. */
        x = 16;
        y = 4;

        if (Surface->bitsPerPixel == 8)
        {
            /* Special resolve origin alignment for 8bpp. */
            x = 32;
        }
    }
    else
    {
        /* Requires 4,4 tile boundary. */
        x = y = 4;

        if (Surface->bitsPerPixel == 8)
        {
            /* Special resolve origin alignment for 8bpp. */
            x = 8;
        }
    }

    if (Hardware->multiPipeResolve)
    {
        /* More y-origin requirement if have to use multi-pipe resolve. */
        y *= Hardware->config->resolvePipes;
    }

    /* Get resolve alignment. */
    width  = Hardware->resolveAlignmentX;
    height = Hardware->resolveAlignmentY;

    if (Surface->bitsPerPixel == 8)
    {
        /* Special resolve size alignment for 8bpp. */
        width = 32;
    }

    gcmTRACE(
        gcvLEVEL_INFO,
        "%s: tiling=%d format=%d => "
        "originAlignment=%d,%d sizeAlignment=%dx%d",
        __FUNCTION__,
        Surface->tiling, Surface->format,
        x, y, width, height
        );

    if (originX)
    {
        *originX = x;
    }

    if (originY)
    {
        *originY = y;
    }

    if (sizeX)
    {
        *sizeX = width;
    }

    if (sizeY)
    {
        *sizeY = height;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;

}

#endif

/*******************************************************************************
**
**  gcoHARDWARE_QueryTileSize
**
**  Query the tile sizes.
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      gctINT32 * TileWidth2D
**          Pointer to a variable receiving the width in pixels per 2D tile.  If
**          the 2D is working in linear space, the width will be 1.  If there is
**          no 2D, the width will be 0.
**
**      gctINT32 * TileHeight2D
**          Pointer to a variable receiving the height in pixels per 2D tile.
**          If the 2D is working in linear space, the height will be 1.  If
**          there is no 2D, the height will be 0.
**
**      gctINT32 * TileWidth3D
**          Pointer to a variable receiving the width in pixels per 3D tile.  If
**          the 3D is working in linear space, the width will be 1.  If there is
**          no 3D, the width will be 0.
**
**      gctINT32 * TileHeight3D
**          Pointer to a variable receiving the height in pixels per 3D tile.
**          If the 3D is working in linear space, the height will be 1.  If
**          there is no 3D, the height will be 0.
**
**      gctUINT32 * Stride
**          Pointer to  variable receiving the stride requirement for all modes.
*/
gceSTATUS gcoHARDWARE_QueryTileSize(
    OUT gctINT32 * TileWidth2D,
    OUT gctINT32 * TileHeight2D,
    OUT gctINT32 * TileWidth3D,
    OUT gctINT32 * TileHeight3D,
    OUT gctUINT32 * Stride
    )
{
    gcmHEADER_ARG("TileWidth2D=0x%x TileHeight2D=0x%x TileWidth3D=0x%x "
                    "TileHeight3D=0x%x Stride=0x%x",
                    TileWidth2D, TileHeight2D, TileWidth3D,
                    TileHeight3D, Stride);

    if (TileWidth2D != gcvNULL)
    {
        /* 1 pixel per 2D tile (linear). */
        *TileWidth2D = 1;
    }

    if (TileHeight2D != gcvNULL)
    {
        /* 1 pixel per 2D tile (linear). */
        *TileHeight2D = 1;
    }

    if (TileWidth3D != gcvNULL)
    {
        /* 4 pixels per 3D tile for now. */
        *TileWidth3D = 4;
    }

    if (TileHeight3D != gcvNULL)
    {
        /* 4 lines per 3D tile. */
        *TileHeight3D = 4;
    }

    if (Stride != gcvNULL)
    {
        /* 64-byte stride requirement. */
        *Stride = 64;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

#if gcdENABLE_3D
/*******************************************************************************
**
**  gcoHARDWARE_QueryTargetCaps
**
**  Query the render target capabilities.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**  OUTPUT:
**
**      gctUINT * MaxWidth
**          Pointer to a variable receiving the maximum width of a render
**          target.
**
**      gctUINT * MaxHeight
**          Pointer to a variable receiving the maximum height of a render
**          target.
**
**      gctUINT * MultiTargetCount
**          Pointer to a variable receiving the number of render targets.
**
**      gctUINT * MaxSamples
**          Pointer to a variable receiving the maximum number of samples per
**          pixel for MSAA.
*/
gceSTATUS
gcoHARDWARE_QueryTargetCaps(
    IN  gcoHARDWARE Hardware,
    OUT gctUINT * MaxWidth,
    OUT gctUINT * MaxHeight,
    OUT gctUINT * MultiTargetCount,
    OUT gctUINT * MaxSamples
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hardware=0x%x MaxWidth=0x%x MaxHeight=0x%x "
                    "MultiTargetCount=0x%x MaxSamples=0x%x",
                    Hardware, MaxWidth, MaxHeight,
                    MultiTargetCount, MaxSamples);

    gcmGETHARDWARE(Hardware);

    if (MaxWidth != gcvNULL)
    {
        /* Return maximum width of render target for XAQ2. */
        if (Hardware->features[gcvFEATURE_8K_RT_FIX])
        {
            *MaxWidth = 8192;
        }
        else if (Hardware->features[gcvFEATURE_8K_RT])
        {

            /* All cores including halti2 has bug in SE to support 8192. */
            *MaxWidth = (8192 - 128);
        }
        else
        {
            *MaxWidth = 2048;
        }
    }

    if (MaxHeight != gcvNULL)
    {
        /* Return maximum height of render target for XAQ2. */
        if (Hardware->features[gcvFEATURE_8K_RT_FIX])
        {
            *MaxHeight = 8192;
        }
        if (Hardware->features[gcvFEATURE_8K_RT])
        {
            /* All cores including halti2 has bug in SE to support 8192. */
            *MaxHeight = (8192 - 128);

        }
        else
        {
            *MaxHeight = 2048;
        }
    }

    if (MultiTargetCount != gcvNULL)
    {
        *MultiTargetCount = Hardware->config->renderTargets;
    }

    if (MaxSamples != gcvNULL)
    {
        *MaxSamples = Hardware->features[gcvFEATURE_MSAA] ? 4 : 0;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif

#if gcdENABLE_3D
/*******************************************************************************
**
**  gcoHARDWARE_QueryTextureCaps
**
**  Query the texture capabilities.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**  OUTPUT:
**
**      gctUINT * MaxWidth
**          Pointer to a variable receiving the maximum width of a texture.
**
**      gctUINT * MaxHeight
**          Pointer to a variable receiving the maximum height of a texture.
**
**      gctUINT * MaxDepth
**          Pointer to a variable receiving the maximum depth of a texture.  If
**          volume textures are not supported, 0 will be returned.
**
**      gctBOOL * Cubic
**          Pointer to a variable receiving a flag whether the hardware supports
**          cubic textures or not.
**
**      gctBOOL * NonPowerOfTwo
**          Pointer to a variable receiving a flag whether the hardware supports
**          non-power-of-two textures or not.
**
**      gctUINT * VertexSamplers
**          Pointer to a variable receiving the number of sampler units in the
**          vertex shader.
**
**      gctUINT * PixelSamplers
**          Pointer to a variable receiving the number of sampler units in the
**          pixel shader.
**
**      gctUINT * MaxAnisoValue
**          Pointer to a variable receiving the maximum parameter value of
**          anisotropic filter.
*/
gceSTATUS gcoHARDWARE_QueryTextureCaps(
    IN  gcoHARDWARE Hardware,
    OUT gctUINT * MaxWidth,
    OUT gctUINT * MaxHeight,
    OUT gctUINT * MaxDepth,
    OUT gctBOOL * Cubic,
    OUT gctBOOL * NonPowerOfTwo,
    OUT gctUINT * VertexSamplers,
    OUT gctUINT * PixelSamplers,
    OUT gctUINT * MaxAnisoValue
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 samplerCount[gcvPROGRAM_STAGE_LAST] = {0};

    gcmHEADER_ARG("Hardware=0x%x MaxWidth=0x%x MaxHeight=0x%x MaxDepth=0x%x "
                    "Cubic=0x%x NonPowerOfTwo=0x%x VertexSamplers=0x%x "
                    "PixelSamplers=0x%x MaxAnisoValue=0x%x",
                    Hardware, MaxWidth, MaxHeight, MaxDepth,
                    Cubic, NonPowerOfTwo, VertexSamplers,
                    PixelSamplers, MaxAnisoValue);

    gcmGETHARDWARE(Hardware);

    if (MaxWidth != gcvNULL)
    {
        /* Maximum texture width for XAQ2. */
        *MaxWidth = Hardware->features[gcvFEATURE_TEXTURE_8K] ? 8192 : 2048;
    }

    if (MaxHeight != gcvNULL)
    {
        /* Maximum texture height for XAQ2. */
        *MaxHeight = Hardware->features[gcvFEATURE_TEXTURE_8K] ? 8192 : 2048;
    }

    if (MaxDepth != gcvNULL)
    {
        /* Maximum texture depth for XAQ2. */
        /* Due to 32-bit address, the actual depth limit is also restricted by width/height.
           OGL driver has a constant and is currently not using the query. Minimum for OCL1.2 is 256. */
        *MaxDepth = Hardware->features[gcvFEATURE_3D_TEXTURE] ? (Hardware->features[gcvFEATURE_TEXTURE_8K] ? 8192 : 2048) : 1;
    }

    if (Cubic != gcvNULL)
    {
        /* XAQ2 supports cube maps. */
        *Cubic = gcvTRUE;
    }

    if (NonPowerOfTwo != gcvNULL)
    {
        *NonPowerOfTwo = gcvTRUE;
    }

    gcmONERROR(
        gcoHARDWARE_QuerySamplerBase(Hardware,
        samplerCount,
        gcvNULL,
        gcvNULL));

    if (VertexSamplers != gcvNULL)
    {
        *VertexSamplers = samplerCount[gcvPROGRAM_STAGE_VERTEX];
    }

    if (PixelSamplers != gcvNULL)
    {
        *PixelSamplers = samplerCount[gcvPROGRAM_STAGE_FRAGMENT];
    }

    if (MaxAnisoValue != gcvNULL)
    {
        /* Return maximum value of anisotropy supported by XAQ2. */
        if(Hardware->features[gcvFEATURE_TEXTURE_ANISOTROPIC_FILTERING])
        {
            /* Anisotropic. */
            *MaxAnisoValue = 16;
        }
        else
        {
            /* Isotropic. */
            *MaxAnisoValue = 1;
        }
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_QueryIndexCaps
**
**  Query the index capabilities.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**
**  OUTPUT:
**
**      gctBOOL * Index8
**          Pointer to a variable receiving the capability for 8-bit indices.
**
**      gctBOOL * Index16
**          Pointer to a variable receiving the capability for 16-bit indices.
**          target.
**
**      gctBOOL * Index32
**          Pointer to a variable receiving the capability for 32-bit indices.
**
**      gctUINT * MaxIndex
**          Maximum number of indices.
*/
gceSTATUS
gcoHARDWARE_QueryIndexCaps(
    IN  gcoHARDWARE Hardware,
    OUT gctBOOL * Index8,
    OUT gctBOOL * Index16,
    OUT gctBOOL * Index32,
    OUT gctUINT * MaxIndex
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hardware=0x%x Index8=0x%x Index16=0x%x Index32=0x%x MaxIndex=0x%x",
                   Hardware, Index8, Index16, Index32, MaxIndex);

    gcmGETHARDWARE(Hardware);

    if (Index8 != gcvNULL)
    {
        /* XAQ2 supports 8-bit indices. */
        *Index8 = gcvTRUE;
    }

    if (Index16 != gcvNULL)
    {
        /* XAQ2 supports 16-bit indices. */
        *Index16 = gcvTRUE;
    }

    if (Index32 != gcvNULL)
    {
        /* Return 32-bit indices support. */
        if (Hardware->features[gcvFEATURE_FE20_BIT_INDEX])
        {
            *Index32 = gcvTRUE;
        }
        else
        {
            *Index32 = gcvFALSE;
        }
    }

    if (MaxIndex != gcvNULL)
    {
        /* Return MaxIndex supported. */
        if (Hardware->features[gcvFEATURE_FE20_BIT_INDEX])
        {
            if ((Hardware->config->chipRevision > 0x5000 && Hardware->config->chipRevision < 0x5100)
            || (Hardware->config->chipRevision > 0x4000 && Hardware->config->chipRevision < 0x4600))
            {
                /* XAQ2 supports up to 2**20 indices. */
                *MaxIndex = (1 << 20) - 1;
            }
            else
            {
                /* XAQ2 supports up to 2**24 indices. */
                *MaxIndex = (1 << 24) - 1;
            }
        }
        else
        {
            /* XAQ2 supports up to 2**16 indices. */
            *MaxIndex = (1 << 16) - 1;
        }
    }
OnError:

    /* Success. */
    gcmFOOTER_NO();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_QueryStreamCaps
**
**  Query the stream capabilities of the hardware.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**  OUTPUT:
**
**      gctUINT * MaxAttributes
**          Pointer to a variable that will hold the maximum number of
**          atrtributes for a primitive on success.
**
**      gctUINT * MaxStreamSize
**          Pointer to a variable that will hold the maximum number of bytes of
**          a stream on success.
**
**      gctUINT * NumberOfStreams
**          Pointer to a variable that will hold the number of streams on
**          success.
**
**      gctUINT * Alignment
**          Pointer to a variable that will receive the stream alignment
**          requirement.
**
**      gctUINT * maxAttribOffset
**          Pointer to a variable that will receive the maximum number of bytes of
**          an attribute in one stream. Normally it should be same MaxStreamSize.
**          But halt4 has some exception.
*/
gceSTATUS gcoHARDWARE_QueryStreamCaps(
    IN  gcoHARDWARE Hardware,
    OUT gctUINT32 * MaxAttributes,
    OUT gctUINT32 * MaxStreamStride,
    OUT gctUINT32 * NumberOfStreams,
    OUT gctUINT32 * Alignment,
    OUT gctUINT32 * MaxAttribOffset
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hardware=0x%x MaxAttributes=0x%x MaxStreamStride=0x%x "
                  "NumberOfStreams=0x%x Alignment=0x%x MaxAttribOffset=0x%x",
                  Hardware, MaxAttributes, MaxStreamStride,
                  NumberOfStreams, Alignment, MaxAttribOffset);

    gcmGETHARDWARE(Hardware);

    if (MaxAttributes != gcvNULL)
    {
        /* Return number of attributes per vertex for XAQ2. */
        *MaxAttributes = Hardware->features[gcvFEATURE_PIPELINE_32_ATTRIBUTES] ?
                         32 : (Hardware->features[gcvFEATURE_HALTI0] ? 16 : 10);
    }

    if (MaxStreamStride != gcvNULL)
    {
        /* Return maximum number of bytes per vertex for XAQ2. */
        *MaxStreamStride = Hardware->features[gcvFEATURE_FE_12bit_stride] ? 2048 : 256;
    }

    if (NumberOfStreams != gcvNULL)
    {
        /* Return number of streams for XAQ2. */
        *NumberOfStreams = Hardware->config->streamCount;
    }

    if (Alignment != gcvNULL)
    {
        /* Return alignment. */
        *Alignment = (Hardware->config->chipModel == gcv700) ? 128 : 8;
    }

    if (MaxAttribOffset != gcvNULL)
    {
        *MaxAttribOffset = Hardware->features[gcvFEATURE_NEW_GPIPE] ? 2047 : 255;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_QueryShaderCaps
**
**  Query the shader capabilities.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**  OUTPUT:
**
**      gctUINT * UnifiedUniforms
**          Pointer to a variable receiving total number of unified uniforms, 0 if not support unified uniforms.
**
**      gctUINT * VertexUniforms
**          Pointer to a variable receiving the number of uniforms in the vertex shader.
**
**      gctUINT * FragmentUniforms
**          Pointer to a variable receiving the number of uniforms in the fragment shader.
**
**      gctUINT * Varyings
**          Pointer to a variable receiving the maximum number of varyings.
**
**      gctUINT * ShaderCoreCount
**          Pointer to a variable receiving the number of shader core count.
**
**      gctUINT * ThreadCount
**          Pointer to a variable receiving the number of thread count.
**
**      gctUINT * VertInstructionCount
**          Pointer to a variable receiving the number of supported vs instruction count.
**
**      gctUINT * FragInstructionCount
**          Pointer to a variable receiving the number of supported fs instruction count.
*/
gceSTATUS
gcoHARDWARE_QueryShaderCaps(
    IN  gcoHARDWARE Hardware,
    OUT gctUINT * UnifiedUniforms,
    OUT gctUINT * VertUniforms,
    OUT gctUINT * FragUniforms,
    OUT gctUINT * Varyings,
    OUT gctUINT * ShaderCoreCount,
    OUT gctUINT * ThreadCount,
    OUT gctUINT * VertInstructionCount,
    OUT gctUINT * FragInstructionCount
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETHARDWARE(Hardware);

    if (UnifiedUniforms)
    {
        *UnifiedUniforms = Hardware->unifiedConst ? Hardware->constMax : 0;
    }

    if (VertUniforms || FragUniforms)
    {
        gctUINT vsConstMax = Hardware->vsConstMax;
        gctUINT psConstMax = Hardware->psConstMax;

        if (Hardware->currentApi == gcvAPI_OPENGL_ES30 &&
            Hardware->patchID == gcvPATCH_GTFES30)
        {
            /* ES30 conform requires at least 256 vs uniforms and 224 ps ones. */
            vsConstMax = gcmMAX(vsConstMax, 256);
            psConstMax = gcmMAX(psConstMax, 224);
        }

        if (VertUniforms != gcvNULL)
        {
            /* Return the vs shader const count. */
            *VertUniforms = vsConstMax;
        }

        if (FragUniforms != gcvNULL)
        {
            /* Return the ps shader const count. */
            *FragUniforms = psConstMax;
        }
    }

    if (Varyings != gcvNULL)
    {
        /* Return the shader varyings count. */
        /* Return the shader core count. */
        *Varyings = Hardware->config->varyingsCount;
    }

    if (ShaderCoreCount != gcvNULL)
    {
        /* Return the shader core count. */
        *ShaderCoreCount = Hardware->config->shaderCoreCount;
    }

    if (ThreadCount != gcvNULL)
    {
        /* Return the shader core count. */
        *ThreadCount = Hardware->config->threadCount;
    }

    if (VertInstructionCount != gcvNULL)
    {
        /* Return the shader core count. */
        *VertInstructionCount = Hardware->vsInstMax;
    }

    if (FragInstructionCount != gcvNULL)
    {
        /* Return the shader core count. */
        *FragInstructionCount = Hardware->psInstMax;
    }

OnError:
    /* Success. */
    gcmFOOTER_ARG("UnifiedUniforms=%u VertUniforms=%u FragUniforms=%u Varyings=%u ShaderCoreCount=%u "
                  "ThreadCount=%u VertInstructionCount=%u FragInstructionCount=%u",
                  gcmOPT_VALUE(UnifiedUniforms), gcmOPT_VALUE(VertUniforms), gcmOPT_VALUE(FragUniforms),
                  gcmOPT_VALUE(Varyings), gcmOPT_VALUE(ShaderCoreCount), gcmOPT_VALUE(ThreadCount),
                  gcmOPT_VALUE(VertInstructionCount), gcmOPT_VALUE(FragInstructionCount));

    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_QueryShaderCapsEx
**
**  Query the shader capabilities.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**
**  OUTPUT:
**
**      gctUINT64 * LocalMemSize
**          Pointer to a variable receiving the size of the local memory
**          available to the shader program.
**
**      gctUINT * AddressBits
**          Pointer to a variable receiving the number of address bits.
**
**      gctUINT * GlobalMemCachelineSize
**          Pointer to a variable receiving the size of global memory cache
**          line in bytes.
**
**      gctUINT * GlobalMemCacheSize
**          Pointer to a variable receiving the size of global memory cache
**          in bytes.
**
**      gctUINT * ClockFrequency
**          Pointer to a variable receiving the shader core clock.
*/
gceSTATUS
gcoHARDWARE_QueryShaderCapsEx(
    IN  gcoHARDWARE Hardware,
    OUT gctUINT64 * LocalMemSize,
    OUT gctUINT * AddressBits,
    OUT gctUINT * GlobalMemCachelineSize,
    OUT gctUINT * GlobalMemCacheSize,
    OUT gctUINT * ClockFrequency
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 globalMemCachelineSize;

    gcmHEADER_ARG("Hardware=0x%x LocalMemSize=0x%x AddressBits=0x%x GlobalMemCachelineSize=0x%x "
                  "GlobalMemCacheSize=0x%x ClockFrequency=0x%x",
                   Hardware, LocalMemSize, AddressBits, GlobalMemCachelineSize,
                   GlobalMemCacheSize, ClockFrequency);

    gcmGETHARDWARE(Hardware);

    if (LocalMemSize != gcvNULL)
    {
        *LocalMemSize = Hardware->config->localStorageSizeInKbyte * 1024ULL;
    }

    if (AddressBits != gcvNULL)
    {
        /* XAQ2 supports 32 bit addresses. */
        *AddressBits = 32;
    }

    /* 64 byte cache lines. */
    globalMemCachelineSize = 64;

    if (GlobalMemCachelineSize != gcvNULL)
    {
        *GlobalMemCachelineSize = globalMemCachelineSize;
    }

    if (GlobalMemCacheSize != gcvNULL)
    {
        /* Global Memory Cache size in bytes. */
        *GlobalMemCacheSize = Hardware->config->l1CacheSizeInKbyte * 1024;
    }

    if (ClockFrequency != gcvNULL)
    {
        gcmONERROR(gcoHARDWARE_QueryFrequency(Hardware));

        /* Return the shader core clock in Mhz. */
        *ClockFrequency = (Hardware->shClk + 500000) / 1000000;
    }

OnError:
    /* Success. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_QueryTileStatus
**
**  Query the linear size for a tile size buffer.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gcoSURF Surface
**          Pointer to surface.
**
**      gctSZIE_T Bytes
**          Size of the surface in bytes.
**
**  OUTPUT:
**
**      gctSIZE_T_PTR Size
**          Pointer to a variable receiving the number of bytes required to
**          store the tile status buffer.
**
**      gctSIZE_T_PTR Alignment
**          Pointer to a variable receiving the alignment requirement.
**
**      gctUINT32_PTR Filler
**          Pointer to a variable receiving the tile status filler for fast
**          clear.
*/
gceSTATUS
gcoHARDWARE_QueryTileStatus(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface,
    IN gctSIZE_T Bytes,
    OUT gctSIZE_T_PTR Size,
    OUT gctUINT_PTR Alignment,
    OUT gctUINT32_PTR Filler
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctBOOL is2BitPerTile;

    gcmHEADER_ARG("Hardware=%p Surface=%p Bytes=%u Size=%p "
                  "Alignment=%p Filler=%p",
                  Hardware, Surface, Bytes, Size, Alignment, Filler);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(Size != gcvNULL);

    /* See if tile status is supported. */
    if (!Hardware->features[gcvFEATURE_FAST_CLEAR])
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Check tile status size. */
    is2BitPerTile = Hardware->features[gcvFEATURE_TILE_STATUS_2BITS];

    if ((Hardware->config->chipModel == gcv500)
    &&  (Hardware->config->chipRevision > 2)
    )
    {
        gctUINT width, height;

        /* Compute the aligned number of tiles for the surface. */
        width  = gcmALIGN(Surface->alignedW, 4) >> 2;
        height = gcmALIGN(Surface->alignedH, 4) >> 2;

        /* Return the linear size. */
        *Size = gcmALIGN(width * height * 4 / 8, 256);
    }
    else if (Hardware->features[gcvFEATURE_128BTILE])
    {
        gctUINT alignment = (Hardware->features[gcvFEATURE_BLT_ENGINE] ? 1 :
                              (Hardware->resolveAlignmentX * Hardware->resolveAlignmentY)) * 4;
        gceCACHE_MODE cacheMode = Surface->cacheMode;

        /* Every cache (128B or 256B) -> 4bit */
        *Size = (cacheMode == gcvCACHE_256) ? (Bytes >> 9) : (Bytes >> 8);

        /* Align the tile status. */
        *Size = gcmALIGN(*Size, alignment);
    }
    else
    {
        gctUINT32 alignment = (Hardware->features[gcvFEATURE_BLT_ENGINE] ? 1 :
                                (Hardware->resolveAlignmentX * Hardware->resolveAlignmentY)) * 4;

        /* Return the linear size. */
        *Size = is2BitPerTile ? (Bytes >> 8) : (Bytes >> 7);

        if (Surface->isMsaa)
        {
            if (Hardware->features[gcvFEATURE_FAST_MSAA] || Hardware->features[gcvFEATURE_SMALL_MSAA])
            {
                gcmASSERT(is2BitPerTile);
                *Size >>= 2;
            }
        }

        /* Align the tile status. */
        *Size = gcmALIGN(*Size, alignment);
    }

    /* Set alignment. */
    if (Alignment != gcvNULL)
    {
        *Alignment = 64;
    }

    if (Filler != gcvNULL)
    {
        *Filler = ((Hardware->features[gcvFEATURE_COMPRESSION_DEC400]) || Surface->vMsaa ||
                    ((Hardware->config->chipModel == gcv500) && (Hardware->config->chipRevision > 2))
                  )
                  ? 0xFFFFFFFF
                  : is2BitPerTile ? 0x55555555
                                  : 0x11111111;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_QueryHzTileStatus(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface,
    IN gctSIZE_T Bytes,
    OUT gctSIZE_T_PTR Size,
    OUT gctUINT_PTR Alignment
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 alignment;

    gcmHEADER_ARG("Hardware=%p Surface=%p Size=%p Alignment=%p",
                  Hardware, Surface, Size, Alignment);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(Size != gcvNULL);

    /* Check tile status size. */
    alignment = (Hardware->features[gcvFEATURE_BLT_ENGINE] ? 1 :
                 Hardware->resolveAlignmentX * Hardware->resolveAlignmentY) * 4;

    /* HZ tile status is 1/64 of HZ buffer size. */
    *Size = Bytes >> 6;

    if (Surface->isMsaa &&
        (Hardware->features[gcvFEATURE_FAST_MSAA] ||
         Hardware->features[gcvFEATURE_SMALL_MSAA]))
    {
        *Size >>= 2;
    }

    /* Align the tile status. */
    *Size = gcmALIGN(*Size, alignment);

    /* Set alignment. */
    if (Alignment != gcvNULL)
    {
        *Alignment = 64;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_QuerySamplerBase(
    IN  gcoHARDWARE Hardware,
    OUT gctUINT32 * SamplerCount,
    OUT gctUINT32 * SamplerBase,
    OUT gctUINT32 * TotalCount
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT samplerCount[gcvPROGRAM_STAGE_LAST] = {0};
    gctUINT samplerBase[gcvPROGRAM_STAGE_LAST] = {0};
    gctUINT totalCount = 0;

    gcmHEADER_ARG("Hardware=0x%x, SamplerCount=0x%x SamplerBase=0x%x TotalCount=0x%x",
                    Hardware, SamplerCount, SamplerBase, TotalCount);

    gcmGETHARDWARE(Hardware);

    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Hardware->features[gcvFEATURE_TX_DESCRIPTOR])
    {
        gctUINT base = 0;
        samplerCount[gcvPROGRAM_STAGE_FRAGMENT] = 16;
        samplerBase[gcvPROGRAM_STAGE_FRAGMENT] = base;
        base += 16;
        if (Hardware->features[gcvFEATURE_GEOMETRY_SHADER])
        {
            samplerCount[gcvPROGRAM_STAGE_GEOMETRY] = 16;
            samplerBase[gcvPROGRAM_STAGE_GEOMETRY] = base;
            base += 16;
        }
        if (Hardware->features[gcvFEATURE_TESSELLATION])
        {
            samplerCount[gcvPROGRAM_STAGE_TES] = 16;
            samplerBase[gcvPROGRAM_STAGE_TES] = base;
            base += 16;
            samplerCount[gcvPROGRAM_STAGE_TCS] = 16;
            samplerBase[gcvPROGRAM_STAGE_TCS] = base;
            base += 16;
        }
        samplerCount[gcvPROGRAM_STAGE_VERTEX] = 16;
        samplerBase[gcvPROGRAM_STAGE_VERTEX] = base;

        samplerBase[gcvPROGRAM_STAGE_COMPUTE] =
        samplerBase[gcvPROGRAM_STAGE_OPENCL] = 0;
    }
    else if (Hardware->features[gcvFEATURE_HALTI1] || Hardware->config->customerID == 0x103)
    {
        samplerCount[gcvPROGRAM_STAGE_VERTEX] = 16;
        samplerCount[gcvPROGRAM_STAGE_FRAGMENT] = 16;
        samplerBase[gcvPROGRAM_STAGE_FRAGMENT] = 0;
        samplerBase[gcvPROGRAM_STAGE_VERTEX] = 16;
    }
    else if ((Hardware->config->chipModel == gcv880) && ((Hardware->config->chipRevision & 0xfff0) == 0x5120))
    {
        samplerCount[gcvPROGRAM_STAGE_VERTEX] = 4;
        samplerCount[gcvPROGRAM_STAGE_FRAGMENT] = 16;
        samplerBase[gcvPROGRAM_STAGE_FRAGMENT] = 0;
        samplerBase[gcvPROGRAM_STAGE_VERTEX] = 16;
    }
    else if (Hardware->config->chipModel > gcv500)
    {
        samplerCount[gcvPROGRAM_STAGE_VERTEX] = 4;
        samplerCount[gcvPROGRAM_STAGE_FRAGMENT] = 8;
        samplerBase[gcvPROGRAM_STAGE_FRAGMENT] = 0;
        samplerBase[gcvPROGRAM_STAGE_VERTEX] = 8;
    }
    else
    {
        samplerCount[gcvPROGRAM_STAGE_VERTEX] = 0;
        samplerCount[gcvPROGRAM_STAGE_FRAGMENT] = 8;
        samplerBase[gcvPROGRAM_STAGE_FRAGMENT] = 0;
        samplerBase[gcvPROGRAM_STAGE_VERTEX] = 8;
    }

    totalCount = samplerCount[gcvPROGRAM_STAGE_VERTEX] + samplerCount[gcvPROGRAM_STAGE_TCS]
                + samplerCount[gcvPROGRAM_STAGE_TES] + samplerCount[gcvPROGRAM_STAGE_GEOMETRY]
                + samplerCount[gcvPROGRAM_STAGE_FRAGMENT];

    samplerCount[gcvPROGRAM_STAGE_COMPUTE] =
    samplerCount[gcvPROGRAM_STAGE_OPENCL]  =
                                            samplerCount[gcvPROGRAM_STAGE_VERTEX] + samplerCount[gcvPROGRAM_STAGE_TCS]
                                            + samplerCount[gcvPROGRAM_STAGE_TES] + samplerCount[gcvPROGRAM_STAGE_GEOMETRY]
                                            + samplerCount[gcvPROGRAM_STAGE_FRAGMENT];

    if (SamplerCount)
    {
         gcoOS_MemCopy(SamplerCount, samplerCount, sizeof(samplerCount));
    }

    if (SamplerBase)
    {
        gcoOS_MemCopy(SamplerBase, samplerBase, sizeof(samplerBase));
    }

    if (TotalCount)
    {
        *TotalCount = totalCount;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_QueryUniformBase(
    IN  gcoHARDWARE Hardware,
    OUT gctUINT32 * VertexBase,
    OUT gctUINT32 * FragmentBase
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hardware=0x%x, VertexBase=0x%x, FragmentBase=0x%x",
                    Hardware, VertexBase, FragmentBase);

    gcmGETHARDWARE(Hardware);

    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (VertexBase)
    {
        *VertexBase = Hardware->config->vsConstBase;
    }

    if (FragmentBase != gcvNULL)
    {
        *FragmentBase = Hardware->config->psConstBase;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#endif

/*******************************************************************************
**
**  gcoHARDWARE_IsFeatureAvailable
**
**  Verifies whether the specified feature is available in hardware.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gceFEATURE Feature
**          Feature to be verified.
*/
gceSTATUS
gcoHARDWARE_IsFeatureAvailable(
    IN gcoHARDWARE Hardware,
    IN gceFEATURE Feature
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x Feature=%d", Hardware, Feature);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Feature < gcvFEATURE_COUNT)
    {
        status = Hardware->features[Feature]
            ? gcvSTATUS_TRUE
            : gcvSTATUS_FALSE;
    }
    else
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_Is2DAvailable
**
**  Verifies whether 2D engine is available.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_Is2DAvailable(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=%x", Hardware);

    gcmGETHARDWARE(Hardware);

    status = Hardware->hw2DEngine
        ? gcvSTATUS_TRUE
        : gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_IsPrimitiveRestart
**
**  Check whether primitive restart is enabled.
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
gcoHARDWARE_IsPrimitiveRestart(
    IN gcoHARDWARE Hardware
    )
{
#if gcdENABLE_3D
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETHARDWARE(Hardware);

    status = Hardware->FEStates->primitiveRestart
           ? gcvSTATUS_TRUE
           : gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
#else
    return gcvSTATUS_OK;
#endif
}


gceSTATUS
gcoHARDWARE_Query3DCoreCount(
    IN gcoHARDWARE Hardware,
    OUT gctUINT32 *Count
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hardware=0x%x Count=0x%x", Hardware, Count);

    gcmGETHARDWARE(Hardware);

    *Count = Hardware->config->gpuCoreCount;

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_QueryCoreIndex(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 CoreID,
    OUT gctUINT32 *CoreIndex
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hardware=0x%x CoreID=0x%x", Hardware, CoreID);

    gcmGETHARDWARE(Hardware);

    *CoreIndex = Hardware->coreIndexs[CoreID];

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_QueryCluster(
    IN gcoHARDWARE Hardware,
    OUT gctINT32  *ClusterMinID,
    OUT gctINT32  *ClusterMaxID,
    OUT gctUINT32  *ClusterCount,
    OUT gctUINT32  *ClusterIDWidth
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hardware=0x%x ClusterMinID=0x%x ClusterMaxID=0x%x ClusterCount = 0x%x",
                  Hardware, ClusterMinID, ClusterMaxID, ClusterCount);

    gcmGETHARDWARE(Hardware);

    if (ClusterMinID)
    {
        *ClusterMinID = Hardware->config->clusterMinID;
    }

    if (ClusterMaxID)
    {
        *ClusterMaxID = Hardware->config->clusterMaxID;
    }

    if (ClusterCount)
    {
        *ClusterCount = Hardware->config->clusterCount;
    }

    if (ClusterIDWidth)
    {
        *ClusterIDWidth = Hardware->config->clusterIDWidth;
    }

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_IsFlatMapped(
    IN gcoHARDWARE Hardware,
    IN gctPHYS_ADDR_T Address
    )
{
    gceSTATUS status = gcvSTATUS_FALSE;
    gctUINT32 i;

    gcmHEADER_ARG("0x%X", Address);

    gcmGETHARDWARE(Hardware);

    for (i = 0; i < Hardware->flatMappingRangeCount; i++)
    {
        if ((Address >= Hardware->flatMappingRanges[i].start) &&
            (Address < Hardware->flatMappingRanges[i].end) &&
            (Hardware->flatMappingRanges[i].flag == gcvFLATMAP_DIRECT) &&
            (Address != ~0ULL))
        {
            status = gcvSTATUS_TRUE;
            break;
        }
    }

OnError:
    gcmFOOTER();
    return status;
}

#if gcdENABLE_3D
gceSTATUS
gcoHARDWARE_DrawOnOneCore(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHINT_PTR hints;
    gctUINT i;

    gcmHEADER_ARG("Hardware=0x%X", Hardware);

    gcmGETHARDWARE(Hardware);

    hints = Hardware->SHStates->programState.hints;

    for (i = 0; i < gcvPROGRAM_STAGE_LAST; i++)
    {
        if (hints && (hints->memoryAccessFlags[gcvSHADER_MACHINE_LEVEL][i] & gceMA_FLAG_WRITE))
        {
            status = gcvSTATUS_TRUE;
            break;
        }
    }

    if (Hardware->QUERYStates->queryStatus[gcvQUERY_OCCLUSION] == gcvQUERY_Enabled)
    {
        status = gcvSTATUS_TRUE;
    }

OnError:
    gcmFOOTER();
    return status;
}
#endif

gceSTATUS
gcoHARDWARE_QuerySuperTileMode(
    IN gcoHARDWARE Hardware,
    OUT gctUINT32_PTR SuperTileMode
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER();

    gcmGETHARDWARE(Hardware);

#if gcdENABLE_3D
    *SuperTileMode = Hardware->config->superTileMode;
#endif

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_QueryChipAxiBusWidth(
    IN gcoHARDWARE Hardware,
    OUT gctBOOL * AXI128Bits
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER();

    gcmGETHARDWARE(Hardware);

    *AXI128Bits = ((Hardware->config->chipFlags & gcvCHIP_AXI_BUS128_BITS) != 0);

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_QueryMultiGPUSyncLength(
    IN gcoHARDWARE Hardware,
    OUT gctUINT32_PTR Bytes
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 coreCount;

    gcmHEADER();

    gcmGETHARDWARE(Hardware);

    coreCount = Hardware->config->gpuCoreCount;
    if (Hardware->features[gcvFEATURE_MULTIGPU_SYNC_V3])
    {
        *Bytes =  (4 + (coreCount - 2) * 8 + 4 * 2 ) * gcmSIZEOF(gctUINT32);
    }
    else if (Hardware->features[gcvFEATURE_MULTIGPU_SYNC_V2])
    {
        *Bytes = (18 + (coreCount - 2) * 10) * gcmSIZEOF(gctUINT32);
    }
    else
    {
        gcmASSERT(coreCount == 2);
        *Bytes = 18 * gcmSIZEOF(gctUINT32);
    }

    if (Hardware->features[gcvFEATURE_BLT_ENGINE])
    {
        /* Lock/Unlock command for blt engine.*/
        *Bytes += 4 * gcmSIZEOF(gctUINT32);

        if (Hardware->features[gcvFEATURE_MULTI_CLUSTER])
        {
            /*add multi cluster control command*/
            *Bytes += 2 * gcmSIZEOF(gctUINT32);
        }

        if (Hardware->features[gcvFEATURE_VMSAA] && !Hardware->features[gcvFEATURE_PE_VMSAA_COVERAGE_CACHE_FIX])
        {
            *Bytes += 2 * gcmSIZEOF(gctUINT32);
        }
    }

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_QueryMultiGPUCacheFlushLength(
    IN gcoHARDWARE Hardware,
    OUT gctUINT32_PTR Bytes
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER();

    gcmGETHARDWARE(Hardware);

    gcmASSERT(Hardware->config->gpuCoreCount > 1);

    *Bytes = ((4 + 4) /* two pair of semaphore-stall */
           +  (2 + 2) /* flush c/z/shL$ and flush vst */
           +  (2));     /* flush tile status */

    if (Hardware->features[gcvFEATURE_HALTI5])
    {
        *Bytes += (2); /* flush I$ */
    }

    if (Hardware->features[gcvFEATURE_TX_DESCRIPTOR])
    {
        *Bytes += (2); /* flush tx desctriptor cache */
    }

    if (Hardware->features[gcvFEATURE_HW_TFB])
    {
        *Bytes += (2); /* flush TFB cache */
    }

    *Bytes *= gcmSIZEOF(gctUINT32);

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_QuerySRAM(
    IN gcoHARDWARE Hardware,
    IN gcePOOL Type,
    OUT gctUINT32 *Size,
    OUT gctUINT32 *GPUVirtAddr,
    OUT gctPHYS_ADDR_T *GPUPhysAddr,
    OUT gctUINT32 *GPUPhysName,
    OUT gctPHYS_ADDR_T *CPUPhysAddr
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hardware=%p Type=%d Size=%p, GPUVirtAddr=%p, GPUPhysAddr=%p, GPUPhysName=%p, GPUPhysAddr=%p",
                  Hardware, Type, Size, GPUVirtAddr, GPUPhysAddr, GPUPhysName, GPUPhysAddr);

    gcmGETHARDWARE(Hardware);

    if ((Type != gcvPOOL_INTERNAL_SRAM) &&
        (Type != gcvPOOL_EXTERNAL_SRAM))
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    if (Size)
    {
        *Size = (Type == gcvPOOL_EXTERNAL_SRAM) ? Hardware->options.extSRAMSizes[0] :
                                                  Hardware->options.sRAMSizes[0];
    }

    if (GPUVirtAddr)
    {
        *GPUVirtAddr = (Type == gcvPOOL_EXTERNAL_SRAM) ? Hardware->options.extSRAMGPUVirtAddrs[0] :
                                                         Hardware->options.sRAMGPUVirtAddrs[0];
    }

    if (GPUPhysAddr)
    {
        *GPUPhysAddr = (Type == gcvPOOL_EXTERNAL_SRAM) ? Hardware->options.extSRAMGPUPhysAddrs[0] :
                                                         gcvINVALID_PHYSICAL_ADDRESS;
    }

    if (GPUPhysName)
    {
        *GPUPhysName = (Type == gcvPOOL_EXTERNAL_SRAM) ? Hardware->options.extSRAMGPUPhysNames[0] : 0;
    }
    if (CPUPhysAddr)
    {
        *CPUPhysAddr = (Type == gcvPOOL_EXTERNAL_SRAM)? Hardware->options.extSRAMCPUPhysAddrs[0] :
                                                        gcvINVALID_PHYSICAL_ADDRESS;
    }

OnError:
    gcmFOOTER();
    return status;
}

#if gcdENABLE_3D && gcdUSE_VX
gceSTATUS
gcoHARDWARE_QueryNNConfig(
    IN gcoHARDWARE Hardware,
    OUT gctPOINTER Config
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    vx_nn_config* NNConfig = (vx_nn_config*) Config;

    gcmHEADER_ARG("Hardware=%p NNConfig=%p", Hardware, NNConfig);

    gcmGETHARDWARE(Hardware);

    if (NNConfig != gcvNULL)
    {
        gcoOS_MemCopy(NNConfig, &Hardware->config->nnConfig, sizeof(vx_nn_config));
        NNConfig->isSet = gcvTRUE;
    }

OnError:
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_QueryHwChipInfo
**
**  Query hw chip information.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**  OUTPUT:
**
**      gctUINT * customerID
**          Pointer to customerID.
**
**      gctUINT * ecoID
**          Pointer to ecoID.
*/
gceSTATUS
gcoHARDWARE_QueryHwChipInfo(
    IN  gcoHARDWARE  Hardware,
    OUT gctUINT32   *customerID,
    OUT gctUINT32   *ecoID
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETHARDWARE(Hardware);

    if (customerID)
    {
        *customerID = Hardware->config->customerID;
    }

    if (ecoID)
    {
        *ecoID = Hardware->config->ecoID;
    }

OnError:
    /* Success. */
    gcmFOOTER_ARG("customerID=%u ecoID=%u ",
                  gcmOPT_VALUE(customerID), gcmOPT_VALUE(ecoID));

    return status;
}
#endif

