//----------------------------------------------------------------------------
//   The confidential and proprietary information contained in this file may
//   only be used by a person authorised under and to the extent permitted
//   by a subsisting licensing agreement from ARM Limited or its affiliates.
//
//          (C) COPYRIGHT [2018] ARM Limited or its affiliates.
//              ALL RIGHTS RESERVED
//
//   This entire notice must be reproduced on all copies of this file
//   and copies of this file may only be made by a person if such person is
//   permitted to do so under the terms of a subsisting license agreement
//   from ARM Limited or its affiliates.
//----------------------------------------------------------------------------


/*
 * Apical(ARM) V4L2 test application 2018
 *
 * This is ARM internal development purpose SW tool running on JUNO.
 */

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <time.h>
#include <linux/videodev2.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include "common.h"
#include "logs.h"

#define STR_VALUE(val) #val
#define STR(name) STR_VALUE(name)

#define PATH_LEN 256
#define MD5_LEN 32

#define STATIC_STREAM_COUNT (ARM_V4L2_TEST_STREAM_MAX - ARM_V4L2_TEST_HAS_RAW)
#define NB_BUFFER           6
#define DUMP_RAW            0
#define FRAME_PACK_QUEUE_SIZE   2
#define FILE_NAME_LENGTH        64

typedef enum {
    V4L2_TEST_CAPTURE_NONE = 0,
    V4L2_TEST_CAPTURE_LEGACY,
	V4L2_TEST_CAPTURE_FRM,
    V4L2_TEST_CAPTURE_DNG,
    V4L2_TEST_CAPTURE_MAX,
} capture_type_t;

typedef struct _frame_t {
    uint32_t            vfd;
    struct v4l2_buffer  vbuf;
    uint32_t 			pixelformat;
    int                 width[VIDEO_MAX_PLANES];
    int                 height[VIDEO_MAX_PLANES];
    int                 bit_depth[VIDEO_MAX_PLANES];
    int                 bytes_per_line[VIDEO_MAX_PLANES];
    void                * paddr[VIDEO_MAX_PLANES];
    int 				num_planes;
} frame_t;

typedef struct _frame_pack_t {
    uint32_t            frame_id;
    uint32_t            frame_flag;
    frame_t             frame_data[ARM_V4L2_TEST_STREAM_MAX];
} frame_pack_t;

typedef struct _capture_module_t {
    /* frame queue */
    frame_pack_t        frame_pack_queue[FRAME_PACK_QUEUE_SIZE];
    pthread_mutex_t     vmutex;

    /* capture hitsory */
    char                last_capture_filename[ARM_V4L2_TEST_STREAM_MAX][FILE_NAME_LENGTH];
    int                 capture_performed;
    uint32_t            last_buf_size;
    uint32_t            last_header_size;
#if ARM_V4L2_TEST_HAS_META
    uint32_t            last_metadata_buf_size;
#endif
} capture_module_t;

static int sensor_bits = 10;

static char *str_on_off[2] = { "ON", "OFF" };
static unsigned char uv_buffer[1920*1080*3];
static unsigned char buffer[1920*1080*3];

uint64_t start_time,end_time;

typedef enum render_mode {
    AFD_RENDER_MODE_CENTER,
    AFD_RENDER_MODE_LEFT_TOP,
    AFD_RENDER_MODE_MAX
} render_mode_t;

/* renderer image parameter */
typedef struct image_info {
    unsigned char *ptr;
    int width;
    int height;
    int bpp;
    uint32_t fmt;
} image_info_t;

#if DUMP_RAW
static int dump_fd = -1;
#endif
unsigned char *displaybuf_fr;
unsigned char *displaybuf_ds1;
unsigned char *displaybuf_ds2;
unsigned char *displaybuf_meta;
int from_md5 = 0;
int to_md5 = 0;
int slt_control = 10;
/**********
 * thread parameters
 */
/* control parameters */
static uint32_t         v4l2_test_thread_exit = 0;
static uint32_t         v4l2_test_thread_preview = 0;
static capture_type_t   v4l2_test_thread_capture = V4L2_TEST_CAPTURE_NONE;
static uint32_t         v4l2_test_thread_capture_count = 0;
static uint32_t         v4l2_test_thread_dump = 0;

static int fps_test_port = -1;
static int open_port_cnt = 1;

/* config parameters */
struct thread_param {
    /* video device info */
    char                        * devname;

    /* format info */
    uint32_t                    width;
    uint32_t                    height;
    uint32_t                    pixformat;
    uint32_t                    wdr_mode;
    uint32_t                    exposure;

    /* for snapshot stream (non-zsl implementation) */
    int32_t                     capture_count;
    int                         videofd;
};

pthread_t tid[STATIC_STREAM_COUNT];


/**********
 * helper functions
 */
 int CalcFileMD5(char *file_name, char *md5_sum)
{
    #define MD5SUM_CMD_FMT "md5sum %." STR(PATH_LEN) "s 2>/dev/null"
    char cmd[PATH_LEN + sizeof (MD5SUM_CMD_FMT)];
    sprintf(cmd, MD5SUM_CMD_FMT, file_name);
    #undef MD5SUM_CMD_FMT

    FILE *p = popen(cmd, "r");
    if (p == NULL) return 0;

    int i, ch;
    for (i = 0; i < MD5_LEN && isxdigit(ch = fgetc(p)); i++) {
        *md5_sum++ = ch;
    }

    *md5_sum = '\0';
    pclose(p);
    return i == MD5_LEN;
}

uint64_t getTimestamp() {
    struct timespec ts;
    int rc;

    rc = clock_gettime(0x00, &ts);
    if (rc == 0) {
        return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    } else {
        return 0;
    }
}

