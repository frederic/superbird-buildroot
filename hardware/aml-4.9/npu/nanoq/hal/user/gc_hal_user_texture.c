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


#include "gc_hal_user_precomp.h"

#if gcdENABLE_3D

#if gcdNULL_DRIVER < 2

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcdZONE_TEXTURE

/******************************************************************************\
|********************************* Structures *********************************|
\******************************************************************************/

#define gcdTEXTURE_PATCH_STATUS_APPENDMEM       0x1

typedef struct _gcsMIPMAP *     gcsMIPMAP_PTR;

/* The internal gcsMIPMAP structure. */
struct _gcsMIPMAP
{
    /* User-specified Internal format of texture */
    gctINT                      internalFormat;

    /* Surface format of texture. */
    gceSURF_FORMAT              format;

    /* Width of the mipmap. */
    gctUINT                     width;

    /* Height of the mipmap.  Only used for 2D and 3D textures. */
    gctUINT                     height;

    /* Depth of the mipmap. It's depth for 3D textures.
    ** array size for 2d array texture.
    ** layer-face number for cubemap array texture.
    */
    gctUINT                     depth;

    /* Number of faces.  6 for cubic maps. */
    gctUINT                     faces;

    /* Size per slice. */
    gctSIZE_T                   sliceSize;

    /* Surface allocated for mipmap. */
    gcePOOL                     pool;
    gcoSURF                     surface;
    gctPOINTER                  locked;
    gctUINT32                   address;
    gctUINT32                   patchStatus;
    /* Next mipmap level. */
    gcsMIPMAP_PTR               next;
};

/* The gcoTEXTURE object. */
struct _gcoTEXTURE
{
    /* The object. */
    gcsOBJECT                   object;

    /* Surface format of texture. */
    gceSURF_FORMAT              format;

    /* Endian hint of texture. */
    gceENDIAN_HINT              endianHint;

    /* Block size for compressed and YUV textures. */
    gctUINT                     blockWidth;
    gctUINT                     blockHeight;

    /* Pointer to head of mipmap chain. */
    gcsMIPMAP_PTR               maps;
    gcsMIPMAP_PTR               tail;
    gcsMIPMAP_PTR               baseLevelMap;

    /* Texture info. */
    gcsTEXTURE                  Info;

    gctINT                      levels;

    gctBOOL                     unsizedDepthTexture;

    /* Texture object type */
    gceTEXTURE_TYPE             type;

    /* Texture validation. */
    gctBOOL                     complete;
    gctINT                      completeMax;
    gctINT                      completeBase;
    gctINT                      completeLevels;

    gctBOOL                     filterable;

    /* Texture descriptor */
    gctBOOL                     descDirty;

    gctINT                      descCurIndex;
    gcsTXDescNode               descArray[gcdMAX_TXDESC_ARRAY_SIZE];
};


/******************************************************************************\
|******************************** Support Code ********************************|
\******************************************************************************/

/*******************************************************************************
**
**  _DestroyMaps
**
**  Destroy all gcsMIPMAP structures attached to an gcoTEXTURE object.
**
**  INPUT:
**
**      gcsMIPMAP_PTR MipMap
**          Pointer to the gcsMIPMAP structure that is the head of the linked
**          list.
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      Nothing.
*/
static gceSTATUS
_DestroyMaps(
    IN gcsMIPMAP_PTR MipMap,
    IN gcoOS Os
    )
{
    gcsMIPMAP_PTR next;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("MipMap=0x%x Os=0x%x", MipMap, Os);

    /* Loop while we have valid gcsMIPMAP structures. */
    while (MipMap != gcvNULL)
    {
        /* Get pointer to next gcsMIPMAP structure. */
        next = MipMap->next;

        if (MipMap->locked != gcvNULL)
        {
            /* Unlock MipMap. */
            gcmONERROR(
                gcoSURF_Unlock(MipMap->surface, MipMap->locked));
        }

        if (MipMap->surface != gcvNULL)
        {
            /* Destroy the attached gcoSURF object. */
            gcmONERROR(gcoSURF_Destroy(MipMap->surface));
        }

        /* Free the gcsMIPMAP structure. */
        gcmONERROR(gcmOS_SAFE_FREE(Os, MipMap));

        /* Continue with next mip map. */
        MipMap = next;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


static gceSTATUS _uploadBlitBlt(
    IN gcsSURF_BLITBLT_ARGS* args
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcoSURF srcSurf = gcvNULL;
    gctBOOL srcLocked, dstLocked;
    gceENGINE engine = gcvENGINE_RENDER;

    gcmHEADER_ARG("args=0x%x", args);

    srcLocked = gcvFALSE;
    dstLocked = gcvFALSE;

    status = gcoHARDWARE_CanDo3DBlitBlt(args->format, args->dstSurf->format);
    if(gcmIS_ERROR(status))
    {
        /* Return the status. */
        gcmFOOTER();
        return status;
    }

    /* sync mode */
    if(gcoHAL_GetOption(gcvNULL, gcvOPTION_ASYNC_PIPE))
    {
        /* use blt engine cmd buffer*/
        engine = gcvENGINE_BLT;
    }

    /* GPU upload no need wait fence.*/
    /* Create the wrapper surface. */
    do
    {
        gctPOINTER srcMemory = gcvNULL;
        gctPOINTER pointer = gcvNULL;
        gcsPOINT srcOrigin;
        gcsSURF_FORMAT_INFO_PTR formatInfo;
        gcsSURF_VIEW srcView = {gcvNULL, 0, 1};
        gcsSURF_VIEW dstView = {gcvNULL, 0, 1};
        gcsSURF_RESOLVE_ARGS rlvArgs = {0};

        gctUINT width = args->rectSize.x;
        gctUINT height = args->rectSize.y;

        srcOrigin.x = 0;
        srcOrigin.y = 0;

        rlvArgs.version = gcvHAL_ARG_VERSION_V2;
        rlvArgs.uArgs.v2.yInverted = gcvFALSE;
        rlvArgs.uArgs.v2.srcOrigin = srcOrigin;
        rlvArgs.uArgs.v2.dstOrigin = args->dstOrigin;
        rlvArgs.uArgs.v2.rectSize  = args->rectSize;
        rlvArgs.uArgs.v2.numSlices = 1;
        rlvArgs.uArgs.v2.engine    = engine;

        dstView.surf = args->dstSurf;
        dstView.firstSlice = args->dstOffset;

        /* Create src memory. */
        gcmERR_BREAK(
            gcoOS_Allocate(gcvNULL,
            gcmSIZEOF(struct _gcoSURF),
            &pointer));

        gcoOS_ZeroMemory(pointer, gcmSIZEOF(struct _gcoSURF));

        srcSurf = (gcoSURF)pointer;

#if gcdENABLE_3D
        gcmERR_BREAK(gcoOS_Allocate(gcvNULL, gcmSIZEOF(gctUINT), (gctPOINTER*)&srcSurf->fcValue));
        gcmERR_BREAK(gcoOS_Allocate(gcvNULL, gcmSIZEOF(gctUINT), (gctPOINTER*)&srcSurf->fcValueUpper));
        gcmERR_BREAK(gcoOS_Allocate(gcvNULL, gcmSIZEOF(gctBOOL), (gctPOINTER*)&srcSurf->tileStatusDisabled));
        gcmERR_BREAK(gcoOS_Allocate(gcvNULL, gcmSIZEOF(gctBOOL), (gctPOINTER*)&srcSurf->dirty));

        gcoOS_ZeroMemory(srcSurf->fcValue, gcmSIZEOF(gctUINT));
        gcoOS_ZeroMemory(srcSurf->fcValueUpper, gcmSIZEOF(gctUINT));
        gcoOS_ZeroMemory(srcSurf->tileStatusDisabled, gcmSIZEOF(gctBOOL));
        gcoOS_ZeroMemory(srcSurf->dirty, gcmSIZEOF(gctBOOL));
#endif

        srcView.surf = srcSurf;

        /* init srcSurfInfo.*/
        srcSurf->type = gcvSURF_LINEAR;
        srcSurf->sampleInfo = args->dstSurf->sampleInfo;
        srcSurf->isMsaa = args->dstSurf->isMsaa;
        srcSurf->vMsaa = args->dstSurf->vMsaa;
        srcSurf->requestW = args->dstSurf->requestW;
        srcSurf->requestH = args->dstSurf->requestH;
        srcSurf->allocedW = args->dstSurf->allocedW;
        srcSurf->allocedH = args->dstSurf->allocedH;
        srcSurf->alignedW = args->dstSurf->alignedW;
        srcSurf->alignedH = args->dstSurf->alignedH;

        gcmERR_BREAK(gcoSURF_QueryFormat(args->format, &formatInfo));
        srcSurf->formatInfo = *formatInfo;
        /* Set dimensions of surface. */
        srcSurf->format = args->format;
        srcSurf->tiling = gcvLINEAR;
        srcSurf->colorSpace = gcd_QUERY_COLOR_SPACE(args->format);
        srcSurf->pfGetAddr = gcoHARDWARE_GetProcCalcPixelAddr(gcvNULL, srcSurf);
        /* Set aligned surface size, the surf will only be operated by Blit engine.
        ** no align requirement
        */
        srcSurf->stride = args->stride;

        srcSurf->sliceSize = height * srcSurf->stride;

        srcSurf->size = srcSurf->layerSize = srcSurf->sliceSize;

        gcmERR_BREAK(gcsSURF_NODE_Construct(
            &srcSurf->node,
            srcSurf->size,
            1,
            srcSurf->type,
            gcvALLOC_FLAG_NONE,
            gcvPOOL_DEFAULT
            ));

        gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_SURFACE,
                      "Allocated surface 0x%x: pool=%d size=%dx%dx%d bytes=%u",
                      &srcSurf->node,
                      srcSurf->node.pool,
                      width,
                      height,
                      1,
                      srcSurf->size);

        /* Upload data to srcSurf.*/
        /* Lock the video memory. */
        gcmERR_BREAK(
            gcoHARDWARE_LockEx(&srcSurf->node,
                             engine,
                             gcvNULL,
                             &srcMemory));

        srcLocked = gcvTRUE;

        /* Full copy.*/
        gcoOS_MemCopy(srcMemory, args->buf, srcSurf->size - (srcSurf->stride - width * (formatInfo->bitsPerPixel >> 3)));

        /* Flush the CPU cache. */
        gcmERR_BREAK(gcoSURF_NODE_Cache(&srcSurf->node,
            srcMemory,
            srcSurf->node.size,
            gcvCACHE_CLEAN));

        gcmDUMP_BUFFER(gcvNULL,
            gcvDUMP_BUFFER_TEXTURE,
            gcsSURF_NODE_GetHWAddress(&srcSurf->node),
            srcSurf->node.logical,
            0,
            srcSurf->size);

        gcmERR_BREAK (gcoHARDWARE_LockEx(&args->dstSurf->node,
                                        engine,
                                        gcvNULL,
                                        gcvNULL));

        dstLocked = gcvTRUE;

        /* set the usage.*/
        rlvArgs.uArgs.v2.bUploadTex = gcvTRUE;

        gcmERR_BREAK(gcoHARDWARE_3DBlitBlt(gcvNULL, &srcView, &dstView, &rlvArgs, gcvFALSE));
    } while (gcvFALSE);

    if (srcSurf)
    {
        if (srcSurf->node.pool != gcvPOOL_UNKNOWN)
        {
            if (srcLocked)
            {
            /* Unlock the surface. */
                gcoHARDWARE_UnlockEx(&srcSurf->node, engine, srcSurf->type);
            }

            if (srcSurf->node.u.normal.node != 0)
            {
                /* Free the video memory. */
                gcmVERIFY_OK(gcsSURF_NODE_Destroy(&srcSurf->node));
            }

            /* Mark the memory as freed. */
            srcSurf->node.pool = gcvPOOL_UNKNOWN;
        }
#if gcdENABLE_3D
        if (srcSurf->fcValue)
        {
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, srcSurf->fcValue));
        }
        if (srcSurf->fcValueUpper)
        {
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, srcSurf->fcValueUpper));
        }
        if (srcSurf->tileStatusDisabled)
        {
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, srcSurf->tileStatusDisabled));
        }
        if (srcSurf->dirty)
        {
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, srcSurf->dirty));
        }
#endif

        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, srcSurf));
    }

    if (dstLocked)
    {
        gcoHARDWARE_UnlockEx (&args->dstSurf->node, engine, args->dstSurf->type);
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/******************************************************************************\
|**************************** gcoTEXTURE Object Code ***************************|
\******************************************************************************/

/*******************************************************************************
**
**  gcoTEXTURE_ConstructEx
**
**  Construct a new gcoTEXTURE object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gcvTEXTURE_TYPE Type
**          Texture object type
**
**  OUTPUT:
**
**      gcoTEXTURE * Texture
**          Pointer to a variable receiving the gcoTEXTURE object pointer.
*/
gceSTATUS
gcoTEXTURE_ConstructEx(
    IN gcoHAL Hal,
    IN gceTEXTURE_TYPE Type,
    OUT gcoTEXTURE * Texture
    )
{
    gcoTEXTURE texture;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER();

    /* Veriffy the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Texture != gcvNULL);

    /* Allocate memory for the gcoTEXTURE object. */
    status = gcoOS_Allocate(gcvNULL,
                            sizeof(struct _gcoTEXTURE),
                            &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    gcoOS_ZeroMemory(pointer, sizeof(struct _gcoTEXTURE));

    texture = pointer;

    /* Initialize the gcoTEXTURE object. */
    texture->object.type = gcvOBJ_TEXTURE;

    /* Unknown texture format. */
    texture->format = gcvSURF_UNKNOWN;

    /* Default endian hint */
    texture->endianHint  = gcvENDIAN_NO_SWAP;

    /* No mip map allocated yet. */
    texture->maps           = gcvNULL;
    texture->tail           = gcvNULL;
    texture->baseLevelMap   = gcvNULL;
    texture->levels         = 0;
    texture->complete       = gcvFALSE;
    texture->completeMax    = -1;
    texture->completeBase   = 0x7fffffff;
    texture->completeLevels = 0;
    texture->unsizedDepthTexture = gcvFALSE;
    texture->type           = Type;
    texture->descDirty      = gcvTRUE;
    texture->descCurIndex   = -1;

    /* Return the gcoTEXTURE object pointer. */
    *Texture = texture;

    /* Success. */
    gcmFOOTER_ARG("*Texture=0x%x", *Texture);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoTEXTURE_Construct [Legacy]
**
**  Construct a new gcoTEXTURE object with unknown type
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**
**  OUTPUT:
**
**      gcoTEXTURE * Texture
**          Pointer to a variable receiving the gcoTEXTURE object pointer.
*/
gceSTATUS
gcoTEXTURE_Construct(
    IN gcoHAL Hal,
    OUT gcoTEXTURE * Texture
    )
{
    return gcoTEXTURE_ConstructEx(Hal, gcvTEXTURE_UNKNOWN, Texture);
}


/*******************************************************************************
**
**  gcoTEXTURE_ConstructSized
**
**  Construct a new sized gcoTEXTURE object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gceSURF_FORMAT Format
**          Format of the requested texture.
**
**      gctUINT Width
**          Requested width of the texture.
**
**      gctUINT Height
**          Requested height of the texture.
**
**      gctUINT Depth
**          Requested depth of the texture.  If 'Depth' is not 0, the texture
**          is a volume map.
**
**      gctUINT Faces
**          Requested faces of the texture.  If 'Faces' is not 0, the texture
**          is a cubic map.
**
**      gctUINT MipMapCount
**          Number of requested mip maps for the texture.  'MipMapCount' must be
**          at least 1.
**
**      gcePOOL Pool
**          Pool to allocate tetxure surfaces from.
**
**  OUTPUT:
**
**      gcoTEXTURE * Texture
**          Pointer to a variable receiving the gcoTEXTURE object pointer.
*/
gceSTATUS
gcoTEXTURE_ConstructSized(
    IN gcoHAL Hal,
    IN gceSURF_FORMAT Format,
 /* IN gceTILING Tiling, */
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth,
    IN gctUINT Faces,
    IN gctUINT MipMapCount,
    IN gcePOOL Pool,
    OUT gcoTEXTURE * Texture
    )
{
    gcoTEXTURE texture;
    gceSTATUS status;
    gcsMIPMAP_PTR map = gcvNULL;
    gctUINT level;
    gctPOINTER pointer = gcvNULL;
    gceSURF_TYPE surfType;

    gcmHEADER_ARG("Format=%d Width=%d Height=%d Depth=%d "
                    "Faces=%d MipMapCount=%d Pool=%d",
                    Format, Width, Height, Depth,
                    Faces, MipMapCount, Pool);

    /* Veriffy the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(MipMapCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Texture != gcvNULL);

    /* Allocate memory for the gcoTEXTURE object. */
    status = gcoOS_Allocate(gcvNULL,
                            sizeof(struct _gcoTEXTURE),
                            &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    gcoOS_ZeroMemory(pointer, sizeof(struct _gcoTEXTURE));

    texture = pointer;

    /* Initialize the gcoTEXTURE object. */
    texture->object.type = gcvOBJ_TEXTURE;

    /* Format of texture. */
    texture->format = Format;
    texture->type   = gcvTEXTURE_2D;

    /* Default endian hint */
    texture->endianHint = gcvENDIAN_NO_SWAP;

    /* No mip map allocated yet. */
    texture->maps           = gcvNULL;
    texture->tail           = gcvNULL;
    texture->baseLevelMap   = gcvNULL;
    texture->levels         = 0;
    texture->complete       = gcvFALSE;
    texture->completeMax    = -1;
    texture->completeBase   = 0x7fffffff;
    texture->completeLevels = 0;

    texture->descDirty      = gcvTRUE;
    texture->descCurIndex   = -1;

    /* Client driver may sent depth = 0 */
    Depth = Depth > 0 ? Depth : 1;

    surfType = (texture->type == gcvTEXTURE_3D) ? gcvSURF_TEXTURE_3D : gcvSURF_TEXTURE;

    /* Loop through all mip map. */
    for (level = 0; MipMapCount-- > 0; ++level)
    {
        /* Query texture support */
        status = gcoHARDWARE_QueryTexture(gcvNULL,
                                          Format,
                                          gcvTILED,
                                          level,
                                          Width,
                                          Height,
                                          Depth,
                                          Faces,
                                          &texture->blockWidth,
                                          &texture->blockHeight);

        if (status < 0)
        {
            /* Error. */
            break;
        }

        if (status == gcvSTATUS_OK)
        {
            /* Create an gcsMIPMAP structure. */
            status = gcoOS_Allocate(gcvNULL,
                                   sizeof(struct _gcsMIPMAP),
                                   &pointer);

            if (status < 0)
            {
                /* Error. */
                break;
            }

            map = pointer;

            /* Initialize the gcsMIPMAP structure. */
            map->format     = Format;
            map->width      = Width;
            map->height     = Height;
            map->depth      = Depth;
            map->faces      = Faces;
            map->pool       = Pool;
            map->locked     = gcvNULL;
            map->next       = gcvNULL;

            /* Construct the surface. */
            status = gcoSURF_Construct(gcvNULL,
                                      gcmALIGN_NP2(Width, texture->blockWidth),
                                      gcmALIGN_NP2(Height, texture->blockHeight),
                                      gcmMAX(gcmMAX(Depth, Faces), 1),
                                      surfType,
                                      Format,
                                      Pool,
                                      &map->surface);

            if (status < 0)
            {
                /* Roll back. */
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, map));

                /* Error. */
                break;
            }

            map->sliceSize = map->surface->sliceSize;

            /* Append the gcsMIPMAP structure to the end of the chain. */
            if (texture->maps == gcvNULL)
            {
                texture->maps = map;
                texture->tail = map;
            }
            else
            {
                texture->tail->next = map;
                texture->tail       = map;
            }
        }

        /* Increment number of levels. */
        texture->levels++;
        texture->completeLevels++;

        /* Move to next mip map level. */
        Width  = gcmMAX(Width  / 2, 1);
        Height = gcmMAX(Height / 2, 1);
        Depth  = gcmMAX(Depth  / 2, 1);
    }

    if ( (status == gcvSTATUS_OK) && (texture->maps == gcvNULL) )
    {
        /* No maps created, so texture is not supported. */
        status = gcvSTATUS_NOT_SUPPORTED;
    }

    if (status < 0)
    {
        /* Roll back. */
        gcmVERIFY_OK(_DestroyMaps(texture->maps, gcvNULL));

        texture->object.type = gcvOBJ_UNKNOWN;

        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, texture));

        /* Error. */
        gcmFOOTER();
        return status;
    }

    /* Set texture completeness. */
    gcmASSERT(texture->levels > 0);
    gcmASSERT(texture->levels == texture->completeLevels);
    texture->complete    = gcvTRUE;
    texture->completeMax = texture->completeLevels - 1;
    texture->completeBase= 0;
    texture->baseLevelMap = texture->maps;

    /* Set texture filterable property. */
    texture->filterable = !map->surface->formatInfo.fakedFormat ||
                          map->surface->paddingFormat;

    /* Return the gcoTEXTURE object pointer. */
    *Texture = texture;

    /* Success. */
    gcmFOOTER_ARG("*Texture=0x%x", *Texture);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoTEXTURE_Destroy
**
**  Destroy an gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoTEXTURE_Destroy(
    IN gcoTEXTURE Texture
    )
{
    gcmHEADER_ARG("Texture=0x%x", Texture);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    /* Free all maps. */
    gcmVERIFY_OK(_DestroyMaps(Texture->maps, gcvNULL));

    /* Mark the gcoTEXTURE object as unknown. */
    Texture->object.type = gcvOBJ_UNKNOWN;

    gcoHAL_FreeTXDescArray(Texture->descArray, Texture->descCurIndex);

    Texture->descCurIndex = -1;

    /* Free the gcoTEXTURE object. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Texture));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gctBOOL
_UseAccurateUpload(
    gceSURF_FORMAT SrcFmt, gcoSURF DstSurf
    )
{
    gcsSURF_FORMAT_INFO_PTR srcFmtInfo = gcvNULL;
    gcsSURF_FORMAT_INFO_PTR DstFmtInfo = &DstSurf->formatInfo;

    gcoSURF_QueryFormat(SrcFmt, &srcFmtInfo);

    if (srcFmtInfo && DstFmtInfo)
    {
        /*
        ** 1: Fake format case
        */
        if (srcFmtInfo->fakedFormat || DstFmtInfo->fakedFormat)
        {
            return gcvTRUE;
        }
        /*
        * 2: If tiling of DstSurf is gcvMULTI_SUPERTILED, use accurateUpload to upload data.
        *    Because gcvMULTI_SUPERTILED not supported by gcoHARDWARE_UploadTexture.
        */
        if (DstSurf->tiling == gcvMULTI_SUPERTILED)
        {
            return gcvTRUE;
        }

        /* 3: If one of them is not NORMALIZED, go accurate path. */
        if ((srcFmtInfo->fmtDataType != gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED) ||
            (DstFmtInfo->fmtDataType != gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED))
        {
            return gcvTRUE;
        }
    }

    /* Otherwise, go non-accurate. */
    return gcvFALSE;
}

