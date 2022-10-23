#include "vpcodec_1_0.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef MAKEANDROID
#include <utils/Log.h>
#endif

#include "include/AML_HWEncoder.h"
#include "include/enc_define.h"

const char version[] = "Amlogic libvpcodec version 1.0";

const char *vl_get_version()
{
    return version;
}

int initEncParams(AMVEncHandle *handle, int width, int height, int frame_rate, int bit_rate, int gop)
{
    memset(&(handle->mEncParams), 0, sizeof(AMVEncParams));
    LOGAPI("bit_rate:%d", bit_rate);
    if ((width % 16 != 0 || height % 2 != 0))
    {
        LOGAPI("Video frame size %dx%d must be a multiple of 16", width, height);
        return -1;
    }
    else if (height % 16 != 0)
    {
        LOGAPI("Video frame height is not standard:%d", height);
    }
    else
    {
        LOGAPI("Video frame size is %d x %d", width, height);
    }
    handle->mEncParams.rate_control = AVC_ON;
    handle->mEncParams.initQP = 0;
    handle->mEncParams.init_CBP_removal_delay = 1600;
    handle->mEncParams.auto_scd = AVC_ON;
    handle->mEncParams.out_of_band_param_set = AVC_ON;
    handle->mEncParams.num_ref_frame = 1;
    handle->mEncParams.num_slice_group = 1;
    handle->mEncParams.nSliceHeaderSpacing = 0;
    handle->mEncParams.fullsearch = AVC_OFF;
    handle->mEncParams.search_range = 16;
    //handle->mEncParams.sub_pel = AVC_OFF;
    //handle->mEncParams.submb_pred = AVC_OFF;
    handle->mEncParams.width = width;
    handle->mEncParams.height = height;
    handle->mEncParams.bitrate = bit_rate;
    handle->mEncParams.frame_rate = 1000 * frame_rate;  // In frames/ms!
    handle->mEncParams.CPB_size = (uint32)(bit_rate >> 1);
    handle->mEncParams.FreeRun = AVC_OFF;
    handle->mEncParams.MBsIntraRefresh = 0;
    handle->mEncParams.MBsIntraOverlap = 0;
    handle->mEncParams.encode_once = 1;
    // Set IDR frame refresh interval
    /*if ((unsigned) gop == 0xffffffff)
    {
        handle->mEncParams.idr_period = -1;//(mIDRFrameRefreshIntervalInSec * mVideoFrameRate);
    }
    else if (gop == 0)
    {
        handle->mEncParams.idr_period = 0;  // All I frames
    }
    else
    {
        handle->mEncParams.idr_period = gop + 1;
    }*/
    if (gop == 0 || gop < 0) {
        handle->mEncParams.idr_period = 0;   //an infinite period, only one I frame
    } else {
        handle->mEncParams.idr_period = gop; //period of I frame, 1 means all frames are I type.
    }
    // Set profile and level
    handle->mEncParams.profile = AVC_BASELINE;
    handle->mEncParams.level = AVC_LEVEL4;
    handle->mEncParams.initQP = 30;
    handle->mEncParams.BitrateScale = AVC_OFF;
    return 0;
}


vl_codec_handle_t vl_video_encoder_init(vl_codec_id_t codec_id, int width, int height, int frame_rate, int bit_rate, int gop, vl_img_format_t img_format)
{
    int ret;
    AMVEncHandle *mHandle = new AMVEncHandle;
    bool has_mix = false;
    if (mHandle == NULL)
        goto exit;
    memset(mHandle, 0, sizeof(AMVEncHandle));
    ret = initEncParams(mHandle, width, height, frame_rate, bit_rate, gop);
    if (ret < 0)
        goto exit;
    ret = AML_HWEncInitialize(mHandle, &(mHandle->mEncParams), &has_mix, 2);
    if (ret < 0)
        goto exit;
    mHandle->mSpsPpsHeaderReceived = false;
    mHandle->mNumInputFrames = -1;  // 1st two buffers contain SPS and PPS
    return (vl_codec_handle_t) mHandle;
exit:
    if (mHandle != NULL)
        delete mHandle;
    return (vl_codec_handle_t) NULL;
}



