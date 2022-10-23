//#define LOG_NDEBUG 0
#define LOG_TAG "AMLVENC"
#ifdef MAKEANDROID
#include <utils/Log.h>
#endif
#include <stdio.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "AML_HWEncoder.h"
#include "enc_api.h"

AMVEnc_Status DetermineFrameNum(AMVEncHandle *Handle, amvenc_info_t* info, uint64_t modTime, uint32 new_frame_rate, bool force_IDR) {
    uint64 modTimeRef = info->modTimeRef;
    int64 currFrameNum;
    int frameInc;
    int border = 200;
    bool no_skip = false;

    /* for now, the default is to encode every frame, To Be Changed */
    if (info->first_frame) {
        info->modTimeRef = modTime;
        info->wrapModTime = 0;
        info->prevProcFrameNum = 0;
        info->prevProcFrameNumOffset = 0;
        info->lastTimeRef = 0;

        /* set frame type to IDR-frame */
        info->nal_unit_type = AVC_NALTYPE_IDR;
        info->slice_type = AVC_I_SLICE;
        info->late_frame_count = 0;
        info->frame_rate = new_frame_rate;
        return AMVENC_SUCCESS;
    } else {
        if ((info->freerun == true) || (force_IDR == true))
            no_skip = true;

        if (info->frame_rate != new_frame_rate) {
            info->prevProcFrameNumOffset += info->prevProcFrameNum;
            info->late_frame_count = 0;
            info->prevProcFrameNum = 0;
            info->modTimeRef = info->lastTimeRef;
            modTimeRef = info->modTimeRef;
            info->wrapModTime = 0;
            info->frame_rate = new_frame_rate;
            no_skip = true;
            LOGAPI("update frame_rate %d\n",new_frame_rate);
        }
        //LOGAPI("modTime %lld, modTimeRef %lld\n",modTime,modTimeRef);

        if (modTime < modTimeRef) {
            /* modTime wrapped around */
            info->wrapModTime += ((uint64) 0xFFFFFFFFFFFFFFFF - modTimeRef) + 1;
            info->modTimeRef = modTimeRef = 0;
            LOGAPI("update info->wrapModTime %lld\n",info->wrapModTime);
        }
        modTime += info->wrapModTime; /* wrapModTime is non zero after wrap-around */

        currFrameNum = (int64) (((modTime - modTimeRef) * (double) info->frame_rate + border) / 1000); /* add small roundings */
        //LOGAPI("modTime %lld, currFrameNum %lld, prevProcFrameNum %lld, late_frame_count %d\n",modTime,currFrameNum,info->prevProcFrameNum,info->late_frame_count);

        if ((currFrameNum <= (int64) (info->prevProcFrameNum - info->late_frame_count)) && (no_skip == false)) {
            LOGAPI("skip currFrameNum %lld, prevProcFrameNum %lld\n",currFrameNum, info->prevProcFrameNum);
            return AMVENC_FAIL; /* this is a late frame do not encode it */
        }

        frameInc = currFrameNum - info->prevProcFrameNum + info->late_frame_count;
        if ((currFrameNum - info->prevProcFrameNum) > 1)
            info->late_frame_count += (int) (currFrameNum - info->prevProcFrameNum - 1);
        else if ((info->late_frame_count > 0) && ((currFrameNum - info->prevProcFrameNum) == 0))
            info->late_frame_count--;

        //LOGAPI("frameInc %d, skip_next_frame %d, late_frame_count %d\n",frameInc,info->skip_next_frame,info->late_frame_count);

        if ((frameInc < info->skip_next_frame + 1) && (no_skip == false)) {
            LOGAPI("skip frameInc %d, skip_next_frame %d\n",frameInc, info->skip_next_frame);
            return AMVENC_FAIL; /* frame skip required to maintain the target bit rate. */
        }

        //AMPreRateControl(&info->hw_info, frameInc, &IDR);
        AMPreRateControl(&info->hw_info, modTime, force_IDR); //frameInc is useless in m8 ratectrl,so use it to pass timecode;
        currFrameNum += info->prevProcFrameNumOffset;
        info->lastTimeRef = modTime;

        //LOGAPI("currFrameNum %lld, lastTimeRef %lld\n",currFrameNum,info->lastTimeRef);

        if (currFrameNum >= (int64) info->frame_rate) { /* first frame or IDR*/
            //info->modTimeRef += (uint32)(info->idrPeriod * 1000 / info->frame_rate);
            info->modTimeRef = modTime; //add this to avoid next modTime too small
            currFrameNum -= info->frame_rate;
            info->prevProcFrameNum = currFrameNum;
            info->prevProcFrameNumOffset = 0;
        } else {
            info->prevProcFrameNum = currFrameNum;
        }
        //LOGAPI("currFrameNum %lld, modTimeRef %lld\n",currFrameNum,info->modTimeRef);

        if (info->frame_in_gop >= (uint) info->idrPeriod && info->idrPeriod > 0) { /* first frame or IDR*/
            info->nal_unit_type = AVC_NALTYPE_IDR;
            info->slice_type = AVC_I_SLICE;
        } else {
            if (force_IDR) {  // todo: force_IDR only need one IDR or reset gop.
                LOGAPI("force next frame to idr :%d, handle: %p.", force_IDR, Handle);
                info->nal_unit_type = AVC_NALTYPE_IDR;
                info->slice_type = AVC_I_SLICE;
            } else {
                info->nal_unit_type = AVC_NALTYPE_SLICE;
                info->slice_type = AVC_P_SLICE;
            }
        }
    }
    LOGAPI("Get Nal Type: %s,  handle: %p.", (info->nal_unit_type == AVC_NALTYPE_IDR) ? "IDR" : "SLICE", Handle);
    return AMVENC_SUCCESS;
}

