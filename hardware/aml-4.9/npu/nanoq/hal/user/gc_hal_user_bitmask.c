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
** The following are inline functions used for bitmask management
*/
#include "gc_hal_user_precomp.h"


#if (gcdENABLE_3D)
/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcdZONE_BUFFER

typedef struct _gcsBITMASKFUNCS
{
    gctBOOL (*test) (gcsBITMASK_PTR Bitmask, gctUINT32 Loc);
    void    (*set) (gcsBITMASK_PTR Bitmask, gctUINT32 Loc);
    void    (*or)(gcsBITMASK_PTR BitmaskResult, gcsBITMASK_PTR Bitmask1, gcsBITMASK_PTR Bitmask2);
    gctBOOL (*testAndClear)(gcsBITMASK_PTR Bitmask,  gctUINT32 Loc);
    gctBOOL (*isAllZero)(gcsBITMASK_PTR Bitmask);
    void    (*init)(gcsBITMASK_PTR Bitmask, gctBOOL AllOne);
    void    (*clear)(gcsBITMASK_PTR Bitmask, gctUINT32 Loc);
    void    (*setAll)(gcsBITMASK_PTR Bitmask, gctBOOL AllOne);
    void    (*setValue)(gcsBITMASK_PTR Bitmask, gctUINT32 Value);
}
gcsBITMASKFUNCS;



static gctBOOL seMaskTest(gcsBITMASK_PTR Bitmask, gctUINT32 Loc)
{
    gcmASSERT(Loc < Bitmask->size);
    return ((Bitmask->me[0] & ((gcmBITMASK_ELT_TYPE) 1 << Loc)) ? gcvTRUE: gcvFALSE);
}

static void seMaskSet(gcsBITMASK_PTR Bitmask, gctUINT32 Loc)
{
    gcmASSERT(Loc < Bitmask->size);
    Bitmask->me[0] |= (gcmBITMASK_ELT_TYPE) 1 << Loc;
}

static void seMaskOR(gcsBITMASK_PTR BitmaskResult, gcsBITMASK_PTR Bitmask1, gcsBITMASK_PTR Bitmask2)
{
    BitmaskResult->me[0] = Bitmask1->me[0] | Bitmask2->me[0];
}

static gctBOOL seMaskTestAndClear(gcsBITMASK_PTR Bitmask, gctUINT32 Loc)
{
    gcmASSERT(Loc < Bitmask->size);
    if (Bitmask->me[0] & ((gcmBITMASK_ELT_TYPE) 1 << Loc))
    {
        Bitmask->me[0] &= ~((gcmBITMASK_ELT_TYPE) 1 << Loc);
        return gcvTRUE;
    }
    return gcvFALSE;
}

static gctBOOL seMaskIsAllZero(gcsBITMASK_PTR Bitmask)
{
    return (Bitmask->me[0]== (gcmBITMASK_ELT_TYPE) 0);
}

static void seMaskInit(gcsBITMASK_PTR Bitmask, gctBOOL AllOne)
{
    Bitmask->numOfElts = 1;
    Bitmask->me[0] = AllOne ?  ((gcmBITMASK_ELT_TYPE) ~0 >> (gcmBITMASK_ELT_BITS - Bitmask->size))
                            :  (gcmBITMASK_ELT_TYPE) 0;
}

static void seMaskSetAll(gcsBITMASK_PTR Bitmask, gctBOOL AllOne)
{
    Bitmask->me[0] = AllOne ?  ((gcmBITMASK_ELT_TYPE) ~0 >> (gcmBITMASK_ELT_BITS - Bitmask->size))
                            :  (gcmBITMASK_ELT_TYPE) 0;
}

static void seMaskClear(gcsBITMASK_PTR Bitmask, gctUINT32 Loc)
{
    gcmASSERT(Loc < Bitmask->size);
    Bitmask->me[0] &= ~((gcmBITMASK_ELT_TYPE) 1 << Loc);
}

static void seMaskSetValue(gcsBITMASK_PTR Bitmask, gctUINT32 Value)
{
    gcmASSERT(Bitmask->size >= 32);
    Bitmask->me[0] = (gcmBITMASK_ELT_TYPE) Value;
}


static gcsBITMASKFUNCS seMaskFuncs =
{
    seMaskTest,
    seMaskSet,
    seMaskOR,
    seMaskTestAndClear,
    seMaskIsAllZero,
    seMaskInit,
    seMaskClear,
    seMaskSetAll,
    seMaskSetValue,
};

