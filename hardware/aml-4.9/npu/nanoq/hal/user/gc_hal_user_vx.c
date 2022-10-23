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
#if gcdUSE_VX && gcdENABLE_3D
#include "gc_hal_vx.h"

#define _GC_OBJ_ZONE            gcdZONE_VX

static gctUINT32 memory_size = 0;
static gctUINT64 free_memory_size = 0;

/******************************************************************************\
|********************************* Structures *********************************|
\******************************************************************************/

/******************************************************************************\
|*********************************** Macros ***********************************|
\******************************************************************************/

/******************************************************************************\
|******************************* API Code *******************************|
\******************************************************************************/

static gctBOOL Is_OneDimKernel(gctUINT32 kernel) {
    return (kernel == gcvVX_KERNEL_OPTICAL_FLOW_PYR_LK);
}

static gctBOOL Is_HistogramKernel(gctUINT32 kernel) {
    return ((kernel == gcvVX_KERNEL_HISTOGRAM) || (kernel == gcvVX_KERNEL_EQUALIZE_HISTOGRAM));
}

typedef struct _gcoVX
{
    gcoHARDWARE hardwares[gcdMAX_3DGPU_COUNT];
    gctUINT32   deviceCount;
} _gcoVX;

static gceSTATUS
_QueryDeviceCoreCount(
    OUT gctUINT32 * DeviceCount,
    OUT gctUINT32 * GPUCountPerDevice
    )
{
    gctUINT chipIDs[32];
    gceMULTI_GPU_MODE mode;
    gctUINT coreIndex;
    gctSTRING attr;
    gctUINT32 i, gpuCount;
    static gctUINT deviceCount = 1;
    static gctUINT gpuCountPerDevice[MAX_GPU_CORE_COUNT] = {1};
    static gctBOOL queriedOnce = gcvFALSE;

    if (queriedOnce)
    {
        goto exit;
    }

    gcoHAL_QueryCoreCount(gcvNULL, gcvHARDWARE_3D, &gpuCount, chipIDs);

    if (gpuCount == 0)
    {
        gcoHAL_QueryCoreCount(gcvNULL, gcvHARDWARE_3D2D, &gpuCount, chipIDs);
    }

    gcoHAL_QueryMultiGPUAffinityConfig(gcvHARDWARE_3D, &mode, &coreIndex);

    if (mode == gcvMULTI_GPU_MODE_COMBINED)      /*Combined Mode*/
    {
        if(gcoHAL_GetOption(gcvNULL, gcvOPTION_OVX_USE_MULTI_DEVICES))
        {
            gcmPRINT("VIV Warning : VIV_OVX_USE_MULIT_DEVICES=1 only for INDEPENDENT mode, back to combinedMode");
            return gcvSTATUS_INVALID_ARGUMENT;
        }

        deviceCount = 1;
        gpuCountPerDevice[0] = gpuCount;
    }
    else     /*For Dependent mode*/
    {
        if (gcoHAL_GetOption(gcvNULL, gcvOPTION_OVX_USE_MULTI_DEVICES))  /*multi-device mode */
        {
            gcoOS_GetEnv(gcvNULL, "VIV_OVX_USE_MULTI_DEVICE", &attr);

            gcmASSERT(attr && attr[0] == '1');

            if (attr[1] == '\0' || (attr[1] == ':' && (attr[2] == '1' || attr[2] == '2' || attr[2] == '4' || attr[2] == '1')))
            {
                if (attr[1] == '\0' || attr[1] == '0')
                {
                    gpuCountPerDevice[0] = 1;
                }
                else
                {
                    gpuCountPerDevice[0] = attr[2] - '0';
                }

                if ((gpuCount % gpuCountPerDevice[0] != 0) || (gpuCountPerDevice[0] > gpuCount))
                {
                    gcmPRINT("VIV Warning: Invalid VIV_OVX_USE_MULIT_DEVICES Env vars GPUCountPerDevivce is invalid");
                    return gcvSTATUS_INVALID_ARGUMENT;
                }

                deviceCount = gpuCount / gpuCountPerDevice[0];
                for (i = 1; i < deviceCount; i++)
                {
                    gpuCountPerDevice[i] = gpuCountPerDevice[0];
                }
            }
            else if (attr[1] == ':' && attr[2] == '[' && attr[3] != '\0')
            {
                char *attrp;
                gctUINT32 count = 0;
#ifdef _WIN32
                char *np;
                attrp = strtok_s(&attr[3], ",", &np);
#else
                attrp = strtok(&attr[3], ",");
#endif
                if (attrp == gcvNULL || ((count = attrp[0] - '0') != 1 && count != 2 && count != 4))
                {
                    gcmPRINT("VIV Warning : VIV_OVX_USE_MULTI_DEVICES setting 1:[1 or 2 or 4,...]");
                }

                gpuCountPerDevice[0] = count;

                do
                {
#ifdef _WIN32
                    attrp= strtok_s(NULL, ",", &np);
#else
                    attrp= strtok(NULL, ",");
#endif
                    if (attrp != NULL)
                    {
                        if ((count = attrp[0] - '0') != 1 && count != 2 && count != 4)
                        {
                            gcmPRINT("VIV Warning : VIV_OVX_USE_MULTI_DEVICES setting 1:[1 or 2 or 4,...]");
                        }
                        else
                        {
                            gpuCountPerDevice[deviceCount++] = count;
                        }
                    }
                }
                while (attrp != NULL);

                count = 0;
                for (i = 0; i < deviceCount; i++)
                {
                    count += gpuCountPerDevice[i];
                }

                if (count != gpuCount)
                {
                    gcmPRINT("VIV Warning: Invalid VIV_OVX_USE_MULIT_DEVICES Env: GPUCountPerDevivce sum doesn't equal to gpu core count");
                    return gcvSTATUS_INVALID_ARGUMENT;
                }
            }
            else
            {
                gcmPRINT("VIV Warning : VIV_OVX_USE_MULTI_DEVICES only supports 1 | 1:1 | 1:2 | 1:4 | 1:[1or2or4,]");
            }
        }
        else    /*multi-device is disable, Independent mode , one device and device has only one gpucore */
        {
            deviceCount = 1;
            gpuCountPerDevice[0] = 1;

            if (coreIndex >= gpuCount) /*coreIndex need small than maxCoreCount*/
            {
                return gcvSTATUS_INVALID_ARGUMENT;
            }
        }
    }

    queriedOnce = gcvTRUE;

exit:
    if (DeviceCount != gcvNULL)
    {
        *DeviceCount = deviceCount;
    }
    if (GPUCountPerDevice != gcvNULL)
    {
        gcoOS_MemCopy((gctPOINTER)GPUCountPerDevice, (gctPOINTER)gpuCountPerDevice, sizeof(gpuCountPerDevice));
    }

    return gcvSTATUS_OK;
}

gctBOOL gcoVX_VerifyHardware();

gceSTATUS gcoVX_CreateHW();