/*******************************************************************************
**
**  gcoTEXTURE_Upload
**
**  Upload data for a mip map to an gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gceTEXTURE_FACE Face
**          Face of mip map to upload.  If 'Face' is gcvFACE_NONE, the data does
**          not represend a face of a cubic map.
**
**      gctUINT Width
**          Width of mip map to upload.
**
**      gctUINT Height
**          Height of mip map to upload.
**
**      gctUINT Slice
**          Slice of mip map to upload (only valid for volume maps).
**
**      gctPOINTER Memory
**          Pointer to mip map data to upload.
**
**      gctINT Stride
**          Stride of mip map data to upload.  If 'Stride' is 0, the stride will
**          be computed.
**
**      gceSURF_FORMAT Format
**          Format of the mip map data to upload.  Color conversion might be
**          required if tehe texture format and mip map data format are
**          different (24-bit RGB would be one example).
**
**      gceSURF_COLOR_SPACE SrcColorSpace
**          Color Space of source data
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoTEXTURE_Upload(
    IN gcoTEXTURE Texture,
    IN gctINT MipMap,
    IN gceTEXTURE_FACE Face,
    IN gctSIZE_T Width,
    IN gctSIZE_T Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctSIZE_T Stride,
    IN gceSURF_FORMAT Format,
    IN gceSURF_COLOR_SPACE SrcColorSpace
    )
{
    gcsMIPMAP_PTR map;
    gctUINT index;
    gctUINT32 address[3] = {0};
    gctPOINTER memory[3] = {gcvNULL};
    gcoSURF srcSurf = gcvNULL;
    gceSTATUS status;
    gctUINT32 offset, sliceSize, width, height, stride;
    gcsSURF_VIEW texView = {gcvNULL, 0, 1};

    gcmHEADER_ARG("Texture=0x%x MipMap=%d Face=%d Width=%u Height=%u "
                  "Slice=%d Memory=0x%x Stride=%u Format=%d, SrcColorSpace=%u",
                  Texture, MipMap, Face, Width, Height,
                  Slice, Memory, Stride, Format, SrcColorSpace);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

    gcmSAFECASTSIZET(width, Width);
    gcmSAFECASTSIZET(height, Height);
    gcmSAFECASTSIZET(stride, Stride);

    /* Walk to the proper mip map level. */
    for (map = Texture->maps; map != gcvNULL; map = map->next)
    {
        /* See if we have reached the proper level. */
        if (MipMap-- == 0)
        {
            break;
        }
    }

    if (map == gcvNULL)
    {
        /* Requested map might be too large. */
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Convert face into index. */
    switch (Face)
    {
    case gcvFACE_NONE:
        /* Use slice for volume maps or 2D arrays. */
        index = Slice;

        switch(Texture->type)
        {
        case gcvTEXTURE_3D:
        case gcvTEXTURE_2D_ARRAY:
        case gcvTEXTURE_CUBEMAP_ARRAY:
            gcmASSERT(map->faces == 1);
            if (index >= map->depth)
            {
                /* The specified slice is outside the allocated texture. */
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }
            break;

        case gcvTEXTURE_2D:
            gcmASSERT(map->faces == 1);
            gcmASSERT(map->depth == 1);
            if (index != 0)
            {
                /* The specified slice is outside the allocated texture. */
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }
            break;

         default:
            break;
        }
        break;

    case gcvFACE_POSITIVE_X:
    case gcvFACE_NEGATIVE_X:
    case gcvFACE_POSITIVE_Y:
    case gcvFACE_NEGATIVE_Y:
    case gcvFACE_POSITIVE_Z:
    case gcvFACE_NEGATIVE_Z:
        /* Get index from Face. */
        index = Face - gcvFACE_POSITIVE_X;
        if (index >= map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }
        break;

    default:
        index = 0;
        break;
    }

    /* Lock the surface. */
    gcmONERROR(gcoSURF_Lock(map->surface, address, memory));

    if (map->surface->hasStencilComponent)
    {
        map->surface->canDropStencilPlane = gcvFALSE;
    }

    if (map->surface->hzNode.valid)
    {
        map->surface->hzDisabled = gcvTRUE;
    }

    gcmSAFECASTSIZET(sliceSize, map->sliceSize);
    /* Compute offset. */
    offset = index * sliceSize;

    texView.surf = map->surface;
    texView.firstSlice = index;

    if (gcoHAL_GetOption(gcvNULL, gcvOPTION_GPU_TEX_UPLOAD)            &&
        gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_BLT_ENGINE))
    {
        /* GPU upload no need wait fence.*/
        gcsSURF_BLITBLT_ARGS args;

        /* init args.*/
        args.format = Format;
        args.stride = (gctUINT32)Stride;
        args.dstSurf = map->surface;
        args.dstOrigin.x = 0;
        args.dstOrigin.y = 0;
        args.rectSize.x = width;
        args.rectSize.y = height;
        args.buf = Memory;
        args.dstOffset = index;

        status = _uploadBlitBlt(&args);
    }
    else
    {
        /* Run old path.*/
        status = gcvSTATUS_NOT_SUPPORTED;
    }

    if (gcmIS_ERROR(status))
    {
        gcmONERROR(gcoSURF_WaitFence(map->surface));

        if (((Format & gcvSURF_FORMAT_OCL) != 0)              ||
              !_UseAccurateUpload(Format, map->surface))
        {
            /* Copy the data. */
            gcmONERROR(gcoHARDWARE_UploadTexture(&texView, offset, 0, 0,
                width, height, Memory, stride, Format));

            /* Flush the CPU cache. */
            gcmONERROR(gcoSURF_NODE_Cache(&map->surface->node,
                memory[0],
                map->surface->node.size,
                gcvCACHE_CLEAN));
        }
        else
        {
            gcsSURF_BLIT_ARGS arg;

            /* Create the wrapper surface. */
            gcmONERROR(gcoSURF_Construct(gcvNULL, width, height, 1, gcvSURF_BITMAP,
                Format, gcvPOOL_USER, &srcSurf));

            /* If user specified stride(alignment in fact), it must be no less than calculated one. */
            gcmASSERT((gctUINT)Stride >= srcSurf->stride);
            gcmONERROR(gcoSURF_WrapSurface(srcSurf, stride, (gctPOINTER) Memory, gcvINVALID_ADDRESS));
            gcmONERROR(gcoSURF_SetColorSpace(srcSurf, SrcColorSpace));

            /* Propagate canDropStencilPlane */
            srcSurf->canDropStencilPlane = map->surface->canDropStencilPlane;

            gcoOS_ZeroMemory(&arg, sizeof(arg));
            arg.srcSurface = srcSurf;
            arg.dstZ = (gctINT)index;
            arg.dstSurface  = map->surface;
            arg.srcWidth    = arg.dstWidth    = (gctINT)width;
            arg.srcHeight   = arg.dstHeight   = (gctINT)height;
            arg.srcDepth    = arg.dstDepth    = 1;
            arg.srcNumSlice = arg.dstNumSlice = 1;
            gcmONERROR(gcoSURF_BlitCPU(&arg));
        }

        /* Dump the buffer. */
        gcmDUMP_BUFFER(gcvNULL, gcvDUMP_BUFFER_TEXTURE, address[0], memory[0], offset, map->sliceSize);
    }

