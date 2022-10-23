/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <pthread.h>
#include <stdbool.h>
#include <libavformat/avformat.h>
#include "demux.h"

#define H2645(format) ((format) == AV_CODEC_ID_H264 || (format) == AV_CODEC_ID_H265)
#define WORD_BE(p) ((p[0]<<8) | p[1])

#define HEVC_NAL_TYPE(b) ((b>>1)&0x3F)
#define HEVC_NAL_VPS 32
#define HEVC_NAL_SPS 33
#define HEVC_NAL_PPS 34

extern int ffmpeg_log;
/* ffmpeg data structure */
static AVStream *video_stream = NULL;
static AVFormatContext *fmt_ctx = NULL;
static int video_stream_idx = -1;

/* for h264/265 */
static int raw_stream;
static int keyframe_seen;
static int nal_size;
static uint8_t s_nal_3[3] = {0,0,1};
static uint8_t s_nal_4[4] = {0,0,0,1};

static struct dmx_cb *dec_cb;
static pthread_t dmx_t;
static int thread_quit;
static struct dmx_v_data v_data;
static int video_frame_count = 0;

static void dump(const char* path, uint8_t *data, int size) {
    FILE* fd = fopen(path, "wb");
    if (!fd)
        return;
    fwrite(data, 1, size, fd);
    fflush(fd);
    fclose(fd);
}

static int insert_nal() {
    int size;
    if (nal_size == 3) {
        dec_cb->write(s_nal_3, sizeof(s_nal_3));
        size = 3;
    } else {
        dec_cb->write(s_nal_4, sizeof(s_nal_4));
        size = 4;
    }
    return size;
}

#if 0
static bool check_nal(uint8_t *data, int size) {
    if (size < 4)
        return false;
    if (nal_size == 3 && data[0] == 0 &&
            data[1] == 0 && data[2] == 1)
        return true;
    if (nal_size == 4 && data[0] == 0 &&
            data[1] == 0 && data[2] == 0 && data[3] == 1)
        return true;
    return false;
}
#endif

static int demux_packet(AVPacket* pkt)
{
    int decoded = pkt->size;
    int64_t pts_us = 0;
    AVRational ns_r = {1,DMX_SECOND};

    if (pkt->stream_index != video_stream_idx)
        return decoded;

    if (pkt->pts != AV_NOPTS_VALUE)
        pts_us = av_rescale_q(pkt->pts, video_stream->time_base, ns_r);

    /* video frame */
    video_frame_count++;
#ifdef DEBUG_FRAME
    printf("video_frame n:%d pts:%llx size:%x\n",
            video_frame_count,
            pts_us, pkt->size);
#endif

    /* refer to ffmpeg hevc_mp4toannexb_filter()
     * and h264_extradata_to_annexb()
     */
    if (H2645(video_stream->codecpar->codec_id)) {
        uint8_t *p = pkt->data;
        int length_size = nal_size;
        int nalu_size = 0;
        int processed = 0;

        /* From raw ES */
        if (raw_stream) {
            dec_cb->write(pkt->data, pkt->size);
            processed = pkt->size;
            //printf("raw es frame\n");
        }

        if (!keyframe_seen && !(pkt->flags & AV_PKT_FLAG_KEY)) {
            printf("drop none I frame at beginning\n");
            return decoded;
        }
        if (!keyframe_seen && (pkt->flags & AV_PKT_FLAG_KEY)) {
            printf("Got I frame at beginning\n");
            keyframe_seen = 1;
        }
        /* data inside pkt->data is like:
         * len0 + data0 + len1 + data1 ... + lenN + dataN
         * lenX depends on the nal_size
         */
        while (processed < pkt->size) {
            int i;

            for (i = 0; i < length_size; i++)
                nalu_size = (nalu_size << 8) | (*p++);
            processed += length_size;

            insert_nal();
            dec_cb->write(p, nalu_size);

#ifdef DEBUG_FRAME
            printf("nalu_size(%04x) %02x %02x %02x %02x",
                    nalu_size, p[0], p[1], p[2], p[3]);
#endif

            p += nalu_size;
            processed += nalu_size;

#ifdef DEBUG_FRAME
            printf(" ... %02x %02x %02x %02x\n",
                    *(p-3), *(p-2), *(p-1), *p);
#endif
        }
    } else {
        dec_cb->write(pkt->data, pkt->size);
    }
    dec_cb->frame_done(pts_us);

    return decoded;
}

static int open_codec_context(int *stream_idx,
        AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret, stream_index;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        return ret;
    } else {
        stream_index = ret;
        *stream_idx = stream_index;
    }

    return 0;
}

/*
 * Refer to h264_parse.c ff_h264_decode_extradata()
 * parse AVCDecoderConfigurationRecord
 * See MPEG-4 Part 15 "Advanced Video Coding (AVC) file format" section 5.2.4.1 for details.
 * data will be release outside
 */
