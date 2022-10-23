#ifndef __YOLOV3_POSTPROCESS__
#define __YOLOV3_POSTPROCESS__

#include "vnn_global.h"
#include "nn_detect_common.h"

void yolov3_postprocess(vsi_nn_graph_t *graph, pDetResult resultData);

#endif