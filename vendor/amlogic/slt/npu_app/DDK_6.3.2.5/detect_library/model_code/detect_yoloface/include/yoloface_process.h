#ifndef __YOLOFACE_PROCESS__
#define __YOLOFACE_PROCESS__

#include "vnn_global.h"
#include "nn_detect_common.h"

typedef unsigned char   uint8_t;

void yolo_face_preprocess(input_image_t imageData, uint8_t *ptr);
void yolo_face_postprocess(vsi_nn_graph_t *graph, pDetResult resultData);

#endif