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
**  gcoHAL object for user HAL layers.
**
*/

#include "gc_hal_user_precomp.h"
#if defined(__QNXNTO__)
#include <sys/mman.h>
#endif

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcdZONE_HAL_API

/*******************************************************************************
***** Version Signature *******************************************************/

#define _gcmTXT2STR(t) #t
#define gcmTXT2STR(t) _gcmTXT2STR(t)
const char * _GAL_VERSION = "\n\0$VERSION$"
                            gcmTXT2STR(gcvVERSION_MAJOR) "."
                            gcmTXT2STR(gcvVERSION_MINOR) "."
                            gcmTXT2STR(gcvVERSION_PATCH) ":"
                            gcmTXT2STR(gcvVERSION_BUILD) "$\n";

/******************************************************************************\
******************************** gcoHAL API Code ********************************
\******************************************************************************/

/*******************************************************************************
**
**  _GetUserDebugOption
**
**  Get user option from env variable VIV_OPTION.
**
**  INPUT:
**
**      gctHAL Hal
**          Pointer to HAL.
**
**  OUTPUT:
**
**      Nothing.
*/
gcsUSER_DEBUG_OPTION gcUserDebugOption =
{
    gcvDEBUG_MSG_NONE       /* gceDEBUG_MSG        debugMsg */
};

static gceSTATUS
_GetUserDebugOption(
    IN gcoHAL Hal
    )
{
    static gctINT envChecked = 0;

    if (!envChecked)
    {
        char* p = gcvNULL;
        gctSTRING pos = gcvNULL;

        gcoOS_GetEnv(gcvNULL, "VIV_DEBUG", &p);
        if (p)
        {
            gcoOS_StrStr(p, "-MSG_LEVEL", &pos);
            if (pos)
            {
                pos += sizeof("-MSG_LEVEL") - 1;
                while (pos[0] == ':')
                {
                    ++pos;
                    if (gcvSTATUS_OK == gcoOS_StrNCmp(pos, "ERROR", sizeof("ERROR") - 1))
                    {
                        /* Output error messages. */
                        gcUserDebugOption.debugMsg = gcvDEBUG_MSG_ERROR;
                        pos += sizeof("ERROR") - 1;
                    }
                    else if (gcvSTATUS_OK == gcoOS_StrNCmp(pos, "WARNING", sizeof("WARNING") - 1))
                    {
                        /* Output error messages. */
                        gcUserDebugOption.debugMsg = gcvDEBUG_MSG_WARNING;
                        pos += sizeof("WARNING") - 1;
                    }
                }
            }
        }

        envChecked = 1;
    }

    Hal->userDebugOption = &gcUserDebugOption;

    return gcvSTATUS_OK;
}

gctUINT32 gcFrameInfos[gcvFRAMEINFO_COUNT] = {0};


gceSTATUS
gcoHAL_FrameInfoOps(
    IN gcoHAL Hal,
    IN gceFRAMEINFO FrameInfo,
    IN gceFRAMEINFO_OP Op,
    IN OUT gctUINT * Val
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Hal=0x%x FrameInfo=%d Op=%d Val=0x%x", Hal, FrameInfo, Op, Val);
    gcmVERIFY_ARGUMENT(FrameInfo < gcvFRAMEINFO_COUNT);
    gcmVERIFY_ARGUMENT(Op < gcvFRAMEINFO_OP_COUNT);


    switch(Op)
    {
    case gcvFRAMEINFO_OP_ZERO:
        gcFrameInfos[FrameInfo] = 0;
        break;

    case gcvFRAMEINFO_OP_INC:
        gcFrameInfos[FrameInfo]++;
        break;

    case gcvFRAMEINFO_OP_DEC:
        if (gcFrameInfos[FrameInfo] == 0)
        {
            gcmPRINT("GAL: FramInfo(%d) underflowed", FrameInfo);
        }
        gcFrameInfos[FrameInfo]--;
        break;


    case gcvFRAMEINFO_OP_GET:
        gcmVERIFY_ARGUMENT(Val != gcvNULL);
        *Val = gcFrameInfos[FrameInfo];
        break;

    case gcvFRAMEINFO_OP_SET:
        gcmVERIFY_ARGUMENT(Val != gcvNULL);
        gcFrameInfos[FrameInfo] = *Val;
        break;

    default:
        gcmPRINT("GAL: invalid FrameInfo operation(%d)", Op);
    }
    gcmFOOTER_NO();
    return status;
}

gctBOOL gcOptions[gcvOPTION_COUNT];

static gceSTATUS
_FillInOptions(
    IN gcoHAL Hal
    )
{
    gctSTRING envctrl = gcvNULL;

    gcoOS_ZeroMemory(gcOptions, sizeof(gcOptions[0]) * gcvOPTION_COUNT);
    gcOptions[gcvOPTION_PREFER_ZCONVERT_BYPASS] = gcvTRUE;
    gcOptions[gcvOPTION_PREFER_GUARDBAND] = gcvFALSE;
    gcOptions[gcvOPTION_PREFER_TILED_DISPLAY_BUFFER] = gcvFALSE;
    gcOptions[gcvOPTION_PREFER_TPG_TRIVIALMODEL] = gcvFALSE;
    gcOptions[gcvOPTION_PREFER_USC_RECONFIG] = gcvFALSE;
    gcOptions[gcvOPTION_PREFER_DISALBE_HZ] = gcvFALSE;

    gcOptions[gcvOPTION_KERNEL_FENCE] = gcvFALSE;
    gcOptions[gcvOPTION_ASYNC_PIPE] = gcvFALSE;
    gcOptions[gcvOPTION_GPU_TEX_UPLOAD] = gcvTRUE;
    gcOptions[gcvOPTION_GPU_BUFOBJ_UPLOAD] = gcvTRUE;
    gcOptions[gcvOPTION_OCL_ASYNC_BLT] = gcvTRUE;
    gcOptions[gcvOPTION_OCL_IN_THREAD] = gcvTRUE;
    gcOptions[gcvOPTION_COMPRESSION_DEC400] = gcvTRUE;
    gcOptions[gcvOPTION_NO_Y_INVERT] = gcvFALSE;
    gcOptions[gcvOPTION_OCL_VIR_SHADER] = gcvTRUE;
    gcOptions[gcvOPTION_OCL_USE_MULTI_DEVICES] = gcvFALSE;


    envctrl = gcvNULL;
    gcOptions[gcvOPTION_PREFER_RA_DEPTH_WRITE] = gcvTRUE;
    if (gcmIS_SUCCESS(gcoOS_GetEnv(gcvNULL, "VIV_DISABLE_RA_DEPTH_WRITE", &envctrl)) && envctrl)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(envctrl, "1")))
        {
            gcOptions[gcvOPTION_PREFER_RA_DEPTH_WRITE] = gcvFALSE;
        }
    }

    gcOptions[gcvOPTION_FBO_PREFER_MEM] = gcvFALSE;
    if (gcmIS_SUCCESS(gcoOS_GetEnv(gcvNULL, "VIV_FBO_PREFER_MEM", &envctrl)) && envctrl)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(envctrl, "1")))
        {
            gcOptions[gcvOPTION_FBO_PREFER_MEM] = gcvTRUE;
        }
    }

    envctrl = gcvNULL;
    if (gcmIS_SUCCESS(gcoOS_GetEnv(gcvNULL, "VIV_FBO_PREFER_TILED", &envctrl)) && envctrl)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(envctrl, "1")))
        {
            gcOptions[gcvOPTION_PREFER_TILED_DISPLAY_BUFFER] = gcvTRUE;
        }
    }

    envctrl = gcvNULL;
    if (gcmIS_SUCCESS(gcoOS_GetEnv(gcvNULL, "VIV_DISABLE_DEC400", &envctrl)) && envctrl)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(envctrl, "1")))
        {
            gcOptions[gcvOPTION_COMPRESSION_DEC400] = gcvFALSE;
        }
    }

    envctrl = gcvNULL;
    if (gcmIS_SUCCESS(gcoOS_GetEnv(gcvNULL, "VIV_DISABLE_HZ", &envctrl)) && envctrl)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(envctrl, "1")))
        {
            gcOptions[gcvOPTION_PREFER_DISALBE_HZ] = gcvTRUE;
        }
    }

    envctrl = gcvNULL;
    if (gcmIS_SUCCESS(gcoOS_GetEnv(gcvNULL, "VIV_OCL_VIR_SHADER", &envctrl)) && envctrl)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(envctrl, "0")))
        {
            gcOptions[gcvOPTION_OCL_VIR_SHADER] = gcvFALSE;
        }
    }

    envctrl = gcvNULL;
    gcOptions[gcvOPTION_PREFER_RA_DEPTH_WRITE] = gcvTRUE;
    if (gcmIS_SUCCESS(gcoOS_GetEnv(gcvNULL, "VIV_DISABLE_RA_DEPTH_WRITE", &envctrl)) && envctrl)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(envctrl, "1")))
        {
            gcOptions[gcvOPTION_FBO_PREFER_MEM] = gcvFALSE;
        }
    }


    /* if VIV_MGPU_AFFINITY is COMBINED , VIV_OCL_USE_MULTI_DEVICE is ignore
        if VIV_MGPU_AFFINITY is INDEPENENT, single device if VIV_OCL_USE_MULTI_DEVICE is false else get mulit-device .
    */

    envctrl = gcvNULL;
    gcoOS_GetEnv(gcvNULL,"VIV_OCL_USE_MULTI_DEVICE", &envctrl);
    if(envctrl == gcvNULL || envctrl[0] == '0')
    {

        gcOptions[gcvOPTION_OCL_USE_MULTI_DEVICES] = gcvFALSE;

    }
    else  if((gcoOS_StrCmp(envctrl, "1") == gcvSTATUS_OK  )
             || (gcoOS_StrCmp(envctrl, "1:1") == gcvSTATUS_OK)
             || (gcoOS_StrCmp(envctrl, "1:2") == gcvSTATUS_OK)
             || (gcoOS_StrCmp(envctrl, "1:4") == gcvSTATUS_OK)
             )/* mulit-device mode validation  in gcoCL_QueryDeviceCount */
    {
        gcOptions[gcvOPTION_OCL_USE_MULTI_DEVICES] = gcvTRUE;
    }

