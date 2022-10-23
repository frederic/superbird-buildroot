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


/*******************************************************************************
**    Profiler for Vivante HAL.
*/
#include "gc_hal_user_precomp.h"

#define _GC_OBJ_ZONE            gcdZONE_PROFILER

#if gcdENABLE_3D
#if VIVANTE_PROFILER

gcmINLINE static gctUINT32
CalcDelta(
    IN gctUINT32 new,
    IN gctUINT32 old
    )
{
    if (new == 0xdeaddead || new == 0xdead)
    {
        return new;
    }
    if (new >= old)
    {
        return new - old;
    }
    else
    {
        return (gctUINT32)((gctUINT64)new + 0x100000000ll - old);
    }
}

static gceSTATUS
_SetProfiler(
    IN gcoPROFILER Profiler
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctCHAR inputFileName[256] = { '\0' };
    gctCHAR profilerName[256] = { '\0' };
    gctHANDLE pid = gcoOS_GetCurrentProcessID();
    static gctUINT8 num = 1;
    gctUINT offset = 0;
    gctCHAR* pos = gcvNULL;
    gctCHAR *env = gcvNULL;
#ifdef ANDROID
    gctBOOL matchResult = gcvFALSE;
#endif

    gcmHEADER_ARG("Profiler=0x%x", Profiler);

    gcoOS_GetEnv(gcvNULL, "VP_OUTPUT", &env);
    if ((env != gcvNULL) && *env != '\0')
    {
        Profiler->fileName = env;
    }

#ifdef ANDROID
    gcoOS_GetEnv(gcvNULL, "VP_PROCESS_NAME", &env);
    if ((env != gcvNULL) && (env[0] != 0)) matchResult = (gcoOS_DetectProcessByName(env) ? gcvTRUE : gcvFALSE);
    if (matchResult != gcvTRUE)
    {
        gcmFOOTER();
        return gcvSTATUS_MISMATCH;
    }
#endif

    /*generate file name for each context*/
    if (Profiler->fileName) gcoOS_StrCatSafe(inputFileName, 256, Profiler->fileName);

    gcmONERROR(gcoOS_StrStr(inputFileName, ".vpd", &pos));
    if (pos) pos[0] = '\0';
    gcmONERROR(gcoOS_PrintStrSafe(profilerName, gcmSIZEOF(profilerName), &offset, "%s_%d_%d.vpd", inputFileName, (gctUINTPTR_T)pid, num++));

    gcmONERROR(gcoOS_Open(gcvNULL, profilerName, gcvFILE_CREATE, &Profiler->file));

    gcoOS_GetEnv(gcvNULL, "VP_ENABLE_PRINT", &env);

    /*set print for vx&cl by default, bufferCount set to 1 becuase of
    print counter need get the counter right now for each operation*/
    if (((env != gcvNULL) && gcmIS_SUCCESS(gcoOS_StrCmp(env, "1"))) ||
        Profiler->profilerClient == gcvCLIENT_OPENCL ||
        Profiler->profilerClient == gcvCLIENT_OPENVX)
    {
        Profiler->enablePrint = gcvTRUE;
        Profiler->bufferCount = 1;
    }

    gcoOS_GetEnv(gcvNULL, "VP_DISABLE_PROBE", &env);
    if ((env != gcvNULL) && gcmIS_SUCCESS(gcoOS_StrCmp(env, "1")))
    {
        Profiler->disableProbe = gcvTRUE;
    }

OnError:

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AllocateCounters(
    IN gcoPROFILER Profiler,
    OUT gcsCounterBuffer_PTR * CounterBuffer
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsCounterBuffer_PTR counterBuffer = gcvNULL;

    gcmONERROR(gcoOS_Allocate(gcvNULL,
        gcmSIZEOF(struct gcsCounterBuffer), (gctPOINTER *)&counterBuffer));

    counterBuffer->couterBufobj = gcvNULL;

    gcmONERROR(gcoOS_Allocate(gcvNULL,
        gcmSIZEOF(gcsPROFILER_COUNTERS) * Profiler->coreCount,
        (gctPOINTER *)&counterBuffer->counters));

    gcoOS_ZeroMemory(counterBuffer->counters, gcmSIZEOF(gcsPROFILER_COUNTERS) * Profiler->coreCount);

    counterBuffer->opType = gcvCOUNTER_OP_NONE;
    counterBuffer->opID = 0;

    counterBuffer->available = gcvTRUE;
    counterBuffer->needDump = gcvTRUE;
    counterBuffer->startPos =
    counterBuffer->endPos = 0;
    counterBuffer->dataSize = 0;

    *CounterBuffer = counterBuffer;

    /* Success. */
    return gcvSTATUS_OK;
OnError:

    if (counterBuffer->counters)
    {
        gcmOS_SAFE_FREE(gcvNULL, counterBuffer->counters);
    }

    if (counterBuffer)
    {
        gcmOS_SAFE_FREE(gcvNULL, counterBuffer);
    }

    return status;
}

static void
_RecordCounters(
    IN gctPOINTER Logical,
    IN gctUINT32 CoreId,
    OUT gcsPROFILER_COUNTERS * Counters
    )
{
    gctUINT32_PTR memory = gcvNULL;
    gctUINT32 offset = 0;
    gctUINT32 clusterIDWidth = 0;

    gcoHARDWARE_QueryCluster(gcvNULL, gcvNULL, gcvNULL, gcvNULL, &clusterIDWidth);

    memory = (gctUINT32_PTR)Logical;

    gcoOS_ZeroMemory(&Counters->counters_part1, gcmSIZEOF(gcsPROFILER_COUNTERS_PART1));
    gcoOS_ZeroMemory(&Counters->counters_part2, gcmSIZEOF(gcsPROFILER_COUNTERS_PART2));
    /* module FE */
    gcmGET_COUNTER(Counters->counters_part1.fe_out_vertex_count, 0);
    gcmGET_COUNTER(Counters->counters_part1.fe_cache_miss_count, 1);
    gcmGET_COUNTER(Counters->counters_part1.fe_cache_lk_count, 2);
    gcmGET_COUNTER(Counters->counters_part1.fe_stall_count, 3);
    gcmGET_COUNTER(Counters->counters_part1.fe_process_count, 4);
    Counters->counters_part1.fe_starve_count = 0xDEADDEAD;
    offset += MODULE_FRONT_END_COUNTER_NUM;

    gcmGET_COUNTER(Counters->counters_part1.vs_shader_cycle_count, 0);
    gcmGET_COUNTER(Counters->counters_part1.vs_inst_counter, 3);
    gcmGET_COUNTER(Counters->counters_part1.vs_rendered_vertice_counter, 4);
    gcmGET_COUNTER(Counters->counters_part1.vs_branch_inst_counter, 5);
    gcmGET_COUNTER(Counters->counters_part1.vs_texld_inst_counter, 6);
    offset += MODULE_VERTEX_SHADER_COUNTER_NUM;

    /* module PA */
    gcmGET_COUNTER(Counters->counters_part1.pa_input_vtx_counter, 0);
    gcmGET_COUNTER(Counters->counters_part1.pa_input_prim_counter, 1);
    gcmGET_COUNTER(Counters->counters_part1.pa_output_prim_counter, 2);
    gcmGET_COUNTER(Counters->counters_part1.pa_trivial_rejected_counter, 3);
    gcmGET_COUNTER(Counters->counters_part1.pa_culled_prim_counter, 4);
    gcmGET_COUNTER(Counters->counters_part1.pa_droped_prim_counter, 5);
    gcmGET_COUNTER(Counters->counters_part1.pa_frustum_clipped_prim_counter, 6);
    gcmGET_COUNTER(Counters->counters_part1.pa_frustum_clipdroped_prim_counter, 7);
    gcmGET_COUNTER(Counters->counters_part1.pa_non_idle_starve_count, 8);
    gcmGET_COUNTER(Counters->counters_part1.pa_starve_count, 9);
    gcmGET_COUNTER(Counters->counters_part1.pa_stall_count, 10);
    gcmGET_COUNTER(Counters->counters_part1.pa_process_count, 11);
    offset += MODULE_PRIMITIVE_ASSEMBLY_COUNTER_NUM;

    /* module SE */
    gcmGET_COUNTER(Counters->counters_part1.se_starve_count, 0);
    gcmGET_COUNTER(Counters->counters_part1.se_stall_count, 1);
    gcmGET_COUNTER(Counters->counters_part1.se_receive_triangle_count, 2);
    gcmGET_COUNTER(Counters->counters_part1.se_send_triangle_count, 3);
    gcmGET_COUNTER(Counters->counters_part1.se_receive_lines_count, 4);
    gcmGET_COUNTER(Counters->counters_part1.se_send_lines_count, 5);
    gcmGET_COUNTER(Counters->counters_part1.se_process_count, 6);
    gcmGET_COUNTER(Counters->counters_part1.se_clipped_triangle_count, 7);
    gcmGET_COUNTER(Counters->counters_part1.se_clipped_line_count, 8);
    gcmGET_COUNTER(Counters->counters_part1.se_culled_lines_count, 9);
    gcmGET_COUNTER(Counters->counters_part1.se_culled_triangle_count, 10);
    gcmGET_COUNTER(Counters->counters_part1.se_trivial_rejected_line_count, 11);
    gcmGET_COUNTER(Counters->counters_part1.se_non_idle_starve_count, 12);
    offset += MODULE_SETUP_COUNTER_NUM;

    /* module RA */
    gcmGET_COUNTER(Counters->counters_part1.ra_input_prim_count, 0);
    gcmGET_COUNTER(Counters->counters_part1.ra_total_quad_count, 1);
    gcmGET_COUNTER(Counters->counters_part1.ra_prefetch_cache_miss_counter, 2);
    gcmGET_COUNTER(Counters->counters_part1.ra_prefetch_hz_cache_miss_counter, 3);
    gcmGET_COUNTER(Counters->counters_part1.ra_valid_quad_count_after_early_z, 4);
    gcmGET_COUNTER(Counters->counters_part1.ra_valid_pixel_count_to_render, 5);
    gcmGET_COUNTER(Counters->counters_part1.ra_output_valid_quad_count, 6);
    gcmGET_COUNTER(Counters->counters_part1.ra_output_valid_pixel_count, 7);
    Counters->counters_part1.ra_eez_culled_counter = 0xDEADDEAD;
    gcmGET_COUNTER(Counters->counters_part1.ra_pipe_cache_miss_counter, 8);
    gcmGET_COUNTER(Counters->counters_part1.ra_pipe_hz_cache_miss_counter, 9);
    gcmGET_COUNTER(Counters->counters_part1.ra_non_idle_starve_count, 10);
    gcmGET_COUNTER(Counters->counters_part1.ra_starve_count, 11);
    gcmGET_COUNTER(Counters->counters_part1.ra_stall_count, 12);
    gcmGET_COUNTER(Counters->counters_part1.ra_process_count, 13);
    offset += MODULE_RASTERIZER_COUNTER_NUM;

    /* module PS */
    gcmGET_COUNTER(Counters->counters_part1.ps_shader_cycle_count, 0);
    gcmGET_COUNTER(Counters->counters_part1.ps_inst_counter, 1);
    gcmGET_COUNTER(Counters->counters_part1.ps_rendered_pixel_counter, 2);
    gcmGET_COUNTER(Counters->counters_part1.ps_branch_inst_counter, 7);
    gcmGET_COUNTER(Counters->counters_part1.ps_texld_inst_counter, 8);
    offset += MODULE_PIXEL_SHADER_COUNTER_NUM;

    /* module TX */
    gcmGET_COUNTER(Counters->counters_part1.tx_total_bilinear_requests, 0);
    gcmGET_COUNTER(Counters->counters_part1.tx_total_trilinear_requests, 1);
    gcmGET_COUNTER(Counters->counters_part1.tx_total_discarded_texture_requests, 2);
    gcmGET_COUNTER(Counters->counters_part1.tx_total_texture_requests, 3);
    gcmGET_COUNTER(Counters->counters_part1.tx_mc0_miss_count, 4);
    gcmGET_COUNTER(Counters->counters_part1.tx_mc0_request_byte_count, 5);
    gcmGET_COUNTER(Counters->counters_part1.tx_mc1_miss_count, 6);
    gcmGET_COUNTER(Counters->counters_part1.tx_mc1_request_byte_count, 7);
    offset += MODULE_TEXTURE_COUNTER_NUM;

    /* module PE */
    gcmGET_COUNTER(Counters->counters_part1.pe0_pixel_count_killed_by_color_pipe, 0);
    gcmGET_COUNTER(Counters->counters_part1.pe0_pixel_count_killed_by_depth_pipe, 1);
    gcmGET_COUNTER(Counters->counters_part1.pe0_pixel_count_drawn_by_color_pipe, 2);
    gcmGET_COUNTER(Counters->counters_part1.pe0_pixel_count_drawn_by_depth_pipe, 3);
    gcmGET_COUNTER(Counters->counters_part1.pe1_pixel_count_killed_by_color_pipe, 4);
    gcmGET_COUNTER(Counters->counters_part1.pe1_pixel_count_killed_by_depth_pipe, 5);
    gcmGET_COUNTER(Counters->counters_part1.pe1_pixel_count_drawn_by_color_pipe, 6);
    gcmGET_COUNTER(Counters->counters_part1.pe1_pixel_count_drawn_by_depth_pipe, 7);
    offset += MODULE_PIXEL_ENGINE_COUNTER_NUM;

    /* module MCC */
    gcmGET_LATENCY_COUNTER(Counters->counters_part2.mcc_axi_min_latency, Counters->counters_part2.mcc_axi_max_latency, 0);
    gcmGET_COUNTER(Counters->counters_part2.mcc_axi_total_latency, 1);
    gcmGET_COUNTER(Counters->counters_part2.mcc_axi_sample_count, 2);
    gcmGET_COUNTER(Counters->counters_part2.mcc_total_read_req_8B_from_colorpipe, 3);
    gcmGET_COUNTER(Counters->counters_part2.mcc_total_read_req_8B_sentout_from_colorpipe, 4);
    gcmGET_COUNTER(Counters->counters_part2.mcc_total_write_req_8B_from_colorpipe, 5);
    gcmGET_COUNTER(Counters->counters_part2.mcc_total_read_req_sentout_from_colorpipe, 6);
    gcmGET_COUNTER(Counters->counters_part2.mcc_total_write_req_from_colorpipe, 7);
    gcmGET_COUNTER(Counters->counters_part2.mcc_total_read_req_8B_from_others, 8);
    gcmGET_COUNTER(Counters->counters_part2.mcc_total_write_req_8B_from_others, 9);
    gcmGET_COUNTER(Counters->counters_part2.mcc_total_read_req_from_others, 10);
    gcmGET_COUNTER(Counters->counters_part2.mcc_total_write_req_from_others, 11);
    offset += MODULE_MEMORY_CONTROLLER_COLOR_COUNTER_NUM;

    /* module MCZ */
    gcmGET_LATENCY_COUNTER(Counters->counters_part2.mcz_axi_min_latency, Counters->counters_part2.mcz_axi_max_latency, 0);
    gcmGET_COUNTER(Counters->counters_part2.mcz_axi_total_latency, 1);
    gcmGET_COUNTER(Counters->counters_part2.mcz_axi_sample_count, 2);
    gcmGET_COUNTER(Counters->counters_part2.mcz_total_read_req_8B_from_colorpipe, 3);
    gcmGET_COUNTER(Counters->counters_part2.mcz_total_read_req_8B_sentout_from_colorpipe, 4);
    gcmGET_COUNTER(Counters->counters_part2.mcz_total_write_req_8B_from_colorpipe, 5);
    gcmGET_COUNTER(Counters->counters_part2.mcz_total_read_req_sentout_from_colorpipe, 6);
    gcmGET_COUNTER(Counters->counters_part2.mcz_total_write_req_from_colorpipe, 7);
    gcmGET_COUNTER(Counters->counters_part2.mcz_total_read_req_8B_from_others, 8);
    gcmGET_COUNTER(Counters->counters_part2.mcz_total_write_req_8B_from_others, 9);
    gcmGET_COUNTER(Counters->counters_part2.mcz_total_read_req_from_others, 10);
    gcmGET_COUNTER(Counters->counters_part2.mcz_total_write_req_from_others, 11);
    offset += MODULE_MEMORY_CONTROLLER_DEPTH_COUNTER_NUM;

    /* module HI0 */
    gcmGET_COUNTER(Counters->counters_part2.hi0_total_read_8B_count, 0);
    gcmGET_COUNTER(Counters->counters_part2.hi0_total_write_8B_count, 1);
    gcmGET_COUNTER(Counters->counters_part2.hi0_total_write_request_count, 2);
    gcmGET_COUNTER(Counters->counters_part2.hi0_total_read_request_count, 3);
    gcmGET_COUNTER(Counters->counters_part2.hi0_axi_cycles_read_request_stalled, 4);
    gcmGET_COUNTER(Counters->counters_part2.hi0_axi_cycles_write_request_stalled, 5);
    gcmGET_COUNTER(Counters->counters_part2.hi0_axi_cycles_write_data_stalled, 6);
    gcmGET_COUNTER(Counters->counters_part2.hi_total_cycle_count, 7);
    gcmGET_COUNTER(Counters->counters_part2.hi_total_idle_cycle_count, 8);
    offset += MODULE_HOST_INTERFACE0_COUNTER_NUM;

    /* module HI1 */
    gcmGET_COUNTER(Counters->counters_part2.hi1_total_read_8B_count, 0);
    gcmGET_COUNTER(Counters->counters_part2.hi1_total_write_8B_count, 1);
    gcmGET_COUNTER(Counters->counters_part2.hi1_total_write_request_count, 2);
    gcmGET_COUNTER(Counters->counters_part2.hi1_total_read_request_count, 3);
    gcmGET_COUNTER(Counters->counters_part2.hi1_axi_cycles_read_request_stalled, 4);
    gcmGET_COUNTER(Counters->counters_part2.hi1_axi_cycles_write_request_stalled, 5);
    gcmGET_COUNTER(Counters->counters_part2.hi1_axi_cycles_write_data_stalled, 6);
    offset += MODULE_HOST_INTERFACE1_COUNTER_NUM;

    /* module L2 */
    gcmGET_COUNTER(Counters->counters_part2.l2_total_axi0_read_request_count, 0);
    gcmGET_COUNTER(Counters->counters_part2.l2_total_axi1_read_request_count, 1);
    gcmGET_COUNTER(Counters->counters_part2.l2_total_axi0_write_request_count, 2);
    gcmGET_COUNTER(Counters->counters_part2.l2_total_axi1_write_request_count, 3);
    gcmGET_COUNTER(Counters->counters_part2.l2_total_read_transactions_request_by_axi0, 4);
    gcmGET_COUNTER(Counters->counters_part2.l2_total_read_transactions_request_by_axi1, 5);
    gcmGET_COUNTER(Counters->counters_part2.l2_total_write_transactions_request_by_axi0, 6);
    gcmGET_COUNTER(Counters->counters_part2.l2_total_write_transactions_request_by_axi1, 7);
    gcmGET_LATENCY_COUNTER(Counters->counters_part2.l2_axi0_min_latency, Counters->counters_part2.l2_axi0_max_latency, 8);
    gcmGET_COUNTER(Counters->counters_part2.l2_axi0_total_latency, 9);
    gcmGET_COUNTER(Counters->counters_part2.l2_axi0_total_request_count, 10);
    gcmGET_LATENCY_COUNTER(Counters->counters_part2.l2_axi1_min_latency, Counters->counters_part2.l2_axi1_max_latency, 11);
    gcmGET_COUNTER(Counters->counters_part2.l2_axi1_total_latency, 12);
    gcmGET_COUNTER(Counters->counters_part2.l2_axi1_total_request_count, 13);
}

static void
_WriteCounters(
    IN gcoPROFILER Profiler
    )
{
    gcsPROFILER_COUNTERS *counters;
    gcsPROFILER_COUNTERS *preCounters;
    gceCOUNTER_OPTYPE opType;
    gctUINT32 opID;
    gctUINT32 coreId;
    gceSTATUS status;
    gctINT32 * counterData = gcvNULL;

    gcmASSERT(Profiler->enable);

    if (Profiler->probeMode)
    {
        for (coreId = 0; coreId < Profiler->coreCount; coreId++)
        {
            _RecordCounters(Profiler->counterBuf->logicalAddress, coreId, &Profiler->counterBuf->counters[coreId]);
        }
    }

    opType = Profiler->counterBuf->opType;
    opID = Profiler->counterBuf->opID;

#define gcmGETCOUNTER(name) ((opID != 0 && opType == gcvCOUNTER_OP_DRAW) ? CalcDelta((counters->name), (preCounters->name)) : (counters->name))

    if (Profiler->counterBuf->needDump)
    {
        gctUINT32 counterIndex;

        counterIndex = 0;

        gcmONERROR(gcoOS_Allocate(gcvNULL, Profiler->counterBuf->dataSize, (gctPOINTER *)&counterData));
        gcoOS_ZeroMemory(counterData, Profiler->counterBuf->dataSize);

        if (opType == gcvCOUNTER_OP_DRAW  && Profiler->perDrawMode)
        {
            gcmRECORD_CONST(VPG_ES30_DRAW);
            gcmRECORD_COUNTER(VPC_ES30_DRAW_NO, opID);
        }
        else
        {
            if (Profiler->coreCount == 1)
            {
                gcmRECORD_CONST(VPG_HW);
            }
        }

        for (coreId = 0; coreId < Profiler->coreCount; coreId++)
        {
            counters = &(Profiler->counterBuf->counters[coreId]);
            preCounters = &(Profiler->counterBuf->prev->counters[coreId]);

            if (Profiler->coreCount > 1)
            {
                gcmRECORD_CONST(VPG_MULTI_GPU);
                gcmRECORD_COUNTER(VPC_ES30_GPU_NO, coreId);
            }

            gcmRECORD_CONST(VPNG_FE);
            gcmRECORD_COUNTER(VPNC_FEDRAWCOUNT, gcmGETCOUNTER(counters_part1.fe_draw_count));
            gcmRECORD_COUNTER(VPNC_FEOUTVERTEXCOUNT, gcmGETCOUNTER(counters_part1.fe_out_vertex_count));
            gcmRECORD_COUNTER(VPNC_FECACHEMISSCOUNT, gcmGETCOUNTER(counters_part1.fe_cache_miss_count));
            gcmRECORD_COUNTER(VPNC_FECACHELKCOUNT, gcmGETCOUNTER(counters_part1.fe_cache_lk_count));
            gcmRECORD_COUNTER(VPNC_FESTALLCOUNT, gcmGETCOUNTER(counters_part1.fe_stall_count));
            gcmRECORD_COUNTER(VPNC_FESTARVECOUNT, gcmGETCOUNTER(counters_part1.fe_starve_count));
            gcmRECORD_COUNTER(VPNC_FEPROCESSCOUNT, gcmGETCOUNTER(counters_part1.fe_stall_count));
            gcmRECORD_CONST(VPG_END);

            gcmRECORD_CONST(VPNG_VS);
            gcmRECORD_COUNTER(VPNC_VSINSTCOUNT, gcmGETCOUNTER(counters_part1.vs_inst_counter));
            gcmRECORD_COUNTER(VPNC_VSBRANCHINSTCOUNT, gcmGETCOUNTER(counters_part1.vs_branch_inst_counter));
            gcmRECORD_COUNTER(VPNC_VSTEXLDINSTCOUNT, gcmGETCOUNTER(counters_part1.vs_texld_inst_counter));
            gcmRECORD_COUNTER(VPNC_VSRENDEREDVERTCOUNT, gcmGETCOUNTER(counters_part1.vs_rendered_vertice_counter));
            gcmRECORD_COUNTER(VPNC_VSNONIDLESTARVECOUNT, gcmGETCOUNTER(counters_part1.vs_non_idle_starve_count));
            gcmRECORD_COUNTER(VPNC_VSSTARVELCOUNT, gcmGETCOUNTER(counters_part1.vs_starve_count));
            gcmRECORD_COUNTER(VPNC_VSSTALLCOUNT, gcmGETCOUNTER(counters_part1.vs_stall_count));
            gcmRECORD_COUNTER(VPNC_VSPROCESSCOUNT, gcmGETCOUNTER(counters_part1.vs_process_count));
            gcmRECORD_COUNTER(VPNC_VSSHADERCYCLECOUNT, gcmGETCOUNTER(counters_part1.vs_shader_cycle_count));
            gcmRECORD_CONST(VPG_END);

            gcmRECORD_CONST(VPNG_PA);
            gcmRECORD_COUNTER(VPNC_PAINVERTCOUNT, gcmGETCOUNTER(counters_part1.pa_input_vtx_counter));
            gcmRECORD_COUNTER(VPNC_PAINPRIMCOUNT, gcmGETCOUNTER(counters_part1.pa_input_prim_counter));
            gcmRECORD_COUNTER(VPNC_PAOUTPRIMCOUNT, gcmGETCOUNTER(counters_part1.pa_output_prim_counter));
            gcmRECORD_COUNTER(VPNC_PADEPTHCLIPCOUNT, gcmGETCOUNTER(counters_part1.pa_depth_clipped_counter));
            gcmRECORD_COUNTER(VPNC_PATRIVIALREJCOUNT, gcmGETCOUNTER(counters_part1.pa_trivial_rejected_counter));
            gcmRECORD_COUNTER(VPNC_PACULLPRIMCOUNT, gcmGETCOUNTER(counters_part1.pa_culled_prim_counter));
            gcmRECORD_COUNTER(VPNC_PADROPPRIMCOUNT, gcmGETCOUNTER(counters_part1.pa_droped_prim_counter));
            gcmRECORD_COUNTER(VPNC_PAFRCLIPPRIMCOUNT, gcmGETCOUNTER(counters_part1.pa_frustum_clipped_prim_counter));
            gcmRECORD_COUNTER(VPNC_PAFRCLIPDROPPRIMCOUNT, gcmGETCOUNTER(counters_part1.pa_frustum_clipdroped_prim_counter));
            gcmRECORD_COUNTER(VPNC_PANONIDLESTARVECOUNT, gcmGETCOUNTER(counters_part1.pa_non_idle_starve_count));
            gcmRECORD_COUNTER(VPNC_PASTARVELCOUNT, gcmGETCOUNTER(counters_part1.pa_starve_count));
            gcmRECORD_COUNTER(VPNC_PASTALLCOUNT, gcmGETCOUNTER(counters_part1.pa_stall_count));
            gcmRECORD_COUNTER(VPNC_PAPROCESSCOUNT, gcmGETCOUNTER(counters_part1.pa_process_count));
            gcmRECORD_CONST(VPG_END);

            gcmRECORD_CONST(VPNG_SETUP);
            gcmRECORD_COUNTER(VPNC_SECULLTRIANGLECOUNT, gcmGETCOUNTER(counters_part1.se_culled_triangle_count));
            gcmRECORD_COUNTER(VPNC_SECULLLINECOUNT, gcmGETCOUNTER(counters_part1.se_culled_lines_count));
            gcmRECORD_COUNTER(VPNC_SECLIPTRIANGLECOUNT, gcmGETCOUNTER(counters_part1.se_clipped_triangle_count));
            gcmRECORD_COUNTER(VPNC_SECLIPLINECOUNT, gcmGETCOUNTER(counters_part1.se_clipped_line_count));
            gcmRECORD_COUNTER(VPNC_SESTARVECOUNT, gcmGETCOUNTER(counters_part1.se_starve_count));
            gcmRECORD_COUNTER(VPNC_SESTALLCOUNT, gcmGETCOUNTER(counters_part1.se_stall_count));
            gcmRECORD_COUNTER(VPNC_SERECEIVETRIANGLECOUNT, gcmGETCOUNTER(counters_part1.se_receive_triangle_count));
            gcmRECORD_COUNTER(VPNC_SESENDTRIANGLECOUNT, gcmGETCOUNTER(counters_part1.se_send_triangle_count));
            gcmRECORD_COUNTER(VPNC_SERECEIVELINESCOUNT, gcmGETCOUNTER(counters_part1.se_receive_lines_count));
            gcmRECORD_COUNTER(VPNC_SESENDLINESCOUNT, gcmGETCOUNTER(counters_part1.se_send_lines_count));
            gcmRECORD_COUNTER(VPNC_SEPROCESSCOUNT, gcmGETCOUNTER(counters_part1.se_process_count));
            gcmRECORD_COUNTER(VPNC_SETRIVIALREJLINECOUNT, gcmGETCOUNTER(counters_part1.se_trivial_rejected_line_count));
            gcmRECORD_COUNTER(VPNC_SENONIDLESTARVECOUNT, gcmGETCOUNTER(counters_part1.se_non_idle_starve_count));
            gcmRECORD_CONST(VPG_END);

            gcmRECORD_CONST(VPNG_RA);
            gcmRECORD_COUNTER(VPNC_RAINPUTPRIMCOUNT, gcmGETCOUNTER(counters_part1.ra_input_prim_count));
            gcmRECORD_COUNTER(VPNC_RATOTALQUADCOUNT, gcmGETCOUNTER(counters_part1.ra_total_quad_count));
            gcmRECORD_COUNTER(VPNC_RAPIPECACHEMISSCOUNT, gcmGETCOUNTER(counters_part1.ra_pipe_cache_miss_counter));
            gcmRECORD_COUNTER(VPNC_RAPIPEHZCACHEMISSCOUNT, gcmGETCOUNTER(counters_part1.ra_pipe_hz_cache_miss_counter));
            gcmRECORD_COUNTER(VPNC_RAVALIDQUADCOUNTEZ, gcmGETCOUNTER(counters_part1.ra_valid_quad_count_after_early_z));
            gcmRECORD_COUNTER(VPNC_RAVALIDPIXCOUNT, gcmGETCOUNTER(counters_part1.ra_valid_pixel_count_to_render));
            gcmRECORD_COUNTER(VPNC_RAOUTPUTQUADCOUNT, gcmGETCOUNTER(counters_part1.ra_output_valid_quad_count));
            gcmRECORD_COUNTER(VPNC_RAOUTPUTPIXELCOUNT, gcmGETCOUNTER(counters_part1.ra_output_valid_pixel_count));
            gcmRECORD_COUNTER(VPNC_RAPREFCACHEMISSCOUNT, gcmGETCOUNTER(counters_part1.ra_prefetch_cache_miss_counter));
            gcmRECORD_COUNTER(VPNC_RAPREFHZCACHEMISSCOUNT, gcmGETCOUNTER(counters_part1.ra_prefetch_hz_cache_miss_counter));
            gcmRECORD_COUNTER(VPNC_RAEEZCULLCOUNT, gcmGETCOUNTER(counters_part1.ra_eez_culled_counter));
            gcmRECORD_COUNTER(VPNC_RANONIDLESTARVECOUNT, gcmGETCOUNTER(counters_part1.ra_non_idle_starve_count));
            gcmRECORD_COUNTER(VPNC_RASTARVELCOUNT, gcmGETCOUNTER(counters_part1.ra_starve_count));
            gcmRECORD_COUNTER(VPNC_RASTALLCOUNT, gcmGETCOUNTER(counters_part1.ra_stall_count));
            gcmRECORD_COUNTER(VPNC_RAPROCESSCOUNT, gcmGETCOUNTER(counters_part1.ra_process_count));
            gcmRECORD_CONST(VPG_END);

            gcmRECORD_CONST(VPNG_TX);
            gcmRECORD_COUNTER(VPNC_TXTOTBILINEARREQ, gcmGETCOUNTER(counters_part1.tx_total_bilinear_requests));
            gcmRECORD_COUNTER(VPNC_TXTOTTRILINEARREQ, gcmGETCOUNTER(counters_part1.tx_total_trilinear_requests));
            gcmRECORD_COUNTER(VPNC_TXTOTDISCARDTEXREQ, gcmGETCOUNTER(counters_part1.tx_total_discarded_texture_requests));
            gcmRECORD_COUNTER(VPNC_TXTOTTEXREQ, gcmGETCOUNTER(counters_part1.tx_total_texture_requests));
            gcmRECORD_COUNTER(VPNC_TXMC0MISSCOUNT, gcmGETCOUNTER(counters_part1.tx_mc0_miss_count));
            gcmRECORD_COUNTER(VPNC_TXMC0REQCOUNT, gcmGETCOUNTER(counters_part1.tx_mc0_request_byte_count));
            gcmRECORD_COUNTER(VPNC_TXMC1MISSCOUNT, gcmGETCOUNTER(counters_part1.tx_mc1_miss_count));
            gcmRECORD_COUNTER(VPNC_TXMC1REQCOUNT, gcmGETCOUNTER(counters_part1.tx_mc1_request_byte_count));
            gcmRECORD_CONST(VPG_END);

            gcmRECORD_CONST(VPNG_PS);
            if (!Profiler->probeMode && !Profiler->bHalti4)
            {
                /*this counter only recode on the first shader core, so just multi shaderCoreCount here*/
                counters->counters_part1.ps_inst_counter *= Profiler->shaderCoreCount;
                if (!Profiler->psRenderPixelFix)
                {
                    /* this counter is not correct on gc2000 510_rc8, so set the value invalid here*/
                    counters->counters_part1.ps_rendered_pixel_counter = 0xdeaddead;
                }
                else
                {
                    /*this counter will caculate twice on each shader core, so need divide by 2 for each shader core*/
                    counters->counters_part1.ps_rendered_pixel_counter = counters->counters_part1.ps_rendered_pixel_counter * (Profiler->shaderCoreCount / 2);
                }
            }

            gcmRECORD_COUNTER(VPNC_PSINSTCOUNT, gcmGETCOUNTER(counters_part1.ps_inst_counter));
            gcmRECORD_COUNTER(VPNC_PSRENDEREDPIXCOUNT, gcmGETCOUNTER(counters_part1.ps_rendered_pixel_counter));
            gcmRECORD_COUNTER(VPNC_PSBRANCHINSTCOUNT, gcmGETCOUNTER(counters_part1.ps_branch_inst_counter));
            gcmRECORD_COUNTER(VPNC_PSTEXLDINSTCOUNT, gcmGETCOUNTER(counters_part1.ps_texld_inst_counter));
            gcmRECORD_COUNTER(VPNC_PSNONIDLESTARVECOUNT, gcmGETCOUNTER(counters_part1.ps_non_idle_starve_count));
            gcmRECORD_COUNTER(VPNC_PSSTARVELCOUNT, gcmGETCOUNTER(counters_part1.ps_starve_count));
            gcmRECORD_COUNTER(VPNC_PSSTALLCOUNT, gcmGETCOUNTER(counters_part1.ps_stall_count));
            gcmRECORD_COUNTER(VPNC_PSPROCESSCOUNT, gcmGETCOUNTER(counters_part1.ps_process_count));
            gcmRECORD_COUNTER(VPNC_PSSHADERCYCLECOUNT, gcmGETCOUNTER(counters_part1.ps_shader_cycle_count));
            gcmRECORD_CONST(VPG_END);

            gcmRECORD_CONST(VPNG_PE);
            gcmRECORD_COUNTER(VPNC_PE0KILLEDBYCOLOR, gcmGETCOUNTER(counters_part1.pe0_pixel_count_killed_by_color_pipe));
            gcmRECORD_COUNTER(VPNC_PE0KILLEDBYDEPTH, gcmGETCOUNTER(counters_part1.pe0_pixel_count_killed_by_depth_pipe));
            gcmRECORD_COUNTER(VPNC_PE0DRAWNBYCOLOR, gcmGETCOUNTER(counters_part1.pe0_pixel_count_drawn_by_color_pipe));
            gcmRECORD_COUNTER(VPNC_PE0DRAWNBYDEPTH, gcmGETCOUNTER(counters_part1.pe0_pixel_count_drawn_by_depth_pipe));
            gcmRECORD_COUNTER(VPNC_PE1KILLEDBYCOLOR, gcmGETCOUNTER(counters_part1.pe1_pixel_count_killed_by_color_pipe));
            gcmRECORD_COUNTER(VPNC_PE1KILLEDBYDEPTH, gcmGETCOUNTER(counters_part1.pe1_pixel_count_killed_by_depth_pipe));
            gcmRECORD_COUNTER(VPNC_PE1DRAWNBYCOLOR, gcmGETCOUNTER(counters_part1.pe1_pixel_count_drawn_by_color_pipe));
            gcmRECORD_COUNTER(VPNC_PE1DRAWNBYDEPTH, gcmGETCOUNTER(counters_part1.pe1_pixel_count_drawn_by_depth_pipe));
            gcmRECORD_CONST(VPG_END);

            gcmRECORD_CONST(VPNG_MCC);
            gcmRECORD_COUNTER(VPNC_MCCREADREQ8BCOLORPIPE, gcmGETCOUNTER(counters_part2.mcc_total_read_req_8B_from_colorpipe));
            gcmRECORD_COUNTER(VPNC_MCCREADREQ8BSOCOLORPIPE, gcmGETCOUNTER(counters_part2.mcc_total_read_req_8B_sentout_from_colorpipe));
            gcmRECORD_COUNTER(VPNC_MCCWRITEREQ8BCOLORPIPE, gcmGETCOUNTER(counters_part2.mcc_total_write_req_8B_from_colorpipe));
            gcmRECORD_COUNTER(VPNC_MCCREADREQSOCOLORPIPE, gcmGETCOUNTER(counters_part2.mcc_total_read_req_sentout_from_colorpipe));
            gcmRECORD_COUNTER(VPNC_MCCWRITEREQCOLORPIPE, gcmGETCOUNTER(counters_part2.mcc_total_write_req_from_colorpipe));
            gcmRECORD_COUNTER(VPNC_MCCREADREQ8BDEPTHPIPE, gcmGETCOUNTER(counters_part2.mcc_total_read_req_8B_from_depthpipe));
            gcmRECORD_COUNTER(VPNC_MCCREADREQ8BSFDEPTHPIPE, gcmGETCOUNTER(counters_part2.mcc_total_read_req_8B_sentout_from_depthpipe));
            gcmRECORD_COUNTER(VPNC_MCCWRITEREQ8BDEPTHPIPE, gcmGETCOUNTER(counters_part2.mcc_total_write_req_8B_from_depthpipe));
            gcmRECORD_COUNTER(VPNC_MCCREADREQSFDEPTHPIPE, gcmGETCOUNTER(counters_part2.mcc_total_read_req_sentout_from_depthpipe));
            gcmRECORD_COUNTER(VPNC_MCCWRITEREQDEPTHPIPE, gcmGETCOUNTER(counters_part2.mcc_total_write_req_from_depthpipe));
            gcmRECORD_COUNTER(VPNC_MCCREADREQ8BOTHERPIPE, gcmGETCOUNTER(counters_part2.mcc_total_read_req_8B_from_others));
            gcmRECORD_COUNTER(VPNC_MCCWRITEREQ8BOTHERPIPE, gcmGETCOUNTER(counters_part2.mcc_total_write_req_8B_from_others));
            gcmRECORD_COUNTER(VPNC_MCCREADREQOTHERPIPE, gcmGETCOUNTER(counters_part2.mcc_total_read_req_from_others));
            gcmRECORD_COUNTER(VPNC_MCCWRITEREQOTHERPIPE, gcmGETCOUNTER(counters_part2.mcc_total_write_req_from_others));
            gcmRECORD_COUNTER(VPNC_MCCAXIMINLATENCY, counters->counters_part2.mcc_axi_min_latency);
            gcmRECORD_COUNTER(VPNC_MCCAXIMAXLATENCY, counters->counters_part2.mcc_axi_max_latency);
            gcmRECORD_COUNTER(VPNC_MCCAXITOTALLATENCY, gcmGETCOUNTER(counters_part2.mcc_axi_total_latency));
            gcmRECORD_COUNTER(VPNC_MCCAXISAMPLECOUNT, gcmGETCOUNTER(counters_part2.mcc_axi_sample_count));

            if (!Profiler->probeMode)
            {
                if (Profiler->axiBus128bits)
                {
                    counters->counters_part2.mc_fe_read_bandwidth *= 2;
                    counters->counters_part2.mc_mmu_read_bandwidth *= 2;
                    counters->counters_part2.mc_blt_read_bandwidth *= 2;
                    counters->counters_part2.mc_sh0_read_bandwidth *= 2;
                    counters->counters_part2.mc_sh1_read_bandwidth *= 2;
                    counters->counters_part2.mc_pe_write_bandwidth *= 2;
                    counters->counters_part2.mc_blt_write_bandwidth *= 2;
                    counters->counters_part2.mc_sh0_write_bandwidth *= 2;
                    counters->counters_part2.mc_sh1_write_bandwidth *= 2;
                }
            }
            else
            {
                counters->counters_part2.mc_fe_read_bandwidth = 0xdeaddead;
                counters->counters_part2.mc_mmu_read_bandwidth = 0xdeaddead;
                counters->counters_part2.mc_blt_read_bandwidth = 0xdeaddead;
                counters->counters_part2.mc_sh0_read_bandwidth = 0xdeaddead;
                counters->counters_part2.mc_sh1_read_bandwidth = 0xdeaddead;
                counters->counters_part2.mc_pe_write_bandwidth = 0xdeaddead;
                counters->counters_part2.mc_blt_write_bandwidth = 0xdeaddead;
                counters->counters_part2.mc_sh0_write_bandwidth = 0xdeaddead;
                counters->counters_part2.mc_sh1_write_bandwidth = 0xdeaddead;
            }
            gcmRECORD_COUNTER(VPNC_MCCFEREADBANDWIDTH, gcmGETCOUNTER(counters_part2.mc_fe_read_bandwidth));
            gcmRECORD_COUNTER(VPNC_MCCMMUREADBANDWIDTH, gcmGETCOUNTER(counters_part2.mc_mmu_read_bandwidth));
            gcmRECORD_COUNTER(VPNC_MCCBLTREADBANDWIDTH, gcmGETCOUNTER(counters_part2.mc_blt_read_bandwidth));
            gcmRECORD_COUNTER(VPNC_MCCSH0READBANDWIDTH, gcmGETCOUNTER(counters_part2.mc_sh0_read_bandwidth));
            gcmRECORD_COUNTER(VPNC_MCCSH1READBANDWIDTH, gcmGETCOUNTER(counters_part2.mc_sh1_read_bandwidth));
            gcmRECORD_COUNTER(VPNC_MCCPEWRITEBANDWIDTH, gcmGETCOUNTER(counters_part2.mc_pe_write_bandwidth));
            gcmRECORD_COUNTER(VPNC_MCCBLTWRITEBANDWIDTH, gcmGETCOUNTER(counters_part2.mc_blt_write_bandwidth));
            gcmRECORD_COUNTER(VPNC_MCCSH0WRITEBANDWIDTH, gcmGETCOUNTER(counters_part2.mc_sh0_write_bandwidth));
            gcmRECORD_COUNTER(VPNC_MCCSH1WRITEBANDWIDTH, gcmGETCOUNTER(counters_part2.mc_sh1_write_bandwidth));
            gcmRECORD_CONST(VPG_END);

            gcmRECORD_CONST(VPNG_MCZ);
            gcmRECORD_COUNTER(VPNC_MCZREADREQ8BCOLORPIPE, gcmGETCOUNTER(counters_part2.mcz_total_read_req_8B_from_colorpipe));
            gcmRECORD_COUNTER(VPNC_MCZREADREQ8BSOCOLORPIPE, gcmGETCOUNTER(counters_part2.mcz_total_read_req_8B_sentout_from_colorpipe));
            gcmRECORD_COUNTER(VPNC_MCZWRITEREQ8BCOLORPIPE, gcmGETCOUNTER(counters_part2.mcz_total_write_req_8B_from_colorpipe));
            gcmRECORD_COUNTER(VPNC_MCZREADREQSOCOLORPIPE, gcmGETCOUNTER(counters_part2.mcz_total_read_req_sentout_from_colorpipe));
            gcmRECORD_COUNTER(VPNC_MCZWRITEREQCOLORPIPE, gcmGETCOUNTER(counters_part2.mcz_total_write_req_from_colorpipe));
            gcmRECORD_COUNTER(VPNC_MCZREADREQ8BDEPTHPIPE, gcmGETCOUNTER(counters_part2.mcz_total_read_req_8B_from_depthpipe));
            gcmRECORD_COUNTER(VPNC_MCZREADREQ8BSFDEPTHPIPE, gcmGETCOUNTER(counters_part2.mcz_total_read_req_8B_sentout_from_depthpipe));
            gcmRECORD_COUNTER(VPNC_MCZWRITEREQ8BDEPTHPIPE, gcmGETCOUNTER(counters_part2.mcz_total_write_req_8B_from_depthpipe));
            gcmRECORD_COUNTER(VPNC_MCZREADREQSFDEPTHPIPE, gcmGETCOUNTER(counters_part2.mcz_total_read_req_sentout_from_depthpipe));
            gcmRECORD_COUNTER(VPNC_MCZWRITEREQDEPTHPIPE, gcmGETCOUNTER(counters_part2.mcz_total_write_req_from_depthpipe));
            gcmRECORD_COUNTER(VPNC_MCZREADREQ8BOTHERPIPE, gcmGETCOUNTER(counters_part2.mcz_total_read_req_8B_from_others));
            gcmRECORD_COUNTER(VPNC_MCZWRITEREQ8BOTHERPIPE, gcmGETCOUNTER(counters_part2.mcz_total_write_req_8B_from_others));
            gcmRECORD_COUNTER(VPNC_MCZREADREQOTHERPIPE, gcmGETCOUNTER(counters_part2.mcz_total_read_req_from_others));
            gcmRECORD_COUNTER(VPNC_MCZWRITEREQOTHERPIPE, gcmGETCOUNTER(counters_part2.mcz_total_write_req_from_others));
            gcmRECORD_COUNTER(VPNC_MCZAXIMINLATENCY, counters->counters_part2.mcz_axi_min_latency);
            gcmRECORD_COUNTER(VPNC_MCZAXIMAXLATENCY, counters->counters_part2.mcz_axi_max_latency);
            gcmRECORD_COUNTER(VPNC_MCZAXITOTALLATENCY, gcmGETCOUNTER(counters_part2.mcz_axi_total_latency));
            gcmRECORD_COUNTER(VPNC_MCZAXISAMPLECOUNT, gcmGETCOUNTER(counters_part2.mcz_axi_sample_count));
            gcmRECORD_CONST(VPG_END);

            if (Profiler->probeMode)
            {
                /*the bandwidth counter need multiply by 2 when AXI bus is 128 bits*/
                if (Profiler->axiBus128bits)
                {
                    counters->counters_part2.hi0_total_read_8B_count *= 2;
                    counters->counters_part2.hi0_total_write_8B_count *= 2;
                    counters->counters_part2.hi1_total_read_8B_count *= 2;
                    counters->counters_part2.hi1_total_write_8B_count *= 2;
                }

                if (counters->counters_part2.hi0_total_read_8B_count == 0xdeaddead ||
                    counters->counters_part2.hi1_total_read_8B_count == 0xdeaddead ||
                    counters->counters_part2.hi0_total_write_8B_count == 0xdeaddead ||
                    counters->counters_part2.hi1_total_write_8B_count == 0xdeaddead)
                {
                    counters->counters_part2.hi_total_read_8B_count = 0xdeaddead;
                    counters->counters_part2.hi_total_write_8B_count = 0xdeaddead;
                }
                else
                {
                    counters->counters_part2.hi_total_read_8B_count = counters->counters_part2.hi0_total_read_8B_count + counters->counters_part2.hi1_total_read_8B_count;
                    counters->counters_part2.hi_total_write_8B_count = counters->counters_part2.hi0_total_write_8B_count + counters->counters_part2.hi1_total_write_8B_count;
                }

                counters->counters_part2.hi_total_readOCB_16B_count = 0xdeaddead;
                counters->counters_part2.hi_total_writeOCB_16B_count = 0xdeaddead;
            }
            /*non probe mode */
            else if (Profiler->axiBus128bits)
            {
                counters->counters_part2.hi_total_read_8B_count *= 2;
                counters->counters_part2.hi_total_write_8B_count *= 2;
            }

            gcmRECORD_CONST(VPNG_HI);
            gcmRECORD_COUNTER(VPNC_HI0READ8BYTE, gcmGETCOUNTER(counters_part2.hi0_total_read_8B_count));
            gcmRECORD_COUNTER(VPNC_HI0WRITE8BYTE, gcmGETCOUNTER(counters_part2.hi0_total_write_8B_count));
            gcmRECORD_COUNTER(VPNC_HI0READREQ, gcmGETCOUNTER(counters_part2.hi0_total_read_request_count));
            gcmRECORD_COUNTER(VPNC_HI0WRITEREQ, gcmGETCOUNTER(counters_part2.hi0_total_write_request_count));
            gcmRECORD_COUNTER(VPNC_HI0AXIREADREQSTALL, gcmGETCOUNTER(counters_part2.hi0_axi_cycles_read_request_stalled));
            gcmRECORD_COUNTER(VPNC_HI0AXIWRITEREQSTALL, gcmGETCOUNTER(counters_part2.hi0_axi_cycles_write_request_stalled));
            gcmRECORD_COUNTER(VPNC_HI0AXIWRITEDATASTALL, gcmGETCOUNTER(counters_part2.hi0_axi_cycles_write_data_stalled));
            gcmRECORD_COUNTER(VPNC_HI1READ8BYTE, gcmGETCOUNTER(counters_part2.hi1_total_read_8B_count));
            gcmRECORD_COUNTER(VPNC_HI1WRITE8BYTE, gcmGETCOUNTER(counters_part2.hi1_total_write_8B_count));
            gcmRECORD_COUNTER(VPNC_HI1READREQ, gcmGETCOUNTER(counters_part2.hi1_total_read_request_count));
            gcmRECORD_COUNTER(VPNC_HI1WRITEREQ, gcmGETCOUNTER(counters_part2.hi1_total_write_request_count));
            gcmRECORD_COUNTER(VPNC_HI1AXIREADREQSTALL, gcmGETCOUNTER(counters_part2.hi1_axi_cycles_read_request_stalled));
            gcmRECORD_COUNTER(VPNC_HI1AXIWRITEREQSTALL, gcmGETCOUNTER(counters_part2.hi1_axi_cycles_write_request_stalled));
            gcmRECORD_COUNTER(VPNC_HI1AXIWRITEDATASTALL, gcmGETCOUNTER(counters_part2.hi1_axi_cycles_write_data_stalled));
            gcmRECORD_COUNTER(VPNC_HITOTALCYCLES, gcmGETCOUNTER(counters_part2.hi_total_cycle_count));
            gcmRECORD_COUNTER(VPNC_HIIDLECYCLES, gcmGETCOUNTER(counters_part2.hi_total_idle_cycle_count));
            gcmRECORD_COUNTER(VPNC_HIREAD8BYTE, gcmGETCOUNTER(counters_part2.hi_total_read_8B_count));
            gcmRECORD_COUNTER(VPNC_HIWRITE8BYTE, gcmGETCOUNTER(counters_part2.hi_total_write_8B_count));
            gcmRECORD_COUNTER(VPNC_HIOCBREAD16BYTE, gcmGETCOUNTER(counters_part2.hi_total_readOCB_16B_count));
            gcmRECORD_COUNTER(VPNC_HIOCBWRITE16BYTE, gcmGETCOUNTER(counters_part2.hi_total_writeOCB_16B_count));
            gcmRECORD_CONST(VPG_END);

            gcmRECORD_CONST(VPNG_L2);
            gcmRECORD_COUNTER(VPNC_L2AXI0READREQCOUNT, gcmGETCOUNTER(counters_part2.l2_total_axi0_read_request_count));
            gcmRECORD_COUNTER(VPNC_L2AXI1READREQCOUNT, gcmGETCOUNTER(counters_part2.l2_total_axi1_read_request_count));
            gcmRECORD_COUNTER(VPNC_L2AXI0WRITEREQCOUNT, gcmGETCOUNTER(counters_part2.l2_total_axi0_write_request_count));
            gcmRECORD_COUNTER(VPNC_L2AXI1WRITEREQCOUNT, gcmGETCOUNTER(counters_part2.l2_total_axi1_write_request_count));
            gcmRECORD_COUNTER(VPNC_L2READTRANSREQBYAXI0, gcmGETCOUNTER(counters_part2.l2_total_read_transactions_request_by_axi0));
            gcmRECORD_COUNTER(VPNC_L2READTRANSREQBYAXI1, gcmGETCOUNTER(counters_part2.l2_total_read_transactions_request_by_axi1));
            gcmRECORD_COUNTER(VPNC_L2WRITETRANSREQBYAXI0, gcmGETCOUNTER(counters_part2.l2_total_write_transactions_request_by_axi0));
            gcmRECORD_COUNTER(VPNC_L2WRITETRANSREQBYAXI1, gcmGETCOUNTER(counters_part2.l2_total_write_transactions_request_by_axi1));
            gcmRECORD_COUNTER(VPNC_L2AXI0MINLATENCY, counters->counters_part2.l2_axi0_min_latency);
            gcmRECORD_COUNTER(VPNC_L2AXI0MAXLATENCY, counters->counters_part2.l2_axi0_max_latency);
            gcmRECORD_COUNTER(VPNC_L2AXI0TOTLATENCY, gcmGETCOUNTER(counters_part2.l2_axi0_total_latency));
            gcmRECORD_COUNTER(VPNC_L2AXI0TOTREQCOUNT, gcmGETCOUNTER(counters_part2.l2_axi0_total_request_count));
            gcmRECORD_COUNTER(VPNC_L2AXI1MINLATENCY, counters->counters_part2.l2_axi1_min_latency);
            gcmRECORD_COUNTER(VPNC_L2AXI1MAXLATENCY, counters->counters_part2.l2_axi1_max_latency);
            gcmRECORD_COUNTER(VPNC_L2AXI1TOTLATENCY, gcmGETCOUNTER(counters_part2.l2_axi1_total_latency));
            gcmRECORD_COUNTER(VPNC_L2AXI1TOTREQCOUNT, gcmGETCOUNTER(counters_part2.l2_axi1_total_request_count));
            gcmRECORD_CONST(VPG_END);

            if (Profiler->coreCount > 1)
            {
                gcmRECORD_CONST(VPG_END);
            }
        }
        if (opType == gcvCOUNTER_OP_DRAW  && Profiler->perDrawMode)
        {
            gcmRECORD_CONST(VPG_END);
        }
        else
        {
            if (Profiler->coreCount == 1)
            {
                gcmRECORD_CONST(VPG_END);
            }
        }

        gcoOS_Seek(gcvNULL, Profiler->file, Profiler->counterBuf->startPos, gcvFILE_SEEK_SET);
        gcmONERROR(gcoPROFILER_Write(Profiler, Profiler->counterBuf->dataSize, counterData));
        gcmOS_SAFE_FREE(gcvNULL, counterData);

    }

    if (Profiler->enablePrint)
    {
        for (coreId = 0; coreId < Profiler->coreCount; coreId++)
        {
            counters = &(Profiler->counterBuf->counters[coreId]);
            preCounters = &(Profiler->counterBuf->prev->counters[coreId]);

            if (Profiler->profilerClient == gcvCLIENT_OPENCL ||
                Profiler->profilerClient == gcvCLIENT_OPENVX)
            {
                if (Profiler->coreCount > 1)
                {
                    gcmPRINT("GPU #%d\n", coreId);
                }
                /* simplify the messages for vx demo */
                /* 0.00000095367 = 1 / 1024 / 1024*/
                gcmPRINT("TOTAL_READ_BANDWIDTH  (MByte): %f\n", counters->counters_part2.hi_total_read_8B_count * 8 * 0.00000095367);
                gcmPRINT("TOTAL_WRITE_BANDWIDTH (MByte): %f\n", counters->counters_part2.hi_total_write_8B_count * 8 * 0.00000095367);
                gcmPRINT("AXI_READ_BANDWIDTH  (MByte): %f\n", counters->counters_part2.hi_total_readOCB_16B_count * 16 * 0.00000095367);
                gcmPRINT("AXI_WRITE_BANDWIDTH (MByte): %f\n", counters->counters_part2.hi_total_writeOCB_16B_count * 16 * 0.00000095367);
                gcmPRINT("DDR_READ_BANDWIDTH (MByte): %f\n", counters->counters_part2.hi_total_read_8B_count * 8 * 0.00000095367 - counters->counters_part2.hi_total_readOCB_16B_count * 16 * 0.00000095367);
                gcmPRINT("DDR_WRITE_BANDWIDTH (MByte): %f\n", counters->counters_part2.hi_total_write_8B_count * 8 * 0.00000095367 - counters->counters_part2.hi_total_writeOCB_16B_count * 16 * 0.00000095367);

                gcmPRINT("GPUTOTALCYCLES: %u\n", gcmGETCOUNTER(counters_part2.hi_total_cycle_count));
                gcmPRINT("GPUIDLECYCLES: %u\n", gcmGETCOUNTER(counters_part2.hi_total_idle_cycle_count));
            }
            else
            {
                if (opType == gcvCOUNTER_OP_FINISH)
                {
                    gcmPRINT("VPG_GPU glFinish/glFlush ID:%d\n", opID);
                }
                else if (opType == gcvCOUNTER_OP_FRAME)
                {
                    gcmPRINT("VPG_GPU Frame ID:%d\n", opID);
                }
                if (Profiler->coreCount > 1)
                {
                    gcmPRINT("GPU #%d\n", coreId);
                }

                gcmPRINT("*********\n");

                gcmPRINT("VPG_GPU\n");
                gcmPRINT("VPC_GPU_READ_BANDWIDTH (Byte): %d\n", gcmGETCOUNTER(counters_part2.hi_total_read_8B_count) * 8);
                gcmPRINT("VPC_GPU_WRITE_BANDWIDTH (Byte): %d\n", gcmGETCOUNTER(counters_part2.hi_total_write_8B_count) * 8);
                gcmPRINT("VPC_GPUTOTALCYCLES: %d\n", gcmGETCOUNTER(counters_part2.hi_total_cycle_count));
                gcmPRINT("VPC_GPUIDLECYCLES: %d\n", gcmGETCOUNTER(counters_part2.hi_total_idle_cycle_count));
                gcmPRINT("*********\n");

                gcmPRINT("VPG_FE\n");
                gcmPRINT("VPC_FEDRAWCOUNT: %d\n", gcmGETCOUNTER(counters_part1.fe_draw_count));
                gcmPRINT("VPC_FEOUTVERTEXCOUNT: %d\n", gcmGETCOUNTER(counters_part1.fe_out_vertex_count));
                gcmPRINT("VPC_FESTALLCOUNT: %d\n", gcmGETCOUNTER(counters_part1.fe_stall_count));
                gcmPRINT("VPC_FESTARVECOUNT: %d\n", gcmGETCOUNTER(counters_part1.fe_starve_count));
                gcmPRINT("*********\n");

                gcmPRINT("VPG_VS\n");
                gcmPRINT("VPC_VSINSTCOUNT: %d\n", gcmGETCOUNTER(counters_part1.vs_inst_counter));
                gcmPRINT("VPC_VSBRANCHINSTCOUNT: %d\n", gcmGETCOUNTER(counters_part1.vs_branch_inst_counter));
                gcmPRINT("VPC_VSTEXLDINSTCOUNT: %d\n", gcmGETCOUNTER(counters_part1.vs_texld_inst_counter));
                gcmPRINT("VPC_VSRENDEREDVERTCOUNT: %d\n", gcmGETCOUNTER(counters_part1.vs_rendered_vertice_counter));
                gcmPRINT("VPC_VSNONIDLESTARVECOUNT: %d\n", gcmGETCOUNTER(counters_part1.vs_non_idle_starve_count));
                gcmPRINT("VPC_VSSTARVELCOUNT: %d\n", gcmGETCOUNTER(counters_part1.vs_starve_count));
                gcmPRINT("VPC_VSSTALLCOUNT: %d\n", gcmGETCOUNTER(counters_part1.vs_stall_count));
                gcmPRINT("VPC_VSPROCESSCOUNT: %d\n", gcmGETCOUNTER(counters_part1.vs_process_count));
                gcmPRINT("*********\n");

                gcmPRINT("VPG_PA\n");
                gcmPRINT("VPC_PAINVERTCOUNT: %d\n", gcmGETCOUNTER(counters_part1.pa_input_vtx_counter));
                gcmPRINT("VPC_PAINPRIMCOUNT: %d\n", gcmGETCOUNTER(counters_part1.pa_input_prim_counter));
                gcmPRINT("VPC_PAOUTPRIMCOUNT: %d\n", gcmGETCOUNTER(counters_part1.pa_output_prim_counter));
                gcmPRINT("VPC_PADEPTHCLIPCOUNT: %d\n", gcmGETCOUNTER(counters_part1.pa_depth_clipped_counter));
                gcmPRINT("VPC_PATRIVIALREJCOUNT: %d\n", gcmGETCOUNTER(counters_part1.pa_trivial_rejected_counter));
                gcmPRINT("VPC_PACULLCOUNT: %d\n", gcmGETCOUNTER(counters_part1.pa_culled_prim_counter));
                gcmPRINT("VPC_PANONIDLESTARVECOUNT: %d\n", gcmGETCOUNTER(counters_part1.pa_non_idle_starve_count));
                gcmPRINT("VPC_PASTARVELCOUNT: %d\n", gcmGETCOUNTER(counters_part1.pa_starve_count));
                gcmPRINT("VPC_PASTALLCOUNT: %d\n", gcmGETCOUNTER(counters_part1.pa_stall_count));
                gcmPRINT("VPC_PAPROCESSCOUNT: %d\n", gcmGETCOUNTER(counters_part1.pa_process_count));
                gcmPRINT("*********\n");

                gcmPRINT("VPG_SETUP\n");
                gcmPRINT("VPC_SETRIANGLECOUNT: %d\n", gcmGETCOUNTER(counters_part1.se_culled_triangle_count));
                gcmPRINT("VPC_SELINECOUNT: %d\n", gcmGETCOUNTER(counters_part1.se_culled_lines_count));
                gcmPRINT("VPC_SESTARVECOUNT: %d\n", gcmGETCOUNTER(counters_part1.se_starve_count));
                gcmPRINT("VPC_SESTALLCOUNT: %d\n", gcmGETCOUNTER(counters_part1.se_stall_count));
                gcmPRINT("VPC_SERECEIVETRIANGLECOUNT: %d\n", gcmGETCOUNTER(counters_part1.se_receive_triangle_count));
                gcmPRINT("VPC_SESENDTRIANGLECOUNT: %d\n", gcmGETCOUNTER(counters_part1.se_send_triangle_count));
                gcmPRINT("VPC_SERECEIVELINESCOUNT: %d\n", gcmGETCOUNTER(counters_part1.se_receive_lines_count));
                gcmPRINT("VPC_SESENDLINESCOUNT: %d\n", gcmGETCOUNTER(counters_part1.se_send_lines_count));
                gcmPRINT("VPC_SEPROCESSCOUNT: %d\n", gcmGETCOUNTER(counters_part1.se_process_count));
                gcmPRINT("VPC_SENONIDLESTARVECOUNT: %d\n", gcmGETCOUNTER(counters_part1.se_non_idle_starve_count));
                gcmPRINT("*********\n");

                gcmPRINT("VPG_RA\n");
                gcmPRINT("VPC_RAVALIDPIXCOUNT: %d\n", gcmGETCOUNTER(counters_part1.ra_valid_pixel_count_to_render));
                gcmPRINT("VPC_RATOTALQUADCOUNT: %d\n", gcmGETCOUNTER(counters_part1.ra_total_quad_count));
                gcmPRINT("VPC_RAVALIDQUADCOUNTEZ: %d\n", gcmGETCOUNTER(counters_part1.ra_valid_quad_count_after_early_z));
                gcmPRINT("VPC_RATOTALPRIMCOUNT: %d\n", gcmGETCOUNTER(counters_part1.ra_input_prim_count));
                gcmPRINT("VPC_RAPIPECACHEMISSCOUNT: %d\n", gcmGETCOUNTER(counters_part1.ra_pipe_cache_miss_counter));
                gcmPRINT("VPC_RAPREFCACHEMISSCOUNT: %d\n", gcmGETCOUNTER(counters_part1.ra_prefetch_cache_miss_counter));
                gcmPRINT("VPC_RAEEZCULLCOUNT: %d\n", gcmGETCOUNTER(counters_part1.ra_eez_culled_counter));
                gcmPRINT("VPC_RANONIDLESTARVECOUNT: %d\n", gcmGETCOUNTER(counters_part1.ra_non_idle_starve_count));
                gcmPRINT("VPC_RASTARVELCOUNT: %d\n", gcmGETCOUNTER(counters_part1.ra_starve_count));
                gcmPRINT("VPC_RASTALLCOUNT: %d\n", gcmGETCOUNTER(counters_part1.ra_stall_count));
                gcmPRINT("VPC_RAPROCESSCOUNT: %d\n", gcmGETCOUNTER(counters_part1.ra_process_count));
                gcmPRINT("*********\n");

                gcmPRINT("VPG_TX\n");
                gcmPRINT("VPC_TXTOTBILINEARREQ: %d\n", gcmGETCOUNTER(counters_part1.tx_total_bilinear_requests));
                gcmPRINT("VPC_TXTOTTRILINEARREQ: %d\n", gcmGETCOUNTER(counters_part1.tx_total_trilinear_requests));
                gcmPRINT("VPC_TXTOTTEXREQ: %d\n", gcmGETCOUNTER(counters_part1.tx_total_texture_requests));
                gcmPRINT("VPC_TXMC0CACHEMISSCOUNT: %d\n", gcmGETCOUNTER(counters_part1.tx_mc0_miss_count));
                gcmPRINT("VPC_TXMC1CACHEMISSCOUNT: %d\n", gcmGETCOUNTER(counters_part1.tx_mc1_miss_count));
                gcmPRINT("*********\n");

                gcmPRINT("VPG_PS\n");
                gcmPRINT("VPC_PSINSTCOUNT: %d\n", gcmGETCOUNTER(counters_part1.ps_inst_counter));
                gcmPRINT("VPC_PSBRANCHINSTCOUNT: %d\n", gcmGETCOUNTER(counters_part1.ps_branch_inst_counter));
                gcmPRINT("VPC_PSTEXLDINSTCOUNT: %d\n", gcmGETCOUNTER(counters_part1.ps_texld_inst_counter));
                gcmPRINT("VPC_PSRENDEREDPIXCOUNT: %d\n", gcmGETCOUNTER(counters_part1.ps_rendered_pixel_counter));
                gcmPRINT("VPC_PSNONIDLESTARVECOUNT: %d\n", gcmGETCOUNTER(counters_part1.ps_non_idle_starve_count));
                gcmPRINT("VPC_PSSTARVELCOUNT: %d\n", gcmGETCOUNTER(counters_part1.ps_starve_count));
                gcmPRINT("VPC_PSSTALLCOUNT: %d\n", gcmGETCOUNTER(counters_part1.ps_stall_count));
                gcmPRINT("VPC_PSPROCESSCOUNT: %d\n", gcmGETCOUNTER(counters_part1.ps_process_count));
                gcmPRINT("VPC_PSSHADERCYCLECOUNT: %d\n", gcmGETCOUNTER(counters_part1.ps_shader_cycle_count));
                gcmPRINT("*********\n");

                gcmPRINT("VPG_PE\n");
                gcmPRINT("VPC_PE0KILLEDBYCOLOR: %d\n", gcmGETCOUNTER(counters_part1.pe0_pixel_count_killed_by_color_pipe));
                gcmPRINT("VPC_PE0KILLEDBYDEPTH: %d\n", gcmGETCOUNTER(counters_part1.pe0_pixel_count_killed_by_depth_pipe));
                gcmPRINT("VPC_PE0DRAWNBYCOLOR: %d\n", gcmGETCOUNTER(counters_part1.pe0_pixel_count_drawn_by_color_pipe));
                gcmPRINT("VPC_PE0DRAWNBYDEPTH: %d\n", gcmGETCOUNTER(counters_part1.pe0_pixel_count_drawn_by_depth_pipe));
                gcmPRINT("VPC_PE1KILLEDBYCOLOR: %d\n", gcmGETCOUNTER(counters_part1.pe1_pixel_count_killed_by_color_pipe));
                gcmPRINT("VPC_PE1KILLEDBYDEPTH: %d\n", gcmGETCOUNTER(counters_part1.pe1_pixel_count_killed_by_depth_pipe));
                gcmPRINT("VPC_PE1DRAWNBYCOLOR: %d\n", gcmGETCOUNTER(counters_part1.pe1_pixel_count_drawn_by_color_pipe));
                gcmPRINT("VPC_PE1DRAWNBYDEPTH: %d\n", gcmGETCOUNTER(counters_part1.pe1_pixel_count_drawn_by_depth_pipe));
                gcmPRINT("*********\n");

                gcmPRINT("VPG_MC\n");
                gcmPRINT("VPC_MCAXIMINLATENCY: %d\n", counters->counters_part2.mcc_axi_min_latency);
                gcmPRINT("VPC_MCAXIMAXLATENCY: %d\n", counters->counters_part2.mcc_axi_max_latency);
                gcmPRINT("VPC_MCAXITOTALLATENCY: %d\n", gcmGETCOUNTER(counters_part2.mcc_axi_total_latency));
                gcmPRINT("VPC_MCAXISAMPLECOUNT: %d\n", gcmGETCOUNTER(counters_part2.mcc_axi_sample_count));
                gcmPRINT("*********\n");

                gcmPRINT("VPG_AXI\n");
                gcmPRINT("VPC_HI0AXIREADREQSTALLED: %d\n", gcmGETCOUNTER(counters_part2.hi0_axi_cycles_read_request_stalled));
                gcmPRINT("VPC_HI0AXIWRITEREQSTALLED: %d\n", gcmGETCOUNTER(counters_part2.hi0_axi_cycles_write_request_stalled));
                gcmPRINT("VPC_HI0AXIWRITEDATASTALLED: %d\n", gcmGETCOUNTER(counters_part2.hi0_axi_cycles_write_data_stalled));
                gcmPRINT("VPC_HI1AXIREADREQSTALLED: %d\n", gcmGETCOUNTER(counters_part2.hi1_axi_cycles_read_request_stalled));
                gcmPRINT("VPC_HI1AXIWRITEREQSTALLED: %d\n", gcmGETCOUNTER(counters_part2.hi1_axi_cycles_write_request_stalled));
                gcmPRINT("VPC_HI1AXIWRITEDATASTALLED: %d\n", gcmGETCOUNTER(counters_part2.hi1_axi_cycles_write_data_stalled));
                gcmPRINT("*********\n");
            }
        }
    }

    Profiler->counterBuf->available = gcvTRUE;
OnError:
    if (counterData)
    {
        gcmOS_SAFE_FREE(gcvNULL, counterData);
    }
    return;

}

static gceSTATUS
_UpdateCounters(
    IN gcoPROFILER Profiler,
    IN gctBOOL clearCounters
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 bufferSize;

    gcmHEADER_ARG("Profiler=0x%x", Profiler);

    if (Profiler->probeMode)
    {
        gcmONERROR(gcoHARDWARE_SetProbeCmd(gcvNULL, gcvPROBECMD_END, Profiler->counterBuf->probeAddress, gcvNULL));

        gcmONERROR(gcoBUFOBJ_GetFence((gcoBUFOBJ)Profiler->counterBuf->couterBufobj, gcvFENCE_TYPE_READ));

        if (clearCounters)
        {
            /*reset probe counters*/
            gcmONERROR(gcoHARDWARE_SetProbeCmd(gcvNULL, gcvPROBECMD_BEGIN, Profiler->counterBuf->probeAddress, gcvNULL));
        }
    }
    else
    {
        gcsHAL_INTERFACE iface;
        gctUINT32 context;
        gctUINT32 coreId;
        gctUINT32 originalCoreIndex;

        gcoHAL_Commit(gcvNULL, gcvFALSE);

        gcmONERROR(gcoHAL_GetCurrentCoreIndex(gcvNULL, &originalCoreIndex));

        /* Set Register clear Flag. */
        iface.ignoreTLS = gcvFALSE;
        iface.command = gcvHAL_READ_PROFILER_REGISTER_SETTING;
        iface.u.SetProfilerRegisterClear.bclear = clearCounters;

        /* Call the kernel. */
        for (coreId = 0; coreId < Profiler->coreCount; coreId++)
        {
            gctUINT32 coreIndex;
            /* Convert coreID in this hardware to global core index. */
            gcmONERROR(gcoHARDWARE_QueryCoreIndex(gcvNULL, coreId, &coreIndex));
            /* Set it to TLS to find correct command queue. */
            gcmONERROR(gcoHAL_SetCoreIndex(gcvNULL, coreIndex));

            /* Call the kernel. */
            gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                IOCTL_GCHAL_INTERFACE,
                &iface, gcmSIZEOF(iface),
                &iface, gcmSIZEOF(iface)));
        }

        iface.ignoreTLS = gcvFALSE;
        iface.command = gcvHAL_READ_ALL_PROFILE_REGISTERS_PART1;

        /* Call the kernel. */
        for (coreId = 0; coreId < Profiler->coreCount; coreId++)
        {
            gctUINT32 coreIndex;
            /* Convert coreID in this hardware to global core index. */
            gcmONERROR(gcoHARDWARE_QueryCoreIndex(gcvNULL, coreId, &coreIndex));
            /* Set it to TLS to find correct command queue. */
            gcmONERROR(gcoHAL_SetCoreIndex(gcvNULL, coreIndex));

            gcmVERIFY_OK(gcoHARDWARE_GetContext(gcvNULL, &context));
            if (context != 0)
                iface.u.RegisterProfileData_part1.context = context;

            /* Call the kernel. */
            gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                IOCTL_GCHAL_INTERFACE,
                &iface, gcmSIZEOF(iface),
                &iface, gcmSIZEOF(iface)));
            Profiler->counterBuf->counters[coreId].counters_part1 = iface.u.RegisterProfileData_part1.Counters;
        }

        iface.ignoreTLS = gcvFALSE;
        iface.command = gcvHAL_READ_ALL_PROFILE_REGISTERS_PART2;

        /* Call the kernel. */
        for (coreId = 0; coreId < Profiler->coreCount; coreId++)
        {
            gctUINT32 coreIndex;
            /* Convert coreID in this hardware to global core index. */
            gcmONERROR(gcoHARDWARE_QueryCoreIndex(gcvNULL, coreId, &coreIndex));
            /* Set it to TLS to find correct command queue. */
            gcmONERROR(gcoHAL_SetCoreIndex(gcvNULL, coreIndex));

            gcmVERIFY_OK(gcoHARDWARE_GetContext(gcvNULL, &context));
            if (context != 0)
                iface.u.RegisterProfileData_part2.context = context;

            /* Call the kernel. */
            gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                IOCTL_GCHAL_INTERFACE,
                &iface, gcmSIZEOF(iface),
                &iface, gcmSIZEOF(iface)));
            Profiler->counterBuf->counters[coreId].counters_part2 = iface.u.RegisterProfileData_part2.Counters;
        }
        /* Restore core index in TLS. */
        gcmONERROR(gcoHAL_SetCoreIndex(gcvNULL, originalCoreIndex));
    }

    Profiler->counterBuf->available = gcvFALSE;

    /*caculate the buffer size of current buffer will used*/
    bufferSize = gcmSIZEOF(gctINT32) * (TOTAL_COUNTER_NUMBER + TOTAL_MODULE_NUMBER) * 2 * Profiler->coreCount;

    if (Profiler->counterBuf->opType == gcvCOUNTER_OP_DRAW  && Profiler->perDrawMode)
    {
        bufferSize += gcmSIZEOF(gctINT32) * 2 * 2;
    }
    else if (Profiler->coreCount == 1)
    {
        bufferSize += gcmSIZEOF(gctINT32) * 2;
    }

    if (Profiler->coreCount > 1)
    {
        bufferSize += gcmSIZEOF(gctINT32) * 2 * 2 * Profiler->coreCount;
    }

    gcoOS_GetPos(gcvNULL, Profiler->file, &(Profiler->counterBuf->startPos));
    Profiler->counterBuf->dataSize = bufferSize;
    Profiler->counterBuf->endPos = Profiler->counterBuf->startPos +
                                   Profiler->counterBuf->dataSize;
    gcoOS_Seek(gcvNULL, Profiler->file, Profiler->counterBuf->endPos, gcvFILE_SEEK_SET);

