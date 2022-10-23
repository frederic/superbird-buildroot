#define LOG_TAG "libvpcodec"
#include "vpcodec_1_0.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "include/AML_HWEncoder.h"
#include "include/enc_define.h"
#include <h264bitstream/h264_stream.h>
#define LOG_LINE() printf("[%s:%d]\n", __FUNCTION__, __LINE__)
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
    } else if (height % 16 != 0) {
        LOGAPI("Video frame height is not standard:%d", height);
    } else {
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
    int dump_opts = 0;
    char *env_h264enc_dump;

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

    mHandle->fd_in = -1;
    mHandle->fd_out = -1;

    /*env_h264enc_dump = getenv("h264enc_dump");
    LOGAPI("h264enc_dump=%s\n", env_h264enc_dump);
	if (env_h264enc_dump)
		dump_opts = atoi(env_h264enc_dump);
    if (dump_opts == 1) {
        mHandle->fd_in = open("/tmp/h264enc_dump_in.raw", O_CREAT | O_WRONLY);
        if (mHandle->fd_in == -1)
            LOGAPI("OPEN file for dump h264enc input failed: %s\n", strerror(errno));
        mHandle->fd_out = -1;
    } else if (dump_opts == 2) {
        mHandle->fd_in  = -1;
        mHandle->fd_out = open("/tmp/h264enc_dump_out.264", O_CREAT | O_WRONLY);
        if (mHandle->fd_out == -1)
            LOGAPI("OPEN file for dump h264enc output failed: %s\n", strerror(errno));
    } else if (dump_opts == 3) {
        mHandle->fd_in = open("/tmp/h264enc_dump_in.raw", O_CREAT | O_WRONLY);
        if (mHandle->fd_in == -1)
            LOGAPI("OPEN file for dump h264enc input failed: %s\n", strerror(errno));

        mHandle->fd_out = open("/tmp/h264enc_dump_out.264", O_CREAT | O_WRONLY);
        if (mHandle->fd_out == -1)
            LOGAPI("OPEN file for dump h264enc output failed: %s\n", strerror(errno));
    } else {
        LOGAPI("h264enc_dump disabled\n");
        mHandle->fd_in = -1;
        mHandle->fd_out = -1;
    }*/

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
    size_t rawdata_size = in_size;
    ssize_t io_ret;
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

                size_t merge_size = sizeof(sps_t) + sizeof(pps_t) + 5 + 5;
				uint8_t *merge_buf = (uint8_t *) malloc(merge_size);
				size_t merge_pos = 0;

				uint8_t *aux_buf = (uint8_t *) malloc(handle->mSPSPPSDataSize + 5);

				if (!merge_buf || !aux_buf)
					return -1;

				memset(merge_buf, 0, merge_size);
				memset(aux_buf, 0, handle->mSPSPPSDataSize + 5);

				memcpy(aux_buf, handle->mSPSPPSData, handle->mSPSPPSDataSize);

				*((int *)(aux_buf + handle->mSPSPPSDataSize)) = 0x01000000;  // add trailing nal delimeter

				h264_stream_t* h = h264_new();
				uint8_t* p = aux_buf;
				int nal_start, nal_end, nal_size;
				int64_t off = 0;
				size_t sz = handle->mSPSPPSDataSize + 5;
				size_t cursor = 0;

				uint8_t *sps_buf = (uint8_t *) malloc(sizeof(sps_t) + 5);
				uint8_t *pps_buf = (uint8_t *) malloc(sizeof(pps_t) + 5);
				memset(sps_buf, 0, sizeof(sps_t) + 5);
				memset(pps_buf, 0, sizeof(pps_t) + 5);

				while (find_nal_unit(p, sz, &nal_start, &nal_end) > 0) {
				   printf("!! Found NAL at offset %lld (0x%04llX), size %lld (0x%04llX) \n",
						  (long long int)(off + (p - aux_buf) + nal_start),
						  (long long int)(off + (p - aux_buf) + nal_start),
						  (long long int)(nal_end - nal_start),
						  (long long int)(nal_end - nal_start) );

		            p += nal_start;
		            read_nal_unit(h, p, nal_end - nal_start);

		            p += (nal_end - nal_start);
		            sz -= nal_end;

		            if (h->nal->nal_unit_type == NAL_UNIT_TYPE_SPS) {
						h->sps->vui_parameters_present_flag = 1;
						h->sps->vui.timing_info_present_flag = 1;
						h->sps->vui.num_units_in_tick = 1;
						h->sps->vui.time_scale = handle->mEncParams.frame_rate / 500;

						nal_size = write_nal_unit(h, sps_buf, sizeof(sps_t) + 5);

		                *((int*)(merge_buf + merge_pos)) = 0x01000000;
		                merge_pos += 4;
		                memcpy(merge_buf + merge_pos, sps_buf + 1, nal_size);
		                merge_pos += nal_size;

		                free(sps_buf);
		            } else if (h->nal->nal_unit_type == NAL_UNIT_TYPE_PPS) {
		                nal_size = write_nal_unit(h, pps_buf, sizeof(pps_t) + 5);

		                *((int*)(merge_buf + merge_pos)) = 0x01000000;
		                merge_pos += 4;
		                memcpy(merge_buf + merge_pos, pps_buf + 1, nal_size);
		                merge_pos += nal_size;

		                free(pps_buf);
		            }
				}

				free(handle->mSPSPPSData);
				handle->mSPSPPSData = merge_buf;
				handle->mSPSPPSDataSize = merge_pos;
				free(aux_buf);
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

        if (format == 0) { //NV12
            videoInput.fmt = AMVENC_NV12;
            videoInput.YCbCr[2] = 0;
        } else if(format == 1) { //NV21
            videoInput.fmt = AMVENC_NV21;
            videoInput.YCbCr[2] = 0;
        } else if (format == 2) { //YV12
            videoInput.fmt = AMVENC_YUV420;
            videoInput.YCbCr[2] = (unsigned long)(videoInput.YCbCr[1] + videoInput.height * videoInput.pitch / 4);
        } else if (format == 3) { //rgb888
            videoInput.fmt = AMVENC_RGB888;
            videoInput.YCbCr[1] = 0;
            videoInput.YCbCr[2] = 0;
        } else if (format == 4) { //bgr888
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

            if (handle->fd_in >= 0) {
                io_ret = write(handle->fd_in, in, rawdata_size);
                if (io_ret == -1) {
                    printf("write raw frame failed: %s\n", errno);
                } else if (io_ret < rawdata_size) {
                    printf("write raw frame: short write %zu vs %zd\n", rawdata_size, io_ret);
                }
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
            if (handle->fd_out >= 0) {
				io_ret = write(handle->fd_out, (void *)out, dataLength);
                if (io_ret == -1) {
                    printf("write h264 packet failed: %s\n", errno);
                } else if (io_ret < dataLength) {
                printf("write raw h264 packet: short write %zu vs %zd\n", dataLength, io_ret);
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

    if (handle->fd_in >= 0) {
        fsync(handle->fd_in);
        close(handle->fd_in);
    }
    if (handle->fd_out >= 0) {
        fsync(handle->fd_out);
        close(handle->fd_out);
    }

    AML_HWEncRelease(handle);
    if (handle->mSPSPPSData)
        free(handle->mSPSPPSData);
    if (handle)
        delete handle;

    return 1;
}