int64_t GetTimeMsec() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int slttest(int f, int t, int delta_s){
    FILE * read_file =NULL;
    FILE * write_file =NULL;
    int width = 1920;
    int height = 1080;
    int ret = -1;
    char name[60] = {'\0'};
    int i, j, k, err_flag;
	int delta_set,delta;
	int temp = 0;
    char md5[MD5_LEN + 1];
    char tempmd5[MD5_LEN + 1];
	int index;
	int max = 0,choosen = 0;
	int from, to;
	from = f;
	to = t;
	delta_set = delta_s;
	err_flag = 0;
	printf("from: frame_%d to frame_%d\n", from, to - 1);
	for (j = from; j < to; j++) {
		sprintf(name, "/media/ca_%d.rgb", j);
		if (!CalcFileMD5(name, tempmd5)) {
			printf("Error occured1: read MD5 fail!\n");
			return -1;
		} else {
			memcpy(md5, tempmd5, sizeof(tempmd5));
		}
		index = 0;
		for (k = j; k < to; k++) {
			sprintf(name, "/media/ca_%d.rgb", k);
			if (CalcFileMD5(name, tempmd5)) {
				if (strcmp(tempmd5, md5) == 0) {
					index ++;
				} else {
					continue;
				}
			} else {
				printf("Error occured2: read MD5 fail!\n");
				return -1;
			}
		}
		if (index > max) {
			max = index;
			choosen = j;
		}
		if (max > ( to - from + 1)/2)
			break;
	}
	sprintf(name, "/media/ca_%d.rgb", choosen);
	printf("choose frame_%d as base, same frames = %d\n", choosen, max);
	if (!CalcFileMD5(name, md5)) {
		printf("Error occured3: read MD5 fail!\n");
		return -1;
	}
	read_file = fopen(name, "r");
	for (i = 0; i < width * height * 3; i++) {
		ret = fread(buffer + i, 1, 1, read_file);
		if (ret < 0)
			printf("read fail\n");
	}
	for (j = from; j < to; j++) {
		sprintf(name, "/media/ca_%d.rgb", j);
		if (!CalcFileMD5(name, tempmd5)) {
			printf("Error occured4: read MD5 fail!\n");
			return -1;
		}
		if (strcmp(tempmd5, md5) == 0) {
			continue;
		}
		write_file = fopen(name, "r");
		if (read_file == NULL || write_file == NULL) {
			printf("open file error!\n");
			return -1;
		}

		for (i = 0; i < width * height * 3; i++) {
			ret = fread(uv_buffer + i, 1, 1, write_file);
			if (ret < 0) {
				printf("write fail\n");
				return -1;
			}
		}
		delta = 0;
		for (i = 0; i < width * height * 3; i++) {
			temp = abs(uv_buffer[i] - buffer[i]);
			if (temp > delta) {
				delta = temp;
			}
			if (temp > delta_set) {
				//printf("location: x: %d, y: %d, temp: %d\n", i / 3 % width , i / (width * 3), temp);
			}
		}
		if (delta > delta_set) {
			printf("WARNING: frame_%d: pixel diff max:%d larger than allow %d\n", j, delta, delta_set);
			err_flag = 1;
		}
		if (write_file)
			fclose(write_file);
	}
	if (err_flag)
		printf("slt test fail!\n");
	else
		printf("slt test success!\n");
	if (read_file)
		fclose(read_file);
	system("rm -rf /media/ca_*");
	return 0;
}
static uint8_t cmd_do_af_refocus = 0;

static void do_af_refocus(int videofd)
{
    struct v4l2_control ctrl;

    ctrl.id = ISP_V4L2_CID_AF_REFOCUS;
    ctrl.value = 0;

    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("Do AF refocus failed");
    }
}

static void do_sensor_test_pattern(int videofd, int val)
{
    struct v4l2_control ctrl;
    ctrl.id = ISP_V4L2_CID_CUSTOM_SET_SENSOR_TESTPATTERN;
    ctrl.value = val;
    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("Do set sensor test pattern");
    }
}

static void do_sensor_preset(int videofd, int preset)
{
    struct v4l2_control ctrl;

    ctrl.id = ISP_V4L2_CID_SENSOR_PRESET;
    ctrl.value = preset;

    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("Do sensor preset failed");
    }
}

static void set_manual_exposure(int videofd, int enable)
{
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_EXPOSURE_AUTO;
    ctrl.value = enable;
    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("set_manual_exposure failed\n");
    }
}

void save_imgae(char *buff, unsigned int size, int flag, int count)
{
    char name[60] = {'\0'};
    int fd = -1;

    if (buff == NULL || size == 0) {
        printf("%s:Error input param\n", __func__);
        return;
    }
	if(count < from_md5 )
		return;
    sprintf(name, "/media/ca_%d.rgb", count);

    fd = open(name, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        printf("%s:Error open file\n", __func__);
        return;
    }
    write(fd, buff, size);
    close(fd);
}


/**********
 * thread function
 */
