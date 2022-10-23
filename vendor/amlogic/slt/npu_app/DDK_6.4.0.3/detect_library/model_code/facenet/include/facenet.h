#ifndef __FACENET_INCLUDE__
#define __FACENET_INCLUDE__

#include "vsi_nn_pub.h"
#include "nn_detect_common.h"
#include "nn_detect_utils.h"

det_status_t model_create(const char * data_file_path, dev_type type, void** context);
void model_getsize(int *width, int *height, int *channel);
void model_setinput(input_image_t imageData, uint8_t* data);
det_status_t model_getresult(pDetResult resultData, uint8_t* data);
void model_release(int flag);

#endif
