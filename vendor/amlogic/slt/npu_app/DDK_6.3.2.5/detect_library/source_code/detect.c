#include <stdio.h>
#include <stdlib.h>

#include "nn_detect.h"
#include "nn_detect_utils.h"
#include "detect_log.h"
#include "vnn_global.h"

#include "vnn_yoloface7d.h"
#include "vnn_yoloface88.h"
#include "yoloface_preprocess.h"
#include "yoloface_postprocess.h"

#include "vnn_yolov27d.h"
#include "vnn_yolov288.h"
#include "yolov2_preprocess.h"
#include "yolov2_postprocess.h"

#include "vnn_yolov37d.h"
#include "vnn_yolov388.h"
#include "yolov3_preprocess.h"
#include "yolov3_postprocess.h"

#include "vnn_facenet7d.h"
#include "vnn_facenet88.h"
#include "facenet_process.h"

#define _SET_STATUS_(status, stat, lbl) do {\
	status = stat; \
	goto lbl; \
}while(0)

typedef enum {
  DEV_REVA = 0,
  DEV_REVB,
  DEV_BUTT,
} dev_type;

typedef enum {
	NETWORK_UNINIT,
	NETWORK_INIT,
	NETWORK_PREPARING,
	NETWORK_PROCESSING,
} network_status;

typedef struct detect_network {
	uint8_t			*input_ptr;
	network_status	status;
	vsi_nn_graph_t	*graph;
}det_network_t, *p_det_network_t;

typedef vsi_nn_graph_t * (*network_create)(const char * data_file_name, vsi_nn_context_t in_ctx);
typedef void (*network_release)(vsi_nn_graph_t * graph, vsi_bool release_ctx);

dev_type g_dev_type = DEV_REVA;
det_network_t network[DET_BUTT]={0};

network_create model_create[DET_BUTT][DEV_BUTT] = {
	{vnn_CreateYoloFace7d, vnn_CreateYoloFace88},
	{vnn_CreateYolov27d, vnn_CreateYolov288},
	{vnn_CreateYolov37d, vnn_CreateYolov388},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{vnn_CreateFacenet7d,vnn_CreateFacenet88},
};
network_release model_release[DET_BUTT][DEV_BUTT] = {
	{vnn_ReleaseYoloFace7d, vnn_ReleaseYoloFace88},
	{vnn_ReleaseYolov27d, vnn_ReleaseYolov288},
	{vnn_ReleaseYolov37d, vnn_ReleaseYolov388},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{vnn_ReleaseFacenet7d,vnn_ReleaseFacenet88},
};
const char * data_file_name[DET_BUTT][DEV_BUTT]= {
	{"/etc/nn_data/yolo_face_7D.export.data", "/etc/nn_data/yolo_face_88.export.data"},
	{"/etc/nn_data/yolov2_7D.export.data", "/etc/nn_data/yolov2_88.export.data"},
	{"/etc/nn_data/yolov3_7D.export.data", "/etc/nn_data/yolov3_88.export.data"},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{"/etc/nn_data/faceNet_7D.export.data", "/etc/nn_data/faceNet_88.export.data"},
};

typedef void (*network_preprocess)(input_image_t imageData, uint8_t **input_data_ptr);
typedef void (*network_postprocess)(vsi_nn_graph_t *graph, pDetResult resultData);

network_preprocess model_preprocess[DET_BUTT] = {
	yolo_face_preprocess,
	yolov2_preprocess,
	yolov3_preprocess,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	facenet_preprocess,
};

network_postprocess model_postprocess[DET_BUTT] = {
	yolo_face_postprocess,
	yolov2_postprocess,
	yolov3_postprocess,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	facenet_postprocess,
};