AMVEnc_Status AML_HWEncInitialize(AMVEncHandle *Handle, AMVEncParams *encParam, bool* has_mix, int force_mode) {
    AMVEnc_Status status = AMVENC_FAIL;
    amvenc_info_t* info = (amvenc_info_t*) calloc(1, sizeof(amvenc_info_t));

    if ((!info) || (!Handle) || (!encParam)) {
        status = AMVENC_MEMORY_FAIL;
        goto exit;
    }

    if (encParam->frame_rate == 0) {
        status = AMVENC_INVALID_FRAMERATE;
        goto exit;
    }

    memset(info, 0, sizeof(amvenc_info_t));
    info->hw_info.dev_fd = -1;
    info->hw_info.dev_id = NO_DEFINE;

    info->enc_width = encParam->width;
    info->enc_height = encParam->height;
    info->outOfBandParamSet = encParam->out_of_band_param_set;
    info->fullsearch_enable = encParam->fullsearch;

    /* now the rate control and performance related parameters */
    info->scdEnable = encParam->auto_scd;
    info->idrPeriod = encParam->idr_period;
    info->search_range = encParam->search_range;
    info->frame_rate = (float) (encParam->frame_rate * 1.0 / 1000);

    info->rcEnable = (encParam->rate_control == AVC_ON) ? true : false;
    info->bitrate = encParam->bitrate;
    info->cpbSize = encParam->CPB_size;
    info->initDelayOffset = (info->bitrate * encParam->init_CBP_removal_delay / 1000);
    info->initQP = encParam->initQP;
    info->freerun = (encParam->FreeRun == AVC_ON) ? true : false;
    info->nSliceHeaderSpacing = encParam->nSliceHeaderSpacing;
    info->MBsIntraRefresh = encParam->MBsIntraRefresh;
    info->MBsIntraOverlap = encParam->MBsIntraOverlap;

    if (info->initQP == 0) {
#if 0
        double L1, L2, L3, bpp;
        bpp = 1.0 * info->bitrate /(info->frame_rate * (info->enc_width *info->enc_height));
        if (info->enc_width <= 176) {
            L1 = 0.1;
            L2 = 0.3;
            L3 = 0.6;
        } else if (info->enc_width <= 352) {
            L1 = 0.2;
            L2 = 0.6;
            L3 = 1.2;
        } else {
            L1 = 0.6;
            L2 = 1.4;
            L3 = 2.4;
        }

        if (bpp <= L1)
        info->initQP = 30;
        else if (bpp <= L2)
        info->initQP = 25;
        else if (bpp <= L3)
        info->initQP = 20;
        else
        info->initQP = 15;
#else
        info->initQP = 26;
#endif
    }

    info->hw_info.init_para.enc_width = info->enc_width;
    info->hw_info.init_para.enc_height = info->enc_height;
    info->hw_info.init_para.initQP = info->initQP;
    info->hw_info.init_para.nSliceHeaderSpacing = info->nSliceHeaderSpacing;
    info->hw_info.init_para.MBsIntraRefresh = info->MBsIntraRefresh;
    info->hw_info.init_para.MBsIntraOverlap = info->MBsIntraOverlap;
    info->hw_info.init_para.rcEnable = info->rcEnable;
    info->hw_info.init_para.bitrate = info->bitrate;
    info->hw_info.init_para.frame_rate = info->frame_rate;
    info->hw_info.init_para.cpbSize = info->cpbSize;
    info->hw_info.init_para.bitrate_scale = (encParam->BitrateScale == AVC_ON) ? true : false;
    info->hw_info.init_para.encode_once = encParam->encode_once;
    status = InitAMVEncode(&info->hw_info, force_mode);
    if (status != AMVENC_SUCCESS) {
        goto exit;
    }

    encParam->dev_id = info->hw_info.dev_id;
    if (AMInitRateControlModule(&info->hw_info) != AMVENC_SUCCESS) {
        status = AMVENC_MEMORY_FAIL;
        goto exit;
    }
    info->first_frame = true; /* set this flag for the first time */

    info->modTimeRef = 0; /* ALWAYS ASSUME THAT TIMESTAMP START FROM 0 !!!*/
    info->late_frame_count = 0;

    if (info->outOfBandParamSet)
        info->state = AMVEnc_Encoding_SPS;
    else
        info->state = AMVEnc_Analyzing_Frame;
    Handle->object = (void *) info;
    *has_mix = (info->hw_info.dev_id == M8) ? true : false;
    LOGAPI("AML_HWEncInitialize success, handle: %p, fd:%d", Handle, info->hw_info.dev_fd);
    return AMVENC_SUCCESS;
exit:
    if (info) {
        AMCleanupRateControlModule(&info->hw_info);
        UnInitAMVEncode(&info->hw_info);
        free(info);
    }
    LOGAPI("AML_HWEncInitialize Fail, error=%d. handle: %p", status, Handle);
    return status;
}