void * video_thread(void *arg)
{
    /* v4l2 variables */
    int                         videofd;
    struct v4l2_capability      v4l2_cap;
    struct v4l2_format          v4l2_fmt;
    struct v4l2_requestbuffers  v4l2_rb;
    int                         v4l2_buf_length = 0;
    //void                        *v4l2_mem[NB_BUFFER];
    //number of buffer* max num of planes
    void                        *v4l2_mem[NB_BUFFER*VIDEO_MAX_PLANES];
    int total_mapped_mem=0;

    /* thread parameters */
    struct thread_param         *tparm = (struct thread_param *)arg;
    pthread_t                   cur_pthread = pthread_self();
    int                         stream_type = -1;

    /* condition & loop flags */
    int                         rc = 0;
    int                         i,j;
    __u32	v4l2_enum_type=V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    unsigned char *displaybuf = NULL;
    uint64_t display_count = 0;
    int64_t start, end;
    /**************************************************
     * find thread id
     *************************************************/
    for (i = 0; i < ARM_V4L2_TEST_STREAM_MAX; i++) {
        if(cur_pthread == tid[i]) {
            stream_type = i;
            break;
        }
    }

    /**************************************************
     * device open
     *************************************************/
    /* Open the file for reading and writing */
    videofd = open(tparm->devname, O_RDWR);
    if (videofd == -1) {
        printf("Error: cannot open video device\n");
        exit(1);
    }
    INFO("[T#%d] The %s device was opened successfully.\n", stream_type, tparm->devname);

    tparm->videofd = videofd;
    do_sensor_test_pattern(videofd, 5);
    /* check capability */
    memset (&v4l2_cap, 0, sizeof (struct v4l2_capability));
    rc = ioctl (videofd, VIDIOC_QUERYCAP, &v4l2_cap);
    if (rc < 0) {
        printf ("Error: get capability.\n");
        goto fatal;
    }

    INFO("[T#%d] VIDIOC_QUERYCAP: capabilities=0x%x, device_caps:0x%x\n",
        stream_type, v4l2_cap.capabilities, v4l2_cap.device_caps);


    /**************************************************
     * format configuration
     *************************************************/
    /* set format */
    memset (&v4l2_fmt, 0, sizeof (struct v4l2_format));
    //v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_fmt.type = v4l2_enum_type;
    v4l2_fmt.fmt.pix_mp.width = tparm->width;
    v4l2_fmt.fmt.pix_mp.height = tparm->height;
    v4l2_fmt.fmt.pix_mp.pixelformat = tparm->pixformat;
    //v4l2_fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
    //v4l2_fmt.fmt.pix_mp.plane_fmt[0].sizeimage = v4l2_fmt.fmt.pix_mp.width * v4l2_fmt.fmt.pix_mp.height;
    v4l2_fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
    //v4l2_fmt.fmt.pix_mp.num_planes = 2;

    INFO("[T#%d] VIDIO_S_FMT: type=%d, w=%d, h=%d, fmt=0x%x, field=%d\n",
        stream_type,
        v4l2_fmt.type,
        v4l2_fmt.fmt.pix.width,
        v4l2_fmt.fmt.pix.height,
        v4l2_fmt.fmt.pix.pixelformat,
        v4l2_fmt.fmt.pix.field);

    rc = ioctl (videofd, VIDIOC_S_FMT, &v4l2_fmt);
    if (rc < 0) {
        printf("Error: set format %d.\n", rc);
        goto fatal;
    }
    rc = ioctl (videofd, VIDIOC_G_FMT, &v4l2_fmt);
    if (rc < 0) {
        printf("Error: get format %d.\n", rc);
        goto fatal;
    }
    INFO("[T#%d] VIDIO_G_FMT: type=%d, w=%d, h=%d, fmt=0x%x, field=%d\n",
        stream_type,
        v4l2_fmt.type,
        v4l2_fmt.fmt.pix.width,
        v4l2_fmt.fmt.pix.height,
        v4l2_fmt.fmt.pix.pixelformat,
        v4l2_fmt.fmt.pix.field);

    //real type and planes here
    v4l2_enum_type=v4l2_fmt.type;
    if(v4l2_fmt.type==V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
        INFO("[T#%d] multiplanar support planes=%d\n",
            stream_type, v4l2_fmt.fmt.pix_mp.num_planes);
    }

    int fr_bitdepth;
    //get the pixel format
    switch(v4l2_fmt.fmt.pix.pixelformat){
    case V4L2_PIX_FMT_RGB32:
        fr_bitdepth = 24;
        break;
    case ISP_V4L2_PIX_FMT_ARGB2101010:
        fr_bitdepth = 30;
        break;
    default:
        fr_bitdepth = (v4l2_fmt.fmt.pix_mp.plane_fmt[i].bytesperline/v4l2_fmt.fmt.pix_mp.width)*8;
        break;
    }

    /**************************************************
     * buffer preparation
     *************************************************/
    /* request buffers */
    memset (&v4l2_rb, 0, sizeof (struct v4l2_requestbuffers));
    v4l2_rb.count = NB_BUFFER;
    //v4l2_rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_rb.type = v4l2_enum_type;
    v4l2_rb.memory = V4L2_MEMORY_MMAP;
    rc = ioctl (videofd, VIDIOC_REQBUFS, &v4l2_rb);
    if (rc < 0) {
        printf("Error: request buffer.\n");
        goto fatal;
    }

    /* map buffers */
    for (i = 0; i < NB_BUFFER; i++) {
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane buf_planes[v4l2_fmt.fmt.pix_mp.num_planes];
        memset (&v4l2_buf, 0, sizeof (struct v4l2_buffer));
        v4l2_buf.index = i;
        //v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.type = v4l2_enum_type;
        v4l2_buf.memory = V4L2_MEMORY_MMAP;
        if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
            v4l2_buf.m.planes=buf_planes;
            v4l2_buf.length = v4l2_fmt.fmt.pix_mp.num_planes;
        }
        rc = ioctl (videofd, VIDIOC_QUERYBUF, &v4l2_buf);
        if (rc < 0) {
            printf("Error: query buffer %d.\n", rc);
            goto fatal;
        }

        if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE){
            v4l2_buf_length = v4l2_buf.length;
            INFO("[T#%d] length: %u offset: %u\n", stream_type, v4l2_buf.length, v4l2_buf.m.offset);
            v4l2_mem[i] = mmap (0, v4l2_buf.length, PROT_READ, MAP_SHARED,
                videofd, v4l2_buf.m.offset);
            ++total_mapped_mem;
            INFO("[T#%d] Buffer[%d] mapped at address %p total_mapped_mem:%d.\n", stream_type, i, v4l2_mem[i],total_mapped_mem);
        }
        else if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
            for (j=0;j<v4l2_fmt.fmt.pix_mp.num_planes;j++) {
                v4l2_buf_length = v4l2_buf.m.planes[j].length;
                INFO("[T#%d] plane:%d multiplanar length: %u offset: %u\n", stream_type, j, v4l2_buf.m.planes[j].length, v4l2_buf.m.planes[j].m.mem_offset);
                v4l2_mem[i*v4l2_fmt.fmt.pix_mp.num_planes + j] = mmap (0, v4l2_buf.m.planes[j].length, PROT_READ, MAP_SHARED,
                    videofd, v4l2_buf.m.planes[j].m.mem_offset);
                ++total_mapped_mem;
                INFO("[T#%d] Buffer[%d] mapped at address %p total_mapped_mem:%d.\n", stream_type,i*v4l2_fmt.fmt.pix_mp.num_planes + j, v4l2_mem[i*v4l2_fmt.fmt.pix_mp.num_planes + j],total_mapped_mem);
            }
        }
        if (v4l2_mem[i] == MAP_FAILED) {
            printf("Error: mmap buffers.\n");
            goto fatal;
        }
    }

    /* queue buffers */
    for (i = 0; i < NB_BUFFER; ++i) {
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane buf_planes[v4l2_fmt.fmt.pix_mp.num_planes];
        memset (&v4l2_buf, 0, sizeof (struct v4l2_buffer));
        v4l2_buf.index = i;
        //v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.type = v4l2_enum_type;
        v4l2_buf.memory = V4L2_MEMORY_MMAP;
        if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
            v4l2_buf.m.planes=buf_planes;
            v4l2_buf.length = v4l2_fmt.fmt.pix_mp.num_planes;
        }
        rc = ioctl (videofd, VIDIOC_QBUF, &v4l2_buf);
        if (rc < 0) {
            printf("Error: queue buffers, rc:%d i:%d\n", rc, i);
            goto fatal;;
        }
    }
    DBG("[T#%d] Queue buf done.\n", stream_type);


    /**************************************************
     * V4L2 stream on, get buffers
     *************************************************/
    /* stream on */
    //int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int type = v4l2_enum_type;
    rc = ioctl (videofd, VIDIOC_STREAMON, &type);
    if (rc < 0) {
        printf("Error: streamon.\n");
        goto fatal;
    }
    INFO("[T#%d] Video stream is on.\n", stream_type);

    if (stream_type == fps_test_port) {
        start = GetTimeMsec();
    }

    /* poll variables */
    const int POLL_TIMEOUT = 2000;
    struct pollfd pfds[1];
    int pollret;

    /* dequeue and display */
    do {
        struct v4l2_buffer v4l2_buf;
        //struct v4l2_plane buf_planes[v4l2_fmt.fmt.pix_mp.num_planes];
        frame_t newframe;
        int idx = -1;

        switch (stream_type) {
        case ARM_V4L2_TEST_STREAM_FR:
            memset(displaybuf_fr, 0, tparm->width * tparm->height * 3);
            displaybuf = displaybuf_fr;
            break;
        case ARM_V4L2_TEST_STREAM_META:
            memset(displaybuf_meta, 0, tparm->width * tparm->height * 3);
            displaybuf = displaybuf_meta;
            break;
        case ARM_V4L2_TEST_STREAM_DS1:
            memset(displaybuf_ds1, 0, tparm->width * tparm->height * 3);
            displaybuf = displaybuf_ds1;
            break;
        case ARM_V4L2_TEST_STREAM_DS2:
            memset(displaybuf_ds2, 0, tparm->width * tparm->height * 3);
            displaybuf = displaybuf_ds2;
            break;
        }

        /* wait (poll) for a frame event */
        //printf ("[T#%d] Start polling (exit flag = %d, capture count = %d)\n",
        //    stream_type, v4l2_test_thread_exit, tparm->capture_count);
        pfds[0].fd = videofd;
        pfds[0].events = POLLIN;
        pfds[0].revents = 0;
        pollret = poll(pfds, 1, POLL_TIMEOUT);
        if (pollret == 0) {
            INFO ("[T#%d] %d ms poll timeout.\n", stream_type, POLL_TIMEOUT);
            if (v4l2_test_thread_exit)
                break;
            else
               continue;
        } else if (pollret < 0) {
            printf ("Error: poll error\n");
            // break;
        } else {
            // DBG ("[T#%d] Frame ready (pollret = %d, results = %d)\n", stream_type, pollret, pfds[0].revents);
        }

        // dqbuf from video node
        memset (&v4l2_buf, 0, sizeof (struct v4l2_buffer));
        //v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.type = v4l2_enum_type;
        v4l2_buf.memory = V4L2_MEMORY_MMAP;
        if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
            //v4l2_buf.m.planes=buf_planes;
            v4l2_buf.m.planes=malloc(v4l2_fmt.fmt.pix_mp.num_planes*sizeof(struct v4l2_plane));
            v4l2_buf.length = v4l2_fmt.fmt.pix_mp.num_planes;
        }
        rc = ioctl (videofd, VIDIOC_DQBUF, &v4l2_buf);
        if (rc < 0) {
            printf ("Error: dequeue buffer.\n");
            if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
                free(v4l2_buf.m.planes);
            break;

        }
        idx = v4l2_buf.index;

        // fill frame_pack and do enqueue_buffer()
        newframe.vfd = videofd;
        newframe.vbuf = v4l2_buf;
        newframe.pixelformat=v4l2_fmt.fmt.pix_mp.pixelformat;
        if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE){
        //if(stream_type != ARM_V4L2_TEST_STREAM_RAW){
            newframe.width[0] = v4l2_fmt.fmt.pix.width;
            newframe.height[0] = v4l2_fmt.fmt.pix.height;
            newframe.bit_depth[0] = sensor_bits;
            newframe.bytes_per_line[0] = (((newframe.width[0] * ((sensor_bits + 7) >> 3)) + 127) >> 7 ) << 7;  // for padding
            newframe.paddr[0] = v4l2_mem[idx];
            DBG("[T#%d] Buffer[%d] single to capture.\n", stream_type, newframe.paddr[0]);
            newframe.num_planes=1;
        }else if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
            newframe.num_planes=v4l2_fmt.fmt.pix_mp.num_planes;
            for (i=0;i<newframe.num_planes;i++) {
                newframe.width[i] = v4l2_fmt.fmt.pix_mp.width;
                newframe.height[i] = v4l2_fmt.fmt.pix_mp.height;
#if ARM_V4L2_TEST_HAS_RAW
                if (stream_type == ARM_V4L2_TEST_STREAM_RAW)
                    newframe.bit_depth[i] = sensor_bits;
                else
#endif
                    newframe.bit_depth[i] = fr_bitdepth;
                /*
                if (stream_type == ARM_V4L2_TEST_STREAM_RAW) {
                    printf("i:%d newframe.bit_depth[i]:%d sensor_bits:%d",i,newframe.bit_depth[i],sensor_bits);
                }
                */
                //newframe.bytes_per_line[i] = (((newframe.width[i] * ((sensor_bits + 7) >> 3)) + 127) >> 7 ) << 7;  // for padding
                newframe.bytes_per_line[i] = v4l2_fmt.fmt.pix_mp.plane_fmt[i].bytesperline;
                newframe.paddr[i] = v4l2_mem[idx*newframe.num_planes+i];
                //DBG("[T#%d] i:%d newframe.paddr:%p to capture idx:%d.\n", stream_type,i, newframe.paddr[i],idx);
                //newframe.paddr[i] = v4l2_mem[idx];
            }
        }

        image_info_t src;
        src.ptr = v4l2_mem[idx];
        src.width = v4l2_fmt.fmt.pix.width;
        src.height = v4l2_fmt.fmt.pix.height;
        src.bpp = 32; // Todo: fixed to ARGB for now
        src.fmt = v4l2_fmt.fmt.pix.pixelformat;

        if (src.fmt == V4L2_PIX_FMT_NV12) {
			memcpy(displaybuf, v4l2_mem[idx * 2], v4l2_fmt.fmt.pix_mp.plane_fmt[0].sizeimage);
			memcpy(displaybuf + v4l2_fmt.fmt.pix_mp.plane_fmt[0].sizeimage, v4l2_mem[idx * 2 + 1],
									v4l2_fmt.fmt.pix_mp.plane_fmt[1].sizeimage);
        } else {
                memcpy(displaybuf, src.ptr, v4l2_fmt.fmt.pix_mp.plane_fmt[0].sizeimage);
        }

        rc = ioctl (videofd, VIDIOC_QBUF, &v4l2_buf);
        if (rc < 0) {
            printf ("Error: queue buffer.\n");
            break;
        }

        /***** select save file or display through different stream_type *****/
        if (stream_type == ARM_V4L2_TEST_STREAM_FR) {
			save_imgae(displaybuf, v4l2_fmt.fmt.pix_mp.plane_fmt[0].sizeimage, stream_type, display_count);
        } else if (stream_type == ARM_V4L2_TEST_STREAM_META) {
        //do nothing
        } else if (stream_type == ARM_V4L2_TEST_STREAM_DS1) {
        //save_imgae(displaybuf, v4l2_fmt.fmt.pix_mp.plane_fmt[0].sizeimage, stream_type);
        } else if (stream_type == ARM_V4L2_TEST_STREAM_DS2) {
        //save_imgae(displaybuf, v4l2_fmt.fmt.pix_mp.plane_fmt[0].sizeimage, stream_type);
        }

        display_count++;
        if ((stream_type == fps_test_port) && (display_count % 100 == 0)) {
            end = GetTimeMsec();
            end = end - start;
            printf("stream port %s fps is : %d\n",
                (stream_type == 0) ? "FR" :
                ((stream_type == 1) ? "Meta":
                ((stream_type == 2) ? "DS1":
                ((stream_type == 3) ? "DS2": "Other"))), (100 * 1000) /end);
            start = GetTimeMsec();
        }

        if (tparm->capture_count > 0)
            tparm->capture_count--;

    } while (tparm->capture_count != 0);
    do_sensor_test_pattern(videofd, 1);

    /**************************************************
     * resource clean-up
     *************************************************/
    /* release all buffers from capture_module */
    free(displaybuf);
    /* stream off */
    rc = ioctl (videofd, VIDIOC_STREAMOFF, &type);
    if (rc < 0) {
        printf("Error: streamoff.\n");
        goto fatal;
    }

    /* unmap buffers */
    //for (i = 0; i < NB_BUFFER; i++) {
    for (i = 0; i < total_mapped_mem; i++) {
        munmap (v4l2_mem[i], v4l2_buf_length);
    }

    if (stream_type == ARM_V4L2_TEST_STREAM_FR) {
        rc = slttest(from_md5,to_md5, slt_control);
        if (rc < 0)
            printf("Error: slt test exit due to error,pls check if isp is ok!\n");
    }
    end_time = GetTimeMsec();
    end_time = end_time - start_time;
    printf("cost time : %ld\n", end_time);