static det_status_t check_input_param(vsi_nn_graph_t*  graph, input_image_t imageData)
{
	int ret = DET_STATUS_OK;
	if (NULL == imageData.data) {
		LOGE("Data buffer is NULL");
		_SET_STATUS_(ret, DET_STATUS_PARAM_ERROR, exit);
	}

	if (imageData.pixel_format != PIX_FMT_RGB888) {
		LOGE("Current only support RGB888");
		_SET_STATUS_(ret, DET_STATUS_PARAM_ERROR, exit);
	}

	int width,height,channel;
	vsi_nn_tensor_t *tensor = vsi_nn_GetTensor( graph, graph->input.tensors[0] );

	width = tensor->attr.size[0];
	height = tensor->attr.size[1];
	channel = tensor->attr.size[2];

	if (width != imageData.width || height != imageData.height || channel != imageData.channel)
	{
		LOGE("Inputsize not match! net VS img iswidth:%dvs%d, height:%dvs%d channel:%dvs%d \n",
				width, imageData.width, height, imageData.height, channel, imageData.channel);
		_SET_STATUS_(ret, DET_STATUS_PARAM_ERROR, exit);
	}
exit:
	return ret;
}

static int find_string_index(char* buffer, char* str, int lenght)
{
	int index = 0;
	int n = strlen(str);
	int i, j;

	for (i = lenght -n -1; i >0 ; i--) {
		for (j=0; j<n; j++) {
			if (buffer[i+j] != str[j]) {
				break;
			}
		}
		if (j == n) {
			index = i;
			break;
		}
	}

	return index;
}

static void check_and_set_dev_type()
{
	#define len 1280
	int index = 0;
	char buffer[len] = {0};
	memset(buffer, 0, sizeof(buffer));

	FILE* fp = popen("cat /proc/cpuinfo", "r");
	if (!fp) {
		LOGW("Popen /proc/cupinfo fail,set default RevA");
		g_dev_type = DEV_REVA;
		return ;
	}
	int length = fread(buffer,1,len,fp);
	index = find_string_index(buffer, "290", length);

	LOGD("Read Cpuinfo:\n%s\n",buffer);
	LOGD("290 index=%d",index);
	switch (buffer[index+3]) {
		case 'a':
			LOGI("set_dev_type REVA");
			g_dev_type = DEV_REVA;
			break;
		case 'b':
			LOGI("set_dev_type REVB");
			g_dev_type = DEV_REVB;
			break;
		default:
			LOGW("set_dev_type err:%c ,and set default RevA", buffer[index+3]);
			g_dev_type = DEV_REVA;
			break;
	}
	pclose(fp);
}

det_status_t det_set_model(det_model_type modelType)
{
	LOGP("Enter, modeltype:%d", modelType);

	int ret = DET_STATUS_OK;
	vsi_status status = VSI_SUCCESS;
	p_det_network_t net = &network[modelType];
	if (net->status) {
		LOGW("Model has been inited! modeltype:%d", modelType);
		_SET_STATUS_(ret, DET_STATUS_OK, exit);
	}

	if (modelType > DET_YOLO_V3 && modelType != DET_FACENET) {
		LOGE("Model not support now!");
		_SET_STATUS_(ret, DET_STATUS_PARAM_ERROR, exit);
	}

	check_and_set_dev_type();

	LOGI("Start create Model");
	net->graph = model_create[modelType][g_dev_type](data_file_name[modelType][g_dev_type],NULL);
	if (NULL == net->graph) {
		LOGE("Create model fail! model_path=%s",data_file_name[modelType][g_dev_type]);
		_SET_STATUS_(ret, DET_STATUS_CREATE_NETWORK_FAIL, exit);
	}

	LOGP("Start vsi_nn_VerifyGraph");
	status = vsi_nn_VerifyGraph(net->graph);
	if (status) {
		LOGE("Model vxVerifyGraph fail!");
		model_release[modelType][g_dev_type](net->graph, TRUE);
		_SET_STATUS_(ret, DET_STATUS_VERIFY_NETWORK_FAIL, exit);
	}

	net->status = NETWORK_INIT;
exit:
	LOGP("Leave, modeltype:%d", modelType);
	return ret;
}

