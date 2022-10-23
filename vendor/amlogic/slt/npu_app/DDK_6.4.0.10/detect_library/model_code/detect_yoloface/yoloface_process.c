#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "yoloface_process.h"
#include "yoloface.h"
#define NN_TENSOR_MAX_DIMENSION_NUMBER 4

/*Preprocess*/
void yolo_face_preprocess(input_image_t imageData, uint8_t *ptr)
{
    int nn_width, nn_height, channels, tmpdata;
    int offset=0, i=0, j=0;
    uint8_t *src = (uint8_t *)imageData.data;

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

    for (i = 0; i < channels; i++) {
        offset = nn_width * nn_height *( channels -1 - i);  // prapare BGR input data
        for (j = 0; j < nn_width * nn_height; j++) {
            tmpdata = (src[j * channels + i]>>1);
            ptr[j + offset] = (uint8_t)((tmpdata >  127) ? 127 : (tmpdata < -128) ? -128 : tmpdata);
        }
    }
    return;
}

/* Postprocess */
static float logistic_activate(float x){return 1./(1. + exp(-x));}

box get_region_box(float *x, float *biases, int n, int index, int i, int j, int w, int h)
{
    box b;

    b.x = (i + logistic_activate(x[index + 0])) / w;
    b.y = (j + logistic_activate(x[index + 1])) / h;
    b.w = exp(x[index + 2]) * biases[2*n]   / w;
    b.h = exp(x[index + 3]) * biases[2*n+1] / h;
    return b;
}

void flatten(float *x, int size, int layers, int batch, int forward)
{
    float *swap = (float*)calloc(size*layers*batch, sizeof(float));
    int i,c,b;
    for (b = 0; b < batch; ++b) {
        for (c = 0; c < layers; ++c) {
            for (i = 0; i < size; ++i) {
                int i1 = b*layers*size + c*size + i;
                int i2 = b*layers*size + i*layers + c;
                if (forward) swap[i2] = x[i1];
                else swap[i1] = x[i2];
            }
        }
    }
    memcpy(x, swap, size*layers*batch*sizeof(float));
    free(swap);
}

void softmax(float *input, int n, float temp, float *output)
{
    int i;
    float sum = 0;
    float largest = -FLT_MAX;
    for (i = 0; i < n; ++i) {
        if (input[i] > largest) largest = input[i];
    }
    for (i = 0; i < n; ++i) {
        float e = exp(input[i]/temp - largest/temp);
        sum += e;
        output[i] = e;
    }
    for (i = 0; i < n; ++i) {
        output[i] /= sum;
    }
}

float overlap(float x1, float w1, float x2, float w2)
{
    float l1 = x1 - w1/2;
    float l2 = x2 - w2/2;
    float left = l1 > l2 ? l1 : l2;
    float r1 = x1 + w1/2;
    float r2 = x2 + w2/2;
    float right = r1 < r2 ? r1 : r2;
    return right - left;
}

float box_intersection(box a, box b)
{
    float area = 0;
    float w = overlap(a.x, a.w, b.x, b.w);
    float h = overlap(a.y, a.h, b.y, b.h);
    if (w < 0 || h < 0)
        return 0;
    area = w*h;
    return area;
}


float box_union(box a, box b)
{
    float i = box_intersection(a, b);
    float u = a.w*a.h + b.w*b.h - i;
    return u;
}

float box_iou(box a, box b)
{
    return box_intersection(a, b)/box_union(a, b);
}

int nms_comparator(const void *pa, const void *pb)
{
    sortable_bbox a = *(sortable_bbox *)pa;
    sortable_bbox b = *(sortable_bbox *)pb;
    float diff = a.probs[a.index][b.classId] - b.probs[b.index][b.classId];
    if (diff < 0) return 1;
    else if(diff > 0) return -1;
    return 0;
}

void do_nms_sort(box *boxes, float **probs, int total, int classes, float thresh)
{
    int i, j, k;
    sortable_bbox *s = (sortable_bbox *)calloc(total, sizeof(sortable_bbox));

    for (i = 0; i < total; ++i) {
        s[i].index = i;
        s[i].classId = 0;
        s[i].probs = probs;
    }

    for (k = 0; k < classes; ++k) {
        for (i = 0; i < total; ++i) {
            s[i].classId = k;
        }
        qsort(s, total, sizeof(sortable_bbox), nms_comparator);
        for (i = 0; i < total; ++i) {
            if (probs[s[i].index][k] == 0)
                continue;
            for (j = i+1; j < total; ++j) {
                box b = boxes[s[j].index];
                if (box_iou(boxes[s[i].index], b) > thresh) {
                    probs[s[j].index][k] = 0;
                }
            }
        }
    }
    free(s);
}

int max_index(float *a, int n)
{
    int i, max_i = 0;
    float max = a[0];

    if (n <= 0)
        return -1;

    for (i = 1; i < n; ++i) {
        if (a[i] > max) {
            max = a[i];
            max_i = i;
        }
    }
    return max_i;
}

