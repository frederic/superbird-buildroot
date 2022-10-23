#ifndef __YOLOV2_PROCESS__
#define __YOLOV2_PROCESS__

#include "vnn_global.h"
#include "nn_detect_common.h"

typedef unsigned char   uint8_t;

void yolov2_preprocess(input_image_t imageData, uint8_t *ptr);
void yolov2_postprocess(vsi_nn_graph_t *graph, pDetResult resultData);

#endif