AMVEnc_Status AML_HWSetInput(AMVEncHandle *Handle, AMVEncFrameIO *input) {
    amvenc_info_t* info = (amvenc_info_t*) Handle->object;
    AMVEnc_Status status = AMVENC_FAIL;
    ulong yuv[13];

    if (info == NULL) {
        LOGAPI("AML_HWSetInput Fail: UNINITIALIZED. handle: %p", Handle);
        return AMVENC_UNINITIALIZED;
    }

    if (info->state == AMVEnc_WaitingForBuffer) {
        goto RECALL_INITFRAME;
    } else if (info->state != AMVEnc_Analyzing_Frame) {
        LOGAPI("AML_HWSetInput Wrong state: %d. handle: %p", info->state, Handle);
        return AMVENC_FAIL;
    }
    if (input->pitch > 0xFFFF) {
        LOGAPI("AML_HWSetInput Fail: NOT_SUPPORTED. handle: %p", Handle);
        return AMVENC_NOT_SUPPORTED; // we use 2-bytes for pitch
    }

    if (AMVENC_SUCCESS != DetermineFrameNum(Handle, info, input->coding_timestamp, (uint32) input->frame_rate, (input->op_flag & AMVEncFrameIO_FORCE_IDR_FLAG))) {
        LOGAPI("AML_HWSetInput SKIPPED_PICTURE, handle: %p", Handle);
        return AMVENC_SKIPPED_PICTURE; /* not time to encode, thus skipping */
    }

    yuv[0] = input->YCbCr[0];
    yuv[1] = input->YCbCr[1];
    yuv[2] = input->YCbCr[2];
    yuv[3] = input->canvas;
    yuv[4] = (input->op_flag & AMVEncFrameIO_FORCE_SKIP_FLAG) ? 1 : 0;
    yuv[5] = input->crop_top;
    yuv[6] = input->crop_bottom;
    yuv[7] = input->crop_left;
    yuv[8] = input->crop_right;
    yuv[9] = input->pitch;
    yuv[10] = input->height;
    yuv[11] = input->scale_width;
    yuv[12] = input->scale_height;

    RECALL_INITFRAME:
    /* initialize and analyze the frame */
    status = AMVEncodeInitFrame(&info->hw_info, &yuv[0], input->type, input->fmt, (info->nal_unit_type == AVC_NALTYPE_IDR));
    if ((status == AMVENC_NEW_IDR) && (info->nal_unit_type != AVC_NALTYPE_IDR)) {
        info->modTimeRef = input->coding_timestamp;
        info->nal_unit_type = AVC_NALTYPE_IDR;
        info->slice_type = AVC_I_SLICE;
        info->prevProcFrameNum = 0;
    }
    if (info->nal_unit_type == AVC_NALTYPE_IDR)
        info->frame_in_gop = 0;

    AMRCInitFrameQP(&info->hw_info, (info->nal_unit_type == AVC_NALTYPE_IDR), input->bitrate, input->frame_rate);

    if (status == AMVENC_SUCCESS) {
        info->state = AMVEnc_Encoding_Frame;
    } else if (status == AMVENC_NEW_IDR) {
        if (info->outOfBandParamSet)
            info->state = AMVEnc_Encoding_Frame;
        else
            // assuming that in-band paramset keeps sending new SPS and PPS.
            info->state = AMVEnc_Encoding_SPS;
    } else if (status == AMVENC_PICTURE_READY) {
        info->state = AMVEnc_WaitingForBuffer; // Input accepted but can't continue
    }
    LOGAPI("AML_HWSetInput status: %d. handle: %p", status, Handle);
    return status;
}