OnError:

    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoPROFILER_Construct(
    OUT gcoPROFILER * Profiler
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcoPROFILER profiler = gcvNULL;
    gctPOINTER pointer = gcvNULL;
    gceCHIPMODEL chipModel;
    gctUINT32 chipRevision;

    gcmHEADER();

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Profiler != gcvNULL);

    /* Allocate the gcoPROFILER object structure. */
    gcmONERROR(gcoOS_Allocate(gcvNULL,
        gcmSIZEOF(struct _gcoPROFILER),
        &pointer));


    profiler = pointer;

    profiler->enable = gcvFALSE;
    profiler->probeMode = gcvFALSE;
    profiler->file = gcvNULL;
    profiler->perDrawMode = gcvFALSE;
    profiler->needDump = gcvFALSE;
    profiler->counterEnable = gcvFALSE;
    profiler->enablePrint = gcvFALSE;
    profiler->disableProbe = gcvFALSE;
    profiler->fileName = DEFAULT_PROFILE_FILE_NAME;
    profiler->profilerClient = 0;
    profiler->counterBuf = gcvNULL;
    profiler->bufferCount = NumOfPerFrameBuf;

    gcmONERROR(gcoHARDWARE_Query3DCoreCount(gcvNULL, &profiler->coreCount));

    gcoHAL_QueryShaderCaps(gcvNULL,
        gcvNULL,
        gcvNULL,
        gcvNULL,
        gcvNULL,
        &profiler->shaderCoreCount,
        gcvNULL,
        gcvNULL,
        gcvNULL);

    profiler->bHalti4 = (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_HALTI4) == gcvSTATUS_TRUE);

    gcoHAL_QueryChipIdentity(gcvNULL, &chipModel, &chipRevision, gcvNULL, gcvNULL);

    if (chipModel == gcv2000 && chipRevision == 0x5108)
    {
        profiler->psRenderPixelFix = gcvFALSE;
    }
    else
    {
        profiler->psRenderPixelFix = gcvTRUE;
    }

    gcoHAL_QueryChipAxiBusWidth(&profiler->axiBus128bits);

    /* Return the gcoPROFILER object. */
    *Profiler = profiler;

    /* Success. */
    gcmFOOTER_ARG("*Profiler=0x%x", *Profiler);
    return gcvSTATUS_OK;

