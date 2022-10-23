#include <math.h>
#include <stdlib.h>
#include <float.h>

#include "yolov3_postprocess.h"

//char *voc_names[] = {"aeroplane", "bicycle", "bird", "boat", "bottle", "bus", "car", "cat", "chair", "cow", "diningtable", "dog", "horse", "motorbike", "person", "pottedplant", "sheep", "sofa", "train", "tvmonitor"};
static char *coco_names[] = {"person","bicycle","car","motorbike","aeroplane","bus","train","truck","boat","traffic light","fire hydrant","stop sign","parking meter","bench","bird","cat","dog","horse","sheep","cow","elephant","bear","zebra","giraffe","backpack","umbrella","handbag","tie","suitcase","frisbee","skis","snowboard","sports ball","kite","baseball bat","baseball glove","skateboard","surfboard","tennis racket","bottle","wine glass","cup","fork","knife","spoon","bowl","banana","apple","sandwich","orange","broccoli","carrot","hot dog","pizza","donut","cake","chair","sofa","pottedplant","bed","diningtable","toilet","tvmonitor","laptop","mouse","remote","keyboard","cell phone","microwave","oven","toaster","sink","refrigerator","book","clock","vase","scissors","teddy bear","hair drier","toothbrush"};


static float overlap(float x1, float w1, float x2, float w2)
{
    float l1 = x1 - w1/2;
    float l2 = x2 - w2/2;
    float left = l1 > l2 ? l1 : l2;
    float r1 = x1 + w1/2;
    float r2 = x2 + w2/2;
    float right = r1 < r2 ? r1 : r2;
    return right - left;
}

static float box_intersection(box a, box b)
{
	float area = 0;
    float w = overlap(a.x, a.w, b.x, b.w);
    float h = overlap(a.y, a.h, b.y, b.h);
    if (w < 0 || h < 0)
		return 0;
    area = w*h;
    return area;
}

static float box_union(box a, box b)
{
    float i = box_intersection(a, b);
    float u = a.w*a.h + b.w*b.h - i;
    return u;
}

static float box_iou(box a, box b)
{
    return box_intersection(a, b)/box_union(a, b);
}

static int nms_comparator(const void *pa, const void *pb)
{
    sortable_bbox a = *(sortable_bbox *)pa;
    sortable_bbox b = *(sortable_bbox *)pb;
    float diff = a.probs[a.index][b.classId] - b.probs[b.index][b.classId];
    if (diff < 0) return 1;
    else if (diff > 0) return -1;
    return 0;
}

static void do_nms_sort(box *boxes, float **probs, int total_in, int classes, float thresh)
{
    int i, j, k;
    sortable_bbox *s = (sortable_bbox *)calloc(total_in, sizeof(sortable_bbox));

    int total = 0;
    for (i = 0; i < total_in; ++i) {
    	if (boxes[i].prob_obj>0) {
    		s[total].index = i;
    		s[total].classId = 0;
    		s[total].probs = probs;
    		total++;
    	}
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
                if (probs[s[j].index][k]>0) {
                	if (box_iou(boxes[s[i].index], b) > thresh) {
                		probs[s[j].index][k] = 0;
                	}
                }
            }
        }
    }
    free(s);
}

static int max_index(float *a, int n)
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

float colors[6][3] = { {1,0,1}, {0,0,1},{0,1,1},{0,1,0},{1,1,0},{1,0,0} };

float get_color(int c, int x, int max)
{
    float ratio = ((float)x/max)*5;
    int i = floor(ratio);
    int j = ceil(ratio);
	float r = 0;
    ratio -= i;
    r = (1-ratio) * colors[i][c] + ratio*colors[j][c];
    return r;
}

static void get_detections_result(pDetResult resultData, int num, float thresh, box *boxes, float **probs, char **names, int classes)
{
    int i,detect_num = 0;
    float left = 0, right = 0, top = 0, bot=0;

    memset(resultData, 0, sizeof(*resultData));
    for (i = 0; i < num; ++i) {
    	if (boxes[i].prob_obj<=thresh) continue;
        int classId = max_index(probs[i], classes);
        float prob = probs[i][classId];
        if (prob > thresh) {
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
            sprintf(resultData->result_name[detect_num].lable_name, "%s: %.0f%% ", names[classId], prob*100);

            detect_num ++;
		}
	}
	resultData->detect_num= detect_num;
}

static float logistic_activate(float x){return 1./(1. + exp(-x));}

static box get_region_box(float *x, float *biases, int n, int index, int i, int j, int w, int h)
{
    box b;
    b.x = (i + logistic_activate(x[index + 0])) / w;
    b.y = (j + logistic_activate(x[index + 1])) / h;
    b.w = exp(x[index + 2]) * biases[2*n]   / w;
    b.h = exp(x[index + 3]) * biases[2*n+1] / h;
    return b;
}

static void flatten(float *x, int size, int layers, int batch, int forward)
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

