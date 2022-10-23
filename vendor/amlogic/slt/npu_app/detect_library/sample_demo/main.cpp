#include <iostream>
#include <fstream>

#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>

#include "nn_detect.h"
#include "nn_detect_utils.h"

using namespace std;
using namespace cv;

#define _CHECK_STATUS_(status, stat, lbl) do {\
	if (status != stat) \
	{ \
		cout << "_CHECK_STATUS_ File" << __FUNCTION__ << __LINE__ <<endl; \
	}\
	goto lbl; \
}while(0)

static void draw_results(IplImage *pImg, DetectResult resultData, int img_width, int img_height, det_model_type type)
{
	int i = 0;
	float left, right, top, bottom;
	CvFont font;
    cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1, 1,0,3,8);

	cout << "\nresultData.detect_num=" << resultData.detect_num <<endl;
	cout << "result type is " << resultData.point[i].type << endl;
	for (i = 0; i < resultData.detect_num; i++) {
		left =  resultData.point[i].point.rectPoint.left*img_width;
        right = resultData.point[i].point.rectPoint.right*img_width;
        top = resultData.point[i].point.rectPoint.top*img_height;
        bottom = resultData.point[i].point.rectPoint.bottom*img_height;
#if 0
		#define XRATE 3.0
		#define YRATE 2.25
		cout << "i:" <<resultData.detect_num <<" left:" << resultData.point[i].point.rectPoint.left <<" right:" << resultData.point[i].point.rectPoint.right ;
        cout << " top:" << resultData.point[i].point.rectPoint.top << " bottom:" << resultData.point[i].point.rectPoint.bottom <<endl;
        left =  resultData.point[i].point.rectPoint.left*XRATE;
        right = resultData.point[i].point.rectPoint.right*XRATE;
        top = resultData.point[i].point.rectPoint.top*YRATE;
        bottom = resultData.point[i].point.rectPoint.bottom*YRATE;
#endif
		cout << "i:" <<resultData.detect_num <<" left:" << left <<" right:" << right << " top:" << top << " bottom:" << bottom <<endl;
		CvPoint pt1;
		CvPoint pt2;
		pt1=cvPoint(left,top);
		pt2=cvPoint(right, bottom);

		cvRectangle(pImg,pt1,pt2,CV_RGB(10,10,250),3,4,0);
		switch (type) {
			case DET_YOLOFACE_V2:
			break;
			case DET_MTCNN_V1:
			{
				int j = 0;
				cv::Mat testImage;
				testImage = cv::cvarrToMat(pImg);
				for (j = 0; j < 5; j ++) {
					cv::circle(testImage, cv::Point(resultData.point[i].tpts.floatX[j]*img_width, resultData.point[i].tpts.floatY[j]*img_height), 2, cv::Scalar(0, 255, 255), 2);
				}
				break;
			}
			case DET_YOLO_V2:
			case DET_YOLO_V3:
			{
				if (top < 50) {
					top = 50;
					left +=10;
					cout << "left:" << left << " top-10:" << top-10 <<endl;
				}
				cvPutText(pImg, resultData.result_name[i].lable_name, cvPoint(left,top-10), &font, CV_RGB(0,255,0));
				break;
			}
			default:
			break;
		}
	}
}

static void crop_face(cv::Mat sourceFrame, cv::Mat& imageROI, DetectResult resultData, int img_height, int img_width) {
	float left, right, top, bottom;
	int tempw,temph;
	CvFont font;

	left =  resultData.point[0].point.rectPoint.left*img_width;
    right = resultData.point[0].point.rectPoint.right*img_width;
    top = resultData.point[0].point.rectPoint.top*img_height;
    bottom = resultData.point[0].point.rectPoint.bottom*img_height;

    tempw = abs((int)(left - right));
	temph = abs((int)(top - bottom));
	if (tempw > 1) tempw = tempw -1;
	if (temph > 1) temph = temph -1;

	if (left + tempw > img_width)
		tempw = img_width - left;
	if (bottom + temph > img_height)
		temph = img_height - top;

	imageROI = sourceFrame(cv::Rect(left, top, tempw, temph));
	cv::imwrite("face.bmp", imageROI);
	return;
}


