#ifndef __YOLOV2_PREPROCESS__
#define __YOLOV2_PREPROCESS__

#include "nn_detect_common.h"

typedef unsigned char   uint8_t;

void yolov2_preprocess(input_image_t imageData, uint8_t **input_data_ptr);

#endif