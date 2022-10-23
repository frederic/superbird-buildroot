/*
 * Copyright (c) 2009, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * cmem.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "amlenc.h"
#include "enc_define.h"
#include "m8venclib.h"

#define ENCODER_PATH       "/dev/amvenc_avc"

#define AMVENC_DEVINFO_M8_FAST "AML-M8-FAST"
#define AMVENC_DEVINFO_M8 "AML-M8"

#define AMVENC_AVC_IOC_MAGIC  'E'
#define AMVENC_AVC_IOC_GET_DEVINFO              _IOW(AMVENC_AVC_IOC_MAGIC, 0xf0, unsigned int)

const AMVencHWFuncPtr m8_fast_dev =
{
    InitFastEncode,
    FastEncodeInitFrame,
    FastEncodeSPS_PPS,
    FastEncodeSlice,
    FastEncodeCommit,
    UnInitFastEncode,
};


const AMVencHWFuncPtr *gdev[] =
{
    &m8_fast_dev,
    NULL,
};

AMVEnc_Status InitAMVEncode(amvenc_hw_t *hw_info, int force_mode)
{
    int fd = -1;
    if (!hw_info)
    {
        printf("AMVENC MEMORY FAIL. \n");
        return AMVENC_MEMORY_FAIL;
    }
    hw_info->dev_fd = -1;
    hw_info->dev_data = NULL;
    fd = open(ENCODER_PATH, O_RDWR);
    if (fd < 0)
    {
        printf("Open /dev/amvenc_avc failed !\n");
        return AMVENC_FAIL;
    }
    hw_info->dev_id = M8_FAST;
    if (gdev[hw_info->dev_id]->Initialize != NULL)
        hw_info->dev_data = gdev[hw_info->dev_id]->Initialize(fd, &hw_info->init_para);
    if (!hw_info->dev_data)
    {
        printf("InitAMVEncode Fail, dev type:%d.\n", hw_info->dev_id);
        hw_info->dev_id = NO_DEFINE;
        close(fd);
        return AMVENC_FAIL;
    }
    hw_info->dev_fd = fd;
    return AMVENC_SUCCESS;
}

AMVEnc_Status AMVEncodeInitFrame(amvenc_hw_t *hw_info, unsigned *yuv, AMVEncBufferType type, AMVEncFrameFmt fmt, int IDRframe)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    if (!hw_info)
        return AMVENC_MEMORY_FAIL;
    if ((hw_info->dev_id <= NO_DEFINE) || (hw_info->dev_id >= MAX_DEV) || (hw_info->dev_fd < 0) || (!hw_info->dev_data))
        return AMVENC_FAIL;
    if (gdev[hw_info->dev_id]->InitFrame != NULL)
        ret = gdev[hw_info->dev_id]->InitFrame(hw_info->dev_data, yuv, type, fmt, IDRframe);
    return ret;
}

AMVEnc_Status AMVEncodeSPS_PPS(amvenc_hw_t *hw_info, unsigned char *outptr, int *datalen)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    if (!hw_info)
        return AMVENC_MEMORY_FAIL;
    if ((hw_info->dev_id <= NO_DEFINE) || (hw_info->dev_id >= MAX_DEV) || (hw_info->dev_fd < 0) || (!hw_info->dev_data))
        return AMVENC_FAIL;
    if (gdev[hw_info->dev_id]->EncodeSPS_PPS != NULL)
        ret = gdev[hw_info->dev_id]->EncodeSPS_PPS(hw_info->dev_data, outptr, datalen);
    return ret;
}

AMVEnc_Status AMVEncodeSlice(amvenc_hw_t *hw_info, unsigned char *outptr, int *datalen)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    if (!hw_info)
        return AMVENC_MEMORY_FAIL;
    if ((hw_info->dev_id <= NO_DEFINE) || (hw_info->dev_id >= MAX_DEV) || (hw_info->dev_fd < 0) || (!hw_info->dev_data))
        return AMVENC_FAIL;
    if (gdev[hw_info->dev_id]->EncodeSlice != NULL)
        ret = gdev[hw_info->dev_id]->EncodeSlice(hw_info->dev_data, outptr, datalen);
    return ret;
}

AMVEnc_Status AMVEncodeCommit(amvenc_hw_t *hw_info, int IDR)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    if (!hw_info)
        return AMVENC_MEMORY_FAIL;
    if ((hw_info->dev_id <= NO_DEFINE) || (hw_info->dev_id >= MAX_DEV) || (hw_info->dev_fd < 0) || (!hw_info->dev_data))
        return AMVENC_FAIL;
    if (gdev[hw_info->dev_id]->Commit != NULL)
        ret = gdev[hw_info->dev_id]->Commit(hw_info->dev_data, IDR);
    return ret;
}

void UnInitAMVEncode(amvenc_hw_t *hw_info)
{
    if (!hw_info)
        return;
    if ((hw_info->dev_data) && (hw_info->dev_id > NO_DEFINE) && (hw_info->dev_id < MAX_DEV))
        gdev[hw_info->dev_id]->Release(hw_info->dev_data);
    hw_info->dev_data = NULL;
    if (hw_info->dev_fd >= 0)
    {
        close(hw_info->dev_fd);
    }
    hw_info->dev_fd = -1;
    hw_info->dev_id = NO_DEFINE;
    return;
}

int StartEncode(char *src, char *dst, int width, int height, int qp, int framerate, int num)
{
    AMVEnc_Status status = AMVENC_FAIL;
    int outfd = -1;
    amvenc_info_t *info = (amvenc_info_t *)malloc(sizeof(amvenc_info_t));
    unsigned char *buffer = (unsigned char *)malloc(512 * 1024 * sizeof(char));
    unsigned char *input = (unsigned char *)malloc(width * height * 2);
    int datalen = 0;
    unsigned framesize  = width * height * 3 / 2;
    unsigned yuv[4];
    FILE *fp = NULL;
    int i = 0;
    if (!info)
    {
        status = AMVENC_MEMORY_FAIL;
        goto exit;
    }
    if ((!buffer) || (!input))
    {
        status = AMVENC_MEMORY_FAIL;
        goto exit;
    }
    if (framerate == 0)
    {
        status =  AMVENC_INVALID_FRAMERATE;
        goto exit;
    }
    fp = fopen(src, "rb");
    if (fp == NULL)
    {
        printf("open src file error!\n");
        status = AMVENC_FAIL;
        goto exit;
    }
    outfd = open(dst, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (outfd < 0)
    {
        printf("open dist file error!\n");
        status = AMVENC_FAIL;
        goto exit;
    }
    memset(info, 0, sizeof(amvenc_info_t));
    info->hw_info.dev_fd = -1;
    info->hw_info.dev_id = NO_DEFINE;
    /* now the rate control and performance related parameters */
    info->enc_width = width;
    info->enc_height = height;
    info->frame_rate = framerate;
    info->initQP = qp;
    if (info->initQP == 0)
    {
        info->initQP = 26;
    }
    info->hw_info.init_para.enc_width = info->enc_width;
    info->hw_info.init_para.enc_height = info->enc_height;
    info->hw_info.init_para.initQP = info->initQP;
    info->hw_info.init_para.nSliceHeaderSpacing = info->nSliceHeaderSpacing;
    info->hw_info.init_para.MBsIntraRefresh = info->MBsIntraRefresh;
    info->hw_info.init_para.MBsIntraOverlap = info->MBsIntraOverlap;
    info->hw_info.init_para.bitrate = info->bitrate;
    info->hw_info.init_para.frame_rate = info->frame_rate;
    info->hw_info.init_para.cpbSize = info->cpbSize;
    info->modTimeRef = 0;     /* ALWAYS ASSUME THAT TIMESTAMP START FROM 0 !!!*/
    info->prevCodedFrameNum = 0;
    info->late_frame_count = 0;
    printf("InitAMVEncode !\n");
    status = InitAMVEncode(&info->hw_info, M8_FAST);
    if (status != AMVENC_SUCCESS)
    {
        printf("InitAMVEncode error!\n");
        goto exit;
    }
    printf("AMVEncodeSPS_PPS !\n");
    memset(buffer, 0, 512 * 1024 * sizeof(char));
    status = AMVEncodeSPS_PPS(&info->hw_info, buffer, &datalen);
    if (status != AMVENC_SUCCESS)
    {
        printf("AMVEncodeSPS_PPS error!\n");
        goto exit;
    }
    write(outfd, (unsigned char *)buffer, datalen);
    /*
        if (fread(input, 1, framesize, fp ) != framesize) {
            printf("read input file error!\n");
            goto exit;
        }
    */
    printf("AMVEncodeInitFrame !\n");
    while (i < num)
    {
        if (fread(input, 1, framesize, fp) != framesize)
        {
            printf("read input file error!\n");
            goto exit;
        }
        yuv[0] = (unsigned)(&input[0]);
        yuv[1] = (unsigned)(yuv[0] + width * height);
        yuv[2] = (unsigned)(yuv[0] + width * height * 5 / 4);
        status = AMVEncodeInitFrame(&info->hw_info, (unsigned *)&yuv[0], VMALLOC_BUFFER, AMVENC_YUV420, ((i % framerate) == 0) ? 1 : 0);
        if ((status != AMVENC_SUCCESS) && (status != AMVENC_NEW_IDR))
        {
            printf("AMVEncodeInitFrame error!\n");
            break;
        }
        printf("AMVEncodeSlice!\n");
        memset(buffer, 0, 512 * 1024 * sizeof(char));
        status = AMVEncodeSlice(&info->hw_info, buffer, &datalen);
        if ((status != AMVENC_SUCCESS) && (status != AMVENC_NEW_IDR) && (status != AMVENC_PICTURE_READY))
        {
            printf("AMVEncodeSlice error!\n");
            break;
        }
        status = AMVEncodeCommit(&info->hw_info, ((i % framerate) == 0) ? 1 : 0);
        write(outfd, (unsigned char *)buffer, datalen);
        i++;
        printf("AMVEncodeSlice %d!\n", i);
    }
    UnInitAMVEncode(&info->hw_info);
    close(outfd);
    fclose(fp);
    free(buffer);
    free(input);
    free(info);
    printf("StartEncode finish. %d\n", status);
    return AMVENC_SUCCESS;
exit:
    if (input)
        free(input);
    if (buffer)
        free(buffer);
    if (outfd >= 0)
        close(outfd);
    if (fp)
        fclose(fp);
    if (info)
    {
        UnInitAMVEncode(&info->hw_info);
        free(info);
    }
    printf("StartEncode Fail, error=%d \n", status);
    return status;
}