static int h264_header_parse(uint8_t *data, int size,
        uint8_t **o_data, int *o_size, int *nal_size) {
    int i, cnt, nalsize;
    const uint8_t *p = data;
    int cur_size = 0;

    if (!data || size <= 0)
        return -1;

    if (data[0] != 1) {
        if (size < 7) return -1;
        if (data[0] == 0 && data[1] == 0 && data[2] == 1)
            *nal_size = 3;
        else if (data[0] == 0 && data[1] == 0 && data[3] == 0 && data[4] == 1)
            *nal_size = 4;
        else {
            printf("wrong header\n");
            return -1;
        }

        raw_stream = 1;
        *o_data = (uint8_t *)malloc(4096);
        if (!*o_data) {
            printf("oom");
            return -1;
        }
        memcpy(*o_data, data, size);
        *o_size = size;
        return 0;
    }

    if (size < 7) {
        printf("avcC %d too short\n", size);
        return -1;
    }

    *o_data = (uint8_t *)malloc(4096);
    if (!*o_data) {
        printf("oom");
        return -1;
    }

    // Store right nal length size that will be used to parse all other nals
    *nal_size = (data[4] & 0x03) + 1;

    // Decode sps from avcC
    cnt = *(p + 5) & 0x1f; // Number of sps
    printf("sps cnt:%d\n", cnt);

    //NAL
    if (cnt) {
        if (*nal_size == 3) {
            memcpy(*o_data + cur_size, s_nal_3, 3);
            cur_size += 3;
        } else if (*nal_size == 4) {
            memcpy(*o_data + cur_size, s_nal_4, 4);
            cur_size += 4;
        } else {
            printf("invalid nal size:%d\n", *nal_size);
            goto err_exit;
        }
    }

    p += 6;
    for (i = 0; i < cnt; i++) {
        nalsize = WORD_BE(p);
        p += 2;
        if (nalsize > size - (p - data))
            goto err_exit;
        memcpy(*o_data + cur_size, p, nalsize);
        p += nalsize;
        cur_size += nalsize;
    }
    // Decode pps from avcC
    cnt = *(p++); // Number of pps
    printf("pps cnt:%d\n", cnt);

    //NAL
    if (cnt) {
        if (*nal_size == 3) {
            memcpy(*o_data + cur_size, s_nal_3, 3);
            cur_size += 3;
        } else if (*nal_size == 4) {
            memcpy(*o_data + cur_size, s_nal_4, 4);
            cur_size += 4;
        } else {
            printf("invalid nal size:%d\n", *nal_size);
            goto err_exit;
        }
    }

    for (i = 0; i < cnt; i++) {
        nalsize = WORD_BE(p);
        p += 2;
        if (nalsize > size - (p - data))
            goto err_exit;
        memcpy(*o_data + cur_size, p, nalsize);
        p += nalsize;
        cur_size += nalsize;
    }
    printf("header parse done. cur_size:%d\n", cur_size);
    *o_size = cur_size;
    return 0;
err_exit:
    free(*o_data);
    return -2;
}

/*
 * Refer to hevc_parse.c ff_hevc_decode_extradata()
 * hvcC
 * data will be release outside
 */
static int h265_header_parse(uint8_t *data, int size,
        uint8_t **o_data, int *o_size, int *nal_size) {
    const uint8_t *p = data;
    int i, cur_size = 0;
    int num_arrays;

    if (!data || size <= 0)
        return -1;

    if (size < 7) {
        printf("header %d too short\n", size);
        return -1;
    }

    *o_data = (uint8_t *)malloc(4096);
    if (!*o_data) {
        printf("oom");
        return -1;
    }

    if ((!data[0] && !data[1] && data[2] == 1) ||
         (!data[0] && !data[1] && !data[2] && data[3] == 1)) {
        /* not hvcC format */
        *o_size = 0;
        *nal_size = 4;
        raw_stream = 1;
        printf("raw header parse done");
        return 0;
    }
    // Store right nal length size that will be used to parse all other nals
    p += 21;
    *nal_size = (*p & 0x03) + 1;
    p++;
    num_arrays = *p;
    p++;

    printf("array cnt:%d nal_size:%d\n", num_arrays, *nal_size);
    // Decode nal units from hvcC
    for (i = 0; i < num_arrays; i++) {
        uint8_t nal_type;
        uint16_t numNalus;
        int j;

        nal_type = *p & 0x3F;
        p++;
        numNalus = WORD_BE(p);
        p += 2;
        for (j = 0; j < numNalus; j++) {
            uint16_t nalsize = WORD_BE(p);
            p += 2;
            //printf("%d: len:%u\n", j, nalsize);
            if (nal_type == HEVC_NAL_VPS || nal_type == HEVC_NAL_SPS ||
                    nal_type == HEVC_NAL_PPS) {
                //NAL header
                if (*nal_size == 3) {
                    memcpy(*o_data + cur_size, s_nal_3, 3);
                    cur_size += 3;
                } else if (*nal_size == 4) {
                    memcpy(*o_data + cur_size, s_nal_4, 4);
                    cur_size += 4;
                } else {
                    printf("invalid nal size:%d\n", *nal_size);
                    goto err_exit;
                }

                memcpy(*o_data + cur_size, p, nalsize);
                cur_size += nalsize;
            }
            p += nalsize;
        }
    }
    printf("header parse done. cur_size:%d\n", cur_size);
    *o_size = cur_size;
    return 0;
err_exit:
    free(*o_data);
    dump("cur.dat", data, size);
    return -2;
}

