#include <stdio.h>
#include <stdlib.h>

#ifdef MAKEANDROID
#include <utils/Log.h>
#endif

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <queue>
#include "vpcodec_1_0.h"
#include "include/AML_HWEncoder.h"
#include "include/enc_define.h"


const char version[] = "Amlogic libvpcodec version 1.0";
#define SEGMENT_LENGTH 3

const char* vl_get_version() {
    return version;
}

int initEncParams(AMVEncHandle* handle,
                  encode_info_t encode_info,
                  qp_param_t* qp_tbl,
                  amvenc_frame_stat_t* frame_info) {

    int width = encode_info.width;
    int height = encode_info.height;
    memset(&(handle->mEncParams), 0, sizeof(AMVEncParams));
    LOGAPI("bit_rate:%d", encode_info.bit_rate);
    if ((width % 16 != 0 || height % 2 != 0))
    {
        LOGAPI("Video frame size %dx%d must be a multiple of 16", width, height);
        return -1;
    } else if (height % 16 != 0) {
        LOGAPI("Video frame height is not standard:%d\n", height);
    } else {
        LOGAPI("Video frame size is %d x %d\n", width, height);
    }
    handle->mEncParams.frame_stat_info.mv_perblock =
      reinterpret_cast<mv_perblock_t*>(calloc(
          1,
          sizeof(mv_perblock_t) * ((width + 15) >> 4) * ((height + 15) >> 4)));
    if (handle->mEncParams.frame_stat_info.mv_perblock == NULL)
        return -1;
    handle->mEncParams.frame_stat_info.mv_hist_bias = 200;
    frame_info = &handle->mEncParams.frame_stat_info;
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
    handle->mEncParams.bitrate = encode_info.bit_rate;
    handle->mEncParams.frame_rate = 1000 * encode_info.frame_rate;  // In frames/ms!
    handle->mEncParams.CPB_size = (uint32)(encode_info.bit_rate >> 1);
    handle->mEncParams.FreeRun = AVC_OFF;
    handle->mEncParams.MBsIntraRefresh = 0;
    handle->mEncParams.MBsIntraOverlap = 0;
    handle->mEncParams.encode_once = 1;

    if (encode_info.img_format == IMG_FMT_NV12) {
      LOGAPI("[%s %d] img_format is IMG_FMT_NV12 \n", __func__, __LINE__);
      handle->mEncParams.fmt = AMVENC_NV12;
    } else if (encode_info.img_format == IMG_FMT_NV21) {
      LOGAPI("[%s %d] img_format is IMG_FMT_NV21 \n", __func__, __LINE__);
      handle->mEncParams.fmt = AMVENC_NV21;
    } else if (encode_info.img_format == IMG_FMT_YV12) {
      LOGAPI("[%s %d] img_format is IMG_FMT_YUV420 \n", __func__, __LINE__);
      handle->mEncParams.fmt = AMVENC_YUV420;
    } else if (encode_info.img_format == IMG_FMT_RGB888) {
      handle->mEncParams.fmt = AMVENC_RGB888;
    } else if (encode_info.img_format == IMG_FMT_BGR888) {
      handle->mEncParams.fmt = AMVENC_BGR888;
    }

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
    if (encode_info.gop == 0 || encode_info.gop < 0) {
        handle->mEncParams.idr_period = 0;   //an infinite period, only one I frame
    } else {
        handle->mEncParams.idr_period = encode_info.gop; //period of I frame, 1 means all frames are I type.
    }
    // Set profile and level
    handle->mEncParams.profile = AVC_BASELINE;
    handle->mEncParams.level = AVC_LEVEL4;
    handle->mEncParams.qp_mode = encode_info.qp_mode;
    handle->mEncParams.qp_param = qp_tbl;
    handle->mEncParams.initQP = qp_tbl->qp_I_base;
    handle->mEncParams.initQP_P = qp_tbl->qp_P_base;
    handle->mEncParams.BitrateScale = AVC_OFF;
    return 0;
}


