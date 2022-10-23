/*
* Copyright (c) 2019 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in below
* which is part of this source code package.
*
* Description:
*/

// Copyright (C) 2019 Amlogic, Inc. All rights reserved.
//
// All information contained herein is Amlogic confidential.
//
// This software is provided to you pursuant to Software License
// Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
// used only in accordance with the terms of this agreement.
//
// Redistribution and use in source and binary forms, with or without
// modification is strictly prohibited without prior written permission
// from Amlogic.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "vpuapifunc.h"
#include "enc_regdefine.h"
#include "vpuerror.h"
#include "main_helper.h"
#include "debug.h"
#if defined(PLATFORM_NON_OS) || defined (PLATFORM_LINUX)
#include <getopt.h>
#endif

#if defined(PLATFORM_LINUX) || defined(PLATFORM_QNX) || defined(PLATFORM_NON_OS)
#include <sys/stat.h>
#include <unistd.h>
#endif

#define BIT_DUMMY_READ_GEN          0x06000000
#define BIT_READ_LATENCY            0x06000004
#define VP5_SET_READ_DELAY           0x01000000
#define VP5_SET_WRITE_DELAY          0x01000004
#define MAX_CODE_BUF_SIZE           (512*1024)

#ifdef PLATFORM_WIN32
#pragma warning(disable : 4996)     //!<< disable waring C4996: The POSIX name for this item is deprecated.
#endif

char* EncPicTypeStringH264[] = {
    "IDR/I",
    "P",
};

char* EncPicTypeStringMPEG4[] = {
    "I",
    "P",
};


static osal_mutex_t s_loadBalancerLock;
static Uint32       s_nextInstance;
static BOOL         s_enableLoadBalance;
static Uint32       s_instances[MAX_NUM_INSTANCE];
enum {
    LB_STATE_NONE,
    LB_STATE_RUNNING
};

void LoadBalancerInit(void)
{
    s_loadBalancerLock  = osal_mutex_create();
    s_nextInstance      = 0;
    s_enableLoadBalance = TRUE;
    osal_memset((void*)s_instances, 0x00, sizeof(s_instances));
}

void LoadBalancerRelease(void)
{
    if (TRUE == s_enableLoadBalance) {
        osal_mutex_destroy(s_loadBalancerLock);
    }
}

void LoadBalancerAddInstance(Uint32 instanceIndex)
{
    if (TRUE == s_enableLoadBalance) {
        osal_mutex_lock(s_loadBalancerLock);
        s_instances[instanceIndex] = LB_STATE_RUNNING;
        osal_mutex_unlock(s_loadBalancerLock);
    }
}

void LoadBalancerRemoveInstance(Uint32 instanceIndex)
{
    if (TRUE == s_enableLoadBalance) {
        osal_mutex_lock(s_loadBalancerLock);
        s_instances[instanceIndex] = LB_STATE_NONE;
        if (s_nextInstance == instanceIndex) {
            Uint32 i = instanceIndex;
            Uint32 count=0;
            while (MAX_NUM_INSTANCE > count) {
                i = (i+1) % MAX_NUM_INSTANCE;
                if (LB_STATE_RUNNING == s_instances[i]) {
                    s_nextInstance = i;
                    break;
                }
                count++;
            }
        }
        osal_mutex_unlock(s_loadBalancerLock);
    }
}

BOOL LoadBalancerGetMyTurn(Uint32 myInstance)
{
    BOOL   myTurn = TRUE;

    if (TRUE == s_enableLoadBalance) {
        osal_mutex_lock(s_loadBalancerLock);
        myTurn = (BOOL)(myInstance == s_nextInstance);
        osal_mutex_unlock(s_loadBalancerLock);
    }

    return myTurn;
}

void LoadBalancerSetNextTurn(void)
{
    if (TRUE == s_enableLoadBalance) {
        osal_mutex_lock(s_loadBalancerLock);
        do {
            s_nextInstance++;
            s_nextInstance %= MAX_NUM_INSTANCE;
        } while (LB_STATE_NONE == s_instances[s_nextInstance]);
        osal_mutex_unlock(s_loadBalancerLock);
    }
}

Uint32 randomSeed;
static BOOL initializedRandom;
Uint32 GetRandom(
    Uint32   start,
    Uint32   end
    )
{
    Uint32   range = end-start+1;

    if (randomSeed == 0) {
        randomSeed = (Uint32)time(NULL);
        VLOG(INFO, "======= RANDOM SEED: %08x ======\n", randomSeed);
    }
    if (initializedRandom == FALSE) {
        srand(randomSeed);
        initializedRandom = TRUE;
    }

    if (range == 0) {
        VLOG(ERR, "%s:%d RANGE IS 0\n", __FUNCTION__, __LINE__);
        return 0;
    }
    else {
        return ((rand()%range) + start);
    }
}

Int32 LoadFirmware(
    Int32       productId,
    Uint8**     retFirmware,
    Uint32*     retSizeInWord,
    const char* path
    )
{
    Int32       nread;
    Uint32      totalRead, allocSize, readSize = 1024*1024;
    Uint8*      firmware = NULL;
    osal_file_t fp;

    if ((fp=osal_fopen(path, "rb")) == NULL)
    {
        VLOG(ERR, "Failed to open %s\n", path);
        return -1;
    }

    totalRead = 0;
    if (PRODUCT_ID_VP_SERIES(productId))
    {
        firmware = (Uint8*)osal_malloc(readSize);
        allocSize = readSize;
        nread = 0;
        while (TRUE)
        {
            if (allocSize < (totalRead+readSize))
            {
                allocSize += 2*nread;
                firmware = (Uint8*)realloc(firmware, allocSize);
            }
            nread = osal_fread((void*)&firmware[totalRead], 1, readSize, fp);//lint !e613
            totalRead += nread;
            if (nread < (Int32)readSize)
                break;
        }
        *retSizeInWord = (totalRead+1)/2;
    }
    else
    {
        Uint16*     pusBitCode;
        pusBitCode = (Uint16 *)osal_malloc(MAX_CODE_BUF_SIZE);
        firmware   = (Uint8*)pusBitCode;
        if (pusBitCode)
        {
            int code;
            while (!osal_feof(fp) || totalRead < (MAX_CODE_BUF_SIZE/2)) {
                code = -1;
                if (fscanf(fp, "%x", &code) <= 0) {
                    /* matching failure or EOF */
                    break;
                }
                pusBitCode[totalRead++] = (Uint16)code;
            }
        }
        *retSizeInWord = totalRead;
    }

    osal_fclose(fp);

    *retFirmware   = firmware;

    return 0;
}


void PrintVpuVersionInfo(
    Uint32   core_idx
    )
{
    Uint32 version;
    Uint32 revision;
    Uint32 productId;

    VPU_GetVersionInfo(core_idx, &version, &revision, &productId);

    VLOG(INFO, "VPU coreNum : [%d]\n", core_idx);
    VLOG(INFO, "Firmware : CustomerCode: %04x | version : %d.%d.%d rev.%d\n",
        (Uint32)(version>>16), (Uint32)((version>>(12))&0x0f), (Uint32)((version>>(8))&0x0f), (Uint32)((version)&0xff), revision);
    VLOG(INFO, "Hardware : %04x\n", productId);
    VLOG(INFO, "API      : %d.%d.%d\n\n", API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);
}



void SetDefaultEncTestConfig(TestEncConfig* testConfig) {
    osal_memset(testConfig, 0, sizeof(TestEncConfig));

    testConfig->stdMode        = STD_HEVC;
    testConfig->frame_endian   = VPU_FRAME_ENDIAN;
    testConfig->stream_endian  = VPU_STREAM_ENDIAN;
    testConfig->source_endian  = VPU_SOURCE_ENDIAN;
    testConfig->mapType        = COMPRESSED_FRAME_MAP;
    testConfig->lineBufIntEn   = TRUE;
#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
    testConfig->srcReleaseIntEnable     = FALSE;
#endif
    testConfig->ringBufferEnable        = FALSE;
    testConfig->ringBufferWrapEnable    = FALSE;

}


