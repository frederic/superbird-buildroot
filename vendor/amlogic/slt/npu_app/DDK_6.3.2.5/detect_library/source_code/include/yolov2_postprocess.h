#ifndef __YOLOV2_POSTPROCESS__
#define __YOLOV2_POSTPROCESS__

#include "vnn_global.h"
#include "nn_detect_common.h"

void yolov2_postprocess(vsi_nn_graph_t *graph, pDetResult resultData);

#endif