det_status_t det_get_model_size(det_model_type modelType, int *width, int *height, int *channel)
{
	LOGP("Enter, modeltype:%d", modelType);
	int ret = DET_STATUS_OK;
	p_det_network_t net = &network[modelType];
	if (!net->status) {
		LOGE("Model has not created! modeltype:%d", modelType);
		_SET_STATUS_(ret, DET_STATUS_ERROR, exit);
	}

	vsi_nn_tensor_t *tensor = vsi_nn_GetTensor(net->graph, net->graph->input.tensors[0] );
	*width = tensor->attr.size[0];
	*height = tensor->attr.size[1];
	*channel = tensor->attr.size[2];
exit:
	LOGP("Leave, modeltype:%d", modelType);
	return ret;
}

det_status_t det_set_input(input_image_t imageData, det_model_type modelType)
{
	LOGP("Enter, modeltype:%d", modelType);
	int ret = DET_STATUS_OK;
	p_det_network_t net = &network[modelType];
	if (!net->status) {
		LOGE("Model has not created! modeltype:%d", modelType);
		_SET_STATUS_(ret, DET_STATUS_ERROR, exit);
	}

	ret = check_input_param(net->graph, imageData);
	if (ret) {
		LOGE("Check_input_param fail.");
		_SET_STATUS_(ret, DET_STATUS_PARAM_ERROR, exit);
	}

	vsi_status status = VSI_FAILURE;
	vsi_nn_graph_t *graph = net->graph;
	vsi_nn_tensor_t *tensor = vsi_nn_GetTensor(graph, graph->input.tensors[0]);

	//preprocess input data
	model_preprocess[modelType](imageData, &net->input_ptr);
	//copy input data to npu tensor
	status = vsi_nn_CopyDataToTensor(graph, tensor, net->input_ptr);
	if (status) {
		LOGE("Vsi_nn_CopyDataToTensor fail");
		_SET_STATUS_(ret, DET_STATUS_ERROR, exit);
	}
	net->status = NETWORK_PREPARING;

exit:
	LOGP("Leave, modeltype:%d", modelType);
	return ret;
}

det_status_t det_get_result(pDetResult resultData, det_model_type modelType)
{
	LOGP("Enter, modeltype:%d", modelType);
	int ret = DET_STATUS_OK;
	p_det_network_t net = &network[modelType];
	if (NETWORK_PREPARING != net->status) {
		LOGE("Model not create or not prepared! status=%d", net->status);
		_SET_STATUS_(ret, DET_STATUS_ERROR, exit);
	}

	vsi_status status = VSI_FAILURE;
	vsi_nn_graph_t *graph = net->graph;

	LOGP("Start Vsi_nn_RunGraph");
	//net->status = NETWORK_PROCESSING;
	status = vsi_nn_RunGraph(graph);
	net->status = NETWORK_INIT;
	if (status) {
		LOGE("Vsi_nn_RunGraph run fail");
		_SET_STATUS_(ret, DET_STATUS_PROCESS_NETWORK_FAIL, exit);
	}

	LOGP("Start Postprocess");
	model_postprocess[modelType](graph, resultData);

exit:
	LOGP("Leave, modeltype:%d", modelType);
	return ret;
}

det_status_t det_release_model(det_model_type modelType)
{
	LOGP("Enter, modeltype:%d", modelType);
	int ret = DET_STATUS_OK;
	p_det_network_t net = &network[modelType];
	if (!net->status) {
		LOGW("Model has benn released!");
		_SET_STATUS_(ret, DET_STATUS_OK, exit);
	}

	model_release[modelType][g_dev_type](net->graph, TRUE);
	if (net->input_ptr) {
		free(net->input_ptr);
		net->input_ptr = NULL;
	}

	net->status = NETWORK_UNINIT;
	net->graph = NULL;

exit:
	LOGP("Leave, modeltype:%d", modelType);
	return ret;
}

det_status_t det_set_log_config(det_debug_level_t level,det_log_format_t output_format)
{
	LOGP("Enter, level:%d", level);
	det_set_log_level(level, output_format);
	LOGP("Leave, level:%d", level);
	return 0;
}
