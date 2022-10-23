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
**  2D support functions.
**
*/

#include "gc_hal_user_precomp.h"
#include "gc_hal_user_brush.h"

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcdZONE_2D

/******************************************************************************\
********************************** Structures **********************************
\******************************************************************************/

struct _gco2D
{
    gcsOBJECT           object;
    gctBOOL             hwAvailable;
    gcoBRUSH_CACHE      brushCache;
    gctBOOL             alignImproved;
    gctBOOL             fullRotation;
    gctBOOL             tiling;
    gctBOOL             minorTiling;
    gcs2D_State         state;
    gcoHARDWARE         hardware;
};

#define _DestroyKernelArray(KernelInfo)                                        \
    if (KernelInfo.kernelStates != gcvNULL)                                    \
    {                                                                          \
        /* Free the array. */                                                  \
        if(gcmIS_ERROR(gcoOS_Free(gcvNULL, KernelInfo.kernelStates)))          \
        {                                                                      \
            gcmTRACE_ZONE(gcvLEVEL_ERROR, gcdZONE_2D,                          \
                "2D Engine: Failed to free kernel table.");                    \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            KernelInfo.kernelStates = gcvNULL;                                 \
        }                                                                      \
                                                                               \
        /* Reset the pointer. */                                               \
        KernelInfo.kernelStates = gcvNULL;                                     \
    }

#define _FreeKernelArray(State)                                                \
        _DestroyKernelArray(State.horSyncFilterKernel)                         \
        _DestroyKernelArray(State.verSyncFilterKernel)                         \
        _DestroyKernelArray(State.horBlurFilterKernel)                         \
        _DestroyKernelArray(State.verBlurFilterKernel)                         \
        _DestroyKernelArray(State.horUserFilterKernel)                         \
        _DestroyKernelArray(State.verUserFilterKernel)

/******************************************************************************\
******************************** gco2D API Code ********************************
\******************************************************************************/