int run_detect_model(int argc, char** argv)
{
	int ret = 0;
	int nn_height, nn_width, nn_channel, img_width, img_height;
	det_model_type type = DET_YOLOFACE_V2;
	DetectResult resultData;

	if (argc !=3) {
		cout << "input param error" <<endl;
		cout << "Usage: " << argv[0] << " type  picture_path"<<endl;
		return -1;
	}
	type = (det_model_type)atoi(argv[1]);

	char* picture_path = argv[2];
	det_set_log_config(DET_DEBUG_LEVEL_WARN,DET_LOG_TERMINAL);
	cout << "det_set_log_config Debug" <<endl;

	//prepare model
	ret = det_set_model(type);
	if (ret) {
		cout << "det_set_model fail. ret=" << ret <<endl;
		return ret;
	}
	cout << "det_set_model success!!" << endl;

	ret = det_get_model_size(type, &nn_width, &nn_height, &nn_channel);
	if (ret) {
		cout << "det_get_model_size fail" <<endl;
		return ret;
	}

	cout << "\nmodel.width:" << nn_width <<endl;
	cout << "model.height:" << nn_height <<endl;
	cout << "model.channel:" << nn_channel << "\n" <<endl;

	//cv::Mat testImage;
	IplImage* frame2process = cvLoadImage(picture_path,CV_LOAD_IMAGE_COLOR);
	if (!frame2process) {
		cout << "Picture : "<< picture_path << " load fail" <<endl;
		det_release_model(type);
		return -1;
	}

	cv::Mat testImage(nn_height,nn_width,CV_8UC1);
	cv::Mat sourceFrame = cvarrToMat(frame2process);
	cv::resize(sourceFrame, testImage, testImage.size());
	img_width = sourceFrame.cols;
	img_height = sourceFrame.rows;

	input_image_t image;
	image.data		= testImage.data;
	image.width 	= testImage.cols;
	image.height 	= testImage.rows;
	image.channel 	= testImage.channels();
	image.pixel_format = PIX_FMT_RGB888;

	cout << "Det_set_input START" << endl;
	ret = det_set_input(image, type);
	if (ret) {
		cout << "det_set_input fail. ret=" << ret << endl;
		det_release_model(type);
		return ret;
	}
	cout << "Det_set_input END" << endl;

	cout << "Det_get_result START" << endl;
	ret = det_get_result(&resultData, type);
	if (ret) {
		cout << "det_get_result fail. ret=" << ret << endl;
		det_release_model(type);
		return ret;
	}
	cout << "Det_get_result END" << endl;

	draw_results(frame2process, resultData, img_width, img_height, type);
	cv::imwrite("output.bmp", sourceFrame);

	det_release_model(type);
	cvReleaseImage(&frame2process);
	return ret;
}

