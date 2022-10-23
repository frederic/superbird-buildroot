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
//#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <time.h>
#include <linux/videodev2.h>
#include <poll.h>
#include <semaphore.h>

#include "common.h"
#include "isp_metadata.h"
#include "renderer.h"
#include "capture.h"
#include "logs.h"
#include <unistd.h>
#include <stdio.h>


#include <sched.h>

#define STATIC_STREAM_COUNT (ARM_V4L2_TEST_STREAM_MAX - ARM_V4L2_TEST_HAS_RAW)
#define NB_BUFFER           6
#define DUMP_RAW            0
static capture_module_t g_cap_mod = { 0, };
static int sensor_bits = 10;

static char *str_on_off[2] = { "ON", "OFF" };

#if DUMP_RAW
static int dump_fd = -1;
#endif
unsigned char *displaybuf;
//char showbuff[1920*1080*3];
int frame_cnt = 0;
//FILE *outfile = NULL;

pthread_mutex_t mutex4q;

/*=================bmp save begin===============*/
//14byte文件头  
/*typedef struct  
{  
    char cfType[2];//文件类型，"BM"(0x4D42)  
    long cfSize;//文件大小（字节）  
    long cfReserved;//保留，值为0  
    long cfoffBits;//数据区相对于文件头的偏移量（字节）  
}__attribute__((packed)) BITMAPFILEHEADER;  */

typedef struct tagBITMAPFILEHEADER {
        short    bfType;//must be 0x4D42.
        unsigned int  bfSize;//the size of the whole bitmap file.
        short    bfReserved1;
        short    bfReserved2;
        unsigned int   bfOffBits;//the sum bits of BITMAPFILEHEADER,BITMAPINFOHEADER and RGBQUAD;the index byte of the image data.
} __attribute__((packed))BITMAPFILEHEADER;
//40byte信息头  
/*typedef struct  
{  
    char ciSize[4];//BITMAPFILEHEADER所占的字节数  
    long ciWidth;//宽度  
    long ciHeight;//高度  
    char ciPlanes[2];//目标设备的位平面数，值为1  
    int ciBitCount;//每个像素的位数  
    char ciCompress[4];//压缩说明  
    char ciSizeImage[4];//用字节表示的图像大小，该数据必须是4的倍数  
    char ciXPelsPerMeter[4];//目标设备的水平像素数/米  
    char ciYPelsPerMeter[4];//目标设备的垂直像素数/米  
    char ciClrUsed[4]; //位图使用调色板的颜色数  
    char ciClrImportant[4]; //指定重要的颜色数，当该域的值等于颜色数时（或者等于0时），表示所有颜色都一样重要  
}__attribute__((packed)) BITMAPINFOHEADER; */ 

typedef struct{
        unsigned int  biSize;//the size of this struct.it is 40 bytes.
        unsigned int       biWidth;//the width of image data. the unit is pixel.
        unsigned int       biHeight;//the height of image data. the unit is pixel.
        short       biPlanes;//must be 1.
        short       biBitCount;//the bit count of each pixel.usually be 1,4,8,or 24.
        unsigned int      biCompression;//is this image compressed.0 indicates no compression.
        unsigned int      biSizeImage;//the size of image data.
        unsigned int       biXPelsPerMeter;
        unsigned int       biYPelsPerMeter;
        unsigned int      biClrUsed;
        unsigned int      biClrImportant;
} __attribute__((packed))BITMAPINFOHEADER;