int yolo_v3_post_process_onescale(float *predictions, int input_size[3] , float *biases, box *boxes, float **probs, float threshold_in)
{
	int i,j;
	int num_class = 80;
	int coords = 4;
	int bb_size = coords + num_class + 1;
	int num_box = input_size[2]/bb_size;
	int modelWidth = input_size[0];
	int modelHeight = input_size[1];
	float threshold=threshold_in;

	for (j = 0; j < modelWidth*modelHeight*num_box; ++j)
		probs[j] = (float *)calloc(num_class+1, sizeof(float *));

    int ck0, batch = 1;
	flatten(predictions, modelWidth*modelHeight, bb_size*num_box, batch, 1);

	for (i = 0; i < modelHeight*modelWidth*num_box; ++i) {
		for (ck0=coords;ck0<bb_size;ck0++ ) {
			int index = bb_size*i;

			predictions[index + ck0] = logistic_activate(predictions[index + ck0]);
			if (ck0==coords){
				if (predictions[index+ck0]<=threshold) {
					break;
				}
			}
		}
	}


	for (i = 0; i < modelWidth*modelHeight; ++i) {
		int row = i / modelWidth;
		int col = i % modelWidth;
		int n =0;
		for (n = 0; n < num_box; ++n) {
			int index = i*num_box + n;
			int p_index = index * bb_size + 4;
			float scale = predictions[p_index];
			int box_index = index * bb_size;
			int class_index = 0;
			class_index = index * bb_size + 5;

			if (scale>threshold) {
				for (j = 0; j < num_class; ++j)	{
					float prob = scale*predictions[class_index+j];
					probs[index][j] = (prob > threshold) ? prob : 0;
				}
				boxes[index] = get_region_box(predictions, biases, n, box_index, col, row, modelWidth, modelHeight);
			}
			boxes[index].prob_obj = (scale>threshold)?scale:0;
		}
	}
	return 0;
}

void yolov3_postprocess(vsi_nn_graph_t *graph, pDetResult resultData)
{
	int nn_width,nn_height, nn_channel;
	vsi_nn_tensor_t *tensor = NULL;

	tensor = vsi_nn_GetTensor(graph, graph->input.tensors[0]);
	nn_width = tensor->attr.size[0];
	nn_height = tensor->attr.size[1];
	nn_channel = tensor->attr.size[2];
	(void)nn_channel;
	int size[3]={nn_width/32, nn_height/32,85*3};

	int sz[10];
    int i, j, stride;
    int output_cnt = 0;
    int output_len = 0;
    int num_class = 80;
    float threshold = 0.5;
    float iou_threshold = 0.4;
    uint8_t *tensor_data = NULL;
	float *predictions = NULL;

	for (i = 0; i < graph->output.num; i++) {
        tensor = vsi_nn_GetTensor(graph, graph->output.tensors[i]);
        sz[i] = 1;
        for (j = 0; j < tensor->attr.dim_num; j++) {
            sz[i] *= tensor->attr.size[j];
        }
        output_len += sz[i];
    }
    predictions = (float *)malloc(sizeof(float) * output_len);

    for (i = 0; i < graph->output.num; i++) {
        tensor = vsi_nn_GetTensor(graph, graph->output.tensors[i]);

        stride = vsi_nn_TypeGetBytes(tensor->attr.dtype.vx_type);
        tensor_data = (uint8_t *)vsi_nn_ConvertTensorToData(graph, tensor);

        float fl = pow(2.,-tensor->attr.dtype.fl);
        for (j = 0; j < sz[i]; j++) {
			int val = tensor_data[stride*j];
            int tmp1=(val>=128)?val-256:val;
            predictions[output_cnt]= tmp1*fl;
            output_cnt++;
        }
        vsi_nn_Free(tensor_data);
	}

    float biases[18] = {10/8., 13/8., 16/8., 30/8., 33/8., 23/8., 30/16., 61/16., 62/16., 45/16., 59/16., 119/16., 116/32., 90/32., 156/32., 198/32., 373/32., 326/32.};
    int size2[3] = {size[0]*2,size[1]*2,size[2]};
    int size4[3] = {size[0]*4,size[1]*4,size[2]};
    int len1 = size[0]*size[1]*size[2];
    int box1 = len1/(num_class+5);

	box *boxes = (box *)calloc(box1*(1+4+16), sizeof(box));
    float **probs = (float **)calloc(box1*(1+4+16), sizeof(float *));

    yolo_v3_post_process_onescale(&predictions[0], size, &biases[12], boxes, &probs[0], threshold); //final layer
    yolo_v3_post_process_onescale(&predictions[len1], size2, &biases[6], &boxes[box1], &probs[box1], threshold);
    yolo_v3_post_process_onescale(&predictions[len1*(1+4)], size4, &biases[0],  &boxes[box1*(1+4)], &probs[box1*(1+4)], threshold);
	do_nms_sort(boxes, probs, box1*21, num_class, iou_threshold);
	get_detections_result(resultData, box1*21, threshold, boxes, probs, coco_names, num_class);

	free(boxes);
	boxes = NULL;

    if (predictions) free(predictions);

	for (j = 0; j < box1*(1+4+16); ++j) {
        free(probs[j]);
    }
	free(probs);
	return;
}
