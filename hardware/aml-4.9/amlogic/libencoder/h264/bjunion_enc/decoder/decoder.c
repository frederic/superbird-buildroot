//#define LOG_NDEBUG 0
#define LOG_TAG "AMLDEC_API"
#include <utils/Log.h>

#include <fcntl.h>
#include <stdio.h>
#include <codec.h>
#include <stdbool.h>
#include "amvideo.h"
#include <linux/ion.h>
#include <ion/ion.h>
#include <sys/mman.h>
#include "vpcodec_1_0.h"
#include <utils/Log.h>

#define EXTERNAL_PTS    (1)
#define SYNC_OUTSIDE    (2)

#define OUT_BUFFER_COUNT   8
#define OUT_BUFFER_WIDTH   1280
#define OUT_BUFFER_HEIGHT  720
#define OUT_BUFFER_SIZE  (OUT_BUFFER_WIDTH*OUT_BUFFER_HEIGHT*3/2)

typedef struct {
    int index;
    int fd;
    void * pBuffer;
    bool own_by_v4l;
    void *fd_ptr; //only for non-nativebuffer!
    struct ion_handle *ion_hnd; //only for non-nativebuffer!
}out_buffer_t;

static codec_para_t v_codec_para;
static codec_para_t *vpcodec;

static struct amvideo_dev *amvideo_dev = NULL;
static out_buffer_t * mOutBuffer = NULL;
static int mIonFd = 0;
static int out_num = 0;

const char* debugDumpES = "/data/DecoderInput.h264";
FILE* dumpfd = NULL;

bool dumpDecoderInput() {
    bool result = false;
    char value[92]; //PROPERTY_VALUE_MAX=92
    property_get ("media.libvpcodec.decoderinput", value, NULL);
    if (strcmp ("true", value) == 0)
        result = true;
    else
        result = false;
    return result;
}

static int amsysfs_set_sysfs_str_1(const char *path, const char *val) {
    int fd;
    int bytes;
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        bytes = write(fd, val, strlen(val));
        close(fd);
        return 0;
    } else {
        ALOGE("unable to open file %s,err: %s", path, strerror(errno));
    }
    return -1;
}

int AllocDmaBuffers() {
    struct ion_handle *ion_hnd;
    int shared_fd;
    int ret = 0;
    int mDecOutWidth = (OUT_BUFFER_WIDTH + 31) & ( ~31);
    int buffer_size = mDecOutWidth * OUT_BUFFER_HEIGHT * 3 / 2;
    mIonFd = ion_open();
    if (mIonFd < 0) {
        ALOGE("ion open failed!\n");
        return -1;
    }
    int i = 0;
    while (i < OUT_BUFFER_COUNT) {
        unsigned int ion_flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
        ret = ion_alloc(mIonFd, buffer_size, 0, ION_HEAP_CARVEOUT_MASK, ion_flags, &ion_hnd);
        if (ret) {
            ALOGE("ion alloc error");
            ion_close(mIonFd);
            return -1;
        }
        ret = ion_share(mIonFd, ion_hnd, &shared_fd);
        if (ret) {
            ALOGE("ion share error!\n");
            ion_free(mIonFd, ion_hnd);
            ion_close(mIonFd);
            return -1;
        }
        void *cpu_ptr = mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);
        if (MAP_FAILED == cpu_ptr) {
            ALOGE("ion mmap error!\n");
            ion_free(mIonFd, ion_hnd);
            ion_close(mIonFd);
            return -1;
        }
        ALOGD("AllocDmaBuffers__shared_fd=%d,mIonFd=%d\n",shared_fd, mIonFd);
        mOutBuffer[i].index = i;
        mOutBuffer[i].fd = shared_fd;
        mOutBuffer[i].pBuffer = NULL;
        mOutBuffer[i].own_by_v4l = false;
        mOutBuffer[i].fd_ptr = cpu_ptr;
        mOutBuffer[i].ion_hnd = ion_hnd;
        i++;
    }
    return ret;
}