int run_detect_facent(int argc, char** argv)
{
	if (argc != 4) {
		cout << "input param error" <<endl;
		cout << "Usage: " << argv[0] << " type  picture_path facenet_falge"<<endl;
		cout << "facenet_falge: 0-->write to emb.db, 1--> facenet inference" <<endl;
		return -1;
	}

	int ret = 0;
	int nn_height, nn_width, nn_channel, img_width, img_height;
	det_model_type type = DET_YOLOFACE_V2;
	DetectResult resultData;

	char* picture_path = argv[2];
	int facenet_falge =(int)atoi(argv[3]);

	det_set_log_config(DET_DEBUG_LEVEL_WARN,DET_LOG_TERMINAL);
	//prepare model
	ret = det_set_model(type);
	if (ret) {
		cout << "det_set_model fail. ret=" << ret <<endl;
		return ret;
	}

	ret = det_get_model_size(type, &nn_width, &nn_height, &nn_channel);
	if (ret) {
		cout << "det_get_model_size fail" <<endl;
		return ret;
	}

	IplImage* frame2process = cvLoadImage(picture_path,CV_LOAD_IMAGE_COLOR);
	if (!frame2process) {
		cout << "Picture : "<< picture_path << " load fail" <<endl;
		det_release_model(type);
		return -1;
	}

	cv::Mat testImage(nn_height,nn_width,CV_8UC1);
	cv::Mat sourceFrame = cvarrToMat(frame2process);
	cv::resize(sourceFrame, testImage, testImage.size());
	img_width = sourceFrame.cols;
	img_height = sourceFrame.rows;

	input_image_t image;
	image.data		= testImage.data;
	image.width 	= testImage.cols;
	image.height 	= testImage.rows;
	image.channel 	= testImage.channels();
	image.pixel_format = PIX_FMT_RGB888;
	ret = det_set_input(image, type);
	if (ret) {
		cout << "det_set_input fail. ret=" << ret << endl;
		det_release_model(type);
		return ret;
	}

	ret = det_get_result(&resultData, type);
	if (ret) {
		cout << "det_get_result fail. ret=" << ret << endl;
		det_release_model(type);
		return ret;
	}
	det_release_model(type);

	if (resultData.detect_num == 0) {
		cout << "No face detected " <<endl;
		return -1;
	}

	cv::Mat imageROI;
	cv::Size ResImgSiz = cv::Size(160, 160);
	cv::Mat  ResImg160 = cv::Mat(ResImgSiz, imageROI.type());

	crop_face(sourceFrame, imageROI, resultData, img_height, img_width);
	cv::resize(imageROI, ResImg160, ResImgSiz, CV_INTER_NN);
	image.data		= ResImg160.data;
	image.width 	= ResImg160.cols;
	image.height 	= ResImg160.rows;
	image.channel 	= ResImg160.channels();
	image.pixel_format = PIX_FMT_RGB888;
	cv::imwrite("face_160.bmp", ResImg160);

	type = DET_FACENET;
	ret = det_set_model(type);
	if (ret) {
		cout << "det_set_model fail. ret=" << ret <<endl;
		return ret;
	}

	ret = det_get_model_size(type, &nn_width, &nn_height, &nn_channel);
	if (ret) {
		cout << "det_get_model_size fail" <<endl;
		return ret;
	}

	ret = det_set_input(image, type);
	if (ret) {
		cout << "det_set_input fail. ret=" << ret << endl;
		det_release_model(type);
		return ret;
	}

	ret = det_get_result(&resultData, type);
	if (ret) {
		cout << "det_get_result fail. ret=" << ret << endl;
		det_release_model(type);
		return ret;
	}

	FILE *fp;
	if (!facenet_falge) {
		fp = fopen("emb.db","ab+");
		if (fp == NULL)	{
			cout << "open fp out_emb fail" << endl;
			return -1;
		}
		fwrite((void *)resultData.facenet_result,128,sizeof(float),fp);
		fclose(fp);
	} else {
		#define NAME_NUM 3
		string name[NAME_NUM]={"deng.liu","jian.cao", "xxxx.xie"};

		fp = fopen("emb.db","rb");
		if (fp == NULL)	{
			cout << "open fp out_emb fail" << endl;
			return -1;
		}

		float sum = 0,temp=0, mindis = -1;
		float threshold = 0.6;
		int index =0, i, j;
		float tempbuff[128];
		float* buffer = resultData.facenet_result;

		for (i=0; i < NAME_NUM; i++)
		{
			memset(tempbuff,0,128*sizeof(float));
			fread(tempbuff,128,sizeof(float),fp);

			sum = 0;
			for (j = 0;j< 128;j++) {
				temp = tempbuff[j]-buffer[j];
				sum = sum + temp*temp;
			}
			temp = sqrt(sum);
			cout <<"i=" << i << " temp=" << temp <<endl;

			//first result
			if (i == 0) {
				mindis = temp;
				index = i;
			} else {
				if (temp < mindis ) {
					mindis = temp;
					index = i;
				}
			}
		}

		cout << "mindis:" << mindis <<", index=" << index <<endl;
		if (mindis < threshold) {
			cout <<"face detected,your id is "<< index << ", name is "<< name[index].c_str() <<endl;
		}
		fclose(fp);
	}

	det_release_model(type);
	cvReleaseImage(&frame2process);
	return ret;
}

int main(int argc, char** argv)
{
	det_model_type type;
	if (argc < 3) {
		cout << "input param error" <<endl;
		cout << "Usage: " << argv[0] << " type  picture_path"<<endl;
		return -1;
	}

	type = (det_model_type)atoi(argv[1]);
	switch (type) {
		case DET_YOLOFACE_V2:
		case DET_YOLO_V2:
		case DET_YOLO_V3:
		case DET_MTCNN_V1:
			run_detect_model(argc, argv);
			break;
		case DET_FACENET:
			run_detect_facent(argc, argv);
			break;
		default:
			cerr << "not support type=" << type <<endl;
			break;
	}

	return 0;
}
