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
****************************** gcoHARDWARE API Code *****************************
\******************************************************************************/

/*******************************************************************************
**
**  gcoHARDWARE_SelectPipe
**
**  Select the current pipe for this hardare context.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an AQHARWDARE object.
**
**      gcePIPE_SELECT Pipe
**          Pipe to select.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_SelectPipe(
    IN gcoHARDWARE Hardware,
    IN gcePIPE_SELECT Pipe,
    INOUT gctPOINTER *Memory
    )
{
    gceSTATUS status = gcvSTATUS_OK;

#if gcdENABLE_3D
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);
#endif
    gcmHEADER_ARG("Hardware=0x%x Pipe=%d", Hardware, Pipe);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Is 2D pipe present? */
    if ((Pipe == gcvPIPE_2D) && !Hardware->hw2DEngine)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

#if gcdENABLE_3D
    if ((Pipe == gcvPIPE_3D) && !Hardware->features[gcvFEATURE_PIPE_3D])
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }
#endif

    /* Don't do anything if the pipe has already been selected. */
    if (Hardware->currentPipe != Pipe)
    {
#if gcdENABLE_3D
        /* Set new pipe. */
        Hardware->currentPipe = Pipe;
        /* Flush current pipe. */
        gcmONERROR(gcoHARDWARE_FlushPipe(Hardware, Memory));

        /* Send semaphore and stall. */
        gcmONERROR(gcoHARDWARE_Semaphore(
            Hardware, gcvWHERE_COMMAND, gcvWHERE_PIXEL, gcvHOW_SEMAPHORE_STALL, Memory
            ));

        gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, Memory);

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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E00) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0E00, Pipe);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


        gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, Memory);

        /* Avoid build warning. */
        stateDelta = stateDelta;
#endif
    }

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