typedef struct tagRGBQUAD{
  char    rgbBlue;
  char    rgbGreen;
  char    rgbRed;
  char    rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO { // bmi 
    BITMAPINFOHEADER bmiHeader; //指定了一个BITMAPINFOHEADER结构，包含了有关设备相关位图的度量和颜色格式的信息
    RGBQUAD          bmiColors[1]; //指定了一个RGBQUAD或DWORD数据类型的数组，定义了位图中的颜色。
} BITMAPINFO; 

void save_bmp(unsigned char * data,int data_size,int w,int h,FILE * out)  
{  
    // 位图文件头  
    BITMAPFILEHEADER bmpheader;   
    BITMAPINFO bmpinfo;   
    int bit = 24;  
  
    bmpheader.bfType = ('M' <<8)|'B';   
    bmpheader.bfReserved1 = 0;   
    bmpheader.bfReserved2 = 0;   
    bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);  //54byte 
    bmpheader.bfSize = bmpheader.bfOffBits + w*h*bit/8;  
  
    bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);   
    bmpinfo.bmiHeader.biWidth = w;   
    bmpinfo.bmiHeader.biHeight = 0-h;   
    bmpinfo.bmiHeader.biPlanes = 1;   
    bmpinfo.bmiHeader.biBitCount = 24;   
    bmpinfo.bmiHeader.biCompression = 0;      //not compress BI_RGB   
    bmpinfo.bmiHeader.biSizeImage = data_size;   
    //bmpinfo.bmiHeader.biXPelsPerMeter = 100;   
    //bmpinfo.bmiHeader.biYPelsPerMeter = 100;   
    //bmpinfo.bmiHeader.biClrUsed = 0;   
    //bmpinfo.bmiHeader.biClrImportant = 0;  
	//printf("bisize:%d,head size:%d\n",sizeof(BITMAPINFOHEADER),bmpheader.bfOffBits);
    fwrite(&bmpheader,1,sizeof(BITMAPFILEHEADER),out);   
    fwrite(&bmpinfo.bmiHeader,1,sizeof(BITMAPINFOHEADER),out);  
    fwrite(data,1,data_size,out); 
	//fflush(out);
	fclose(out);
}  


/*=================bmp save end=================*/

/**********
 * thread parameters
 */
/* control parameters */
static uint32_t         v4l2_test_thread_exit = 0;
static uint32_t         v4l2_test_thread_preview = 0;
static capture_type_t   v4l2_test_thread_capture = V4L2_TEST_CAPTURE_NONE;
static uint32_t         v4l2_test_thread_capture_count = 0;
static uint32_t         v4l2_test_thread_dump = 0;

/* config parameters */
struct thread_param {
    /* video device info */
    char                        * devname;

    /* display device info */
    char                        * fbp;
    struct fb_var_screeninfo    vinfo;
    struct fb_fix_screeninfo    finfo;

    /* format info */
    uint32_t                    width;
    uint32_t                    height;
    uint32_t                    pixformat;

    /* for snapshot stream (non-zsl implementation) */
    int32_t                     capture_count;
};

pthread_t tid[STATIC_STREAM_COUNT];


/**********
 * helper functions
 */
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

static void do_sensor_preset(int videofd, int preset)
{
    struct v4l2_control ctrl;

    ctrl.id = ISP_V4L2_CID_SENSOR_PRESET;
    ctrl.value = preset;

    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("Do sensor preset failed");
    }
}