int FreeDmaBuffers() {
    int mDecOutWidth = (OUT_BUFFER_WIDTH + 31) & ( ~31);
    int buffer_size = mDecOutWidth * OUT_BUFFER_HEIGHT * 3 / 2;
    int i = 0;
    while (i < OUT_BUFFER_COUNT) {
        munmap(mOutBuffer[i].fd_ptr, buffer_size);
        close(mOutBuffer[i].fd);
        ALOGD("FreeDmaBuffers_mOutBuffer[i].fd=%d,mIonFd=%d\n", mOutBuffer[i].fd, mIonFd);
        ion_free(mIonFd, mOutBuffer[i].ion_hnd);
        i++;
    }
    int ret = ion_close(mIonFd);
    return ret;
}

void Yuv_Memcpy(char *Src, int Src_width,  int Src_height, char *Dst, int Dst_width,  int Dst_height) {
    if (Dst == NULL || Src == NULL)
        return;
    int w, h;
    char* dst = Dst;
    char* src = Src;
    for (h=0; h<Dst_height; h++) {
        memcpy(dst,src,Dst_width);
        dst += Dst_width;
        src += Src_width;
    }
    src = Src + Src_width*Src_height;
    for (h=0; h<Dst_height/2; h++) {
        memcpy(dst,src,Dst_width);
        dst += Dst_width;
        src += Src_width;
    }
}

/**
 * 初始化解码器
 *
 *@param : codec_id 解码器类型
 *@return : 成功返回解码器handle，失败返回 <= 0
 */
vl_codec_handle_t vl_video_decoder_init(vl_codec_id_t codec_id) {
    /* **********init decoder***************/
    ALOGD("vl_video_decoder_init");
    int ret = CODEC_ERROR_NONE;
    memset(&v_codec_para, 0, sizeof(codec_para_t));
    vpcodec = &v_codec_para;

    v_codec_para.has_video = 1;
    if (codec_id == CODEC_ID_H264) {
        v_codec_para.video_type = VFORMAT_H264;
        v_codec_para.am_sysinfo.format = VIDEO_DEC_FORMAT_H264;
        v_codec_para.am_sysinfo.param = (void *)(EXTERNAL_PTS | SYNC_OUTSIDE);
    }
    v_codec_para.stream_type = STREAM_TYPE_ES_VIDEO;
    //v_codec_para.am_sysinfo.rate = 96000 / atoi(argv[4]);
    //v_codec_para.am_sysinfo.height = atoi(argv[3]);
    //v_codec_para.am_sysinfo.width = atoi(argv[2]);
    v_codec_para.has_audio = 0;
    v_codec_para.noblock = 0;

    amsysfs_set_sysfs_str_1("/sys/class/vfm/map", "rm default");
    amsysfs_set_sysfs_str_1("/sys/class/vfm/map", "add default decoder ionvideo");

    ret = codec_init(vpcodec);
    if (ret != CODEC_ERROR_NONE) {
        ALOGE("codec init failed, ret=-0x%x", -ret);
        return -1;
    }

    /************ init ionvideo*****************/
    mOutBuffer = (out_buffer_t *)malloc(sizeof(out_buffer_t) * OUT_BUFFER_COUNT);
    memset(mOutBuffer, 0, sizeof(out_buffer_t) * OUT_BUFFER_COUNT);
    AllocDmaBuffers();
    amvideo_dev = new_amvideo(FLAGS_V4L_MODE);
    amvideo_dev->display_mode = 0;
    ret = amvideo_init(amvideo_dev, 0, OUT_BUFFER_WIDTH, OUT_BUFFER_HEIGHT, V4L2_PIX_FMT_NV21, OUT_BUFFER_COUNT);

    if (ret < 0) {
        ALOGE("amvideo_init failed =%d\n", ret);
        amvideo_release(amvideo_dev);
        amvideo_dev = NULL;
    }

    int i = 0;
    vframebuf_t vf;
    memset(&vf, 0, sizeof(vframebuf_t));
    while (i < OUT_BUFFER_COUNT) {
        vf.index = mOutBuffer[i].index;
        vf.fd = mOutBuffer[i].fd;
        vf.length = OUT_BUFFER_SIZE;
        ALOGD("main amlv4l_queuebuf i=%d, vf.index=%d, vf.fd=%d\n", i, vf.index, vf.fd);
        int ret = amlv4l_queuebuf(amvideo_dev, &vf);
        if (ret < 0) {
            ALOGE("amlv4l_queuebuf failed =%d\n", ret);
        }

        if (i == 1) {
            ret = amvideo_start(amvideo_dev);
            ALOGD("amvideo_start ret=%d\n", ret);
            if (ret < 0) {
                ALOGE("amvideo_start failed =%d\n", ret);
                amvideo_release(amvideo_dev);
                amvideo_dev = NULL;
            }
        }
        i++;
    }
    if (dumpDecoderInput()) {
        ALOGD("dumpDecoderInput = true");
        if (dumpfd != NULL)
            fclose(dumpfd);
        dumpfd = fopen(debugDumpES,"wb");
        if (!dumpfd) {
            ALOGE("-- dump file create fail. %d----\n",errno);
        }
    }
    return 1;
}