fatal:

    close(videofd);

    MSG("thread %d terminated ...\n", stream_type);

    return NULL;
}


#if ARM_V4L2_TEST_HAS_RAW
/**********
 * raw capture functions
 */
int prepareRawCapture(struct thread_param * tparam) {
    // Reset capture count
    tparam->capture_count = 10000;

    return pthread_create(&tid[ARM_V4L2_TEST_STREAM_RAW], NULL, &video_thread, tparam);
}

void finishRawCapture(struct thread_param * tparam) {
    tparam->capture_count = 0; //force thread exit
    pthread_join(tid[ARM_V4L2_TEST_STREAM_RAW], NULL);
}
#endif

void usage(char * prog){
    MSG("\nUsage:\t%s { -c command} {-p sensor preset} {-f FR_OUT fmt} {-r resolution} ( {-b fb device} {-v v4l2 device} )\n"
        "\tcommand		: menu command 1-7 (Optional,  default=-1 not enabled)\n\n"
        "\tsensor preset: sensor 0-n-1 sensor modes   (Optional,  default=current preset)\n\n"
        "\tFR_OUT fmt   : output format of FR  (Optional, default 0:ARGB2101010, 1:ARGB8888, 2: YUV NV12)\n"
        "\tresolution   : output resolution        (Optional, default :current preset  0:1080p , 1:12MP, 2:720p)\n"
        "\tfb device    : device name of fb node   (Optional,  default=/dev/fb0)\n"
        "\tv4l2 device  : device name of v4l2 node (Optional,  default=/dev/video0)\n\n"
        , prog);
}