#if gcdUSE_VX
    envctrl = gcvNULL;
    gcOptions[gcvOPTION_OVX_ENABLE_NN_ZDP3] = gcvTRUE;
    if (gcmIS_SUCCESS(gcoOS_GetEnv(gcvNULL, "VIV_VX_ENABLE_NN_ZDP3", &envctrl)) && envctrl)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(envctrl, "0")))
        {
            gcOptions[gcvOPTION_OVX_ENABLE_NN_ZDP3] = gcvFALSE;
        }
    }

    envctrl = gcvNULL;
    gcOptions[gcvOPTION_OVX_ENABLE_NN_ZDP6] = gcvTRUE;
    if (gcmIS_SUCCESS(gcoOS_GetEnv(gcvNULL, "VIV_VX_ENABLE_NN_ZDP6", &envctrl)) && envctrl)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(envctrl, "0")))
        {
            gcOptions[gcvOPTION_OVX_ENABLE_NN_ZDP6] = gcvFALSE;
        }
    }

    envctrl = gcvNULL;
    gcOptions[gcvOPTION_OVX_ENABLE_NN_STRIDE] = gcvTRUE;
    if (gcmIS_SUCCESS(gcoOS_GetEnv(gcvNULL, "VIV_VX_ENABLE_NN_STRIDE", &envctrl)) && envctrl)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(envctrl, "0")))
        {
            gcOptions[gcvOPTION_OVX_ENABLE_NN_STRIDE] = gcvFALSE;
        }
    }

    if (gcmIS_SUCCESS(gcoOS_GetEnv(gcvNULL, "VIV_NO_Y_INVERT", &envctrl)) && envctrl)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(envctrl, "1")))
        {
            gcOptions[gcvOPTION_NO_Y_INVERT] = gcvTRUE;
        }
    }

    envctrl = gcvNULL;
    gcoOS_GetEnv(gcvNULL,"VIV_OVX_USE_MULTI_DEVICE", &envctrl); /*mulit-device mode validation  in gcoVX_QueryDeviceCount*/
    if (envctrl == gcvNULL || envctrl[0] == '0')
    {
         gcOptions[gcvOPTION_OVX_USE_MULTI_DEVICES] = gcvFALSE;
    }
    else if (envctrl != gcvNULL && envctrl[0] == '1')
    {
         gcOptions[gcvOPTION_OVX_USE_MULTI_DEVICES] = gcvTRUE;
    }

    envctrl = gcvNULL;
    gcOptions[gcvOPTION_OVX_ENABLE_NN_DDR_BURST_SIZE_256B] = gcvTRUE;
    if (gcmIS_SUCCESS(gcoOS_GetEnv(gcvNULL, "VIV_VX_ENABLE_NN_DDR_BURST_SIZE_256B", &envctrl)) && envctrl)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(envctrl, "0")))
        {
            gcOptions[gcvOPTION_OVX_ENABLE_NN_DDR_BURST_SIZE_256B] = gcvFALSE;
        }
    }

#endif

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHAL_GetOption
**
**  Get option boolean result.
**
**  INPUT:
**      gcoHAL Hal
**          Hal oboject
**
**      gceOPTION Option
**          Option to query about
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHAL_GetOption(
     IN gcoHAL Hal,
     IN gceOPTION Option
     )
{
    gceSTATUS status;
    gcmHEADER_ARG("Hal=0x%x Option=%d", Hal, Option);
    gcmASSERT(Option < gcvOPTION_COUNT);

    status = (gcOptions[Option])? gcvSTATUS_TRUE : gcvSTATUS_FALSE;

    gcmFOOTER_NO();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_SetOption
**
**  Set option value.
**
**  INPUT:
**      gcoHAL Hal
**          Hal oboject
**
**      gceOPTION Option
**          Option to set
**
**      gctBOOL Value
**          Value to set
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHAL_SetOption(
     IN gcoHAL Hal,
     IN gceOPTION Option,
     IN gctBOOL Value
     )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hal=0x%x Option=%d Value=%d", Hal, Option, Value);

    if (Option < gcvOPTION_COUNT)
    {
        gcOptions[Option] = Value;
    }
    else
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
    }

    gcmFOOTER_NO();
    return status;
}


/*******************************************************************************
**
**  gcoHAL_GetUserDebugOption
**
**  Get user debug option.
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      Nothing.
*/
gcsUSER_DEBUG_OPTION *
gcoHAL_GetUserDebugOption(
    void
    )
{
    return &gcUserDebugOption;
}

/*******************************************************************************
**
**  gcoHAL_ConstructEx
**
**  Construct a new gcoHAL object.
**
**  INPUT:
**
**      gctPOINTER Context
**          Pointer to a context that can be used by the platform specific
**          functions.
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**  OUTPUT:
**
**      gcoHAL * Hal
**          Pointer to a variable that will hold the gcoHAL object pointer.
*/
gceSTATUS
gcoHAL_ConstructEx(
    IN gctPOINTER Context,
    IN gcoOS Os,
    OUT gcoHAL * Hal
    )
{
    gctBOOL created = gcvFALSE;
    gcoHAL hal = gcPLS.hal;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;
    gcsHAL_INTERFACE iface;
    gctINT32 i;

    gcmHEADER_ARG("Context=0x%x OS=0x%x", Context, Os);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Hal != gcvNULL);

    if (hal == gcvNULL)
    {
        /* Create the gcoHAL object. */
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  gcmSIZEOF(struct _gcoHAL),
                                  &pointer));
        hal     = pointer;
        created = gcvTRUE;

        gcoOS_ZeroMemory(hal, gcmSIZEOF(struct _gcoHAL));

        /* Initialize the object. */
        hal->object.type = gcvOBJ_HAL;

#if defined(_WIN32) && defined(EMULATOR)
        iface.ignoreTLS = gcvFALSE;
#else
        iface.ignoreTLS = gcvTRUE;
#endif

        iface.command = gcvHAL_VERSION;
        gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                       IOCTL_GCHAL_INTERFACE,
                                       &iface, gcmSIZEOF(iface),
                                       &iface, gcmSIZEOF(iface)));

        /* Test if versions match. */
        if ((iface.u.Version.major != gcvVERSION_MAJOR)
        ||  (iface.u.Version.minor != gcvVERSION_MINOR)
        ||  (iface.u.Version.patch != gcvVERSION_PATCH)
        ||  (iface.u.Version.build != gcvVERSION_BUILD)
        )
        {
            gcmPRINT("HAL user version %d.%d.%d build %u",
                     gcvVERSION_MAJOR, gcvVERSION_MINOR,
                     gcvVERSION_PATCH, gcvVERSION_BUILD);
            gcmPRINT("HAL kernel version %d.%d.%d build %u",
                     iface.u.Version.major, iface.u.Version.minor,
                     iface.u.Version.patch, iface.u.Version.build);

            gcmONERROR(gcvSTATUS_VERSION_MISMATCH);
        }

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
    gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                  "HAL user version %d.%d.%d build %u",
                  gcvVERSION_MAJOR, gcvVERSION_MINOR,
                  gcvVERSION_PATCH, gcvVERSION_BUILD);
    gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                  "HAL kernel version %d.%d.%d build %u",
                  iface.u.Version.major, iface.u.Version.minor,
                  iface.u.Version.patch, iface.u.Version.build);