bool check_qp_tbl(const qp_param_t* qp_tbl) {
  if (qp_tbl == NULL) {
    return false;
  }
  if (qp_tbl->qp_min < 0 || qp_tbl->qp_min > 51 || qp_tbl->qp_max < 0 ||
      qp_tbl->qp_max > 51) {
    LOGAPI("qp_min or qp_max out of range [0, 51]\n");
    return false;
  }
  if (qp_tbl->qp_I_base < 0 || qp_tbl->qp_I_base > 51 ||
      qp_tbl->qp_P_base < 0 || qp_tbl->qp_P_base > 51) {
    LOGAPI("qp_I_base or qp_P_base out of range [0, 51]\n");
    return false;
  }
  if (qp_tbl->qp_min > qp_tbl->qp_max || qp_tbl->qp_I_min > qp_tbl->qp_I_max ||
      qp_tbl->qp_P_min > qp_tbl->qp_P_max) {
    LOGAPI("min qp larger than max qp\n");
    return false;
  }
  if (qp_tbl->qp_I_min < qp_tbl->qp_min || qp_tbl->qp_I_min > qp_tbl->qp_max ||
      qp_tbl->qp_I_max < qp_tbl->qp_min || qp_tbl->qp_I_max > qp_tbl->qp_max) {
    LOGAPI("qp_min_I or qp_max_I out of range [qp_min, qp_max]\n");
    return false;
  }
  if (qp_tbl->qp_P_min < qp_tbl->qp_min || qp_tbl->qp_P_min > qp_tbl->qp_max ||
      qp_tbl->qp_P_max < qp_tbl->qp_min || qp_tbl->qp_P_max > qp_tbl->qp_max) {
    LOGAPI("qp_min_P or qp_max_P out of range [qp_min, qp_max]\n");
    return false;
  }
  return true;
}
vl_codec_handle_t vl_video_encoder_init(vl_codec_id_t codec_id,
                                        encode_info_t encode_info,
                                        qp_param_t* qp_tbl,
                                        amvenc_frame_stat_t* frame_info) {
    int ret;
    AMVEncHandle *mHandle = new AMVEncHandle;
    bool has_mix = false;
    if (mHandle == NULL)
        goto exit;

    if (!check_qp_tbl(qp_tbl)) {
        goto exit;
    }

    mHandle->object = NULL;
    mHandle->userData = NULL;
    mHandle->debugEnable = 0;
    mHandle->mSPSPPSData = NULL;
    mHandle->mSPSPPSDataSize = 0;
    mHandle->mKeyFrameRequested = 0;
    mHandle->mSpsPpsHeaderReceived = false;
    mHandle->mNumInputFrames = -1;  // 1st two buffers contain SPS and PPS
    mHandle->mEnableVFR = false;
    mHandle->mTotalSizeInSegment = 0;
    mHandle->mTotalFrameInSegment = 0;
    mHandle->mMinFramerate = encode_info.frame_rate;
    mHandle->mCurrFramerate = encode_info.frame_rate;
    mHandle->mSkippedFrame = 0;
    mHandle->mContinuousSkipCnt = 0;
    mHandle->mLastSize = 0;
    mHandle->mLastTwoSize = 0;
    mHandle->mLastType = 0;
    mHandle->mLastTwoType = 0;
    ret = initEncParams(mHandle, encode_info, qp_tbl, frame_info);
    if (ret < 0) {
      LOGAPI("initEncParams is failed\n\n");
      goto exit;
    }
      ret = AML_HWEncInitialize(mHandle, &(mHandle->mEncParams), &has_mix, 2);
    if (ret < 0) {
      LOGAPI("AML_HWEncInitialize is failed\n\n");
      goto exit;
    }

    return (vl_codec_handle_t) mHandle;
exit:
    if (mHandle != NULL)
        delete mHandle;
    return (vl_codec_handle_t) NULL;
}