/*******************************************************************************
**
**  gco2D_Construct
**
**  Construct a new gco2D object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Poniter to an gcoHAL object.
**
**  OUTPUT:
**
**      gco2D * Engine
**          Pointer to a variable that will hold the pointer to the gco2D object.
*/
gceSTATUS
gco2D_Construct(
    IN gcoHAL Hal,
    OUT gco2D * Engine
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_Destroy
**
**  Destroy an gco2D object.
**
**  INPUT:
**
**      gco2D Engine
**          Poniter to an gco2D object to destroy.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_Destroy(
    IN gco2D Engine
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetBrushLimit
**
**  Sets the maximum number of brushes in the cache.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT MaxCount
**          Maximum number of brushes allowed in the cache at the same time.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetBrushLimit(
    IN gco2D Engine,
    IN gctUINT MaxCount
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_GetBrushCache
**
**  Return a pointer to the brush cache.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**  OUTPUT:
**
**      gcoBRUSH_CACHE * BrushCache
**          A pointer to gcoBRUSH_CACHE object.
*/
gceSTATUS
gco2D_GetBrushCache(
    IN gco2D Engine,
    IN OUT gcoBRUSH_CACHE * BrushCache
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_FlushBrush
**
**  Flush the brush.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gcoBRUSH Brush
**          A pointer to a valid gcoBRUSH object.
**
**      gceSURF_FORMAT Format
**          Format for destination surface when using color conversion.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_FlushBrush(
    IN gco2D Engine,
    IN gcoBRUSH Brush,
    IN gceSURF_FORMAT Format
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_LoadSolidBrush
**
**  Program the specified solid color brush.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gceSURF_FORMAT Format
**          Format for destination surface when using color conversion.
**
**      gctUINT32 ColorConvert
**          The value of the Color parameter is stored directly in internal
**          color register and is used either directly to initialize pattern
**          or is converted to the format of destination before it is used.
**          The later happens if ColorConvert is not zero.
**
**      gctUINT32 Color
**          The color value of the pattern. The value will be used to
**          initialize 8x8 pattern. If the value is in destination format,
**          set ColorConvert to 0. Otherwise, provide the value in ARGB8
**          format and set ColorConvert to 1 to instruct the hardware to
**          convert the value to the destination format before it is
**          actually used.
**
**      gctUINT64 Mask
**          64 bits of mask, where each bit corresponds to one pixel of 8x8
**          pattern. Each bit of the mask is used to determine transparency
**          of the corresponding pixel, in other words, each mask bit is used
**          to select between foreground or background ROPs. If the bit is 0,
**          background ROP is used on the pixel; if 1, the foreground ROP
**          is used. The mapping between Mask parameter bits and actual
**          pattern pixels is as follows:
**
**          +----+----+----+----+----+----+----+----+
**          |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
**          +----+----+----+----+----+----+----+----+
**          | 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |
**          +----+----+----+----+----+----+----+----+
**          | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 |
**          +----+----+----+----+----+----+----+----+
**          | 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 |
**          +----+----+----+----+----+----+----+----+
**          | 39 | 38 | 37 | 36 | 35 | 34 | 33 | 32 |
**          +----+----+----+----+----+----+----+----+
**          | 47 | 46 | 45 | 44 | 43 | 42 | 41 | 40 |
**          +----+----+----+----+----+----+----+----+
**          | 55 | 54 | 53 | 52 | 51 | 50 | 49 | 48 |
**          +----+----+----+----+----+----+----+----+
**          | 63 | 62 | 61 | 60 | 59 | 58 | 57 | 56 |
**          +----+----+----+----+----+----+----+----+
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_LoadSolidBrush(
    IN gco2D Engine,
    IN gceSURF_FORMAT Format,
    IN gctUINT32 ColorConvert,
    IN gctUINT32 Color,
    IN gctUINT64 Mask
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_LoadMonochromeBrush
**
**  Create a new monochrome gcoBRUSH object.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 OriginX
**      gctUINT32 OriginY
**          Specifies the origin of the pattern in 0..7 range.
**
**      gctUINT32 ColorConvert
**          The values of FgColor and BgColor parameters are stored directly in
**          internal color registers and are used either directly to initialize
**          pattern or converted to the format of destination before actually
**          used. The later happens if ColorConvert is not zero.
**
**      gctUINT32 FgColor
**      gctUINT32 BgColor
**          Foreground and background colors of the pattern. The values will be
**          used to initialize 8x8 pattern. If the values are in destination
**          format, set ColorConvert to 0. Otherwise, provide the values in
**          ARGB8 format and set ColorConvert to 1 to instruct the hardware to
**          convert the values to the destination format before they are
**          actually used.
**
**      gctUINT64 Bits
**          64 bits of pixel bits. Each bit represents one pixel and is used
**          to choose between foreground and background colors. If the bit
**          is 0, the background color is used; otherwise the foreground color
**          is used. The mapping between Bits parameter and the actual pattern
**          pixels is the same as of the Mask parameter.
**
**      gctUINT64 Mask
**          64 bits of mask, where each bit corresponds to one pixel of 8x8
**          pattern. Each bit of the mask is used to determine transparency
**          of the corresponding pixel, in other words, each mask bit is used
**          to select between foreground or background ROPs. If the bit is 0,
**          background ROP is used on the pixel; if 1, the foreground ROP
**          is used. The mapping between Mask parameter bits and the actual
**          pattern pixels is as follows:
**
**          +----+----+----+----+----+----+----+----+
**          |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
**          +----+----+----+----+----+----+----+----+
**          | 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |
**          +----+----+----+----+----+----+----+----+
**          | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 |
**          +----+----+----+----+----+----+----+----+
**          | 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 |
**          +----+----+----+----+----+----+----+----+
**          | 39 | 38 | 37 | 36 | 35 | 34 | 33 | 32 |
**          +----+----+----+----+----+----+----+----+
**          | 47 | 46 | 45 | 44 | 43 | 42 | 41 | 40 |
**          +----+----+----+----+----+----+----+----+
**          | 55 | 54 | 53 | 52 | 51 | 50 | 49 | 48 |
**          +----+----+----+----+----+----+----+----+
**          | 63 | 62 | 61 | 60 | 59 | 58 | 57 | 56 |
**          +----+----+----+----+----+----+----+----+
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_LoadMonochromeBrush(
    IN gco2D Engine,
    IN gctUINT32 OriginX,
    IN gctUINT32 OriginY,
    IN gctUINT32 ColorConvert,
    IN gctUINT32 FgColor,
    IN gctUINT32 BgColor,
    IN gctUINT64 Bits,
    IN gctUINT64 Mask
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_LoadColorBrush
**
**  Create a color gcoBRUSH object.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 OriginX
**      gctUINT32 OriginY
**          Specifies the origin of the pattern in 0..7 range.
**
**      gctPOINTER Address
**          Location of the pattern bitmap in video memory.
**
**      gceSURF_FORMAT Format
**          Format of the source bitmap.
**
**      gctUINT64 Mask
**          64 bits of mask, where each bit corresponds to one pixel of 8x8
**          pattern. Each bit of the mask is used to determine transparency
**          of the corresponding pixel, in other words, each mask bit is used
**          to select between foreground or background ROPs. If the bit is 0,
**          background ROP is used on the pixel; if 1, the foreground ROP
**          is used. The mapping between Mask parameter bits and the actual
**          pattern pixels is as follows:
**
**          +----+----+----+----+----+----+----+----+
**          |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
**          +----+----+----+----+----+----+----+----+
**          | 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |
**          +----+----+----+----+----+----+----+----+
**          | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 |
**          +----+----+----+----+----+----+----+----+
**          | 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 |
**          +----+----+----+----+----+----+----+----+
**          | 39 | 38 | 37 | 36 | 35 | 34 | 33 | 32 |
**          +----+----+----+----+----+----+----+----+
**          | 47 | 46 | 45 | 44 | 43 | 42 | 41 | 40 |
**          +----+----+----+----+----+----+----+----+
**          | 55 | 54 | 53 | 52 | 51 | 50 | 49 | 48 |
**          +----+----+----+----+----+----+----+----+
**          | 63 | 62 | 61 | 60 | 59 | 58 | 57 | 56 |
**          +----+----+----+----+----+----+----+----+
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_LoadColorBrush(
    IN gco2D Engine,
    IN gctUINT32 OriginX,
    IN gctUINT32 OriginY,
    IN gctUINT32 Address,
    IN gceSURF_FORMAT Format,
    IN gctUINT64 Mask
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetMonochromeSource
**
**  Configure color source.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctBOOL ColorConvert
**          The values of FgColor and BgColor parameters are stored directly in
**          internal color registers and are used either directly as the source
**          color or converted to the format of destination before actually
**          used.  The later happens if ColorConvert is gcvTRUE.
**
**      gctUINT8 MonoTransparency
**          This value is used in gcvSURF_SOURCE_MATCH transparency mode.  The
**          value can be either 0 or 1 and is compared against each mono pixel
**          to determine transparency of the pixel.  If the values found are
**          equal, the pixel is transparent; otherwise it is opaque.
**
**      gceSURF_MONOPACK DataPack
**          Determines how many horizontal pixels are there per each 32-bit
**          chunk of monochrome bitmap.  For example, if set to gcvSURF_PACKED8,
**          each 32-bit chunk is 8-pixel wide, which also means that it defines
**          4 vertical lines of pixels.
**
**      gctBOOL CoordRelative
**          If gcvFALSE, the source origin represents absolute pixel coordinate
**          within the source surface. If gcvTRUE, the source origin represents the
**          offset from the destination origin.
**
**      gceSURF_TRANSPARENCY Transparency
**          gcvSURF_OPAQUE - each pixel of the bitmap overwrites the destination.
**          gcvSURF_SOURCE_MATCH - source pixels compared against register value
**              to determine the transparency.  In simple terms, the
**              transaprency comes down to selecting the ROP code to use.
**              Opaque pixels use foreground ROP and transparent ones use
**              background ROP.
**          gcvSURF_SOURCE_MASK - monochrome source mask defines transparency.
**          gcvSURF_PATTERN_MASK - pattern mask defines transparency.
**
**      gctUINT32 FgColor
**      gctUINT32 BgColor
**          The values are used to represent foreground and background colors
**          of the source.  If the values are in destination format, set
**          ColorConvert to gcvFALSE. Otherwise, provide the values in A8R8G8B8
**          format and set ColorConvert to gcvTRUE to instruct the hardware to
**          convert the values to the destination format before they are
**          actually used.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetMonochromeSource(
    IN gco2D Engine,
    IN gctBOOL ColorConvert,
    IN gctUINT8 MonoTransparency,
    IN gceSURF_MONOPACK DataPack,
    IN gctBOOL CoordRelative,
    IN gceSURF_TRANSPARENCY Transparency,
    IN gctUINT32 FgColor,
    IN gctUINT32 BgColor
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_SetColorSource
**
**  Configure color source. This function is deprecated. Please use
**  gco2D_SetColorSourceEx instead.
**
**  This function is only working with old PE (<2.0).
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 Address
**          Source surface base address.
**
**      gctUINT32 Stride
**          Stride of the source surface in bytes.
**
**      gceSURF_FORMAT Format
**          Color format of the source surface.
**
**      gceSURF_ROTATION Rotation
**          Type of rotation.
**
**      gctUINT32 SurfaceWidth
**          Required only if the surface is rotated. Equal to the width
**          of the source surface in pixels.
**
**      gctBOOL CoordRelative
**          If gcvFALSE, the source origin represents absolute pixel coordinate
**          within the source surface.  If gcvTRUE, the source origin represents
**          the offset from the destination origin.
**
**      gceSURF_TRANSPARENCY Transparency
**          gcvSURF_OPAQUE - each pixel of the bitmap overwrites the destination.
**          gcvSURF_SOURCE_MATCH - source pixels compared against register value
**              to determine the transparency.  In simple terms, the
**              transaprency comes down to selecting the ROP code to use.
**              Opaque pixels use foreground ROP and transparent ones use
**              background ROP.
**          gcvSURF_SOURCE_MASK - monochrome source mask defines transparency.
**          gcvSURF_PATTERN_MASK - pattern mask defines transparency.
**
**      gctUINT32 TransparencyColor
**          This value is used in gcvSURF_SOURCE_MATCH transparency mode.  The
**          value is compared against each pixel to determine transparency of
**          the pixel.  If the values found are equal, the pixel is transparent;
**          otherwise it is opaque.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetColorSource(
    IN gco2D Engine,
    IN gctUINT32 Address,
    IN gctUINT32 Stride,
    IN gceSURF_FORMAT Format,
    IN gceSURF_ROTATION Rotation,
    IN gctUINT32 SurfaceWidth,
    IN gctBOOL CoordRelative,
    IN gceSURF_TRANSPARENCY Transparency,
    IN gctUINT32 TransparencyColor
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_SetColorSourceEx
**
**  Configure color source.
**
**  This function is only working with old PE (<2.0).
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 Address
**          Source surface base address.
**
**      gctUINT32 Stride
**          Stride of the source surface in bytes.
**
**      gceSURF_FORMAT Format
**          Color format of the source surface.
**
**      gceSURF_ROTATION Rotation
**          Type of rotation.
**
**      gctUINT32 SurfaceWidth
**          Required only if the surface is rotated. Equal to the width
**          of the source surface in pixels.
**
**      gctUINT32 SurfaceHeight
**          Required only if the surface is rotated in PE2.0. Equal to the height
**          of the source surface in pixels.
**
**      gctBOOL CoordRelative
**          If gcvFALSE, the source origin represents absolute pixel coordinate
**          within the source surface.  If gcvTRUE, the source origin represents
**          the offset from the destination origin.
**
**      gceSURF_TRANSPARENCY Transparency
**          gcvSURF_OPAQUE - each pixel of the bitmap overwrites the destination.
**          gcvSURF_SOURCE_MATCH - source pixels compared against register value
**              to determine the transparency.  In simple terms, the
**              transaprency comes down to selecting the ROP code to use.
**              Opaque pixels use foreground ROP and transparent ones use
**              background ROP.
**          gcvSURF_SOURCE_MASK - monochrome source mask defines transparency.
**          gcvSURF_PATTERN_MASK - pattern mask defines transparency.
**
**      gctUINT32 TransparencyColor
**          This value is used in gcvSURF_SOURCE_MATCH transparency mode.  The
**          value is compared against each pixel to determine transparency of
**          the pixel.  If the values found are equal, the pixel is transparent;
**          otherwise it is opaque.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetColorSourceEx(
    IN gco2D Engine,
    IN gctUINT32 Address,
    IN gctUINT32 Stride,
    IN gceSURF_FORMAT Format,
    IN gceSURF_ROTATION Rotation,
    IN gctUINT32 SurfaceWidth,
    IN gctUINT32 SurfaceHeight,
    IN gctBOOL CoordRelative,
    IN gceSURF_TRANSPARENCY Transparency,
    IN gctUINT32 TransparencyColor
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/* Same as gco2D_SetColorSourceEx, but with better 64bit SW-path support.
** Please do NOT export the API now.
*/
gceSTATUS
gco2D_SetColorSource64(
    IN gco2D Engine,
    IN gctUINT32 Address,
    IN gctPOINTER Logical,
    IN gctUINT32 Stride,
    IN gceSURF_FORMAT Format,
    IN gceSURF_ROTATION Rotation,
    IN gctUINT32 SurfaceWidth,
    IN gctUINT32 SurfaceHeight,
    IN gctBOOL CoordRelative,
    IN gceSURF_TRANSPARENCY Transparency,
    IN gctUINT32 TransparencyColor
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_SetColorSourceAdvanced
**
**  Configure color source.
**
**  This function is only working with PE 2.0 and above.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gcoSURF Source
**          Source surface.
**
**      gctBOOL CoordRelative
**          If gcvFALSE, the source origin represents absolute pixel coordinate
**          within the source surface.  If gcvTRUE, the source origin represents
**          the offset from the destination origin.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetColorSourceAdvanced(
    IN gco2D Engine,
    IN gctUINT32 Address,
    IN gctUINT32 Stride,
    IN gceSURF_FORMAT Format,
    IN gceSURF_ROTATION Rotation,
    IN gctUINT32 SurfaceWidth,
    IN gctUINT32 SurfaceHeight,
    IN gctBOOL CoordRelative
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_SetMaskedSource
**
**  Configure masked color source. This function is deprecated. Please use
**  gco2D_SetMaskedSourceEx instead.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 Address
**          Source surface base address.
**
**      gctUINT32 Stride
**          Stride of the source surface in bytes.
**
**      gceSURF_FORMAT Format
**          Color format of the source surface.
**
**      gctBOOL CoordRelative
**          If gcvFALSE, the source origin represents absolute pixel coordinate
**          within the source surface.  If gcvTRUE, the source origin represents
**          the offset from the destination origin.
**
**      gceSURF_MONOPACK MaskPack
**          Determines how many horizontal pixels are there per each 32-bit
**          chunk of monochrome mask.  For example, if set to gcvSURF_PACKED8,
**          each 32-bit chunk is 8-pixel wide, which also means that it defines
**          4 vertical lines of pixel mask.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetMaskedSource(
    IN gco2D Engine,
    IN gctUINT32 Address,
    IN gctUINT32 Stride,
    IN gceSURF_FORMAT Format,
    IN gctBOOL CoordRelative,
    IN gceSURF_MONOPACK MaskPack
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_SetMaskedSourceEx
**
**  Configure masked color source.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 Address
**          Source surface base address.
**
**      gctUINT32 Stride
**          Stride of the source surface in bytes.
**
**      gceSURF_FORMAT Format
**          Color format of the source surface.
**
**      gctBOOL CoordRelative
**          If gcvFALSE, the source origin represents absolute pixel coordinate
**          within the source surface.  If gcvTRUE, the source origin represents
**          the offset from the destination origin.
**
**      gceSURF_MONOPACK MaskPack
**          Determines how many horizontal pixels are there per each 32-bit
**          chunk of monochrome mask.  For example, if set to gcvSURF_PACKED8,
**          each 32-bit chunk is 8-pixel wide, which also means that it defines
**          4 vertical lines of pixel mask.
**
**      gceSURF_ROTATION Rotation
**          Type of rotation in PE2.0.
**
**      gctUINT32 SurfaceWidth
**          Required only if the surface is rotated in PE2.0. Equal to the width
**          of the source surface in pixels.
**
**      gctUINT32 SurfaceHeight
**          Required only if the surface is rotated in PE2.0. Equal to the height
**          of the source surface in pixels.
**
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetMaskedSourceEx(
    IN gco2D Engine,
    IN gctUINT32 Address,
    IN gctUINT32 Stride,
    IN gceSURF_FORMAT Format,
    IN gctBOOL CoordRelative,
    IN gceSURF_MONOPACK MaskPack,
    IN gceSURF_ROTATION Rotation,
    IN gctUINT32 SurfaceWidth,
    IN gctUINT32 SurfaceHeight
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/* Same as gco2D_SetMaskedSourceEx, but with better 64bit SW-path support.
** Please do NOT export the API now.
*/
gceSTATUS
gco2D_SetMaskedSource64(
    IN gco2D Engine,
    IN gctUINT32 Address,
    IN gctPOINTER Logical,
    IN gctUINT32 Stride,
    IN gceSURF_FORMAT Format,
    IN gctBOOL CoordRelative,
    IN gceSURF_MONOPACK MaskPack,
    IN gceSURF_ROTATION Rotation,
    IN gctUINT32 SurfaceWidth,
    IN gctUINT32 SurfaceHeight
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_SetSource
**
**  Setup the source rectangle.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gcsRECT_PTR SrcRect
**          Pointer to a valid source rectangle.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetSource(
    IN gco2D Engine,
    IN gcsRECT_PTR SrcRect
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetTargetRect
**
**  Setup the target rectangle for multi src blit.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gcsRECT_PTR Rect
**          Pointer to a valid target rectangle.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetTargetRect(
    IN gco2D Engine,
    IN gcsRECT_PTR Rect
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetClipping
**
**  Set clipping rectangle.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gcsRECT_PTR Rect
**          Pointer to a valid destination rectangle.
**          The valid range of the coordinates is 0..32768.
**          A pixel is valid if the following is true:
**              (pixelX >= Left) && (pixelX < Right) &&
**              (pixelY >= Top)  && (pixelY < Bottom)
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetClipping(
    IN gco2D Engine,
    IN gcsRECT_PTR Rect
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetTarget
**
**  Configure destination. This function is deprecated. Please use
**  gco2D_SetTargetEx instead.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 Address
**          Destination surface base address.
**
**      gctUINT32 Stride
**          Stride of the destination surface in bytes.
**
**      gceSURF_ROTATION Rotation
**          Set to not zero if the destination surface is 90 degree rotated.
**
**      gctUINT32 SurfaceWidth
**          Required only if the surface is rotated. Equal to the width
**          of the destination surface in pixels.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetTarget(
    IN gco2D Engine,
    IN gctUINT32 Address,
    IN gctUINT32 Stride,
    IN gceSURF_ROTATION Rotation,
    IN gctUINT32 SurfaceWidth
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_SetTargetEx
**
**  Configure destination.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 Address
**          Destination surface base address.
**
**      gctUINT32 Stride
**          Stride of the destination surface in bytes.
**
**      gceSURF_ROTATION Rotation
**          Set to not zero if the destination surface is 90 degree rotated.
**
**      gctUINT32 SurfaceWidth
**          Required only if the surface is rotated. Equal to the width
**          of the destination surface in pixels.
**
**      gctUINT32 SurfaceHeight
**          Required only if the surface is rotated in PE 2.0. Equal to the height
**          of the destination surface in pixels.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetTargetEx(
    IN gco2D Engine,
    IN gctUINT32 Address,
    IN gctUINT32 Stride,
    IN gceSURF_ROTATION Rotation,
    IN gctUINT32 SurfaceWidth,
    IN gctUINT32 SurfaceHeight
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/* Same as gco2D_SetTargetEx, but with better 64bit SW-path support.
** Please do NOT export the API now.
*/
gceSTATUS
gco2D_SetTarget64(
    IN gco2D Engine,
    IN gctUINT32 Address,
    IN gctPOINTER Logical,
    IN gctUINT32 Stride,
    IN gceSURF_ROTATION Rotation,
    IN gctUINT32 SurfaceWidth,
    IN gctUINT32 SurfaceHeight
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_CalcStretchFactor
**
**  Calculate the stretch factors based on the sizes.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctINT32 SrcSize
**          Source size for horizontal or vertical direction.
**
**      gctINT32 DstSize
**          Destination size for horizontal or vertical direction.
**
**  OUTPUT:
**
**      gctINT32_PTR Factor
**          Stretch factor in 16.16 fixed point format.
*/
gceSTATUS
gco2D_CalcStretchFactor(
    IN gco2D Engine,
    IN gctINT32 SrcSize,
    IN gctINT32 DstSize,
    OUT gctUINT32_PTR Factor
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_SetStretchFactors
**
**  Calculate and program the stretch factors.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 HorFactor
**          Horizontal stretch factor.
**
**      gctUINT32 VerFactor
**          Vertical stretch factor.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetStretchFactors(
    IN gco2D Engine,
    IN gctUINT32 HorFactor,
    IN gctUINT32 VerFactor
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetStretchRectFactors
**
**  Calculate and program the stretch factors based on the rectangles.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gcsRECT_PTR SrcRect
**          Pointer to a valid source rectangle.
**
**      gcsRECT_PTR DstRect
**          Pointer to a valid destination rectangle.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetStretchRectFactors(
    IN gco2D Engine,
    IN gcsRECT_PTR SrcRect,
    IN gcsRECT_PTR DstRect
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_ConstructSingleColorBrush
**
**  Create a new solid color gcoBRUSH object.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 ColorConvert
**          The value of the Color parameter is stored directly in internal
**          color register and is used either directly to initialize pattern
**          or is converted to the format of destination before it is used.
**          The later happens if ColorConvert is not zero.
**
**      gctUINT32 Color
**          The color value of the pattern. The value will be used to
**          initialize 8x8 pattern. If the value is in destination format,
**          set ColorConvert to 0. Otherwise, provide the value in ARGB8
**          format and set ColorConvert to 1 to instruct the hardware to
**          convert the value to the destination format before it is
**          actually used.
**
**      gctUINT64 Mask
**          64 bits of mask, where each bit corresponds to one pixel of 8x8
**          pattern. Each bit of the mask is used to determine transparency
**          of the corresponding pixel, in other words, each mask bit is used
**          to select between foreground or background ROPs. If the bit is 0,
**          background ROP is used on the pixel; if 1, the foreground ROP
**          is used. The mapping between Mask parameter bits and actual
**          pattern pixels is as follows:
**
**          +----+----+----+----+----+----+----+----+
**          |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
**          +----+----+----+----+----+----+----+----+
**          | 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |
**          +----+----+----+----+----+----+----+----+
**          | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 |
**          +----+----+----+----+----+----+----+----+
**          | 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 |
**          +----+----+----+----+----+----+----+----+
**          | 39 | 38 | 37 | 36 | 35 | 34 | 33 | 32 |
**          +----+----+----+----+----+----+----+----+
**          | 47 | 46 | 45 | 44 | 43 | 42 | 41 | 40 |
**          +----+----+----+----+----+----+----+----+
**          | 55 | 54 | 53 | 52 | 51 | 50 | 49 | 48 |
**          +----+----+----+----+----+----+----+----+
**          | 63 | 62 | 61 | 60 | 59 | 58 | 57 | 56 |
**          +----+----+----+----+----+----+----+----+
**
**  OUTPUT:
**
**      gcoBRUSH * Brush
**          Pointer to the variable that will hold the gcoBRUSH object pointer.
*/
gceSTATUS
gco2D_ConstructSingleColorBrush(
    IN gco2D Engine,
    IN gctUINT32 ColorConvert,
    IN gctUINT32 Color,
    IN gctUINT64 Mask,
    gcoBRUSH * Brush
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_ConstructMonochromeBrush
**
**  Create a new monochrome gcoBRUSH object.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 OriginX
**      gctUINT32 OriginY
**          Specifies the origin of the pattern in 0..7 range.
**
**      gctUINT32 ColorConvert
**          The values of FgColor and BgColor parameters are stored directly in
**          internal color registers and are used either directly to initialize
**          pattern or converted to the format of destination before actually
**          used. The later happens if ColorConvert is not zero.
**
**      gctUINT32 FgColor
**      gctUINT32 BgColor
**          Foreground and background colors of the pattern. The values will be
**          used to initialize 8x8 pattern. If the values are in destination
**          format, set ColorConvert to 0. Otherwise, provide the values in
**          ARGB8 format and set ColorConvert to 1 to instruct the hardware to
**          convert the values to the destination format before they are
**          actually used.
**
**      gctUINT64 Bits
**          64 bits of pixel bits. Each bit represents one pixel and is used
**          to choose between foreground and background colors. If the bit
**          is 0, the background color is used; otherwise the foreground color
**          is used. The mapping between Bits parameter and the actual pattern
**          pixels is the same as of the Mask parameter.
**
**      gctUINT64 Mask
**          64 bits of mask, where each bit corresponds to one pixel of 8x8
**          pattern. Each bit of the mask is used to determine transparency
**          of the corresponding pixel, in other words, each mask bit is used
**          to select between foreground or background ROPs. If the bit is 0,
**          background ROP is used on the pixel; if 1, the foreground ROP
**          is used. The mapping between Mask parameter bits and the actual
**          pattern pixels is as follows:
**
**          +----+----+----+----+----+----+----+----+
**          |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
**          +----+----+----+----+----+----+----+----+
**          | 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |
**          +----+----+----+----+----+----+----+----+
**          | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 |
**          +----+----+----+----+----+----+----+----+
**          | 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 |
**          +----+----+----+----+----+----+----+----+
**          | 39 | 38 | 37 | 36 | 35 | 34 | 33 | 32 |
**          +----+----+----+----+----+----+----+----+
**          | 47 | 46 | 45 | 44 | 43 | 42 | 41 | 40 |
**          +----+----+----+----+----+----+----+----+
**          | 55 | 54 | 53 | 52 | 51 | 50 | 49 | 48 |
**          +----+----+----+----+----+----+----+----+
**          | 63 | 62 | 61 | 60 | 59 | 58 | 57 | 56 |
**          +----+----+----+----+----+----+----+----+
**
**  OUTPUT:
**
**      gcoBRUSH * Brush
**          Pointer to the variable that will hold the gcoBRUSH object pointer.
*/
gceSTATUS
gco2D_ConstructMonochromeBrush(
    IN gco2D Engine,
    IN gctUINT32 OriginX,
    IN gctUINT32 OriginY,
    IN gctUINT32 ColorConvert,
    IN gctUINT32 FgColor,
    IN gctUINT32 BgColor,
    IN gctUINT64 Bits,
    IN gctUINT64 Mask,
    gcoBRUSH * Brush
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_ConstructColorBrush
**
**  Create a color gcoBRUSH object.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 OriginX
**      gctUINT32 OriginY
**          Specifies the origin of the pattern in 0..7 range.
**
**      gctPOINTER Address
**          Location of the pattern bitmap in system memory.
**
**      gceSURF_FORMAT Format
**          Format of the source bitmap.
**
**      gctUINT64 Mask
**          64 bits of mask, where each bit corresponds to one pixel of 8x8
**          pattern. Each bit of the mask is used to determine transparency
**          of the corresponding pixel, in other words, each mask bit is used
**          to select between foreground or background ROPs. If the bit is 0,
**          background ROP is used on the pixel; if 1, the foreground ROP
**          is used. The mapping between Mask parameter bits and the actual
**          pattern pixels is as follows:
**
**          +----+----+----+----+----+----+----+----+
**          |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
**          +----+----+----+----+----+----+----+----+
**          | 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |
**          +----+----+----+----+----+----+----+----+
**          | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 |
**          +----+----+----+----+----+----+----+----+
**          | 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 |
**          +----+----+----+----+----+----+----+----+
**          | 39 | 38 | 37 | 36 | 35 | 34 | 33 | 32 |
**          +----+----+----+----+----+----+----+----+
**          | 47 | 46 | 45 | 44 | 43 | 42 | 41 | 40 |
**          +----+----+----+----+----+----+----+----+
**          | 55 | 54 | 53 | 52 | 51 | 50 | 49 | 48 |
**          +----+----+----+----+----+----+----+----+
**          | 63 | 62 | 61 | 60 | 59 | 58 | 57 | 56 |
**          +----+----+----+----+----+----+----+----+
**
**  OUTPUT:
**
**      gcoBRUSH * Brush
**          Pointer to the variable that will hold the gcoBRUSH object pointer.
*/
gceSTATUS
gco2D_ConstructColorBrush(
    IN gco2D Engine,
    IN gctUINT32 OriginX,
    IN gctUINT32 OriginY,
    IN gctPOINTER Address,
    IN gceSURF_FORMAT Format,
    IN gctUINT64 Mask,
    gcoBRUSH * Brush
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_Clear
**
**  Clear one or more rectangular areas.
**  The color is specified in A8R8G8B8 format.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 RectCount
**          The number of rectangles to draw. The array of line positions
**          pointed to by Position parameter must have at least RectCount
**          positions.
**
**      gcsRECT_PTR Rect
**          Points to an array of positions in (x0, y0)-(x1, y1) format.
**
**      gctUINT32 Color32
**          A8R8G8B8 clear color value.
**
**      gctUINT8 FgRop
**          Foreground ROP to use with opaque pixels.
**
**      gctUINT8 BgRop
**          Background ROP to use with transparent pixels.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_Clear(
    IN gco2D Engine,
    IN gctUINT32 RectCount,
    IN gcsRECT_PTR Rect,
    IN gctUINT32 Color32,
    IN gctUINT8 FgRop,
    IN gctUINT8 BgRop,
    IN gceSURF_FORMAT DstFormat
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_Line
**
**  Draw one or more Bresenham lines.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 LineCount
**          The number of lines to draw. The array of line positions pointed
**          to by Position parameter must have at least LineCount positions.
**
**      gcsRECT_PTR Position
**          Points to an array of positions in (x0, y0)-(x1, y1) format.
**
**      gcoBRUSH Brush
**          Brush to use for drawing.
**
**      gctUINT8 FgRop
**          Foreground ROP to use with opaque pixels.
**
**      gctUINT8 BgRop
**          Background ROP to use with transparent pixels.
**
**      gceSURF_FORMAT DstFormat
**          Format of the destination buffer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_Line(
    IN gco2D Engine,
    IN gctUINT32 LineCount,
    IN gcsRECT_PTR Position,
    IN gcoBRUSH Brush,
    IN gctUINT8 FgRop,
    IN gctUINT8 BgRop,
    IN gceSURF_FORMAT DstFormat
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;

    return status;
}

/*******************************************************************************
**
**  gco2D_ColorLine
**
**  Draw one or more Bresenham lines based on the 32-bit color.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 LineCount
**          The number of lines to draw. The array of line positions pointed
**          to by Position parameter must have at least LineCount positions.
**
**      gcsRECT_PTR Position
**          Points to an array of positions in (x0, y0)-(x1, y1) format.
**
**      gctUINT32 Color32
**          Source color in A8R8G8B8 format.
**
**      gctUINT8 FgRop
**          Foreground ROP to use with opaque pixels.
**
**      gctUINT8 BgRop
**          Background ROP to use with transparent pixels.
**
**      gceSURF_FORMAT DstFormat
**          Format of the destination buffer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_ColorLine(
    IN gco2D Engine,
    IN gctUINT32 LineCount,
    IN gcsRECT_PTR Position,
    IN gctUINT32 Color32,
    IN gctUINT8 FgRop,
    IN gctUINT8 BgRop,
    IN gceSURF_FORMAT DstFormat
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_Blit
**
**  Generic blit.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 RectCount
**          The number of rectangles to draw. The array of rectangle positions
**          pointed to by Rect parameter must have at least RectCount
**          positions.
**
**      gcsRECT_PTR Rect
**          Points to an array of positions in (x0, y0)-(x1, y1) format.
**
**      gctUINT8 FgRop
**          Foreground ROP to use with opaque pixels.
**
**      gctUINT8 BgRop
**          Background ROP to use with transparent pixels.
**
**      gceSURF_FORMAT DstFormat
**          Format of the destination buffer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_Blit(
    IN gco2D Engine,
    IN gctUINT32 RectCount,
    IN gcsRECT_PTR Rect,
    IN gctUINT8 FgRop,
    IN gctUINT8 BgRop,
    IN gceSURF_FORMAT DstFormat
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_BatchBlit
**
**  Generic blit for a batch of source destination rectangle pairs.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 RectCount
**          The number of rectangles to draw. The array of rectangle positions
**          pointed to by SrcRect and DstRect parameters must have at least
**          RectCount positions.
**
**      gcsRECT_PTR SrcRect
**          Points to an array of positions in (x0, y0)-(x1, y1) format.
**
**      gcsRECT_PTR DstRect
**          Points to an array of positions in (x0, y0)-(x1, y1) format.
**
**      gctUINT8 FgRop
**          Foreground ROP to use with opaque pixels.
**
**      gctUINT8 BgRop
**          Background ROP to use with transparent pixels.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_BatchBlit(
    IN gco2D Engine,
    IN gctUINT32 RectCount,
    IN gcsRECT_PTR SrcRect,
    IN gcsRECT_PTR DstRect,
    IN gctUINT8 FgRop,
    IN gctUINT8 BgRop,
    IN gceSURF_FORMAT DstFormat
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;

    return status;
}

/*******************************************************************************
**
**  gco2D_StretchBlit
**
**  Stretch blit.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 RectCount
**          The number of rectangles to draw. The array of rectangle positions
**          pointed to by Rect parameter must have at least RectCount
**          positions.
**
**      gcsRECT_PTR Rect
**          Points to an array of rectangles. All rectangles are assumed to be
**          of the same size.
**
**      gctUINT8 FgRop
**          Foreground ROP to use with opaque pixels.
**
**      gctUINT8 BgRop
**          Background ROP to use with transparent pixels.
**
**      gceSURF_FORMAT DstFormat
**          Format of the destination buffer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_StretchBlit(
    IN gco2D Engine,
    IN gctUINT32 RectCount,
    IN gcsRECT_PTR Rect,
    IN gctUINT8 FgRop,
    IN gctUINT8 BgRop,
    IN gceSURF_FORMAT DstFormat
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;

    return status;
}

/*******************************************************************************
**
**  gco2D_MonoBlit
**
**  Monochrome blit.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctPOINTER StreamBits
**          Pointer to the monochrome bitmap.
**
**      gcsPOINT_PTR StreamSize
**          Size of the monochrome bitmap in pixels.
**
**      gcsRECT_PTR StreamRect
**          Bounding rectangle of the area within the bitmap to render.
**
**      gceSURF_MONOPACK SrcStreamPack
**          Source bitmap packing.
**
**      gceSURF_MONOPACK DstStreamPack
**          Packing of the bitmap in the command stream.
**
**      gcsRECT_PTR DstRect
**          Pointer to an array of destination rectangles.
**
**      gctUINT32 FgRop
**          Foreground and background ROP codes.
**
**      gctUINT32 BgRop
**          Background ROP to use with transparent pixels.
**
**      gceSURF_FORMAT DstFormat
**          Dstination surface format.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_MonoBlit(
    IN gco2D Engine,
    IN gctPOINTER StreamBits,
    IN gcsPOINT_PTR StreamSize,
    IN gcsRECT_PTR StreamRect,
    IN gceSURF_MONOPACK SrcStreamPack,
    IN gceSURF_MONOPACK DstStreamPack,
    IN gcsRECT_PTR DstRect,
    IN gctUINT32 FgRop,
    IN gctUINT32 BgRop,
    IN gceSURF_FORMAT DstFormat
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_SetKernelSize
**
**  Set kernel size.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT8 HorKernelSize
**          Kernel size for the horizontal pass.
**
**      gctUINT8 VerKernelSize
**          Kernel size for the vertical pass.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetKernelSize(
    IN gco2D Engine,
    IN gctUINT8 HorKernelSize,
    IN gctUINT8 VerKernelSize
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetFilterType
**
**  Set filter type.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gceFILTER_TYPE FilterType
**          Filter type for the filter blit.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetFilterType(
    IN gco2D Engine,
    IN gceFILTER_TYPE FilterType
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetUserFilterKernel
**
**  Set the user defined filter kernel.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gceFILTER_PASS_TYPE PassType
**          Pass type for the filter blit.
**
**      gctUINT16_PTR KernelArray
**          Pointer to the kernel array from user.
**
**      gctINT ArrayLen
**          Length of the kernel array in bytes.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetUserFilterKernel(
    IN gco2D Engine,
    IN gceFILTER_PASS_TYPE PassType,
    IN gctUINT16_PTR KernelArray
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_EnableUserFilterPasses
**
**  Select the pass(es) to be done for user defined filter.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctBOOL HorPass
**          Enable horizontal filter pass if HorPass is gcvTRUE.
**          Otherwise disable this pass.
**
**      gctBOOL VerPass
**          Enable vertical filter pass if VerPass is gcvTRUE.
**          Otherwise disable this pass.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_EnableUserFilterPasses(
    IN gco2D Engine,
    IN gctBOOL HorPass,
    IN gctBOOL VerPass
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_FreeFilterBuffer
**
**  Frees the temporary buffer allocated by filter blit operation.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_FreeFilterBuffer(
    IN gco2D Engine
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;

    return status;
}

/*******************************************************************************
**
**  gco2D_FilterBlit
**
**  Filter blit. This function is deprecated. Please use gco2D_ FilterBlitEx2
**  instead.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 SrcAddress
**          Base address of the source surface in local memory.
**
**      gctUINT SrcStride
**          Stride of the source surface in bytes.
**
**      gctUINT32 SrcUAddress
**          Base address of U channel of the source surface in local memory for YUV format.
**
**      gctUINT SrcUStride
**          Stride of U channel of the source surface in bytes for YUV format.
**
**      gctUINT32 SrcVAddress
**          Base address of V channel of the source surface in local memory for YUV format.
**
**      gctUINT SrcVStride
**          Stride of of V channel the source surface in bytes for YUV format.
**
**      gceSURF_FORMAT SrcFormat
**          Format of the source surface.
**
**      gceSURF_ROTATION SrcRotation
**          Specifies the source surface rotation angle.
**
**      gctUINT32 SrcSurfaceWidth
**          The width in pixels of the source surface.
**
**      gcsRECT_PTR SrcRect
**          Coordinates of the entire source image.
**
**      gctUINT32 DstAddress
**          Base address of the destination surface in local memory.
**
**      gctUINT DstStride
**          Stride of the destination surface in bytes.
**
**      gceSURF_FORMAT DstFormat
**          Format of the destination surface.
**
**      gceSURF_ROTATION DstRotation
**          Specifies the destination surface rotation angle.
**
**      gctUINT32 DstSurfaceWidth
**          The width in pixels of the destination surface.
**
**      gcsRECT_PTR DstRect
**          Coordinates of the entire destination image.
**
**      gcsRECT_PTR DstSubRect
**          Coordinates of a sub area within the destination to render.
**          If DstSubRect is gcvNULL, the complete image will be rendered
**          using coordinates set by DstRect.
**          If DstSubRect is not gcvNULL and DstSubRect and DstRect are
**          no equal, DstSubRect is assumed to be within DstRect and
**          will be used to render the sub area only.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_FilterBlit(
    IN gco2D Engine,
    IN gctUINT32 SrcAddress,
    IN gctUINT SrcStride,
    IN gctUINT32 SrcUAddress,
    IN gctUINT SrcUStride,
    IN gctUINT32 SrcVAddress,
    IN gctUINT SrcVStride,
    IN gceSURF_FORMAT SrcFormat,
    IN gceSURF_ROTATION SrcRotation,
    IN gctUINT32 SrcSurfaceWidth,
    IN gcsRECT_PTR SrcRect,
    IN gctUINT32 DstAddress,
    IN gctUINT DstStride,
    IN gceSURF_FORMAT DstFormat,
    IN gceSURF_ROTATION DstRotation,
    IN gctUINT32 DstSurfaceWidth,
    IN gcsRECT_PTR DstRect,
    IN gcsRECT_PTR DstSubRect
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;

    return status;
}

/*******************************************************************************
**
**  gco2D_FilterBlitEx
**
**  Filter blit. This function is deprecated. Please use gco2D_ FilterBlitEx2
**  instead.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 SrcAddress
**          Base address of the source surface in local memory.
**
**      gctUINT SrcStride
**          Stride of the source surface in bytes.
**
**      gctUINT32 SrcUAddress
**          Base address of U channel of the source surface in local memory for YUV format.
**
**      gctUINT SrcUStride
**          Stride of U channel of the source surface in bytes for YUV format.
**
**      gctUINT32 SrcVAddress
**          Base address of V channel of the source surface in local memory for YUV format.
**
**      gctUINT SrcVStride
**          Stride of of V channel the source surface in bytes for YUV format.
**
**      gceSURF_FORMAT SrcFormat
**          Format of the source surface.
**
**      gceSURF_ROTATION SrcRotation
**          Specifies the source surface rotation angle.
**
**      gctUINT32 SrcSurfaceWidth
**          The width in pixels of the source surface.
**
**      gctUINT32 SrcSurfaceHeight
**          The height in pixels of the source surface for the rotaion in PE 2.0.
**
**      gcsRECT_PTR SrcRect
**          Coordinates of the entire source image.
**
**      gctUINT32 DstAddress
**          Base address of the destination surface in local memory.
**
**      gctUINT DstStride
**          Stride of the destination surface in bytes.
**
**      gceSURF_FORMAT DstFormat
**          Format of the destination surface.
**
**      gceSURF_ROTATION DstRotation
**          Specifies the destination surface rotation angle.
**
**      gctUINT32 DstSurfaceWidth
**          The width in pixels of the destination surface.
**
**      gctUINT32 DstSurfaceHeight
**          The height in pixels of the destination surface for the rotation in PE 2.0.
**
**      gcsRECT_PTR DstRect
**          Coordinates of the entire destination image.
**
**      gcsRECT_PTR DstSubRect
**          Coordinates of a sub area within the destination to render.
**          If DstSubRect is gcvNULL, the complete image will be rendered
**          using coordinates set by DstRect.
**          If DstSubRect is not gcvNULL and DstSubRect and DstRect are
**          no equal, DstSubRect is assumed to be within DstRect and
**          will be used to render the sub area only.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_FilterBlitEx(
    IN gco2D Engine,
    IN gctUINT32 SrcAddress,
    IN gctUINT SrcStride,
    IN gctUINT32 SrcUAddress,
    IN gctUINT SrcUStride,
    IN gctUINT32 SrcVAddress,
    IN gctUINT SrcVStride,
    IN gceSURF_FORMAT SrcFormat,
    IN gceSURF_ROTATION SrcRotation,
    IN gctUINT32 SrcSurfaceWidth,
    IN gctUINT32 SrcSurfaceHeight,
    IN gcsRECT_PTR SrcRect,
    IN gctUINT32 DstAddress,
    IN gctUINT DstStride,
    IN gceSURF_FORMAT DstFormat,
    IN gceSURF_ROTATION DstRotation,
    IN gctUINT32 DstSurfaceWidth,
    IN gctUINT32 DstSurfaceHeight,
    IN gcsRECT_PTR DstRect,
    IN gcsRECT_PTR DstSubRect
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_FilterBlitEx2
**
**  Filter blit.
**
**  Note:
**      If the output format is multi planar YUV, do only color conversion.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 SrcAddress
**          Base address of the source surface in local memory.
**
**      gctUINT SrcStride
**          Stride of the source surface in bytes.
**
**      gctUINT32 SrcUAddress
**          Base address of U channel of the source surface in local memory for YUV format.
**
**      gctUINT SrcUStride
**          Stride of U channel of the source surface in bytes for YUV format.
**
**      gctUINT32 SrcVAddress
**          Base address of V channel of the source surface in local memory for YUV format.
**
**      gctUINT SrcVStride
**          Stride of of V channel the source surface in bytes for YUV format.
**
**      gceSURF_FORMAT SrcFormat
**          Format of the source surface.
**
**      gceSURF_ROTATION SrcRotation
**          Specifies the source surface rotation angle.
**
**      gctUINT32 SrcSurfaceWidth
**          The width in pixels of the source surface.
**
**      gctUINT32 SrcSurfaceHeight
**          The height in pixels of the source surface for the rotaion in PE 2.0.
**
**      gcsRECT_PTR SrcRect
**          Coordinates of the entire source image.
**
**      gctUINT32 DstAddress
**          Base address of the destination surface in local memory.
**
**      gctUINT DstStride
**          Stride of the destination surface in bytes.
**
**      gceSURF_FORMAT DstFormat
**          Format of the destination surface.
**
**      gceSURF_ROTATION DstRotation
**          Specifies the destination surface rotation angle.
**
**      gctUINT32 DstSurfaceWidth
**          The width in pixels of the destination surface.
**
**      gctUINT32 DstSurfaceHeight
**          The height in pixels of the destination surface for the rotaion in PE 2.0.
**
**      gcsRECT_PTR DstRect
**          Coordinates of the entire destination image.
**
**      gcsRECT_PTR DstSubRect
**          Coordinates of a sub area within the destination to render.
**          If DstSubRect is gcvNULL, the complete image will be rendered
**          using coordinates set by DstRect.
**          If DstSubRect is not gcvNULL and DstSubRect and DstRect are
**          no equal, DstSubRect is assumed to be within DstRect and
**          will be used to render the sub area only.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_FilterBlitEx2(
    IN gco2D                Engine,
    IN gctUINT32_PTR        SrcAddresses,
    IN gctUINT32            SrcAddressNum,
    IN gctUINT32_PTR        SrcStrides,
    IN gctUINT32            SrcStrideNum,
    IN gceTILING            SrcTiling,
    IN gceSURF_FORMAT       SrcFormat,
    IN gceSURF_ROTATION     SrcRotation,
    IN gctUINT32            SrcSurfaceWidth,
    IN gctUINT32            SrcSurfaceHeight,
    IN gcsRECT_PTR          SrcRect,
    IN gctUINT32_PTR        DstAddresses,
    IN gctUINT32            DstAddressNum,
    IN gctUINT32_PTR        DstStrides,
    IN gctUINT32            DstStrideNum,
    IN gceTILING            DstTiling,
    IN gceSURF_FORMAT       DstFormat,
    IN gceSURF_ROTATION     DstRotation,
    IN gctUINT32            DstSurfaceWidth,
    IN gctUINT32            DstSurfaceHeight,
    IN gcsRECT_PTR          DstRect,
    IN gcsRECT_PTR          DstSubRect
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_EnableAlphaBlend
**
**  For normal Blit,
**   On old PE (<2.0), ROP4 must be set as 0x8888 when enable alpha blending.
**   On new PE (>=2.0), ROP4 is not limited when enable alpha blending.
**
**  For FilterBlit:
**   ROP4 is always set as 0xCCCC internally.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT8 SrcGlobalAlphaValue
**      gctUINT8 DstGlobalAlphaValue
**          Global alpha value for the color components.
**
**      gceSURF_PIXEL_ALPHA_MODE SrcAlphaMode
**      gceSURF_PIXEL_ALPHA_MODE DstAlphaMode
**          Per-pixel alpha component mode.
**
**      gceSURF_GLOBAL_ALPHA_MODE SrcGlobalAlphaMode
**      gceSURF_GLOBAL_ALPHA_MODE DstGlobalAlphaMode
**          Global/per-pixel alpha values selection.
**
**      gceSURF_BLEND_FACTOR_MODE SrcFactorMode
**      gceSURF_BLEND_FACTOR_MODE DstFactorMode
**          Final blending factor mode.
**
**      gceSURF_PIXEL_COLOR_MODE SrcColorMode
**      gceSURF_PIXEL_COLOR_MODE DstColorMode
**          Per-pixel color component mode.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_EnableAlphaBlend(
    IN gco2D Engine,
    IN gctUINT8 SrcGlobalAlphaValue,
    IN gctUINT8 DstGlobalAlphaValue,
    IN gceSURF_PIXEL_ALPHA_MODE SrcAlphaMode,
    IN gceSURF_PIXEL_ALPHA_MODE DstAlphaMode,
    IN gceSURF_GLOBAL_ALPHA_MODE SrcGlobalAlphaMode,
    IN gceSURF_GLOBAL_ALPHA_MODE DstGlobalAlphaMode,
    IN gceSURF_BLEND_FACTOR_MODE SrcFactorMode,
    IN gceSURF_BLEND_FACTOR_MODE DstFactorMode,
    IN gceSURF_PIXEL_COLOR_MODE SrcColorMode,
    IN gceSURF_PIXEL_COLOR_MODE DstColorMode
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_EnableAlphaBlendAdvanced
**
**  Enable alpha blending engine in the hardware.
**
**  This function is only working with PE 2.0 and above.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gceSURF_PIXEL_ALPHA_MODE SrcAlphaMode
**      gceSURF_PIXEL_ALPHA_MODE DstAlphaMode
**          Per-pixel alpha component mode.
**
**      gceSURF_GLOBAL_ALPHA_MODE SrcGlobalAlphaMode
**      gceSURF_GLOBAL_ALPHA_MODE DstGlobalAlphaMode
**          Global/per-pixel alpha values selection.
**
**      gceSURF_BLEND_FACTOR_MODE SrcFactorMode
**      gceSURF_BLEND_FACTOR_MODE DstFactorMode
**          Final blending factor mode.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_EnableAlphaBlendAdvanced(
    IN gco2D Engine,
    IN gceSURF_PIXEL_ALPHA_MODE SrcAlphaMode,
    IN gceSURF_PIXEL_ALPHA_MODE DstAlphaMode,
    IN gceSURF_GLOBAL_ALPHA_MODE SrcGlobalAlphaMode,
    IN gceSURF_GLOBAL_ALPHA_MODE DstGlobalAlphaMode,
    IN gceSURF_BLEND_FACTOR_MODE SrcFactorMode,
    IN gceSURF_BLEND_FACTOR_MODE DstFactorMode
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetPorterDuffBlending
**
**  Enable alpha blending engine in the hardware and setup the blending modes
**  using the Porter Duff rule defined.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gce2D_PORTER_DUFF_RULE Rule
**          Porter Duff blending rule.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetPorterDuffBlending(
    IN gco2D Engine,
    IN gce2D_PORTER_DUFF_RULE Rule
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_DisableAlphaBlend
**
**  Disable alpha blending engine in the hardware and engage the ROP engine.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_DisableAlphaBlend(
    IN gco2D Engine
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_GetPackSize
**
**  Retrieve monochrome stream pack size.
**
**  INPUT:
**
**      gceSURF_MONOPACK StreamPack
**          Stream pack code.
**
**  OUTPUT:
**
**      gctUINT32 * PackWidth
**      gctUINT32 * PackHeight
**          Monochrome stream pack size.
*/
gceSTATUS
gco2D_GetPackSize(
    IN gceSURF_MONOPACK StreamPack,
    OUT gctUINT32 * PackWidth,
    OUT gctUINT32 * PackHeight
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_Flush
**
**  Flush the 2D pipeline.
**
**  INPUT:
**      gco2D Engine
**          Pointer to an gco2D object.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gco2D_Flush(
    IN gco2D Engine
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_LoadPalette
**
**  Load 256-entry color table for INDEX8 source surfaces.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gctUINT FirstIndex
**          The index to start loading from (0..255).
**
**      gctUINT IndexCount
**          The number of indices to load (FirstIndex + IndexCount <= 256).
**
**      gctPOINTER ColorTable
**          Pointer to the color table to load. The value of the pointer should
**          be set to the first value to load no matter what the value of
**          FirstIndex is. The table must consist of 32-bit entries that contain
**          color values in either ARGB8 or the destination color format
**          (see ColorConvert).
**
**      gctBOOL ColorConvert
**          If set to gcvTRUE, the 32-bit values in the table are assumed to be
**          in ARGB8 format and will be converted by the hardware to the
**          destination format as needed.
**          If set to gcvFALSE, the 32-bit values in the table are assumed to be
**          preconverted to the destination format.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_LoadPalette(
    IN gco2D Engine,
    IN gctUINT FirstIndex,
    IN gctUINT IndexCount,
    IN gctPOINTER ColorTable,
    IN gctBOOL ColorConvert
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_SetBitBlitMirror
**
**  Enable/disable 2D BitBlt mirrorring.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gctBOOL HorizontalMirror
**          Horizontal mirror enable flag.
**
**      gctBOOL VerticalMirror
**          Vertical mirror enable flag.
**
**  OUTPUT:
**
**      gceSTATUS
**          Returns gcvSTATUS_OK if successful.
*/
gceSTATUS
gco2D_SetBitBlitMirror(
    IN gco2D Engine,
    IN gctBOOL HorizontalMirror,
    IN gctBOOL VerticalMirror
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetTransparencyAdvancedEx
**
**  Setup the source, target and pattern transparency modes.
**  It also enable or disable DFB color key mode.
**
**  This function is only working with full DFB 2D core.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gce2D_TRANSPARENCY SrcTransparency
**          Source Transparency.
**
**      gce2D_TRANSPARENCY DstTransparency
**          Destination Transparency.
**
**      gce2D_TRANSPARENCY PatTransparency
**          Pattern Transparency.
**
**      gctBOOL EnableDFBColorKeyMode
**          Enable/disable DFB color key mode.
**          The transparent pixels will be bypassed when
**          enabling DFB color key mode. Otherwise those
**          pixels maybe processed by the following pipes.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetTransparencyAdvancedEx(
    IN gco2D Engine,
    IN gce2D_TRANSPARENCY SrcTransparency,
    IN gce2D_TRANSPARENCY DstTransparency,
    IN gce2D_TRANSPARENCY PatTransparency,
    IN gctBOOL EnableDFBColorKeyMode
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetTransparencyAdvanced
**
**  Set the transparency for source, destination and pattern. This function is
**  deprecated. Please use gco2D_SetTransparencyAdvancedEx instead.
**
**  This function is only working with PE 2.0 and above.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gce2D_TRANSPARENCY SrcTransparency
**          Source Transparency.
**
**      gce2D_TRANSPARENCY DstTransparency
**          Destination Transparency.
**
**      gce2D_TRANSPARENCY PatTransparency
**          Pattern Transparency.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetTransparencyAdvanced(
    IN gco2D Engine,
    IN gce2D_TRANSPARENCY SrcTransparency,
    IN gce2D_TRANSPARENCY DstTransparency,
    IN gce2D_TRANSPARENCY PatTransparency
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetROP
**
**  Set the ROP for source, destination and pattern.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gctUINT8 FgROP
**          Foreground ROP.
**
**      gctUINT8 BgROP
**          Background ROP.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetROP(
    IN gco2D Engine,
    IN gctUINT8 FgROP,
    IN gctUINT8 BgROP
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetSourceColorKeyAdvanced
**
**  Set the source color key.
**  Color channel values should specified in the range allowed by the source format.
**  When target format is A8, only Alpha component is used. Otherwise, Alpha component
**  is not used.
**
**  This function is only working with PE 2.0 and above.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gctUINT32 ColorKey
**          The color key value in A8R8G8B8 format.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetSourceColorKeyAdvanced(
    IN gco2D Engine,
    IN gctUINT32 ColorKey
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;

    return status;
}

/*******************************************************************************
**
**  gco2D_SetSourceColorKeyRangeAdvanced
**
**  Set the source color key range.
**  Color channel values should specified in the range allowed by the source format.
**  Lower color key's color channel values should be less than or equal to
**  the corresponding color channel value of ColorKeyHigh.
**  When target format is A8, only Alpha components are used. Otherwise, Alpha
**  components are not used.
**
**  This function is only working with PE 2.0 and above.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gctUINT32 ColorKeyLow
**          The low color key value in A8R8G8B8 format.
**
**      gctUINT8 ColorKeyHigh
**          The high color key value in A8R8G8B8 format.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetSourceColorKeyRangeAdvanced(
    IN gco2D Engine,
    IN gctUINT32 ColorKeyLow,
    IN gctUINT32 ColorKeyHigh
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetTargetColorKeyAdvanced
**
**  Set the target color key.
**  Color channel values should specified in the range allowed by the target format.
**  When target format is A8, only Alpha component is used. Otherwise, Alpha component
**  is not used.
**
**  This function is only working with PE 2.0 and above.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gctUINT32 ColorKey
**          The color key value in A8R8G8B8 format.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetTargetColorKeyAdvanced(
    IN gco2D Engine,
    IN gctUINT32 ColorKey
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetTargetColorKeyRangeAdvanced
**
**  Set the source color key range.
**  Color channel values should specified in the range allowed by the target format.
**  Lower color key's color channel values should be less than or equal to
**  the corresponding color channel value of ColorKeyHigh.
**  When target format is A8, only Alpha components are used. Otherwise, Alpha
**  components are not used.
**
**  This function is only working with PE 2.0 and above.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gctUINT32 ColorKeyLow
**          The low color key value in A8R8G8B8 format.
**
**      gctUINT32 ColorKeyHigh
**          The high color key value in A8R8G8B8 format.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetTargetColorKeyRangeAdvanced(
    IN gco2D Engine,
    IN gctUINT32 ColorKeyLow,
    IN gctUINT32 ColorKeyHigh
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetYUVColorMode
**
**  Set the YUV color space mode.
**
**  This function is only working with PE 2.0 and above.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gce2D_YUV_COLOR_MODE Mode
**          Mode is 601, 709 or user defined conversion.
**          gcv2D_YUV_USER_DEFINED_CLAMP means clampping Y to [16, 235], and
**          U/V to [16, 240].
**
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetYUVColorMode(
    IN gco2D Engine,
    IN gce2D_YUV_COLOR_MODE Mode
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetSourceGlobalColorAdvanced
**
**  Set the source global color value in A8R8G8B8 format.
**
**  This function is only working with PE 2.0 and above.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gctUINT32 Color32
**          Source color.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS gco2D_SetSourceGlobalColorAdvanced(
    IN gco2D Engine,
    IN gctUINT32 Color32
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetTargetGlobalColor
**
**  Set the source global color value in A8R8G8B8 format.
**
**  This function is only working with PE 2.0 and above.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gctUINT32 Color32
**          Target color.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS gco2D_SetTargetGlobalColorAdvanced(
    IN gco2D Engine,
    IN gctUINT32 Color32
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetPixelMultiplyModesAdvanced
**
**  Set the source and target pixel multiply modes.
**
**  This function is only working with PE 2.0 and above.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gce2D_PIXEL_COLOR_MULTIPLY_MODE SrcPremultiplySrcAlpha
**          Source color premultiply with Source Alpha.
**
**      gce2D_PIXEL_COLOR_MULTIPLY_MODE DstPremultiplyDstAlpha
**          Destination color premultiply with Destination Alpha.
**
**      gce2D_GLOBAL_COLOR_MULTIPLY_MODE SrcPremultiplyGlobalMode
**          Source color premultiply with Global color's Alpha or Color.
**
**      gce2D_PIXEL_COLOR_MULTIPLY_MODE DstDemultiplyDstAlpha
**          Destination color demultiply with Destination Alpha.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS gco2D_SetPixelMultiplyModeAdvanced(
    IN gco2D Engine,
    IN gce2D_PIXEL_COLOR_MULTIPLY_MODE SrcPremultiplySrcAlpha,
    IN gce2D_PIXEL_COLOR_MULTIPLY_MODE DstPremultiplyDstAlpha,
    IN gce2D_GLOBAL_COLOR_MULTIPLY_MODE SrcPremultiplyGlobalMode,
    IN gce2D_PIXEL_COLOR_MULTIPLY_MODE DstDemultiplyDstAlpha
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetAutoFlushCycles
**
**  Set the GPU clock cycles, after which the idle 2D engine
**  will trigger a flush.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      UINT32 Cycles
**          Source color premultiply with Source Alpha.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS gco2D_SetAutoFlushCycles(
    IN gco2D Engine,
    IN gctUINT32 Cycles
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;

    return status;
}

/*******************************************************************************
**
**  gco2D_ProfileEngine
**
**  Read the profile registers available in the 2D engine and set them in the profile.
**  pixelsRendered counter is reset to 0 after reading.
**
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      OPTIONAL gcs2D_PROFILE_PTR Profile
**          Pointer to a gcs2D_Profile structure.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_ProfileEngine(
    IN gco2D Engine,
    OPTIONAL gcs2D_PROFILE_PTR Profile
    )
{
#if (VIVANTE_PROFILER && 0)
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Profile=0x%x", Engine, Profile);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_2D);

    /* Read all 2D profile registers. */
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_PROFILE_REGISTERS_2D;
    iface.u.RegisterProfileData2D.hwProfile2D = gcmPTR_TO_UINT64(Profile);

    /* Call the kernel. */
    status = gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, gcmSIZEOF(iface),
        &iface, gcmSIZEOF(iface)
        );

    /* Return status. */
    gcmFOOTER();
    return status;
#else
    return gcvSTATUS_NOT_SUPPORTED;
#endif /* VIVANTE_PROFILER */
}

/*******************************************************************************
**
**  gco2D_EnableDither.
**
**  Enable or disable dithering.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable dithering, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_EnableDither(
    IN gco2D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetGenericSource
**
**  Configure color source for linear, tile, super-tile, multi-tile. Also for
**      YUV format source.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 Address
**          Source surface base addresses.
**
**      gctUINT32 AddressNum
**          Number of source surface base addresses.
**
**      gctUINT32 Strides
**          Strides of the source surface in bytes.
**
**      gctUINT32 StrideNum
**          Number of stride of the source surface.
**
**      gceSURF_FORMAT Format
**          Color format of the source surface.
**
**      gceSURF_ROTATION Rotation
**          Type of rotation.
**
**      gctUINT32 SurfaceWidth
**          Required only if the surface is rotated. Equal to the width
**          of the source surface in pixels.
**
**      gctUINT32 SurfaceHeight
**          Required only if the surface is rotated in PE2.0. Equal to the height
**          of the source surface in pixels.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetGenericSource(
    IN gco2D               Engine,
    IN gctUINT32_PTR       Addresses,
    IN gctUINT32           AddressNum,
    IN gctUINT32_PTR       Strides,
    IN gctUINT32           StrideNum,
    IN gceTILING           Tiling,
    IN gceSURF_FORMAT      Format,
    IN gceSURF_ROTATION    Rotation,
    IN gctUINT32           SurfaceWidth,
    IN gctUINT32           SurfaceHeight
)
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_SetGenericTarget
**
**  Configure color source for linear, tile, super-tile, multi-tile. Also for
**      YUV format source.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 Address
**          Source surface base addresses.
**
**      gctUINT32 AddressNum
**          Number of source surface base addresses.
**
**      gctUINT32 Strides
**          Strides of the source surface in bytes.
**
**      gctUINT32 StrideNum
**          Number of stride of the source surface.
**
**      gceSURF_FORMAT Format
**          Color format of the source surface.
**
**      gceSURF_ROTATION Rotation
**          Type of rotation.
**
**      gctUINT32 SurfaceWidth
**          Required only if the surface is rotated. Equal to the width
**          of the source surface in pixels.
**
**      gctUINT32 SurfaceHeight
**          Required only if the surface is rotated in PE2.0. Equal to the height
**          of the source surface in pixels.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetGenericTarget(
    IN gco2D               Engine,
    IN gctUINT32_PTR       Addresses,
    IN gctUINT32           AddressNum,
    IN gctUINT32_PTR       Strides,
    IN gctUINT32           StrideNum,
    IN gceTILING           Tiling,
    IN gceSURF_FORMAT      Format,
    IN gceSURF_ROTATION    Rotation,
    IN gctUINT32           SurfaceWidth,
    IN gctUINT32           SurfaceHeight
)
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetCurrentSource
**
**  Support multi-source.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 Address
**          Source surface base addresses.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_SetCurrentSourceIndex(
    IN gco2D        Engine,
    IN gctUINT32    SrcIndex
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_MultiSourceBlit
**
**  Multi blit with mulit sources.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to an gco2D object.
**
**      gctUINT32 SourceMask
**          Indicate which source of the total 4 or 8 would be used to do
**          MultiSrcBlit(composition). Bit N represents the source index N.
**
**      gcsRECT_PTR Rect
**          Points to an array of positions in (x0, y0)-(x1, y1) format.
**
**      gctUINT32 RectCount
**          The number of rectangles to draw. The array of rectangle positions
**          pointed to by Rect parameter must have at least RectCount
**          positions. If RectCount equals to 0, use multi dest rectangle set
**          through gco2D_SetTargetRect for every source rectangle.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_MultiSourceBlit(
    IN gco2D Engine,
    IN gctUINT32 SourceMask,
    IN gcsRECT_PTR DstRect,
    IN gctUINT32 RectCount
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}

/*******************************************************************************
**
**  gco2D_SetGdiStretchMode
**
**  Enable/disable 2D GDI stretch mode for integral multiple stretch.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gctBOOL Enable
**          Enable/disable integral multiple stretch.
**
**  OUTPUT:
**
**      gceSTATUS
**          Returns gcvSTATUS_OK if successful.
*/
gceSTATUS
gco2D_SetGdiStretchMode(
    IN gco2D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetSourceTileStatus
**
**  Config tile status for source surface.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gce2D_TILE_STATUS_CONFIG TSControl
**          Config tile status.
**
**      gceSURF_FORMAT CompressedFormat
**          Compressed format.
**
**      gctUINT32 ClearValue
**          Value for tiles that are marked as clear.
**
**      gctUINT32 GpuAddress
**          GpuAddress for tile status buffer.
**
**  OUTPUT:
**
**      gceSTATUS
**          Returns gcvSTATUS_OK if successful.
*/
gceSTATUS
gco2D_SetSourceTileStatus(
    IN gco2D Engine,
    IN gce2D_TILE_STATUS_CONFIG TileStatusConfig,
    IN gceSURF_FORMAT CompressedFormat,
    IN gctUINT32 ClearValue,
    IN gctUINT32 GpuAddress
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetTargetTileStatus
**
**  Config tile status for target surface.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gce2D_TILE_STATUS_CONFIG TSControl
**          Config tile status.
**
**      gceSURF_FORMAT CompressedFormat
**          Compressed format.
**
**      gctUINT32 ClearValue
**          Value for tiles that are marked as clear.
**
**      gctUINT32 GpuAddress
**          GpuAddress for tile status buffer.
**
**  OUTPUT:
**
**      gceSTATUS
**          Returns gcvSTATUS_OK if successful.
*/
gceSTATUS
gco2D_SetTargetTileStatus(
    IN gco2D Engine,
    IN gce2D_TILE_STATUS_CONFIG TileStatusConfig,
    IN gceSURF_FORMAT CompressedFormat,
    IN gctUINT32 ClearValue,
    IN gctUINT32 GpuAddress
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_QueryU32
**
**  Query 2D engine for unsigned 32 bit information.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gce2D_QUERY Item
**          Item to query.
**
**  OUTPUT:
**
**      gceSTATUS
**          Returns gcvSTATUS_OK if successful.
**
**      gctUINT32_PTR Value
**          Value for the queried item.
**
*/
gceSTATUS
gco2D_QueryU32(
    IN gco2D Engine,
    IN gce2D_QUERY Item,
    OUT gctUINT32_PTR Value
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetStateU32
**
**  Set 2D engine state with unsigned 32 bit information.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gce2D_STATE State
**          State to change.
**
**      gctUINT32 Value
**          Value for State.
**
**
**  SUPPORTED State:
**
**      gcv2D_STATE_SPECIAL_FILTER_MIRROR_MODE
**          Mirror sub rectangles.
**
**      gcv2D_STATE_SUPER_TILE_VERSION
**          Support different super tile versions of
**          gcv2D_SUPER_TILE_VERSION_V1, gcv2D_SUPER_TILE_VERSION_V2
**          and gcv2D_SUPER_TILE_VERSION_V1.
**
**      gcv2D_STATE_EN_GAMMA
**          Enalbe or disable the en-GAMMA pass for output.
**
**      gcv2D_STATE_DE_GAMMA
**          Enalbe or disable the de-GAMMA pass for input.
**
**      gcv2D_STATE_PROFILE_ENABLE
**          Support different profile flags of gcv2D_STATE_PROFILE_NONE,
**          gcv2D_STATE_PROFILE_COMMAND, gcv2D_STATE_PROFILE_SURFACE and
**          gcv2D_STATE_PROFILE_ALL.
**
**      gcv2D_STATE_XRGB_ENABLE
**          Enalbe or disable XRGB.
**
****
**  OUTPUT:
**
**      gceSTATUS
**          Returns gcvSTATUS_OK if successful.
**
*/
gceSTATUS
gco2D_SetStateU32(
    IN gco2D Engine,
    IN gce2D_STATE State,
    IN gctUINT32 Value
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetStateArrayU32
**
**  Set 2D engine state with array of unsigned 32 integer.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gce2D_STATE State
**          State to change.
**
**      gctUINT32_PTR Array
**          Pointer to input array.
**
**      gctINT32 ArraySize
**          Size of Array.
**
**  OUTPUT:
**
**      gceSTATUS
**          Returns gcvSTATUS_OK if successful.
**
**
**  SUPPORTED State:
**
**      gcv2D_STATE_ARRAY_EN_GAMMA
**          For every item of Array, bits[9:0] are for RED, bits[19:10] are for
**          GREEN, bits[29:20] are for BLUE. ArraySize must be
**          gcd2D_GAMMA_TABLE_SIZE.
**
**      gcv2D_STATE_ARRAY_DE_GAMMA
**          For every item of Array, bits[7:0] are for RED, bits[15:8] are for
**          GREEN, bits[23:16] are for BLUE. ArraySize must be
**          gcd2D_GAMMA_TABLE_SIZE.
**
*/
gceSTATUS
gco2D_SetStateArrayU32(
    IN gco2D Engine,
    IN gce2D_STATE State,
    IN gctUINT32_PTR Array,
    IN gctINT32 ArraySize
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_SetStateArrayI32
**
**  Set 2D engine state with array of signed 32 integer.
**
**  INPUT:
**
**      gco2D Engine
**          Pointer to the gco2D object.
**
**      gce2D_STATE State
**          State to change.
**
**      gctUINT32_PTR Array
**          Pointer to input array.
**
**      gctINT32 ArraySize
**          Size of Array.
**
**  OUTPUT:
**
**      gceSTATUS
**          Returns gcvSTATUS_OK if successful.
**
**
**  SUPPORTED State:
**
**      gce2D_STATE_ARRAY_CSC_YUV_TO_RGB
**      gce2D_STATE_ARRAY_CSC_RGB_TO_YUV
**
**      The formula for gce2D_STATE_ARRAY_CSC_RGB_TO_YUV
**        Y = ((C00*R + C01*G + C02*B) + D0 + 128)>>8
**        U = ((C10*R + C11*G + C12*B) + D1 + 128)>>8
**        V = ((C20*R + C21*G + C22*B) + D2 + 128)>>8
**
**      The formula for gce2D_STATE_ARRAY_CSC_YUV_TO_RGB
**        R = ((C00*Y + C01*U + C02*V) + D0 + 128)>>8
**        G = ((C10*Y + C11*U + C12*V) + D1 + 128)>>8
**        B = ((C20*Y + C21*U + C22*V) + D2 + 128)>>8
**
**        Array contains the coefficients in the sequence:
**        C00, C01, C02, C10, C11, C12, C20, C21, C22, D0, D1, D2.
**
**        Note: all the input conversion shares the same coefficients.
**              Cxx must be integer from 32767 to -32768.
**              Dx must be integer from 16777215 to -16777216.
**
*/
gceSTATUS
gco2D_SetStateArrayI32(
    IN gco2D Engine,
    IN gce2D_STATE State,
    IN gctINT32_PTR Array,
    IN gctINT32 ArraySize
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_MonoBlitEx
**
**  Extend Monochrome blit for 8x1 packed stream.
**
**  NOTE:
**      1.  If SrcRect equals to gcvNULL, the stream is used as monochrome
**          source; Else, it is used as the source mask.
**      2.  The Stream rectangle size is the same with DstRect.
**
**  INPUT:
**
**      gctPOINTER   StreamBits
**          Pointer to the 8x1 packed stream.
**
**      gctINT32     StreamStride
**          The stride in byte of stream.
**
**      gctINT32     StreamWidth
**          The width in pixel of stream.
**
**      gctINT32     StreamHeight
**          The height in pixel of stream.
**
**      gctINT32     StreamX, StreamY
**          The starting offest in stream.
**
**      gctUINT32    FgColor, BgColor
**          Foreground/Background color values in A8R8G8B8 only useful when
**          SrcRect equals to gcvNULL .
**
**      gcsRECT_PTR  SrcRect
**          Pointer to src rectangle for masked source blit.
**
**      gcsRECT_PTR  DstRect
**          Pointer to dst rectangle.
**
**      gctUINT32 FgRop
**          Foreground ROP to use with opaque pixels.
**
**      gctUINT32 BgRop
**          Background ROP to use with transparent pixels.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco2D_MonoBlitEx(
    IN gco2D        Engine,
    IN gctPOINTER   StreamBits,
    IN gctINT32     StreamStride,
    IN gctINT32     StreamWidth,
    IN gctINT32     StreamHeight,
    IN gctINT32     StreamX,
    IN gctINT32     StreamY,
    IN gctUINT32    FgColor,
    IN gctUINT32    BgColor,
    IN gcsRECT_PTR  SrcRect,
    IN gcsRECT_PTR  DstRect,
    IN gctUINT8     FgRop,
    IN gctUINT8     BgRop
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gco2D_Get2DEngine
**
**  Get the pointer to the gco2D object which is the current one of this thread
**
**  OUTPUT:
**
**      gco2D * Engine
**          Pointer to a variable receiving the gco2D object pointer.
*/
gceSTATUS
gco2D_Get2DEngine(
    OUT gco2D * Engine
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*****************************************************************************
*********
**
**  gco2D_Set2DEngine
**
**  Set the pointer as the gco2D object to be current 2D engine of this thread
**
**
**   gco2D Engine
**       The gco2D object that needs to be set.
**
**  OUTPUT:
**
**   nothing.
*/
gceSTATUS
gco2D_Set2DEngine(
     IN gco2D Engine
     )
{
    return gcvSTATUS_NOT_SUPPORTED;
}


/*****************************************************************************
*********
**
**  gco2D_UnSet2DEngine
**
**  UnSet the pointer as the gco2D object from this thread, restore the old
**  hardware object.
**
**  INPUT:
**
**
**   gco2D Engine
**       The gco2D object that needs to be unset.
**
**  OUTPUT:
**
**   nothing.
*/
gceSTATUS
gco2D_UnSet2DEngine(
     IN gco2D Engine
     )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

gceSTATUS
gco2D_Commit(
    IN gco2D Engine,
    IN gctBOOL Stall
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=%x Stall=%d", Engine, Stall);

    /* Commit the command buffer to hardware. */
    gcmONERROR(gcoHARDWARE_Commit(Engine->hardware));

    if (Stall)
    {
        /* Stall the hardware. */
        gcmONERROR(gcoHARDWARE_Stall(Engine->hardware));
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco2D_NatureRotateTranslation
**
**    Translate the nature rotation rule to the gpu 2D specific rotation rule.
**
**  INPUT:
**
**      gctBOOL IsSrcRot
**          Tranlate to src rotation or dst rotation.
**
**      gce2D_NATURE_ROTATION NatureRotation
**          Nature rotation rule.
**
**      gctINT32 SrcSurfaceWidth
**          The width of the src surface.
**
**      gctINT32 SrcSurfaceHeight
**          The height of the src surface.
**
**      gctINT32 DstSurfaceWidth
**          The width of the dst surface.
**
**      gctINT32 DstSurfaceHeight
**          The height of the dst surface.
**
**      gcsRECT_PTR SrcRect
**          Pointer to the src rectangle before rotation.
**
**      gcsRECT_PTR DstRect
**          Pointer to the dst rectangle before rotation.
**
**  OUTPUT:
**
**      gcsRECT_PTR SrcRect
**          Pointer to the dst rectangle after rotation.
**          Only change when IsSrcRot is enabled.
**
**      gcsRECT_PTR DstRect
**          Pointer to the dst rectangle after rotation.
**          Only change when IsSrcRot is disabled.
**
**      gceSURF_ROTATION* SrcRotation
**          Pointer to the src rotation after translation.
**          Only change when IsSrcRot is enabled.
**
**      gceSURF_ROTATION* DstRotation
**          Pointer to the dst rotation after translation.
**          Only change when IsSrcRot is disabled.
**
*/
gceSTATUS
gco2D_NatureRotateTranslation(
    IN gctBOOL IsSrcRot,
    IN gce2D_NATURE_ROTATION NatureRotation,
    IN gctINT32 SrcSurfaceWidth,
    IN gctINT32 SrcSurfaceHeight,
    IN gctINT32 DstSurfaceWidth,
    IN gctINT32 DstSurfaceHeight,
    IN OUT gcsRECT_PTR SrcRect,
    IN OUT gcsRECT_PTR DstRect,
    OUT gceSURF_ROTATION * SrcRotation,
    OUT gceSURF_ROTATION * DstRotation
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}
