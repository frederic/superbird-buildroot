#include "vp_hevc_codec_1_0.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef MAKEANDROID
#include <utils/Log.h>
#define LOGCAT
#endif

#include "include/AML_HEVCEncoder.h"
#include "include/enc_define.h"

const char version[] = "Amlogic libvp_hevc_codec version 1.0";

const char *vl_get_version() {
    return version;
}

AMVEnc_Status initEncParams(AMVHEVCEncHandle *handle, int width, int height, int frame_rate, int bit_rate, int gop) {
    memset(&(handle->mEncParams), 0, sizeof(AMVHEVCEncParams));
    VLOG(DEBUG, "bit_rate:%d", bit_rate);
    if ((width % 16 != 0 || height % 2 != 0)) {
        VLOG(DEBUG, "Video frame size %dx%d must be a multiple of 16", width, height);
        //return -1;
    } else if (height % 16 != 0) {
        VLOG(DEBUG, "Video frame height is not standard:%d", height);
    } else {
        VLOG(DEBUG, "Video frame size is %d x %d", width, height);
    }
    handle->mEncParams.rate_control = HEVC_OFF;
    handle->mEncParams.initQP = 0;
    handle->mEncParams.init_CBP_removal_delay = 1600;
    handle->mEncParams.auto_scd = HEVC_OFF;
    handle->mEncParams.out_of_band_param_set = HEVC_OFF;
    handle->mEncParams.num_ref_frame = 1;
    handle->mEncParams.num_slice_group = 1;
    //handle->mEncParams.nSliceHeaderSpacing = 0;
    handle->mEncParams.fullsearch = HEVC_OFF;
    handle->mEncParams.search_range = 16;
    //handle->mEncParams.sub_pel = HEVC_OFF;
    //handle->mEncParams.submb_pred = HEVC_OFF;
    handle->mEncParams.width = width;
    handle->mEncParams.height = height;
    handle->mEncParams.src_width = width;
    handle->mEncParams.src_height = height;
    handle->mEncParams.bitrate = bit_rate;
    handle->mEncParams.frame_rate = frame_rate;
    handle->mEncParams.CPB_size = (uint32)(bit_rate >> 1);
    handle->mEncParams.FreeRun = HEVC_OFF;
    handle->mEncParams.MBsIntraRefresh = 0;
    handle->mEncParams.MBsIntraOverlap = 0;
    handle->mEncParams.encode_once = 1;
    // Set IDR frame refresh interval
    if (gop <= 0) {
        handle->mEncParams.idr_period = 0;   //an infinite period, only one I frame
    } else {
        handle->mEncParams.idr_period = gop; //period of I frame, 1 means all frames are I type.
    }
    VLOG(DEBUG, "mEncParams.idrPeriod: %d, gop %d\n", handle->mEncParams.idr_period, gop);
    // Set profile and level
    handle->mEncParams.profile = HEVC_MAIN;
    handle->mEncParams.level = HEVC_LEVEL_NONE; // firmware determines a level.
    handle->mEncParams.tier = HEVC_TIER_MAIN;
    handle->mEncParams.initQP = 30;
    handle->mEncParams.BitrateScale = HEVC_OFF;
    handle->mEncParams.refresh_type = HEVC_CRA;
    return AMVENC_SUCCESS;
}


vl_codec_handle_t vl_video_encoder_init(vl_codec_id_t codec_id, int width, int height, int frame_rate, int bit_rate, int gop) {
    AMVEnc_Status ret;
    AMVHEVCEncHandle *mHandle = new AMVHEVCEncHandle;
    bool has_mix = false;
    (void)codec_id;

    if (mHandle == NULL)
        goto exit;
    memset(mHandle, 0, sizeof(AMVHEVCEncHandle));
    ret = initEncParams(mHandle, width, height, frame_rate, bit_rate, gop);
    if (ret < AMVENC_SUCCESS)
        goto exit;
    ret = AML_HEVCInitialize(mHandle, &(mHandle->mEncParams), &has_mix, 2);
    if (ret < AMVENC_SUCCESS)
        goto exit;
    mHandle->mSpsPpsHeaderReceived = false;
    mHandle->mNumInputFrames = -1;  // 1st two buffers contain SPS and PPS
    return (vl_codec_handle_t) mHandle;
exit:
    if (mHandle != NULL)
        delete mHandle;
    return (vl_codec_handle_t) NULL;
}

