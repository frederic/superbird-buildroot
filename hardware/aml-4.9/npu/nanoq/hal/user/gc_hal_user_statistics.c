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
/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcdZONE_UTILITY

void _calculateTotal(
    gctUINT Start,
    gctFLOAT_PTR Total
    )
{
    /* Get stat object */
    gcsSTATISTICS* stat = &(gcPLS.hal->statistics);
    gctUINT i;
    gctUINT64 total = 0;

    /* Subtract values and find total */
    for (i = VIV_STAT_EARLY_Z_LATENCY_FRAMES; i < VIV_STAT_EARLY_Z_SAMPLE_FRAMES; i++)
    {
        total += stat->frameTime[Start + i];
    }

    /* find avg */
    *Total = (gctFLOAT) total;
}

void
_decideOnEarlyZMode(
    void
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gco3D engine;
    /* get stat and earlyZ */
    gcsSTATISTICS* stat = &(gcPLS.hal->statistics);
    gcsSTATISTICS_EARLYZ* earlyZ = &(stat->earlyZ);
    status = gco3D_Get3DEngine(&engine);
    if(status != gcvSTATUS_OK)
    {
        return;
    }

    if (!(earlyZ->disabled))
    {
        if (earlyZ->nextCheckPoint < stat->frame)
        {
            /* Self correction. It may happen after a call to gcfSTATISTICS_DisableDynamicEarlyZ */
            earlyZ->nextCheckPoint = stat->frame - (stat->frame % VIV_STAT_FRAME_BUFFER_SIZE) + VIV_STAT_FRAME_BUFFER_SIZE;
        }
        else if (earlyZ->nextCheckPoint == stat->frame)
        {
            gctUINT windowFrame = stat->frame % VIV_STAT_FRAME_BUFFER_SIZE;
            const gctUINT start = VIV_STAT_FRAME_BUFFER_SIZE - VIV_STAT_EARLY_Z_SAMPLE_FRAMES - 1;

            if (windowFrame == 0)
            {
                /* next check point */
                earlyZ->nextCheckPoint += start;
            }
            else if (windowFrame == start)
            {
                /* Switch to HZ off */
                gco3D_SwitchDynamicEarlyDepthMode(engine);

                /* next check point */
                earlyZ->nextCheckPoint += VIV_STAT_EARLY_Z_SAMPLE_FRAMES;
            }
            else if (windowFrame == (start + VIV_STAT_EARLY_Z_SAMPLE_FRAMES))
            {

                gctFLOAT prev;
                gctFLOAT curr;

                /* check if the previous data is there */
                _calculateTotal(VIV_STAT_FRAME_BUFFER_SIZE - (2 * VIV_STAT_EARLY_Z_SAMPLE_FRAMES), &prev);
                _calculateTotal(VIV_STAT_FRAME_BUFFER_SIZE - (1 * VIV_STAT_EARLY_Z_SAMPLE_FRAMES), &curr);

                if ((prev * VIV_STAT_EARLY_Z_FACTOR) < curr)
                {
                    /* previous mode was better, roll back HZ change */
                    gco3D_SwitchDynamicEarlyDepthMode(engine);

                    /* advance switch back count */
                    earlyZ->switchBackCount += (earlyZ->switchBackCount == 8 ? 0 : 1);
                }
                else
                {
                    /* No switch needed, new mode is better. Reset switch back count */
                    earlyZ->switchBackCount = 0;
                }

                earlyZ->nextCheckPoint += 1;
                earlyZ->nextCheckPoint += earlyZ->switchBackCount * VIV_STAT_FRAME_BUFFER_SIZE;
            }
        }
   }
}

/* Marks the frame end and triggers statistical calculations and decisions.*/
void
gcfSTATISTICS_MarkFrameEnd (
    )
{

#if VIV_STAT_ENABLE_STATISTICS
    /* Get stat object */
    gcsSTATISTICS* stat = &(gcPLS.hal->statistics);

    gctUINT64 currentTime;

    gcmHEADER();
    /* Get Time */
    gcoOS_GetTime(&currentTime);

    if (stat->previousFrameTime == (gctUINT64) 0)
    {
        stat->previousFrameTime = currentTime;
    }
    else
    {
        /* Get time elapsed inside the frame */
        gctUINT64 elapsedTime = currentTime - stat->previousFrameTime;
        gctUINT currentFrame = stat->frame % VIV_STAT_FRAME_BUFFER_SIZE;

        /* set elapsed time */
        stat->frameTime[currentFrame] = elapsedTime;

        /* Call decide functions */
        _decideOnEarlyZMode();

        /* advance frameNo */
        stat->frame += 1;

        /* set prev time */
        stat->previousFrameTime = currentTime;
    }

    /* Return the status. */
    gcmFOOTER_NO();

#endif /* VIV_STAT_ENABLE_STATISTICS */
}

/* Sets whether the dynmaic HZ is disabled or not .*/
void
gcfSTATISTICS_DisableDynamicEarlyZ (
    IN gctBOOL Disabled
    )
{
#if VIV_STAT_ENABLE_STATISTICS

    /* get stat and earlyZ */
    gcsSTATISTICS* stat = &(gcPLS.hal->statistics);
    gcsSTATISTICS_EARLYZ* earlyZ = &(stat->earlyZ);

    /* Set Mode */
    earlyZ->disabled = Disabled;

    if (Disabled)
    {
        /* set gco3d false because it needs to get out of the way. */
        gco3D_DisableDynamicEarlyDepthMode(gcvNULL, gcvFALSE);
    }
#endif /* VIV_STAT_ENABLE_STATISTICS */
}

/* Add a frame based data into current statistics. */
void
gcfSTATISTICS_AddData(
    IN gceSTATISTICS Key,
    IN gctUINT Value
    )
{
    /* To be implemented */
}
#endif
