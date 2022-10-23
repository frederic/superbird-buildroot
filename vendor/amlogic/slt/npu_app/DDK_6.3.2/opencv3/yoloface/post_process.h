#ifndef  POST_PROCESS_H_
#define  POST_PROCESS_H_

#ifdef _cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>


typedef struct{
    float x, y, w, h;
} box;


typedef struct{
    int index;
    int classId;
    float **probs;
} sortable_bbox;

#define MAX_DETECT_NUM 100

typedef struct _DetectResult {
   char name[MAX_DETECT_NUM][64];
   int  detect_num;
   CvPoint pt1[MAX_DETECT_NUM];
   CvPoint pt2[MAX_DETECT_NUM];
}DetectResult;


//int yolo_v2_post_process(unsigned char *src, float *predictions, int width, int height, int modelWidth, int modelHeight, int input_num, int *input_size );

#ifdef _cplusplus
}
#endif
#endif

