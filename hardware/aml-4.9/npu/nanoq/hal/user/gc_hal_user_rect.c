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
**    @file
**    gcsRECT_PTR structre for user HAL layers.
**
*/

#include "gc_hal_user_precomp.h"

#define _GC_OBJ_ZONE        gcdZONE_RECT

/******************************************************************************\
******************************** gcsRECT_PTR API Code ******************************
\******************************************************************************/

/*******************************************************************************
**
**    gcsRECT_Set
**
**    Initialize rectangle structure.
**
**    INPUT:
**
**        gctINT32 Left
**        gctINT32 Top
**        gctINT32 Right
**        gctINT32 Bottom
**            Coordinates of the rectangle to set.
**
**    OUTPUT:
**
**        gcsRECT_PTR Rect
**            Initialized rectangle structure.
*/
gceSTATUS
gcsRECT_Set(
    OUT gcsRECT_PTR Rect,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom
    )
{
    gcmHEADER_ARG("Left=%d Top=%d Right=%d Bottom=%d",
                  Left, Top, Right, Bottom);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Rect != gcvNULL);

    /* Set coordinates. */
    Rect->left = Left;
    Rect->top = Top;
    Rect->right = Right;
    Rect->bottom = Bottom;

    /* Success. */
    gcmFOOTER_ARG("Rect=0x%x", Rect);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**    gcsRECT_Width
**
**    Return the width of the rectangle.
**
**    INPUT:
**
**        gcsRECT_PTR Rect
**            Pointer to a valid rectangle structure.
**
**    OUTPUT:
**
**        gctINT32 * Width
**            Pointer to a variable that will receive the width of the rectangle.
*/
gceSTATUS
gcsRECT_Width(
    IN gcsRECT_PTR Rect,
    OUT gctINT32 * Width
    )
{
    gcmHEADER_ARG("Rect=0x%x", Rect);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Rect != gcvNULL);
    gcmVERIFY_ARGUMENT(Width != gcvNULL);

    /* Compute and return width. */
    *Width = Rect->right - Rect->left;

    /* Success. */
    gcmFOOTER_ARG("*Width=%d", *Width);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**    gcsRECT_Height
**
**    Return the height of the rectangle.
**
**    INPUT:
**
**        gcsRECT_PTR Rect
**            Pointer to a valid rectangle structure.
**
**    OUTPUT:
**
**        gctINT32 * Height
**            Pointer to a variable that will receive the height of the rectangle.
*/
gceSTATUS
gcsRECT_Height(
    IN gcsRECT_PTR Rect,
    OUT gctINT32 * Height
    )
{
    gcmHEADER_ARG("Rect=0x%x", Rect);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Rect != gcvNULL);
    gcmVERIFY_ARGUMENT(Height != gcvNULL);

    /* Compute and return height. */
    *Height = Rect->bottom - Rect->top;

    /* Success. */
    gcmFOOTER_ARG("*Width=%d", *Height);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**    gcsRECT_Normalize