#endif

        /* Query chip info */
        iface.command = gcvHAL_CHIP_INFO;
        gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                       IOCTL_GCHAL_INTERFACE,
                                       &iface, gcmSIZEOF(iface),
                                       &iface, gcmSIZEOF(iface)));

        hal->chipCount = iface.u.ChipInfo.count;

        for (i = 0; i < hal->chipCount; i++)
        {
            hal->chipTypes[i] = iface.u.ChipInfo.types[i];
            hal->chipIDs[i] = iface.u.ChipInfo.ids[i];

            switch (hal->chipTypes[i])
            {
            case gcvHARDWARE_3D:
                hal->is3DAvailable = gcvTRUE;
                break;

            case gcvHARDWARE_2D:
                hal->separated2D = gcvTRUE;
                break;

            case gcvHARDWARE_3D2D:
                hal->is3DAvailable = gcvTRUE;
                hal->hybrid2D = gcvTRUE;
                break;

            default:
                break;
            }
        }

        hal->defaultHwType = hal->separated2D ? gcvHARDWARE_2D
                           : hal->hybrid2D ? gcvHARDWARE_3D2D
                           : hal->is3DAvailable ? gcvHARDWARE_3D
                           : gcvHARDWARE_VG;

        hal->isGpuBenchSmoothTriangle = gcvFALSE;
    }

    /* Get user option. */
    _GetUserDebugOption(hal);

    _FillInOptions(hal);

    /* Return pointer to the gcoHAL object. */
    *Hal = hal;

    /* Success. */
    gcmFOOTER_ARG("*Hal=0x%x", *Hal);
    return gcvSTATUS_OK;

OnError:
    /* Roll back. */
    if ((hal != gcvNULL) && created)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, hal));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_DestroyEx
