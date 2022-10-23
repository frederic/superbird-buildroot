#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "facenet_process.h"
#include "facenet.h"

#define NN_TENSOR_MAX_DIMENSION_NUMBER 4

/*Preprocess*/
void facenet_preprocess(input_image_t imageData, uint8_t *ptr)
{
    int nn_width, nn_height, channels, tmpdata;
    int offset=0, i=0, j=0;
    uint8_t *src = (uint8_t *)imageData.data;
    uint32_t reorder[3] = {0,1,2};
    int order;

    model_getsize(&nn_width, &nn_height, &channels);
    memset(ptr, 0, nn_width * nn_height * channels * sizeof(uint8_t));

    if (imageData.pixel_format == PIX_FMT_NV21) {
        offset = nn_width * nn_height;

        for (j = 0; j < nn_width * nn_height; j++) {
            tmpdata = (src[j]>>1);
            tmpdata = (uint8_t)((tmpdata >  127) ? 127 : (tmpdata < -128) ? -128 : tmpdata);
            ptr[j] = tmpdata;
            ptr[j + offset*1] = tmpdata;
            ptr[j + offset*2] = tmpdata;
        }
        return;
    }

   /*
    for (i = 0; i < channels; i++) {
        offset = nn_width * nn_height *( channels -1 - i);  // prapare BGR input data
        for (j = 0; j < nn_width * nn_height; j++) {
        	tmpdata = (src[j * channels + i]>>1);
        	ptr[j + offset] = (uint8_t)((tmpdata >  127) ? 127 : (tmpdata < -128) ? -128 : tmpdata);
        }
    }
    */

    for (i = 0; i < nn_width * nn_height; i++) {
        offset =  channels*i;
        for (j = 0; j < channels; j++) {
            order = reorder[j];
            tmpdata = (src[offset + order]>>1);
            ptr[j + offset] = (uint8_t)((tmpdata >  127) ? 127 : (tmpdata < -128) ? -128 : tmpdata);
        }
    }
    return;
}

/* Postprocess */
void facenet_postprocess(vsi_nn_graph_t *graph, pDetResult resultData)
{
    uint32_t i;
    vsi_nn_tensor_t *tensor;

    int output_len = 128;
    uint8_t *tensor_data = NULL;
    float *buffer = NULL,sum = 0;

    resultData->detect_num = output_len;
    buffer = resultData->facenet_result;
    tensor = vsi_nn_GetTensor(graph, graph->output.tensors[0]);
    tensor_data = (uint8_t *)vsi_nn_ConvertTensorToData(graph, tensor);
    for (i = 0; i < output_len; i++) {
        vsi_nn_DtypeToFloat32(&tensor_data[i], &buffer[i], &tensor->attr.dtype);
        sum = buffer[i]*buffer[i]+sum;
    }

    sum = 1.0/sum;
    for (i = 0; i < output_len; i++) {
        buffer[i] = sqrt(sum)*buffer[i];
    }
    if (tensor_data) vsi_nn_Free(tensor_data);
}