gceSTATUS gcoVX_Construct(OUT gcoVX * VXObj )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT i, deviceCount, perDeviceGpuCount[MAX_GPU_CORE_COUNT], startIndex = 0;
    gctUINT32 gpuCoreIndexs[] = {0, 1, 2, 3, 4, 5, 6, 7};
    gctPOINTER pointer = gcvNULL;
    gcoVX vxObj = gcvNULL;

    gcmHEADER();

    gcmONERROR(
        gcoOS_Allocate(gcvNULL,
                       gcmSIZEOF(struct _gcoVX),
                       &pointer));

    vxObj = (gcoVX) pointer;

    gcoOS_ZeroMemory(vxObj, gcmSIZEOF(struct _gcoVX));
    gcmONERROR(_QueryDeviceCoreCount(&deviceCount, perDeviceGpuCount));
    gcmONERROR(gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_3D));

    if (deviceCount == 1 && perDeviceGpuCount[0] == 1) /*Special deal with independent mode*/
    {
        gctUINT32 mainCoreIndex;
        gceMULTI_GPU_MODE mode;

        gcoHAL_QueryMultiGPUAffinityConfig(gcvHARDWARE_3D, &mode, &mainCoreIndex);
        gpuCoreIndexs[0] = mainCoreIndex;
    }

    for (i = 0; i < deviceCount; i++)
    {
        gcmONERROR(gcoVX_CreateHW(i, perDeviceGpuCount[i], &gpuCoreIndexs[startIndex], &vxObj->hardwares[i]));
        startIndex += perDeviceGpuCount[i];
    }

    vxObj->deviceCount = deviceCount;

    *VXObj = vxObj;

     gcmFOOTER();
    return gcvSTATUS_OK;
OnError:

     if(vxObj)
     {

         for(i = 0; i < vxObj->deviceCount; i++)
         {
             if(vxObj->hardwares[i])
             {
                 gcoHARDWARE_Destroy(vxObj->hardwares[i], gcvFALSE);
             }
         }

         gcoOS_Free(gcvNULL, vxObj);
     }

     gcmFOOTER();
     return status;
}

gceSTATUS gcoVX_Destroy(IN gcoVX VXObj)
{
    gctUINT32 i;

    gcmASSERT(VXObj != gcvNULL);

    for(i = 0; i < VXObj->deviceCount; i++)
    {
        if(VXObj->hardwares[i])
        {
            gcoHARDWARE_Destroy(VXObj->hardwares[i], gcvFALSE);
        }
    }

    gcoOS_Free(gcvNULL, VXObj);

    return gcvSTATUS_OK;
}

gceSTATUS
gcoVX_SetFeatueCap(vx_evis_no_inst_s *evisNoInst)
{
    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_EVIS_NO_ABSDIFF))
    {
        evisNoInst->noAbsDiff = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_EVIS_NO_BITREPLACE))
    {
        evisNoInst->noBitReplace = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_EVIS_NO_CORDIAC))
    {
        evisNoInst->noMagPhase = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_EVIS_NO_DP32))
    {
        evisNoInst->noDp32 = gcvTRUE;
        evisNoInst->clamp8Output = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_EVIS_NO_FILTER))
    {
        evisNoInst->noFilter = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_EVIS_NO_BOXFILTER))
    {
        evisNoInst->noBoxFilter = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_EVIS_NO_IADD))
    {
        evisNoInst->noIAdd = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_EVIS_NO_SELECTADD))
    {
        evisNoInst->noSelectAdd = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_EVIS_LERP_7OUTPUT))
    {
        evisNoInst->lerp7Output  = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_EVIS_ACCSQ_8OUTPUT))
    {
        evisNoInst->accsq8Output = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_EVIS_VX2))
    {
        evisNoInst->isVX2 = gcvTRUE;
    }

    evisNoInst->isSet = gcvTRUE;

    return gcvSTATUS_OK;
}