int vl_video_encoder_encode(vl_codec_handle_t codec_handle, vl_frame_type_t frame_type, unsigned char *in, int in_size, unsigned char *out, int format)
{
    int ret;
    uint8_t *outPtr = NULL;
    uint32_t dataLength = 0;
    int type;
    AMVEncHandle *handle = (AMVEncHandle *)codec_handle;
    if (!handle->mSpsPpsHeaderReceived)
    {
        ret = AML_HWEncNAL(handle, (unsigned char *)out, (unsigned int *)&in_size/*should be out size*/, &type);
        if (ret == AMVENC_SUCCESS)
        {
            handle->mSPSPPSDataSize = 0;
            handle->mSPSPPSData = (uint8_t *)malloc(in_size);
            if (handle->mSPSPPSData)
            {
                handle->mSPSPPSDataSize = in_size;
                memcpy(handle->mSPSPPSData, (unsigned char *)out, handle->mSPSPPSDataSize);
                LOGAPI("get mSPSPPSData size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
            }
            handle->mNumInputFrames = 0;
            handle->mSpsPpsHeaderReceived = true;
        }
        else
        {
            LOGAPI("Encode SPS and PPS error, encoderStatus = %d. handle: %p\n", ret, (void *)handle);
            return -1;
        }
    }
    if (handle->mNumInputFrames >= 0)
    {
        AMVEncFrameIO videoInput;
        memset(&videoInput, 0, sizeof(videoInput));
        videoInput.height = handle->mEncParams.height;
        videoInput.pitch = ((handle->mEncParams.width + 15) >> 4) << 4;
        /* TODO*/
        videoInput.bitrate = handle->mEncParams.bitrate;
        videoInput.frame_rate = handle->mEncParams.frame_rate / 1000;
        videoInput.coding_timestamp = handle->mNumInputFrames * 1000 / videoInput.frame_rate;  // in ms
        //LOGAPI("mNumInputFrames %lld, videoInput.coding_timestamp %llu, videoInput.frame_rate %f\n", handle->mNumInputFrames, videoInput.coding_timestamp,videoInput.frame_rate);

        videoInput.YCbCr[0] = (unsigned long)&in[0];
        videoInput.YCbCr[1] = (unsigned long)(videoInput.YCbCr[0] + videoInput.height * videoInput.pitch);
        if (format == 0) {
            videoInput.fmt = AMVENC_NV21;
            videoInput.YCbCr[2] = 0;
        } else if (format == 1) {
            videoInput.fmt = AMVENC_YUV420;
            videoInput.YCbCr[2] = (unsigned long)(videoInput.YCbCr[1] + videoInput.height * videoInput.pitch / 4);
        } else if (format == 2) {
            videoInput.fmt = AMVENC_RGB888;
            videoInput.YCbCr[1] = 0;
            videoInput.YCbCr[2] = 0;
        } else if (format == 3) {
            videoInput.fmt = AMVENC_BGR888;
        }
        videoInput.canvas = 0xffffffff;
        videoInput.type = VMALLOC_BUFFER;
        videoInput.disp_order = handle->mNumInputFrames;
        videoInput.op_flag = 0;
        if (handle->mKeyFrameRequested == true)
        {
            videoInput.op_flag = AMVEncFrameIO_FORCE_IDR_FLAG;
            handle->mKeyFrameRequested = false;
        }
        ret = AML_HWSetInput(handle, &videoInput);
        ++(handle->mNumInputFrames);
        if (ret == AMVENC_SUCCESS || ret == AMVENC_NEW_IDR)
        {
            if (ret == AMVENC_NEW_IDR)
            {
                outPtr = (uint8_t *) out + handle->mSPSPPSDataSize;
                dataLength  = /*should be out size */in_size - handle->mSPSPPSDataSize;
            }
            else
            {
                outPtr = (uint8_t *) out;
                dataLength  = /*should be out size */in_size;
            }
        }
        else if (ret < AMVENC_SUCCESS)
        {
            LOGAPI("encoderStatus = %d at line %d, handle: %p", ret, __LINE__, (void *)handle);
            return -1;
        }

        ret = AML_HWEncNAL(handle, (unsigned char *)outPtr, (unsigned int *)&dataLength, &type);
        if (ret == AMVENC_PICTURE_READY)
        {
            if (type == AVC_NALTYPE_IDR)
            {
                if (handle->mSPSPPSData)
                {
                    memcpy((uint8_t *) out, handle->mSPSPPSData, handle->mSPSPPSDataSize);
                    dataLength += handle->mSPSPPSDataSize;
                    LOGAPI("copy mSPSPPSData to buffer size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
                }
            }
        }
        else if ((ret == AMVENC_SKIPPED_PICTURE) || (ret == AMVENC_TIMEOUT))
        {
            dataLength = 0;
            if (ret == AMVENC_TIMEOUT)
            {
                handle->mKeyFrameRequested = true;
                ret = AMVENC_SKIPPED_PICTURE;
            }
            LOGAPI("ret = %d at line %d, handle: %p", ret, __LINE__, (void *)handle);
        }
        else if (ret != AMVENC_SUCCESS)
        {
            dataLength = 0;
        }

        if (ret < AMVENC_SUCCESS)
        {
            LOGAPI("encoderStatus = %d at line %d, handle: %p", ret , __LINE__, (void *)handle);
            return -1;
        }
    }
    LOGAPI("vl_video_encoder_encode return %d\n",dataLength);
    return dataLength;
}

int vl_video_encoder_destory(vl_codec_handle_t codec_handle)
{
    AMVEncHandle *handle = (AMVEncHandle *)codec_handle;
    AML_HWEncRelease(handle);
    if (handle->mSPSPPSData)
        free(handle->mSPSPPSData);
    if (handle)
        delete handle;
    return 1;
}