OnError:
    /* Free the allocated memory. */
    if (profiler != gcvNULL)
    {
        gcmOS_SAFE_FREE(gcvNULL, profiler);
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoPROFILER_Destroy(
    IN gcoPROFILER Profiler
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface;
    gcsCounterBuffer_PTR counterBuffer = Profiler->counterBuf;

    gcmHEADER_ARG("Profiler=0x%x", Profiler);

    gcmONERROR(gcoBUFOBJ_WaitFence((gcoBUFOBJ)counterBuffer->prev->couterBufobj, gcvFENCE_TYPE_READ));

    do
    {
        if (!Profiler->counterBuf->available)
        {
            _WriteCounters(Profiler);
        }

        Profiler->counterBuf = Profiler->counterBuf->next;
    }
    while (Profiler->counterBuf != counterBuffer);

    gcmONERROR(gcoPROFILER_Flush(Profiler));

    if (Profiler->file != gcvNULL)
    {
        gcmONERROR(gcoOS_Close(gcvNULL, Profiler->file));
    }

    while (Profiler->counterBuf != gcvNULL)
    {
        /* Get the head of the list. */
        counterBuffer = Profiler->counterBuf;

        /* Remove the head buffer from the list. */
        if (counterBuffer->next == counterBuffer)
        {
            Profiler->counterBuf = gcvNULL;
        }
        else
        {
            counterBuffer->prev->next =
            Profiler->counterBuf = counterBuffer->next;
            counterBuffer->next->prev = counterBuffer->prev;
        }

        gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, counterBuffer->counters));
        gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, counterBuffer));
    }

    /* disable profiler in kernel. */
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_SET_PROFILE_SETTING;
    iface.u.SetProfileSetting.enable = gcvFALSE;

    /* Call the kernel. */
    gcoOS_DeviceControl(gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, gcmSIZEOF(iface),
        &iface, gcmSIZEOF(iface));

    /* Free the gcoPROFILER structure. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Profiler));

OnError:
    /* Success. */
    gcmFOOTER();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoPROFILER_Initialize(
    IN gcoPROFILER Profiler
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface;
    gcsCounterBuffer_PTR counterBuffer = gcvNULL;
    gctUINT32 i = 0;

    gcmHEADER_ARG("Profiler=0x%x", Profiler);

    if (!gcmIS_SUCCESS(_SetProfiler(Profiler)))
    {
        goto OnError;
    }

    /*initialize the counter buffer list*/
    for (i = 0; i < Profiler->bufferCount; i++)
    {
        gcmONERROR(_AllocateCounters(Profiler, &counterBuffer));

        if (Profiler->counterBuf == gcvNULL)
        {
            Profiler->counterBuf = counterBuffer;
            counterBuffer->prev =
            counterBuffer->next = counterBuffer;
        }
        else
        {
            /* Add to the tail. */
            counterBuffer->prev = Profiler->counterBuf->prev;
            counterBuffer->next = Profiler->counterBuf;
            Profiler->counterBuf->prev->next = counterBuffer;
            Profiler->counterBuf->prev = counterBuffer;
        }
    }

    /*do profile in new way by probe*/
    if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_PROBE) && !Profiler->disableProbe)
    {
        gctUINT32 physicalAddress;
        gctPOINTER logicalAddress;
        gctUINT32 size;
        gctUINT32 clusterIDWidth = 0;
        gcoBUFOBJ counterBufobj;

        gcoHAL_ConfigPowerManagement(gcvTRUE);

        /* disable old profiler in kernel. */
        iface.ignoreTLS = gcvFALSE;
        iface.command = gcvHAL_SET_PROFILE_SETTING;
        iface.u.SetProfileSetting.enable = gcvFALSE;

        /* Call the kernel. */
        gcmONERROR(gcoOS_DeviceControl(gcvNULL,
            IOCTL_GCHAL_INTERFACE,
            &iface, gcmSIZEOF(iface),
            &iface, gcmSIZEOF(iface)));

        gcmONERROR(gcoHARDWARE_QueryCluster(gcvNULL, gcvNULL, gcvNULL, gcvNULL, &clusterIDWidth));

        size = gcmSIZEOF(gctUINT32) * TOTAL_PROBE_NUMBER * Profiler->coreCount * (gctUINT32)(1 << clusterIDWidth);

        counterBuffer = Profiler->counterBuf;

        do
        {
            gcmONERROR(gcoBUFOBJ_Construct(gcvNULL, gcvBUFOBJ_TYPE_GENERIC_BUFFER, &counterBufobj));
            gcmONERROR(gcoBUFOBJ_Upload(counterBufobj, gcvNULL, 0, size, gcvBUFOBJ_USAGE_STATIC_DRAW));
            gcoBUFOBJ_Lock(counterBufobj, &physicalAddress, &logicalAddress);

            counterBuffer->probeAddress = physicalAddress;
            counterBuffer->logicalAddress = logicalAddress;
            counterBuffer->couterBufobj = (gctHANDLE)counterBufobj;

            counterBuffer->opType = gcvCOUNTER_OP_NONE;
            counterBuffer = counterBuffer->next;
        }
        while (counterBuffer != Profiler->counterBuf);

        Profiler->probeMode = gcvTRUE;
    }
    /* do profiling in old way*/
    else
    {
        gctUINT32 coreId;
        gctUINT32 originalCoreIndex;

        /* enable profiler in kernel. */
        iface.ignoreTLS = gcvFALSE;
        iface.command = gcvHAL_SET_PROFILE_SETTING;
        iface.u.SetProfileSetting.enable = gcvTRUE;

        gcmONERROR(gcoHAL_GetCurrentCoreIndex(gcvNULL, &originalCoreIndex));

        for (coreId = 0; coreId < Profiler->coreCount; coreId++)
        {
            gctUINT32 coreIndex;
            /* Convert coreID in this hardware to global core index. */
            gcmONERROR(gcoHARDWARE_QueryCoreIndex(gcvNULL, coreId, &coreIndex));
            /* Set it to TLS to find correct command queue. */
            gcmONERROR(gcoHAL_SetCoreIndex(gcvNULL, coreIndex));

            gcoHAL_ConfigPowerManagement(gcvFALSE);

            /* Call the kernel. */
            gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                IOCTL_GCHAL_INTERFACE,
                &iface, gcmSIZEOF(iface),
                &iface, gcmSIZEOF(iface)));
        }
        /* Restore core index in TLS. */
        gcmONERROR(gcoHAL_SetCoreIndex(gcvNULL, originalCoreIndex));

        Profiler->probeMode = gcvFALSE;
    }

    Profiler->needDump = gcvTRUE;
    Profiler->enable = gcvTRUE;

    gcmWRITE_CONST(VPHEADER);
    gcmWRITE_BUFFER(4, VPHEADER_VERSION);
    if (Profiler->profilerClient <= gcvCLIENT_OPENGL)
    {
        gcmWRITE_BUFFER(2, VPFILETYPE_GL);
    }
    else
    {
        gcmWRITE_BUFFER(2, VPFILETYPE_CL);
    }

    /* Success. */
    gcmFOOTER_ARG("Profiler=0x%x", Profiler);
    return status;

