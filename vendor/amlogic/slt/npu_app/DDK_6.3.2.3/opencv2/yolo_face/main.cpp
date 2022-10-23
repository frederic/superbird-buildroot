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
#include "vxc_util.h"
#include "vxc_nn_dynamic_fixed_point_8.h"
#include "vx_utility.h"

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
static vx_context context;
static vx_graph graph;
static vx_image imageInput;
static vx_bool nn_created = vx_false_e;
static tensors_info_t  outputs_info;


using namespace std;
using namespace cv;

void vx_destory()
{
    if (nn_created == vx_true_e)
    {
        vxcReleaseNeuralNetwork();
    }
    
    if (input != NULL)
    {
        vxReleaseTensor(&input);
    }

    if (graph != NULL)
    {
        vxReleaseGraph(&graph);
    }

    if (context != NULL)
    {
        vxReleaseContext(&context);
    }
}

void vx_setup(char * model_name)
{
    vx_status status = VX_FAILURE;
    vx_uint32                   num_of_dims;
    vx_uint32                   image_size[NN_TENSOR_MAX_DIMENSION_NUMBER] = {0, 0, 0, 0};
    vx_uint32                   image_stride_size[NN_TENSOR_MAX_DIMENSION_NUMBER] = {0, 0, 0, 0};
    vx_enum                     data_format;
	vx_tensor_create_params_t   tensor_create_params;

    int                         i;
    /*============= Create VX context and build VX Alexnet graph ============ */
    context = vxCreateContext();
    _CHECK_OBJ(context, exit);
    graph = vxCreateGraph(context);
    _CHECK_OBJ(graph, exit);
    /* Prepare input tensor */
    num_of_dims    = NN_INPUT_DIMENSION_NUMBER;
    image_size[0]  = NN_INPUT_WIDTH;
    image_size[1]  = NN_INPUT_HEIGHT;
    image_size[2]  = NN_INPUT_CHANNEL;
    if (num_of_dims > 3) image_size[3]  = 1;
    data_format    = NN_INPUT_DATA_FORMAT;
    
    tensor_create_params.num_of_dims = num_of_dims;
    tensor_create_params.sizes = (vx_int32 *)image_size;
    tensor_create_params.data_format = data_format;
#if defined(NN_TENSOR_DATA_FORMAT_INT8) || defined(NN_TENSOR_DATA_FORMAT_INT16)
    tensor_create_params.quant_format = NN_INPUT_QUANT_TYPE;
    tensor_create_params.quant_data.dfp.fixed_point_pos = NN_INPUT_FIXED_POINT_POS;
#elif defined(NN_TENSOR_DATA_FORMAT_UINT8)
    tensor_create_params.quant_format = NN_INPUT_QUANT_TYPE;
    tensor_create_params.quant_data.affine.scale = NN_INPUT_AFFINE_SCALE;
    tensor_create_params.quant_data.affine.zeroPoint = NN_INPUT_AFFINE_ZERO_POINT;
#endif
    input = vxCreateTensor2(context, (const vx_tensor_create_params_t*)&tensor_create_params, sizeof(tensor_create_params));
    _CHECK_OBJ(input, exit);
    
    image_stride_size[0] = vxcGetTypeSize(data_format);
    for (i = 1; i < NN_INPUT_DIMENSION_NUMBER; i++)
    {
        image_stride_size[i] = image_stride_size[i-1] * image_size[i-1];
    }
	inputs_tensor_addressing       = vxCreateTensorAddressing(context, image_size, image_stride_size, NN_INPUT_DIMENSION_NUMBER);
    _CHECK_OBJ(inputs_tensor_addressing, exit);
	
	status = vxcCreateNeuralNetwork(graph, model_name, input, &outputs_info);
    _CHECK_STATUS(status, exit);
    nn_created = vx_true_e;

    status = vxVerifyGraph(graph);
    _CHECK_STATUS(status, exit);

    return;
exit:
    vx_destory();
}

