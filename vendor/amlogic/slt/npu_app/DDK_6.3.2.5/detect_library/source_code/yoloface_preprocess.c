#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "yoloface_preprocess.h"

void yolo_face_preprocess(input_image_t imageData, uint8_t **input_data_ptr)
{
	int nn_width, nn_height, channels, tmpdata;
	int offset=0, i=0, j=0;
	uint8_t *src = (uint8_t *)imageData.data;

	/* called after check_input_param 
	   so the model_size equals to img_size */
	nn_width = imageData.width;
	nn_height = imageData.height;
	channels = imageData.channel;

	if (NULL == *input_data_ptr) {
		*input_data_ptr = (uint8_t*) malloc(nn_width * nn_height * channels * sizeof(uint8_t));
		memset(*input_data_ptr, 0, nn_width * nn_height * channels * sizeof(uint8_t));
	}

	for (i = 0; i < channels; i++) {
        offset = nn_width * nn_height *( channels -1 - i);  // prapare BGR input data
        for (j = 0; j < nn_width * nn_height; j++) {
        	tmpdata = (src[j * channels + i]>>1);
        	(*input_data_ptr)[j + offset] = (uint8_t)((tmpdata >  127) ? 127 : (tmpdata < -128) ? -128 : tmpdata);
        }
    }
    return;
}