OnError:

    Profiler->enable = gcvFALSE;
    /* Success. */
    gcmFOOTER_ARG("Profiler=0x%x", Profiler);
    return status;
}

gceSTATUS
gcoPROFILER_Disable(
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface;
    gcmHEADER();

    gcoHAL_ConfigPowerManagement(gcvTRUE);
    /* disable profiler in kernel. */
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_SET_PROFILE_SETTING;
    iface.u.SetProfileSetting.enable = gcvFALSE;

    /* Call the kernel. */
    status = gcoOS_DeviceControl(gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, gcmSIZEOF(iface),
        &iface, gcmSIZEOF(iface));

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoPROFILER_EnableCounters(
    IN gcoPROFILER Profiler,
    IN gceCOUNTER_OPTYPE operationType
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Profiler=0x%x operationType=%d", Profiler, operationType);

    if (!Profiler)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_NOT_SUPPORTED;
    }

    gcmASSERT(Profiler->enable);

    /* reset profiler counter */
    if (Profiler->counterEnable == gcvFALSE)
    {
        if (Profiler->probeMode)
        {
            /* enable hw profiler counter here because of cl maybe do begin in another context which hw counter did not enabled */
            gcmONERROR(gcoHARDWARE_EnableCounters(gcvNULL));

            gcmONERROR(gcoHARDWARE_SetProbeCmd(gcvNULL, gcvPROBECMD_BEGIN, Profiler->counterBuf->probeAddress, gcvNULL));
        }
        else
        {
            gcsHAL_INTERFACE iface;
            gctUINT32 context;
            gctUINT32 coreId;
            gctUINT32 originalCoreIndex;

            gcmONERROR(gcoHAL_Commit(gcvNULL, gcvFALSE));

            gcmONERROR(gcoHAL_GetCurrentCoreIndex(gcvNULL, &originalCoreIndex));

            iface.ignoreTLS = gcvFALSE;
            iface.command = gcvHAL_READ_ALL_PROFILE_REGISTERS_PART1;

            for (coreId = 0; coreId < Profiler->coreCount; coreId++)
            {
                gctUINT32 coreIndex;
                /* Convert coreID in this hardware to global core index. */
                gcmONERROR(gcoHARDWARE_QueryCoreIndex(gcvNULL, coreId, &coreIndex));
                /* Set it to TLS to find correct command queue. */
                gcmONERROR(gcoHAL_SetCoreIndex(gcvNULL, coreIndex));

                gcmVERIFY_OK(gcoHARDWARE_GetContext(gcvNULL, &context));
                if (context != 0)
                    iface.u.RegisterProfileData_part1.context = context;

                /* Call the kernel. */
                gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                    IOCTL_GCHAL_INTERFACE,
                    &iface, gcmSIZEOF(iface),
                    &iface, gcmSIZEOF(iface)));
            }

            iface.ignoreTLS = gcvFALSE;
            iface.command = gcvHAL_READ_ALL_PROFILE_REGISTERS_PART2;

            for (coreId = 0; coreId < Profiler->coreCount; coreId++)
            {
                gctUINT32 coreIndex;
                /* Convert coreID in this hardware to global core index. */
                gcmONERROR(gcoHARDWARE_QueryCoreIndex(gcvNULL, coreId, &coreIndex));
                /* Set it to TLS to find correct command queue. */
                gcmONERROR(gcoHAL_SetCoreIndex(gcvNULL, coreIndex));

                gcmVERIFY_OK(gcoHARDWARE_GetContext(gcvNULL, &context));
                if (context != 0)
                    iface.u.RegisterProfileData_part2.context = context;

                /* Call the kernel. */
                gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                    IOCTL_GCHAL_INTERFACE,
                    &iface, gcmSIZEOF(iface),
                    &iface, gcmSIZEOF(iface)));
            }
            /* Restore core index in TLS. */
            gcmONERROR(gcoHAL_SetCoreIndex(gcvNULL, originalCoreIndex));
        }

        Profiler->counterEnable = gcvTRUE;
    }