vx_status vx_image_process(vx_uint8 * src,int width,int height ,char * image_name)
{
	vx_status status = VX_FAILURE;
    vx_uint32                   output_size[NN_TENSOR_MAX_DIMENSION_NUMBER];
    vx_uint32                   output_stride_size[NN_TENSOR_MAX_DIMENSION_NUMBER];
	vx_int8 *input_data_ptr = NULL;
    vx_int32 tmpdata; //zxw
    vx_uint32                   num_of_dims = 3;
    int                         size;
    vx_enum                     data_format;
	vx_enum                     quant_format;
	vx_int32                    zero_point;
	vx_float32                  scale;
	vx_uint8                    fix_point_pos;
    int channels = NN_INPUT_CHANNEL,i=0,j=0,offset=0;
	vx_tensor                   output = NULL;
    void*                       output_ptr = NULL;
	vx_tensor_addressing        output_user_addr = NULL;
	//unsigned char *orgPixel = NULL;
    vx_float32 * outBuf = NULL;
	
	data_format    = NN_INPUT_DATA_FORMAT;
	if(input_data_ptr == NULL)
		input_data_ptr = (vx_int8*) malloc(NN_INPUT_WIDTH * NN_INPUT_HEIGHT * channels * sizeof(vx_int8));
    if (input_data_ptr == NULL) return NULL;

    for (i = 0; i < channels; i++)
    {
        //offset = NN_INPUT_WIDTH * NN_INPUT_HEIGHT *( channels -1 - i);  // prapare BGR input data
		offset = 173056*(2-i);
        for (j = 0; j < 173056; j++)
        {
			tmpdata = (src[j * channels + i]>>2);
			input_data_ptr[j  + offset] = (vx_int8)((tmpdata >  127) ? 127 : (tmpdata < -128) ? -128 : tmpdata);
        }
    }
    _CHECK_OBJ(input_data_ptr, process_exit);
    status = vxCopyTensorPatch(input, NULL, inputs_tensor_addressing, input_data_ptr, VX_WRITE_ONLY, 0);  //input_data_ptr to src
    _CHECK_STATUS(status, process_exit);
	
    // do some clean up
	
	status = vxProcessGraph(graph);
    _CHECK_STATUS(status, process_exit);

    for (j=0; j < outputs_info.size;j++)
    {
        /* process result */
        output = outputs_info.tensors[j];
        status = vxQueryTensor(output, VX_TENSOR_NUM_OF_DIMS, &num_of_dims, sizeof(num_of_dims));
        _CHECK_STATUS(status, process_exit);

        status = vxQueryTensor(output, VX_TENSOR_DIMS, output_size, sizeof(output_size));
        _CHECK_STATUS(status, process_exit);

        status = vxQueryTensor(output, VX_TENSOR_DATA_TYPE, &data_format, sizeof(data_format));
        _CHECK_STATUS(status, process_exit);
        status = vxQueryTensor(output, VX_TENSOR_FIXED_POINT_POS, &fix_point_pos, sizeof(fix_point_pos));
        _CHECK_STATUS(status, process_exit);
        status = vxQueryTensor(output, VX_TENSOR_QUANT_FORMAT, &quant_format, sizeof(quant_format));
        status = vxQueryTensor(output, VX_TENSOR_ZERO_POINT, &zero_point, sizeof(zero_point));
        status = vxQueryTensor(output, VX_TENSOR_SCALE, &scale, sizeof(scale));

        output_stride_size[0] = vxcGetTypeSize(data_format);
        size = output_stride_size[0] * output_size[0];
        for (i = 1; i < (int)num_of_dims; i++)
        {
            output_stride_size[i] = output_stride_size[i-1] * output_size[i-1];
            size *= output_size[i];
        }
        output_ptr = malloc(size);

        _CHECK_OBJ(output_ptr, process_exit);

        output_user_addr = vxCreateTensorAddressing(
                context,
                &output_size[0],
                &output_stride_size[0],
                num_of_dims
                );
        _CHECK_OBJ(output_user_addr, process_exit);

        status = vxCopyTensorPatch(
                output,
                NULL,
                output_user_addr,
                output_ptr,
                VX_READ_ONLY,
                0
                );
        _CHECK_STATUS(status, process_exit);

        outBuf=showResult(output_ptr, size / output_stride_size[0],j,num_of_dims,output_size,data_format, quant_format,fix_point_pos, zero_point, scale, outputs_info.size);
		//orgPixel = (unsigned char *)prepareImageDataForDisplay("2.jpg",width,height,3);
		yolo_v2_post_process(NULL,outBuf,width,height,13,13,size/output_stride_size[0], (int *)output_size);

	//writeBMToFile("yolo_face_output.bmp",orgPixel,width, height,3,2);
    }

process_exit:
    if(outBuf != NULL)
    {
        free(outBuf);
        outBuf = NULL; 
    }
	if(input_data_ptr != NULL)
	{
		free(input_data_ptr);
		input_data_ptr = NULL;
	}
	
	if (output_ptr != NULL)
	{
		free(output_ptr);
		output_ptr = NULL;
	}

	if(output_user_addr)
	{
		vxReleaseTensorAddressing(&output_user_addr);
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
        printf("example: ./yolofaceint8  ./dynamic_fixed_point-8.export.data\n\n");
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
