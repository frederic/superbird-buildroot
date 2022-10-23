#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "yolov3_preprocess.h"

void yolov3_preprocess(input_image_t imageData, uint8_t **input_data_ptr)
{
	int nn_width, nn_height, channels, tmpdata;
	int offset=0, i=0, j=0;
	uint8_t *src = (uint8_t *)imageData.data;

	/* called after check_input_param 
	so the model_size equals to img_size*/
	nn_width = imageData.width;
	nn_height = imageData.height;
	channels = imageData.channel;

	if (NULL == *input_data_ptr) {
		*input_data_ptr = (uint8_t*) malloc(nn_width * nn_height * channels * sizeof(uint8_t));
		memset(*input_data_ptr, 0, nn_width * nn_height * channels * sizeof(uint8_t));
	}

	#if 1
	for (i = 0; i < channels; i++) {
        offset = nn_width * nn_height *( channels -1 - i);  // prapare BGR input data
        for (j = 0; j < nn_width * nn_height; j++) {
        	tmpdata = (src[j * channels + i]>>1);
        	(*input_data_ptr)[j  + offset] = (uint8_t)((tmpdata >  127) ? 127 : (tmpdata < -128) ? -128 : tmpdata);
        }
    }
    #else
    float mean,scale,val;
	int fl = 2;//= ；pow(2., tensor->attr.dtype.fl);
	scale = mean_value[3];
	float mean_value[4] = {0.0,0.0,0.0,255.0};

    for (i = 0; i < channels; i ++) {
        mean = mean_value[i];
        offset = nn_width * nn_height * i;

        for (j = 0; j < nn_width * nn_height; j ++) {
            val = ((float)src[offset + j] - mean) / scale;
            int tmp2 = floor(val*fl+0.5);
            tmpdata = (tmp2>=128)?127:tmp2;
            (*input_data_ptr)[j + offset] = tmpdata;
        }
    }
	#endif

    return;
}