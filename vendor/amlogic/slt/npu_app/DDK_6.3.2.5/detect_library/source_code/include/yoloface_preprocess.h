#ifndef __YOLOFACE_PREPROCESS__
#define __YOLOFACE_PREPROCESS__

#include "nn_detect_common.h"

typedef unsigned char   uint8_t;

void yolo_face_preprocess(input_image_t imageData, uint8_t **input_data_ptr);

#endif