static void do_other_commands(int videofd)
{
    if( cmd_do_af_refocus ) {
        cmd_do_af_refocus = 0;
        do_af_refocus(videofd);
    }
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
    void                        *v4l2_mem[NB_BUFFER*VIDEO_MAX_PLANES];
    int total_mapped_mem=0;
	cpu_set_t mask;
    /* thread parameters */
    struct thread_param         *tparm = (struct thread_param *)arg;
    pthread_t                   cur_pthread = pthread_self();
    int                         stream_type = -1;

    /* condition & loop flags */
    int                         rc = 0;
    int                         i,j,num=0;
	char name_buff[32];
    int multiplanar=0;
    //__u32	v4l2_enum_type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    __u32	v4l2_enum_type=V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	char filename[64]; 
	
	/*CPU_ZERO(&mask);
	CPU_SET(2, &mask);

	if (pthread_setaffinity_np(pthread_self(), sizeof(mask),&mask) < 0) 
	{
		printf("pthread_setaffinity_np\n");
	}*/
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

    /* check capability */
    memset (&v4l2_cap, 0, sizeof (struct v4l2_capability));
    rc = ioctl (videofd, VIDIOC_QUERYCAP, &v4l2_cap);
    if (rc < 0) {
        printf ("Error: get capability.\n");
        goto fatal;
    }
    INFO("[T#%d] VIDIOC_QUERYCAP: capabilities=0x%x, device_caps:0x%x\n",
            stream_type,
			v4l2_cap.capabilities,
			v4l2_cap.device_caps);

    //do_sensor_preset(videofd,1);
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
    	ERR("Error: set format %d.\n",rc);
        printf ("Error: set format.\n");
        goto fatal;
    }
    rc = ioctl (videofd, VIDIOC_G_FMT, &v4l2_fmt);
    if (rc < 0) {
    	ERR("Error: get format %d.\n",rc);
        printf ("Error: get format.\n");
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
    			stream_type,
				v4l2_fmt.fmt.pix_mp.num_planes);
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
        	//printf(" rc:%d",rc);
            printf("Error: query buffer.\n");
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
			for(j=0;j<v4l2_fmt.fmt.pix_mp.num_planes;j++){
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
        	//printf(" rc:%d i:%d",rc,i);
            printf("Error: queue buffers.\n");
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

    /* poll variables */
    const int POLL_TIMEOUT = 2000;
    struct pollfd pfds[1];
    int pollret;

    /* dequeue and display */
    do {
        struct v4l2_buffer v4l2_buf;
        //struct v4l2_plane buf_planes[v4l2_fmt.fmt.pix_mp.num_planes];
        frame_t newframe;
        int length = 0;
        int idx = -1;
#if DUMP_RAW
		if(frame_cnt % 5 == 0)
		memset(filename, 0, sizeof(filename));
		sprintf(filename, "%s_%d.rgb24", "/mnt/sda/dump", frame_cnt);
		dump_fd = open(filename, O_CREAT|O_RDWR |O_TRUNC, 0666);
		if (dump_fd < 0)		
			printf("Dump File error");
#endif
		//memset(displaybuf, 0, 1920*1080*3);

        /* wait (poll) for a frame event */
        //printf ("[T#%d] Start polling (exit flag = %d, capture count = %d)\n",
        //        stream_type, v4l2_test_thread_exit, tparm->capture_count);
        pfds[0].fd = videofd;
        pfds[0].events = POLLIN;
        pfds[0].revents = 0;
        pollret = poll(pfds, 1, POLL_TIMEOUT);
        if(pollret == 0) {
            INFO ("[T#%d] %d ms poll timeout.\n", stream_type, POLL_TIMEOUT);
            if (v4l2_test_thread_exit)
                break;
            else
               continue;
        } else if (pollret < 0) {
            printf ("Error: poll error\n");
           // break;
        } else {
            //DBG ("[T#%d] Frame ready (pollret = %d, results = %d)\n", stream_type, pollret, pfds[0].revents);
        }


        // dqbuf from video node
        memset (&v4l2_buf, 0, sizeof (struct v4l2_buffer));
        //v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.type = v4l2_enum_type;
        v4l2_buf.memory = V4L2_MEMORY_MMAP;
        if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
			//v4l2_buf.m.planes=buf_planes;
			if(v4l2_buf.m.planes == NULL)
			{
				v4l2_buf.m.planes=malloc(v4l2_fmt.fmt.pix_mp.num_planes*sizeof(struct v4l2_plane));
			}
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
        //if(	stream_type	!= ARM_V4L2_TEST_STREAM_RAW){
			newframe.width[0] = v4l2_fmt.fmt.pix.width;
			newframe.height[0] = v4l2_fmt.fmt.pix.height;
			newframe.bit_depth[0] = sensor_bits;
			newframe.bytes_per_line[0] = (((newframe.width[0] * ((sensor_bits + 7) >> 3)) + 127) >> 7 ) << 7;  // for padding
			newframe.paddr[0] = v4l2_mem[idx];
			DBG("[T#%d] Buffer[%d] single to capture.\n", stream_type, newframe.paddr[0]);
			newframe.num_planes=1;
        }else if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
        	newframe.num_planes=v4l2_fmt.fmt.pix_mp.num_planes;
        	for(i=0;i<newframe.num_planes;i++){
        		newframe.width[i] = v4l2_fmt.fmt.pix_mp.width;
				newframe.height[i] = v4l2_fmt.fmt.pix_mp.height;
#if ARM_V4L2_TEST_HAS_RAW
				if(stream_type==ARM_V4L2_TEST_STREAM_RAW)
					newframe.bit_depth[i] = sensor_bits;
				else
#endif
					newframe.bit_depth[i] = fr_bitdepth;
				/*if(stream_type==ARM_V4L2_TEST_STREAM_RAW){
					printf("i:%d newframe.bit_depth[i]:%d sensor_bits:%d",i,newframe.bit_depth[i],sensor_bits);
				}*/
				//newframe.bytes_per_line[i] = (((newframe.width[i] * ((sensor_bits + 7) >> 3)) + 127) >> 7 ) << 7;  // for padding
				newframe.bytes_per_line[i] = v4l2_fmt.fmt.pix_mp.plane_fmt[i].bytesperline;
				newframe.paddr[i] = v4l2_mem[idx*newframe.num_planes+i];
				//DBG("[T#%d] i:%d newframe.paddr:%p to capture idx:%d.\n", stream_type,i, newframe.paddr[i],idx);
				//newframe.paddr[i] = v4l2_mem[idx];
        	}
        }


	uint64_t startt, endt;
	image_info_t src;

	/*src.ptr = v4l2_mem[idx];
	src.width = v4l2_fmt.fmt.pix.width;
	src.height = v4l2_fmt.fmt.pix.height;
	src.bpp = 32;   // Todo: fixed to ARGB for now
	src.fmt = v4l2_fmt.fmt.pix.pixelformat;*/

	//memcpy(displaybuf, v4l2_mem[idx],1920*1080*3);

	rc = ioctl (videofd, VIDIOC_QBUF, &v4l2_buf);
	if (rc < 0) {
		printf ("Error: queue buffer.\n");
		break;
	}
	pthread_mutex_lock(&mutex4q);
	memcpy(displaybuf,v4l2_mem[idx],1920*1080*3);
	/*
	for(num=0;num<1920*1080*3-2;num++)
	{
		displaybuf[num]=*((unsigned char*)(v4l2_mem[idx])+num+2);
		displaybuf[num+1]=*((unsigned char*)(v4l2_mem[idx])+num+1);
		displaybuf[num+2]=*((unsigned char*)(v4l2_mem[idx])+num);
		num=num+2;	
	}*/
		
	/*if(opencv_ok == 0)
	{
		//sprintf(name_buff,"temp%d.bmp",bmp_name++);
		//if(bmp_name == 10) bmp_name = 0;
		printf("before save temp.bmp\n");
		outfile = fopen("/media/yolo_face/temp.bmp","wb");
		if(outfile == NULL)
		{
			printf("open temp bmp fail\n");
			pthread_mutex_unlock(&mutex4q);
			break;
		}
		//save_bmp_flag = 0;
		save_bmp(displaybuf,1920*1080*3,1920,1080,outfile); 
		//usleep(10000);
		//sem_post(&camera_sig);
	}*/
	pthread_mutex_unlock(&mutex4q);
	//printf("after save temp.bmp\n");
	//usleep(10000);
	//renderImage(tparm->fbp, tparm->vinfo, tparm->finfo, displaybuf, AFD_RENDER_MODE_LEFT_TOP);

	} while (tparm->capture_count != 0);

    /**************************************************
     * resource clean-up
     *************************************************/
    /* release all buffers from capture_module */
	free(displaybuf);
    release_capture_module_stream(&g_cap_mod, stream_type);
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
    tparam->capture_count = 50000;

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
//int main(int argc, char *argv[])