int vl_video_encoder_encode(vl_codec_handle_t codec_handle, vl_frame_type_t frame_type, unsigned char *in, unsigned int outputBufferLen, unsigned char *out, int format) {
    AMVEnc_Status ret;
    uint32_t dataLength = 0;
    int type = 0;
    AMVHEVCEncHandle *handle = (AMVHEVCEncHandle *)codec_handle;

    (void)frame_type;

    handle->mOutputBufferLen = outputBufferLen;
    if (!handle->mSpsPpsHeaderReceived) {
        ret = AML_HEVCEncHeader(handle, (unsigned char *)out, (unsigned int *)&dataLength, &type);
        if (ret == AMVENC_SUCCESS) {
            handle->mSPSPPSDataSize = 0;
            handle->mSPSPPSData = (uint8_t *)malloc(dataLength);
            if (handle->mSPSPPSData) {
                handle->mSPSPPSDataSize = dataLength;
                memcpy(handle->mSPSPPSData, (unsigned char *)out, handle->mSPSPPSDataSize);
                VLOG(DEBUG, "get mSPSPPSData size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
            }
            handle->mNumInputFrames = 0;
            handle->mSpsPpsHeaderReceived = true;
        } else {
            VLOG(ERR, "Encode SPS and PPS error, encoderStatus = %d. handle: %p\n", ret, (void *)handle);
            if (ret == AMVENC_OVERFLOW)
                return ERR_OVERFLOW;
            else
                return ERR_HARDWARE;
        }
    }
    if (handle->mNumInputFrames >= 0) {
        AMVHEVCEncFrameIO videoInput;
        memset(&videoInput, 0, sizeof(videoInput));
        videoInput.height = handle->mEncParams.height;
        videoInput.pitch = handle->mEncParams.width;//((handle->mEncParams.width + 15) >> 4) << 4;
        /* TODO*/
        videoInput.bitrate = handle->mEncParams.bitrate;
        videoInput.frame_rate = handle->mEncParams.frame_rate / 1000;
        videoInput.coding_timestamp = handle->mNumInputFrames * 1000 / videoInput.frame_rate;  // in ms
        videoInput.YCbCr[0] = (unsigned long)&in[0];
        videoInput.YCbCr[1] = (unsigned long)(videoInput.YCbCr[0] + videoInput.height * videoInput.pitch);
        if (format == 0) {
            videoInput.fmt = AMVENC_NV21;
            videoInput.YCbCr[2] = 0;
        } else if (format == 1) {
            videoInput.fmt = AMVENC_NV12;
            videoInput.YCbCr[2] = 0;
        } else if (format == 2) {
            videoInput.fmt = AMVENC_RGB888;
            videoInput.YCbCr[1] = 0;
            videoInput.YCbCr[2] = 0;
        } else {
            videoInput.fmt = AMVENC_YUV420;
            videoInput.YCbCr[2] = (unsigned long)(videoInput.YCbCr[1] + videoInput.height * videoInput.pitch / 4);
        }
        videoInput.canvas = 0xffffffff;
        videoInput.type = VMALLOC_BUFFER;
        videoInput.disp_order = handle->mNumInputFrames;
        videoInput.op_flag = 0;
        //if(handle->mNumInputFrames == 0)
           //handle->mKeyFrameRequested = true;
        if (handle->mKeyFrameRequested == true) {
            videoInput.op_flag = AMVEncFrameIO_FORCE_IDR_FLAG;
            handle->mKeyFrameRequested = false;
            VLOG(INFO, "Force encode a IDR frame at %d frame", handle->mNumInputFrames);
        }
        //if (handle->idrPeriod == 0) {
            //videoInput.op_flag |= AMVEncFrameIO_FORCE_IDR_FLAG;
        //}
        ret = AML_HEVCSetInput(handle, &videoInput);
        ++(handle->mNumInputFrames);

        VLOG(DEBUG, "AML_HEVCSetInput ret %d\n", ret);
        if (ret == AMVENC_SUCCESS) {
        } else if (ret < AMVENC_SUCCESS) {
            VLOG(ERR, "encoderStatus = %d at line %d, handle: %p", ret, __LINE__, (void *)handle);
            if (ret == AMVENC_NOT_SUPPORTED)
                return ERR_NOTSUPPORT;
            else
                return ERR_HARDWARE;
        }

        ret = AML_HEVCEncNAL(handle, out, (unsigned int *)&dataLength, &type);
        if (ret == AMVENC_PICTURE_READY) {
            if (type == HEVC_IDR || type == HEVC_CRA  || type == HEVC_BLA) {
                if ((handle->mPrependSPSPPSToIDRFrames || handle->mNumInputFrames == 1) && (handle->mSPSPPSData)) {
                    if (handle->mSPSPPSDataSize + dataLength > handle->mOutputBufferLen) {
                        VLOG(ERR, "nal size %d bigger than output buffer %d!\n", handle->mSPSPPSDataSize + dataLength, handle->mOutputBufferLen);
                        return ERR_OVERFLOW;
                    }
                    memmove(out + handle->mSPSPPSDataSize, out, dataLength);
                    memcpy(out, handle->mSPSPPSData, handle->mSPSPPSDataSize);
                    dataLength += handle->mSPSPPSDataSize;
                    VLOG(DEBUG, "copy mSPSPPSData to buffer size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
                }
            }
        } else if ((ret == AMVENC_SKIPPED_PICTURE) || (ret == AMVENC_TIMEOUT)) {
            dataLength = 0;
            if (ret == AMVENC_TIMEOUT) {
                handle->mKeyFrameRequested = true;
                ret = AMVENC_SKIPPED_PICTURE;
            }
            VLOG(DEBUG, "ret = %d at line %d, handle: %p", ret, __LINE__, (void *)handle);
        } else if (ret != AMVENC_SUCCESS) {
            dataLength = 0;
            VLOG(ERR, "encoderStatus = %d at line %d, handle: %p", ret , __LINE__, (void *)handle);
            if (ret == AMVENC_OVERFLOW)
                return ERR_OVERFLOW;
            else
                return ERR_HARDWARE;
        }
    }
    return dataLength;
}

int vl_video_encoder_destory(vl_codec_handle_t codec_handle) {
    AMVHEVCEncHandle *handle = (AMVHEVCEncHandle *)codec_handle;
    AML_HEVCRelease(handle);
    if (handle->mSPSPPSData)
        free(handle->mSPSPPSData);
    if (handle)
        delete handle;
    return 1;
}