static void get_detections_result(pDetResult resultData, int num, float thresh, box *boxes, float **probs,  int classes)
{
    int i,detect_num = 0;
    float left = 0, right = 0, top = 0, bot=0;

    memset(resultData, 0, sizeof(*resultData));
    for (i = 0; i < num; ++i) {
        int classId = max_index(probs[i], classes);
        float prob = probs[i][classId];
        if (prob > thresh) {
            #if 0
            left  = (boxes[i].x-boxes[i].w/2.)*width;
            right = (boxes[i].x+boxes[i].w/2.)*width;
            top   = (boxes[i].y-boxes[i].h/2.)*height;
            bot   = (boxes[i].y+boxes[i].h/2.)*height;

            if (left < 0) left = 0;
            if (right > width-1) right = width-1;
            if (top < 0) top = 0;
            if (bot > height-1) bot = height-1;
            #endif
            if (detect_num >= MAX_DETECT_NUM) {
                break;
            }
            left  = boxes[i].x-boxes[i].w/2.;
            right = boxes[i].x+boxes[i].w/2.;
            top   = boxes[i].y-boxes[i].h/2.;
            bot   = boxes[i].y+boxes[i].h/2.;

            if (left < 0) left = 0;
            if (right > 1) right = 1.0;
            if (top < 0) top = 0;
            if (bot > 1) bot = 1.0;

            resultData->point[detect_num].type = DET_RECTANGLE_TYPE;
            resultData->point[detect_num].point.rectPoint.left = left;
            resultData->point[detect_num].point.rectPoint.top = top;
            resultData->point[detect_num].point.rectPoint.right = right;
            resultData->point[detect_num].point.rectPoint.bottom = bot;
            resultData->result_name[detect_num].lable_id = classId;
            detect_num ++;
        }
    }
    resultData->detect_num= detect_num;
}


static int yolo_post_process(pDetResult resultData, float *predictions, int modelWidth, int modelHeight, int input_num, int *input_size )
{
    int i,j,n;
    float threshold = 0.24;
    int num_class = 1;
    int num_box = 5;
    int grid_size = 13;
    float biases[10] = {1.08,1.19,  3.42,4.41,  6.63,11.38,  9.42,5.11,  16.62,10.52};

    box *boxes = (box *)calloc(modelWidth*modelHeight*num_box, sizeof(box));
    float **probs = (float **)calloc(modelWidth*modelHeight*num_box, sizeof(float *));

    for (j = 0; j < modelWidth*modelHeight*num_box; ++j)
        probs[j] = (float *)calloc(num_class+1, sizeof(float *));

    {
        int i,b;
        int coords = 4,classes = 1;
        int size = coords + classes + 1;
        #if 1
        int w = input_size[0];
        int h = input_size[1];
        int n = input_size[2]/size;
        #else
        int w = 13;
        int h = 13;
        int n = 5;
        #endif
        int batch = 1;
        flatten(predictions, w*h, size*n, batch, 1);
        for (b = 0; b < batch; ++b) {
            for (i = 0; i < h*w*n; ++i) {
                int index = size*i + b*input_num;
                predictions[index + 4] = logistic_activate(predictions[index + 4]);
            }
        }

        for (b = 0; b < batch; ++b) {
            for (i = 0; i < h*w*n; ++i) {
                int index = size*i + b*input_num;
                softmax(predictions + index + 5, classes, 1, predictions + index + 5);
            }
        }
    }

    for (i = 0; i < modelWidth*modelHeight; ++i) {
        int row = i / modelWidth;
        int col = i % modelWidth;
        for (n = 0; n < num_box; ++n) {
            int index = i*num_box + n;
            int p_index = index * (num_class + 5) + 4;
            float scale = predictions[p_index];
            int box_index = index * (num_class + 5);
            int class_index = 0;
            boxes[index] = get_region_box(predictions, biases, n, box_index, col, row, modelWidth, modelHeight);

            class_index = index * (num_class + 5) + 5;
            for (j = 0; j < num_class; ++j) {
                float prob = scale*predictions[class_index+j];
                probs[index][j] = (prob > threshold) ? prob : 0;
            }
        }
    }
    do_nms_sort(boxes, probs, grid_size*grid_size*num_box, num_class, 0.4);

    get_detections_result(resultData, grid_size*grid_size*num_box, threshold, boxes, probs, num_class);

    free(boxes);
    boxes = NULL;

    for (j = 0; j < grid_size*grid_size*num_box; ++j)
        free(probs[j]);
    free(probs);

    return 0;
}

void yolo_face_postprocess(vsi_nn_graph_t *graph, pDetResult resultData)
{
    uint32_t i,j,stride;
    vsi_nn_tensor_t *tensor;

    uint8_t *tensor_data = NULL;
    int output_len = 0;
    int output_cnt = 0;
    float *buffer= NULL;
    int sz[10];

    vx_uint32   output_size[NN_TENSOR_MAX_DIMENSION_NUMBER] = {0};

    for (i = 0; i < graph->output.num; i++) {
        tensor = vsi_nn_GetTensor(graph, graph->output.tensors[i]);
        sz[i] = 1;
        for (j = 0; j < tensor->attr.dim_num; j++) {
            sz[i] *= tensor->attr.size[j];
        }
        output_len += sz[i];
        output_size[0] = tensor->attr.size[0];
        output_size[1] = tensor->attr.size[1];
        output_size[2] = tensor->attr.size[2];
    }

    buffer = (float *)malloc(sizeof(float) * output_len);
    for (i = 0; i < graph->output.num; i++) {
        tensor = vsi_nn_GetTensor(graph, graph->output.tensors[i]);

        stride = vsi_nn_TypeGetBytes(tensor->attr.dtype.vx_type);
        tensor_data = (uint8_t *)vsi_nn_ConvertTensorToData(graph, tensor);

        float fl = pow(2.,-tensor->attr.dtype.fl);
        for (j = 0; j < sz[i]; j++) {
            int val = tensor_data[stride*j];
            int tmp1=(val>=128)?val-256:val;
            buffer[output_cnt]= tmp1*fl;
            output_cnt++;
        }
        vsi_nn_Free(tensor_data);
    }

    yolo_post_process(resultData,buffer,13,13,output_len, (int *)output_size);

    if (buffer) free(buffer);
	return;
}