**
**    Ensures that top left corner is to the left and above the right bottom.
**
**    INPUT:
**
**        gcsRECT_PTR Rect
**            Pointer to a valid rectangle structure.
**
**    OUTPUT:
**
**        gcsRECT_PTR Rect
**            Normalized rectangle.
*/
gceSTATUS
gcsRECT_Normalize(
    IN OUT gcsRECT_PTR Rect
    )
{
    gctINT32 temp;

    gcmHEADER_ARG("Rect=0x%x", Rect);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Rect != gcvNULL);

    if (Rect->left > Rect->right)
    {
        /* Swap left and right coordinates. */
        temp = Rect->left;
        Rect->left = Rect->right;
        Rect->right = temp;
    }

    if (Rect->top > Rect->bottom)
    {
        /* Swap top and bottom coordinates. */
        temp = Rect->top;
        Rect->top = Rect->bottom;
        Rect->bottom = temp;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**    gcsRECT_IsEqual
**
**    Compare two rectangles.
**
**    INPUT:
**
**        gcsRECT_PTR Rect1
**            Pointer to a valid rectangle structure.
**
**        gcsRECT_PTR Rect2
**            Pointer to a valid rectangle structure.
**
**    OUTPUT:
**
**        gctBOOL * Equal
**          Poniter to a variable that will receive a gcvTRUE when the rectangles
**          are equal or gcvFALSE when they are not.
*/
gceSTATUS
gcsRECT_IsEqual(
    IN gcsRECT_PTR Rect1,
    IN gcsRECT_PTR Rect2,
    OUT gctBOOL * Equal
    )
{
    gcmHEADER_ARG("Rect1=0x%x Rect2=0x%x", Rect1, Rect2);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Rect1 != gcvNULL);
    gcmVERIFY_ARGUMENT(Rect2 != gcvNULL);
    gcmVERIFY_ARGUMENT(Equal != gcvNULL);

    /* Compute and return equality. */
    *Equal = (Rect1->left == Rect2->left) &&
             (Rect1->top == Rect2->top) &&
             (Rect1->right == Rect2->right) &&
             (Rect1->bottom == Rect2->bottom);

    /* Success. */
    gcmFOOTER_ARG("*Equal=%d", *Equal);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**    gcsRECT_IsOfEqualSize
**
**    Compare the sizes of two rectangles.
**
**    INPUT:
**
**        gcsRECT_PTR Rect1
**            Pointer to a valid rectangle structure.
**
**        gcsRECT_PTR Rect2
**            Pointer to a valid rectangle structure.
**
**    OUTPUT:
**
**        gctBOOL * EqualSize
**           Pointer to a variable that will receive gcvTRUE when the rectangles
**          are of equal size or gcvFALSE if tey are not.
*/
gceSTATUS
gcsRECT_IsOfEqualSize(
    IN gcsRECT_PTR Rect1,
    IN gcsRECT_PTR Rect2,
    OUT gctBOOL * EqualSize
    )
{
    gcmHEADER_ARG("Rect1=0x%x Rect2=0x%x", Rect1, Rect2);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Rect1 != gcvNULL);
    gcmVERIFY_ARGUMENT(Rect2 != gcvNULL);
    gcmVERIFY_ARGUMENT(EqualSize != gcvNULL);

    /* Commpute and return equality. */
    *EqualSize =
        ((Rect1->right - Rect1->left) == (Rect2->right - Rect2->left)) &&
        ((Rect1->bottom - Rect1->top) == (Rect2->bottom - Rect2->top));

    /* Success. */
    gcmFOOTER_ARG("*EqualSize=%d", *EqualSize);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**    gcsRECT_RelativeRotation
**
**    Compute the related rotation based on Orientation.
**
**    INPUT:
**
**        gceSURF_ROTATION Orientation
**            Orientation rotation.
**
**        gceSURF_ROTATION *Relation
**            Pointer to a rotation.
**
**    OUTPUT:
**
**        gceSURF_ROTATION *Relation
**           Pointer to the related rotation based on Orientation.
*/
gceSTATUS
gcsRECT_RelativeRotation(
    IN gceSURF_ROTATION Orientation,
    IN OUT gceSURF_ROTATION *Relation)
{
    gceSURF_ROTATION o = gcmGET_PRE_ROTATION(Orientation);
    gceSURF_ROTATION t = gcmGET_PRE_ROTATION(*Relation);

    if (o == gcvSURF_FLIP_X || o == gcvSURF_FLIP_Y)
    {
        o = gcvSURF_0_DEGREE;
    }

    if (t == gcvSURF_FLIP_X || t == gcvSURF_FLIP_Y)
    {
        t = gcvSURF_0_DEGREE;
    }

    switch (o)
    {
    case gcvSURF_0_DEGREE:
        break;

    case gcvSURF_90_DEGREE:
        switch (t)
        {
        case gcvSURF_0_DEGREE:
            t = gcvSURF_270_DEGREE;
            break;

        case gcvSURF_90_DEGREE:
            t = gcvSURF_0_DEGREE;
            break;

        case gcvSURF_180_DEGREE:
            t = gcvSURF_90_DEGREE;
            break;

        case gcvSURF_270_DEGREE:
            t = gcvSURF_180_DEGREE;
            break;

        default:
            return gcvSTATUS_NOT_SUPPORTED;
        }
        break;

    case gcvSURF_180_DEGREE:
        switch (t)
        {
        case gcvSURF_0_DEGREE:
            t = gcvSURF_180_DEGREE;
            break;

        case gcvSURF_90_DEGREE:
            t = gcvSURF_270_DEGREE;
            break;

        case gcvSURF_180_DEGREE:
            t = gcvSURF_0_DEGREE;
            break;

        case gcvSURF_270_DEGREE:
            t = gcvSURF_90_DEGREE;
            break;

        default:
            return gcvSTATUS_NOT_SUPPORTED;
        }
        break;

    case gcvSURF_270_DEGREE:
        switch (t)
        {
        case gcvSURF_0_DEGREE:
            t = gcvSURF_90_DEGREE;
            break;

        case gcvSURF_90_DEGREE:
            t = gcvSURF_180_DEGREE;
            break;

        case gcvSURF_180_DEGREE:
            t = gcvSURF_270_DEGREE;
            break;

        case gcvSURF_270_DEGREE:
            t = gcvSURF_0_DEGREE;
            break;

        default:
            return gcvSTATUS_NOT_SUPPORTED;
        }
        break;

    default:
        return gcvSTATUS_NOT_SUPPORTED;
    }

    *Relation = t | gcmGET_POST_ROTATION(*Relation);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**    gcsRECT_Rotate
**
**    Compute the related rotation based on Orientation.
**
**    INPUT:
**
**      gcsRECT_PTR Rect
**          Pointer to the rectangle to be rotated.
**
**        gceSURF_ROTATION Rotation
**            Original rotation.
**
**        gceSURF_ROTATION toRotation
**            Target rotation.
**
**      gctINT32 SurfaceWidth
**          The width of surface.
**
**      gctINT32 SurfaceHeight
**          The height of surface.
**
**    OUTPUT:
**
**        gcsRECT_PTR Rect
**           Pointer to the rectangle which has been rotation to toRotation.
*/
gceSTATUS
gcsRECT_Rotate(
    IN OUT gcsRECT_PTR Rect,
    IN gceSURF_ROTATION Rotation,
    IN gceSURF_ROTATION toRotation,
    IN gctINT32 SurfaceWidth,
    IN gctINT32 SurfaceHeight
    )
{
    gceSTATUS status;
    gctINT32 temp;
    gceSURF_ROTATION rot = gcmGET_PRE_ROTATION(Rotation);
    gceSURF_ROTATION tRot = gcmGET_PRE_ROTATION(toRotation);

    gcmHEADER_ARG("Rect=0x%x, Rotation=%d, toRotation=%d, Width=%d, Height=%d",
        Rect, Rotation, toRotation, SurfaceWidth, SurfaceHeight);

    /* Verify the arguments. */
    if ((Rect == gcvNULL) || (Rect->right <= Rect->left)
        || (Rect->bottom <= Rect->top))
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    if (tRot == gcvSURF_90_DEGREE
        || tRot == gcvSURF_270_DEGREE)
    {
        temp = SurfaceWidth;
        SurfaceWidth = SurfaceHeight;
        SurfaceHeight = temp;
    }

    gcmONERROR(gcsRECT_RelativeRotation(tRot, &rot));

    switch (rot)
    {
    case gcvSURF_0_DEGREE:
        break;

    case gcvSURF_90_DEGREE:
        if ((SurfaceWidth < Rect->bottom) || (SurfaceWidth < Rect->top))
        {
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

        temp = Rect->left;
        Rect->left = SurfaceWidth - Rect->bottom;
        Rect->bottom = Rect->right;
        Rect->right = SurfaceWidth - Rect->top;
        Rect->top = temp;

        break;

    case gcvSURF_180_DEGREE:
        if ((SurfaceWidth < Rect->right) || (SurfaceWidth < Rect->left)
            || (SurfaceHeight < Rect->bottom) || (SurfaceHeight < Rect->top))
        {
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

        temp = Rect->left;
        Rect->left = SurfaceWidth - Rect->right;
        Rect->right = SurfaceWidth - temp;
        temp = Rect->top;
        Rect->top = SurfaceHeight - Rect->bottom;
        Rect->bottom = SurfaceHeight - temp;

        break;

    case gcvSURF_270_DEGREE:
        if ((SurfaceHeight < Rect->right) || (SurfaceHeight < Rect->left))
        {
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

        temp = Rect->left;
        Rect->left = Rect->top;
        Rect->top = SurfaceHeight - Rect->right;
        Rect->right = Rect->bottom;
        Rect->bottom = SurfaceHeight - temp;

        break;

    default:
        gcmFOOTER_ARG("Status=%d", gcvSTATUS_NOT_SUPPORTED);
        return gcvSTATUS_NOT_SUPPORTED;
    }

    gcmFOOTER_ARG("Rect=(%d, %d, %d, %d)",
        Rect->left, Rect->top, Rect->right, Rect->bottom);
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}