int vl_video_encoder_encode(vl_codec_handle_t codec_handle,
                            vl_frame_type_t frame_type,
                            unsigned char* out,
                            buffer_info_t* buffer_info,
                            vl_param_runtime_t param_runtime) {

    int ret;
    uint8_t *outPtr = NULL;
    uint32_t dataLength = 0;
    int type;
    AMVEncHandle *handle = (AMVEncHandle *)codec_handle;
  if (buffer_info == NULL) {
    LOGAPI("invalid buffer_info\n");
    return -1;
  } else if (!out) {
    LOGAPI("vl_video_encoder_encode, out[mmap] is null\n");
    return -1;
  }

  *(param_runtime.idr) = 0;
  handle->bufType = (AMVEncBufferType)(buffer_info->buf_type);

  handle->mEnableVFR = param_runtime.enable_vfr;
  handle->mMinFramerate = param_runtime.min_frame_rate;
    if (!handle->mSpsPpsHeaderReceived) {
        ret = AML_HWEncNAL(handle, (unsigned char *)out, (unsigned int *)&dataLength, &type);
        if (ret == AMVENC_SUCCESS) {

            handle->mSPSPPSDataSize = 0;
            handle->mSPSPPSData = (uint8_t *)malloc(dataLength);
            if (handle->mSPSPPSData) {
                handle->mSPSPPSDataSize = dataLength;
                memcpy(handle->mSPSPPSData, (unsigned char *)out, handle->mSPSPPSDataSize);
                LOGAPI("get mSPSPPSData size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
            }
            handle->mNumInputFrames = 0;
            handle->mSpsPpsHeaderReceived = true;
        } else {
            LOGAPI("Encode SPS and PPS error, encoderStatus = %d. handle: %p\n", ret, (void *)handle);
            return -1;
        }
    }
    if (handle->mNumInputFrames >= 0) {
        AMVEncFrameIO videoInput;
        memset(&videoInput, 0, sizeof(videoInput));
        videoInput.height = handle->mEncParams.height;
        videoInput.pitch = ((handle->mEncParams.width + 15) >> 4) << 4;
        /* TODO*/
        videoInput.bitrate = handle->mEncParams.bitrate;
        videoInput.frame_rate = handle->mEncParams.frame_rate / 1000;

        videoInput.coding_timestamp = (uint64_t)(handle->mNumInputFrames) * 1000 / videoInput.frame_rate;  // in ms
        LOGAPI("mNumInputFrames %d, coding_timestamp %llu, frame_rate %f\n", handle->mNumInputFrames, videoInput.coding_timestamp,videoInput.frame_rate);

        videoInput.fmt = handle->mEncParams.fmt;
        if (handle->bufType == DMA_BUFF) {
            dma_info_t* dma_info;
            dma_info = &(buffer_info->buf_info.dma_info);
            if (videoInput.fmt == AMVENC_NV21 || videoInput.fmt == AMVENC_NV12) {
                if (dma_info->num_planes != 1
                    && dma_info->num_planes != 2) {
                    LOGAPI("invalid num_planes %d\n", dma_info->num_planes);
                    return -1;
                }
            } else if (videoInput.fmt == AMVENC_YUV420) {
                if (dma_info->num_planes != 1
                    && dma_info->num_planes != 3) {
                    LOGAPI("YV12 invalid num_planes %d\n", dma_info->num_planes);
                    return -1;
                }
            }
            videoInput.num_planes = dma_info->num_planes;
            for (uint32 i = 0; i < dma_info->num_planes; i++) {
                if (dma_info->shared_fd[i] < 0) {
                    LOGAPI("invalid dma_fd %d\n", dma_info->shared_fd[i]);
                    return -1;
                }
                videoInput.YCbCr[i] = dma_info->shared_fd[i];
                LOGAPI("dma_fd %d\n", dma_info->shared_fd[i]);
            }
        } else {
            unsigned char* in = buffer_info->buf_info.in_ptr;
            videoInput.YCbCr[0] = (unsigned long)&in[0];
            videoInput.YCbCr[1] = (unsigned long)(videoInput.YCbCr[0] + videoInput.height * videoInput.pitch);
            if (videoInput.fmt == AMVENC_NV21 || videoInput.fmt == AMVENC_NV12) {
                videoInput.YCbCr[2] = 0;
            } else if (videoInput.fmt == AMVENC_RGB888 || (videoInput.fmt == AMVENC_BGR888)) {
                videoInput.YCbCr[1] = 0;
                videoInput.YCbCr[2] = 0;
            } else if (videoInput.fmt == AMVENC_YUV420) {
                videoInput.YCbCr[2] = (unsigned long)(videoInput.YCbCr[1] + videoInput.height * videoInput.pitch / 4);
            }
        }
        videoInput.canvas = 0xffffffff;
        videoInput.type = handle->bufType;
        videoInput.disp_order = handle->mNumInputFrames;
        videoInput.op_flag = 0;

        if (handle->mKeyFrameRequested == true) {
            videoInput.op_flag = AMVEncFrameIO_FORCE_IDR_FLAG;
            handle->mKeyFrameRequested = false;
        }

        ret = AML_HWSetInput(handle, &videoInput);
        ++(handle->mNumInputFrames);

        ++(handle->mTotalFrameInSegment);

        if (ret == AMVENC_SKIPPED_PICTURE) {
          handle->frameQueue.push(0);
          if (handle->mNumInputFrames > (int)(handle->mEncParams.frame_rate / 1000) * SEGMENT_LENGTH) {
            handle->mTotalSizeInSegment -= handle->frameQueue.front();
            handle->frameQueue.pop();
            --(handle->mTotalFrameInSegment);
          }
          return 0;
        }

        if (ret == AMVENC_SUCCESS || ret == AMVENC_NEW_IDR) {
            if (ret == AMVENC_NEW_IDR) {

                outPtr = (uint8_t *) out + handle->mSPSPPSDataSize;
            } else {
                outPtr = (uint8_t *) out;
            }
    } else if (ret == AMVENC_SKIPPED_PICTURE) {
      LOGAPI("encoderStatus = %d at line %d, handle: %p", ret, __LINE__,
             (void*)handle);
      return 0;
    } else if (ret < AMVENC_SUCCESS) {
      LOGAPI("encoderStatus = %d at line %d, handle: %p", ret, __LINE__,
             (void*)handle);
      return -1;
    }

        ret = AML_HWEncNAL(handle, (unsigned char *)outPtr, (unsigned int *)&dataLength, &type);
        if (ret == AMVENC_PICTURE_READY) {
            if (type == AVC_NALTYPE_IDR) {
                if (handle->mSPSPPSData) {
                    memcpy((uint8_t *) out, handle->mSPSPPSData, handle->mSPSPPSDataSize);
                    dataLength += handle->mSPSPPSDataSize;
                    LOGAPI("copy mSPSPPSData to buffer size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
                    *(param_runtime.idr) = 1;
                }
            }
        } else if ((ret == AMVENC_SKIPPED_PICTURE) || (ret == AMVENC_TIMEOUT)) {
            dataLength = 0;
            if (ret == AMVENC_TIMEOUT) {
                handle->mKeyFrameRequested = true;
                ret = AMVENC_SKIPPED_PICTURE;
            }

            LOGAPI("ret = %d at line %d, handle: %p", ret, __LINE__, (void *)handle);
        } else if (ret != AMVENC_SUCCESS) {
            dataLength = 0;
        }

        if (ret < AMVENC_SUCCESS) {
            LOGAPI("encoderStatus = %d at line %d, handle: %p", ret , __LINE__, (void *)handle);

            return -1;
        }
    }
    LOGAPI("vl_video_encoder_encode return %d, type %d\n", dataLength, type);
    handle->mLastTwoSize = handle->mLastSize;
    handle->mLastSize = dataLength;
    handle->mLastTwoType = handle->mLastType;
    handle->mLastType = type;
    handle->mTotalSizeInSegment += dataLength;
    handle->frameQueue.push(dataLength);
    if (handle->mNumInputFrames > (int)(handle->mEncParams.frame_rate / 1000) * SEGMENT_LENGTH) {
      handle->mTotalSizeInSegment -= handle->frameQueue.front();
      handle->frameQueue.pop();
      --(handle->mTotalFrameInSegment);
    }
    return dataLength;
}

static void clear(std::queue<int>* queue) {
  std::queue<int> empty;
  std::swap(*queue, empty);
}

int vl_video_encoder_destroy(vl_codec_handle_t codec_handle) {
    AMVEncHandle *handle = (AMVEncHandle *)codec_handle;
    AML_HWEncRelease(handle);
    if (handle->mSPSPPSData)
        free(handle->mSPSPPSData);
    clear(&handle->frameQueue);
    if (handle->mEncParams.frame_stat_info.mv_perblock) {
      LOGAPI(
          "inside vl_video_encoder_destory "
          "handle->mEncParams.frame_stat_info.mv_perblock = %p\n",
          handle->mEncParams.frame_stat_info.mv_perblock);
      free(handle->mEncParams.frame_stat_info.mv_perblock);
      handle->mEncParams.frame_stat_info.mv_perblock = NULL;
    }
    if (handle)
        delete handle;
    return 1;
}
