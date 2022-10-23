#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <dirent.h>
#include <queue>
#include <fcntl.h>
#include <string.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <VX/vx.h>
#include <VX/vxu.h>
#include <VX/vx_api.h>
#include <VX/vx_khr_cnn.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sched.h>
#include <sys/resource.h>
#include "post_process.h"
#include "vnn_yoloface.h"


#define NN_TENSOR_MAX_DIMENSION_NUMBER 4

extern void vx_setup(char * model_name);
extern void vx_destory();
extern vx_status vx_image_process(vx_uint8 *src,int width,int height,char * image_name);
extern void vx_get_input(int *width, int *height);
extern int display_fb(char *bmpname);
extern "C" { void *camera_thread_func(void *arg); }
extern "C" { int yolo_v2_post_process(unsigned char *src, float *predictions, int width, int height, int modelWidth, int modelHeight, int input_num, int *input_size );}
extern unsigned char *displaybuf;

char name[30];
DetectResult gDetectResult;
char *fbp;
int opencv_ok = 0;
const char *xcmd="echo 1080p60hz > /sys/class/display/mode;\
fbset -fb /dev/fb0 -g 1920 1080 1920 2160 24;\
echo 1 > /sys/class/graphics/fb0/freescale_mode;\
echo 0 0 1919 1079 >  /sys/class/graphics/fb0/window_axis;\
echo 0 0 1919 1079 > /sys/class/graphics/fb0/free_scale_axis;\
echo 0x10001 > /sys/class/graphics/fb0/free_scale;\
echo 0 > /sys/class/graphics/fb0/blank;";
vx_tensor                   input = NULL;
vx_tensor                   output = NULL;
vx_tensor_addressing        inputs_tensor_addressing = NULL;
pthread_mutex_t mutex4q;

//static vx_node node;
static int fbfd = 0;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static long int screensize = 0;
static int vx_handle=false;
static int nn_width=0, nn_height=0;
static unsigned int tmpVal;
static int pingflag = 0;
static CvFont font;
static vx_df_image format = VX_DF_IMAGE_RGB;
static vx_bool nn_created = vx_false_e;

vsi_nn_graph_t *graph = NULL;


using namespace std;
using namespace cv;

void vx_destory()
{
    if (graph != NULL) {
        vnn_ReleaseYoloFace( graph, TRUE );
    }
}

void vx_setup(char * model_name)
{
    if (model_name == NULL) {
        printf("model_name is null \n");
        exit(-1);
    }

    if (graph == NULL) {
        graph =vnn_CreateYoloFace(model_name, NULL);
    }
}

void  yolo_face_get_input(int* width, int* height)
{
    if ( graph == NULL ) {
        printf("error: not setup network\n");
        exit(-1);
    }
    vsi_nn_tensor_t *tensor = vsi_nn_GetTensor(graph, graph->input.tensors[0]);
    *width = tensor->attr.size[0];
    *height = tensor->attr.size[1];
}

void read_and_postprocess(vsi_nn_graph_t *graph, int width,int height)
{
    uint32_t i,j,stride;
    vsi_nn_tensor_t *tensor;

    vsi_status status = VSI_FAILURE;
    uint8_t *tensor_data = NULL;
    int output_len = 0;
    int output_cnt = 0;
    float *buffer= NULL;
    int sz[10];

    vx_uint32   output_size[NN_TENSOR_MAX_DIMENSION_NUMBER];

    for(i = 0; i < graph->output.num; i++)
    {
        tensor = vsi_nn_GetTensor(graph, graph->output.tensors[i]);
        sz[i] = 1;
        for(j = 0; j < tensor->attr.dim_num; j++)
        {
            sz[i] *= tensor->attr.size[j];
        }
        output_len += sz[i];

        output_size[0] = tensor->attr.size[0];
        output_size[1] = tensor->attr.size[1];
        output_size[2] = tensor->attr.size[2];
    }

    buffer = (float *)malloc(sizeof(float) * output_len);
    for(i = 0; i < graph->output.num; i++)
    {
        tensor = vsi_nn_GetTensor(graph, graph->output.tensors[i]);

        stride = vsi_nn_TypeGetBytes(tensor->attr.dtype.vx_type);
        tensor_data = (uint8_t *)vsi_nn_ConvertTensorToData(graph, tensor);

        float fl = pow(2.,-tensor->attr.dtype.fl);
        for(j = 0; j < sz[i]; j++)
        {
            int val = tensor_data[stride*j];
            int tmp1=(val>=128)?val-256:val;
            buffer[output_cnt]= tmp1*fl;
            output_cnt++;
        }
    }
    yolo_v2_post_process(NULL,buffer,width,height,13,13,output_len, (int *)output_size);

    if(tensor_data) vsi_nn_Free(tensor_data);
    if (buffer) free(buffer);


}