OnError:
    if (srcSurf != gcvNULL)
        gcoSURF_Destroy(srcSurf);

    if (map && (memory[0] != gcvNULL))
        gcmVERIFY_OK(gcoSURF_Unlock(map->surface, memory[0]));

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_UploadSub
**
**  Upload data for a mip map to an gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gctUINT MipMap
**          Mip map level to upload.
**
**      gceTEXTURE_FACE Face
**          Face of mip map to upload.  If 'Face' is gcvFACE_NONE, the data does
**          not represend a face of a cubic map.
**
**      gctUINT XOffset
**          XOffset offset into mip map to upload.
**
**      gctUINT YOffset
**          YOffset offset into mip map to upload.
**
**      gctUINT Width
**          Width of mip map to upload.
**
**      gctUINT Height
**          Height of mip map to upload.
**
**      gctUINT Slice
**          Slice of mip map to upload (only valid for volume maps).
**
**      gctPOINTER Memory
**          Pointer to mip map data to upload.
**
**      gctINT Stride
**          Stride of mip map data to upload.  If 'Stride' is 0, the stride will
**          be computed.
**
**      gceSURF_FORMAT Format
**          Format of the mip map data to upload.  Color conversion might be
**          required if tehe texture format and mip map data format are
**          different (24-bit RGB would be one example).
**
**      gceSURF_COLOR_SPACE SrcColorSpace
**          Color Space of source data
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoTEXTURE_UploadSub(
    IN gcoTEXTURE Texture,
    IN gctINT MipMap,
    IN gceTEXTURE_FACE Face,
    IN gctSIZE_T XOffset,
    IN gctSIZE_T YOffset,
    IN gctSIZE_T Width,
    IN gctSIZE_T Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctSIZE_T Stride,
    IN gceSURF_FORMAT Format,
    IN gceSURF_COLOR_SPACE SrcColorSpace,
    IN gctUINT32 PhysicalAddress
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 address[3] = {0};
    gctPOINTER memory[3] = {gcvNULL};
    gcsMIPMAP_PTR map;
    gctUINT index;
    gctUINT32 offset, sliceSize, width, height, stride, xOffset, yOffset;
    gctBOOL forceSW = gcvFALSE;
    gcsSURF_VIEW texView = {gcvNULL, 0, 1};

    gcmHEADER_ARG("Texture=0x%x MipMap=%d Face=%d XOffset=%u YOffset=%u Width=%u Height=%u "
                  "Slice=%d Memory=0x%x Stride=%u Format=%d, SrcColorSpace=%u",
                  Texture, MipMap, Face, XOffset, YOffset, Width, Height,
                  Slice, Memory, Stride, Format, SrcColorSpace);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

    gcmSAFECASTSIZET(width, Width);
    gcmSAFECASTSIZET(height, Height);
    gcmSAFECASTSIZET(stride, Stride);
    gcmSAFECASTSIZET(xOffset, XOffset);
    gcmSAFECASTSIZET(yOffset, YOffset);

    /* Walk to the proper mip map level. */
    for (map = Texture->maps; map != gcvNULL; map = map->next)
    {
        /* See if we have reached the proper level. */
        if (MipMap-- == 0)
        {
            break;
        }
    }

    if (!map || !map->surface)
    {
        /* Requested map might be too large. */
        status = gcvSTATUS_MIPMAP_TOO_LARGE;
        goto OnError;
    }

    if ((XOffset + Width > map->width) || (YOffset + Height > map->height))
    {
        /* Requested upload does not match the mip map. */
        gcmERR_RETURN(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Convert face into index. */
    switch (Face)
    {
    case gcvFACE_NONE:
        /* Use slice for volume maps or 2D arrays. */
        index = Slice;

        switch(Texture->type)
        {
        case gcvTEXTURE_3D:
        case gcvTEXTURE_2D_ARRAY:
        case gcvTEXTURE_CUBEMAP_ARRAY:
            gcmASSERT(map->faces == 1);
            if (index >= map->depth)
            {
                /* The specified slice is outside the allocated texture. */
                gcmERR_RETURN(gcvSTATUS_INVALID_ARGUMENT);
            }
            break;
        case gcvTEXTURE_2D:
            gcmASSERT(map->faces == 1);
            gcmASSERT(map->depth == 1);
            if (index != 0)
            {
                /* The specified slice is outside the allocated texture. */
                gcmERR_RETURN(gcvSTATUS_INVALID_ARGUMENT);
            }
            break;

         default:
            break;
        }
        break;

    case gcvFACE_POSITIVE_X:
    case gcvFACE_NEGATIVE_X:
    case gcvFACE_POSITIVE_Y:
    case gcvFACE_NEGATIVE_Y:
    case gcvFACE_POSITIVE_Z:
    case gcvFACE_NEGATIVE_Z:
        /* Get index from Face. */
        index = Face - gcvFACE_POSITIVE_X;
        if (index >= map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
            gcmERR_RETURN(gcvSTATUS_INVALID_ARGUMENT);
        }
        break;

    default:
        index = 0;
        break;
    }

    gcmSAFECASTSIZET(sliceSize, map->sliceSize);
    offset = index * sliceSize;

    texView.surf = map->surface;
    texView.firstSlice = index;

    /* Lock the surface. */
    gcmERR_RETURN(gcoSURF_Lock(map->surface, address, memory));

    if (map->surface->hasStencilComponent)
    {
        map->surface->canDropStencilPlane = gcvFALSE;
    }

    if (map->surface->hzNode.valid)
    {
        map->surface->hzDisabled = gcvTRUE;
    }


    if (gcoHAL_GetOption(gcvNULL, gcvOPTION_GPU_TEX_UPLOAD)            &&
        gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_BLT_ENGINE) &&
        !forceSW)
    {
        /* GPU upload no need wait fence.*/
        gcsSURF_BLITBLT_ARGS args;

        /* init args.*/
        args.format = Format;
        args.stride = (gctUINT32)Stride;
        args.dstSurf = map->surface;
        args.dstOrigin.x = (gctUINT32)XOffset;
        args.dstOrigin.y = (gctUINT32)YOffset;
        args.rectSize.x = (gctUINT32)Width;
        args.rectSize.y = (gctUINT32)Height;
        args.buf = Memory;
        args.dstOffset = index;

        status = _uploadBlitBlt(&args);
    }
    /* Currently limit for GLBM3.0 only, will remove the limitation later. */
    else if (PhysicalAddress != gcvINVALID_ADDRESS && Format == gcvSURF_A8B8G8R8 &&
        XOffset == 0 && YOffset == 0 && Width == 128 && Height == 128 && !forceSW)
    {
        gcsSURF_VIEW wrapView = {gcvNULL, 0, 1};
        gcsSURF_VIEW mipView  = {map->surface, index, 1};

        do
        {
            /* Create the wrapper surface. */
            gcmERR_BREAK(gcoSURF_Construct(gcvNULL, width, height, 1, gcvSURF_BITMAP,
                                         Format, gcvPOOL_USER, &wrapView.surf));

            /* If user specified stride(alignment in fact), it must be no less than calculated one. */
            gcmASSERT((gctUINT)Stride >= wrapView.surf->stride);
            gcmERR_BREAK(gcoSURF_WrapSurface(wrapView.surf, (gctUINT)Stride, (gctPOINTER) Memory, PhysicalAddress));
            gcmERR_BREAK(gcoSURF_SetColorSpace(wrapView.surf, SrcColorSpace));

            /* Propagate canDropStencilPlane */
            wrapView.surf->canDropStencilPlane = map->surface->canDropStencilPlane;

            gcmERR_BREAK(gcoSURF_ResolveRect(&wrapView, &mipView, gcvNULL));

            gcmDUMP_BUFFER(gcvNULL,
                           gcvDUMP_BUFFER_MEMORY,
                           gcsSURF_NODE_GetHWAddress(&wrapView.surf->node),
                           wrapView.surf->node.logical,
                           offset,
                           wrapView.surf->size);


        } while (gcvFALSE);

        if (wrapView.surf)
        {
            gcoSURF_Destroy(wrapView.surf);
        }
    }
    else
    {
        /* For client memory, not support GPU copy now */
        status = gcvSTATUS_NOT_SUPPORTED;
    }

    if (gcmIS_ERROR(status))
    {

        gcmONERROR(gcoSURF_WaitFence(map->surface));

        if (_UseAccurateUpload(Format, map->surface) == gcvFALSE)
        {
            /* Copy the data. */
            gcmONERROR(gcoHARDWARE_UploadTexture(&texView,
                                                 offset,
                                                 xOffset,
                                                 yOffset,
                                                 width,
                                                 height,
                                                 Memory,
                                                 stride,
                                                 Format));

            /* Flush the CPU cache. */
            gcmONERROR(gcoSURF_NODE_Cache(&map->surface->node,
                                          memory[0],
                                          map->surface->node.size,
                                          gcvCACHE_CLEAN));
        }
        else
        {
            gcoSURF srcSurf = gcvNULL;
            gcsSURF_BLIT_ARGS arg;

            do
            {
                /* Create the wrapper surface. */
                gcmERR_BREAK(gcoSURF_Construct(gcvNULL, width, height, 1, gcvSURF_BITMAP,
                                             Format, gcvPOOL_USER, &srcSurf));

                /* If user specified stride(alignment in fact), it must be no less than calculated one. */
                gcmASSERT((gctUINT)Stride >= srcSurf->stride);
                gcmERR_BREAK(gcoSURF_WrapSurface(srcSurf, (gctUINT)Stride, (gctPOINTER)Memory, gcvINVALID_ADDRESS));
                gcmERR_BREAK(gcoSURF_SetColorSpace(srcSurf, SrcColorSpace));

                gcoOS_ZeroMemory(&arg, sizeof(arg));
                arg.srcSurface = srcSurf;
                arg.dstX = (gctINT)XOffset;
                arg.dstY = (gctINT)YOffset;
                arg.dstZ = (gctINT)index;
                arg.dstSurface  = map->surface;
                arg.srcWidth    = arg.dstWidth    = (gctINT)Width;
                arg.srcHeight   = arg.dstHeight   = (gctINT)Height;
                arg.srcDepth    = arg.dstDepth    = 1;
                arg.srcNumSlice = arg.dstNumSlice = 1;
                gcmERR_BREAK(gcoSURF_BlitCPU(&arg));
            } while (gcvFALSE);

            if (srcSurf)
            {
                gcmONERROR(gcoSURF_Destroy(srcSurf));
            }
        }

        /* Dump the buffer. */
        gcmDUMP_BUFFER(gcvNULL,
                       gcvDUMP_BUFFER_TEXTURE,
                       address[0],
                       memory[0],
                       offset,
                       map->sliceSize);
    }

OnError:
    /* Unlock the surface. */
    if (memory[0] && map && map->surface)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(map->surface, memory[0]));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_GetMipMap
**
**  Get the gcoSURF object pointer for a certain mip map level.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gctUINT MipMap
**          Mip map level to get the gcoSURF object pointer for.
**
**  OUTPUT:
**
**      gcoSURF * Surface
**          Pointer to a variable that receives the gcoSURF object pointer.
*/
gceSTATUS
gcoTEXTURE_GetMipMap(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    OUT gcoSURF * Surface
    )
{
    gcsMIPMAP_PTR map;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Texture=0x%x MipMap=%d", Texture, MipMap);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);
    gcmDEBUG_VERIFY_ARGUMENT(Surface != gcvNULL);

    /* Get the pointer to the first gcsMIPMAP. */
    map = Texture->maps;

    /* Loop until we reached the desired mip map. */
    while (MipMap-- != 0)
    {
        /* Bail out if we are beyond the mip map chain. */
        if (map == gcvNULL)
        {
            break;
        }

        /* Get next gcsMIPMAP_PTR pointer. */
        map = map->next;
    }

    if ((map == gcvNULL) || (map->surface == gcvNULL))
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        /* Could not find requested mip map. */
        return status;
    }

    /* Return pointer to gcoSURF object tied to the gcsMIPMAP. */
    *Surface = map->surface;

    /* Success. */
    gcmFOOTER_ARG("*Surface=0x%x", *Surface);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoTEXTURE_GetMipMapFace
**
**  Get the gcoSURF object pointer an offset into the memory for a certain face
**  inside a certain mip map level.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gctUINT MipMap
**          Mip map level to get the gcoSURF object pointer for.
**
**      gceTEXTURE_FACE
**          Face to compute offset for.
**
**  OUTPUT:
**
**      gcoSURF * Surface
**          Pointer to a variable that receives the gcoSURF object pointer.
**
**      gctUINT32_PTR Offset
**          Pointer to a variable that receives the offset for the face.
*/
gceSTATUS
gcoTEXTURE_GetMipMapFace(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    IN gceTEXTURE_FACE Face,
    OUT gcoSURF * Surface,
    OUT gctSIZE_T_PTR Offset
    )
{
    gcsMIPMAP_PTR map;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Texture=0x%x MipMap=%d Face=%d", Texture, MipMap, Face);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);
    gcmDEBUG_VERIFY_ARGUMENT(Surface != gcvNULL);

    /* Get the pointer to the first gcsMIPMAP. */
    map = Texture->maps;

    /* Loop until we reached the desired mip map. */
    while (MipMap-- != 0)
    {
        /* Bail out if we are beyond the mip map chain. */
        if (map == gcvNULL)
        {
            break;
        }

        /* Get next gcsMIPMAP_PTR pointer. */
        map = map->next;
    }

    if ((map == gcvNULL) || (map->surface == gcvNULL))
    {
        /* Could not find requested mip map. */
        gcmFOOTER();
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    gcmASSERT(map->faces == 6);

    if ((Face < gcvFACE_POSITIVE_X) || (Face > gcvFACE_NEGATIVE_Z))
    {
        /* Could not find requested slice. */
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    /* Return pointer to gcoSURF object tied to the gcsMIPMAP. */
    *Surface = map->surface;

    if (Offset)
    {
        /* Return offset to face. */
        *Offset = ((Face - gcvFACE_POSITIVE_X) * map->sliceSize);
    }

    /* Success. */
    gcmFOOTER_ARG("*Surface=0x%x *Offset=%d", *Surface, gcmOPT_VALUE(Offset));
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoTEXTURE_GetMipMapSlice
**
**  Get the gcoSURF object pointer an offset into the memory for a certain slice
**  inside a certain mip map level.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gctUINT MipMap
**          Mip map level to get the gcoSURF object pointer for.
**
**      gctUINT Slice
**          Slice to compute offset for.
**
**  OUTPUT:
**
**      gcoSURF * Surface
**          Pointer to a variable that receives the gcoSURF object pointer.
**
**      gctUINT32_PTR Offset
**          Pointer to a variable that receives the offset for the face.
*/
gceSTATUS
gcoTEXTURE_GetMipMapSlice(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    IN gctUINT Slice,
    OUT gcoSURF * Surface,
    OUT gctSIZE_T_PTR Offset
    )
{
    gcsMIPMAP_PTR map;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Texture=0x%x MipMap=%d Slice=%d", Texture, MipMap, Slice);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);
    gcmDEBUG_VERIFY_ARGUMENT(Surface != gcvNULL);

    /* Get the pointer to the first gcsMIPMAP. */
    map = Texture->maps;

    /* Loop until we reached the desired mip map. */
    while (MipMap-- != 0)
    {
        /* Bail out if we are beyond the mip map chain. */
        if (map == gcvNULL)
        {
            break;
        }

        /* Get next gcsMIPMAP_PTR pointer. */
        map = map->next;
    }

    if ((map == gcvNULL) || (map->surface == gcvNULL))
    {
        /* Could not find requested mip map. */
        gcmFOOTER();
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (Slice >= map->depth)
    {
        /* Could not find requested mip map. */
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    /* Return pointer to gcoSURF object tied to the gcsMIPMAP. */
    *Surface = map->surface;

    if (Offset)
    {
        /* Return offset to face. */
        *Offset = Slice * map->sliceSize;
    }

    /* Success. */
    gcmFOOTER_ARG("*Surface=0x%x *Offset=%d", *Surface, gcmOPT_VALUE(Offset));
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoTEXTURE_AddMipMap
**
**  Add a new mip map to the gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gctINT Level
**          Mip map level to add.
**
**      gctINT InternalFormat
**          InternalFormat of the level requested by app, used for complete check.
**
**      gceSURF_FORMAT Format
**          Format of mip map level to add.
**
**      gctUINT Width
**          Width of mip map to add.  Should be larger or equal than 1.
**
**      gctUINT Height
**          Height of mip map to add.  Can be 0 for 1D texture maps, or larger
**          or equal than 1 for 2D, 3D, or cubic maps.
**
**      gctUINT Depth
**          Depth of mip map to add.  Can be 0 for 1D, 2D, or cubic maps, or
**          larger or equal than 1 for 3D texture maps.
**
**      gctUINT Faces
**          Number of faces of the mip map to add.  Can be 0 for 1D, 2D, or 3D
**          texture maps, or 6 for cubic maps.
**
**      gctBOOL unsizedDepthTexture
**          Indicate whehter dpeth texture is specified by sized or unsized internal format
**          to support different behavior betwee GL_OES_depth_texture and OES3.0 core spec.
**
**      gcePOOL Pool
**          Pool to allocate memory from for the mip map.
**
**
**  OUTPUT:
**
**      gcoSURF * Surface
**          Pointer to a variable that receives the gcoSURF object pointer.
**          'Surface' can be gcvNULL, in which case no surface will be returned.
*/
gceSTATUS
gcoTEXTURE_AddMipMap(
    IN gcoTEXTURE Texture,
    IN gctINT Level,
    IN gctINT InternalFormat,
    IN gceSURF_FORMAT Format,
    IN gctSIZE_T Width,
    IN gctSIZE_T Height,
    IN gctSIZE_T Depth,
    IN gctUINT Faces,
    IN gcePOOL Pool,
    IN gctBOOL Filterable,
    OUT gcoSURF * Surface
    )
{
    return gcoTEXTURE_AddMipMapEx(
        Texture,
        Level,
        InternalFormat,
        Format,
        Width,
        Height,
        Depth,
        Faces,
        Pool,
        0,
        gcvFALSE,
        Filterable,
        Surface
        );
}

/*******************************************************************************
**
**  gcoTEXTURE_AddMipMapEx
**
**  Add a new mip map to the gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gctINT Level
**          Mip map level to add.
**
**      gctINT InternalFormat
**          InternalFormat of the level requested by app, used for complete check.
**
**      gceSURF_FORMAT Format
**          Format of mip map level to add.
**
**      gctUINT Width
**          Width of mip map to add.  Should be larger or equal than 1.
**
**      gctUINT Height
**          Height of mip map to add.  Can be 0 for 1D texture maps, or larger
**          or equal than 1 for 2D, 3D, or cubic maps.
**
**      gctUINT Depth
**          Depth of mip map to add.  Can be 0 for 1D, 2D, or cubic maps, or
**          larger or equal than 1 for 3D texture maps.
**
**      gctUINT Faces
**          Number of faces of the mip map to add.  Can be 0 for 1D, 2D, or 3D
**          texture maps, or 6 for cubic maps.
**
**      gctBOOL unsizedDepthTexture
**          Indicate whehter dpeth texture is specified by sized or unsized internal format
**          to support different behavior betwee GL_OES_depth_texture and OES3.0 core spec.
**
**      gcePOOL Pool
**          Pool to allocate memory from for the mip map.
**
**      gctUINT32 Samples
**          Sample number for MSAA texture.
**
**      gctBOOL Protected
**          Indicate whether will allocate with secure flag when construct surface.
**
**  OUTPUT:
**
**      gcoSURF * Surface
**          Pointer to a variable that receives the gcoSURF object pointer.
**          'Surface' can be gcvNULL, in which case no surface will be returned.
*/
gceSTATUS
gcoTEXTURE_AddMipMapEx(
    IN gcoTEXTURE Texture,
    IN gctINT Level,
    IN gctINT InternalFormat,
    IN gceSURF_FORMAT Format,
    IN gctSIZE_T Width,
    IN gctSIZE_T Height,
    IN gctSIZE_T Depth,
    IN gctUINT Faces,
    IN gcePOOL Pool,
    IN gctUINT32 Samples,
    IN gctBOOL Protected,
    IN gctBOOL Filterable,
    OUT gcoSURF * Surface
    )
{
    gctINT level;
    gctINT internalFormat = gcvUNKNOWN_MIPMAP_IMAGE_FORMAT;
    gcsMIPMAP_PTR map;
    gcsMIPMAP_PTR next;
    gceSTATUS status;
    gceSURF_FORMAT format;
    gctUINT width, height, depth, faces;
    gceSURF_TYPE surfType;

    gcmHEADER_ARG("Texture=0x%x Level=%d Format=%d Width=%u Height=%u "
                    "Depth=%u Faces=0x%x Pool=%d",
                    Texture, Level, Format, Width, Height,
                    Depth, Faces, Pool);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    gcmSAFECASTSIZET(width, Width);
    gcmSAFECASTSIZET(height, Height);
    gcmSAFECASTSIZET(depth, Depth);
    gcmSAFECASTSIZET(faces, Faces);

    if (Level < 0)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    /* Remove format modifiers. */
    format = (gceSURF_FORMAT) (Format & ~gcvSURF_FORMAT_OCL);

    /* Set initial level. */
    map  = gcvNULL;
    next = Texture->maps;
    /* Find the correct mip level. */
    surfType = Protected ? gcvSURF_TEXTURE | gcvSURF_PROTECTED_CONTENT : gcvSURF_TEXTURE;
    surfType |= (Texture->type == gcvTEXTURE_3D) ? gcvSURF_3D : 0;
    for (level = 0; level <= Level; level += 1)
    {
        /* Create gcsMIPMAP structure if doesn't yet exist. */
        if (next == gcvNULL)
        {
            gctPOINTER pointer = gcvNULL;

            gcmONERROR(
                gcoOS_Allocate(gcvNULL,
                               sizeof(struct _gcsMIPMAP),
                               &pointer));

            next = pointer;

            /* Update the texture object format which will be used for _UploadSub. */
            Texture->format = Format;

            /* Initialize the gcsMIPMAP structure. */
            next->internalFormat = gcvUNKNOWN_MIPMAP_IMAGE_FORMAT;
            next->format     = gcvSURF_UNKNOWN;
            next->width      = ~0U;
            next->height     = ~0U;
            next->depth      = ~0U;
            next->faces      = ~0U;
            next->sliceSize  = ~0U;
            next->pool       = gcvPOOL_UNKNOWN;
            next->surface    = gcvNULL;
            next->locked     = gcvNULL;
            next->patchStatus = 0;
            next->next       = gcvNULL;

            /* Append the gcsMIPMAP structure to the end of the chain. */
            if (Texture->maps == gcvNULL)
            {
                /* Save texture format. */
                Texture->format = format;

                /* Update map pointers. */
                Texture->maps = next;
                Texture->tail = next;
            }
            else
            {
                /* Update map pointers. */
                Texture->tail->next = next;
                Texture->tail       = next;
            }

            /* Increment the number of levels. */
            Texture->levels ++;
        }
        else
        {
            internalFormat = next->internalFormat;
        }

        /* Move to the next level. */
        map  = next;
        next = next->next;
    }

    if (map == gcvNULL)
    {
        status = gcvSTATUS_HEAP_CORRUPTED;
        gcmFOOTER();
        return status;
    }

    /* If caller specified it, use the specified one, otherwise the inherited one */
    if (InternalFormat != gcvUNKNOWN_MIPMAP_IMAGE_FORMAT)
    {
        internalFormat = InternalFormat;
    }

    /* Client driver may sent depth = 0 */
    depth = depth > 0 ? depth : 1;
    /* Client driver may sent Faces = 0 */
    faces = faces > 0 ? faces : 1;

    /* Query texture support */
    gcmONERROR(
        gcoHARDWARE_QueryTexture(gcvNULL,
                                 format,
                                 gcvTILED,
                                 Level,
                                 width, height, depth,
                                 faces,
                                 &Texture->blockWidth,
                                 &Texture->blockHeight));

    if (gcmIS_SUCCESS(status))
    {
        /* Free the surface (and all the texture's mipmaps) if it exists and is different. */
        if
        (
            (map != gcvNULL) &&
            (map->surface != gcvNULL)   &&
            (
                (map->format != format) ||
                (map->width  != width)  ||
                (map->height != height) ||
                (map->depth  != depth)  ||
                (map->faces  != faces)  ||
                (map->pool   != Pool)
            )
        )
        {
            /* Unlock the surface. */
            if (map->locked != gcvNULL)
            {
                status = gcoSURF_Unlock(map->surface, map->locked);

                if (gcmIS_ERROR(status))
                {
                    /* Error. */
                    gcmFOOTER();
                    return status;
                }

                map->locked = gcvNULL;
            }

            /* Destroy the surface. */
            if (map->surface != gcvNULL)
            {
                status = gcoSURF_Destroy(map->surface);

                if (gcmIS_ERROR(status))
                {
                    /* Error. */
                    gcmFOOTER();
                    return status;
                }
            }

            /* Reset the surface pointer. */
            map->surface = gcvNULL;
            map->patchStatus = 0;

            /* A surface has been removed. */
            gcmASSERT(Texture->completeLevels > 0);
            Texture->completeLevels --;
        }

        if (map != gcvNULL && map->surface == gcvNULL)
        {
            /* Construct the surface. */
            gcmONERROR(
                gcoSURF_Construct(gcvNULL,
                                  gcmALIGN_NP2(width, Texture->blockWidth),
                                  gcmALIGN_NP2(height, Texture->blockHeight),
                                  gcmMAX(gcmMAX(depth, faces), 1),
                                  surfType,
                                  Format,
                                  Pool,
                                  &map->surface));

            gcmONERROR(gcoSURF_SetSamples(map->surface, Samples));

            /* When replace mipmap, the tex format could change */
            Texture->format = Format;

            /* Initialize the gcsMIPMAP structure. */
            map->format    = format;
            map->width     = width;
            map->height    = height;
            map->depth     = depth;
            map->faces     = faces;
            map->sliceSize = map->surface->sliceSize;
            map->pool      = Pool;

            /* Update the valid surface count. */
            gcmASSERT(Texture->completeLevels < Texture->levels);
            Texture->completeLevels ++;

            /* Invalidate completeness status. */
            Texture->completeMax = -1;
            Texture->completeBase = 0x7fffffff;
            Texture->baseLevelMap = gcvNULL;;
        }

        /* Set texture filterable property. */
        Texture->filterable = (!map->surface->formatInfo.fakedFormat ||
                              map->surface->paddingFormat) &&
                              Filterable;

        /* Update internal format no matter surface was reallocated or not */
        map->internalFormat = internalFormat;

        /* Return the gcoSURF pointer. */
        if (Surface != gcvNULL)
        {
            *Surface = map->surface;
        }

        /* Set descriptor dirty as texture surface is changed */
        Texture->descDirty = gcvTRUE;
    }

OnError:
    /* Return the status. */
    gcmFOOTER_ARG("*Surface=0x%08x status=%d", gcmOPT_VALUE(Surface), status);
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_AddMipMapFromClient
**
**  Add a new mip map by wrapping a client surface to the gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gctINT Level
**          Mip map level to add.
**
**     gcoSURF Surface
**          Client surface to be wrapped
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gcoTEXTURE_AddMipMapFromClient(
    IN gcoTEXTURE Texture,
    IN gctINT Level,
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Texture=0x%x Level=%d Surface=0x%x", Texture, Level, Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Texture->maps != gcvNULL)
    {
        _DestroyMaps(Texture->maps, gcvNULL);
        Texture->maps = gcvNULL;
    }

    gcmONERROR(
        gcoTEXTURE_AddMipMapFromSurface(Texture,
        Level,
        Surface));

    /* Reference client surface. */
    gcmONERROR(
        gcoSURF_ReferenceSurface(Surface));

    /* Copy surface format to texture. */
    Texture->format = Surface->format;

    /* Set texture filterable property. */
    Texture->filterable = !Surface->formatInfo.fakedFormat ||
                          Surface->paddingFormat;

    /* Set descriptor dirty as texture surface is changed */
    Texture->descDirty = gcvTRUE;
    /* Success. */
    status = gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_AddMipMapFromSurface
**
**  Add a new mip map from an existing surface to the gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gctINT Level
**          Mip map level to add.
**
**     gcoSURF    Surface
**          Surface which will be added to mipmap
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gcoTEXTURE_AddMipMapFromSurface(
    IN gcoTEXTURE Texture,
    IN gctINT     Level,
    IN gcoSURF    Surface
    )
{
    gcsMIPMAP_PTR map;
    gceSTATUS status;
    gctUINT width, height, face;
    gceSURF_FORMAT format;
    gceTILING tiling;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Texture=0x%x Level=%d Surface=0x%x", Texture, Level, Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Level != 0)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (Texture->maps != gcvNULL)
    {
        _DestroyMaps(Texture->maps, gcvNULL);
        Texture->maps = gcvNULL;
    }

    /* Currently only support image sourced from 2D,
    ** otherwise the face or slice index cannot be known.
    */

    format = Surface->format;
    tiling = Surface->tiling;
    width  = Surface->requestW;
    height = Surface->requestH;
    face   = Surface->requestD;

    /* Query texture support */
    status = gcoHARDWARE_QueryTexture(gcvNULL,
                                      format,
                                      tiling,
                                      Level,
                                      width, height, 0,
                                      face,
                                      &Texture->blockWidth,
                                      &Texture->blockHeight);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    /* Create an gcsMIPMAP structure. */
    status = gcoOS_Allocate(gcvNULL,
                               gcmSIZEOF(struct _gcsMIPMAP),
                               &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    gcoOS_MemFill(pointer, 0, gcmSIZEOF(struct _gcsMIPMAP));

    /* Save texture format. */
    Texture->format = format;

    map = pointer;

    /* Initialize the gcsMIPMAP structure. */
    map->width      = width;
    map->height     = height;
    map->depth      = 1;
    map->faces      = 1;
    map->sliceSize  = Surface->sliceSize;
    map->pool       = Surface->node.pool;
    map->surface    = Surface;
    map->locked     = gcvNULL;
    map->next       = gcvNULL;
    map->patchStatus = 0;
    map->format     = format;

    /* Append the gcsMIPMAP structure to the end of the chain. */
    Texture->maps = map;
    Texture->tail = map;

    /* Increment the number of levels. */
    Texture->levels ++;
    Texture->completeLevels ++;

    /* Set texture completeness. */
    gcmASSERT(Texture->levels > 0);
    gcmASSERT(Texture->levels == Texture->completeLevels);
    Texture->complete    = gcvTRUE;
    Texture->completeMax = 0;
    Texture->completeBase= 0;
    Texture->baseLevelMap = Texture->maps;

    /* Set texture filterable property. */
    Texture->filterable = !Surface->formatInfo.fakedFormat ||
                          Surface->paddingFormat;

    /* Set descriptor dirty as texture surface is changed */
    Texture->descDirty = gcvTRUE;

#if gcdDUMP
    {
        gctUINT32 address[3];
        gctPOINTER memory[3];

        gcmVERIFY_OK(gcoSURF_Lock(Surface, address, memory));

        gcmDUMP_BUFFER(gcvNULL,
                       gcvDUMP_BUFFER_TEXTURE,
                       address[0],
                       memory[0],
                       0,
                       map->sliceSize);

        gcmVERIFY_OK(gcoSURF_Unlock(Surface, memory[0]));
    }
#endif

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_LockMipMap
**
**  Lock a mip map level in case it's not locked into GPU before.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gctUINT MipMap
**          Mip map level to get the gcoSURF object pointer for.
**
**  OUTPUT:
**
**      gctUINT32 * Address
**          Pointer to a variable that receives the GPU physical address.
**
**      gctPOINTER * Memory
**          Pointer to a variable that receives the CPU logical address.
*/
gceSTATUS
gcoTEXTURE_LockMipMap(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    OPTIONAL OUT gctUINT32 * Address,
    OPTIONAL OUT gctPOINTER * Memory
    )
{
    gcsMIPMAP_PTR map;
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 address[3] = {0};
    gctPOINTER memory[3] = {gcvNULL};

    gcmHEADER_ARG("Texture=0x%x MipMap=%d", Texture, MipMap);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    /* Get the pointer to the first gcsMIPMAP. */
    map = Texture->maps;

    /* Loop until we reached the desired mip map. */
    while (MipMap-- != 0)
    {
        /* Bail out if we are beyond the mip map chain. */
        if (map == gcvNULL)
        {
            break;
        }

        /* Get next gcsMIPMAP_PTR pointer. */
        map = map->next;
    }

    if ((map == gcvNULL) || (map->surface == gcvNULL))
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        /* Could not find requested mip map. */
        return status;
    }

    if (!map->locked)
    {
        /* Lock the texture surface. */
        status = gcoSURF_Lock(map->surface, address, memory);
        map->address = address[0];
        map->locked = memory[0];
    }

    if (Address)
    {
        *Address = map->address;
    }

    if (Memory)
    {
        *Memory = map->locked;
    }

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_SetEndianHint
**
**  Set the endian hint.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gctUINT Red, Green, Blue, Alpha
**          The border color for the respective color channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoTEXTURE_SetEndianHint(
    IN gcoTEXTURE Texture,
    IN gceENDIAN_HINT EndianHint
    )
{
    gcmHEADER_ARG("Texture=0x%x EndianHint=%d", Texture, EndianHint);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    /* Save the endian hint. */
    Texture->endianHint = EndianHint;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoTEXTURE_Disable
**
**  Disable a texture sampler.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gctINT Sampler
**          The physical sampler to disable.
**
**      gctBOOL DefaultInteger
**          Default value is integer or not.
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoTEXTURE_Disable(
    IN gcoHAL Hal,
    IN gctINT Sampler,
    IN gctBOOL DefaultInteger
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Sampler=%d DefaultInteger=%d", Sampler, DefaultInteger);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Sampler >= 0);

#if gcdENABLE_KERNEL_FENCE
    gcoHARDWARE_SetHWSlot(gcvNULL,gcvENGINE_RENDER,gcvHWSLOT_TEXTURE, 0, Sampler);
#endif
    /* Disable the texture. */
    status = gcoHARDWARE_DisableTextureSampler(gcvNULL, Sampler, DefaultInteger);

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_Flush
**
**  Flush the texture cache, when used in the pixel shader.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoTEXTURE_Flush(
    IN gcoTEXTURE Texture
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Texture=0x%x", Texture);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    /* Flush the texture cache. */
    status = gcoHARDWARE_FlushTexture(gcvNULL, gcvFALSE);

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_FlushVS
**
**  Flush the texture cache, when used in the vertex shader.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoTEXTURE_FlushVS(
    IN gcoTEXTURE Texture
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Texture=0x%x", Texture);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    /* Flush the texture cache. */
    status = gcoHARDWARE_FlushTexture(gcvNULL, gcvTRUE);

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_QueryCaps
**
**  Query the texture capabilities.
**
**  INPUT:
**
**      Nothing.
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
*/
gceSTATUS
gcoTEXTURE_QueryCaps(
    IN  gcoHAL    Hal,
    OUT gctUINT * MaxWidth,
    OUT gctUINT * MaxHeight,
    OUT gctUINT * MaxDepth,
    OUT gctBOOL * Cubic,
    OUT gctBOOL * NonPowerOfTwo,
    OUT gctUINT * VertexSamplers,
    OUT gctUINT * PixelSamplers
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("MaxWidth=0x%x MaxHeight=0x%x MaxDepth=0x%x Cubic=0x%x "
                    "NonPowerOfTwo=0x%x VertexSamplers=0x%x PixelSamplers=0x%x",
                    MaxWidth, MaxHeight, MaxDepth, Cubic,
                    NonPowerOfTwo, VertexSamplers, PixelSamplers);

    /* Route to gcoHARDWARE function. */
    status = gcoHARDWARE_QueryTextureCaps(gcvNULL,
                                          MaxWidth,
                                          MaxHeight,
                                          MaxDepth,
                                          Cubic,
                                          NonPowerOfTwo,
                                          VertexSamplers,
                                          PixelSamplers,
                                          gcvNULL);

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_GetClosestFormat
**
**  Returns the closest supported format to the one specified in the call.
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
gcoTEXTURE_GetClosestFormat(
    IN gcoHAL Hal,
    IN gceSURF_FORMAT InFormat,
    OUT gceSURF_FORMAT* OutFormat
    )
{
    return gcoTEXTURE_GetClosestFormatEx(Hal,
                                         InFormat,
                                         gcvTEXTURE_UNKNOWN,
                                         OutFormat);
}


/*******************************************************************************
**
**  gcoTEXTURE_GetClosestFormatEx
**
**  Returns the closest supported format to the one specified in the call.
**
**  INPUT:
**
**      gceSURF_FORMAT APIValue
**          API value.
**
**  INPUT
**      gceTEXTURE_TYPE TextureType
**          Texture type
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Closest supported API value.
*/
gceSTATUS
gcoTEXTURE_GetClosestFormatEx(
    IN gcoHAL Hal,
    IN gceSURF_FORMAT InFormat,
    IN gceTEXTURE_TYPE TextureType,
    OUT gceSURF_FORMAT* OutFormat
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("InFormat=%d TextureType=%d", InFormat, TextureType);

    /* Route to gcoHARDWARE function. */
    status = gcoHARDWARE_GetClosestTextureFormat(gcvNULL,
                                                 InFormat,
                                                 TextureType,
                                                 OutFormat);

    /* Return status. */
    gcmFOOTER();
    return status;
}



/*******************************************************************************
**
**  gcoTEXTURE_GetFormatInfo
**
**  Returns the format info structure from the texture mipmap.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**  OUTPUT:
**
**      gcsSURF_FORMAT_INFO_PTR * TxFormatInfo
**          Format Info structure.
*/
gceSTATUS
gcoTEXTURE_GetFormatInfo(
    IN gcoTEXTURE Texture,
    IN gctINT preferLevel,
    OUT gcsSURF_FORMAT_INFO_PTR * TxFormatInfo
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Texture=0x%x TxFormatInfo=0x%x",
                    Texture, TxFormatInfo);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);
    gcmASSERT(preferLevel >= 0 && preferLevel < Texture->levels);

    if (TxFormatInfo != gcvNULL)
    {
        gcsMIPMAP_PTR prefMipMap = Texture->maps;
        while (preferLevel--)
        {
            prefMipMap = prefMipMap->next;
        }

        if (prefMipMap->surface)
        {
            *TxFormatInfo = &prefMipMap->surface->formatInfo;
        }
        else
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
        }
    }

    gcmFOOTER_NO();

    return status;
}


/*******************************************************************************
**
**  gcoTEXTURE_UploadYUV
**
**  Upload YUV data for a mip map to an gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object. Texture format must be
**          gcvSURF_YUY2.
**
**      gceTEXTURE_FACE Face
**          Face of mip map to upload.  If 'Face' is gcvFACE_NONE, the data does
**          not represend a face of a cubic map.
**
**      gctUINT Width
**          Width of mip map to upload.
**
**      gctUINT Height
**          Height of mip map to upload.
**
**      gctUINT Slice
**          Slice of mip map to upload (only valid for volume maps).
**
**      gctPOINTER Memory[3]
**          Pointer to mip map data to upload. YUV data may at most have 3
**          planes, 3 addresses are specified. Same for 'Stride[3]' below.
**
**      gctINT Stride[3]
**          Stride of mip map data to upload.  If 'Stride' is 0, the stride will
**          be computed.
**
**      gceSURF_FORMAT Format
**          Format of the mip map data to upload.  Color conversion might be
**          required if the texture format and mip map data format are
**          different.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS gcoTEXTURE_UploadYUV(
    IN gcoTEXTURE Texture,
    IN gceTEXTURE_FACE Face,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Slice,
    IN gctPOINTER Memory[3],
    IN gctINT Stride[3],
    IN gceSURF_FORMAT Format
    )
{
    gcsMIPMAP_PTR map;
    gctUINT index;
    gctUINT32 address[3] = {0};
    gctPOINTER memory[3] = {gcvNULL};
    gceSTATUS status;
    gctUINT32 offset, sliceSize;

    gcmHEADER_ARG("Texture=0x%x Face=%d Width=%d Height=%d "
                    "Slice=%d Memory=0x%x Stride=%d Format=%d",
                    Texture, Face, Width, Height,
                    Slice, Memory, Stride, Format);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

    /* Walk to the proper mip map level. */
    for (map = Texture->maps; map != gcvNULL; map = map->next)
    {
        /* See if the map matches the requested size. */
        if ( (Width == map->width) && (Height == map->height) )
        {
            break;
        }
    }

    if (map == gcvNULL)
    {
        status = gcvSTATUS_MIPMAP_TOO_LARGE;
        gcmFOOTER();
        /* Requested map might be too large. */
        return status;
    }

    if (map->format != gcvSURF_YUY2)
    {
        /*
         * Only support YUY2 texture format.
         * It makes no sense for UYVY or other YUV formats.
         */
        status = gcvSTATUS_NOT_SUPPORTED;
        gcmFOOTER();

        return status;
    }

    /* Convert face into index. */
    switch (Face)
    {
    case gcvFACE_NONE:
        /* Use slice for volume maps. */
        index = Slice;
        switch(Texture->type)
        {
        case gcvTEXTURE_3D:
        case gcvTEXTURE_2D_ARRAY:
            gcmASSERT(map->faces == 1);
            if (index >= map->depth)
            {
                /* The specified slice is outside the allocated texture. */
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }
            break;

        case gcvTEXTURE_2D:
            gcmASSERT(map->faces == 1);
            gcmASSERT(map->depth == 1);
            if (index != 0)
            {
                /* The specified slice is outside the allocated texture. */
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }
            break;

         default:
            break;
        }
        break;

    case gcvFACE_POSITIVE_X:
    case gcvFACE_NEGATIVE_X:
    case gcvFACE_POSITIVE_Y:
    case gcvFACE_NEGATIVE_Y:
    case gcvFACE_POSITIVE_Z:
    case gcvFACE_NEGATIVE_Z:
        /* Get index from Face. */
        index = Face - gcvFACE_POSITIVE_X;
        if (index > map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        break;

    default:
        index = 0;
        break;
    }

    /* Lock the surface. */
    gcmONERROR(gcoSURF_Lock(map->surface, address, memory));

    if (map->surface->hasStencilComponent)
    {
        map->surface->canDropStencilPlane = gcvFALSE;
    }

    gcmSAFECASTSIZET(sliceSize, map->sliceSize);
    /* Compute offset. */
    offset = index * sliceSize;

    gcmONERROR(gcoSURF_WaitFence(map->surface));

    /* Copy the data. */
    gcmONERROR(gcoHARDWARE_UploadTextureYUV(map->format,
                                            address[0],
                                            memory[0],
                                            offset,
                                            map->surface->stride,
                                            0,
                                            0,
                                            Width,
                                            Height,
                                            Memory,
                                            Stride,
                                            Format));

    /* Flush the CPU cache. */
    gcmONERROR(gcoSURF_NODE_Cache(&map->surface->node,
                                  memory[0],
                                  map->surface->node.size,
                                  gcvCACHE_CLEAN));

    /* Dump the buffer. */
    gcmDUMP_BUFFER(gcvNULL,
                   gcvDUMP_BUFFER_TEXTURE,
                   address[0],
                   memory[0],
                   offset,
                   map->sliceSize);

    /* Unlock the surface. */
    gcmVERIFY_OK(gcoSURF_Unlock(map->surface, memory[0]));

    /* Return the status. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_UploadCompressed
**
**  Upload compressed data for a mip map to an gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gceTEXTURE_FACE Face
**          Face of mip map to upload.  If 'Face' is gcvFACE_NONE, the data does
**          not represent a face of a cubic map.
**
**      gctUINT Width
**          Width of mip map to upload.
**
**      gctUINT Height
**          Height of mip map to upload.
**
**      gctUINT Slice
**          Slice of mip map to upload (only valid for volume maps).
**
**      gctPOINTER Memory
**          Pointer to mip map data to upload.
**
**      gctSIZE_T Size
**          Size of the compressed data to upload.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoTEXTURE_UploadCompressed(
    IN gcoTEXTURE Texture,
    IN gctINT MipMap,
    IN gceTEXTURE_FACE Face,
    IN gctSIZE_T Width,
    IN gctSIZE_T Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctSIZE_T Size
    )
{
    gcsMIPMAP_PTR map;
    gctUINT index;
    gctUINT32 address[3] = {0};
    gctPOINTER memory[3] = {gcvNULL};
    gceSTATUS status;
    gctUINT32 offset, width, height, size;

    gcmHEADER_ARG("Texture=0x%x Mipmap=%d, Face=%d Width=%u Height=%u Slice=%d Memory=0x%x Size=%u",
                    Texture, MipMap, Face, Width, Height, Slice, Memory, Size);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

    gcmSAFECASTSIZET(width, Width);
    gcmSAFECASTSIZET(height, Height);
    gcmSAFECASTSIZET(size, Size);

    /* Walk to the proper mip map level. */
    for (map = Texture->maps; map != gcvNULL; map = map->next)
    {
        /* See if we have reached the proper level. */
        if (MipMap-- == 0)
        {
            break;
        }
    }

    if (map == gcvNULL)
    {
        /* Requested map might be too large. */
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Convert face into index. */
    switch (Face)
    {
    case gcvFACE_NONE:
        /* Use slice for volume maps or 2D arrays. */
        index = Slice;

        switch(Texture->type)
        {
        case gcvTEXTURE_3D:
        case gcvTEXTURE_2D_ARRAY:
        case gcvTEXTURE_CUBEMAP_ARRAY:
            gcmASSERT(map->faces == 1);
            if (index >= map->depth)
            {
                /* The specified slice is outside the allocated texture. */
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }
            break;

        case gcvTEXTURE_2D:
            gcmASSERT(map->faces == 1);
            gcmASSERT(map->depth == 1);
            if (index != 0)
            {
                /* The specified slice is outside the allocated texture. */
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }
            break;

         default:
            break;
        }
        break;

    case gcvFACE_POSITIVE_X:
    case gcvFACE_NEGATIVE_X:
    case gcvFACE_POSITIVE_Y:
    case gcvFACE_NEGATIVE_Y:
    case gcvFACE_POSITIVE_Z:
    case gcvFACE_NEGATIVE_Z:
        /* Get index from Face. */
        index = Face - gcvFACE_POSITIVE_X;
        if (index >= map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }
        break;

    default:
        index = 0;
        break;
    }

    /* Lock the surface. */
    gcmONERROR(gcoSURF_Lock(map->surface, address, memory));

    /* Compute offset. */
    gcmSAFECASTSIZET(offset, index * map->sliceSize);

    /* Copy the data. */
    gcmONERROR(gcoHARDWARE_UploadCompressedTexture(map->surface,
                                                   Memory,
                                                   offset,
                                                   0,
                                                   0,
                                                   width,
                                                   height,
                                                   size));

    /* Dump the buffer. */
    gcmDUMP_BUFFER(gcvNULL, gcvDUMP_BUFFER_TEXTURE, address[0], memory[0], offset, map->sliceSize);

OnError:
    if (map && memory[0])
    {
        /* Unlock the surface. */
        gcmVERIFY_OK(gcoSURF_Unlock(map->surface, memory[0]));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_UploadCompressedSub
**
**  Upload compressed data for a mip map to an gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gceTEXTURE_FACE Face
**          Face of mip map to upload.  If 'Face' is gcvFACE_NONE, the data does
**          not represend a face of a cubic map.
**
**      gctUINT Width
**          Width of mip map to upload.
**
**      gctUINT Height
**          Height of mip map to upload.
**
**      gctUINT Slice
**          Slice of mip map to upload (only valid for volume maps).
**
**      gctPOINTER Memory
**          Pointer to mip map data to upload.
**
**      gctSIZE_T Size
**          Size of the compressed data to upload.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoTEXTURE_UploadCompressedSub(
    IN gcoTEXTURE Texture,
    IN gctINT MipMap,
    IN gceTEXTURE_FACE Face,
    IN gctSIZE_T XOffset,
    IN gctSIZE_T YOffset,
    IN gctSIZE_T Width,
    IN gctSIZE_T Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctSIZE_T Size
    )
{
    gcsMIPMAP_PTR map;
    gctUINT index;
    gctUINT32 address[3] = {0};
    gctPOINTER memory[3] = {gcvNULL};
    gceSTATUS status;
    gctUINT32 offset, width, height, size, xOffset, yOffset;

    gcmHEADER_ARG("Texture=0x%x Face=%d Width=%u Height=%u Slice=%d Memory=0x%x Size=%u",
                   Texture, Face, Width, Height, Slice, Memory, Size);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

    gcmSAFECASTSIZET(width, Width);
    gcmSAFECASTSIZET(height, Height);
    gcmSAFECASTSIZET(size, Size);
    gcmSAFECASTSIZET(xOffset, XOffset);
    gcmSAFECASTSIZET(yOffset, YOffset);

    /* Walk to the proper mip map level. */
    for (map = Texture->maps; map != gcvNULL; map = map->next)
    {
        /* See if we have reached the proper level. */
        if (MipMap-- == 0)
        {
            break;
        }
    }

    if (!map || !map->surface)
    {
        /* Requested map might be too large, or not been created yet. */
        gcmERR_RETURN(gcvSTATUS_INVALID_ARGUMENT);
    }

    if ((XOffset + Width > map->width) || (YOffset + Height > map->height))
    {
        /* Requested upload does not match the mip map. */
        gcmERR_RETURN(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Convert face into index. */
    switch (Face)
    {
    case gcvFACE_NONE:
        /* Use slice for volume maps or 2D arrays. */
        index = Slice;

        switch(Texture->type)
        {
        case gcvTEXTURE_3D:
        case gcvTEXTURE_2D_ARRAY:
        case gcvTEXTURE_CUBEMAP_ARRAY:
            gcmASSERT(map->faces == 1);
            if (index >= map->depth)
            {
                /* The specified slice is outside the allocated texture. */
                gcmERR_RETURN(gcvSTATUS_INVALID_ARGUMENT);
            }
            break;

        case gcvTEXTURE_2D:
            gcmASSERT(map->faces == 1);
            gcmASSERT(map->depth == 1);
            if (index != 0)
            {
                /* The specified slice is outside the allocated texture. */
                gcmERR_RETURN(gcvSTATUS_INVALID_ARGUMENT);
            }
            break;

         default:
            break;
        }
        break;

    case gcvFACE_POSITIVE_X:
    case gcvFACE_NEGATIVE_X:
    case gcvFACE_POSITIVE_Y:
    case gcvFACE_NEGATIVE_Y:
    case gcvFACE_POSITIVE_Z:
    case gcvFACE_NEGATIVE_Z:
        /* Get index from Face. */
        index = Face - gcvFACE_POSITIVE_X;
        if (index >= map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
            gcmERR_RETURN(gcvSTATUS_INVALID_ARGUMENT);
        }
        break;

    default:
        index = 0;
        break;
    }

    /* Lock the surface. */
    gcmERR_RETURN(gcoSURF_Lock(map->surface, address, memory));


    /* Compute offset. */
    gcmSAFECASTSIZET(offset, index*map->sliceSize);

    /* Copy the data. */
    gcmONERROR(gcoHARDWARE_UploadCompressedTexture(map->surface,
                                                   Memory,
                                                   offset,
                                                   xOffset,
                                                   yOffset,
                                                   width,
                                                   height,
                                                   size));

    /* Dump the buffer. */
    gcmDUMP_BUFFER(gcvNULL, gcvDUMP_BUFFER_TEXTURE, address[0], memory[0], offset, map->sliceSize);

OnError:
    /* Unlock the surface. */
    if (map && map->surface && memory[0])
    {
        gcmVERIFY_OK(gcoSURF_Unlock(map->surface, memory[0]));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*
** This function is only working for ES11 now.
** You needn't put any ES20 app patch here.
** All ES20 app can have RTT tile status buffer by default withouth patch
*/
gceSTATUS
gcoTEXTURE_RenderIntoMipMap(
    IN gcoTEXTURE Texture,
    IN gctINT Level
    )
{
    gcsMIPMAP_PTR map   = gcvNULL;
    gceSURF_TYPE type   = gcvSURF_TYPE_UNKNOWN;
    gceSTATUS status    = gcvSTATUS_OK;
    gctBOOL hasTileFiller;
    gcePATCH_ID patchId = gcvPATCH_INVALID;

    gcmHEADER_ARG("Texture=0x%x Level=%d", Texture, Level);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    /* Get the pointer to the first gcsMIPMAP. */
    map = Texture->maps;

    /* Loop until we reached the desired mip map. */
    while (Level-- != 0)
    {
        /* Bail out if we are beyond the mip map chain. */
        if (map == gcvNULL)
        {
            break;
        }

        /* Get next gcsMIPMAP_PTR pointer. */
        map = map->next;
    }

    if ((map == gcvNULL) || (map->surface == gcvNULL))
    {
        /* Could not find requested mip map. */
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_SINGLE_BUFFER)
                          == gcvSTATUS_TRUE)
    {
        gcsSURF_VIEW texView = {map->surface, 0, 1};
        status = gcoSURF_DisableTileStatus(&texView, gcvTRUE);

        /* Surface is already renderable!. */
        /*status = gcvSTATUS_OK;*/
        gcmFOOTER();
        return status;
    }

    if (gcoHARDWARE_QuerySurfaceRenderable(gcvNULL, map->surface) == gcvSTATUS_OK)
    {
        gcmFOOTER();
        return gcvSTATUS_OK;
    }

    gcoHARDWARE_GetPatchID(gcvNULL, &patchId);
    if (patchId == gcvPATCH_GLBM21 || patchId == gcvPATCH_GLBM25 || patchId == gcvPATCH_GLBM27 || patchId == gcvPATCH_GFXBENCH)
    {
        hasTileFiller = gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TILE_FILLER) == gcvSTATUS_TRUE;
    }
    else
    {
        hasTileFiller = gcvFALSE;
    }

    switch (map->format)
    {
    case gcvSURF_D16:
        /* fall through */
    case gcvSURF_D24S8:
        /* fall through */
    case gcvSURF_D24X8:
        /* fall through */
    case gcvSURF_D32:
        type = gcvSURF_DEPTH_NO_TILE_STATUS;
        break;

    default:
        type = hasTileFiller ? gcvSURF_RENDER_TARGET : gcvSURF_RENDER_TARGET_NO_TILE_STATUS;
        break;
    }

    if (map->surface->type == gcvSURF_TEXTURE)
    {
        gcsSURF_VIEW newView = {gcvNULL, 0, 1};
        if (map->locked != gcvNULL)
        {
                status = gcoSURF_Unlock(map->surface, map->locked);

                if (gcmIS_ERROR(status))
                {
                    /* Error. */
                    gcmFOOTER();
                    return status;
                }

                map->locked = gcvNULL;
        }

        /* Construct a new surface. */
        status = gcoSURF_Construct(gcvNULL,
                                   gcmALIGN_NP2(map->width, Texture->blockWidth),
                                   gcmALIGN_NP2(map->height, Texture->blockHeight),
                                   gcmMAX(gcmMAX(map->depth, map->faces), 1),
                                   type,
                                   map->format,
                                   gcvPOOL_DEFAULT,
                                   &newView.surf);

        if (gcmIS_SUCCESS(status))
        {
            gcsSURF_VIEW oldView = {map->surface, 0, 1};

            /* Copy the data. */
            status = gcoSURF_ResolveRect(&oldView, &newView, gcvNULL);
            if (gcmIS_ERROR(status))
            {
                gcmVERIFY_OK(gcoSURF_Destroy(newView.surf));
                gcmFOOTER();
                return status;
            }

            /* Destroy the old surface. */
            gcmVERIFY_OK(gcoSURF_Destroy(map->surface));

            /* Set the new surface. */
            map->surface = newView.surf;

            /* Mark the mipmap as not resolvable. */
            gcmVERIFY_OK(
                gcoSURF_SetResolvability(map->surface, gcvFALSE));
        }
    }

    /* Success. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoTEXTURE_IsRenderable(
    IN gcoTEXTURE Texture,
    IN gctUINT Level
    )
{
    gcsMIPMAP_PTR map;
    gcoSURF surface;
    gceSTATUS status;

    gcmHEADER_ARG("Texture=0x%x Level=%d", Texture, Level);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    /* Get the pointer to the first gcsMIPMAP. */
    map = Texture->maps;

    /* Loop until we reached the desired mip map. */
    while (Level-- != 0)
    {
        /* Bail out if we are beyond the mip map chain. */
        if (map == gcvNULL)
        {
            break;
        }

        /* Get next gcsMIPMAP_PTR pointer. */
        map = map->next;
    }

    if ((map == gcvNULL) || (map->surface == gcvNULL))
    {
        /* Could not find requested mip map. */
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    /* Get mipmap surface. */
    surface = map->surface;

    /* Check whether the surface is renderable. */
    status = gcoHARDWARE_QuerySurfaceRenderable(gcvNULL, surface);

    /* Return status. */
    gcmFOOTER();
    return status;
}

/* Prepare HZ/TS for renderable texture surface */
gceSTATUS
gcoTEXTURE_PrepareForRender(
    IN gcoTEXTURE Texture,
    IN gctINT Level,
    IN gctUINT Flag,
    IN gctBOOL Dirty
)
{
    gcsMIPMAP_PTR map;
    gceSTATUS status = gcvSTATUS_OK;
    gcoSURF surface;
    gcsSURF_VIEW texView = {gcvNULL, 0, 1};

    gcmHEADER_ARG("Texture=0x%x Level=%d", Texture, Level);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    /* Get the pointer to the first gcsMIPMAP. */
    map = Texture->maps;

    /* Loop until we reached the desired mip map. */
    while (Level-- != 0)
    {
        /* Bail out if we are beyond the mip map chain. */
        if (map == gcvNULL)
        {
            break;
        }

        /* Get next gcsMIPMAP_PTR pointer. */
        map = map->next;
    }

    if ((map == gcvNULL) || (map->surface == gcvNULL))
    {
        /* Could not find requested mip map. */
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    surface = map->surface;

    texView.surf = map->surface;
    texView.numSlices = map->surface->requestD;

    /*
    ** The surface can be directly rendered.
    */
    if (gcoHARDWARE_QuerySurfaceRenderable(gcvNULL, surface) == gcvSTATUS_OK)
    {
        /* Change correct type to make disable tile status buffer work as expected. */
        switch (surface->formatInfo.fmtClass)
        {
        case gcvFORMAT_CLASS_DEPTH:
            surface->type = gcvSURF_DEPTH;
            break;

        default:
            gcmASSERT(surface->formatInfo.fmtClass == gcvFORMAT_CLASS_RGBA);
            surface->type = gcvSURF_RENDER_TARGET;
            break;
        }

#if gcdRTT_DISABLE_FC
        gcmONERROR(gcoSURF_DisableTileStatus(surface, gcvTRUE));
#else
        if ((gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TEXTURE_TILE_STATUS_READ) == gcvFALSE) &&
            (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TILE_FILLER) == gcvFALSE))
        {
            gcmONERROR(gcoSURF_DisableTileStatus(&texView, gcvTRUE));
        }
        else
        {
            if (surface->hzNode.pool == gcvPOOL_UNKNOWN &&
                !(Flag & gcvSURF_NO_HZ))
            {
                /* Try to allocate HZ buffer */
                gcmONERROR(gcoSURF_AllocateHzBuffer(surface));

                /* Lock Hz to same count of main surface, which make the lock/unlock match */
                gcmONERROR(gcoSURF_LockHzBuffer(surface));
            }

            if (surface->tileStatusNode.pool == gcvPOOL_UNKNOWN &&
                !(Flag & gcvSURF_NO_TILE_STATUS))
            {
                surface->TSDirty = Dirty;

                /* Try to allocate tile status buffer. */
                gcmONERROR(gcoSURF_AllocateTileStatus(surface));

                /* Lock TS to same count of main surface, which make the lock/unlock match */
                gcmONERROR(gcoSURF_LockTileStatus(surface));
            }
        }
#endif
        gcmFOOTER();
        return gcvSTATUS_OK;
    }
    else
    {
        status = gcvSTATUS_NOT_SUPPORTED;
    }

OnError:

    /* Success. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoTEXTURE_ReplaceMipmapIntoRenderable(
    IN gcoTEXTURE Texture,
    IN gctINT Level,
    IN gctUINT Flag,
    IN gctBOOL Dirty
)
{
    gcsMIPMAP_PTR map;
    gceSURF_TYPE type;
    gceSTATUS status = gcvSTATUS_OK;
    gcoSURF surface;

    gcmHEADER_ARG("Texture=0x%x Level=%d", Texture, Level);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    /* Get the pointer to the first gcsMIPMAP. */
    map = Texture->maps;

    /* Loop until we reached the desired mip map. */
    while (Level-- != 0)
    {
        /* Bail out if we are beyond the mip map chain. */
        if (map == gcvNULL)
        {
            break;
        }

        /* Get next gcsMIPMAP_PTR pointer. */
        map = map->next;
    }

    if ((map == gcvNULL) || (map->surface == gcvNULL))
    {
        /* Could not find requested mip map. */
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    surface = map->surface;

    if (gcoHARDWARE_QuerySurfaceRenderable(gcvNULL, surface) != gcvSTATUS_OK)
    {
        /* re-create surface for rendering, which can be sampled later too. */
        switch (surface->formatInfo.fmtClass)
        {
        case gcvFORMAT_CLASS_DEPTH:
            type = gcvSURF_DEPTH;
            break;

        default:
            gcmASSERT(surface->formatInfo.fmtClass == gcvFORMAT_CLASS_RGBA);
            type = gcvSURF_RENDER_TARGET;
            break;
        }

    #if gcdRTT_DISABLE_FC
        type |= gcvSURF_NO_TILE_STATUS;
    #endif

        if (Flag & gcvSURF_NO_TILE_STATUS)
        {
            type |= gcvSURF_NO_TILE_STATUS;
        }

        /* Renderable surface serve as texture later */
        type |= gcvSURF_CREATE_AS_TEXTURE;

        if (map->surface->type == gcvSURF_TEXTURE)
        {
            if (map->locked != gcvNULL)
            {
                    status = gcoSURF_Unlock(map->surface, map->locked);

                    if (gcmIS_ERROR(status))
                    {
                        /* Error. */
                        gcmFOOTER();
                        return status;
                    }

                    map->locked = gcvNULL;
            }

            /* Construct a new surface. */
            status = gcoSURF_Construct(gcvNULL,
                                       gcmALIGN_NP2(map->width, Texture->blockWidth),
                                       gcmALIGN_NP2(map->height, Texture->blockHeight),
                                       gcmMAX(gcmMAX(map->depth, map->faces), 1),
                                       type,
                                       map->format,
                                       gcvPOOL_DEFAULT,
                                       &surface);

            if (gcmIS_SUCCESS(status))
            {
                if (Dirty)
                {
                    gcsSURF_VIEW oldView = {map->surface, 0, 1};
                    gcsSURF_VIEW newView = {surface, 0, 1};
                    /* Copy the data. */
                    status = gcoSURF_ResolveRect(&oldView, &newView, gcvNULL);
                }
                if (gcmIS_ERROR(status))
                {
                    gcmVERIFY_OK(gcoSURF_Destroy(surface));
                    gcmFOOTER();
                    return status;
                }

                /* Destroy the old surface. */
                gcmVERIFY_OK(gcoSURF_Destroy(map->surface));

                /* Set the new surface. */
                map->surface = surface;
            }
        }
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoTEXTURE_RenderIntoMipMap2(
    IN gcoTEXTURE Texture,
    IN gctINT Level,
    IN gctBOOL Sync
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Texture=0x%x Level=%d", Texture, Level);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    if (gcoTEXTURE_PrepareForRender(Texture, Level, 0, Sync) != gcvSTATUS_OK)
    {
        gcmONERROR(gcoTEXTURE_ReplaceMipmapIntoRenderable(Texture, Level, 0, Sync));
    }

OnError:

    gcmFOOTER();
    return status;
}


gceSTATUS
gcoTEXTURE_IsComplete(
    IN gcoTEXTURE Texture,
    IN gcsTEXTURE_PTR Info,
    IN gctINT BaseLevel,
    IN gctINT MaxLevel
    )
{
    gceSTATUS status;
    gcsTEXTURE_PTR info = gcvNULL;

    gcmHEADER_ARG("Texture=0x%x", Texture);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    /* If no mipmap was specified */
    if (MaxLevel < 0 || BaseLevel < 0 || BaseLevel > MaxLevel)
    {
        Texture->complete = gcvFALSE;
    }

    info = (Info == gcvNULL) ? &Texture->Info : Info;

    info->baseLevel = BaseLevel;
    info->maxLevel = MaxLevel;

    /* Determine the completeness. */
    if (MaxLevel > Texture->completeMax || BaseLevel < Texture->completeBase)
    {
        gctINT level;
        gctINT internalFormat = gcvUNKNOWN_MIPMAP_IMAGE_FORMAT;
        gceSURF_FORMAT format = gcvSURF_UNKNOWN;
        gctUINT width  = ~0U;
        gctUINT height = ~0U;
        gctUINT depth  = ~0U;
        gctUINT faces  = ~0U;
        gcsMIPMAP_PTR prev;
        gcsMIPMAP_PTR curr;

        /* Assume complete texture. */
        Texture->complete = gcvTRUE;

        /* Set initial level. */
        prev = gcvNULL;
        curr = Texture->maps;
        Texture->baseLevelMap = gcvNULL;

        for (level = 0; level <= MaxLevel; level += 1)
        {
            if (level < BaseLevel)
            {
                /* Move to the next level. */
                curr = curr->next;
                continue;
            }

            /* Does the level exist? */
            if (curr == gcvNULL)
            {
                /* Incomplete. */
                Texture->complete = gcvFALSE;
                break;
            }

            /* Is there a surface attached? */
            if (curr->surface == gcvNULL)
            {
                /* Incomplete. */
                Texture->complete = gcvFALSE;
                break;
            }

            /* Set texture parameters if we are at level 0. */
            if (prev == gcvNULL)
            {
                internalFormat = curr->internalFormat;
                format = curr->format;
                width  = curr->width;
                height = curr->height;
                depth  = curr->depth;
                faces  = curr->faces;
            }
            else
            {
                /* Does the level match the expected? */
                if ((internalFormat != curr->internalFormat) ||
                    (format != curr->format) ||
                    (width  != curr->width)  ||
                    (height != curr->height) ||
                    (depth  != curr->depth)  ||
                    (faces  != curr->faces))
                {
                    /* Incomplete/invalid. */
                    Texture->complete = gcvFALSE;
                    break;
                }
            }

            if (prev == gcvNULL)
            {
                Texture->baseLevelMap = curr;
            }

            /* Compute the size of the next level. */
            width  = gcmMAX(width  / 2, 1);
            height = gcmMAX(height / 2, 1);
            if (Texture->type == gcvTEXTURE_3D)
            {
                depth  = gcmMAX(depth  / 2, 1);
            }

            /* Move to the next level. */
            prev = curr;
            curr = curr->next;
        }

        if (Texture->complete)
        {
            /* Set texture format. */
            Texture->format = format;

            /* Validate completeness. */
            Texture->completeMax = MaxLevel;
            Texture->completeBase = BaseLevel;
        }
        else
        {
            /* Reset completeMax to initial value. */
            Texture->completeMax  = -1;
            Texture->completeBase = 0x7fffffff;
            Texture->baseLevelMap = gcvNULL;
        }
    }
    else if (Texture->complete)
    {
        /* Need redirect to base map */

        gctINT skipLevels = BaseLevel;
        Texture->baseLevelMap = Texture->maps;
        while (skipLevels --)
        {
            Texture->baseLevelMap = Texture->baseLevelMap->next;
        }
    }

    /* Determine the status. */
    status = Texture->complete
        ? gcvSTATUS_OK
        : gcvSTATUS_INVALID_MIPMAP;

    if (status == gcvSTATUS_OK)
    {
        switch (Texture->format)
        {
        case gcvSURF_A32F:
            /* Fall through */
        case gcvSURF_L32F:
            /* Fall through */
        case gcvSURF_A32L32F:
            if ((info->magFilter != gcvTEXTURE_POINT)
             || (info->minFilter != gcvTEXTURE_POINT)
             || (
                    (info->mipFilter != gcvTEXTURE_NONE)
                 && (info->mipFilter != gcvTEXTURE_POINT)
                )
               )
            {
                Texture->complete = gcvFALSE;

                status = gcvSTATUS_NOT_SUPPORTED;
            }
            break;

        default:
            break;
        }
    }

    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoTEXTURE_CheckTexLevel0Attrib(
    IN gcoTEXTURE Texture,
    IN gctINT MaxLevel,
    IN gctINT usedLevel
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Texture=0x%x", Texture);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    if (usedLevel == 0)
    {
        gcsMIPMAP_PTR curr;
        curr = Texture->maps;

        if (curr == gcvNULL)
        {
            gcmFOOTER();
            return gcvSTATUS_INVALID_MIPMAP;
        }

        if (curr->surface == gcvNULL)
        {
            gcmFOOTER();
            return gcvSTATUS_INVALID_MIPMAP;
        }

        if (MaxLevel > 0)
        {
            curr = curr->next;
            if ((curr != gcvNULL) && (curr->surface != gcvNULL))
            {
                status = gcvSTATUS_INVALID_MIPMAP;
            }
        }
    }
    /* Return status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_UpdateTextureDesc(
    IN gcoTEXTURE Texture,
    IN gcsTEXTURE_PTR TexParam
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsTXDESC_UPDATE_INFO updateInfo;
    gcsMIPMAP_PTR mipmap = Texture->maps;
    gcsMIPMAP_PTR baseMipMap = gcvNULL;
    gcoSURF baseSurf = gcvNULL;
    gctINT i;
    gctINT baseLevel = gcmMAX(0, TexParam->baseLevel);
    gctINT maxLevel = gcmMIN(TexParam->maxLevel, Texture->levels - 1);
    gctINT layers = 1, layerIndex = 0;
    gctUINT arrayIndex;
    gcsTXDescNode *pDescNode;

    gcmHEADER_ARG("Texture=0x%x TexParam=0x%x", Texture, TexParam);

    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    gcoOS_ZeroMemory((gctPOINTER)&updateInfo, gcmSIZEOF(updateInfo));

    for (i = 0; i <= maxLevel; i++, mipmap = mipmap->next)
    {
        if (i < baseLevel)
        {
            continue;
        }

        if (!baseSurf)
        {
            baseSurf = mipmap->surface;
            baseMipMap = mipmap;
        }

        gcmGETHARDWAREADDRESS(mipmap->surface->node, updateInfo.lodAddr[i]);

        if (mipmap->surface->formatInfo.fmtClass == gcvFORMAT_CLASS_ASTC)
        {
            updateInfo.astcSize[i] = mipmap->surface->format -
                                        (mipmap->surface->formatInfo.sRGB ? gcvSURF_ASTC4x4_SRGB : gcvSURF_ASTC4x4);
            updateInfo.astcSRGB[i] = mipmap->surface->formatInfo.sRGB;
        }
    }

    if (!baseSurf || !baseMipMap)
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);

    if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TEXTURE_ASTC_BASE_LOD_FIX) == gcvFALSE &&
        baseSurf->formatInfo.fmtClass == gcvFORMAT_CLASS_ASTC &&
        baseLevel > 0)
    {
        for (i = 0; i < baseLevel; ++i)
        {
            updateInfo.astcSize[i] = updateInfo.astcSize[baseLevel];
            updateInfo.astcSRGB[i] = updateInfo.astcSRGB[baseLevel];
        }
    }

    updateInfo.type = Texture->type;
    updateInfo.unsizedDepthTexture = Texture->unsizedDepthTexture;
    updateInfo.baseLevelSurf = baseSurf;
    updateInfo.baseLevelWidth = baseMipMap->width * baseSurf->sampleInfo.x;
    updateInfo.baseLevelHeight = baseMipMap->height * baseSurf->sampleInfo.y;
    updateInfo.baseLevelDepth = baseMipMap->depth;
    updateInfo.levels = Texture->levels;
    updateInfo.endianHint = Texture->endianHint;

    layers = baseSurf->formatInfo.layers;

    if (Texture->descCurIndex >= (gcdMAX_TXDESC_ARRAY_SIZE - 1))
    {
        gcoHAL_FreeTXDescArray(Texture->descArray, Texture->descCurIndex);

        Texture->descCurIndex = -1;
    }

    arrayIndex = ++Texture->descCurIndex;

    pDescNode = &Texture->descArray[arrayIndex];

    gcmASSERT(layers <= 2);

    do
    {
        if (!pDescNode->descNode[layerIndex])
           {
               gcoOS_Allocate(gcvNULL, gcmSIZEOF(gcsSURF_NODE), (gctPOINTER *)&pDescNode->descNode[layerIndex]);
               gcoOS_ZeroMemory((gctPOINTER)pDescNode->descNode[layerIndex], gcmSIZEOF(gcsSURF_NODE));
               gcmONERROR(gcsSURF_NODE_Construct(pDescNode->descNode[layerIndex],
                                                 256,
                                                 64,
                                                 gcvSURF_TXDESC,
                                                 0,
                                                 gcvPOOL_DEFAULT
                                                 ));

           }
           else
           {
               gcmASSERT(0);
           }

           gcmASSERT(pDescNode->descLocked[layerIndex] == gcvNULL);

           gcmONERROR(gcoSURF_LockNode(pDescNode->descNode[layerIndex],
                                       gcvNULL,
                                       &pDescNode->descLocked[layerIndex]));

           updateInfo.desc = pDescNode->descLocked[layerIndex];
#if gcdDUMP
           gcmGETHARDWAREADDRESSP(pDescNode->descNode[layerIndex], updateInfo.physical);
#endif

        mipmap = Texture->maps;

        for (i = 0; i <= maxLevel; i++, mipmap = mipmap->next)
        {
            if (i < baseLevel)
            {
                continue;
            }

            updateInfo.lodAddr[i] += layerIndex * mipmap->surface->layerSize;
        }

        /* For multi-layer, we have two descriptors, so RG->RG for layer0's descriptor,
           BA->RG for layer1's descriptor. */
        if (layers == 2)
        {
            if (layerIndex == 0)
            {
                updateInfo.borderColor32[0] = TexParam->borderColor[0].floatValue;
                updateInfo.borderColor32[1] = TexParam->borderColor[1].floatValue;
                updateInfo.borderColor32[2] = 0;
                updateInfo.borderColor32[3] = 1;
            }
            else
            {
                updateInfo.borderColor32[0] = TexParam->borderColor[2].floatValue;
                updateInfo.borderColor32[1] = TexParam->borderColor[3].floatValue;
                updateInfo.borderColor32[2] = 0;
                updateInfo.borderColor32[3] = 1;
            }
        }
        else
        {
            updateInfo.borderColor32[0] = TexParam->borderColor[0].floatValue;
            updateInfo.borderColor32[1] = TexParam->borderColor[1].floatValue;
            updateInfo.borderColor32[2] = TexParam->borderColor[2].floatValue;
            updateInfo.borderColor32[3] = TexParam->borderColor[3].floatValue;
        }

        gcmONERROR(gcoHARDWARE_UpdateTextureDesc(gcvNULL, TexParam, &updateInfo));

    }
    while(++layerIndex < layers);

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoTEXTURE_SetDescDirty(
    IN gcoTEXTURE Texture
    )
{
    gcmHEADER_ARG("Texture=0x%x", Texture);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    Texture->descDirty = gcvTRUE;

    gcmFOOTER_NO();

    return gcvSTATUS_OK;
}

static gceSTATUS
_ReplaceSurfaceForBorderPatch(
    gcoTEXTURE Texture,
    gcsMIPMAP_PTR Map
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcoSURF surface;
    gcoSURF curSurf = Map->surface;
    gctUINT32 srcAddress = 0, dstAddress = 0;
    gcsSURF_NODE_PTR lockedSrcNode = gcvNULL, lockedDstNode = gcvNULL;

    gcmHEADER_ARG("Map=%p", Map);
    gcmVERIFY_ARGUMENT(Texture != gcvNULL);
    gcmVERIFY_ARGUMENT(Map != gcvNULL);

    gcmASSERT(curSurf);

    if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TX_BORDER_CLAMP_FIX))
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    gcmASSERT(curSurf->node.pool != gcvPOOL_USER);
    /* Construct the surface. */
    gcmONERROR(
        gcoSURF_Construct(gcvNULL,
                          gcmALIGN_NP2(curSurf->requestW, Texture->blockWidth),
                          gcmALIGN_NP2(curSurf->requestH, Texture->blockHeight),
                          curSurf->requestD,
                          curSurf->type | curSurf->hints,
                          curSurf->format | gcvSURF_FORMAT_PATCH_BORDER,
                          gcvPOOL_DEFAULT,
                          &surface));

    gcmONERROR(gcoSURF_SetSamples(surface, (curSurf->sampleInfo.x * curSurf->sampleInfo.y)));

    gcmASSERT(curSurf->node.pool != gcvPOOL_UNKNOWN);
    gcmASSERT(surface->node.pool != gcvPOOL_UNKNOWN);

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_BLT_ENGINE))
    {
        gcmONERROR(gcoHARDWARE_LockEx(&curSurf->node, gcvENGINE_RENDER, &srcAddress, gcvNULL));
        lockedSrcNode = &curSurf->node;
        gcmONERROR(gcoHARDWARE_LockEx(&surface->node, gcvENGINE_RENDER, &dstAddress, gcvNULL));
        lockedDstNode = &surface->node;
        gcmONERROR(gcoHARDWARE_3DBlitCopy(gcvNULL, gcvENGINE_RENDER, srcAddress, dstAddress, (gctUINT32)curSurf->size, gcvFALSE));
        gcmONERROR(gcoHARDWARE_UnlockEx(lockedSrcNode, gcvENGINE_RENDER, curSurf->type));
        lockedSrcNode = gcvNULL;
        gcmONERROR(gcoHARDWARE_UnlockEx(lockedDstNode, gcvENGINE_RENDER, surface->type));
        lockedDstNode = gcvNULL;

        if (curSurf->tileStatusNode.pool != gcvPOOL_UNKNOWN)
        {
            gcmASSERT(surface->tileStatusNode.pool != gcvPOOL_UNKNOWN);
            gcmONERROR(gcoHARDWARE_LockEx(&curSurf->tileStatusNode, gcvENGINE_RENDER, &srcAddress, gcvNULL));
            lockedSrcNode = &curSurf->tileStatusNode;
            gcmONERROR(gcoHARDWARE_LockEx(&surface->tileStatusNode, gcvENGINE_RENDER, &dstAddress, gcvNULL));
            lockedDstNode = &surface->tileStatusNode;
            gcmONERROR(gcoHARDWARE_3DBlitCopy(gcvNULL, gcvENGINE_RENDER, srcAddress, dstAddress, (gctUINT32)curSurf->tileStatusNode.size, gcvFALSE));
            gcmONERROR(gcoHARDWARE_UnlockEx(lockedSrcNode, gcvENGINE_RENDER, gcvSURF_TILE_STATUS));
            lockedSrcNode = gcvNULL;
            gcmONERROR(gcoHARDWARE_UnlockEx(lockedDstNode, gcvENGINE_RENDER, gcvSURF_TILE_STATUS));
            lockedDstNode = gcvNULL;
        }
    }
    else
    {
        /* Finally, try memory copy. */
        lockedSrcNode = &curSurf->node;
        lockedDstNode = &surface->node;
        gcoOS_MemCopy(lockedDstNode->logical, lockedSrcNode->logical, curSurf->size);
    }

    gcsSURF_NODE_GetFence(&curSurf->node, gcvENGINE_RENDER, gcvFENCE_TYPE_READ);
    gcsSURF_NODE_GetFence(&surface->node, gcvENGINE_RENDER, gcvFENCE_TYPE_WRITE);

    gcmASSERT(curSurf->hzNode.pool == gcvPOOL_UNKNOWN);
    gcmASSERT(surface->hzNode.pool == gcvPOOL_UNKNOWN);

    Map->surface = surface;
    Map->patchStatus |= gcdTEXTURE_PATCH_STATUS_APPENDMEM;
    gcoSURF_Destroy(curSurf);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    if (lockedSrcNode)
    {
        gcmONERROR(gcoHARDWARE_UnlockEx(lockedSrcNode, gcvENGINE_RENDER, surface->type));
    }

    if (lockedDstNode)
    {
        gcmONERROR(gcoHARDWARE_UnlockEx(lockedDstNode, gcvENGINE_RENDER, surface->type));
    }

    if (surface)
    {
        gcoSURF_Destroy(surface);
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoTEXTURE_BindTextureDesc(
    IN gcoTEXTURE Texture,
    IN gctINT Sampler,
    IN gcsTEXTURE_PTR Info,
    IN gctINT TextureLayer
    )
{
    gceSTATUS status;
    gcsMIPMAP_PTR map;
    gctINT lod;
    gctINT baseLevel;
    gctFLOAT maxLevel;
    gcsMIPMAP_PTR baseMipMap = gcvNULL;
    gctUINT32 address[3] = {0};
    gctPOINTER memory[3] = {gcvNULL};
    gcsTEXTURE_PTR pTexParams = gcvNULL;
    gcsTEXTURE tmpInfo;
    gcoSURF texSurf;
    gcsSAMPLER samplerInfo;
    gcsTXDescNode *pDescNode;
    gcsSURF_VIEW texView = {gcvNULL, 0, 1};
    gctBOOL canTsEnable = gcvTRUE;
    gctBOOL anyTsEnableForMultiSlice = gcvFALSE;
    gctUINT i = 0;

    gcmHEADER_ARG("Texture=0x%x Sampler=%d Info=0x%x TextureLayer=%d",
                  Texture, Sampler, Info, TextureLayer);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    gcmVERIFY_ARGUMENT(TextureLayer <= 1);

    if (Sampler >= 0)
    {
        /* Use the local struct Info for every thread.*/
        tmpInfo = *Info;

        pTexParams = &tmpInfo;

        baseLevel  = gcmMAX(0, pTexParams->baseLevel);

        maxLevel = (gcvTEXTURE_NONE == pTexParams->mipFilter)
                 ? (baseLevel + 0.3f)    /* Add <0.5 bias to let HW min/mag determination correct*/
                 : (gctFLOAT)gcmMIN(pTexParams->maxLevel, Texture->levels - 1);

        if ((gcvTEXTURE_NONE == pTexParams->mipFilter) &&
            (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TX_MIPFILTER_NONE_FIX)))
        {
            maxLevel = (gctFLOAT)baseLevel;
        }

        pTexParams->lodMax = Info->lodMax;

        pTexParams->lodMin = gcmMAX(pTexParams->lodMin, 0.0f);

        gcmASSERT(baseLevel <= (gctINT)maxLevel);

        /* Make sure we have maps defined in range [baseLevel, maxLevel].
        ** Parameter "Info" is the local variable, all updating in gcoTEXTURE_IsComplete
        ** should on it.
        */
        gcmONERROR(gcoTEXTURE_IsComplete(Texture, &tmpInfo, baseLevel, (gctINT)maxLevel));

        baseMipMap = Texture->baseLevelMap;

        /* Normal textures(include linear RGB) can have up to 14 lods. */
        for (map = baseMipMap, lod = baseLevel; map && (lod <= (gctINT)maxLevel); map = map->next, lod++)
        {
            if (((map->patchStatus & gcdTEXTURE_PATCH_STATUS_APPENDMEM) == 0) &&
                 ((pTexParams->s == gcvTEXTURE_BORDER) ||
                  (pTexParams->t == gcvTEXTURE_BORDER) ||
                  (pTexParams->r == gcvTEXTURE_BORDER)))
            {
                gcmVERIFY_OK(_ReplaceSurfaceForBorderPatch(Texture, map));
            }

            texSurf = map->surface;
            if (map->locked == gcvNULL)
            {
                /* Lock the texture surface. */
                gcmONERROR(gcoSURF_Lock(map->surface, address, memory));
                map->address = address[0];
                map->locked = memory[0];
            }

            /*
            ** For mipmap texture, we have to disable all mipmap's tile status buffer if it has.
            ** Later we may can squeeze all mipmap surface into a big to use a big tile status buffer
            ** Then we can support render into mipmap/multi-slice texture.
            */
            texView.surf = map->surface;
            texView.firstSlice = 0;
            texView.numSlices = texSurf->requestD;
            for (i = texView.firstSlice; i < (texView.firstSlice + texView.numSlices); i++)
            {
                if ((texView.surf->tileStatusNode.pool != gcvPOOL_UNKNOWN) &&
                    (texView.surf->tileStatusDisabled[i] == gcvFALSE))
                {
                    anyTsEnableForMultiSlice = gcvTRUE;
                    break;
                }
            }

            if (((gctINT)maxLevel > baseLevel) &&
                (texSurf->tileStatusNode.pool != gcvPOOL_UNKNOWN) &&
                anyTsEnableForMultiSlice)
            {
                /*
                ** We have no way to sample MSAA surface for now as we can't resolve MSAA into itself.
                ** MSAA texture has been put into shadow rendering path.
                */
                gcmASSERT(!texSurf->isMsaa);
                gcoSURF_DisableTileStatus(&texView, gcvTRUE);
                samplerInfo.hasTileStatus = gcvFALSE;
            }
        }

        if (Texture->descDirty || Info->descDirty)
        {
            gcmONERROR(_UpdateTextureDesc(Texture, Info));
            Texture->descDirty = gcvFALSE;
            Info->descDirty = gcvFALSE;
        }

        pDescNode = &Texture->descArray[Texture->descCurIndex];

        if (!pDescNode->descLocked[TextureLayer])
        {
            gcmASSERT(pDescNode->descNode[TextureLayer]);
            gcmONERROR(gcoSURF_LockNode(pDescNode->descNode[TextureLayer],
                                        gcvNULL,
                                        &pDescNode->descLocked[TextureLayer]));
        }


        texSurf = baseMipMap->surface;
        anyTsEnableForMultiSlice = gcvFALSE;
        texView.surf = texSurf;
        texView.firstSlice = 0;
        texView.numSlices = baseMipMap->surface->requestD;

        samplerInfo.formatInfo = &texSurf->formatInfo;
        samplerInfo.textureInfo = pTexParams;
        samplerInfo.baseLevelSurf = texSurf;
        samplerInfo.texType = Texture->type;
        samplerInfo.filterable = Texture->filterable;
        samplerInfo.descNode = pDescNode->descNode[TextureLayer];

        for (i = texView.firstSlice; i < (texView.firstSlice + texView.numSlices); i++)
        {
            if ((texView.surf->tileStatusNode.pool != gcvPOOL_UNKNOWN) &&
                (texView.surf->tileStatusDisabled[i] == gcvFALSE))
            {
                anyTsEnableForMultiSlice = gcvTRUE;
                break;
            }
        }

        if (texView.numSlices > 1)
        {
            if (texView.surf->tileStatusNode.pool != gcvPOOL_UNKNOWN)
            {
                gctUINT i = 0;
                for (; i < texView.numSlices; i++)
                {
                    if (texView.surf->tileStatusDisabled[i] == gcvTRUE)
                    {
                        canTsEnable = gcvFALSE;
                        break;
                    }
                    if (texView.surf->fcValue[i] != texView.surf->fcValue[0])
                    {
                        canTsEnable = gcvFALSE;
                        break;
                    }
                    if (texView.surf->fcValueUpper[i] != texView.surf->fcValueUpper[0])
                    {
                        canTsEnable = gcvFALSE;
                        break;
                    }
                }
            }
            else
            {
                canTsEnable = gcvFALSE;
            }
        }
        else
        {
            if ((texView.surf->tileStatusNode.pool == gcvPOOL_UNKNOWN) || (texView.surf->tileStatusDisabled[texView.firstSlice] == gcvTRUE))
            {
                canTsEnable = gcvFALSE;
            }
        }

        if ((texSurf->tileStatusNode.pool != gcvPOOL_UNKNOWN) && anyTsEnableForMultiSlice)
        {
            /*
            ** 1, ES3.1 MSAA texture.
            ** 2, multisampled_render_to_texture has been put into shadow rendering path.
            */
            gcmASSERT((!texSurf->isMsaa) ||
                      (Texture->type == gcvTEXTURE_2D_MS || Texture->type == gcvTEXTURE_2D_MS_ARRAY));

            if ((gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TEXTURE_TILE_STATUS_READ) == gcvFALSE)               ||
                (texSurf->compressed && gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TX_DECOMPRESSOR) == gcvFALSE) ||
                (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TX_MULTISAMPLER_FC_FIX) == gcvFALSE)                 ||
                ((texSurf->bitsPerPixel < 16) &&
                    (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TX_8BPP_TS_FIX) == gcvFALSE))                    ||
                !canTsEnable
                )
            {
                texView.surf = baseMipMap->surface;
                texView.numSlices = baseMipMap->surface->requestD;
                gcoSURF_DisableTileStatus(&texView, gcvTRUE);
                samplerInfo.hasTileStatus = gcvFALSE;
            }
            else
            {
                samplerInfo.hasTileStatus = gcvTRUE;
                samplerInfo.compressedDecFormat = texSurf->compressDecFormat;
            }
        }
        else
        {
            samplerInfo.hasTileStatus = gcvFALSE;
        }

        /* Bind to hardware. */
        gcmONERROR(gcoHARDWARE_BindTextureDesc(gcvNULL, Sampler, &samplerInfo));

        /* If success, update info in share object. */
        Texture->Info = tmpInfo;
    }
    else
    {
        /* Unlock the texture if locked. */
        for (map = Texture->maps; map != gcvNULL; map = map->next)
        {
            if (map->locked != gcvNULL)
            {
                /* Unlock the texture surface. */
                gcmONERROR(gcoSURF_Unlock(map->surface, memory[0]));

            }
            /* Mark the surface as unlocked. */
            map->locked = gcvNULL;
        }
    }

OnError:
    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}


gceSTATUS
gcoTEXTURE_BindTextureEx(
    IN gcoTEXTURE Texture,
    IN gctINT Target,
    IN gctINT Sampler,
    IN gcsTEXTURE_PTR Info,
    IN gctINT textureLayer
    )
{
    gceSTATUS status;
    gcsMIPMAP_PTR map;
    gctINT lod;
    gctINT baseLevel;
    gctFLOAT maxLevel;
    gcsSAMPLER samplerInfo;
    gcsMIPMAP_PTR baseMipMap = gcvNULL;
    gcsMIPMAP_PTR textureMap = gcvNULL;
    gctUINT32 address[3] = {0};
    gctPOINTER memory[3] = {gcvNULL};
    gcsTEXTURE_PTR pTexParams = gcvNULL;
    gcsTEXTURE tmpInfo;
#if gcdUSE_NPOT_PATCH
    gceTEXTURE_ADDRESSING r = gcvTEXTURE_INVALID, s = gcvTEXTURE_INVALID, t = gcvTEXTURE_INVALID;
    gceTEXTURE_FILTER mip = gcvTEXTURE_NONE;
    gctBOOL np2_forced = gcvFALSE;
#endif
    gcsSURF_VIEW texView = {gcvNULL, 0, 1};
    gctBOOL canTsEnable = gcvTRUE;
    gctBOOL anyTsEnableForMultiSlice = gcvFALSE;
    gctUINT i = 0;
    gcmHEADER_ARG("Texture=0x%x Sampler=%d", Texture, Sampler);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    /* Use the local struct Info for every thread.*/
    tmpInfo = *Info;
    pTexParams = &tmpInfo;

    gcoOS_ZeroMemory(&samplerInfo, gcmSIZEOF(gcsSAMPLER));

    baseLevel  = gcmMAX(0, pTexParams->baseLevel);
#if gcdUSE_NPOT_PATCH
    if (gcPLS.bNeedSupportNP2Texture)
    {
        gctINT skipLevels = baseLevel;

        /* Need redirect to base map */
        baseMipMap = Texture->maps;
        while (skipLevels--)
        {
            baseMipMap = baseMipMap->next;
        }

        if (baseMipMap &&
            ((baseMipMap->width  & (baseMipMap->width - 1)) ||
             (baseMipMap->height & (baseMipMap->height - 1))
            )
           )
        {
            r = pTexParams->r;
            s = pTexParams->s;
            t = pTexParams->t;
            mip = pTexParams->mipFilter;
            np2_forced = gcvTRUE;

            pTexParams->r = pTexParams->s = pTexParams->t = gcvTEXTURE_CLAMP;
            pTexParams->mipFilter = gcvTEXTURE_NONE;
        }
    }
#endif
    textureMap = Texture->maps;


    if (textureMap && textureMap->surface)
    {
        /*Not support multi_tile/multi_supertile mipmap sampler */
        if (textureMap->surface->tiling == gcvMULTI_TILED ||
            textureMap->surface->tiling == gcvMULTI_SUPERTILED)
        {
            pTexParams->mipFilter = gcvTEXTURE_NONE;
            pTexParams->lodBias = (gctFLOAT)baseLevel;
        }
    }


    maxLevel = (gcvTEXTURE_NONE == pTexParams->mipFilter)
             ? (baseLevel + 0.3f)    /* Add <0.5 bias to let HW min/mag determination correct*/
             : (gctFLOAT)gcmMIN(pTexParams->maxLevel, Texture->levels - 1);

    if ((gcvTEXTURE_NONE == pTexParams->mipFilter) &&
        (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TX_MIPFILTER_NONE_FIX)))
    {
        maxLevel = (gctFLOAT)baseLevel;
    }

    /* If HALT > 1 and bug18 fixed. The HW will clamp lodMax with maxLevel, no need SW doing it.*/
    if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TEX_BASELOD) &&
        gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_BUG_FIXES18)
       )
    {
        pTexParams->lodMax = Info->lodMax;
    }
    else
    {
        pTexParams->lodMax = gcmMIN(maxLevel, Info->lodMax);
    }

    gcmASSERT(gcvSTATUS_FALSE == gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TX_BORDER_CLAMP));

    pTexParams->lodMin = gcmMAX(pTexParams->lodMin, 0.0f);
    gcmASSERT(baseLevel <= (gctINT)maxLevel);

    /* Make sure we have maps defined in range [baseLevel, maxLevel].
    ** Parameter "Info" is the local variable, all updating in gcoTEXTURE_IsComplete
    ** should on it.
    */
    status = gcoTEXTURE_IsComplete(Texture, &tmpInfo, baseLevel, (gctINT)maxLevel);

    /* Special case for external texture. */
    if ((status == gcvSTATUS_INVALID_MIPMAP)
     && (Texture->levels == 0))
    {
        /* For external textures, allow sampler to be
        still bound when no texture is bound. */
        status = gcvSTATUS_OK;
        gcmASSERT(Texture->maps == gcvNULL);
    }

    if (gcmIS_ERROR(status))
    {
#if gcdUSE_NPOT_PATCH
        if (np2_forced)
        {
            pTexParams->r = r;
            pTexParams->s = s;
            pTexParams->t = t;
            pTexParams->mipFilter = mip;
        }
#endif
        gcmFOOTER_ARG("status=%d", status);
        return status;
    }

    baseMipMap = Texture->baseLevelMap;

    /* Check if the texture object is resident. */
    if (Sampler >= 0)
    {
        /* Set format and dimension info. */
        samplerInfo.endianHint  = Texture->endianHint;

        if (baseMipMap != gcvNULL)
        {
            samplerInfo.format      = baseMipMap->surface->format;
            samplerInfo.unsizedDepthTexture = Texture->unsizedDepthTexture;
            samplerInfo.formatInfo  = &baseMipMap->surface->formatInfo;
            samplerInfo.tiling      = baseMipMap->surface->tiling;
            samplerInfo.width       = baseMipMap->width;
            samplerInfo.height      = baseMipMap->height;
            samplerInfo.depth       = baseMipMap->depth;
            samplerInfo.faces       = baseMipMap->faces;
            samplerInfo.texType     = Texture->type;
            samplerInfo.hAlignment  = baseMipMap->surface->hAlignment;
            samplerInfo.addressing  = gcvSURF_NO_STRIDE_TILED;
            samplerInfo.hasTileStatus = 0;
            samplerInfo.baseLevelSurf = baseMipMap->surface;
            samplerInfo.cacheMode = baseMipMap->surface->cacheMode;

#if gcdENABLE_KERNEL_FENCE
            gcoHARDWARE_SetHWSlot(gcvNULL, gcvENGINE_RENDER, gcvHWSLOT_TEXTURE, baseMipMap->surface->node.u.normal.node, Sampler);
#endif
        }
        else
        {
            /* Set dummy values, to sample black (0,0,0,1). */
            samplerInfo.format      = gcvSURF_A8R8G8B8;
            samplerInfo.tiling      = gcvTILED;
            samplerInfo.width       = 0;
            samplerInfo.height      = 0;
            samplerInfo.depth       = 1;
            samplerInfo.faces       = 1;
            samplerInfo.texType     = gcvTEXTURE_UNKNOWN;
            samplerInfo.hAlignment  = gcvSURF_FOUR;
            samplerInfo.addressing  = gcvSURF_NO_STRIDE_TILED;
            samplerInfo.lodNum      = 0;
            samplerInfo.hasTileStatus = 0;
            samplerInfo.cacheMode = gcvCACHE_NONE;

            gcoSURF_QueryFormat(gcvSURF_A8R8G8B8, &samplerInfo.formatInfo);
        }

        samplerInfo.baseLod = 0;
        samplerInfo.needYUVAssembler = gcvFALSE;

        /* Does this texture map have a valid tilestatus node attached to it?. */
        if ((baseMipMap != gcvNULL)
         && (baseMipMap->surface != gcvNULL))
        {
            gcoSURF baseSurf = baseMipMap->surface;

            /* Update horizontal alignment to the surface's alignment. */
            switch (baseSurf->tiling)
            {
            case gcvLINEAR:
                samplerInfo.hAlignment = gcvSURF_SIXTEEN;
                /* Linear texture with stride. */
                samplerInfo.addressing = gcvSURF_STRIDE_LINEAR;
                switch (samplerInfo.format)
                {
                case gcvSURF_YV12:
                case gcvSURF_I420:
                case gcvSURF_NV12:
                case gcvSURF_NV21:
                case gcvSURF_NV16:
                case gcvSURF_NV61:
                    /* Need YUV assembler. */
                    samplerInfo.lodNum = 3;
                    samplerInfo.needYUVAssembler = gcvTRUE;
                    break;

                case gcvSURF_YUY2:
                case gcvSURF_UYVY:
                case gcvSURF_YVYU:
                case gcvSURF_VYUY:
                case gcvSURF_AYUV:
                default:
                    /* Need linear texture. */
                    samplerInfo.lodNum = 1;
                    break;
                }
                break;

            case gcvTILED:
                samplerInfo.hAlignment = baseSurf->hAlignment;
                samplerInfo.lodNum = 1;
                break;

            case gcvSUPERTILED:
                samplerInfo.hAlignment = gcvSURF_SUPER_TILED;
                samplerInfo.lodNum = 1;
                break;

            case gcvMULTI_TILED:
                samplerInfo.hAlignment = gcvSURF_SPLIT_TILED;
                samplerInfo.lodNum = 2;
                break;

            case gcvMULTI_SUPERTILED:
                samplerInfo.hAlignment = gcvSURF_SPLIT_SUPER_TILED;
                samplerInfo.lodNum = 2;
                break;

            case gcvYMAJOR_SUPERTILED:
                samplerInfo.hAlignment = gcvSURF_SPLIT_TILED;
                samplerInfo.lodNum = 1;
                break;

            default:
                gcmASSERT(gcvFALSE);
                status = gcvSTATUS_NOT_SUPPORTED;
#if gcdUSE_NPOT_PATCH
                if (np2_forced)
                {
                    pTexParams->r = r;
                    pTexParams->s = s;
                    pTexParams->t = t;
                    pTexParams->mipFilter = mip;
                }
#endif
                gcmFOOTER();
                return status;
            }

            /* Set all the LOD levels. */
            gcmGETHARDWAREADDRESS(baseSurf->node, samplerInfo.lodAddr[0]);
            samplerInfo.lodAddr[0] += (textureLayer * baseSurf->layerSize);

            if (samplerInfo.lodNum == 3)
            {
                /* YUV-assembler needs 3 lods. */
                if (baseSurf->flags & gcvSURF_FLAG_MULTI_NODE)
                {
                    gcmGETHARDWAREADDRESS(baseSurf->node2, samplerInfo.lodAddr[1]);
                    gcmGETHARDWAREADDRESS(baseSurf->node3, samplerInfo.lodAddr[2]);
                }
                else
                {
                    samplerInfo.lodAddr[1] = samplerInfo.lodAddr[0] + baseSurf->uOffset;
                    samplerInfo.lodAddr[2] = samplerInfo.lodAddr[0] + baseSurf->vOffset;
                }

                /* Save strides. */
                samplerInfo.lodStride[0] = baseSurf->stride;
                samplerInfo.lodStride[1] = baseSurf->uStride;
                samplerInfo.lodStride[2] = baseSurf->vStride;
            }
            else if (samplerInfo.lodNum == 2)
            {
                gcmGETHARDWAREADDRESSBOTTOM(baseSurf->node, samplerInfo.lodAddr[1]);
                samplerInfo.lodAddr[1] += (textureLayer * baseSurf->layerSize);
            }
            else
            {
                samplerInfo.baseLod = baseLevel;

                /* Normal textures(include linear RGB) can have up to 14 lods. */
                for (map = baseMipMap, lod = baseLevel; map && (lod <= (gctINT)maxLevel); map = map->next, lod++)
                {
                    baseSurf = map->surface;
                    if (map->locked == gcvNULL)
                    {
                        /* Lock the texture surface. */
                        status = gcoSURF_Lock(map->surface, address, memory);
                        map->address = address[0];
                        map->locked = memory[0];

                        if (gcmIS_ERROR(status))
                        {
#if gcdUSE_NPOT_PATCH
                            if (np2_forced)
                            {
                                pTexParams->r = r;
                                pTexParams->s = s;
                                pTexParams->t = t;
                                pTexParams->mipFilter = mip;
                            }
#endif
                            /* Error. */
                            gcmFOOTER();
                            return status;
                        }
                    }

                    texView.surf = map->surface;
                    texView.firstSlice = 0;
                    texView.numSlices = map->surface->requestD;

                    for (i = texView.firstSlice; i < (texView.firstSlice + texView.numSlices); i++)
                    {
                        if ((texView.surf->tileStatusNode.pool != gcvPOOL_UNKNOWN) &&
                            (texView.surf->tileStatusDisabled[i] == gcvFALSE))
                        {
                            anyTsEnableForMultiSlice = gcvTRUE;
                            break;
                        }
                    }

                    /*
                    ** For mipmap texture, we have to disable all mipmap's tile status buffer if it has.
                    ** Later we may can squeeze all mipmap surface into a big to use a big tile status buffer
                    ** Then we can support render into mipmap/multi-slice texture.
                    */
                    if (((gctINT)maxLevel > baseLevel) &&
                        (baseSurf->tileStatusNode.pool != gcvPOOL_UNKNOWN) &&
                        anyTsEnableForMultiSlice)
                    {
                        /*
                        ** We have no way to sample MSAA surface for now as we can't resolve MSAA into itself.
                        ** MSAA texture has been put into shadow rendering path.
                        */
                        gcmASSERT(!baseSurf->isMsaa);
                        gcoSURF_DisableTileStatus(&texView, gcvTRUE);
                        samplerInfo.hasTileStatus = gcvFALSE;
                    }

                    samplerInfo.lodAddr[lod]   = map->address + textureLayer * baseSurf->layerSize;
                    samplerInfo.lodStride[lod] = baseSurf->stride;

                    if (baseSurf->formatInfo.fmtClass == gcvFORMAT_CLASS_ASTC)
                    {
                        samplerInfo.astcSize[lod] = baseSurf->format -
                                                    (baseSurf->formatInfo.sRGB ? gcvSURF_ASTC4x4_SRGB : gcvSURF_ASTC4x4);
                        samplerInfo.astcSRGB[lod] = baseSurf->formatInfo.sRGB;
                    }
                }

                /* Set correct lodNum. */
                samplerInfo.lodNum = lod;
            }

            baseSurf = baseMipMap->surface;
            anyTsEnableForMultiSlice = gcvFALSE;
            texView.surf = baseSurf;
            texView.firstSlice = 0;
            texView.numSlices = baseSurf->requestD;

            for (i = texView.firstSlice; i < (texView.firstSlice + texView.numSlices); i++)
            {
                if ((texView.surf->tileStatusNode.pool != gcvPOOL_UNKNOWN) &&
                    (texView.surf->tileStatusDisabled[i] == gcvFALSE))
                {
                    anyTsEnableForMultiSlice = gcvTRUE;
                    break;
                }
            }

            if (texView.numSlices > 1)
            {
                if (texView.surf->tileStatusNode.pool != gcvPOOL_UNKNOWN)
                {
                    gctUINT i = 0;
                    for (; i < texView.numSlices; i++)
                    {
                        if (texView.surf->tileStatusDisabled[i] == gcvTRUE)
                        {
                            canTsEnable = gcvFALSE;
                            break;
                        }
                        if (texView.surf->fcValue[i] != texView.surf->fcValue[0])
                        {
                            canTsEnable = gcvFALSE;
                            break;
                        }
                        if (texView.surf->fcValueUpper[i] != texView.surf->fcValueUpper[0])
                        {
                            canTsEnable = gcvFALSE;
                            break;
                        }
                    }
                }
                else
                {
                    canTsEnable = gcvFALSE;
                }
            }
            else
            {
                if ((texView.surf->tileStatusNode.pool == gcvPOOL_UNKNOWN) || (texView.surf->tileStatusDisabled[texView.firstSlice] == gcvTRUE))
                {
                    canTsEnable = gcvFALSE;
                }
            }

            if ((baseSurf->tileStatusNode.pool != gcvPOOL_UNKNOWN) && anyTsEnableForMultiSlice)
            {
                /*
                ** 1, ES3.1 MSAA texture.
                ** 2, multisampled_render_to_texture has been put into shadow rendering path.
                */
                gcmASSERT((!baseSurf->isMsaa) ||
                          (Texture->type == gcvTEXTURE_2D_MS || Texture->type == gcvTEXTURE_2D_MS_ARRAY));

                if ((gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TEXTURE_TILE_STATUS_READ) == gcvFALSE)                ||
                    (baseSurf->compressed && gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TX_DECOMPRESSOR) == gcvFALSE) ||
                    (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TX_MULTISAMPLER_FC_FIX) == gcvFALSE)                  ||
                    ((baseSurf->bitsPerPixel < 16) &&
                        (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TX_8BPP_TS_FIX) == gcvFALSE))                     ||
                    (baseSurf->isMsaa && gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_MSAA_TEXTURE) == gcvFALSE)        ||
                    !canTsEnable
                    )
                {
                    texView.surf = baseMipMap->surface;
                    texView.numSlices = baseMipMap->surface->requestD;
                    texView.firstSlice = 0;
                    gcoSURF_DisableTileStatus(&texView, gcvTRUE);
                    samplerInfo.hasTileStatus = gcvFALSE;
                }
                else
                {
                    samplerInfo.hasTileStatus = !baseSurf->tileStatusDisabled[0];
                    samplerInfo.compressedDecFormat = baseSurf->compressDecFormat;
                }
            }
            else
            {
                samplerInfo.hasTileStatus = gcvFALSE;
            }
        }

        /* Copy the texture info. */
        samplerInfo.textureInfo = pTexParams;

        samplerInfo.filterable  = Texture->filterable;

        /* Bind to hardware. */
        status = gcoHARDWARE_BindTexture(gcvNULL, Sampler, &samplerInfo);

        if (gcmIS_ERROR(status) && (status != gcvSTATUS_NOT_SUPPORTED))
        {
#if gcdUSE_NPOT_PATCH
            if (np2_forced)
            {
                pTexParams->r = r;
                pTexParams->s = s;
                pTexParams->t = t;
                pTexParams->mipFilter = mip;
            }
#endif
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }
    else
    {
        /* Unlock the texture if locked. */
        for (map = Texture->maps; map != gcvNULL; map = map->next)
        {
            if (map->locked != gcvNULL)
            {
                /* Unlock the texture surface. */
                status = gcoSURF_Unlock(map->surface, memory[0]);

                if (gcmIS_ERROR(status))
                {
#if gcdUSE_NPOT_PATCH
                    if (np2_forced)
                    {
                        pTexParams->r = r;
                        pTexParams->s = s;
                        pTexParams->t = t;
                        pTexParams->mipFilter = mip;
                    }
#endif
                    /* Error. */
                    gcmFOOTER();
                    return status;
                }

                /* Mark the surface as unlocked. */
                map->locked = gcvNULL;
            }
        }
    }

#if gcdUSE_NPOT_PATCH
    if (np2_forced)
    {
        pTexParams->r = r;
        pTexParams->s = s;
        pTexParams->t = t;
        pTexParams->mipFilter = mip;
    }
#endif
    /* If success, update info in share object. */
    Texture->Info = tmpInfo;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoTEXTURE_BindTexture(
    IN gcoTEXTURE Texture,
    IN gctINT Target,
    IN gctINT Sampler,
    IN gcsTEXTURE_PTR Info
    )
{
    return gcoTEXTURE_BindTextureEx(Texture, Target, Sampler, Info, 0);
}

/*******************************************************************************
**
**  gcoTEXTURE_InitParams
**
**  Init the client side texture parameters to default value.
**  Some clients only know part of HAL define texture parameters, it doesn't make
**  sense for client to initial its unknown parameters.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gcsTEXTURE_PTR TexParams
**          Pointer to gcsTEXTURE, which contains HAL tex params but allocated by clients.
*/

gceSTATUS
gcoTEXTURE_InitParams(
    IN gcoHAL Hal,
    IN gcsTEXTURE_PTR TexParams
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hal=0x%x TexParams=0x%x", Hal, TexParams);

    if (TexParams)
    {
        gcoOS_ZeroMemory(TexParams, sizeof(gcsTEXTURE));
        TexParams->s                                = gcvTEXTURE_WRAP;
        TexParams->t                                = gcvTEXTURE_WRAP;
        TexParams->r                                = gcvTEXTURE_WRAP;
        TexParams->swizzle[gcvTEXTURE_COMPONENT_R]  = gcvTEXTURE_SWIZZLE_R;
        TexParams->swizzle[gcvTEXTURE_COMPONENT_G]  = gcvTEXTURE_SWIZZLE_G;
        TexParams->swizzle[gcvTEXTURE_COMPONENT_B]  = gcvTEXTURE_SWIZZLE_B;
        TexParams->swizzle[gcvTEXTURE_COMPONENT_A]  = gcvTEXTURE_SWIZZLE_A;
        TexParams->border[gcvTEXTURE_COMPONENT_R]   = 0;
        TexParams->border[gcvTEXTURE_COMPONENT_G]   = 0;
        TexParams->border[gcvTEXTURE_COMPONENT_B]   = 0;
        TexParams->border[gcvTEXTURE_COMPONENT_A]   = 0;
        TexParams->minFilter                        = gcvTEXTURE_POINT;
        TexParams->mipFilter                        = gcvTEXTURE_LINEAR;
        TexParams->magFilter                        = gcvTEXTURE_LINEAR;
        TexParams->anisoFilter                      = 1;
        TexParams->lodBias                          = 0.0f;
        TexParams->lodMin                           = -1000.0f;
        TexParams->lodMax                           = 1000.0f;
        TexParams->maxLevel                         = 14;
        TexParams->baseLevel                        = 0;
        TexParams->compareMode                      = gcvTEXTURE_COMPARE_MODE_NONE;
        TexParams->compareFunc                      = gcvCOMPARE_LESS_OR_EQUAL;
        TexParams->dsMode                           = gcvTEXTURE_DS_MODE_DEPTH;
        TexParams->sRGB                             = gcvTEXTURE_DECODE;
        TexParams->descDirty                        = gcvFALSE;
    }

    /* Success. */
    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoTEXTURE_SetDepthTextureFlag(
    IN gcoTEXTURE Texture,
    IN gctBOOL  unsized
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Texture=0x%x unsized=%d", Texture, unsized);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    Texture->unsizedDepthTexture = unsized;

    gcmFOOTER_NO();
    return status;

}

gceSTATUS
gcoTEXTURE_BindTextureTS(
    IN gcsTEXTURE_BINDTEXTS_ARGS * args
    )
{
    gceSTATUS status;
    gcmHEADER_ARG(" args=0x%d", args);

    status = gcoHARDWARE_BindTextureTS(gcvNULL);

    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoTEXTURE_GenerateMipMap(
    IN gcoTEXTURE Texture,
    IN gctINT BaseLevel,
    IN gctINT MaxLevel,
    IN gctBOOL sRGBDecode
    )
{
    gctUINT maxLevel = gcmMIN(MaxLevel, Texture->levels - 1);
    gctUINT baseLevel = gcmMAX(0, BaseLevel);
    gctUINT genLevel = maxLevel - baseLevel;

    gceSTATUS status = gcvSTATUS_OK;
    gcoSURF tempSurf[16] = {0};
    gctUINT *indices = gcvNULL;
    gcsMIPMAP_PTR baseMipMap = Texture->maps;
    gcsMIPMAP_PTR map = gcvNULL;

    gcs3DBLIT_INFO info;
    gctUINT width;
    gctUINT height;
    gctUINT depth;
    gctUINT faces;
    gctUINT i, j ;
    gctUINT z, l;

    gcmHEADER_ARG("Texture=0x%x, BaseLevel=%d MaxLevel=%d ", Texture, BaseLevel, MaxLevel);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    if (baseLevel == maxLevel)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }


    for (i = 0; i < baseLevel; i++)
    {
        baseMipMap = baseMipMap->next;
    }

    width = baseMipMap->width;
    height = baseMipMap->height;
    depth = baseMipMap->depth;
    faces = baseMipMap->faces;

    gcoOS_Allocate(gcvNULL, depth * sizeof(gctUINT), (gctPOINTER *) &indices);
    gcoOS_ZeroMemory(&info, gcmSIZEOF(gcs3DBLIT_INFO));

    if(Texture->type == gcvTEXTURE_3D)
    {
        info.LODs[0] = tempSurf[0] = baseMipMap->surface;

        for (l = 1; l <= genLevel; ++l)
        {
            if(width > 1)
            {
                width >>= 1;
            }

            if(height > 1)
            {
                height >>= 1;
            }

            gcmONERROR(gcoSURF_Construct(gcvNULL,
               width, height, depth,
               gcvSURF_TEXTURE,
               baseMipMap->format,
               gcvPOOL_DEFAULT,
               &tempSurf[l]));

            info.LODs[l] = tempSurf[l];
            info.LODsSliceSize[l] = tempSurf[l]->sliceSize;
        }

        for (z = 0; z < depth; ++z)
        {
            gcmONERROR(gcoHARDWARE_3DBlitMipMap(
                gcvNULL,
                gcvENGINE_RENDER,
                &info,
                z,
                sRGBDecode,
                gcvNULL
                ));
        }

        for (l = 1, map = baseMipMap->next; l <= genLevel; ++l, map = map->next)
        {
            gctUINT k = gcmMIN((gctUINT) (1 << l), depth);
            gctUINT r = depth / k;

            for (i = 0; i < r; ++i)
            {
                gcoSURF srcSurfs[32] = {0};
                gctFLOAT weights[32] = {0};
                gcoSURF dstSurf = map->surface;

                for (j = 0; j < k; ++j)
                {
                    srcSurfs[j] = tempSurf[l];
                    weights[j] = 1.0f / (gctFLOAT) k;
                    indices[j] = i * k + j;
                }

                gcoSURF_MixSurfacesCPU(dstSurf, i, srcSurfs, indices, weights, k);
            }
        }
    }
    else
    {
        gctUINT slice = depth * faces;

        for (l = 0, map = baseMipMap; l <= genLevel; l++, map = map->next)
        {
            info.LODs[l] = map->surface;
            info.LODsSliceSize[l] = map->surface->sliceSize;
        }

        for (i = 0; i < slice; i++)
        {
            gcmONERROR(gcoHARDWARE_3DBlitMipMap(
                gcvNULL,
                gcvENGINE_RENDER,
                &info,
                i,
                sRGBDecode,
                gcvNULL
                ));
        }
    }

OnError:

    for (l = 1; l <= genLevel; ++l)
    {
        if (tempSurf[l] != gcvNULL)
        {
            gcoSURF_Destroy(tempSurf[l]);
        }
    }
    if (indices)
    {
        gcmOS_SAFE_FREE(gcvNULL, indices);
    }

    /* Fallback to CPU resampling */
    if (gcmIS_ERROR(status))
    {
        gcsSURF_BLIT_ARGS  blitArgs;

#if !gcdDUMP
        gcePATCH_ID patchID = gcvPATCH_INVALID;
        gcoHAL_GetPatchID(gcvNULL, &patchID);

        if (patchID != gcvPATCH_GFXBENCH)
#endif
        {
            gcsMIPMAP_PTR prevMap;
            gctBOOL sRGBDecodeEnable = gcvFALSE;
            prevMap = baseMipMap;
            map = baseMipMap->next;

            if (sRGBDecode &&
                ((baseMipMap->surface->format == gcvSURF_SBGR8) ||
                (baseMipMap->surface->format == gcvSURF_A8_SBGR8) ||
                (baseMipMap->surface->format == gcvSURF_X8_SBGR8) ||
                (baseMipMap->surface->format == gcvSURF_A8_SRGB8) ||
                (baseMipMap->surface->format == gcvSURF_X8_SRGB8)))
            {
                sRGBDecodeEnable = gcvTRUE;
            }

            for (l = 0; l < genLevel; l++, map = map->next)
            {
                gcoSURF srcSurface = prevMap->surface;
                gcoSURF destSurface = map->surface;

                gcoOS_ZeroMemory(&blitArgs, sizeof(blitArgs));
                blitArgs.srcX               = 0;
                blitArgs.srcY               = 0;
                blitArgs.srcZ               = 0;
                blitArgs.srcWidth           = srcSurface->requestW;
                blitArgs.srcHeight          = srcSurface->requestH;
                blitArgs.srcDepth           = srcSurface->requestD;
                blitArgs.srcSurface         = srcSurface;
                blitArgs.dstX               = 0;
                blitArgs.dstY               = 0;
                blitArgs.dstZ               = 0;
                blitArgs.dstWidth           = destSurface->requestW;
                blitArgs.dstHeight          = destSurface->requestH;
                blitArgs.dstDepth           = destSurface->requestD;
                blitArgs.dstSurface         = destSurface;
                blitArgs.xReverse           = gcvFALSE;
                blitArgs.yReverse           = gcvFALSE;
                blitArgs.scissorTest        = gcvFALSE;
                blitArgs.srcNumSlice        = srcSurface->requestD;
                blitArgs.dstNumSlice        = 1;
                blitArgs.needDecode         = sRGBDecodeEnable;
                status = gcoSURF_BlitCPU(&blitArgs);
                prevMap = map;
            }
        }
#if !gcdDUMP
          else
          {
              /* Skip SW GENERATION for low levels */
          }
#endif
    }


    gcmFOOTER();

    return status;
}

#else /* gcdNULL_DRIVER < 2 */


/*******************************************************************************
** NULL DRIVER STUBS
*/

gceSTATUS
gcoTEXTURE_ConstructEx(
    IN gcoHAL Hal,
    IN gceTEXTURE_TYPE Type,
    OUT gcoTEXTURE * Texture
    )
{
    *Texture = gcvNULL;
    return gcvSTATUS_OK;
}


gceSTATUS gcoTEXTURE_Construct(
    IN gcoHAL Hal,
    OUT gcoTEXTURE * Texture
    )
{
    *Texture = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_ConstructSized(
    IN gcoHAL Hal,
    IN gceSURF_FORMAT Format,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth,
    IN gctUINT Faces,
    IN gctUINT MipMapCount,
    IN gcePOOL Pool,
    OUT gcoTEXTURE * Texture
    )
{
    *Texture = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_Destroy(
    IN gcoTEXTURE Texture
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_Upload(
    IN gcoTEXTURE Texture,
    IN gctINT MipMap,
    IN gceTEXTURE_FACE Face,
    IN gctSIZE_T Width,
    IN gctSIZE_T Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctSIZE_T Stride,
    IN gceSURF_FORMAT Format,
    IN gceSURF_COLOR_SPACE SrcColorSpace
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoTEXTURE_UploadSub(
    IN gcoTEXTURE Texture,
    IN gctINT MipMap,
    IN gceTEXTURE_FACE Face,
    IN gctSIZE_T XOffset,
    IN gctSIZE_T YOffset,
    IN gctSIZE_T Width,
    IN gctSIZE_T Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctSIZE_T Stride,
    IN gceSURF_FORMAT Format,
    IN gceSURF_COLOR_SPACE SrcColorSpace,
    IN gctUINT32 PhysicalAddress
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_GetMipMap(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    OUT gcoSURF * Surface
    )
{
    *Surface = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_GetMipMapFace(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    IN gceTEXTURE_FACE Face,
    OUT gcoSURF * Surface,
    OUT gctSIZE_T_PTR Offset
    )
{
    *Surface = gcvNULL;
    *Offset = 0;
    return gcvSTATUS_OK;
}

gceSTATUS
gcoTEXTURE_GetMipMapSlice(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    IN gctUINT Slice,
    OUT gcoSURF * Surface,
    OUT gctSIZE_T_PTR Offset
    )
{
    *Surface = gcvNULL;
    *Offset = 0;
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_AddMipMap(
    IN gcoTEXTURE Texture,
    IN gctINT Level,
    IN gctINT InternalFormat,
    IN gceSURF_FORMAT Format,
    IN gctSIZE_T Width,
    IN gctSIZE_T Height,
    IN gctSIZE_T Depth,
    IN gctUINT Faces,
    IN gcePOOL Pool,
    IN gctBOOL Filterable,
    OUT gcoSURF * Surface
    )
{
    if (Surface)
    {
        *Surface = gcvNULL;
    }
    return gcvSTATUS_OK;
}

gceSTATUS
gcoTEXTURE_AddMipMapEx(
    IN gcoTEXTURE Texture,
    IN gctINT Level,
    IN gctINT InternalFormat,
    IN gceSURF_FORMAT Format,
    IN gctSIZE_T Width,
    IN gctSIZE_T Height,
    IN gctSIZE_T Depth,
    IN gctUINT Faces,
    IN gcePOOL Pool,
    IN gctUINT32 Samples,
    IN gctBOOL Protected,
    IN gctBOOL Filterable,
    OUT gcoSURF * Surface
    )
{
    if (Surface)
    {
        *Surface = gcvNULL;
    }
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_AddMipMapFromClient(
    IN gcoTEXTURE Texture,
    IN gctINT Level,
    IN gcoSURF Surface
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_AddMipMapFromSurface(
    IN gcoTEXTURE Texture,
    IN gctINT     Level,
    IN gcoSURF    Surface
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoTEXTURE_LockMipMap(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    OPTIONAL OUT gctUINT32 * Address,
    OPTIONAL OUT gctPOINTER * Memory
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_SetEndianHint(
    IN gcoTEXTURE Texture,
    IN gceENDIAN_HINT EndianHint
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_Disable(
    IN gcoHAL Hal,
    IN gctINT Sampler,
    IN gctBOOL DefaultInteger
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_Flush(
    IN gcoTEXTURE Texture
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_QueryCaps(
    IN  gcoHAL    Hal,
    OUT gctUINT * MaxWidth,
    OUT gctUINT * MaxHeight,
    OUT gctUINT * MaxDepth,
    OUT gctBOOL * Cubic,
    OUT gctBOOL * NonPowerOfTwo,
    OUT gctUINT * VertexSamplers,
    OUT gctUINT * PixelSamplers
    )
{
    *MaxWidth = 8192;
    *MaxHeight = 8192;
    *MaxDepth = 8192;
    *Cubic = gcvTRUE;
    *NonPowerOfTwo = gcvTRUE;
    if (VertexSamplers != gcvNULL)
    {
        *VertexSamplers = 4;
    }
    *PixelSamplers = 8;
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_GetClosestFormat(
    IN gcoHAL Hal,
    IN gceSURF_FORMAT InFormat,
    OUT gceSURF_FORMAT* OutFormat
    )
{
    *OutFormat = InFormat;
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_GetClosestFormatEx(
    IN gcoHAL Hal,
    IN gceSURF_FORMAT InFormat,
    IN gceTEXTURE_TYPE TextureType,
    OUT gceSURF_FORMAT* OutFormat
    )
{
    *OutFormat = InFormat;
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_UploadCompressed(
    IN gcoTEXTURE Texture,
    IN gctINT MipMap,
    IN gceTEXTURE_FACE Face,
    IN gctSIZE_T Width,
    IN gctSIZE_T Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctSIZE_T Size
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoTEXTURE_UploadCompressedSub(
    IN gcoTEXTURE Texture,
    IN gctINT MipMap,
    IN gceTEXTURE_FACE Face,
    IN gctSIZE_T XOffset,
    IN gctSIZE_T YOffset,
    IN gctSIZE_T Width,
    IN gctSIZE_T Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctSIZE_T Size
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_RenderIntoMipMap(
    IN gcoTEXTURE Texture,
    IN gctINT Level
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_RenderIntoMipMap2(
    IN gcoTEXTURE Texture,
    IN gctINT Level,
    IN gctBOOL Sync
    )
{
    return gcvSTATUS_OK;
}


gceSTATUS gcoTEXTURE_IsRenderable(
    IN gcoTEXTURE Texture,
    IN gctUINT Level
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_IsComplete(
    IN gcoTEXTURE Texture,
    IN gcsTEXTURE_PTR Info,
    IN gctINT BaseLevel,
    IN gctINT MaxLevel
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_BindTexture(
    IN gcoTEXTURE Texture,
    IN gctINT Target,
    IN gctINT Sampler,
    IN gcsTEXTURE_PTR Info
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoTEXTURE_InitParams(
    IN gcoHAL Hal,
    IN gcsTEXTURE_PTR TexParams
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoTEXTURE_SetDepthTextureFlag(
    IN gcoTEXTURE Texture,
    IN gctBOOL  unsized
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoTEXTURE_BindTextureTS(
    IN gcsTEXTURE_BINDTEXTS_ARGS * args
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoTEXTURE_SetDescDirty(
    IN gcoTEXTURE Texture
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoTEXTURE_BindTextureDesc(
    IN gcoTEXTURE Texture,
    IN gctINT Sampler,
    IN gcsTEXTURE_PTR Info,
    IN gctINT TextureLayer
    )
{
    return gcvSTATUS_OK;
}



gceSTATUS
gcoTEXTURE_GenerateMipMap(
    IN gcoTEXTURE Texture,
    IN gctINT BaseLevel,
    IN gctINT MaxLevel,
    IN gctBOOL sRGBDecode
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoTEXTURE_BindTextureEx(
    IN gcoTEXTURE Texture,
    IN gctINT Target,
    IN gctINT Sampler,
    IN gcsTEXTURE_PTR Info,
    IN gctINT textureLayer
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoTEXTURE_CheckTexLevel0Attrib(
    IN gcoTEXTURE Texture,
    IN gctINT MaxLevel,
    IN gctINT usedLevel
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoTEXTURE_FlushVS(
    IN gcoTEXTURE Texture
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoTEXTURE_GetFormatInfo(
    IN gcoTEXTURE Texture,
    IN gctINT preferLevel,
    OUT gcsSURF_FORMAT_INFO_PTR * TxFormatInfo
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_UploadYUV(
    IN gcoTEXTURE Texture,
    IN gceTEXTURE_FACE Face,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Slice,
    IN gctPOINTER Memory[3],
    IN gctINT Stride[3],
    IN gceSURF_FORMAT Format
    )
{
    return gcvSTATUS_OK;
}



#endif /* gcdNULL_DRIVER < 2 */
#endif

