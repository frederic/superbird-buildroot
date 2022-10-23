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


/*******************************************************************************
**
**  gcoHARDWARE_TranslateDestinationFormat
**
**  Translate API destination color format to its hardware value.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gceSURF_FORMAT APIValue
**          API value.
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Corresponding hardware value.
*/
gceSTATUS gcoHARDWARE_TranslateDestinationFormat(
    IN gcoHARDWARE Hardware,
    IN gceSURF_FORMAT APIValue,
    IN gctBOOL EnableXRGB,
    OUT gctUINT32* HwValue,
    OUT gctUINT32* HwSwizzleValue,
    OUT gctUINT32* HwIsYUVValue
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;

    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_TranslateDestinationTransparency
**
**  Translate API transparency mode to its hardware value.
**  MASK transparency mode is reserved.
**
**  INPUT:
**
**      gce2D_TRANSPARENCY APIValue
**          API value.
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Corresponding hardware value.
*/
gceSTATUS gcoHARDWARE_TranslateDestinationTransparency(
    IN gce2D_TRANSPARENCY APIValue,
    OUT gctUINT32 * HwValue
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gcoHARDWARE_TranslateDestinationRotation
**
**  Translate API transparency mode to its hardware value.
**
**  INPUT:
**
**      gce2D_TRANSPARENCY APIValue
**          API value.
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Corresponding hardware value.
*/