/**
 * 视频解码
 *
 *@param : handle 视频解码器handle
 *@param : in 待解码的数据
 *@param : in_size 待解码的数据长度
 *@param : out 解码后的数据，内部分配空间
 *@return ：成功返回解码后的数据长度，失败返回 <= 0
 */
int vl_video_decoder_decode(vl_codec_handle_t handle, char * in, int in_size, char ** out) {
    int isize = 0;
    int ret = 0;
    ALOGD("vl_video_decoder_decode in_size = %d\n", in_size);
    if (dumpfd != NULL)
        fwrite(in, 1, in_size, dumpfd);
    do {
        ret = codec_write(vpcodec, in+isize, in_size-isize);
        if (ret < 0) {
            if (errno != EAGAIN) {
                ALOGE("write data failed, errno %d\n", errno);
                return -1;
            } else {
                usleep(10000);
                continue;
            }
        } else {
            isize += ret;
        }
    } while (isize < in_size);
    struct buf_status vbuf;
    ret = codec_get_vbuf_state(vpcodec, &vbuf);
    if (ret != 0) {
        ALOGE("codec_get_vbuf_state error: %x\n", -ret);
    } else {
        ALOGD("vbuf.data_len=%d\n",vbuf.data_len);
    }

    vframebuf_t vf;
    int len = 0;
    void *cpu_ptr = NULL;
    ret = amlv4l_dequeuebuf(amvideo_dev, &vf);
    if (ret >= 0) {
        int i = 0;
        while (i < OUT_BUFFER_COUNT) {
            if (vf.fd == mOutBuffer[i].fd) {
                cpu_ptr = mOutBuffer[i].fd_ptr;
                break;
            }
            i++;
        }
        int width = vf.width;
        int height = vf.height;
        len = width*height*3/2;
        Yuv_Memcpy(cpu_ptr, OUT_BUFFER_WIDTH, OUT_BUFFER_HEIGHT, *out, width, height);
        ALOGD("__get one frame,out_num=%d, width=%d, height=%d, mOutBuffer[i].index=%d\n",out_num++, width, height, mOutBuffer[i].index);
        vf.index = mOutBuffer[i].index;
        vf.fd = mOutBuffer[i].fd;
        vf.length = OUT_BUFFER_SIZE;
        ret = amlv4l_queuebuf(amvideo_dev, &vf);
        if (ret < 0) {
            ALOGD("amlv4l_queuebuf failed =%d\n", ret);
        }
        return len;
    }
    ALOGD("vl_video_decoder_decode have no output\n");
    return 0;
}

/**
 * 销毁解码器
 *@param : handle 视频解码器handle
 *@return ：成功返回1，失败返回0
 */
int vl_video_decoder_destory(vl_codec_handle_t handle) {
    codec_close(vpcodec);
    if (amvideo_dev) {
        amvideo_release(amvideo_dev);
        amvideo_dev = NULL;
    }
    amsysfs_set_sysfs_str_1("/sys/class/vfm/map", "rm default");
    amsysfs_set_sysfs_str_1("/sys/class/vfm/map", "add default decoder ppmgr deinterlace amvideo");

    ALOGD("vl_video_decoder_destory\n");
    FreeDmaBuffers();
    if (mOutBuffer != NULL) {
        free(mOutBuffer);
        mOutBuffer = NULL;
    }
    if (dumpfd != NULL)
        fclose(dumpfd);
    dumpfd = NULL;
    return 1;
}



