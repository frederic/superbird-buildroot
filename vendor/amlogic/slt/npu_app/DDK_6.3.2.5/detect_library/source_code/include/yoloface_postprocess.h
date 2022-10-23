#ifndef __YOLOFACE_POSTPROCESS__
#define __YOLOFACE_POSTPROCESS__

#include "vnn_global.h"
#include "nn_detect_common.h"

void yolo_face_postprocess(vsi_nn_graph_t *graph, pDetResult resultData);

#endif