static gctBOOL meMaskTest(gcsBITMASK_PTR Bitmask, gctUINT32 Loc)
{
    gcmASSERT(Loc < Bitmask->size);
    return ((Bitmask->me[Loc / gcmBITMASK_ELT_BITS] & ((gcmBITMASK_ELT_TYPE) 1 << (Loc % gcmBITMASK_ELT_BITS))) ? gcvTRUE: gcvFALSE);
}

static void meMaskSet(gcsBITMASK_PTR Bitmask, gctUINT32 Loc)
{
    gcmASSERT(Loc < Bitmask->size);
    Bitmask->me[Loc / gcmBITMASK_ELT_BITS] |= ((gcmBITMASK_ELT_TYPE) 1 << (Loc % gcmBITMASK_ELT_BITS));
}

static void meMaskOR(gcsBITMASK_PTR BitmaskResult, gcsBITMASK_PTR Bitmask1, gcsBITMASK_PTR Bitmask2)
{
    gctUINT32 i;
    gctUINT32 minIndex = gcmMIN(Bitmask1->numOfElts, Bitmask2->numOfElts);
    for (i = 0; i < minIndex; i++)
    {
        BitmaskResult->me[i] = Bitmask1->me[i] | Bitmask2->me[i];
    }
}

static gctBOOL meMaskTestAndClear(gcsBITMASK_PTR Bitmask, gctUINT32 Loc)
{
    gcmASSERT(Loc < Bitmask->size);
    if (Bitmask->me[Loc / gcmBITMASK_ELT_BITS] & ((gcmBITMASK_ELT_TYPE) 1 << (Loc % gcmBITMASK_ELT_BITS)))
    {
        Bitmask->me[Loc / gcmBITMASK_ELT_BITS] &= ~((gcmBITMASK_ELT_TYPE) 1 << (Loc % gcmBITMASK_ELT_BITS));
        return gcvTRUE;
    }
    return gcvFALSE;
}

static gctBOOL meMaskIsAllZero(gcsBITMASK_PTR Bitmask)
{
    gctUINT32 i;
    for (i = 0; i < Bitmask->numOfElts; i++)
    {
        if (Bitmask->me[i])
        {
            return gcvFALSE;
        }
    }

    return gcvTRUE;
}


static void meMaskInit(gcsBITMASK_PTR Bitmask, gctBOOL AllOne)
{
    gctUINT32 i;
    Bitmask->numOfElts = (Bitmask->size + (gcmBITMASK_ELT_BITS -1)) / gcmBITMASK_ELT_BITS;
    Bitmask->remainedSize = Bitmask->size & (gcmBITMASK_ELT_BITS -1);
    gcmASSERT(Bitmask->numOfElts <= gcmBITMASK_ELT_MAXNUM);

    for (i = 0; i < Bitmask->numOfElts; i++)
    {
        Bitmask->me[i] = AllOne ? (gcmBITMASK_ELT_TYPE) ~0 : 0;
    }

    if (Bitmask->remainedSize)
    {
        Bitmask->me[Bitmask->numOfElts-1] >>= (gcmBITMASK_ELT_BITS - Bitmask->remainedSize);
    }
}

static void meMaskSetAll(gcsBITMASK_PTR Bitmask, gctBOOL AllOne)
{
    gctUINT32 i;
    for (i = 0; i < Bitmask->numOfElts; i++)
    {
        Bitmask->me[i] = AllOne ? (gcmBITMASK_ELT_TYPE) ~0 : 0;
    }

    if (Bitmask->remainedSize)
    {
        Bitmask->me[Bitmask->numOfElts-1] >>= (gcmBITMASK_ELT_BITS - Bitmask->remainedSize);
    }
}


static void meMaskClear(gcsBITMASK_PTR Bitmask, gctUINT32 Loc)
{
    Bitmask->me[Loc / gcmBITMASK_ELT_BITS] &= ~((gcmBITMASK_ELT_TYPE) 1 << (Loc % gcmBITMASK_ELT_BITS));
}

static void meMaskSetValue(gcsBITMASK_PTR Bitmask, gctUINT32 Value)
{
    Bitmask->me[0] = (gcmBITMASK_ELT_TYPE) Value;
}


static gcsBITMASKFUNCS meMaskFuncs =
{
    meMaskTest,
    meMaskSet,
    meMaskOR,
    meMaskTestAndClear,
    meMaskIsAllZero,
    meMaskInit,
    meMaskClear,
    meMaskSetAll,
    meMaskSetValue,
};