void *camera_thread_func(void *arg)
{
    /* variables for device name / format with default values  */
	int sensor_preset=0;
    uint8_t fr_out_fmt = 0;
    int resolution = 0;
    uint32_t width=0, height=0;
    uint32_t pixel_format = ISP_V4L2_PIX_FMT_ARGB2101010;
    char *fbdevname = "/dev/fb0";
    char *v4ldevname = "/dev/video0";
    int rc = 0;
    int i;
    int command = 1;  //test
	displaybuf = (unsigned char *)malloc(1920*1080*3);

    int c;
	/*
    while(optind < argc){
		if((c = getopt (argc, argv, "c:p:f:r:b:v:")) != -1){
			switch(c){
			case 'c':
				command = atoi(optarg);
				break;
			case 'p':
				sensor_preset = atoi(optarg);
				break;
			case 'f':
				fr_out_fmt = atoi(optarg);
				break;
			case 'r':
				resolution = atoi(optarg);
				break;
			case 'b':
				fbdevname = optarg;
				break;
			case 'v':
				v4ldevname = optarg;
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

*/

    MSG("Params: command=%d sensor_preset:%d FR_OUT fmt = %d, resolution:%d, fb device=%s, v4l2 device=%s \n",
    		command,sensor_preset,fr_out_fmt,resolution, fbdevname, v4ldevname);

    switch(fr_out_fmt) {
    case 0:
        pixel_format = V4L2_PIX_FMT_RGB24;
        break;
    case 1:
        pixel_format = V4L2_PIX_FMT_RGB24;
        break;
    case 2:
		pixel_format = V4L2_PIX_FMT_RGB24;
		break;
    default:
        ERR("Invalid FR_OUT fmt %d !\n", fr_out_fmt);
        return (void*)1;
    }

    if(resolution>=0){
		switch(resolution) {
		case 0:
			width = 1920;
			height = 1080;
			break;
		case 1:
			width = 4000;
			height = 3000;
			break;
		case 2:
			width = 1280;
			height = 720;
			break;
		default:
			ERR("Invalid resolution %d !\n", fr_out_fmt);
			return (void*)1;
		}
    }
#if 0
    /**************************************************
     * Frame buffer initialize
     *************************************************/
    int fbfd = 0;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;
    char *fbp = 0;

    /* Open the file for reading and writing */
    fbfd = open(fbdevname, O_RDWR);
    if (fbfd == -1) {
        printf("Error: cannot open framebuffer device\n");
        exit(1);
    }
    MSG("The %s device was opened successfully.\n", fbdevname);

    /* Get fixed screen information */
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        printf("Error reading fixed information\n");
        exit(2);
    }

    /* Get variable screen information */
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        printf("Error reading variable information\n");
        exit(3);
    }

	vinfo.red.offset = 0;
	vinfo.green.offset = 8;
	vinfo.blue.offset = 16;

	if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo) == -1) {
        printf("Error reading variable information\n");
    }
    MSG("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    /* Figure out the size of the screen in bytes */
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    /* Map the device to memory */
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fbfd, 0);
    if (fbp < 0) {
        printf("Error: failed to map framebuffer device to memory\n");
        exit(4);
    }
    MSG("The framebuffer device was mapped to memory successfully.\n");