OnError:

    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoPROFILER_End(
    IN gcoPROFILER Profiler,
    IN gceCOUNTER_OPTYPE operationType,
    IN gctUINT32 OpID
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctBOOL clearCounters;

    gcmHEADER_ARG("Profiler=0x%x", Profiler);

    if (!Profiler)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_NOT_SUPPORTED;
    }

    gcmASSERT(Profiler->enable);

    /* for the counter which need print, should get the counter right now for each draw/frame.*/
    if (Profiler->enablePrint)
    {
        clearCounters = gcvTRUE;
        Profiler->counterBuf->opType = operationType;
        Profiler->counterBuf->opID = OpID;
        Profiler->counterBuf->needDump = Profiler->needDump;

        /*update the counters info of currrent buffer*/
        gcmONERROR(_UpdateCounters(Profiler, clearCounters));

        gcoBUFOBJ_WaitFence((gcoBUFOBJ)Profiler->counterBuf->couterBufobj, gcvFENCE_TYPE_READ);

        _WriteCounters(Profiler);

        Profiler->counterBuf->available = gcvTRUE;
    }
    else
    {
        if (!Profiler->counterBuf->available)
        {
            gctUINT32 tempPos;
            gcoOS_GetPos(gcvNULL, Profiler->file, &tempPos);

            gcoBUFOBJ_WaitFence((gcoBUFOBJ)Profiler->counterBuf->couterBufobj, gcvFENCE_TYPE_READ);

            _WriteCounters(Profiler);

            gcoOS_Seek(gcvNULL, Profiler->file, tempPos, gcvFILE_SEEK_SET);

            Profiler->counterBuf->available = gcvTRUE;
        }

        if (operationType == gcvCOUNTER_OP_FRAME || operationType == gcvCOUNTER_OP_FINISH)
        {
            clearCounters = gcvTRUE;
        }
        else
        {
            clearCounters = gcvFALSE;
        }

        Profiler->counterBuf->opType = operationType;
        Profiler->counterBuf->opID = OpID;
        Profiler->counterBuf->needDump = Profiler->needDump;

        /*update the counters info of currrent buffer*/
        gcmONERROR(_UpdateCounters(Profiler, clearCounters));
    }

    Profiler->counterBuf = Profiler->counterBuf->next;

OnError:

    gcmFOOTER_NO();
    return status;
}

/* Write data to profile file. */
gceSTATUS
gcoPROFILER_Write(
    IN gcoPROFILER Profiler,
    IN gctSIZE_T ByteCount,
    IN gctCONST_POINTER Data
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Profiler=0x%x ByteCount=%lu Data=0x%x", Profiler, ByteCount, Data);

    if (!Profiler)
    {
        gcmFOOTER();
        return gcvSTATUS_NOT_SUPPORTED;
    }
    /* Check if already destroyed. */
    if (Profiler->enable)
    {
        status = gcoOS_Write(gcvNULL,
            Profiler->file,
            ByteCount, Data);
    }

    gcmFOOTER();
    return status;
}

/* Flush data out. */
gceSTATUS
gcoPROFILER_Flush(
    IN gcoPROFILER Profiler
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Profiler=0x%x", Profiler);

    if (!Profiler)
    {
        gcmFOOTER();
        return gcvSTATUS_NOT_SUPPORTED;
    }
    if (Profiler->enable)
    {
        status = gcoOS_Flush(gcvNULL, Profiler->file);
    }

    gcmFOOTER();
    return status;
}
#endif /* VIVANTE_PROFILER */
#endif /* gcdENABLE_3D */