/**********
 * main function
 */

void parse_fmt_res(uint8_t fmt, int res, uint32_t fr_wdr_mode, uint32_t fr_exposure, void *param)
{
    struct thread_param *t_param = NULL;
    uint32_t pixel_format = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t wdr_mode = 0;
    uint32_t exposure = 0;

    if (param == NULL) {
        ERR("Error input param\n");
        return;
    }

    t_param = param;

    switch (fmt) {
    case 0:
        pixel_format = V4L2_PIX_FMT_RGB24;
        break;
    case 1:
        pixel_format = V4L2_PIX_FMT_NV12;
        break;
    case 2:
        pixel_format = V4L2_PIX_FMT_SBGGR16;
        break;
    default:
        ERR("Invalid FR_OUT fmt %d !\n", fmt);
        break;
    }

    switch (res) {
    case 0:
        width = 3840;
        height = 2160;
        break;
    case 1:
        width = 1920;
        height = 1080;
        break;
    case 2:
        width = 1280;
        height = 720;
        break;
    case 3:
        width = 640;
        height = 480;
        break;
    default:
        ERR("Invalid resolution %d !\n", res);
        break;
    }

    switch (fr_wdr_mode) {
    case 0:
        wdr_mode = 0;
        break;
    case 1:
        wdr_mode = 1;
        break;
    case 2:
        wdr_mode = 2;
        break;
    default:
        ERR("Invalid FR wdr mode %d !\n", fr_wdr_mode);
        break;
    }

    switch (fr_exposure) {
    case 1:
        exposure = 1;
        break;
    case 2:
        exposure = 2;
        break;
    case 3:
        exposure = 3;
        break;
    case 4:
        exposure = 4;
        break;
    default:
        ERR("Invalid FR exposure %d !\n", fr_exposure);
        break;
    }

    t_param->pixformat = pixel_format;
    t_param->width = width;
    t_param->height = height;
    t_param->wdr_mode = wdr_mode;
    t_param->exposure = exposure;

    ERR("pixel fmt 0x%x, width %d, height %d, wdr_mode %d, exposure %d",
        pixel_format, width, height, wdr_mode, exposure);
}