vx_status vx_image_process(vx_uint8 * src,int width,int height ,char * image_name)
{
    vsi_status status = VSI_FAILURE;
    
    int nn_width ,nn_height;
    int channels=3, offset = 0;
    int i=0, j=0;

    vx_int32            tmpdata;
    vsi_nn_tensor_t *   tensor;
    uint8_t*           input_data_ptr = NULL;

    yolo_face_get_input(&nn_width, &nn_height);
    tensor = vsi_nn_GetTensor( graph, graph->input.tensors[0] );    

    if(input_data_ptr == NULL)
        input_data_ptr = (uint8_t*) malloc(nn_width * nn_height * channels * sizeof(uint8_t));
    if (input_data_ptr == NULL) return status;

    for (i = 0; i < channels; i++)
    {
        offset = nn_width * nn_height *( channels -1 - i);  // prapare BGR input data
        for (j = 0; j < 173056; j++)
        {
            tmpdata = (src[j * channels + i]>>2);
            input_data_ptr[j  + offset] = (uint8_t)((tmpdata >  127) ? 127 : (tmpdata < -128) ? -128 : tmpdata);
        }
    }

    status = vsi_nn_CopyDataToTensor(graph, tensor, input_data_ptr);
    if (status) {
        printf("vsi_nn_CopyDataToTensor fail status =%d\n",status);
        return status;
    }

    status = vsi_nn_RunGraph( graph );

    read_and_postprocess(graph, width,height);

    if(input_data_ptr != NULL)
    {
        free(input_data_ptr);
        input_data_ptr = NULL;
    }

    return status;
}

vx_uint8 * convertFrameToBmpData(cv::Mat &frame)
{
    vx_uint8 *src = frame.ptr<vx_uint8>(0);

    return src;
}


vx_status vx_yolo_v2(cv::Mat &frame, int width ,int height )
{
    vx_uint8 *                    src = NULL;
    vx_status status;
    vx_uint32 count = 0;

    src = convertFrameToBmpData(frame);
    status = vx_image_process(src,width,height,name);
    return 0;
}


//static struct timeval tmsStart, tmsEnd;

static void draw_results(IplImage *pImg,const char *name)
{
    //char filename[255];
	//RGB(250,10,10);
    for (unsigned ii=0; ii<gDetectResult.detect_num; ii++) {
            cvRectangle(pImg,gDetectResult.pt1[ii],gDetectResult.pt2[ii],CV_RGB(10,10,250),3,4,0);        
            //cvPutText(pImg, gDetectResult.name[ii], cvPoint(gDetectResult.pt1[ii].x,gDetectResult.pt1[ii].y-10), &font, CV_RGB(0,255,0));     //zxw
        }
	//memset(fbp,0,1920*1080*3);
	//memcpy(fbp,pImg->imageData,1920*1080*3);
	if(pingflag == 0)
	{
		memcpy(fbp+1920*1080*3,pImg->imageData,1920*1080*3);
		vinfo.activate = FB_ACTIVATE_NOW;
		vinfo.vmode &= ~FB_VMODE_YWRAP;
		vinfo.yoffset = 1080;
		pingflag = 1;
		ioctl(fbfd, FBIOPAN_DISPLAY, &vinfo);
	}
	else
	{
		memcpy(fbp,pImg->imageData,1920*1080*3);
		vinfo.activate = FB_ACTIVATE_NOW;
		vinfo.vmode &= ~FB_VMODE_YWRAP;
		vinfo.yoffset = 0;
		pingflag = 0;
		ioctl(fbfd, FBIOPAN_DISPLAY, &vinfo);
	}

	#if 0
    if(name != NULL)
    {
        memset(filename,0,255);
        strcpy(filename,"result/");
        strcat(filename,name);
        strcat(filename,".png");
//        printf("save image :%s \r\n",filename);
        cvSaveImage("output.bmp",pImg);
    }
	#endif
}
static int init_fb(void)
{
	long int i;
    // Open the file for reading and writing
    fbfd = open("/dev/fb0", O_RDWR);
    if (!fbfd)
    {
        printf("Error: cannot open framebuffer device.\n");
        exit(1);
    }

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo))
    {
        printf("Error reading fixed information.\n");
        exit(2);
    }

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo))
    {
        printf("Error reading variable information.\n");
        exit(3);
    }
    printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel );
/*============add for display BGR begin================,for imx290,reverse color*/
	vinfo.red.offset = 0;
	vinfo.green.offset = 8;
	vinfo.blue.offset = 16;
	//vinfo.activate = FB_ACTIVATE_NOW;   //zxw
	//vinfo.vmode &= ~FB_VMODE_YWRAP;
	if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo) == -1) {
        printf("Error reading variable information\n");
    }
/*============add for display BGR end ================*/	
    // Figure out the size of the screen in bytes
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 4;  //8 to 4

    // Map the device to memory
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                       fbfd, 0);
					   
    if (fbp == NULL)
    {
        printf("Error: failed to map framebuffer device to memory.\n");
        exit(4);
    }
	return 0;
}
static void *thread_func(void *x)
{
    cpu_set_t mask;
    IplImage *frame2process = NULL,*frameclone = NULL;
    int width , height; 
    bool bFrameReady = false;
    cv::Mat yolo_v2Image(nn_height,nn_width,CV_8UC1);
    int i = 0,ret = 0;
    FILE *tfd;
    struct timeval tmsStart, tmsEnd;

    setpriority(PRIO_PROCESS, pthread_self(), -15);

   while (true) {
        pthread_mutex_lock(&mutex4q);
        if(opencv_ok == 1)
        {
            memcpy(frame2process->imageData,displaybuf,1920*1080*3);
        }
        else
        {
            if(frame2process == NULL)
                frame2process = cvCreateImage(cvSize(1920, 1080), IPL_DEPTH_8U, 3);
            if (frame2process == NULL)
            {
                pthread_mutex_unlock(&mutex4q);
                usleep(100000);
                //printf("can't load temp bmp in thread to parse\n");
                continue;
                }
            if(frame2process->width != 1920)
            {
                printf("read image not 1920 width\n");
                pthread_mutex_unlock(&mutex4q);
                continue;
            }
            printf("prepare 1080p image ok\n");
            opencv_ok = 1;   //zxw
        }
        pthread_mutex_unlock(&mutex4q);


        gettimeofday(&tmsStart, 0);
        cv::Mat sourceFrame = frame2process;
        cv::resize(sourceFrame,yolo_v2Image,yolo_v2Image.size());
        gettimeofday(&tmsEnd, 0);
        tmpVal = 1000 * (tmsEnd.tv_sec - tmsStart.tv_sec) + (tmsEnd.tv_usec - tmsStart.tv_usec) / 1000;
        //printf("time over %dms\n",tmpVal);
        gettimeofday(&tmsStart, 0);
        //cvSaveImage("frame.jpg",&qImg);
        vx_yolo_v2(yolo_v2Image,1920,1080);
        draw_results(frame2process,"output");
        gettimeofday(&tmsEnd, 0);
        tmpVal = 1000 * (tmsEnd.tv_sec - tmsStart.tv_sec) + (tmsEnd.tv_usec - tmsStart.tv_usec) / 1000;
        if(tmpVal < 56)
        printf("FPS:%d\n",1000/(tmpVal+8));
        sourceFrame.release();
    }
}

int main( int argc, char** argv )
{
    int i;
    pthread_t tid[2];
    if(argc != 2)
    {
        printf("\n\n************input error******************\n");
        printf("Usage: ./xxxx  model_data_path\n");
        printf("example: ./yolofaceint8  ./yolo_face.export.data\n\n");
        exit(-1);
    }
    char * model_data  = argv[1];
    printf("Load model_data_path:%s\n",model_data);

    printf("Launching CNN FPGA...\n");
    //vx_get_input(&nn_width,&nn_height); 
    nn_width = nn_height = 416;
    vx_setup(model_data);
    system(xcmd);
    init_fb();
    pthread_mutex_init(&mutex4q,NULL);
    //sem_init(&camera_sig,0,0);
    //cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);  //ckao

    if (0 != pthread_create(&tid[0],NULL,thread_func,NULL)) {
        fprintf(stderr, "Couldn't create thread func\n");
        return -1;
    }  
    if (0 != pthread_create(&tid[1],NULL,camera_thread_func,NULL)) {
        fprintf(stderr, "Couldn't create camera thread func\n");
        return -1;
    }	
    while(1)
    {
        for(i=0;i<2;i++)
        {
            pthread_join(tid[i], NULL);
        }
        sleep(1);
    }
    /* release resource */
    vx_destory();
    //cvDestroyWindow(namewin);
    return 0;
}