**
**  Destroy an gcoHAL object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object that needs to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
**
*******************************************************************************/
gceSTATUS
gcoHAL_DestroyEx(
    IN gcoHAL Hal
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hal=0x%x", Hal);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hal, gcvOBJ_HAL);

    /* Free the gcoHAL object. */
    gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, Hal));

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
**  gcoHAL_Construct
**
**  Construct a new gcoHAL object. Empty function only for compatibility.
**
**  INPUT:
**
**      gctPOINTER Context
**          Pointer to a context that can be used by the platform specific
**          functions.
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHAL_Construct(
    IN gctPOINTER Context,
    IN gcoOS Os,
    OUT gcoHAL * Hal
    )
{
    /* Return gcoHAL object for compatibility to prevent any failure in applications. */
    *Hal = gcPLS.hal;

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHAL_Destroy
**
**  Destroy an gcoHAL object. Empty function only for compatibility.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object that needs to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
**
*******************************************************************************/
gceSTATUS
gcoHAL_Destroy(
    IN gcoHAL Hal
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHAL_Call
**
**  Call the kernel HAL layer.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gcsHAL_INTERFACE * Interface
**          Pointer to an gcsHAL_INTERFACE structure that defines the command to
**          be executed by the kernel HAL layer.
**
**  OUTPUT:
**
**      gcsHAL_INTERFACE * Interface
**          Pointer to an gcsHAL_INTERFACE structure that will be filled in by
**          the kernel HAL layer.
*/
gceSTATUS
gcoHAL_Call(
    IN gcoHAL Hal,
    IN OUT gcsHAL_INTERFACE * Interface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Interface=0x%x", Interface);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Interface != gcvNULL);

    Interface->ignoreTLS = gcvFALSE;


    /* Call kernel service. */
    status = gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        Interface, gcmSIZEOF(gcsHAL_INTERFACE),
        Interface, gcmSIZEOF(gcsHAL_INTERFACE)
        );

    if (gcmIS_SUCCESS(status))
    {
        status = Interface->status;
    }

    if (status == gcvSTATUS_OUT_OF_MEMORY)
    {
        {
            /* Commit any command queue to memory. */
            gcmONERROR(gcoHARDWARE_Commit(gcvNULL));

            /* Stall the hardware. */
            gcmONERROR(gcoHARDWARE_Stall(gcvNULL));
        }


        /* Retry kernel call again. */
        status = gcoOS_DeviceControl(
            gcvNULL,
            IOCTL_GCHAL_INTERFACE,
            Interface, gcmSIZEOF(gcsHAL_INTERFACE),
            Interface, gcmSIZEOF(gcsHAL_INTERFACE)
            );

        if (gcmIS_SUCCESS(status))
        {
            status = Interface->status;
        }
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_MapMemory
**
**  Map a range of video memory into the process space.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gctUINT32 PhysName
**          Physical memory name of video memory to map.
**
**      gctSIZE_T NumberOfBytes
**          Number of bytes to map.
**
**  OUTPUT:
**
**      gctPOINTER * Logical
**          Pointer to a variable that will hold the logical address of the
**          mapped video memory.
*/
gceSTATUS
gcoHAL_MapMemory(
    IN gcoHAL Hal,
    IN gctUINT32 PhysName,
    IN gctSIZE_T NumberOfBytes,
    OUT gctPOINTER * Logical
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("PhysName=0x%x NumberOfBytes=%lu",
                  PhysName, NumberOfBytes);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);

    /* Call kernel API to map the memory. */
    iface.command              = gcvHAL_MAP_MEMORY;
    iface.u.MapMemory.physName = PhysName;
    iface.u.MapMemory.bytes    = NumberOfBytes;
    gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

    /* Return logical address. */
    *Logical = gcmUINT64_TO_PTR(iface.u.MapMemory.logical);

    /* Success. */
    gcmFOOTER_ARG("*Logical=0x%x", *Logical);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_UnmapMemory
**
**  Unmap a range of video memory from the process space.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gctUINT32 PhysName
**          Physical memory name of video memory to unmap.
**
**      gctSIZE_T NumberOfBytes
**          Number of bytes to unmap.
**
**      gctPOINTER Logical
**          Logical address of the video memory to unmap.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHAL_UnmapMemory(
    IN gcoHAL Hal,
    IN gctUINT32 PhysName,
    IN gctSIZE_T NumberOfBytes,
    IN gctPOINTER Logical
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("PhysName=0x%x NumberOfBytes=%lu Logical=0x%x",
                  PhysName, NumberOfBytes, Logical);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);

    /* Call kernel API to unmap the memory. */
    iface.command                = gcvHAL_UNMAP_MEMORY;
    iface.u.UnmapMemory.physName = PhysName;
    iface.u.UnmapMemory.bytes    = NumberOfBytes;
    iface.u.UnmapMemory.logical  = gcmPTR_TO_UINT64(Logical);
    status = gcoHAL_Call(gcvNULL, &iface);

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_ScheduleUnmapMemory
**
**  Schedule an unmap of a buffer mapped through its physical address.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gctUINT32 PhysName
**          Physical memory name of video memory to unmap.
**
**      gctSIZE_T NumberOfBytes
**          Number of bytes to unmap.
**
**      gctPOINTER Logical
**          Logical address of the video memory to unmap.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHAL_ScheduleUnmapMemory(
    IN gcoHAL Hal,
    IN gctUINT32 PhysName,
    IN gctSIZE_T NumberOfBytes,
    IN gctPOINTER Logical
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("PhysName=0x%x NumberOfBytes=%lu Logical=0x%x",
                  PhysName, NumberOfBytes, Logical);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(NumberOfBytes > 0);
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);

    /* Schedule an event to unmap the user memory. */
    iface.command                = gcvHAL_UNMAP_MEMORY;
    iface.engine                 = gcvENGINE_RENDER;
    iface.u.UnmapMemory.bytes    = NumberOfBytes;
    iface.u.UnmapMemory.physName = PhysName;
    iface.u.UnmapMemory.logical  = gcmPTR_TO_UINT64(Logical);
    status = gcoHAL_ScheduleEvent(gcvNULL, &iface);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_GetBaseAddr(
    IN  gcoHAL Hal,
    OUT gctUINT32 *BaseAddr
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER();
    gcmVERIFY_ARGUMENT(BaseAddr);

    gcmONERROR(gcoHARDWARE_GetBaseAddr(gcvNULL, BaseAddr));

OnError:
    gcmFOOTER_ARG("BaseAddr=0x%X, status=%d", gcmOPT_VALUE(BaseAddr), status);
    return status;
}

/*******************************************************************************
**
**  gcoHAL_SendFence
**
**  Insert fence command in command buffer
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      Nothing.
*/
#if gcdENABLE_3D
gceSTATUS
gcoHAL_SendFence(
    IN gcoHAL Hal
    )
{
#if gcdSYNC
    gceSTATUS status = gcvSTATUS_OK;
    gctBOOL fenceEnable;

    gcmHEADER();


    if (!gcoHAL_GetOption(gcvNULL, gcvOPTION_KERNEL_FENCE))
    {
        gcoHARDWARE_GetFenceEnabled(gcvNULL, &fenceEnable);

        if (fenceEnable)
        {
            gcmVERIFY_OK(gcoHARDWARE_SendFence(gcvNULL, gcvTRUE, gcvENGINE_RENDER, gcvNULL));
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return status;
#else
    return gcvSTATUS_OK;
#endif
}
#endif /* gcdENABLE_3D */

/*******************************************************************************
**
**  gcoHAL_Commit
**
**  Commit the current command buffer to hardware and optionally wait until the
**  hardware is finished.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gctBOOL Stall
**          gcvTRUE if the thread needs to wait until the hardware has finished
**          executing the committed command buffer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHAL_Commit(
    IN gcoHAL Hal,
    IN gctBOOL Stall
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Stall=%d", Stall);


#if gcdENABLE_3D && gcdSYNC
    {
        gctBOOL fenceEnable;

        gcoHARDWARE_GetFenceEnabled(gcvNULL, &fenceEnable);

        /*only send rlv fence here before commit as hw fence will be send when commit*/
        if (fenceEnable && !gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_FENCE_32BIT)
            && !gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_FENCE_64BIT))
        {
            gcoHARDWARE_SendFence(gcvNULL, gcvTRUE, gcvENGINE_RENDER, gcvNULL);
        }
    }
#endif

    /* Commit the command buffer to hardware. */
    gcmONERROR(gcoHARDWARE_Commit(gcvNULL));

    if (Stall)
    {
        /* Stall the hardware. */
        gcmONERROR(gcoHARDWARE_Stall(gcvNULL));

#if gcdDEC_ENABLE_AHB
        gcmONERROR(gcoDECHARDWARE_FlushDECCompression(gcvNULL, gcvFALSE, gcvTRUE));
#endif
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
**  gcoHAL_GetVGEngine
**
**  Get the pointer to the gcoVG object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gcoVG * Engine
**          Pointer to a variable receiving the gcoVG object pointer.
*/

#if gcdENABLE_3D
/*******************************************************************************
**
**  gcoHAL_Get3DEngine
**
**  Get the pointer to the gco3D object. Don't use it!!!
**  Keep it here for external library (libgcu.so)
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gco3D * Engine
**          Pointer to a variable receiving the gco3D object pointer.
*/
gceSTATUS
gcoHAL_Get3DEngine(
    IN gcoHAL Hal,
    OUT gco3D * Engine
    )
{
    gceSTATUS status;
    gcsTLS_PTR tls;

    gcmHEADER();

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Engine != gcvNULL);

    gcmONERROR(gcoOS_GetTLS(&tls));

    /* Return NULL to the gco3D object. */
    *Engine = gcvNULL;

    /* Success. */
    gcmFOOTER_ARG("*Engine=0x%x", *Engine);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;

}

/*******************************************************************************
**
**  gcoHAL_SetFscaleValue
**
**  Set the fscale value used when GPU is gcvPOWER_ON.
**
**  INPUT:
**
**      gctUINT FscalueValue
**          Fscale value to be set.
**
**  OUTPUT:
**
**          Nothing.
*/
gceSTATUS
gcoHAL_SetFscaleValue(
    IN gctUINT FscaleValue
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("FscaleValue=0x%X", FscaleValue);

    iface.command = gcvHAL_SET_FSCALE_VALUE;
    iface.u.SetFscaleValue.value = FscaleValue;

    status = gcoHAL_Call(gcvNULL, &iface);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_GetFscaleValue
**
**  Get current, minimal and maximal fscale value .
**
**  INPUT:
**
**          Nothing.
**
**  OUTPUT:
**
**          FscaleValue
**              Current fscale value.
**
**          MinFscaleValue
**              Minimal fscale value can be used. Pass gcvNULL if not needed.
**
**          MaxFscaleValue
**              Maximal fscale value can be used. Pass gcvNULL if not needed.
*/
gceSTATUS
gcoHAL_GetFscaleValue(
    OUT gctUINT * FscaleValue,
    OUT gctUINT * MinFscaleValue,
    OUT gctUINT * MaxFscaleValue
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("FscaleValue=0x%X MinFscaleValue=0x%X MaxFscaleValue=0x%X",
                   FscaleValue, MinFscaleValue, MaxFscaleValue);

    gcmVERIFY_ARGUMENT(FscaleValue != gcvNULL);

    iface.command = gcvHAL_GET_FSCALE_VALUE;

    gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

    *FscaleValue =iface.u.GetFscaleValue.value;

    if (MinFscaleValue)
    {
        *MinFscaleValue =iface.u.GetFscaleValue.minValue;
    }

    if (MaxFscaleValue)
    {
        *MaxFscaleValue =iface.u.GetFscaleValue.maxValue;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
OnError:
    gcmFOOTER();
    return status;
}

/* Set PLS enable NP2Texture */
gceSTATUS
gcoHAL_SetBltNP2Texture(
    gctBOOL enable
    )
{
#if gcdUSE_NPOT_PATCH
    gcPLS.bNeedSupportNP2Texture = enable;
#endif

    return gcvSTATUS_OK;
}
#endif /* gcdENABLE_3D */

/*******************************************************************************
**
**  gcoHAL_GetPatchID
**
**  Get the patch ID according to current process name.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gcePATCH_ID * PatchID
**          Pointer to a variable receiving the gcePATCH_ID.
**
*******************************************************************************/
gceSTATUS
gcoHAL_GetPatchID(
    IN  gcoHAL Hal,
    OUT gcePATCH_ID * PatchID
    )
{
#if gcdENABLE_3D
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER();


    status = gcoHARDWARE_GetPatchID(gcvNULL, PatchID);

    gcmFOOTER_ARG("*PatchID=%d", *PatchID);
    return status;
#else
    *PatchID = 0;
    return gcvSTATUS_INVALID_ARGUMENT;
#endif
}

/*******************************************************************************
**
**  gcoHAL_SetPatchID
**
**  Set patch ID
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  INPUT:
**
**      gcePATCH_ID PatchID
**          Patch value to protend.
**
*******************************************************************************/
gceSTATUS
gcoHAL_SetPatchID(
    IN  gcoHAL Hal,
    IN  gcePATCH_ID PatchID
    )
{
#if gcdENABLE_3D
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER();


    status = gcoHARDWARE_SetPatchID(gcvNULL, PatchID);
    gcmFOOTER();
    return status;
#else
    return gcvSTATUS_INVALID_ARGUMENT;
#endif
}


/*******************************************************************************
**
**  gcoHAL_SetGlobalPatchID
**
**  Set global patch ID, mostly time used by vPlayer to pretend to be any app.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  INPUT:
**
**      gcePATCH_ID PatchID
**          Patch value to protend.
**
*******************************************************************************/
gceSTATUS
gcoHAL_SetGlobalPatchID(
    IN  gcoHAL Hal,
    IN  gcePATCH_ID PatchID
    )
{
#if gcdENABLE_3D
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER();

    gcPLS.patchID = PatchID;
    gcmPRINT("!!!Set PatchID=%d", PatchID);

    gcmFOOTER();
    return status;
#else
    return gcvSTATUS_INVALID_ARGUMENT;
#endif
}


gceSTATUS
gcoHAL_GetSpecialHintData(
    IN  gcoHAL Hal,
    OUT gctINT * Hint
    )
{
#if gcdENABLE_3D
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER();


    status = gcoHARDWARE_GetSpecialHintData(gcvNULL, Hint);
    gcmFOOTER_ARG("*Hint=%d", *Hint);
    return status;
#else
    return gcvSTATUS_INVALID_ARGUMENT;
#endif
}

/* Schedule an event. */
gceSTATUS
gcoHAL_ScheduleEvent(
    IN gcoHAL Hal,
    IN OUT gcsHAL_INTERFACE * Interface
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Interface=0x%x", Interface);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Interface != gcvNULL);


    /* Send event to hardware layer. */
    status = gcoHARDWARE_CallEvent(gcvNULL, Interface);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_SetPowerManagementState
**
**  Set GPU to a specified power state.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gceCHIPPOWERSTATE State
**          Power State.
**
*/
gceSTATUS
gcoHAL_SetPowerManagementState(
    IN gcoHAL Hal,
    IN gceCHIPPOWERSTATE State
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("State=%d", State);

    /* Call kernel API to set power management state. */
    iface.command                    = gcvHAL_SET_POWER_MANAGEMENT_STATE;
    iface.u.SetPowerManagement.state = State;
    status = gcoHAL_Call(gcvNULL, &iface);

    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_Compact(
    IN gcoHAL Hal
    )
{
    gceSTATUS status;

    gcmHEADER();

    status = gcoOS_Compact(gcvNULL);

    gcmFOOTER();
    return status;
}

#if VIVANTE_PROFILER_SYSTEM_MEMORY
gceSTATUS
gcoHAL_ProfileStart(
    IN gcoHAL Hal
    )
{
    gceSTATUS status;

    gcmHEADER();

    status = gcoOS_ProfileStart(gcvNULL);

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_ProfileEnd(
    IN gcoHAL Hal,
    IN gctCONST_STRING Title
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Title=%s", Title);

    status = gcoOS_ProfileEnd(gcvNULL, Title);

    gcmFOOTER();
    return status;
}
#else
gceSTATUS
gcoHAL_ProfileStart(
    IN gcoHAL Hal
    )
{
    gcmHEADER();
    gcmFOOTER_NO();
    return gcvSTATUS_INVALID_REQUEST;
}

gceSTATUS
gcoHAL_ProfileEnd(
    IN gcoHAL Hal,
    IN gctCONST_STRING Title
    )
{
    gcmHEADER_ARG("Title=%s", Title);
    gcmFOOTER_NO();
    return gcvSTATUS_INVALID_REQUEST;
}
#endif

/*******************************************************************************
**
**  gcoHAL_SetTimer
**
**  Set Timer to start/stop.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to a gcoHAL object.
**
**      gctUINT32 Timer
**          Which Timer to select for being started or stopped.
**
**      gctBOOL Start
**          Start (gcvTRUE) or Stop (gcvFALSE) request.
**
*/
gceSTATUS
gcoHAL_SetTimer(
    IN gcoHAL Hal,
    IN gctUINT32 Timer,
    IN gctBOOL Start
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("Timer=0x%0x Start=0x%0x", Timer, Start);

    /* Call kernel API to map the memory. */
    iface.command             = gcvHAL_TIMESTAMP;
    iface.engine              = gcvENGINE_RENDER;
    iface.u.TimeStamp.timer   = Timer;
    iface.u.TimeStamp.request = Start;
    gcmONERROR(gcoHAL_ScheduleEvent(gcvNULL, &iface));

    /* Commit exisiting command buffer, to execute any scheduled timer events. */
    gcmONERROR(gcoHAL_Commit(gcvNULL, gcvFALSE));

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
**  gcoHAL_GetTimerTime
**
**  Get Time delta from a Timer in microseconds.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to a gcoHAL object.
**
**      gctUINT32 Timer
**          Which Timer to select.
**
**      gctINT32_PTR TimeDelta
**          Pointer to a variable, which will store the result.
**
*/
gceSTATUS
gcoHAL_GetTimerTime(
    IN gcoHAL Hal,
    IN gctUINT32 Timer,
    OUT gctINT32_PTR TimeDelta
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("Timer=0x%0x", Timer);

    if (TimeDelta == gcvNULL)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Commit exisiting command buffer, to execute any scheduled timer events. */
    gcmONERROR(gcoHAL_Commit(gcvNULL, gcvTRUE));

    /* Call kernel API to map the memory. */
    iface.command             = gcvHAL_TIMESTAMP;
    iface.u.TimeStamp.timer   = Timer;
    iface.u.TimeStamp.request = 2;
    gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

    /* Return time delta value. */
    *TimeDelta = iface.u.TimeStamp.timeDelta;

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
**  gcoHAL_SetTimeOut
**
**  set timeout value.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to a gcoHAL object.
**
**      gctUINT32 timeOut
**          Which timeOut value to set.
**
*/
gceSTATUS
gcoHAL_SetTimeOut(
    IN gcoHAL Hal,
    IN gctUINT32 timeOut
    )
{
#if gcdGPU_TIMEOUT
    gcsHAL_INTERFACE iface;
    gceSTATUS status;
#endif
    gcmHEADER_ARG("Hal=0x%x timeOut=%d", Hal, timeOut);

#if gcdGPU_TIMEOUT
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_SET_TIMEOUT;
    iface.u.SetTimeOut.timeOut = timeOut;
    status = gcoOS_DeviceControl(gcvNULL,
                                 IOCTL_GCHAL_INTERFACE,
                                 &iface, gcmSIZEOF(iface),
                                 &iface, gcmSIZEOF(iface));
    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }
#endif
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoHAL_AddFrameDB(
    void
    )
{
#if gcdFRAME_DB
    gceSTATUS status;
    gcsHAL_INTERFACE ioctl;

    gcmHEADER();

    /* Test if we have outgrown our frame count. */
    if (gcPLS.hal->frameDBIndex >= gcdFRAME_DB)
    {
        gcmONERROR(gcvSTATUS_TOO_COMPLEX);
    }

    /* Get frame information. */
    ioctl.ignoreTLS                = gcvFALSE;
    ioctl.command                  = gcvHAL_GET_FRAME_INFO;
    ioctl.u.GetFrameInfo.frameInfo =
        gcmPTR_TO_UINT64(&gcPLS.hal->frameDB[gcPLS.hal->frameDBIndex]);
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                   IOCTL_GCHAL_INTERFACE,
                                   &ioctl, gcmSIZEOF(ioctl),
                                   &ioctl, gcmSIZEOF(ioctl)));

    /* Increment frame index. */
    gcPLS.hal->frameDBIndex += 1;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
#else
    return gcvSTATUS_NOT_SUPPORTED;
#endif
}

gceSTATUS
gcoHAL_InitGPUProfile(
    void
    )
{
    gcmHEADER();
    gcoHAL_ConfigPowerManagement(gcvFALSE);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoHAL_DumpGPUProfile(
    void
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE ioctl;

    gcmHEADER();

    gcmONERROR(gcoHAL_Commit(gcvNULL, gcvTRUE));

    ioctl.ignoreTLS = gcvFALSE;
    ioctl.command = gcvHAL_DUMP_GPU_PROFILE;
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                   IOCTL_GCHAL_INTERFACE,
                                   &ioctl, gcmSIZEOF(ioctl),
                                   &ioctl, gcmSIZEOF(ioctl)));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

 OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#if gcdFRAME_DB
static gceSTATUS
gcoHAL_PrintFrameDB(
    gctFILE File,
    gctCONST_STRING Format,
    ...
    )
{
    gctCHAR buffer[256];
    gctUINT offset = 0;
    gceSTATUS status;
    gctARGUMENTS arguments;

    gcmHEADER_ARG("File=0x%x Format=0x%x", File, Format);

    /* Format the string. */
    gcmARGUMENTS_START(arguments, Format);
    gcmONERROR(gcoOS_PrintStrVSafe(buffer, gcmSIZEOF(buffer), &offset, Format, arguments));
    gcmARGUMENTS_END(arguments);

    if (File != gcvNULL)
    {
        /* Print to file. */
        gcmONERROR(gcoOS_Write(gcvNULL, File, offset, buffer));
    }
    else
    {
        /* Print to debugger. */
        gcmPRINT("%s", buffer);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif

#if gcdFRAME_DB
#define gcmGET_COUNTER(record, field, usePrev) \
    ( record->field \
    - (usePrev ? record[-1].field : 0) \
    )
#endif

gceSTATUS
gcoHAL_DumpFrameDB(
    gctCONST_STRING Filename OPTIONAL
    )
{
#if gcdFRAME_DB
    gctINT i, j;
    gctFILE file = gcvNULL;
    gcsHAL_FRAME_INFO * record;
    gceSTATUS status;

    gcmHEADER_ARG("Filename=%s", gcmOPT_STRING(Filename));

    if (Filename != gcvNULL)
    {
        /* Open the filename. Silently ignore any errors. */
        gcoOS_Open(gcvNULL, Filename, gcvFILE_CREATETEXT, &file);
    }

    /* Loop through all records. */
    for (i = 0, record = gcPLS.hal->frameDB;
         i < gcPLS.hal->frameDBIndex;
         ++i, ++record)
    {
        const gctBOOL usePrev = (i > 0) && !gcdFRAME_DB_RESET;

        /* Print frame info. */
        gcmONERROR(gcoHAL_PrintFrameDB(
            file,
            "Frame %d: tick=%llu delta=%llu\n",
            i + 1,
            record->ticks,
            (i == 0) ? 0 : record->ticks - record[-1].ticks));

        for (j = 0; (j < 8) && (record->mcCycles[j] > 0); ++j)
        {
            /* Print HW cycles. */
            gcmONERROR(gcoHAL_PrintFrameDB(
                file,
                "  HW[%d]: cycles=%u idle=%u mcCycles=%u\n",
                j,
                record->cycles[j],
                record->idleCycles[j],
                gcmGET_COUNTER(record, mcCycles[j], (i > 0))));
        }

        for (j = 0; (j < 8) && (record->mcCycles[j] > 0); ++j)
        {
            /* Print HW bandwidth. */
            gcmONERROR(gcoHAL_PrintFrameDB(
                file,
                "  BW[%d]: readBytes=%u/%llu writeBytes=%u/%llu\n",
                j,
                record->readRequests[j],
                (gctUINT64) record->readBytes8[j] << 3,
                record->writeRequests[j],
                (gctUINT64) record->writeBytes8[j] << 3));
        }

        /* Print shader HW counters. */
        gcmONERROR(gcoHAL_PrintFrameDB(
            file,
            "  SH: cycles=%u vs=%u/%u ps=%u/%u\n",
            gcmGET_COUNTER(record, shaderCycles, usePrev),
            gcmGET_COUNTER(record, vsInstructionCount, usePrev),
            gcmGET_COUNTER(record, vsTextureCount, usePrev),
            gcmGET_COUNTER(record, psInstructionCount, usePrev),
            gcmGET_COUNTER(record, psTextureCount, usePrev)));

        /* Print texture HW counters. */
        gcmONERROR(gcoHAL_PrintFrameDB(
            file,
            "  TX: bi=%u tri=%u bytes=%llu hit=%u miss=%u\n",
            gcmGET_COUNTER(record, bilinearRequests, usePrev),
            gcmGET_COUNTER(record, trilinearRequests, usePrev),
            (gctUINT64) gcmGET_COUNTER(record, txBytes8, usePrev) << 3,
            gcmGET_COUNTER(record, txHitCount, usePrev),
            gcmGET_COUNTER(record, txMissCount, usePrev)));

        /* Print primitive assembly HW counters. */
        gcmONERROR(gcoHAL_PrintFrameDB(
            file,
            "  PA: vertex=%u primitives=%u/-%u/@%u/#%u/%u\n",
            gcmGET_COUNTER(record, vertexCount, usePrev),
            gcmGET_COUNTER(record, primitiveCount, usePrev),
            gcmGET_COUNTER(record, rejectedPrimitives, usePrev),
            gcmGET_COUNTER(record, culledPrimitives, usePrev),
            gcmGET_COUNTER(record, clippedPrimitives, usePrev),
            gcmGET_COUNTER(record, outPrimitives, usePrev)));

        /* Print rasterizer HW counters. */
        gcmONERROR(gcoHAL_PrintFrameDB(
            file,
            "  RA: primitives=%u quads=-%u/%u/%u pixels=%u\n",
            gcmGET_COUNTER(record, inPrimitives, usePrev),
            gcmGET_COUNTER(record, culledQuadCount, usePrev),
            gcmGET_COUNTER(record, totalQuadCount, usePrev),
            gcmGET_COUNTER(record, quadCount, usePrev),
            gcmGET_COUNTER(record, totalPixelCount, usePrev)));

        for (j = 0; (j < 8) && (record->mcCycles[j] > 0); ++j)
        {
            /* Print per-pipe pixel count. */
            gcmONERROR(gcoHAL_PrintFrameDB(
                file,
                "  PE[%d]: color=-%u/+%u depth=-%u/+%u\n",
                j,
                gcmGET_COUNTER(record, colorKilled[j], usePrev),
                gcmGET_COUNTER(record, colorDrawn[j], usePrev),
                gcmGET_COUNTER(record, depthKilled[j], usePrev),
                gcmGET_COUNTER(record, depthDrawn[j], usePrev)));
        }
    }

    /* Reset frame index. */
    gcPLS.hal->frameDBIndex = 0;

    if (file != gcvNULL)
    {
        /* Close the file. */
        gcmONERROR(gcoOS_Close(gcvNULL, file));
        file = gcvNULL;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    if (file != gcvNULL)
    {
        /* Close the file. */
        gcmONERROR(gcoOS_Close(gcvNULL, file));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
#else
    return gcvSTATUS_NOT_SUPPORTED;
#endif
}

gceSTATUS
gcoHAL_ExportVideoMemory(
    IN gctUINT32 Handle,
    IN gctUINT32 Flags,
    OUT gctINT32 * FD
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    iface.command = gcvHAL_EXPORT_VIDEO_MEMORY;

    iface.u.ExportVideoMemory.node = Handle;
    iface.u.ExportVideoMemory.flags = Flags;

    status = gcoHAL_Call(gcvNULL, &iface);

    *FD = iface.u.ExportVideoMemory.fd;

    return status;
}

gceSTATUS
gcoHAL_NameVideoMemory(
    IN gctUINT32 Handle,
    OUT gctUINT32 * Name
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    iface.command = gcvHAL_NAME_VIDEO_MEMORY;

    iface.u.NameVideoMemory.handle = Handle;

    status = gcoHAL_Call(gcvNULL, &iface);

    *Name = iface.u.NameVideoMemory.name;

    return status;
}

gceSTATUS
gcoHAL_ImportVideoMemory(
    IN gctUINT32 Name,
    OUT gctUINT32 * Handle
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    iface.command = gcvHAL_IMPORT_VIDEO_MEMORY;

    iface.u.ImportVideoMemory.name = Name;

    status = gcoHAL_Call(gcvNULL, &iface);

    *Handle = iface.u.ImportVideoMemory.handle;

    return status;
}

gceSTATUS
gcoHAL_GetVideoMemoryFd(
    IN gctUINT32 Handle,
    OUT gctINT * Fd
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Handle=%d", Handle);

    gcmVERIFY_ARGUMENT(Fd != gcvNULL);

    iface.command = gcvHAL_GET_VIDEO_MEMORY_FD;

    iface.u.GetVideoMemoryFd.handle = Handle;

    status = gcoHAL_Call(gcvNULL, &iface);

    *Fd = iface.u.GetVideoMemoryFd.fd;

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_GetHardware(
    IN gcoHAL Hal,
    OUT gcoHARDWARE* Hw
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcoHARDWARE Hardware = gcvNULL;

    gcmHEADER_ARG("Hal=0x%x Hw=0x%x", Hal, Hw);

    *Hw = gcvNULL;

    gcmGETHARDWARE(Hardware);

OnError:
    *Hw = Hardware;

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_GetProductName(
    IN gcoHAL Hal,
    OUT gctSTRING *ProductName,
    OUT gctUINT *PID
    )
{
    gceSTATUS status= gcvSTATUS_OK;
    gcoHARDWARE hardware = gcvNULL;

    gcmHEADER_ARG("Hal=0x%x ProductName=0x%x", Hal, ProductName);

    {
        gcmGETHARDWARE(hardware);

        status = gcoHARDWARE_GetProductName(hardware, ProductName, PID);
    }

OnError:
    gcmFOOTER_ARG("return productName=%s", gcmOPT_VALUE(ProductName));
    return status;
}


/*******************************************************************************
**
**  gcoHAL_CreateShBuffer
**
**  Create shared buffer.
**  The shared buffer can be used across processes. Other process needs call
**  gcoHAL_MapShBuffer before use it.
**
**  INPUT:
**
**      gctUINT32 Size
**          Specify the shared buffer size.
**
**  OUTPUT:
**
**      gctSHBUF * ShBuf
**          Pointer to hold return shared buffer handle.
*/
gceSTATUS
gcoHAL_CreateShBuffer(
    IN gctUINT32 Size,
    OUT gctSHBUF * ShBuf
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Size=%u", Size);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(ShBuf != gcvNULL);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_SHBUF;
    iface.u.ShBuf.command = gcvSHBUF_CREATE;
    iface.u.ShBuf.bytes   = Size;

    /* Call kernel driver. */
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                   IOCTL_GCHAL_INTERFACE,
                                   &iface, gcmSIZEOF(iface),
                                   &iface, gcmSIZEOF(iface)));

    /* Copy out shared buffer handle. */
    *ShBuf = (gctSHBUF)(gctUINTPTR_T) iface.u.ShBuf.id;

    /* Success. */
    gcmFOOTER_ARG("*ShBuf=%u", (gctUINT32)(gctUINTPTR_T) *ShBuf);
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gcoHAL_DestroyShBuffer
**
**  Destroy(dereference) specified shared buffer.
**  When a shared buffer is created by gcoHAL_CreateShBuffer, or mapped by
**  gcoHAL_MapShBuffer, it must be destroyed to free the resource.
**
**  INPUT:
**
**      gctSHBUF ShBuf
**          Specify the shared buffer to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHAL_DestroyShBuffer(
    IN gctSHBUF ShBuf
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("ShBuf=%u", (gctUINT32)(gctUINTPTR_T) ShBuf);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_SHBUF;
    iface.u.ShBuf.command = gcvSHBUF_DESTROY;
    iface.u.ShBuf.id      = (gctUINT64)(gctUINTPTR_T) ShBuf;

    /* Call kernel driver. */
    status = gcoOS_DeviceControl(gcvNULL,
                                 IOCTL_GCHAL_INTERFACE,
                                 &iface, gcmSIZEOF(iface),
                                 &iface, gcmSIZEOF(iface));

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_MapShBuffer
**
**  Map shared buffer into this process so that it can be used in this process.
**  Call gcoHAL_DestroyShBuffer to destroy the map.
**
**  INPUT:
**
**      gctSHBUF ShBuf
**          Specify the shared buffer to be mapped.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHAL_MapShBuffer(
    IN gctSHBUF ShBuf
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("ShBuf=%u", (gctUINT32)(gctUINTPTR_T) ShBuf);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(ShBuf != gcvNULL);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_SHBUF;
    iface.u.ShBuf.command = gcvSHBUF_MAP;
    iface.u.ShBuf.id      = (gctUINT64)(gctUINTPTR_T) ShBuf;

    /* Call kernel driver. */
    status = gcoOS_DeviceControl(gcvNULL,
                                 IOCTL_GCHAL_INTERFACE,
                                 &iface, gcmSIZEOF(iface),
                                 &iface, gcmSIZEOF(iface));

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_WriteShBuffer
**
**  Write data into shared buffer.
**
**  INPUT:
**
**      gctSHBUF ShBuf
**          Specify the shared buffer to be written to.
**
**      gctPOINTER Data
**          Pointer to hold the source data.
**
**      gctUINT32 ByteCount
**          Specify number of bytes to write. If this is larger than
**          shared buffer size, gcvSTATUS_INVALID_ARGUMENT is returned.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHAL_WriteShBuffer(
    IN gctSHBUF ShBuf,
    IN gctCONST_POINTER Data,
    IN gctUINT32 ByteCount
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("ShBuf=%u Data=0x%08X ByteCount=%u",
                  (gctUINT32)(gctUINTPTR_T) ShBuf, Data, ByteCount);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_SHBUF;
    iface.u.ShBuf.command = gcvSHBUF_WRITE;
    iface.u.ShBuf.id      = (gctUINT64)(gctUINTPTR_T) ShBuf;
    iface.u.ShBuf.data    = (gctUINT64)(gctUINTPTR_T) Data;
    iface.u.ShBuf.bytes   = ByteCount;

    /* Call kernel driver. */
    status = gcoOS_DeviceControl(gcvNULL,
                                 IOCTL_GCHAL_INTERFACE,
                                 &iface, gcmSIZEOF(iface),
                                 &iface, gcmSIZEOF(iface));

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_ReadShBuffer
**
**  Read data from shared buffer.
**
**  INPUT:
**
**      gctSHBUF ShBuf
**          Specify the shared buffer to be read from.
**
**      gctPOINTER Data
**          Pointer to save output data.
**
**      gctUINT32 ByteCount
**          Specify number of bytes to read.
**          If this is larger than shared buffer size, only avaiable bytes are
**          copied. If smaller, copy requested size.
**
**  OUTPUT:
**
**      gctUINT32 * BytesRead
**          Pointer to hold how many bytes actually read from shared buffer.
*/
gceSTATUS
gcoHAL_ReadShBuffer(
    IN gctSHBUF ShBuf,
    IN gctPOINTER Data,
    IN gctUINT32 ByteCount,
    OUT gctUINT32 * BytesRead
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("ShBuf=%u Data=0x%08X ByteCount=%u",
                  (gctUINT32)(gctUINTPTR_T) ShBuf, Data, ByteCount);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_SHBUF;
    iface.u.ShBuf.command = gcvSHBUF_READ;
    iface.u.ShBuf.id      = (gctUINT64)(gctUINTPTR_T) ShBuf;
    iface.u.ShBuf.data    = (gctUINT64)(gctUINTPTR_T) Data;
    iface.u.ShBuf.bytes   = ByteCount;

    /* Call kernel driver. */
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                   IOCTL_GCHAL_INTERFACE,
                                   &iface, gcmSIZEOF(iface),
                                   &iface, gcmSIZEOF(iface)));

    /* Return copied size. */
    *BytesRead = iface.u.ShBuf.bytes;

    /* Success. */
    gcmFOOTER_ARG("*BytesRead=%u", *BytesRead);
    return status;

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_SelectChannel(
    IN gcoHAL Hal,
    IN gctBOOL Priority,
    IN gctUINT32 ChannelId
    )
{
#if gcdENABLE_3D
    return gcoHARDWARE_SelectChannel(gcvNULL, Priority, ChannelId);
#else
    return gcvSTATUS_NOT_SUPPORTED;
#endif
}

gceSTATUS
gcoHAL_MCFESemaphore(
    IN gctUINT32        SemaHandle,
    IN gctBOOL          SendSema
    )
{
#if gcdENABLE_3D
    return gcoHARDWARE_McfeSemapore(gcvNULL, SemaHandle, SendSema, gcvNULL);
#else
    return gcvSTATUS_NOT_SUPPORTED;
#endif
}

gceSTATUS
gcoHAL_AllocateMCFESemaphore(
    OUT gctUINT32 *     SemaHandle
    )
{
#if gcdENABLE_3D
    return gcoHARDWARE_AllocateMcfeSemaphore(gcvNULL, SemaHandle);
#else
    return gcvSTATUS_NOT_SUPPORTED;
#endif
}

gceSTATUS
gcoHAL_FreeMCFESemaphore(
    IN gctUINT32        SemaHandle
    )
{
#if gcdENABLE_3D
    return gcoHARDWARE_FreeMcfeSemaphore(gcvNULL, SemaHandle);
#else
    return gcvSTATUS_NOT_SUPPORTED;
#endif
}


/*******************************************************************************
**
**  gcoHAL_ConfigPowerManagement
**
**  Enable/disable power management. After power management is disabled, there is no auto IDLE/SUSPEND/OFF.
**
**  Input:
**
**      Enable.
**          Enable or disable power management.
**
*/
gceSTATUS
gcoHAL_ConfigPowerManagement(
    IN gctBOOL Enable
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;
    gcmHEADER_ARG("%d", Enable);

    iface.command = gcvHAL_CONFIG_POWER_MANAGEMENT;
    iface.u.ConfigPowerManagement.enable = Enable;

    gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_AllocateVideoMemory(
    IN gctUINT Alignment,
    IN gceVIDMEM_TYPE Type,
    IN gctUINT32 Flag,
    IN gcePOOL Pool,
    IN OUT gctSIZE_T * Bytes,
    OUT gctUINT32_PTR Node
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;
    struct _gcsHAL_ALLOCATE_LINEAR_VIDEO_MEMORY * alvm
        = (struct _gcsHAL_ALLOCATE_LINEAR_VIDEO_MEMORY *) &iface.u;

    gcmHEADER_ARG("Node=%p, Bytes=%u, Alignement=%d, Type=%d, Flag=%d, Pool=%d",
                  Node, *Bytes, Alignment, Type, Flag, Pool);

    iface.command   = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;

    alvm->bytes = *Bytes;

    alvm->alignment = Alignment;
    alvm->type      = (gctUINT32)Type;
    alvm->pool      = Pool;
    alvm->flag      = Flag;

    gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

    *Node  = alvm->node;
    *Bytes = (gctSIZE_T)alvm->bytes;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_LockVideoMemory(
    IN gctUINT32 Node,
    IN gctBOOL Cacheable,
    IN gceENGINE engine,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Logical
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Node=%d, Cacheable=%d", Node, Cacheable);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Node != 0);

    if (engine == gcvENGINE_RENDER)
    {
        /* Fill in the kernel call structure. */
        iface.engine = gcvENGINE_RENDER;
        iface.command = gcvHAL_LOCK_VIDEO_MEMORY;
        iface.u.LockVideoMemory.node = Node;
        iface.u.LockVideoMemory.cacheable = Cacheable;

        /* Call the kernel. */
        gcmONERROR(gcoHAL_Call(gcvNULL, &iface));
    }
    else if (engine == gcvENGINE_BLT)
    {
        /* Fill in the kernel call structure. */
        iface.engine = gcvENGINE_BLT;
        iface.command = gcvHAL_LOCK_VIDEO_MEMORY;
        iface.u.LockVideoMemory.node = Node;
        iface.u.LockVideoMemory.cacheable = Cacheable;

        /* Call the kernel. */
        gcmONERROR(gcoHAL_Call(gcvNULL, &iface));
    }
    else
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (Address)
    {
        /* Return physical address. */
        *Address = iface.u.LockVideoMemory.address;
    }

    if (Logical)
    {
        /* Return logical address. */
        *Logical = gcmUINT64_TO_PTR(iface.u.LockVideoMemory.memory);
    }

    /* Success. */
    gcmFOOTER_ARG("*Address=0x%x *Logical=0x%x", gcmOPT_VALUE(Address), gcmOPT_VALUE(Logical));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_UnlockVideoMemory(
    IN gctUINT32 Node,
    IN gceVIDMEM_TYPE Type,
    IN gceENGINE engine
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Node=0x%x", Node);

    if (engine == gcvENGINE_RENDER)
    {
        iface.engine = gcvENGINE_RENDER;
        iface.command = gcvHAL_UNLOCK_VIDEO_MEMORY;
        iface.u.UnlockVideoMemory.node = Node;
        iface.u.UnlockVideoMemory.type = (gctUINT32)Type;

        gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

        /* Schedule an event for the unlock. Always goto render pipe for now */
        gcmONERROR(gcoHARDWARE_CallEvent(gcvNULL, &iface));
    }
    else if (engine == gcvENGINE_BLT)
    {
        if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_ASYNC_BLIT) != gcvSTATUS_TRUE)
        {
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

        iface.engine = gcvENGINE_BLT;
        iface.command = gcvHAL_UNLOCK_VIDEO_MEMORY;
        iface.u.UnlockVideoMemory.node = Node;
        iface.u.UnlockVideoMemory.type = (gctUINT32)Type;

        gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

        /* Schedule an event for the unlock. Always goto render pipe for now */
        gcmONERROR(gcoHARDWARE_CallEvent(gcvNULL, &iface));
    }
    else
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
    }

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_ReleaseVideoMemory(
    IN gctUINT32 Node
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Node=0x%x", Node);

    /* Release the allocated video memory synchronously. */
    iface.command = gcvHAL_RELEASE_VIDEO_MEMORY;
    iface.u.ReleaseVideoMemory.node = Node;

    /* Call kernel HAL. */
    gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_WrapUserMemory
**
**  Wrap a memory from other allocator to a vidmem node.
**
**  INPUT:
**
**      gctUINT32 Handle
**          Handle of user memory.
**
**      gctUINT32 Flag
**          Allocation flag of user memory.
**
**  OUTPUT:
**
**      gctUINT32_PTR Node
**          Pointer to a variable that will receive the wrapped vidmem node.
*/
gceSTATUS
gcoHAL_WrapUserMemory(
    IN gcsUSER_MEMORY_DESC_PTR UserMemoryDesc,
    IN gceVIDMEM_TYPE Type,
    OUT gctUINT32_PTR Node
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;
#if defined(__QNXNTO__)
    gctPOINTER lock_addr = gcmINT2PTR(gcmPTR2SIZE(UserMemoryDesc->logical) & ~(__PAGESIZE - 1));    /* Get the address aligned to pagesize */
    size_t lock_size = (gcmPTR2SIZE(UserMemoryDesc->logical) & (__PAGESIZE - 1)) + UserMemoryDesc->size;        /* Get the size to lock (from the address + size */
#endif

    gcmHEADER_ARG("UserMemoryDesc=%d", UserMemoryDesc);

    iface.command = gcvHAL_WRAP_USER_MEMORY;
    iface.u.WrapUserMemory.type = (gctUINT32)Type;

#if defined(__QNXNTO__)
    mlock(lock_addr, lock_size);
#endif

    gcoOS_MemCopy(&iface.u.WrapUserMemory.desc, UserMemoryDesc, gcmSIZEOF(gcsUSER_MEMORY_DESC));

    /* Call kernel driver. */
    gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

    /* Get wrapped node. */
    *Node = iface.u.WrapUserMemory.node;

    gcmFOOTER_ARG("*Node=%d", *Node);
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_QueryResetTimeStamp(
    OUT gctUINT64_PTR ResetTimeStamp,
    OUT gctUINT64_PTR ContextID
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface;

    gcmHEADER();

    iface.u.QueryResetTimeStamp.timeStamp = 0;

    iface.command = gcvHAL_QUERY_RESET_TIME_STAMP;

    /* Call kernel driver. */
    gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

    *ResetTimeStamp = iface.u.QueryResetTimeStamp.timeStamp;

    if (ContextID)
    {
        *ContextID = iface.u.QueryResetTimeStamp.contextID;
    }

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_WaitFence(
    IN gctUINT32 Handle,
    IN gctUINT32 TimeOut
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface;

    gcmHEADER();

    iface.u.WaitFence.handle  = Handle;
    iface.u.WaitFence.timeOut = TimeOut;

    iface.command = gcvHAL_WAIT_FENCE;

    /* Call kernel driver. */
    gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_LockVideoNode(
    IN gctUINT32 Handle,
    IN gctBOOL Cacheable,
    OUT gctUINT32 *Address,
    OUT gctPOINTER *Memory
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface;

    gcmHEADER();

    gcmASSERT(Address != gcvNULL);
    gcmASSERT(Memory != gcvNULL);
    gcmASSERT(Handle != 0);

    iface.command = gcvHAL_LOCK_VIDEO_MEMORY;
    iface.u.LockVideoMemory.node = Handle;
    iface.u.LockVideoMemory.cacheable = Cacheable;

    /* Call kernel driver. */
    gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

    *Address = iface.u.LockVideoMemory.address;
    *Memory = gcmUINT64_TO_PTR(iface.u.LockVideoMemory.memory);

OnError:
    gcmFOOTER();
    return status;
}

#if gcdENABLE_3D
gceSTATUS
gcoHAL_SetCompilerFuncTable(
    IN gcoHAL Hal,
    IN gcsVSC_APIS *VscAPIs
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hal=0x%x VscAPIs=0x%x", Hal, VscAPIs);

    status = gcoHARDWARE_SetCompilerFuncTable(gcvNULL, VscAPIs);

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_FreeTXDescArray(
    gcsTXDescNode *DescArray,
    gctINT CurIndex
    )
{
    gctINT j;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("DescArray=0x%x, CurIndex=%d", DescArray, CurIndex);

    for (j = 0; j <= CurIndex; j++)
    {
        gctINT i;
        /* Free descriptor node */
        for (i = 0; i < 2; i++)
        {
            if (DescArray[j].descLocked[i] != gcvNULL)
            {
                /* Unlock texture descriptor. */
                gcmONERROR(gcoSURF_UnLockNode(DescArray[j].descNode[i], gcvSURF_TXDESC));
                DescArray[j].descLocked[i] = gcvNULL;
            }
            if (DescArray[j].descNode[i])
            {
                gcmONERROR(gcsSURF_NODE_Destroy(DescArray[j].descNode[i]));
                gcmOS_SAFE_FREE(gcvNULL, DescArray[j].descNode[i]);
                DescArray[j].descNode[i] = gcvNULL;
            }
        }
    }
OnError:
    gcmFOOTER();
    return status;
}



#endif


gceSTATUS
gcoHAL_ScheduleSignal(
    IN gctSIGNAL Signal,
    IN gctSIGNAL AuxSignal,
    IN gctINT ProcessID,
    IN gceKERNEL_WHERE FromWhere
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    iface.command             = gcvHAL_SIGNAL;
    iface.engine              = gcvENGINE_RENDER;
    iface.u.Signal.signal     = gcmPTR_TO_UINT64(Signal);
    iface.u.Signal.auxSignal  = gcmPTR_TO_UINT64(AuxSignal);
    iface.u.Signal.process    = ProcessID;
    iface.u.Signal.fromWhere  = FromWhere;

    gcmONERROR(gcoHAL_ScheduleEvent(gcvNULL, &iface));

    return gcvSTATUS_OK;

OnError:
    return status;
}

gceSTATUS
gcoHAL_GetGraphicBufferFd(
    IN gctUINT32 Node[3],
    IN gctSHBUF ShBuf,
    IN gctSIGNAL Signal,
    OUT gctINT32 * Fd
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    iface.command                      = gcvHAL_GET_GRAPHIC_BUFFER_FD;
    iface.engine                       = gcvENGINE_RENDER;
    iface.ignoreTLS                    = gcvFALSE;
    iface.u.GetGraphicBufferFd.node[0] = Node[0];
    iface.u.GetGraphicBufferFd.node[1] = Node[1];
    iface.u.GetGraphicBufferFd.node[2] = Node[2];
    iface.u.GetGraphicBufferFd.shBuf   = gcmPTR_TO_UINT64(ShBuf);
    iface.u.GetGraphicBufferFd.signal  = gcmPTR_TO_UINT64(Signal);

    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                   IOCTL_GCHAL_INTERFACE,
                                   &iface, gcmSIZEOF(iface),
                                   &iface, gcmSIZEOF(iface)));

    *Fd = iface.u.GetGraphicBufferFd.fd;
    return gcvSTATUS_OK;

OnError:
    return status;
}

gceSTATUS
gcoHAL_AlignToTile(
    IN OUT gctUINT32 * Width,
    IN OUT gctUINT32 * Height,
    IN  gceSURF_TYPE Type,
    IN  gceSURF_FORMAT Format
    )
{
    gceSTATUS status;

    status = gcoHARDWARE_AlignToTileCompatible(
        gcvNULL,
        Type & 0xFF,
        Type & ~0xFF,
        Format,
        Width,
        Height,
        1,
        gcvNULL,
        gcvNULL,
        gcvNULL
        );

    return status;
}

/*******************************************************************************
 **
 ** gcoHAL_GetPLS
 **
 ** Get access to the process local storage.
 **
 ** INPUT:
 **
 **     Nothing.
 **
 ** OUTPUT:
 **
 **     gcsPLS_PTR * PLS
 **         Pointer to a variable that will hold the pointer to the PLS.
 */
extern gcsPLS gcPLS;
gceSTATUS
gcoHAL_GetPLS(
    OUT gcsPLS_PTR * PLS
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("PLS=%p", PLS);

    if (!gcPLS.processID)
    {
        gcmPRINT("PLS isn't existed");
        status = gcvSTATUS_TRUE;
        gcmONERROR(status);
    }

    *PLS = &gcPLS;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    *PLS = gcvNULL;

    gcmFOOTER();
    return status;
}