#endif
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
            //.fbp        = fbp,
            //.finfo      = finfo,
            //.vinfo      = vinfo,

            .width      = width,
            .height     = height,
            .pixformat  = pixel_format,

            .capture_count = -1
        },
#if ARM_V4L2_TEST_HAS_META
        {
            .devname    = v4ldevname,
            .fbp        = fbp,
            .finfo      = finfo,
            .vinfo      = vinfo,

            .width      = 1*1024*1024,
            .height     = 1,
            .pixformat  = ISP_V4L2_PIX_FMT_META,

            .capture_count = -1
        }
#endif
    };

#if ARM_V4L2_TEST_HAS_RAW
    struct thread_param tparam_raw = {
        .devname    = v4ldevname,
        .fbp        = fbp,
        .finfo      = finfo,
        .vinfo      = vinfo,

        .width      = width,
        .height     = height,
        .pixformat  = V4L2_PIX_FMT_SBGGR16,

        .capture_count = 1
    };
#endif



    /* init mutex lock for frame_pack_t */
    init_capture_module(&g_cap_mod);

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
    for (i = 0; i < STATIC_STREAM_COUNT; i++) {
        rc = pthread_create(&tid[i], NULL, &video_thread, &tparam[i]);
        if (rc != 0)
            ERR("can't create thread :[%s]\n", strerror(rc));
        else
            MSG("Thread %d created successfully\n", i);

        usleep(100000);
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
        V4L2_TEST_MENU_EXIT,
        V4L2_TEST_MENU_MAX
    };

    if(command>=V4L2_TEST_MENU_MAX)
        command=-1;

    do {
        int menu;
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

    for (i = 0; i < STATIC_STREAM_COUNT; i++) {
        pthread_join(tid[i], NULL);
    }

    MSG("terminating v4l2 test app, thank you ...\n");

fatal:
    //munmap(fbp, screensize);
    //close(fbfd);
    return (void*)0;
}