/*
** Initialize size Bitmask with all bit armed.
*/
void
gcsBITMASK_InitAllOne(
    gcsBITMASK_PTR Bitmask,
    gctUINT32 size
    )
{
    Bitmask->size = size;

    gcmASSERT(size <= gcmBITMASK_ELT_BITS * gcmBITMASK_ELT_MAXNUM);

    if (size <= gcmBITMASK_ELT_BITS)
    {
        Bitmask->op = &seMaskFuncs;
    }
    else
    {
        Bitmask->op = &meMaskFuncs;
    }

    (*Bitmask->op->init)(Bitmask, gcvTRUE);

    return;
}

/*
** Initialize sized Bitmask with all zero set.
*/
void
gcsBITMASK_InitAllZero(
    gcsBITMASK_PTR Bitmask,
    gctUINT32 size
    )
{
    Bitmask->size = size;

    gcmASSERT(size <= gcmBITMASK_ELT_BITS * gcmBITMASK_ELT_MAXNUM);

    if (size <= gcmBITMASK_ELT_BITS)
    {
        Bitmask->op = &seMaskFuncs;
    }
    else
    {
        Bitmask->op = &meMaskFuncs;
    }

    (*Bitmask->op->init)(Bitmask, gcvFALSE);

    return;
}

/*
** Initialize BitmaskResult with Bitmask1 or Bitmask2
** BitmaskResult can NOT be either of Bitmask1 or Bitmask2.
*/
void
gcsBITMASK_InitOR(
    gcsBITMASK_PTR BitmaskResult,
    gcsBITMASK_PTR Bitmask1,
    gcsBITMASK_PTR Bitmask2
    )
{
    gctUINT32 size = gcmMAX(Bitmask1->size, Bitmask2->size);

    gcsBITMASK_InitAllZero(BitmaskResult, size);

    (*BitmaskResult->op->or)(BitmaskResult, Bitmask1, Bitmask2);
}

/*
** Initialize a Bitmask with 32bit value
*/
void
gcsBITMASK_InitWithValue(
    gcsBITMASK_PTR BitmaskResult,
    gctUINT32 InitValue
    )
{
    gctUINT32 size = 32;

    gcsBITMASK_InitAllZero(BitmaskResult, size);

    (*BitmaskResult->op->setValue)(BitmaskResult, InitValue);
}

/*
** Check Loc bit is armed or not in Bitmask
*/
gctBOOL
gcsBITMASK_Test(
    gcsBITMASK_PTR Bitmask,
    gctUINT32 Loc
    )
{
    return (*Bitmask->op->test)(Bitmask, Loc);
}

/*
** Check Loc bit is armed and clear it
*/
gctBOOL
gcsBITMASK_TestAndClear(
    gcsBITMASK_PTR Bitmask,
    gctUINT32 Loc
    )
{
    return (*Bitmask->op->testAndClear)(Bitmask, Loc);
}

/*
** Check all bits are armed in Bitmask or not
*/
gctBOOL
gcsBITMASK_IsAllZero(
    gcsBITMASK_PTR Bitmask
    )
{
    return (*Bitmask->op->isAllZero)(Bitmask);
}

/*
** Arm Loc bit in Bitmask
*/
void
gcsBITMASK_Set(
    gcsBITMASK_PTR Bitmask,
    gctUINT32 Loc
    )
{
    (*Bitmask->op->set)(Bitmask, Loc);
}

/*
** Clear Loc bit in Bitmask
*/
void
gcsBITMASK_Clear(
    gcsBITMASK_PTR Bitmask,
    gctUINT32 Loc
    )
{
    (*Bitmask->op->clear)(Bitmask, Loc);
}

/*
** Clear or set all bits in Bitmask
*/
void
gcsBITMASK_SetAll(
    gcsBITMASK_PTR Bitmask,
    gctBOOL AllOne
    )
{
    (*Bitmask->op->setAll)(Bitmask, AllOne);
}

/*
**  Merge multiple[count] Bitmask in BitmaskArray into BitmaskResult
*/
void
gcsBITMASK_MergeBitMaskArray(
    gcsBITMASK_PTR BitmaskResult,
    gcsBITMASK_PTR *BitmaskArray,
    gctUINT32 Count
    )
{
    gctUINT32 i;
    for (i = 0; i < Count; i++)
    {
        (*BitmaskResult->op->or)(BitmaskResult, BitmaskResult, BitmaskArray[i]);
    }
    return;
}

/*
** Merge Bitmask into BitmaskResult
*/
void
gcsBITMASK_OR(
    gcsBITMASK_PTR BitmaskResult,
    gcsBITMASK_PTR Bitmask
    )
{
    (*BitmaskResult->op->or)(BitmaskResult, BitmaskResult, Bitmask);
}

#endif/* #if (gcdENABLE_3D || gcdENABLE_2D) */