static void Vp5DisplayEncodedInformation(
    EncHandle       handle,
    CodStd          codec,
    Uint32          frameNo,
    EncOutputInfo*  encodedInfo,
    Int32           srcEndFlag,
    Int32           srcFrameIdx,
    Int32           performance
    )
{
    QueueStatusInfo queueStatus;

    VPU_EncGiveCommand(handle, ENC_GET_QUEUE_STATUS, &queueStatus);

    if (encodedInfo == NULL) {
        if (performance == TRUE ) {
            VLOG(INFO, "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
            VLOG(INFO, "                                                           USEDSRC            | FRAME  |  HOST  |  PREP_S   PREP_E    PREP   |  PROCE_S   PROCE_E  PROCE  |  ENC_S    ENC_E     ENC    |\n");
            VLOG(INFO, "I     NO     T   RECON  RD_PTR   WR_PTR     BYTES  SRCIDX  IDX IDC      Vcore | CYCLE  |  TICK  |   TICK     TICK     CYCLE  |   TICK      TICK    CYCLE  |   TICK     TICK     CYCLE  | RQ IQ\n");
            VLOG(INFO, "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
        }
        else {
            VLOG(INFO, "---------------------------------------------------------------------------------------------------------------------------\n");
            VLOG(INFO, "                                                              USEDSRC         |                CYCLE\n");
            VLOG(INFO, "I     NO     T   RECON   RD_PTR    WR_PTR     BYTES  SRCIDX   IDX IDC   Vcore | FRAME PREPARING PROCESSING ENCODING | RQ IQ\n");
            VLOG(INFO, "---------------------------------------------------------------------------------------------------------------------------\n");
        }
    } else {
        if (performance == TRUE) {
            VLOG(INFO, "%02d %5d %5d %5d   %08x %08x %8x    %2d     %2d %08x    %2d  %8u %8u (%8u,%8u,%8u) (%8u,%8u,%8u) (%8u,%8u,%8u)   %d  %d\n",
                handle->instIndex, encodedInfo->encPicCnt, encodedInfo->picType, encodedInfo->reconFrameIndex, encodedInfo->rdPtr, encodedInfo->wrPtr,
                encodedInfo->bitstreamSize, (srcEndFlag == 1 ? -1 : srcFrameIdx), encodedInfo->encSrcIdx,
                encodedInfo->releaseSrcFlag,
                0,
                encodedInfo->frameCycle, encodedInfo->encHostCmdTick,
                encodedInfo->encPrepareStartTick, encodedInfo->encPrepareEndTick, encodedInfo->prepareCycle,
                encodedInfo->encProcessingStartTick, encodedInfo->encProcessingEndTick, encodedInfo->processing,
                encodedInfo->encEncodeStartTick, encodedInfo->encEncodeEndTick, encodedInfo->EncodedCycle,
                queueStatus.reportQueueCount, queueStatus.instanceQueueCount);
        }
        else {
            VLOG(INFO, "%02d %5d %5d %5d    %08x  %08x %8x     %2d     %2d %04x    %d  %8d %8d %8d %8d      %d %d\n",
                handle->instIndex, encodedInfo->encPicCnt, encodedInfo->picType, encodedInfo->reconFrameIndex, encodedInfo->rdPtr, encodedInfo->wrPtr,
                encodedInfo->bitstreamSize, (srcEndFlag == 1 ? -1 : srcFrameIdx), encodedInfo->encSrcIdx,
                encodedInfo->releaseSrcFlag,
                0,
                encodedInfo->frameCycle, encodedInfo->prepareCycle, encodedInfo->processing, encodedInfo->EncodedCycle,
                queueStatus.reportQueueCount, queueStatus.instanceQueueCount);
        }
    }
}
/*lint -esym(438, ap) */
void
    DisplayEncodedInformation(
    EncHandle      handle,
    CodStd         codec,
    Uint32         frameNo,
    EncOutputInfo* encodedInfo,
    ...
    )
{
    int srcEndFlag;
    int srcFrameIdx;
    int performance = FALSE;
    va_list         ap;

    switch (codec) {
    case STD_HEVC:
        va_start(ap, encodedInfo);
        srcEndFlag = va_arg(ap, Uint32);
        srcFrameIdx = va_arg(ap, Uint32);
        performance = va_arg(ap, Uint32);
        va_end(ap);
        Vp5DisplayEncodedInformation(handle, codec, frameNo, encodedInfo, srcEndFlag , srcFrameIdx, performance);
        break;
    case STD_AVC:
        if(handle->productId == PRODUCT_ID_521) {
            va_start(ap, encodedInfo);
            srcEndFlag = va_arg(ap, Uint32);
            srcFrameIdx = va_arg(ap, Uint32);
            performance = va_arg(ap, Uint32);
            va_end(ap);
            Vp5DisplayEncodedInformation(handle, codec, frameNo, encodedInfo, srcEndFlag , srcFrameIdx, performance);
        }
        break;
    default:
        break;
    }

    return;
}
/*lint +esym(438, ap) */


void replace_character(char* str,
    char  c,
    char  r)
{
    int i=0;
    int len;
    len = strlen(str);

    for(i=0; i<len; i++)
    {
        if (str[i] == c)
            str[i] = r;
    }
}



void ChangePathStyle(
    char *str
    )
{
}

BOOL AllocFBMemory(Uint32 coreIdx, vpu_buffer_t *pFbMem, FrameBuffer* pFb,Uint32 memSize, Uint32 memNum, Int32 memTypes, Int32 instIndex)
{
    Uint32 i =0;
    for (i = 0; i < memNum; i++) {
        pFbMem[i].size = memSize;
        if (vdi_allocate_dma_memory(coreIdx, &pFbMem[i], memTypes, instIndex) < 0) {
            VLOG(ERR, "fail to allocate src buffer\n");
            return FALSE;
        }
        pFb[i].bufY         = pFbMem[i].phys_addr;
        pFb[i].bufCb        = (PhysicalAddress) - 1;
        pFb[i].bufCr        = (PhysicalAddress) - 1;
        pFb[i].size         = memSize;
        pFb[i].updateFbInfo = TRUE;
    }
    return TRUE;
}
#if defined(_WIN32) || defined(__MSDOS__)
#define DOS_FILESYSTEM
#define IS_DIR_SEPARATOR(__c) ((__c == '/') || (__c == '\\'))
#else
/* UNIX style */
#define IS_DIR_SEPARATOR(__c) (__c == '/')
#endif

char* GetDirname(
    const char* path
    )
{
    int length;
    int i;
    char* upper_dir;

    if (path == NULL) return NULL;

    length = strlen(path);
    for (i=length-1; i>=0; i--) {
        if (IS_DIR_SEPARATOR(path[i])) break;
    }

    if (i<0) {
        upper_dir = strdup(".");
    } else {
        upper_dir = strdup(path);
        upper_dir[i] = 0;
    }

    return upper_dir;
}

char* GetBasename(
    const char* pathname
    )
{
    const char* base = NULL;
    const char* p    = pathname;

    if (p == NULL) {
        return NULL;
    }

#if defined(DOS_FILESYSTEM)
    if (isalpha((int)p[0]) && p[1] == ':') {
        p += 2;
    }
#endif

    for (base=p; *p; p++) {//lint !e443
        if (IS_DIR_SEPARATOR(*p)) {
            base = p+1;
        }
    }

    return (char*)base;
}

char* GetFileExtension(
    const char* filename
    )
{
    Int32      len;
    Int32      i;

    len = strlen(filename);
    for (i=len-1; i>=0; i--) {
        if (filename[i] == '.') {
            return (char*)&filename[i+1];
        }
    }

    return NULL;
}

void byte_swap(unsigned char* data, int len)
{
    Uint8 temp;
    Int32 i;

    for (i=0; i<len; i+=2) {
        temp      = data[i];
        data[i]   = data[i+1];
        data[i+1] = temp;
    }
}

void word_swap(unsigned char* data, int len)
{
    Uint16  temp;
    Uint16* ptr = (Uint16*)data;
    Int32   i, size = len/(int)sizeof(Uint16);

    for (i=0; i<size; i+=2) {
        temp      = ptr[i];
        ptr[i]   = ptr[i+1];
        ptr[i+1] = temp;
    }
}

void dword_swap(unsigned char* data, int len)
{
    Uint32  temp;
    Uint32* ptr = (Uint32*)data;
    Int32   i, size = len/(int)sizeof(Uint32);

    for (i=0; i<size; i+=2) {
        temp      = ptr[i];
        ptr[i]   = ptr[i+1];
        ptr[i+1] = temp;
    }
}

void lword_swap(unsigned char* data, int len)
{
    Uint64  temp;
    Uint64* ptr = (Uint64*)data;
    Int32   i, size = len/(int)sizeof(Uint64);

    for (i=0; i<size; i+=2) {
        temp      = ptr[i];
        ptr[i]   = ptr[i+1];
        ptr[i+1] = temp;
    }
}

BOOL IsEndOfFile(FILE* fp)
{
    BOOL  result = FALSE;
    Int32 idx  = 0;
    char  cTemp;

    // Check current fp pos
    if (osal_feof(fp) != 0) {
        result = TRUE;
    }

    // Check next fp pos
    // Ignore newline character
    do {
        cTemp = fgetc(fp);
        idx++;

        if (osal_feof(fp) != 0) {
            result = TRUE;
            break;
        }
    } while (cTemp == '\n' || cTemp == '\r');

    // Revert fp pos
    idx *= (-1);
    osal_fseek(fp, idx, SEEK_CUR);

    return result;
}

BOOL CalcYuvSize(
    Int32   format,
    Int32   picWidth,
    Int32   picHeight,
    Int32   cbcrInterleave,
    size_t  *lumaSize,
    size_t  *chromaSize,
    size_t  *frameSize,
    Int32   *bitDepth,
    Int32   *packedFormat,
    Int32   *yuv3p4b)
{
    Int32   temp_picWidth;
    Int32   chromaWidth = 0, chromaHeight = 0;

    if ( bitDepth != 0)
        *bitDepth = 0;
    if ( packedFormat != 0)
        *packedFormat = 0;
    if ( yuv3p4b != 0)
        *yuv3p4b = 0;

    if (!lumaSize || !chromaSize || !frameSize )
        return FALSE;

    switch (format)
    {
    case FORMAT_420:
        *lumaSize = picWidth * picHeight;
        *chromaSize = picWidth * picHeight / 2;
        *frameSize = picWidth * picHeight * 3 /2;
        break;
    case FORMAT_YUYV:
    case FORMAT_YVYU:
    case FORMAT_UYVY:
    case FORMAT_VYUY:
        if ( packedFormat != 0)
            *packedFormat = 1;
        *lumaSize = picWidth * picHeight;
        *chromaSize = picWidth * picHeight;
        *frameSize = *lumaSize + *chromaSize;
        break;
    case FORMAT_224:
        *lumaSize = picWidth * picHeight;
        *chromaSize = picWidth * picHeight;
        *frameSize = picWidth * picHeight * 4 / 2;
        break;
    case FORMAT_422:
        *lumaSize = picWidth * picHeight;
        *chromaSize = picWidth * picHeight;
        *frameSize = picWidth * picHeight * 4 / 2;
        break;
    case FORMAT_444:
        *lumaSize  = picWidth * picHeight;
        *chromaSize = picWidth * picHeight * 2;
        *frameSize = picWidth * picHeight * 3;
        break;
    case FORMAT_400:
        *lumaSize  = picWidth * picHeight;
        *chromaSize = 0;
        *frameSize = picWidth * picHeight;
        break;
    case FORMAT_422_P10_16BIT_MSB:
    case FORMAT_422_P10_16BIT_LSB:
        if ( bitDepth != NULL) {
            *bitDepth = 10;
        }
        *lumaSize = picWidth * picHeight * 2;
        *chromaSize = *lumaSize;
        *frameSize = *lumaSize + *chromaSize;
        break;
    case FORMAT_420_P10_16BIT_MSB:
    case FORMAT_420_P10_16BIT_LSB:
        if ( bitDepth != 0)
            *bitDepth = 10;
        *lumaSize = picWidth * picHeight * 2;
        chromaWidth = picWidth;
        chromaHeight = picHeight;
        if (picWidth & 0x01) {
            chromaWidth = chromaWidth+1;
        }
        if (picHeight & 0x01) {
            chromaHeight = chromaHeight+1;
        }
        *chromaSize = chromaWidth * chromaHeight;
        *frameSize = *lumaSize + *chromaSize;
        break;
    case FORMAT_YUYV_P10_16BIT_MSB:   // 4:2:2 10bit packed
    case FORMAT_YUYV_P10_16BIT_LSB:
    case FORMAT_YVYU_P10_16BIT_MSB:
    case FORMAT_YVYU_P10_16BIT_LSB:
    case FORMAT_UYVY_P10_16BIT_MSB:
    case FORMAT_UYVY_P10_16BIT_LSB:
    case FORMAT_VYUY_P10_16BIT_MSB:
    case FORMAT_VYUY_P10_16BIT_LSB:
        if ( bitDepth != 0)
            *bitDepth = 10;
        if ( packedFormat != 0)
            *packedFormat = 1;
        *lumaSize = picWidth * picHeight * 2;
        *chromaSize = picWidth * picHeight * 2;
        *frameSize = *lumaSize + *chromaSize;
        break;
    case FORMAT_420_P10_32BIT_MSB:
    case FORMAT_420_P10_32BIT_LSB:
        if ( bitDepth != 0)
            *bitDepth = 10;
        if ( yuv3p4b != 0)
            *yuv3p4b = 1;
        temp_picWidth = VPU_ALIGN32(picWidth);
        chromaWidth = ((VPU_ALIGN16(temp_picWidth/2*(1<<cbcrInterleave))+2)/3*4);
        if ( cbcrInterleave == 1)
        {
            *lumaSize = (temp_picWidth+2)/3*4 * picHeight;
            *chromaSize = chromaWidth * picHeight/2;
        } else {
            *lumaSize = (temp_picWidth+2)/3*4 * picHeight;
            *chromaSize = chromaWidth * picHeight/2*2;
        }
        *frameSize = *lumaSize + *chromaSize;
        break;
    case FORMAT_YUYV_P10_32BIT_MSB:
    case FORMAT_YUYV_P10_32BIT_LSB:
    case FORMAT_YVYU_P10_32BIT_MSB:
    case FORMAT_YVYU_P10_32BIT_LSB:
    case FORMAT_UYVY_P10_32BIT_MSB:
    case FORMAT_UYVY_P10_32BIT_LSB:
    case FORMAT_VYUY_P10_32BIT_MSB:
    case FORMAT_VYUY_P10_32BIT_LSB:
        if ( bitDepth != 0)
            *bitDepth = 10;
        if ( packedFormat != 0)
            *packedFormat = 1;
        if ( yuv3p4b != 0)
            *yuv3p4b = 1;
        *frameSize = ((picWidth*2)+2)/3*4 * picHeight;
        *lumaSize = *frameSize/2;
        *chromaSize = *frameSize/2;
        break;
    default:
        *frameSize = picWidth * picHeight * 3 / 2;
        VLOG(ERR, "%s:%d Not supported format(%d)\n", __FILE__, __LINE__, format);
        return FALSE;
    }
    return TRUE;
}

FrameBufferFormat GetPackedFormat (
    int srcBitDepth,
    int packedType,
    int p10bits,
    int msb)
{
    FrameBufferFormat format = FORMAT_YUYV;

    // default pixel format = P10_16BIT_LSB (p10bits = 16, msb = 0)
    if (srcBitDepth == 8) {

        switch(packedType) {
        case PACKED_YUYV:
            format = FORMAT_YUYV;
            break;
        case PACKED_YVYU:
            format = FORMAT_YVYU;
            break;
        case PACKED_UYVY:
            format = FORMAT_UYVY;
            break;
        case PACKED_VYUY:
            format = FORMAT_VYUY;
            break;
        default:
            format = FORMAT_ERR;
        }
    }
    else if (srcBitDepth == 10) {
        switch(packedType) {
        case PACKED_YUYV:
            if (p10bits == 16) {
                format = (msb == 0) ? FORMAT_YUYV_P10_16BIT_LSB : FORMAT_YUYV_P10_16BIT_MSB;
            }
            else if (p10bits == 32) {
                format = (msb == 0) ? FORMAT_YUYV_P10_32BIT_LSB : FORMAT_YUYV_P10_32BIT_MSB;
            }
            else {
                format = FORMAT_ERR;
            }
            break;
        case PACKED_YVYU:
            if (p10bits == 16) {
                format = (msb == 0) ? FORMAT_YVYU_P10_16BIT_LSB : FORMAT_YVYU_P10_16BIT_MSB;
            }
            else if (p10bits == 32) {
                format = (msb == 0) ? FORMAT_YVYU_P10_32BIT_LSB : FORMAT_YVYU_P10_32BIT_MSB;
            }
            else {
                format = FORMAT_ERR;
            }
            break;
        case PACKED_UYVY:
            if (p10bits == 16) {
                format = (msb == 0) ? FORMAT_UYVY_P10_16BIT_LSB : FORMAT_UYVY_P10_16BIT_MSB;
            }
            else if (p10bits == 32) {
                format = (msb == 0) ? FORMAT_UYVY_P10_32BIT_LSB : FORMAT_UYVY_P10_32BIT_MSB;
            }
            else {
                format = FORMAT_ERR;
            }
            break;
        case PACKED_VYUY:
            if (p10bits == 16) {
                format = (msb == 0) ? FORMAT_VYUY_P10_16BIT_LSB : FORMAT_VYUY_P10_16BIT_MSB;
            }
            else if (p10bits == 32) {
                format = (msb == 0) ? FORMAT_VYUY_P10_32BIT_LSB : FORMAT_VYUY_P10_32BIT_MSB;
            }
            else {
                format = FORMAT_ERR;
            }
            break;
        default:
            format = FORMAT_ERR;
        }
    }
    else {
        format = FORMAT_ERR;
    }

    return format;
}


BOOL SetupEncoderOpenParam(
    EncOpenParam*   param,
    TestEncConfig*  config,
    ENC_CFG*        encCfg
    )
{
    param->bitstreamFormat = config->stdMode;
    if (strlen(config->cfgFileName) != 0) {
        if (GetEncOpenParam(param, config, encCfg) == FALSE) {
            VLOG(ERR, "[ERROR] Failed to parse CFG file(GetEncOpenParam)\n");
            return FALSE;
        }
    }
    else {
        if (GetEncOpenParamDefault(param, config) == FALSE) {
            VLOG(ERR, "[ERROR] Failed to parse CFG file(GetEncOpenParamDefault)\n");
            return FALSE;
        }
    }
    if (0 == param->streamBufCount) param->streamBufCount = ENC_STREAM_BUF_COUNT;
    if (0 == param->streamBufSize)  param->streamBufSize  = ENC_STREAM_BUF_SIZE;

    if (strlen(config->cfgFileName) != 0) {
        if (param->srcBitDepth == 8) {
            config->srcFormat = FORMAT_420;
        }
        else if (param->srcBitDepth == 10) {
            if (config->srcFormat == FORMAT_420) {
                config->srcFormat = FORMAT_420_P10_16BIT_MSB; //set srcFormat in ParseArgumentAndSetTestConfig function
                return FALSE;
            }
        }
    }

    if (config->optYuvPath[0] != 0)
        strcpy(config->yuvFileName, config->optYuvPath);


    if (config->packedFormat >= PACKED_YUYV) {
        int p10bits = 0;
        FrameBufferFormat packedFormat;
        if (config->srcFormat == FORMAT_420_P10_16BIT_MSB || config->srcFormat == FORMAT_420_P10_16BIT_LSB )
            p10bits = 16;
        if (config->srcFormat == FORMAT_420_P10_32BIT_MSB || config->srcFormat == FORMAT_420_P10_32BIT_LSB )
            p10bits = 32;

        packedFormat = GetPackedFormat(param->srcBitDepth, config->packedFormat, p10bits, 1);
        if (packedFormat == -1) {
            VLOG(ERR, "[ERROR] Failed to GetPackedFormat\n");
            return FALSE;
        }
        param->srcFormat      = packedFormat;
        param->nv21           = 0;
        param->cbcrInterleave = 0;
    }
    else {
        param->srcFormat      = config->srcFormat;
        param->nv21           = config->nv21;
    }
    param->packedFormat   = config->packedFormat;
    param->cbcrInterleave = config->cbcrInterleave;
    param->frameEndian    = config->frame_endian;
    param->streamEndian   = config->stream_endian;
    param->sourceEndian   = config->source_endian;
    param->lineBufIntEn   = config->lineBufIntEn;
    param->coreIdx        = config->coreIdx;
    param->cbcrOrder      = CBCR_ORDER_NORMAL;
    param->lowLatencyMode = (config->lowLatencyMode&0x3);		// 2bits lowlatency mode setting. bit[1]: low latency interrupt enable, bit[0]: fast bitstream-packing enable.
    param->EncStdParam.vpParam.useLongTerm = (config->useAsLongtermPeriod > 0 && config->refLongtermPeriod > 0) ? 1 : 0;

    if (PRODUCT_ID_VP_SERIES(config->productId)) {
#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
        param->srcReleaseIntEnable    = config->srcReleaseIntEnable;
#endif
        param->ringBufferEnable       = config->ringBufferEnable;
        param->ringBufferWrapEnable   = config->ringBufferWrapEnable;
        if (config->ringBufferEnable == TRUE) {
            param->streamBufCount = 1;
            param->lineBufIntEn = FALSE;
        }

    }
    return TRUE;
}

void GenRegionToMap(
    VpuRect *region,        /**< The size of the ROI region for H.265 (start X/Y in CTU, end X/Y int CTU)  */
    int *roiQp,
    int num,
    Uint32 mapWidth,
    Uint32 mapHeight,
    Uint8 *roiCtuMap)
{
    Int32 roi_id, blk_addr;
    Uint32 roi_map_size      = mapWidth * mapHeight;

    //init roi map
    for (blk_addr=0; blk_addr<(Int32)roi_map_size; blk_addr++)
        roiCtuMap[blk_addr] = 0;

    //set roi map. roi_entry[i] has higher priority than roi_entry[i+1]
    for (roi_id=(Int32)num-1; roi_id>=0; roi_id--)
    {
        Uint32 x, y;
        VpuRect *roi = region + roi_id;

        for (y=roi->top; y<=roi->bottom; y++)
        {
            for (x=roi->left; x<=roi->right; x++)
            {
                roiCtuMap[y*mapWidth + x] = *(roiQp + roi_id);
            }
        }
    }
}


void GenRegionToQpMap(
    VpuRect *region,        /**< The size of the ROI region for H.265 (start X/Y in CTU, end X/Y int CTU)  */
    int *roiQp,
    int num,
    int initQp,
    Uint32 mapWidth,
    Uint32 mapHeight,
    Uint8 *roiCtuMap)
{
    Int32 roi_id, blk_addr;
    Uint32 roi_map_size      = mapWidth * mapHeight;

    //init roi map
    for (blk_addr=0; blk_addr<(Int32)roi_map_size; blk_addr++)
        roiCtuMap[blk_addr] = initQp;

    //set roi map. roi_entry[i] has higher priority than roi_entry[i+1]
    for (roi_id=(Int32)num-1; roi_id>=0; roi_id--)
    {
        Uint32 x, y;
        VpuRect *roi = region + roi_id;

        for (y=roi->top; y<=roi->bottom; y++)
        {
            for (x=roi->left; x<=roi->right; x++)
            {
                roiCtuMap[y*mapWidth + x] = *(roiQp + roi_id);
            }
        }
    }
}





int openRoiMapFile(TestEncConfig *encConfig)
{
    if (encConfig->roi_enable) {
        if (encConfig->roi_file_name) {
            ChangePathStyle(encConfig->roi_file_name);
            if ((encConfig->roi_file = osal_fopen(encConfig->roi_file_name, "r")) == NULL) {
                VLOG(ERR, "fail to open ROI file, %s\n", encConfig->roi_file_name);
                return FALSE;
            }
        }
    }
    return TRUE;
}

int allocateRoiMapBuf(EncHandle handle, TestEncConfig encConfig, vpu_buffer_t *vbRoi, int srcFbNum, int ctuNum)
{
    int i;
    Int32 coreIdx = handle->coreIdx;
    if (encConfig.roi_enable) {
        //number of roi buffer should be the same as source buffer num.
        for (i = 0; i < srcFbNum ; i++) {
            vbRoi[i].size = ctuNum;
            if (vdi_allocate_dma_memory(coreIdx, &vbRoi[i], ENC_ETC, handle->instIndex) < 0) {
                VLOG(ERR, "fail to allocate ROI buffer\n" );
                return FALSE;
            }
        }
    }
    return TRUE;
}

// Define tokens for parsing scaling list file
const char* MatrixType[SCALING_LIST_SIZE_NUM][SL_NUM_MATRIX] =
{
    {"INTRA4X4_LUMA", "INTRA4X4_CHROMAU", "INTRA4X4_CHROMAV", "INTER4X4_LUMA", "INTER4X4_CHROMAU", "INTER4X4_CHROMAV"},
    {"INTRA8X8_LUMA", "INTRA8X8_CHROMAU", "INTRA8X8_CHROMAV", "INTER8X8_LUMA", "INTER8X8_CHROMAU", "INTER8X8_CHROMAV"},
    {"INTRA16X16_LUMA", "INTRA16X16_CHROMAU", "INTRA16X16_CHROMAV", "INTER16X16_LUMA", "INTER16X16_CHROMAU", "INTER16X16_CHROMAV"},
    {"INTRA32X32_LUMA", "INTRA32X32_CHROMAU_FROM16x16_CHROMAU", "INTRA32X32_CHROMAV_FROM16x16_CHROMAV","INTER32X32_LUMA", "INTER32X32_CHROMAU_FROM16x16_CHROMAU", "INTER32X32_CHROMAV_FROM16x16_CHROMAV"}
};

const char* MatrixType_DC[SCALING_LIST_SIZE_NUM - 2][SL_NUM_MATRIX] =
{
    {"INTRA16X16_LUMA_DC", "INTRA16X16_CHROMAU_DC", "INTRA16X16_CHROMAV_DC", "INTER16X16_LUMA_DC", "INTER16X16_CHROMAU_DC", "INTER16X16_CHROMAV_DC"},
    {"INTRA32X32_LUMA_DC", "INTRA32X32_CHROMAU_DC_FROM16x16_CHROMAU", "INTRA32X32_CHROMAV_DC_FROM16x16_CHROMAV", "INTER32X32_LUMA_DC","INTER32X32_CHROMAU_DC_FROM16x16_CHROMAU","INTER32X32_CHROMAV_DC_FROM16x16_CHROMAV"},
};

static Uint8* get_sl_addr(UserScalingList* sl, Uint32 size_id, Uint32 mat_id)
{
    Uint8* addr = NULL;

    switch(size_id)
    {
    case SCALING_LIST_4x4:
        addr = sl->s4[mat_id];
        break;
    case SCALING_LIST_8x8:
        addr = sl->s8[mat_id];
        break;
    case SCALING_LIST_16x16:
        addr = sl->s16[mat_id];
        break;
    case SCALING_LIST_32x32:
        addr = sl->s32[mat_id];
        break;
    }
    return addr;
}

int parse_user_scaling_list(UserScalingList* sl, FILE* fp_sl, CodStd  stdMode)
{
#define LINE_SIZE (1024)
    const Uint32 scaling_list_size[SCALING_LIST_SIZE_NUM] = {16, 64, 64, 64};
    char line[LINE_SIZE];
    Uint32 i;
    Uint32 size_id, mat_id, data, num_coef = 0;
    Uint8* src = NULL;
    Uint8* ref = NULL;
    char* ret;
    const char* type_str;

    if ( fp_sl == NULL)
        return 0;

    for(size_id = 0; size_id < SCALING_LIST_SIZE_NUM; size_id++)  {// for 4, 8, 16, 32
        num_coef = scaling_list_size[size_id];//lint !e644

        if (stdMode == STD_AVC && size_id > SCALING_LIST_8x8)
            break;

        for(mat_id = 0; mat_id < SL_NUM_MATRIX; mat_id++) { // for intra_y, intra_cb, intra_cr, inter_y, inter_cb, inter_cr
            src = get_sl_addr(sl, size_id, mat_id);

            if(size_id == SCALING_LIST_32x32 && (mat_id % 3)) { // derive scaling list of chroma32x32 from that of chrom16x16
                ref = get_sl_addr(sl, size_id - 1, mat_id);

                for(i = 0; i < num_coef; i++)
                    src[i] = ref[i];
            }
            else {
                fseek(fp_sl,0,0);
                type_str = MatrixType[size_id][mat_id];

                do {
                    ret = fgets(line, LINE_SIZE, fp_sl);
                    if((ret == NULL) || (strstr(line, type_str) == NULL && feof(fp_sl))) {
                        VLOG(ERR,"Error: can't read a scaling list matrix(%s)\n", type_str);
                        return 0;
                    }
                } while (strstr(line, type_str) == NULL);

                // get all coeff
                for(i = 0; i < num_coef; i++) {
                    if(fscanf(fp_sl, "%d,", &data) != 1) {
                        VLOG(ERR,"Error: can't read a scaling list matrix(%s)\n", type_str);
                        return 0;
                    }
                    if (stdMode == STD_AVC && data < 4) {
                        VLOG(ERR,"Error: invald value in scaling list matrix(%s %d)\n", type_str, data);
                        return 0;
                    }
                    src[i] = data;
                }

                // get DC coeff for 16, 32
                if(size_id > SCALING_LIST_8x8) {
                    fseek(fp_sl,0,0);
                    type_str = MatrixType_DC[size_id - 2][mat_id];

                    do {
                        ret = fgets(line, LINE_SIZE, fp_sl);
                        if((ret == NULL) || (strstr(line, type_str) == NULL && feof(fp_sl))) {
                            VLOG(ERR,"Error: can't read a scaling list matrix(%s)\n", type_str);
                            return 0;
                        }
                    } while (strstr(line, type_str) == NULL);

                    if(fscanf(fp_sl, "%d,", &data) != 1) {
                        VLOG(ERR,"Error: can't read a scaling list matrix(%s)\n", type_str);
                        return 0;
                    }
                    if (stdMode == STD_AVC && data < 4) {
                        VLOG(ERR,"Error: invald value in scaling list matrix(%s %d)\n", type_str, data);
                        return 0;
                    }
                    if(size_id == SCALING_LIST_16x16)
                        sl->s16dc[mat_id] = data;
                    else // SCALING_LIST_32x32
                        sl->s32dc[mat_id/3] = data;
                }
            }
        } // for matrix id
    } // for size id
    return 1;
}

int parse_custom_lambda(Uint32 buf[NUM_CUSTOM_LAMBDA], FILE* fp)
{
    int i, j = 0;
    char lineStr[256] = {0, };
    for(i = 0; i < 52; i++)
    {
        if( NULL == fgets(lineStr, 256, fp) )
        {
            VLOG(ERR,"Error: can't read custom_lambda\n");
            return 0;
        }
        else
        {
            sscanf(lineStr, "%d\n", &buf[j++]);
        }
    }
    for(i = 0; i < 52; i++)
    {
        if( NULL == fgets(lineStr, 256, fp) )
        {
            VLOG(ERR,"Error: can't read custom_lambda\n");
            return 0;
        }
        else
            sscanf(lineStr, "%d\n", &buf[j++]);
    }

    return 1;
}


#if defined(PLATFORM_NON_OS) || defined (PLATFORM_LINUX)
struct option* ConvertOptions(
    struct OptionExt*   cnmOpt,
    Uint32              nItems
    )
{
    struct option*  opt;
    Uint32          i;

    opt = (struct option*)osal_malloc(sizeof(struct option) * nItems);
    if (opt == NULL) {
        return NULL;
    }

    for (i=0; i<nItems; i++) {
        osal_memcpy((void*)&opt[i], (void*)&cnmOpt[i], sizeof(struct option));
    }

    return opt;
}
#endif

#ifdef PLATFORM_LINUX
int mkdir_recursive(
    char *path,
    mode_t omode
    )
{
    struct stat sb;
    mode_t numask, oumask;
    int first, last, retval;
    char *p;

    p = path;
    oumask = 0;
    retval = 0;
    if (p[0] == '/')        /* Skip leading '/'. */
        ++p;
    for (first = 1, last = 0; !last ; ++p) {//lint !e441 !e443
        if (p[0] == '\0')
            last = 1;
        else if (p[0] != '/')
            continue;
        *p = '\0';
        if (p[1] == '\0')
            last = 1;
        if (first) {
            /*
            * POSIX 1003.2:
            * For each dir operand that does not name an existing
            * directory, effects equivalent to those cased by the
            * following command shall occcur:
            *
            * mkdir -p -m $(umask -S),u+wx $(dirname dir) &&
            *    mkdir [-m mode] dir
            *
            * We change the user's umask and then restore it,
            * instead of doing chmod's.
            */
            oumask = umask(0);
            numask = oumask & ~(S_IWUSR | S_IXUSR);
            (void)umask(numask);
            first = 0;
        }
        if (last)
            (void)umask(oumask);
        if (mkdir(path, last ? omode : S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
            if (errno == EEXIST || errno == EISDIR) {
                if (stat(path, &sb) < 0) {
                    VLOG(INFO, "%s", path);
                    retval = 1;
                    break;
                } else if (!S_ISDIR(sb.st_mode)) {
                    if (last)
                        errno = EEXIST;
                    else
                        errno = ENOTDIR;
                    VLOG(INFO, "%s", path);
                    retval = 1;
                    break;
                }
            } else {
                VLOG(INFO, "%s", path);
                retval = 1;
                break;
            }
        } else {
            VLOG(INFO, "%s", path);
            chmod(path, omode);
        }
        if (!last)
            *p = '/';
    }
    if (!first && !last)
        (void)umask(oumask);
    return (retval);
}
#endif

int file_exist(
    char* path
    )
{
#ifdef _MSC_VER
    DWORD   attributes;
    char    temp[4096];
    LPCTSTR lp_path = (LPCTSTR)temp;

    if (path == NULL) {
        return FALSE;
    }

    strcpy(temp, path);
    replace_character(temp, '/', '\\');
    attributes = GetFileAttributes(lp_path);
    return (attributes != (DWORD)-1);
#else
    return !access(path, F_OK);
#endif
}

BOOL MkDir(
    char* path
    )
{
#if defined(PLATFORM_NON_OS) || defined(PLATFORM_QNX)
    /* need to implement */
    return FALSE;
#else
#ifdef _MSC_VER
    char cmd[4096];
#endif
    if (file_exist(path))
        return TRUE;

#ifdef _MSC_VER
    sprintf(cmd, "mkdir %s", path);
    replace_character(cmd, '/', '\\');
    if (system(cmd)) {
        return FALSE;
    }
    return TRUE;
#else
    return mkdir_recursive(path, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
#endif
}

void SetMapData(int coreIdx, TestEncConfig encConfig, EncOpenParam encOP, EncParam *encParam, int srcFrameWidth, int srcFrameHeight, unsigned long addrCustomMap)
{
    int productId = VPU_GetProductId(coreIdx);

    if (productId == PRODUCT_ID_521 && encOP.bitstreamFormat == STD_AVC) {
        Uint8               roiMapBuf[MAX_MB_NUM] = {0,};
        Uint8               modeMapBuf[MAX_MB_NUM] = {0,};
        AvcEncCustomMap     customMapBuf[MAX_MB_NUM];	// custom map 1 MB data = 8bits
        int MbWidth         = VPU_ALIGN16(encOP.picWidth) >> 4;
        int MbHeight        = VPU_ALIGN16(encOP.picHeight) >> 4;
        int h, w, mbAddr;
        osal_memset(&customMapBuf[0], 0x00, MAX_MB_NUM);
        if (encParam->customMapOpt.customRoiMapEnable == 1) {
            int sumQp = 0;
            int bufSize = MbWidth*MbHeight;

            if (osal_fread(&roiMapBuf[0], 1, bufSize, encConfig.roi_file) != bufSize) {
                osal_fseek(encConfig.roi_file, 0, SEEK_SET);
                osal_fread(&roiMapBuf[0], 1, bufSize, encConfig.roi_file);
            }

            for (h = 0; h < MbHeight; h++) {
                for (w = 0; w < MbWidth; w++) {
                    mbAddr = w + h*MbWidth;
                    customMapBuf[mbAddr].field.mb_qp = MAX(MIN(roiMapBuf[mbAddr]&0x3f, 51), 0);
                    sumQp += customMapBuf[mbAddr].field.mb_qp;
                }
            }
            encParam->customMapOpt.roiAvgQp = (sumQp + (bufSize>>1)) / bufSize; // round off.
        }

        if (encParam->customMapOpt.customModeMapEnable == 1) {
                int bufSize = MbWidth*MbHeight;

                if (osal_fread(&modeMapBuf[0], 1, bufSize, encConfig.mode_map_file) != bufSize) {
                    osal_fseek(encConfig.mode_map_file, 0, SEEK_SET);
                    osal_fread(&modeMapBuf[0], 1, bufSize, encConfig.mode_map_file);
                }

                for (h = 0; h < MbHeight; h++) {
                    for (w = 0; w < MbWidth; w++) {
                        mbAddr = w + h*MbWidth;
                        customMapBuf[mbAddr].field.mb_force_mode = modeMapBuf[mbAddr] & 0x3;
                    }
                }
        }

        encParam->customMapOpt.addrCustomMap = addrCustomMap;
        vdi_write_memory(coreIdx, encParam->customMapOpt.addrCustomMap, (unsigned char*)customMapBuf, MAX_MB_NUM, VDI_LITTLE_ENDIAN);
    }
    else {
        // for HEVC custom map.
        Uint8               roiMapBuf[MAX_SUB_CTU_NUM] = {0,};
        Uint8               lambdaMapBuf[MAX_SUB_CTU_NUM] = {0,};
        Uint8               modeMapBuf[MAX_CTU_NUM] = {0,};
        EncCustomMap        customMapBuf[MAX_CTU_NUM];	// custom map 1 CTU data = 64bits
        int ctuMapWidthCnt  = VPU_ALIGN64(encOP.picWidth) >> 6;
        int ctuMapHeightCnt = VPU_ALIGN64(encOP.picHeight) >> 6;
        int ctuMapStride = VPU_ALIGN64(encOP.picWidth) >> 6;
        int subCtuMapStride = VPU_ALIGN64(encOP.picWidth) >> 5;
        int i, sumQp = 0;;
        int h, w, ctuPos, initQp;
        Uint8* src;
        VpuRect region[MAX_ROI_NUMBER];        /**< The size of the ROI region for H.265 (start X/Y in CTU, end X/Y int CTU)  */
        int roiQp[MAX_ROI_NUMBER];       /**< An importance level for the given ROI region for H.265. The higher an ROI level is, the more important the region is with a lower QP.  */

        osal_memset(&customMapBuf[0], 0x00, MAX_CTU_NUM * 8);
        if (encParam->customMapOpt.customRoiMapEnable == 1) {
            int bufSize = (VPU_ALIGN64(encOP.picWidth) >> 5) * (VPU_ALIGN64(encOP.picHeight) >> 5);

            if (productId == PRODUCT_ID_521) {
                if (osal_fread(&roiMapBuf[0], 1, bufSize, encConfig.roi_file) != bufSize) {
                    osal_fseek(encConfig.roi_file, 0, SEEK_SET);
                    osal_fread(&roiMapBuf[0], 1, bufSize, encConfig.roi_file);
                }
            }
            else {
                int roiNum = 0;
                // sample code to convert ROI coordinate to ROI map.
                osal_memset(&region[0], 0, sizeof(VpuRect)*MAX_ROI_NUMBER);
                osal_memset(&roiQp[0], 0, sizeof(int)*MAX_ROI_NUMBER);
                if (encConfig.roi_file_name[0]) {
                    char lineStr[256] = {0,};
                    int  val;
                    fgets(lineStr, 256, encConfig.roi_file);
                    sscanf(lineStr, "%x\n", &val);
                    if (val != 0xFFFF)
                        VLOG(ERR, "can't find the picture delimiter \n");

                    fgets(lineStr, 256, encConfig.roi_file);
                    sscanf(lineStr, "%d\n", &val);  // picture index

                    fgets(lineStr, 256, encConfig.roi_file);
                    sscanf(lineStr, "%d\n", &roiNum);   // number of roi regions

                    for (i = 0; i < roiNum; i++) {
                        fgets(lineStr, 256, encConfig.roi_file);
                        if (parseRoiCtuModeParam(lineStr, &region[i], &roiQp[i], srcFrameWidth, srcFrameHeight) == 0) {
                            VLOG(ERR, "CFG file error : %s value is not available. \n", encConfig.roi_file_name);
                        }
                    }
                }
                encParam->customMapOpt.customRoiMapEnable    = (roiNum != 0) ? encParam->customMapOpt.customRoiMapEnable : 0;

                if (encParam->customMapOpt.customRoiMapEnable) {
                    initQp = encConfig.roi_avg_qp;
                    GenRegionToQpMap(&region[0], &roiQp[0], roiNum, initQp, subCtuMapStride, VPU_ALIGN64(encOP.picHeight) >> 5, &roiMapBuf[0]);
                }
            }

            for (h = 0; h < ctuMapHeightCnt; h++) {
                src = &roiMapBuf[subCtuMapStride * h * 2];
                for (w = 0; w < ctuMapWidthCnt; w++, src += 2) {
                    ctuPos = (h * ctuMapStride + w);

                    // store in the order of sub-ctu.
                    customMapBuf[ctuPos].field.sub_ctu_qp_0 = MAX(MIN(*src, 51), 0);
                    customMapBuf[ctuPos].field.sub_ctu_qp_1 = MAX(MIN(*(src + 1), 51), 0);
                    customMapBuf[ctuPos].field.sub_ctu_qp_2 = MAX(MIN(*(src + subCtuMapStride), 51), 0);
                    customMapBuf[ctuPos].field.sub_ctu_qp_3 = MAX(MIN(*(src + subCtuMapStride + 1), 51), 0);
                    sumQp += (customMapBuf[ctuPos].field.sub_ctu_qp_0 + customMapBuf[ctuPos].field.sub_ctu_qp_1 + customMapBuf[ctuPos].field.sub_ctu_qp_2 + customMapBuf[ctuPos].field.sub_ctu_qp_3);
                }
            }
            if (productId == PRODUCT_ID_521)
                encParam->customMapOpt.roiAvgQp = (sumQp + (bufSize>>1)) / bufSize; // round off.
        }

        if (encParam->customMapOpt.customLambdaMapEnable == 1) {
            int bufSize = (VPU_ALIGN64(encOP.picWidth) >> 5) * (VPU_ALIGN64(encOP.picHeight) >> 5);

            if (osal_fread(&lambdaMapBuf[0], 1, bufSize, encConfig.lambda_map_file) != bufSize) {
                osal_fseek(encConfig.lambda_map_file, 0, SEEK_SET);
                osal_fread(&lambdaMapBuf[0], 1, bufSize, encConfig.lambda_map_file);
            }

            for (h = 0; h < ctuMapHeightCnt; h++) {
                src = &lambdaMapBuf[subCtuMapStride * 2 * h];
                for (w = 0; w < ctuMapWidthCnt; w++, src += 2) {
                    ctuPos = (h * ctuMapStride + w);

                    // store in the order of sub-ctu.
                    customMapBuf[ctuPos].field.lambda_sad_0 = *src;
                    customMapBuf[ctuPos].field.lambda_sad_1 = *(src + 1);
                    customMapBuf[ctuPos].field.lambda_sad_2 = *(src + subCtuMapStride);
                    customMapBuf[ctuPos].field.lambda_sad_3 = *(src + subCtuMapStride + 1);
                }
            }
        }

        if ((encParam->customMapOpt.customModeMapEnable == 1)
            || (encParam->customMapOpt.customCoefDropEnable == 1)) {
                int bufSize = (VPU_ALIGN64(encOP.picWidth) >> 6) * (VPU_ALIGN64(encOP.picHeight) >> 6);

                if (osal_fread(&modeMapBuf[0], 1, bufSize, encConfig.mode_map_file) != bufSize) {
                    osal_fseek(encConfig.mode_map_file, 0, SEEK_SET);
                    osal_fread(&modeMapBuf[0], 1, bufSize, encConfig.mode_map_file);
                }

                for (h = 0; h < ctuMapHeightCnt; h++) {
                    src = &modeMapBuf[ctuMapStride * h];
                    for (w = 0; w < ctuMapWidthCnt; w++, src++) {
                        ctuPos = (h * ctuMapStride + w);
                        customMapBuf[ctuPos].field.ctu_force_mode = (*src) & 0x3;
                        customMapBuf[ctuPos].field.ctu_coeff_drop  = ((*src) >> 2) & 0x1;
                    }
                }
        }

        encParam->customMapOpt.addrCustomMap = addrCustomMap;
        vdi_write_memory(coreIdx, encParam->customMapOpt.addrCustomMap, (unsigned char*)customMapBuf, MAX_CTU_NUM * 8, VDI_LITTLE_ENDIAN);
    }

}

RetCode SetChangeParam(EncHandle handle, TestEncConfig encConfig, EncOpenParam encOP, Int32 changedCount)
{
    int i;
    RetCode ret;
    ENC_CFG ParaChagCfg;
    EncChangeParam changeParam;

    osal_memset(&ParaChagCfg, 0x00, sizeof(ENC_CFG));
    osal_memset(&changeParam, 0x00, sizeof(EncChangeParam));

    parseVpChangeParamCfgFile(&ParaChagCfg, encConfig.changeParam[changedCount].cfgName);

    changeParam.enable_option           = encConfig.changeParam[changedCount].enableOption;

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_PPS) {
        changeParam.constIntraPredFlag			= ParaChagCfg.vpCfg.constIntraPredFlag;
        changeParam.lfCrossSliceBoundaryEnable	= ParaChagCfg.vpCfg.lfCrossSliceBoundaryEnable;
        changeParam.weightPredEnable			= ParaChagCfg.vpCfg.weightPredEnable;
        changeParam.disableDeblk				= ParaChagCfg.vpCfg.disableDeblk;
        changeParam.betaOffsetDiv2				= ParaChagCfg.vpCfg.betaOffsetDiv2;
        changeParam.tcOffsetDiv2				= ParaChagCfg.vpCfg.tcOffsetDiv2;
        changeParam.chromaCbQpOffset			= ParaChagCfg.vpCfg.chromaCbQpOffset;
        changeParam.chromaCrQpOffset			= ParaChagCfg.vpCfg.chromaCrQpOffset;
        if (encConfig.stdMode == STD_AVC) {
            changeParam.transform8x8Enable      = ParaChagCfg.vpCfg.transform8x8;
            changeParam.entropyCodingMode       = ParaChagCfg.vpCfg.entropyCodingMode;
        }
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_INDEPEND_SLICE) {
        changeParam.independSliceMode			= ParaChagCfg.vpCfg.independSliceMode;
        changeParam.independSliceModeArg		= ParaChagCfg.vpCfg.independSliceModeArg;
        if (encConfig.stdMode == STD_AVC) {
            changeParam.avcSliceMode            = ParaChagCfg.vpCfg.avcSliceMode;
            changeParam.avcSliceArg             = ParaChagCfg.vpCfg.avcSliceArg;
        }
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_DEPEND_SLICE) {
        changeParam.dependSliceMode				= ParaChagCfg.vpCfg.dependSliceMode;
        changeParam.dependSliceModeArg			= ParaChagCfg.vpCfg.dependSliceModeArg;
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_RDO) {
        changeParam.coefClearDisable			= ParaChagCfg.vpCfg.coefClearDisable;
        changeParam.intraNxNEnable				= ParaChagCfg.vpCfg.intraNxNEnable;
        changeParam.maxNumMerge					= ParaChagCfg.vpCfg.maxNumMerge;
        changeParam.customLambdaEnable			= ParaChagCfg.vpCfg.customLambdaEnable;
        changeParam.customMDEnable				= ParaChagCfg.vpCfg.customMDEnable;
        if (encConfig.stdMode == STD_AVC) {
            changeParam.rdoSkip                 = ParaChagCfg.vpCfg.rdoSkip;
            changeParam.lambdaScalingEnable     = ParaChagCfg.vpCfg.lambdaScalingEnable;
        }
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_RC_TARGET_RATE) {
        changeParam.bitRate						= ParaChagCfg.RcBitRate;
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_RC) {
        changeParam.hvsQPEnable					= ParaChagCfg.vpCfg.hvsQPEnable;
        changeParam.hvsQpScale					= ParaChagCfg.vpCfg.hvsQpScale;
        changeParam.vbvBufferSize				= ParaChagCfg.VbvBufferSize;
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_RC_MIN_MAX_QP) {
        changeParam.minQpI						= ParaChagCfg.vpCfg.minQp;
        changeParam.maxQpI						= ParaChagCfg.vpCfg.maxQp;
        changeParam.hvsMaxDeltaQp   			= ParaChagCfg.vpCfg.maxDeltaQp;
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_RC_INTER_MIN_MAX_QP) {
        changeParam.minQpP = ParaChagCfg.vpCfg.minQp;
        changeParam.minQpB = ParaChagCfg.vpCfg.minQp;
        changeParam.maxQpP = ParaChagCfg.vpCfg.maxQp;
        changeParam.maxQpB = ParaChagCfg.vpCfg.maxQp;
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_RC_BIT_RATIO_LAYER) {
        for (i=0 ; i<MAX_GOP_NUM; i++)
            changeParam.fixedBitRatio[i]	= ParaChagCfg.vpCfg.fixedBitRatio[i];
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_BG) {
        changeParam.s2fmeDisable                = ParaChagCfg.vpCfg.s2fmeDisable;
        changeParam.bgThrDiff					= ParaChagCfg.vpCfg.bgThrDiff;
        changeParam.bgThrMeanDiff				= ParaChagCfg.vpCfg.bgThrMeanDiff;
        changeParam.bgLambdaQp					= ParaChagCfg.vpCfg.bgLambdaQp;
        changeParam.bgDeltaQp					= ParaChagCfg.vpCfg.bgDeltaQp;
        changeParam.s2fmeDisable                = ParaChagCfg.vpCfg.s2fmeDisable;
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_NR) {
        changeParam.nrYEnable					= ParaChagCfg.vpCfg.nrYEnable;
        changeParam.nrCbEnable					= ParaChagCfg.vpCfg.nrCbEnable;
        changeParam.nrCrEnable					= ParaChagCfg.vpCfg.nrCrEnable;
        changeParam.nrNoiseEstEnable			= ParaChagCfg.vpCfg.nrNoiseEstEnable;
        changeParam.nrNoiseSigmaY				= ParaChagCfg.vpCfg.nrNoiseSigmaY;
        changeParam.nrNoiseSigmaCb				= ParaChagCfg.vpCfg.nrNoiseSigmaCb;
        changeParam.nrNoiseSigmaCr				= ParaChagCfg.vpCfg.nrNoiseSigmaCr;

        changeParam.nrIntraWeightY				= ParaChagCfg.vpCfg.nrIntraWeightY;
        changeParam.nrIntraWeightCb				= ParaChagCfg.vpCfg.nrIntraWeightCb;
        changeParam.nrIntraWeightCr				= ParaChagCfg.vpCfg.nrIntraWeightCr;
        changeParam.nrInterWeightY				= ParaChagCfg.vpCfg.nrInterWeightY;
        changeParam.nrInterWeightCb				= ParaChagCfg.vpCfg.nrInterWeightCb;
        changeParam.nrInterWeightCr				= ParaChagCfg.vpCfg.nrInterWeightCr;
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_CUSTOM_MD) {
        changeParam.pu04DeltaRate               = ParaChagCfg.vpCfg.pu04DeltaRate;
        changeParam.pu08DeltaRate               = ParaChagCfg.vpCfg.pu08DeltaRate;
        changeParam.pu16DeltaRate               = ParaChagCfg.vpCfg.pu16DeltaRate;
        changeParam.pu32DeltaRate               = ParaChagCfg.vpCfg.pu32DeltaRate;
        changeParam.pu04IntraPlanarDeltaRate    = ParaChagCfg.vpCfg.pu04IntraPlanarDeltaRate;
        changeParam.pu04IntraDcDeltaRate        = ParaChagCfg.vpCfg.pu04IntraDcDeltaRate;
        changeParam.pu04IntraAngleDeltaRate     = ParaChagCfg.vpCfg.pu04IntraAngleDeltaRate;
        changeParam.pu08IntraPlanarDeltaRate    = ParaChagCfg.vpCfg.pu08IntraPlanarDeltaRate;
        changeParam.pu08IntraDcDeltaRate        = ParaChagCfg.vpCfg.pu08IntraDcDeltaRate;
        changeParam.pu08IntraAngleDeltaRate     = ParaChagCfg.vpCfg.pu08IntraAngleDeltaRate;
        changeParam.pu16IntraPlanarDeltaRate    = ParaChagCfg.vpCfg.pu16IntraPlanarDeltaRate;
        changeParam.pu16IntraDcDeltaRate        = ParaChagCfg.vpCfg.pu16IntraDcDeltaRate;
        changeParam.pu16IntraAngleDeltaRate     = ParaChagCfg.vpCfg.pu16IntraAngleDeltaRate;
        changeParam.pu32IntraPlanarDeltaRate    = ParaChagCfg.vpCfg.pu32IntraPlanarDeltaRate;
        changeParam.pu32IntraDcDeltaRate        = ParaChagCfg.vpCfg.pu32IntraDcDeltaRate;
        changeParam.pu32IntraAngleDeltaRate     = ParaChagCfg.vpCfg.pu32IntraAngleDeltaRate;
        changeParam.cu08IntraDeltaRate          = ParaChagCfg.vpCfg.cu08IntraDeltaRate;
        changeParam.cu08InterDeltaRate          = ParaChagCfg.vpCfg.cu08InterDeltaRate;
        changeParam.cu08MergeDeltaRate          = ParaChagCfg.vpCfg.cu08MergeDeltaRate;
        changeParam.cu16IntraDeltaRate          = ParaChagCfg.vpCfg.cu16IntraDeltaRate;
        changeParam.cu16InterDeltaRate          = ParaChagCfg.vpCfg.cu16InterDeltaRate;
        changeParam.cu16MergeDeltaRate          = ParaChagCfg.vpCfg.cu16MergeDeltaRate;
        changeParam.cu32IntraDeltaRate          = ParaChagCfg.vpCfg.cu32IntraDeltaRate;
        changeParam.cu32InterDeltaRate          = ParaChagCfg.vpCfg.cu32InterDeltaRate;
        changeParam.cu32MergeDeltaRate          = ParaChagCfg.vpCfg.cu32MergeDeltaRate;
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_INTRA_PARAM) {
        changeParam.intraQP                     = ParaChagCfg.vpCfg.intraQP;
        changeParam.intraPeriod                 = ParaChagCfg.vpCfg.intraPeriod;
    }

    ret = VPU_EncGiveCommand(handle, ENC_SET_PARA_CHANGE, &changeParam);

    return ret;
}


BOOL GetBitstreamToBuffer(
    EncHandle handle,
    Uint8* pBuffer,
    PhysicalAddress rdAddr,
    PhysicalAddress wrAddr,
    PhysicalAddress streamBufStartAddr,
    PhysicalAddress streamBufEndAddr,
    Uint32 streamSize,
    EndianMode endian,
    BOOL ringbufferEnabled
    )
{
    Int32 coreIdx      = -1;
    Uint32 room         = 0;

    if (NULL == handle) {
        VLOG(ERR, "<%s:%d> NULL point exception\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    coreIdx = VPU_HANDLE_CORE_INDEX(handle);

    if (0 == streamBufStartAddr || 0 == streamBufEndAddr) {
        VLOG(ERR, "<%s:%d> Wrong Address, start or end Addr\n", __FUNCTION__, __LINE__);
        return FALSE;
    } else if (0 == rdAddr || 0 == wrAddr) {
        VLOG(ERR, "<%s:%d> Wrong Address, read or write Addr\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    if (TRUE == ringbufferEnabled) {
        if ((rdAddr + streamSize) > streamBufEndAddr) {
            //wrap around on ringbuffer
            room = streamBufEndAddr - rdAddr;
            vdi_read_memory(coreIdx, rdAddr, pBuffer, room, endian);
            vdi_read_memory(coreIdx, streamBufStartAddr, pBuffer + room, (streamSize-room), endian);
        }
        else {
            vdi_read_memory(coreIdx, rdAddr, pBuffer, streamSize, endian);
        }
    } else { //Line buffer
        vdi_read_memory(coreIdx, rdAddr, pBuffer, streamSize, endian);
    }

    return TRUE;
}