static void* dmx_thread_func (void *arg) {
    AVPacket pkt;

    /* initialize packet, set data to NULL, let the demuxer fill it */
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    /* read frames from the file */
    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        if (thread_quit)
            break;
        demux_packet(&pkt);
        av_packet_unref(&pkt);
    }

    if (!thread_quit)
        dec_cb->eos();

    printf("dmx done, total vframe:%d\n", video_frame_count);
    return NULL;
}

static void log_callback(void *ptr, int level,
        const char *fmt, va_list vargs)
{
    if (ffmpeg_log)
        vprintf(fmt, vargs);
}

int demux_init(const char *file, struct dmx_cb *cb)
{
    int ret = 0;
    AVCodecParameters *dec_ctx = NULL;

    if (!file || !cb) {
        printf("null pointer\n");
        return 1;
    }

    dec_cb = cb;

    av_log_set_level(AV_LOG_ERROR);
    //av_log_set_level(AV_LOG_DEBUG);
    av_log_set_callback(log_callback);

    fmt_ctx = avformat_alloc_context();
    /* open input file, and allocate format context */
    ret = avformat_open_input(&fmt_ctx, file, NULL, NULL);
    if (ret) {
        printf("Could not open source file %s ret:%d %s\n", file, ret, av_err2str(ret));
        return 2;
    }

    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        printf("Could not find stream information\n");
        ret = 3;
        goto end;
    }

    if (open_codec_context(&video_stream_idx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
        uint8_t *header;
        int h_size;

        video_stream = fmt_ctx->streams[video_stream_idx];
        dec_ctx = video_stream->codecpar;

        /* allocate image where the decoded image will be put */
        v_data.width = dec_ctx->width;
        v_data.height = dec_ctx->height;
        if (dec_ctx->codec_id == AV_CODEC_ID_H264)
            v_data.type = VIDEO_TYPE_H264;
        else if (dec_ctx->codec_id == AV_CODEC_ID_H265)
            v_data.type = VIDEO_TYPE_H265;
        else if (dec_ctx->codec_id == AV_CODEC_ID_MPEG2VIDEO)
            v_data.type = VIDEO_TYPE_MPEG2;
        else if (dec_ctx->codec_id == AV_CODEC_ID_VP9)
            v_data.type = VIDEO_TYPE_VP9;
        else if (dec_ctx->codec_id == AV_CODEC_ID_AV1)
            v_data.type = VIDEO_TYPE_AV1;
        else if (dec_ctx->codec_id == AV_CODEC_ID_MJPEG)
            v_data.type = VIDEO_TYPE_MJPEG;
        else {
            printf("format not supported %d\n", dec_ctx->codec_id);
            ret = 3;
            goto end;
        }


        //printf("AV_CODEC_ID_H264:%d AV_CODEC_ID_H265:%d\n", AV_CODEC_ID_H264, AV_CODEC_ID_H265);
        printf("video stream: format:%d %dx%d tu:%d/%d\n ",
                dec_ctx->codec_id, v_data.width, v_data.height,
                video_stream->time_base.num, video_stream->time_base.den);

        dec_cb->meta_done(&v_data);

        printf("video header:%d\n", dec_ctx->extradata_size);
        h_size = dec_ctx->extradata_size;
        if (dec_ctx->codec_id == AV_CODEC_ID_H264 &&
                dec_ctx->extradata_size) {
            if(h264_header_parse(dec_ctx->extradata,
                        dec_ctx->extradata_size,
                        &header, &h_size, &nal_size)) {
                printf("parse header fail\n");
                ret = 5;
                goto end;
            }
            dec_cb->write(header, h_size);
            free(header);
        } else if (dec_ctx->codec_id == AV_CODEC_ID_H265 &&
                dec_ctx->extradata_size) {
            if(h265_header_parse(dec_ctx->extradata,
                        dec_ctx->extradata_size,
                        &header, &h_size, &nal_size)) {
                printf("parse header fail\n");
                ret = 6;
                goto end;
            }
            if (h_size)
                dec_cb->write(header, h_size);
            free(header);
        } else if (dec_ctx->extradata_size) {
            dec_cb->write(dec_ctx->extradata, dec_ctx->extradata_size);
        }
    }

    /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, file, 0);

    if (!video_stream) {
        printf("Could not find audio or video stream in the input, aborting\n");
        ret = 7;
        goto end;
    }

    if (pthread_create(&dmx_t, NULL, dmx_thread_func, NULL)) {
        printf("Could not create thread, aborting\n");
        ret = 8;
        goto end;
    }

    return 0;

end:
    avformat_close_input(&fmt_ctx);
    return ret;
}

int dmx_destroy() {
    thread_quit = true;
    pthread_join(dmx_t, NULL);
    avformat_close_input(&fmt_ctx);
    if (fmt_ctx)
        avformat_free_context(fmt_ctx);
    return 0;
}
