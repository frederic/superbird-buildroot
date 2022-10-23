#ifndef __YOLOV3_PROCESS__
#define __YOLOV3_PROCESS__
#include "vsi_nn_pub.h"
#include "nn_detect_common.h"
typedef unsigned char   uint8_t;
void yolov3_preprocess(input_image_t imageData, uint8_t *ptr);
void yolov3_postprocess(vsi_nn_graph_t *graph, pDetResult resultData);
#endif