int main(int argc, char *argv[])
{
    /* variables for device name / format with default values  */
    int sensor_preset = 0;
    uint8_t fr_out_fmt = 0;
    uint8_t ds1_out_fmt = 0;
    uint8_t ds2_out_fmt = 0;
    int fr_res = 1;
    int ds1_res = 2;
    int ds2_res = 2;
    int fr_num = 31;
    int ds_num = 21;
    uint32_t pixel_format = ISP_V4L2_PIX_FMT_ARGB2101010;
    uint32_t wdr_mode = 0;
    uint32_t exposure = 1;
    char *v4ldevname = "/dev/video0";
    int rc = 0;
    int i;
    int command = 1;
	start_time = GetTimeMsec();
    if (argc < 0) {
        printf("v4l test API\n");
        printf("usage:\n");
        printf(" example   : ./v4l2_test  -c 1 -p 0 -F 0 -f 0 -D 0 -R 1 -r 2 -d 2 -N 31 -n 21 -w 0 -e 1 -v /dev/video0 \n");
        printf("    c : command           : default 1, 7: set_manual_exposure(just enable manual exposure)\n");
        printf("    p : sensor_preset     : default 0 \n");
        printf("    F : fr_out_fmt        : 0 : rgb24 1:nv12 2: raw16 \n");
        printf("    f : ds1_out_fmt       : 0 : rgb24 1:nv12 \n");
        printf("    D : ds2_out_fmt       : 0 : rgb24 1:nv12 \n");
        printf("    R : fr_out_resolution : 0 : 4k    1: 1080p  2: 720p  3. 480p\n");
        printf("    r : ds1_out_resolution: 0 : 4k    1: 1080p  2: 720p  3. 480p\n");
        printf("    d : ds2_out_resolution: 0 : 4k    1: 1080p  2: 720p  3. 480p\n");
        printf("    w : wdr mode          : 0 : linear 1: native 2: fs lin\n");
        printf("    e : exposure value    : min 1, max 4, default is 1\n");
        printf("    v : videodev          : default: /dev/video0\n");
        printf("    N : slt test to N frame \n");
        printf("    n : slt test from n frame \n");
        printf("    t : run the port count, default is 1\n");
        printf("    x : fps print port. default: -1, no print. 0:  fr, 1: meta, 2: ds1, 3: ds2\n");
		printf("    y : pixel diff between frames\n");
        return -1;
    }

    int c;
	system("echo 0xff15c0e8 > /sys/kernel/debug/aml_reg/paddr;");
    while(optind < argc){
        if ((c = getopt (argc, argv, "c:p:F:f:D:R:r:d:N:n:w:e:v:t:x:y:")) != -1) {
            switch (c) {
            case 'c':
                command = atoi(optarg);
                break;
            case 'p':
                sensor_preset = atoi(optarg);
                break;
            case 'F':
                fr_out_fmt = atoi(optarg);
                break;
            case 'f':
                ds1_out_fmt = atoi(optarg);
                break;
            case 'D':
                ds2_out_fmt = atoi(optarg);
                break;
            case 'R':
                fr_res = atoi(optarg);
                break;
            case 'r':
                ds1_res = atoi(optarg);
                break;
            case 'd':
                ds2_res = atoi(optarg);
                break;
            case 'N':
                fr_num = atoi(optarg);
                break;
            case 'n':
                ds_num = atoi(optarg);
                break;
            case 'w':
                wdr_mode = atoi(optarg);
                break;
            case 'e':
                exposure = atoi(optarg);
                break;
            case 'v':
                v4ldevname = optarg;
                break;
            case 't':
                open_port_cnt = atoi(optarg);
                break;
            case 'x':
                fps_test_port = atoi(optarg);
                break;
            case 'y':
                slt_control = atoi(optarg);
                break;
            case '?':
                usage(argv[0]);
                exit(1);
            }
        }else{
            MSG("Invalid argument %s\n",argv[optind]);
            usage(argv[0]);
            exit(1);
        }
    }

    printf("ds1_out_fmt = %d, ds2_out_fmt = %d, ds1_res = %d, ds2_res = %d\n", ds1_out_fmt, ds2_out_fmt, ds1_res, ds2_res);
    /**************************************************
     * Frame buffer initialize
     *************************************************/
    to_md5 = fr_num;
	from_md5 = ds_num;
    /* Open the file for reading and writing */

    /* Get fixed screen information */

    printf("fr_num = %d, ds_num = %d\n", fr_num, ds_num);

    /**************************************************
     * Starting streams
     *************************************************/
    /* init thread parameters */
    v4l2_test_thread_exit = 0;
    v4l2_test_thread_preview = 0;
    v4l2_test_thread_capture = V4L2_TEST_CAPTURE_NONE;
    v4l2_test_thread_capture_count = 0;
    v4l2_test_thread_dump = 0;

    /* init thread parameters */
    struct thread_param tparam[STATIC_STREAM_COUNT] = {
        {
            .devname    = v4ldevname,

            .width      = 1920,
            .height     = 1080,
            .pixformat  = pixel_format,
            .wdr_mode	= 0,
            .exposure	= 1,

            .capture_count = fr_num,
        },
#if ARM_V4L2_TEST_HAS_META
        {
            .devname    = v4ldevname,

            .width      = 1*1024*1024,
            .height     = 1,
            .pixformat  = ISP_V4L2_PIX_FMT_META,
            .wdr_mode	= 0,
            .exposure	= 1,

            .capture_count = -1,
        },
#endif
        {
            .devname    = v4ldevname,

            .width      = 1280,
            .height     = 720,
            .pixformat  = V4L2_PIX_FMT_RGB24,
            .wdr_mode	= 0,
            .exposure	= 1,

            .capture_count = ds_num,
        },
        {
            .devname    = v4ldevname,

            .width      = 1280,
            .height     = 720,
            .pixformat  = V4L2_PIX_FMT_RGB24,
            .wdr_mode	= 0,
            .exposure	= 1,

            .capture_count = ds_num,
        },
    };

    parse_fmt_res(fr_out_fmt, fr_res, wdr_mode, exposure, &tparam[ARM_V4L2_TEST_STREAM_FR]);
    parse_fmt_res(ds1_out_fmt, ds1_res, wdr_mode, exposure, &tparam[ARM_V4L2_TEST_STREAM_DS1]);
    parse_fmt_res(ds2_out_fmt, ds2_res, wdr_mode, exposure, &tparam[ARM_V4L2_TEST_STREAM_DS2]);

    displaybuf_fr = (unsigned char *)malloc(tparam[0].width * tparam[0].height * 3);
    displaybuf_meta = (unsigned char *)malloc(tparam[1].width * tparam[1].height * 3);
    displaybuf_ds1 = (unsigned char *)malloc(tparam[2].width * tparam[2].height * 3);
    displaybuf_ds2 = (unsigned char *)malloc(tparam[3].width * tparam[2].height * 3);

#if ARM_V4L2_TEST_HAS_RAW
    struct thread_param tparam_raw = {
        .devname    = v4ldevname,

        .width      = 1920,
        .height     = 1080,
        .pixformat  = V4L2_PIX_FMT_SBGGR16,
        .wdr_mode	= 0,
        .exposure	= 1,
        .capture_count = 1
    };
#endif


    if(sensor_preset>=0){
        int videofd = open(v4ldevname, O_RDWR);
        if (videofd == -1) {
            printf("Error: cannot open video device\n");
            exit(1);
        }
        MSG("Setting %d sensor preset\n", sensor_preset);
        do_sensor_preset(videofd,sensor_preset);
        close(videofd);
        sleep(3); //let the sensor settle
    }

    /* Launch threads */
    for (i = 0; i < open_port_cnt; i++) {
        rc = pthread_create(&tid[i], NULL, &video_thread, &tparam[i]);
        if (rc != 0)
            ERR("can't create thread :[%s]\n", strerror(rc));
        else
            MSG("Thread %d created successfully\n", i);

        usleep(300000);
    }


    /**************************************************
     * Control interface
     *************************************************/
    enum {
        V4L2_TEST_MENU_PREVIEW_ON_OFF = 1,
        V4L2_TEST_MENU_DO_CAPTURE_LEGACY,
        V4L2_TEST_MENU_DO_CAPTURE_FRM,
        V4L2_TEST_MENU_DO_CAPTURE_DNG,
        V4L2_TEST_MENU_DO_AF_REFOCUS,
        V4L2_TEST_MENU_DUMP_LAST_CAPTURE,
        V4L2_TEST_MENU_SET_MANUAL_EXP,
        V4L2_TEST_MENU_EXIT,
        V4L2_TEST_MENU_MAX
    };

    if(command>=V4L2_TEST_MENU_MAX)
        command=-1;

    do {
        int menu;
        int enable;
        if(command>=V4L2_TEST_MENU_PREVIEW_ON_OFF){
            menu=command;
        }else{
            MSG("\nV4L2 test application\n");
            MSG("%d) turn preview %s\n",
                V4L2_TEST_MENU_PREVIEW_ON_OFF, str_on_off[v4l2_test_thread_preview]);
            MSG("%d) Do capture (Legacy)\n", V4L2_TEST_MENU_DO_CAPTURE_LEGACY);
            MSG("%d) Do capture (FRM)\n", V4L2_TEST_MENU_DO_CAPTURE_FRM);
            MSG("%d) Do capture (DNG)\n", V4L2_TEST_MENU_DO_CAPTURE_DNG);
            MSG("%d) Do AF Refocus\n", V4L2_TEST_MENU_DO_AF_REFOCUS);
            MSG("%d) Dump last capture\n", V4L2_TEST_MENU_DUMP_LAST_CAPTURE);
            MSG("%d) SET_MANUAL_EXP\n", V4L2_TEST_MENU_SET_MANUAL_EXP);
            MSG("%d) Exit\n", V4L2_TEST_MENU_EXIT);
            MSG("Choose menu > ");
            //fflush(stdout);
            scanf("%d", &menu);
        }
        switch(menu) {
        case V4L2_TEST_MENU_PREVIEW_ON_OFF:
            v4l2_test_thread_preview = (v4l2_test_thread_preview + 1) % 2;
            usleep(250000);
            break;
        case V4L2_TEST_MENU_DO_CAPTURE_LEGACY:
        case V4L2_TEST_MENU_DO_CAPTURE_FRM:
#if ARM_V4L2_TEST_HAS_RAW
            /* turn on raw capture stream */
            if (prepareRawCapture(&tparam_raw) != 0) {
                ERR("Error: Can't start raw stream, cancelling capture.\n");
                break;
            }
#endif
            if(menu==V4L2_TEST_MENU_DO_CAPTURE_LEGACY)
                v4l2_test_thread_capture = V4L2_TEST_CAPTURE_LEGACY;
            else
                v4l2_test_thread_capture = V4L2_TEST_CAPTURE_FRM;
            v4l2_test_thread_capture_count = 1;

            do {
                printf("sleeping while v4l2_test_thread_capture:%d\n",v4l2_test_thread_capture);
                usleep(200000);
            } while(v4l2_test_thread_capture != V4L2_TEST_CAPTURE_NONE);

#if ARM_V4L2_TEST_HAS_RAW
            /* wait raw capture stream to be turned off */
            finishRawCapture(&tparam_raw);
#endif

            break;
        case V4L2_TEST_MENU_DO_CAPTURE_DNG:
#if ARM_V4L2_TEST_HAS_RAW
            /* turn on raw capture stream */
            if (prepareRawCapture(&tparam_raw) != 0) {
                ERR("Error: Can't start raw stream, cancelling capture.\n");
                break;
            }
#endif

            v4l2_test_thread_capture = V4L2_TEST_CAPTURE_DNG;
            v4l2_test_thread_capture_count = 1;

            do {
                usleep(200000);
            } while(v4l2_test_thread_capture != V4L2_TEST_CAPTURE_NONE);

#if ARM_V4L2_TEST_HAS_RAW
            /* wait raw capture stream to be turned off */
            finishRawCapture(&tparam_raw);
#endif
            break;
        case V4L2_TEST_MENU_DO_AF_REFOCUS:
            cmd_do_af_refocus = 1;
            break;
        case V4L2_TEST_MENU_DUMP_LAST_CAPTURE:
            MSG("Turning off preview ...\n");
            v4l2_test_thread_preview = 0;
            v4l2_test_thread_dump = STREAM_FLAG_PREVIEW;
            do {
                usleep(200000);
            } while(v4l2_test_thread_dump);
            break;
        case V4L2_TEST_MENU_SET_MANUAL_EXP:
            enable = 1;
            set_manual_exposure(tparam[0].videofd, enable);
            break;
        case V4L2_TEST_MENU_EXIT:
            v4l2_test_thread_exit = 1;
            break;
        default:
            ERR("invalid input.\n");
            break;
        }

        if(command>=V4L2_TEST_MENU_PREVIEW_ON_OFF)
            v4l2_test_thread_exit = 1;

    } while (!v4l2_test_thread_exit);


    /**************************************************
     * Terminating threads and process
     *************************************************/
    MSG("terminating all threads ...\n");

    for (i = 0; i < open_port_cnt; i++) {
        pthread_join(tid[i], NULL);
    }

    MSG("terminating v4l2 test app, thank you ...\n");
    return 0;
}