AMVEnc_Status AML_HWEncNAL(AMVEncHandle *Handle, unsigned char *buffer, unsigned int *buf_nal_size, int *nal_type) {
    AMVEnc_Status ret = AMVENC_FAIL;
    amvenc_info_t* info = (amvenc_info_t*) Handle->object;
    int datalen = 0;

    if (info == NULL) {
        LOGAPI("AML_HWEncNAL Fail: UNINITIALIZED. handle: %p", Handle);
        return AMVENC_UNINITIALIZED;
    }

    LOGAPI("AML_HWEncNAL state: %d. handle: %p", info->state, Handle);
    switch (info->state) {
        case AMVEnc_Initializing:
            return AMVENC_ALREADY_INITIALIZED;
        case AMVEnc_Encoding_SPS:
            /* encode SPS */
            ret = AMVEncodeSPS_PPS(&info->hw_info, buffer, &datalen);
            if (ret != AMVENC_SUCCESS) {
                LOGAPI("AML_HWEncNAL state: %d, err=%d, handle: %p", info->state, ret, Handle);
                return ret;
            }
            if (info->outOfBandParamSet)
                info->state = AMVEnc_Analyzing_Frame;
            else
                // SetInput has been called before SPS and PPS.
                info->state = AMVEnc_Encoding_Frame;

            *nal_type = AVC_NALTYPE_PPS;
            *buf_nal_size = datalen;
            break;
        case AMVEnc_Encoding_Frame:
            if (*nal_type == 0xff) {
                LOGAPI("AML_HWEncNAL Cancel Current Frame. handle: %p", Handle);
                info->state = AMVEnc_Analyzing_Frame;
                ret = AMVENC_SUCCESS;
                return ret;
            }
            ret = AMVEncodeSlice(&info->hw_info, buffer, &datalen);
            if (ret == AMVENC_TIMEOUT) {
                LOGAPI("AML_HWEncNAL state: %d, err=%d. handle: %p", info->state, ret, Handle);
                //ret = AMVENC_SKIPPED_PICTURE; //need debug
                info->state = AMVEnc_Analyzing_Frame;  // walk around to fix the poll timeout
                return ret;
            }

            if ((ret != AMVENC_PICTURE_READY) && (ret != AMVENC_SUCCESS) && (ret != AMVENC_NEW_IDR)) {
                LOGAPI("AML_HWEncNAL state: %d, err=%d. handle: %p", info->state, ret, Handle);
                return ret;
            }

            info->nal_unit_type = (ret == AMVENC_NEW_IDR) ? AVC_NALTYPE_IDR : info->nal_unit_type;
            *nal_type = info->nal_unit_type;
            *buf_nal_size = datalen;

            if ((ret == AMVENC_PICTURE_READY) || (ret == AMVENC_NEW_IDR)) {
                info->first_frame = false;
                ret = AMPostRateControl(&info->hw_info, (info->nal_unit_type == AVC_NALTYPE_IDR), &info->skip_next_frame, (datalen << 3));
                if ((ret == AMVENC_SKIPPED_PICTURE) && (info->freerun == false)) {
                    info->state = AMVEnc_Analyzing_Frame;
                    return ret;
                }
                /*we need to reencode the frame**/
                if (ret == AMVENC_REENCODE_PICTURE) {
                    ret = AMVEncodeSlice(&info->hw_info, buffer, &datalen);
                    if ((ret != AMVENC_PICTURE_READY) && (ret != AMVENC_SUCCESS) && (ret != AMVENC_NEW_IDR)) {
                        LOGAPI("AML_HWEncNAL state: %d, err=%d. handle: %p", info->state, ret, Handle);
                        return ret;
                    }
                    info->nal_unit_type = (ret == AMVENC_NEW_IDR) ? AVC_NALTYPE_IDR : info->nal_unit_type;
                    *nal_type = info->nal_unit_type;
                    *buf_nal_size = datalen;
                    if (ret == AMVENC_NEW_IDR) {
                        info->first_frame = false;
                        ret = AMPostRateControl(&info->hw_info, (info->nal_unit_type == AVC_NALTYPE_IDR)/*true*/, &info->skip_next_frame, (datalen << 3));
                        if ((ret == AMVENC_SKIPPED_PICTURE) && (info->freerun == false)) {
                            info->state = AMVEnc_Analyzing_Frame;
                            return ret;
                        }
                    } else {
                        LOGAPI("re-encode failed:%d. handle: %p", ret, Handle);
                    }
                }
                if (AMVEncodeCommit(&info->hw_info, (info->nal_unit_type == AVC_NALTYPE_IDR)) != AMVENC_SUCCESS) {
                    LOGAPI("Encode Commit  failed:%d. handle: %p", ret, Handle);
                } else {
                    info->frame_in_gop++;
                }
                info->state = AMVEnc_Analyzing_Frame;
                ret = AMVENC_PICTURE_READY;
            } else {
                ret = AMVENC_SUCCESS;
            }
            break;
        default:
            ret = AMVENC_WRONG_STATE;
    }
    LOGAPI("AML_HWEncNAL next state: %d, ret=%d. handle: %p", info->state, ret, Handle);
    return ret;
}

AMVEnc_Status AML_HWEncRelease(AMVEncHandle *Handle) {
    amvenc_info_t* info = (amvenc_info_t*) Handle->object;
    if (info) {
        AMCleanupRateControlModule(&info->hw_info);
        UnInitAMVEncode(&info->hw_info);
        free(info);
    }
    Handle->object = NULL;
    LOGAPI("AML_HWEncRelease Done, handle: %p", Handle);
    return AMVENC_SUCCESS;
}

