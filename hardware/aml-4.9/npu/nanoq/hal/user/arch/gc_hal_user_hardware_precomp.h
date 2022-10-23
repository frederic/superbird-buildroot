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


#ifndef __gc_hal_user_hardware_precomp_h_
#define __gc_hal_user_hardware_precomp_h_

#include "gc_hal_user.h"
#include "gc_hal_user_hardware.h"

#if defined(__GNUC__) && !gcmIS_DEBUG(gcdDEBUG_TRACE)
    gcmINLINE static void
    __do_nothing(
        IN gctUINT32 Level,
        IN gctUINT32 Zone,
        IN gctCONST_STRING Message,
        ...
        )
    {
        static volatile int c;

        c++;
    }

#   undef  gcmTRACE_ZONE
#   define gcmTRACE_ZONE            __do_nothing
#endif

#endif /* __gc_hal_user_hardware_precomp_h_ */