gceSTATUS
gcoVX_Initialize(vx_evis_no_inst_s *evisNoInst)
{
    gceSTATUS status;
    gcsTLS_PTR __tls__;

    gcmHEADER();

    gcmONERROR(gcoOS_GetTLS(&__tls__));
    if(__tls__->engineVX == gcvNULL)
    {
        gcmONERROR(gcoVX_Construct(&__tls__->engineVX));
    }

    gcmONERROR(gcoVX_GetEvisNoInstFeatureCap(evisNoInst));
    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
gceSTATUS
gcoVX_Replay(
    IN gctPOINTER CmdBuffer,
    IN gctUINT32  CmdBytes
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("CmdBuffer=0x%x CmdBytes=%d", CmdBuffer, CmdBytes);

    gcmASSERT(gcoVX_VerifyHardware());
    gcmONERROR(gcoHARDWAREVX_ReplayCmd(gcvNULL, CmdBuffer, CmdBytes));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*
** For OCX Mulit-Device  we need flush more GPU cache for every GPU 's commit .
** The resource(SH cache) may be used in GPU1 but be released on GPU0 ,the event only trigger to
** flush GPU0's internel cache but the cache is still in  GPU1.
** So we flush all cache in user command in every OCL's commit for safe.
**
*/

gceSTATUS gcoVX_MultiDevcieCacheFlush()
{
    return gcoHARDWARE_MultiGPUCacheFlush(gcvNULL, gcvNULL);
}

gceSTATUS
gcoVX_Commit(
    IN gctBOOL Flush,
    IN gctBOOL Stall,
    INOUT gctPOINTER *pCmdBuffer,
    INOUT gctUINT32  *pCmdBytes
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Flush=%d Stall=%d", Flush, Stall);

    gcmASSERT(gcoVX_VerifyHardware());

    if (Flush)
    {
        gcmONERROR(gcoHARDWARE_FlushPipe(gcvNULL, gcvNULL));
    }

    if(gcoHAL_GetOption(gcvNULL, gcvOPTION_OVX_USE_MULTI_DEVICES))
    {
        gcmONERROR(gcoVX_MultiDevcieCacheFlush());
    }

    /* Commit the command buffer to hardware. */
    gcmONERROR(gcoHARDWAREVX_CommitCmd(gcvNULL, pCmdBuffer, pCmdBytes));

    if (Stall)
    {
        /* Stall the hardware. */
        gcmONERROR(gcoHARDWARE_Stall(gcvNULL));
    }

    /* Success. */
    status = gcvSTATUS_OK;

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_BindImage(
    IN gctUINT32            Index,
    IN gcsVX_IMAGE_INFO_PTR Info
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=%d, Info=%p", Index, Info);

    gcmASSERT(gcoVX_VerifyHardware());
    gcmONERROR(gcoHARDWAREVX_BindImage(gcvNULL, Index, Info));

    /* Success. */
    status = gcvSTATUS_OK;

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
    gcoVX_SetImageInfo(
    IN  gcUNIFORM Uniform,
    IN gcsVX_IMAGE_INFO_PTR Info
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Uniform=%p, Info=%p", Uniform, Info);

    gcmASSERT(gcoVX_VerifyHardware());

    gcmONERROR(gcoHARDWAREVX_SetImageInfo(gcvNULL,Uniform->address,
                                   GetUniformPhysical(Uniform), Info));

    /* Success. */
    status = gcvSTATUS_OK;
OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_BindUniform(
    IN gctUINT32            RegAddress,
    IN gctUINT32            Index,
    IN gctUINT32            *Value,
    IN gctUINT32            Num
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=%d, Value=%p", Index, Value);

    /* GCREG_GPIPE_UNIFORMS_Address : 0x34000
     * GCREG_SH_UNIFORMS_Address    : 0x30000
     * GCREG_PIXEL_UNIFORMS_Address : 0x36000
     */
    gcmASSERT(gcoVX_VerifyHardware());
    gcmONERROR(gcoHARDWAREVX_BindUniform(gcvNULL, RegAddress, Index, Value, Num));

    /* Success. */
    status = gcvSTATUS_OK;

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_InvokeThreadWalker(
    IN gcsTHREAD_WALKER_INFO_PTR Info
    )
{
    gceSTATUS status;
    gceAPI currentApi;

    gcmHEADER_ARG("Info=0x%x", Info);

    /* Get Current API. */
    gcmONERROR(gcoHARDWARE_GetAPI(gcvNULL, &currentApi, gcvNULL));

    if (currentApi != gcvAPI_OPENCL)
    {
        /* Set HAL API to OpenCL. */
        gcmONERROR(gcoHARDWARE_SetAPI(gcvNULL, gcvAPI_OPENCL));
    }

    /* add env to skip shader execution */
    {
        gctSTRING envctrl = gcvNULL;
        gctBOOL bSkipShader = gcvFALSE;
        if (gcmIS_SUCCESS(gcoOS_GetEnv(gcvNULL, "VIV_VX_SKIP_SHADER", &envctrl)) && envctrl
            && gcmIS_SUCCESS(gcoOS_StrCmp(envctrl, "1")))
        {
            bSkipShader = gcvTRUE;
        }

        if (!bSkipShader)
        {
            /* Route to hardware. */
            gcmONERROR(gcoHARDWARE_InvokeThreadWalkerCL(gcvNULL, Info));
        }
    }


    if (currentApi != gcvAPI_OPENCL && currentApi != 0 )
    {
        /* Restore HAL API. */
        gcmONERROR(gcoHARDWARE_SetAPI(gcvNULL, currentApi));
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_InvokeKernel(
    IN gcsVX_KERNEL_PARAMETERS_PTR  Parameters
    )
{
    gcsVX_THREAD_WALKER_PARAMETERS  twParameters;
    gceSTATUS                       status = gcvSTATUS_OK;
    gctUINT32                       xCount;
    gctUINT32                       yCount;
    gctUINT32                       groupSizeX;
    gctUINT32                       groupSizeY;
    gctUINT32                       groupCountX;
    gctUINT32                       groupCountY;
    gctUINT32                       maxShaderGroups;
    gctUINT32                       maxGroupSize;
    gctUINT32                       shaderGroupSize;
    gctUINT                         maxComputeUnits;
    gcsTHREAD_WALKER_INFO           info;

    gcmHEADER_ARG("Parameters=%p", Parameters);

    /* Calculate thread allocation. */
#if gcdVX_OPTIMIZER > 2
    if (Parameters->tiled)
    {
        if (Parameters->xoffset + Parameters->xcount <= Parameters->xmax)
        {
            xCount = gcmALIGN_NP2(Parameters->xcount, Parameters->xstep) / Parameters->xstep;
            yCount = gcmALIGN_NP2(Parameters->ycount, Parameters->ystep) / Parameters->ystep;
        }
        else
        {
            xCount = gcmALIGN_NP2(Parameters->xmax - Parameters->xoffset, Parameters->xstep) / Parameters->xstep;
            yCount = gcmALIGN_NP2(Parameters->ymax - Parameters->yoffset, Parameters->ystep) / Parameters->ystep;
        }
    }
    else
#endif
    {
        xCount = gcmALIGN_NP2(Parameters->xmax - Parameters->xmin, Parameters->xstep) / Parameters->xstep;
        yCount = gcmALIGN_NP2(Parameters->ymax - Parameters->ymin, Parameters->ystep) / Parameters->ystep;
    }

    /* Number of shader cores */
    gcmONERROR(
        gcoHARDWARE_QueryShaderCaps(gcvNULL,
                                    gcvNULL,
                                    gcvNULL,
                                    gcvNULL,
                                    gcvNULL,
                                    &maxComputeUnits,
                                    gcvNULL,
                                    gcvNULL,
                                    gcvNULL));

    shaderGroupSize = 4 * maxComputeUnits;

    /*  Compute work group size in each direction. Use 109 rather than 128. Some temp regs may be reserved if gpipe is running. */
#if gcdVX_OPTIMIZER > 1
    gcmASSERT(Parameters->instructions->regs_count);
    maxShaderGroups = 109/Parameters->instructions->regs_count;
#else
    gcmASSERT(Parameters->instructions.regs_count);
    maxShaderGroups = 109/Parameters->instructions.regs_count;
#endif
    maxGroupSize = maxShaderGroups * shaderGroupSize;

    /* Default groupSize */
    if (Parameters->groupSizeX == 0)
    {
        Parameters->groupSizeX = shaderGroupSize;
    }

    if (Parameters->groupSizeY == 0)
    {
        Parameters->groupSizeY = shaderGroupSize;
    }

    groupSizeX = (xCount >= Parameters->groupSizeX) ? Parameters->groupSizeX : xCount;
    groupSizeY = (yCount >= Parameters->groupSizeY) ? Parameters->groupSizeY : yCount;

    /* Compute number of groups in each direction. */
    groupCountX = (xCount + groupSizeX - 1) / groupSizeX;
    groupCountY = (yCount + groupSizeY - 1) / groupSizeY;

    /* If xCount/yCount is not multiple of corresponding groupsize
     * Most kernels should be OK. But histogram should not count any extra borders in bin[0].
     * So adjust groupSize to a next lower number that can be divided by xCount */
    if (Is_HistogramKernel(Parameters->kernel))
    {
        while (groupCountX * groupSizeX != xCount)
        {
            groupCountX++;
            groupSizeX = gcmMAX(1, xCount / groupCountX);
        }

        while (groupCountY * groupSizeY != yCount)
        {
            groupCountY++;
            groupSizeY = gcmMAX(1, yCount / groupCountY);
        }
    }

    if (groupSizeX * groupSizeY > maxGroupSize)
    {
        if (groupSizeX > groupSizeY)
        {
            groupSizeX = gcmMAX(1, maxGroupSize / groupSizeY);
        }
        else
        {
            groupSizeY = gcmMAX(1, maxGroupSize / groupSizeX);
        }

        /*re-compute number of groups in each direction when groupSize has beeen changed*/
        groupCountX = (xCount + groupSizeX - 1) / groupSizeX;
        groupCountY = (yCount + groupSizeY - 1) / groupSizeY;
    }

    gcoOS_ZeroMemory(&twParameters, gcmSIZEOF(gcsVX_THREAD_WALKER_PARAMETERS));

    twParameters.valueOrder         = Parameters->order;

    /* ToDo: Not to detect kernel for workDim */
    if (Is_OneDimKernel(Parameters->kernel))
        twParameters.workDim = 1;
    else
        twParameters.workDim = 2;

    twParameters.workGroupSizeX     = groupSizeX;
    twParameters.workGroupCountX    = groupCountX;

    twParameters.workGroupSizeY     = groupSizeY;
    twParameters.workGroupCountY    = groupCountY;

#if gcdVX_OPTIMIZER > 2
    if (Parameters->tiled)
    {
        twParameters.globalOffsetX  = Parameters->xmin + Parameters->xoffset;
        twParameters.globalScaleX   = Parameters->xstep;

        twParameters.globalOffsetY  = Parameters->ymin + Parameters->yoffset;
        twParameters.globalScaleY   = Parameters->ystep;
    }
    else
#endif
    {
        twParameters.globalOffsetX  = Parameters->xmin;
        twParameters.globalScaleX   = Parameters->xstep;

        twParameters.globalOffsetY  = Parameters->ymin;
        twParameters.globalScaleY   = Parameters->ystep;
    }

#if gcdVX_OPTIMIZER > 1
    twParameters.tileMode           = Parameters->tileMode;
#endif

    gcoOS_ZeroMemory(&info, gcmSIZEOF(gcsTHREAD_WALKER_INFO));

    info.dimensions         = twParameters.workDim;
    info.valueOrder         = twParameters.valueOrder;

    info.globalOffsetX      = twParameters.globalOffsetX;
    info.globalOffsetY      = twParameters.globalOffsetY;
    info.globalScaleX       = twParameters.globalScaleX;
    info.globalScaleY       = twParameters.globalScaleY;
    info.workGroupCountX    = twParameters.workGroupCountX;
    info.workGroupCountY    = twParameters.workGroupCountY;
    info.workGroupSizeX     = twParameters.workGroupSizeX;
    info.workGroupSizeY     = twParameters.workGroupSizeY;
    info.barrierUsed        = Parameters->hasBarrier;
    info.memoryAccessFlag   = Parameters->hasAtomic ? (gceMA_FLAG_ATOMIC | gceMA_FLAG_EVIS_ATOMADD)
                                                     : gceMA_FLAG_NONE;
    gcmONERROR(gcoVX_InvokeThreadWalker(&info));
    /* Success. */
    status = gcvSTATUS_OK;
OnError:
    gcmFOOTER_ARG("%d", status);
    return status;
}



gceSTATUS
gcoVX_KernelConstruct(
    IN OUT gcoVX_Hardware_Context   *Context
    )
{
    gceSTATUS           status = gcvSTATUS_OK;

    gcmHEADER_ARG("Context=%p", Context);

#if GC_VX_ASM
    if(Context->instructions->source != gcvNULL)
        gcmONERROR(gcoHARDWAREVX_GenerateMC(Context));
    else
#endif

    gcmONERROR(gcoHARDWAREVX_KenrelConstruct(Context));

#if !gcdVX_OPTIMIZER
    if (Context->node == gcvNULL)
    {
        gctPOINTER logical = gcvNULL;
        gctUINT32 size = Context->instructions->count * 4 * 4;
        gcmONERROR(gcoVX_AllocateMemoryEx(&size, gcvSURF_ICACHE, gcvPOOL_DEFAULT, 256, &Context->nodePhysicalAdress, &logical, &Context->node));
        gcoOS_MemCopy(logical, (gctPOINTER)Context->instructions->binarys, Context->instructions->count * 4 * 4);
        gcmDUMP(gcvNULL, "#[info instruction memory");
        gcmDUMP_BUFFER(gcvNULL, gcvDUMP_BUFFER_MEMORY, Context->nodePhysicalAdress, (gctUINT32*)logical, 0, Context->instructions->count * 4 * 4);

    }

    gcmONERROR(gcoHARDWAREVX_LoadKernel(gcvNULL, Context->nodePhysicalAdress, Context->instructions->count, Context->instructions->regs_count, Context->order));

#endif

OnError:
    gcmFOOTER_ARG("%d", status);
    return status;
}

gceSTATUS
gcoVX_LockKernel(
    IN OUT gcoVX_Hardware_Context   *Context
    )
{
    gceSTATUS           status = gcvSTATUS_OK;

    gcmHEADER_ARG("Context=%p", Context);

#if gcdVX_OPTIMIZER
    {
        gctPOINTER logical = gcvNULL;
        gctUINT size = Context->instructions->count * 4 * 4;
        gcmONERROR(gcoVX_AllocateMemoryEx(&size, gcvSURF_ICACHE, gcvPOOL_DEFAULT, 256, &Context->instructions->physical, &logical, &Context->node));
        gcoOS_MemCopy(logical, (gctPOINTER)Context->instructions->binarys, Context->instructions->count * 4 * 4);
        gcmDUMP(gcvNULL, "#[info instruction memory");
        gcmDUMP_BUFFER(gcvNULL, gcvDUMP_BUFFER_MEMORY, Context->instructions->physical, (gctUINT32*)logical, 0, Context->instructions->count * 4 * 4);
    }

OnError:
#endif
    gcmFOOTER_ARG("%d", status);
    return status;
}

gceSTATUS
gcoVX_BindKernel(
    IN OUT gcoVX_Hardware_Context   *Context
    )
{
    gceSTATUS           status = gcvSTATUS_OK;

    gcmHEADER_ARG("Context=%p", Context);

    gcmASSERT(gcoVX_VerifyHardware());
#if gcdVX_OPTIMIZER
#if gcdVX_OPTIMIZER > 1
    gcmONERROR(gcoHARDWAREVX_LoadKernel(gcvNULL,
                                        Context->instructions->physical,
                                        Context->instructions->count,
                                        Context->instructions->regs_count,
                                        &Context->order));
#else
    gcmONERROR(gcoHARDWAREVX_LoadKernel(gcvNULL,
                                        Context->instructions->physical,
                                        Context->instructions->count,
                                        Context->instructions->regs_count,
                                        Context->order));
#endif

OnError:
#endif
    gcmFOOTER_ARG("%d", status);
    return status;
}

gceSTATUS gcoVX_GetMemorySize(OUT gctUINT32_PTR Size)
{
    *Size = memory_size;
    return gcvSTATUS_OK;
}

gceSTATUS gcoVX_ZeroMemorySize()
{
    memory_size = 0;

    free_memory_size = 0;

    return gcvSTATUS_OK;
}

gceSTATUS
gcoVX_AllocateMemory(
    IN gctUINT32        Size,
    OUT gctPOINTER*     Logical,
    OUT gctUINT32*      Physical,
    OUT gcsSURF_NODE_PTR* Node
    )
{
    gceSTATUS          status = gcvSTATUS_OK;
    gctUINT32          size = Size;

    gcmHEADER_ARG("Size=%u Logical=%p", Size);

    gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_3D);

    gcmONERROR(gcoVX_AllocateMemoryEx(&size, gcvSURF_VERTEX, gcvPOOL_DEFAULT, 64, Physical, Logical, Node));

OnError:

    gcmFOOTER_ARG("%d", status);
    return status;
}

gceSTATUS
gcoVX_FreeMemory(
    IN gcsSURF_NODE_PTR Node
    )
{

    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Node=0x%x", Node);

    gcmONERROR(gcoVX_FreeMemoryEx(Node, gcvSURF_VERTEX));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_LoadKernelShader(
    IN gcsPROGRAM_STATE *ProgramState
    )
{

    gceSTATUS status;
    gceAPI    currentApi;

    gcmHEADER_ARG("StateBufferSize=%u StateBuffer=0x%x Hints=0x%x",
                  ProgramState->stateBufferSize, ProgramState->stateBuffer, ProgramState->hints);

    gcmASSERT(gcoVX_VerifyHardware());
    /* Set the hardware type. */
    gcmONERROR(gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_3D));

    /* Switch to the 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D, gcvNULL));

    /* Get Current API. */
    gcmVERIFY_OK(gcoHARDWARE_GetAPI(gcvNULL, &currentApi, gcvNULL));

    if (currentApi == 0)
    {
        /* Set HAL API to OpenCL only when there is API is not set. */
        gcmVERIFY_OK(gcoHARDWARE_SetAPI(gcvNULL, gcvAPI_OPENCL));
    }

    if (!gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_PIPE_CL))
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Set rounding mode */
    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_SHADER_HAS_RTNE))
    {
        gcmVERIFY_OK(gcoHARDWARE_SetRTNERounding(gcvNULL, gcvTRUE));
    }

    /* gcmONERROR(gcoCLHardware_Construct()) ; */

    /* Load kernel states. */
    status = gcLoadShaders(gcvNULL,
                           ProgramState);

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_InvokeKernelShader(
    IN gcSHADER            Kernel,
    IN gctUINT             WorkDim,
    IN size_t              GlobalWorkOffset[3],
    IN size_t              GlobalWorkScale[3],
    IN size_t              GlobalWorkSize[3],
    IN size_t              LocalWorkSize[3],
    IN gctUINT             ValueOrder,
    IN gctBOOL             BarrierUsed,
    IN gctUINT32           MemoryAccessFlag,
    IN gctBOOL             bDual16
    )
{
    gcsTHREAD_WALKER_INFO   info;
    gceSTATUS               status = gcvSTATUS_OK;

    gcmHEADER_ARG("Kernel=0x%x WorkDim=%d", Kernel, WorkDim);

    gcmASSERT(gcoVX_VerifyHardware());
    gcoOS_ZeroMemory(&info, gcmSIZEOF(gcsTHREAD_WALKER_INFO));

    info.dimensions      = WorkDim;
    info.globalSizeX     = (gctUINT32)GlobalWorkSize[0];
    info.globalOffsetX   = (gctUINT32)GlobalWorkOffset[0];
    info.globalScaleX    = (gctUINT32)GlobalWorkScale[0];
    info.workGroupSizeX  = LocalWorkSize[0] ? (gctUINT32)LocalWorkSize[0] : 1;

    info.workGroupCountX = info.globalSizeX / info.workGroupSizeX;

    if (WorkDim > 1)
    {
        info.globalSizeY     = (gctUINT32)GlobalWorkSize[1];
        info.globalOffsetY   = (gctUINT32)GlobalWorkOffset[1];
        info.globalScaleY    = (gctUINT32)GlobalWorkScale[1];
        info.workGroupSizeY  = LocalWorkSize[1] ? (gctUINT32)LocalWorkSize[1] : 1;

        info.workGroupCountY = info.globalSizeY / info.workGroupSizeY;

    }
    if (WorkDim > 2)
    {
        info.globalSizeZ     = (gctUINT32)GlobalWorkSize[2];
        info.globalOffsetZ   = (gctUINT32)GlobalWorkOffset[2];
        info.globalScaleZ    = (gctUINT32)GlobalWorkScale[2];
        info.workGroupSizeZ  = LocalWorkSize[2] ? (gctUINT32)LocalWorkSize[2] : 1;
        info.workGroupCountZ = info.globalSizeZ / info.workGroupSizeZ;
    }

    info.traverseOrder     = 0;  /* XYZ */
    info.enableSwathX      = 0;
    info.enableSwathY      = 0;
    info.enableSwathZ      = 0;
    info.swathSizeX        = 0;
    info.swathSizeY        = 0;
    info.swathSizeZ        = 0;
    info.valueOrder        = ValueOrder;
    info.barrierUsed       = BarrierUsed;
    info.memoryAccessFlag  = MemoryAccessFlag;
    info.bDual16           = bDual16;

    gcmONERROR(gcoVX_InvokeThreadWalker(&info));

OnError:
    gcmFOOTER_ARG("%d", status);
    return status;
}

gceSTATUS
gcoVX_Flush(
    IN gctBOOL      Stall
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Stall=%d", Stall);

    /* need to Verify Hardware ,there is a call at context destroy ? ; */
    /* Flush the current pipe. */
    gcmONERROR(gcoHARDWARE_FlushPipe(gcvNULL, gcvNULL));

    /* Commit the command buffer to hardware. */
    gcmONERROR(gcoHARDWARE_Commit(gcvNULL));

    if (Stall)
    {
        /* Stall the hardware. */
        gcmONERROR(gcoHARDWARE_Stall(gcvNULL));
    }

OnError:

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_TriggerAccelerator(
    IN gctUINT32              CmdAddress,
    IN gceVX_ACCELERATOR_TYPE Type,
    IN gctUINT32              EventId,
    IN gctBOOL                waitEvent,
    IN gctUINT32              gpuId,
    IN gctBOOL                sync,
    IN gctUINT32              syncEventID
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Cmd Address=%d", CmdAddress);

    gcmASSERT(gcoVX_VerifyHardware());
    gcmONERROR(gcoHARDWAREVX_TriggerAccelerator(gcvNULL, CmdAddress, Type, EventId, waitEvent, gpuId, sync, syncEventID));

OnError:

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_ProgrammCrossEngine(
    IN gctPOINTER             Data,
    IN gceVX_ACCELERATOR_TYPE Type,
    IN gctPOINTER             Options,
    IN OUT gctUINT32_PTR      *Instruction
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Data=%p Type=%d Instruction=%p", Data, Type, Instruction);

    gcmASSERT(gcoVX_VerifyHardware());
    switch (Type)
    {
    case gcvVX_ACCELERATOR_TP:
        gcmONERROR(gcoHARDWAREVX_ProgrammeTPEngine(gcvNULL, Data, Instruction));
        break;
    case gcvVX_ACCELERATOR_NN:
        gcmONERROR(gcoHARDWAREVX_ProgrammeNNEngine(gcvNULL, Data, Options, Instruction));
        break;
    default:
        gcmASSERT(0);
        break;
    }
OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_SetNNImage(
    IN gctPOINTER Data,
    IN OUT gctUINT32_PTR *Instruction
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Data=%p Instruction=%p", Data, Instruction);

    gcmASSERT(gcoVX_VerifyHardware());
    gcmONERROR(gcoHARDWAREVX_SetNNImage(gcvNULL, Data, Instruction));

OnError:
    gcmFOOTER();
    return status;
}

 /*  Three modes for openVX
   *      1. combined mode, one device use all gpucore.
   *         VIV_MGPU_AFFINITY=0, default mode
   *      2. independent mode, one device use one gpucore
   *         VIV_MGPU_AFFINITY=1:0 or 1:1,
   *      3. mulit-device mode, mulit device ,and every device will use one or more gpu count.
   *         this mode only work on independent mode.
   *         VIV_MGPU_AFFINITY=1:0
   *         VIV_OVX_USE_MULTI_DEVICE=1 or 1:1, 1:2,1:4, 1:2 means multi-device is enable, and every
   *         device will use 2 gpu core.
   *
   */

gceSTATUS
gcoVX_QueryDeviceCount(
    OUT gctUINT32 * DeviceCount
    )
{
    return _QueryDeviceCoreCount(DeviceCount, gcvNULL);
}

gceSTATUS
gcoVX_QueryCoreCount(
    gctUINT32  DeviceID,
    OUT gctUINT32 *CoreCount
    )
{
    gctUINT32 deviceCount, gpuCountPerDevice[MAX_GPU_CORE_COUNT];

    _QueryDeviceCoreCount(&deviceCount, gpuCountPerDevice);

    if (deviceCount <= DeviceID)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (CoreCount != gcvNULL)
    {
        *CoreCount = gpuCountPerDevice[DeviceID];
    }

    return gcvSTATUS_OK;
}

gceSTATUS
gcoVX_QueryMultiCore(
    OUT gctBOOL *IsMultiCore
    )
{
    gctUINT32 i, deviceCount, gpuCountPerDevice[MAX_GPU_CORE_COUNT];

    _QueryDeviceCoreCount(&deviceCount, gpuCountPerDevice);

    for (i = 0; i < deviceCount; i++)
    {
        if (gpuCountPerDevice[i] > 1)
            break;
    }

    if (IsMultiCore != gcvNULL)
    {
        if (i < deviceCount)
        {
            *IsMultiCore = gcvTRUE;
        }
        else
        {
            *IsMultiCore = gcvFALSE;
        }
    }

    return gcvSTATUS_OK;
}

gceSTATUS
gcoVX_GetNNConfig(
    IN OUT gctPOINTER Config
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Config=%p", Config);

    gcmONERROR(gcoHARDWARE_QueryNNConfig(gcvNULL, Config));

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_QueryHWChipInfo(
    IN OUT vx_hw_chip_info * HwChipInfo
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("HwChipInfo=%p", HwChipInfo);

    if (HwChipInfo != gcvNULL)
    {
        gcmONERROR(gcoHARDWARE_QueryHwChipInfo(gcvNULL,
            &HwChipInfo->customerID,
            &HwChipInfo->ecoID));
    }
    else
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
    }

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_WaitNNEvent(
    gctUINT32 EventId
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("id=%d", EventId);
    gcmASSERT(gcoVX_VerifyHardware());
    gcmONERROR(gcoHARDWAREVX_WaitNNEvent(gcvNULL, EventId));

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_FlushCache(
    IN gctBOOL      InvalidateICache,
    IN gctBOOL      FlushPSSHL1Cache,
    IN gctBOOL      FlushNNL1Cache,
    IN gctBOOL      FlushTPL1Cache,
    IN gctBOOL      FlushSHL1Cache,
    IN gctBOOL      Stall
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Stall=%d", Stall);

    gcmASSERT(gcoVX_VerifyHardware());
    /* Flush the shader cache. */
    gcmONERROR(gcoHARDWAREVX_FlushCache(gcvNULL, InvalidateICache, FlushPSSHL1Cache, FlushNNL1Cache, FlushTPL1Cache, FlushSHL1Cache));

    if (Stall)
    {
        /* Commit the command buffer to hardware. */
        gcmONERROR(gcoHARDWARE_Commit(gcvNULL));

        /* Stall the hardware. */
        gcmONERROR(gcoHARDWARE_Stall(gcvNULL));
    }

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_AllocateMemoryExAddAllocflag(
    IN OUT gctUINT *        Bytes,
    IN  gceSURF_TYPE        Type,
    IN  gctUINT32           alignment,
    IN  gctUINT32           allocflag,
    OUT gctUINT32 *         Physical,
    OUT gctPOINTER *        Logical,
    OUT gctUINT32 * CpuPhysicalAddress,
    OUT gcsSURF_NODE_PTR *  Node
    )
{
    gceSTATUS status;
    gctUINT bytes = 0;
    gcsSURF_NODE_PTR node = gcvNULL;
    gctPOINTER pointer = gcvNULL;
    gctPOINTER logical = gcvNULL;
    gctUINT32  physical = 0;
    gcmHEADER_ARG("*Bytes=%lu", *Bytes);

    /* memory allocat/release no need to gcoVX_VerifyHardware()); */
    if (!gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_SH_IMAGE_LD_LAST_PIXEL_FIX))
    {
        /* Allocate extra 15 bytes to avoid cache overflow */
        bytes = gcmALIGN(*Bytes + 15, 64);
    }
    else
    {
        bytes = gcmALIGN(*Bytes, 64);
    }

    /* By default, align it to 64 byte */
    if (alignment == 0)
    {
        alignment = 64;
    }

    gcmASSERT(Node);

    /* Allocate node. */
    gcmONERROR(gcoOS_Allocate(gcvNULL,
                              gcmSIZEOF(gcsSURF_NODE),
                              &pointer));

    node = (gcsSURF_NODE_PTR)pointer;

    gcmONERROR(gcsSURF_NODE_Construct(
        node,
        bytes,
        alignment,
        Type,
        allocflag,
        gcvPOOL_DEFAULT
        ));

    /* Lock the buffer. */
    gcmONERROR(gcoHARDWARE_LockAddCpuPhysicalAddr(node,
                                &physical,
                                &logical, CpuPhysicalAddress));

    /* Return allocated number of bytes. */
    if (Bytes)
    {
        *Bytes = bytes;
    }
    if (Logical)
    {
        *Logical = logical;
    }
    if (Physical)
    {
        *Physical = physical;
    }

    memory_size += bytes;

    /* Return node. */
    *Node = node;

    /* Success. */
    gcmFOOTER_ARG("*Bytes=%lu *Physical=0x%x *Logical=0x%x *CpuPhysicalAddress=0x%x *Node=0x%x",
                  bytes, physical, logical, *CpuPhysicalAddress, *Node);
    return gcvSTATUS_OK;

OnError:

    /* Return the status. */
    if(node != gcvNULL)
    {
        gcoOS_Free(gcvNULL, node);
        node = gcvNULL;
    }
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_AllocateMemoryEx(
    IN OUT gctUINT *        Bytes,
    IN  gceSURF_TYPE        Type,
    IN  gcePOOL             Pool,
    IN  gctUINT32           Alignment,
    OUT gctUINT32 *         Physical,
    OUT gctPOINTER *        Logical,
    OUT gcsSURF_NODE_PTR *  Node
    )
{
    gceSTATUS status;
    gctUINT bytes = 0;
    gcsSURF_NODE_PTR node = gcvNULL;
    gctPOINTER pointer = gcvNULL;
    gctPOINTER logical = gcvNULL;
    gctUINT32  physical = 0;

    gcmHEADER_ARG("*Bytes=%lu", *Bytes);

    /* memory allocat/release no need to gcoVX_VerifyHardware()); */
    if (!gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_SH_IMAGE_LD_LAST_PIXEL_FIX) && Pool != gcvPOOL_INTERNAL_SRAM && Pool != gcvPOOL_EXTERNAL_SRAM)
    {
        /* Allocate extra 15 bytes to avoid cache overflow */
        bytes = gcmALIGN(*Bytes + 15, 64);
    }
    else
    {
        bytes = gcmALIGN(*Bytes, 64);
    }

    /* By default, align it to 64 byte */
    if (Alignment == 0)
    {
        Alignment = 64;
    }

    gcmASSERT(Node);

    /* Allocate node. */
    gcmONERROR(gcoOS_Allocate(gcvNULL,
                              gcmSIZEOF(gcsSURF_NODE),
                              &pointer));

    node = (gcsSURF_NODE_PTR)pointer;

    gcmONERROR(gcsSURF_NODE_Construct(
        node,
        bytes,
        Alignment,
        Type,
        gcvALLOC_FLAG_NONE,
        Pool
        ));

    /* Lock the buffer. */
    gcmONERROR(gcoHARDWARE_Lock(node,
                                &physical,
                                &logical));

    /* Return allocated number of bytes. */
    if (Bytes)
    {
        *Bytes = bytes;
    }
    if (Logical)
    {
        *Logical = logical;
    }
    if (Physical)
    {
        *Physical = physical;
    }

    memory_size += bytes;

    /* Return node. */
    *Node = node;

    /* Success. */
    gcmFOOTER_ARG("*Bytes=%lu *Physical=0x%x *Logical=0x%x *Node=0x%x",
                  bytes, physical, logical, *Node);
    return gcvSTATUS_OK;

OnError:

    /* Return the status. */
    if(node != gcvNULL)
    {
        gcoOS_Free(gcvNULL, node);
        node = gcvNULL;
    }
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_FreeMemoryEx(
    IN gcsSURF_NODE_PTR     Node,
    IN gceSURF_TYPE         Type
    )
{
    /*gcmSWITCHHARDWAREVAR*/
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Node=0x%x Type=%d", Node, Type);

    /* Do we have a buffer allocated? */
    if (Node && Node->pool != gcvPOOL_UNKNOWN)
    {
        free_memory_size -= Node->size;
        /* Unlock the buffer. */
        gcmONERROR(gcoHARDWARE_Unlock(Node,Type));

        /* Create an event to free the video memory. */
        gcmONERROR(gcsSURF_NODE_Destroy(Node));

        /* Free node. */
        gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, Node));
    }

OnError:

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_SwitchContext(
    IN  gctUINT DeviceID,
    OUT gcoHARDWARE *SavedHardware,
    OUT gceHARDWARE_TYPE *SavedType,
    OUT gctUINT32    *SavedCoreIndex
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 coreIndex = 0;
    gcsTLS_PTR tls;

    gcmHEADER();
    gcmVERIFY_ARGUMENT(SavedHardware != gcvNULL);

    gcmONERROR(gcoOS_GetTLS(&tls));

    if(tls->engineVX == gcvNULL)
    {
        gcmONERROR(gcoVX_Construct(&tls->engineVX));
    }

    gcmONERROR(gcoHAL_GetCurrentCoreIndex(gcvNULL, SavedCoreIndex));
    gcmONERROR(gcoHARDWARE_Get3DHardware(SavedHardware));
    gcmONERROR(gcoHAL_GetHardwareType(gcvNULL, SavedType));

    gcmASSERT(DeviceID < tls->engineVX->deviceCount);
    gcmONERROR(gcoHARDWARE_Set3DHardware(tls->engineVX->hardwares[DeviceID]));
    gcoHARDWARE_QueryCoreIndex(tls->engineVX->hardwares[DeviceID], 0, &coreIndex);

    gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_3D ); /*Note! setHardwareType will QueryMultiGPUAffinityConfig and reset CoreIndex*/
    gcoHAL_SetCoreIndex(gcvNULL, coreIndex);

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_RestoreContext(
    IN gcoHARDWARE Hardware,
    gceHARDWARE_TYPE PreType,
    gctUINT32 PreCoreIndex
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER();

    gcmONERROR(gcoHARDWARE_Set3DHardware(Hardware));

    gcmONERROR(gcoHAL_SetHardwareType(gcvNULL, PreType));
    gcmONERROR(gcoHAL_SetCoreIndex(gcvNULL, PreCoreIndex));

OnError:
    gcmFOOTER();
    return status;
}

gctBOOL gcoVX_VerifyHardware()  /* Verify VX not use the default hardware*/
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsTLS_PTR tls;

    gcmHEADER();

    gcmONERROR(gcoOS_GetTLS(&tls));

    if(tls->currentHardware != tls->defaultHardware)
    {
        gcmFOOTER();
        return gcvTRUE;
    }

OnError:
    gcmFOOTER();
    return gcvFALSE ;
}


gceSTATUS
gcoVX_CaptureState(
    IN OUT gctUINT8 *CaptureBuffer,
    IN gctUINT32 InputSizeInByte,
    IN OUT gctUINT32 *pOutputSizeInByte,
    IN gctBOOL Enabled,
    IN gctBOOL dropCommandEnabled
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("CaptureBuffer=0x%x InputSizeInByte=%d pOutputSizeInByte=0x%x Enabled=%d dropCommandEnabled=%d",
                   CaptureBuffer, InputSizeInByte, pOutputSizeInByte, Enabled, dropCommandEnabled);
    gcmASSERT(gcoVX_VerifyHardware());
    gcmONERROR(gcoHARDWAREVX_CaptureState(gcvNULL, CaptureBuffer, InputSizeInByte, pOutputSizeInByte, Enabled, dropCommandEnabled));

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_CaptureInitState(
    IN OUT gctPOINTER *CaptureBuffer,
    IN gctUINT32 InputSizeInByte,
    IN OUT gctUINT32_PTR OutputSizeInByte,
    IN gctUINT32 deviceCount
    )
{
    gcsTLS_PTR  tls = gcvNULL;
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 i = 0;

    gcmHEADER();
    gcmONERROR(gcoOS_GetTLS( &tls));

    for (i = 0; i < deviceCount; i++)
    {
        gcoHARDWAREVX_CaptureInitState(tls->engineVX->hardwares[i], CaptureBuffer[i],
                                       InputSizeInByte, &OutputSizeInByte[i]);
    }

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_SetRemapAddress(
    IN gctUINT32 remapStart,
    IN gctUINT32 remapEnd,
    IN gceVX_REMAP_TYPE remapType
    )
{

    gceSTATUS status = gcvSTATUS_OK;
    gcsTLS_PTR  tls = gcvNULL;
    gctUINT32 i ;
    gcmHEADER();

    gcmONERROR(gcoOS_GetTLS( &tls));

    if(tls->engineVX == gcvNULL)
    {
        gcmONERROR(gcoVX_Construct(&tls->engineVX));
    }

    for(i = 0; i < tls->engineVX->deviceCount; i++)
    {
        gcmONERROR(gcoHARDWAREVX_SetRemapAddress(tls->engineVX->hardwares[i], remapStart, remapEnd, remapType));
    }
OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_ProgrammYUV2RGBScale(
    IN gctPOINTER Data,
    IN gctUINT32  gpuId,
    IN gctBOOL    mGpuSync
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Data=%p", Data);

    gcmASSERT(gcoVX_VerifyHardware());

    gcmONERROR(gcoHARDWAREVX_YUV2RGBScale(gcvNULL, Data, gpuId, mGpuSync));

OnError:
    gcmFOOTER();
    return status;
}


gceSTATUS
gcoVX_CreateHW(
    IN gctUINT32  DeviceID,
    IN gctUINT32  GpuCountPerDevice,
    IN gctUINT32  GpuCoreIndexs[],
    OUT gcoHARDWARE * Hardware
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcoHARDWARE  hardware = gcvNULL;
    gctBOOL      changed = gcvFALSE;
    gceHARDWARE_TYPE preType = gcvHARDWARE_INVALID ;
    gctUINT32   preCoreIndex;
    gcmHEADER_ARG("DeviceID=%d GpuCount=%d", DeviceID, GpuCountPerDevice);

    gcmVERIFY_OK(gcoHAL_GetHardwareType(gcvNULL, &preType));
    gcmVERIFY_OK(gcoHAL_GetCurrentCoreIndex(gcvNULL, &preCoreIndex));
    gcmVERIFY_OK(gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_3D));

    gcmONERROR(gcoHAL_SetCoreIndex(gcvNULL, GpuCoreIndexs[0]));

    changed = gcvTRUE;
    gcmONERROR(gcoHARDWARE_ConstructEx(gcPLS.hal,
                                       gcvFALSE,
                                       gcvFALSE,
                                       gcvHARDWARE_3D,
                                       GpuCountPerDevice,
                                       GpuCoreIndexs,
                                       &hardware));

    /* gcoHARDWARE_SetMultiGPUMode(hardware,gcvMULTI_GPU_MODE_INDEPENDENT); */
    if (gcoHARDWARE_IsFeatureAvailable(hardware, gcvFEATURE_MCFE))
    {
        gcoHARDWARE_SelectChannel(hardware, 0, 1);
    }
    /* Switch to the 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(hardware, gcvPIPE_3D, gcvNULL));

    /* Set HAL API to OpenCL only when there is API is not set. */
    gcmVERIFY_OK(gcoHARDWARE_SetAPI(hardware, gcvAPI_OPENCL));

    if (!gcoHARDWARE_IsFeatureAvailable(hardware, gcvFEATURE_PIPE_CL))
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }



    gcmONERROR(gcoHARDWARE_SetFenceEnabled(hardware, gcvTRUE));
    gcmVERIFY_OK(gcoHARDWAREVX_InitVX(hardware));

    *Hardware = hardware;

    gcmVERIFY_OK(gcoHAL_SetHardwareType(gcvNULL, preType));
    gcmVERIFY_OK(gcoHAL_SetCoreIndex(gcvNULL, preCoreIndex));

    gcmFOOTER();
    return status;

OnError:
    if(changed)
    {
        gcmVERIFY_OK(gcoHAL_SetHardwareType(gcvNULL, preType));
        gcmVERIFY_OK(gcoHAL_SetCoreIndex(gcvNULL, preCoreIndex));
    }
    if(hardware)
    {
        gcoHARDWARE_Destroy(hardware, gcvFALSE);
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVX_DestroyHW(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmONERROR(gcoHARDWARE_Destroy((gcoHARDWARE)Hardware, gcvFALSE));

    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS gcoVX_GetEvisNoInstFeatureCap(
    OUT vx_evis_no_inst_s *EvisNoInst
    )
{
    gcsTLS_PTR tls;
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmONERROR(gcoOS_GetTLS(&tls));

    if(tls->engineVX == gcvNULL)
    {
        gcmONERROR(gcoVX_Construct(&tls->engineVX));
    }

    gcmASSERT(EvisNoInst);

    if(EvisNoInst->isSet)
    {
        return gcvSTATUS_OK;
    }

    hardware = tls->engineVX->hardwares[0]; /*Use the 0th hardware as Query input */

    if(gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_EVIS))
    {
        EvisNoInst->supportEVIS = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(hardware, gcvFEATURE_EVIS_NO_ABSDIFF))
    {
        EvisNoInst->noAbsDiff = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(hardware, gcvFEATURE_EVIS_NO_BITREPLACE))
    {
        EvisNoInst->noBitReplace = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(hardware, gcvFEATURE_EVIS_NO_CORDIAC))
    {
        EvisNoInst->noMagPhase = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(hardware, gcvFEATURE_EVIS_NO_DP32))
    {
        EvisNoInst->noDp32 = gcvTRUE;
        EvisNoInst->clamp8Output = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(hardware, gcvFEATURE_EVIS_NO_FILTER))
    {
        EvisNoInst->noFilter = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_EVIS_NO_BOXFILTER))
    {
        EvisNoInst->noBoxFilter = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(hardware, gcvFEATURE_EVIS_NO_IADD))
    {
        EvisNoInst->noIAdd = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(hardware, gcvFEATURE_EVIS_NO_SELECTADD))
    {
        EvisNoInst->noSelectAdd = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_EVIS_LERP_7OUTPUT))
    {
        EvisNoInst->lerp7Output  = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(hardware, gcvFEATURE_EVIS_ACCSQ_8OUTPUT))
    {
        EvisNoInst->accsq8Output = gcvTRUE;
    }

    if (gcoHARDWARE_IsFeatureAvailable(hardware, gcvFEATURE_EVIS_VX2))
    {
        EvisNoInst->isVX2 = gcvTRUE;
    }

    EvisNoInst->isSet = gcvTRUE;
    return gcvSTATUS_OK;

    OnError:
    return status;
}


#endif /* gcdUSE_VX */
