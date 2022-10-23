#include "yoloface.h"

#include "vnn_yoloface.h"
#include "yoloface_process.h"

vsi_nn_graph_t * g_graph = NULL;

det_status_t model_create(const char * data_file_path, dev_type type)
{
	det_status_t ret = DET_STATUS_ERROR;
	vsi_status status = VSI_SUCCESS;
	char model_path[48];

	switch (type) {
		case DEV_REVA:
			sprintf(model_path, "%s%s", data_file_path, "/yolo_face_7d.nb");
			break;
		case DEV_REVB:
			sprintf(model_path, "%s%s", data_file_path, "/yolo_face_88.nb");
			break;
		case DEV_MS1:
			sprintf(model_path, "%s%s", data_file_path, "/yolo_face_99.nb");
			break;
		case DEV_A1:
			sprintf(model_path, "%s%s", data_file_path, "/yolo_face_a1.nb");
			break;
		default:
			break;
	}
	g_graph = vnn_CreateYoloFace(model_path, NULL);
	_CHECK_PTR(g_graph, exit);

	status = vsi_nn_VerifyGraph(g_graph);
	_CHECK_STATUS(status, exit);

	ret = DET_STATUS_OK;
exit:
	return ret;
}

void model_getsize(int *width, int *height, int *channel)
{
	if (g_graph) {
		vsi_nn_tensor_t *tensor = NULL;
		tensor = vsi_nn_GetTensor(g_graph, g_graph->input.tensors[0] );

		*width = tensor->attr.size[0];
		*height = tensor->attr.size[1];
		*channel = tensor->attr.size[2];
	}
}

void model_setinput(input_image_t imageData, uint8_t* data)
{
	yolo_face_preprocess(imageData, data);
}

det_status_t model_getresult(pDetResult resultData, uint8_t* data)
{
	vsi_status status = VSI_FAILURE;
	det_status_t ret = DET_STATUS_ERROR;
	vsi_nn_tensor_t *tensor = vsi_nn_GetTensor(g_graph, g_graph->input.tensors[0]);

	status = vsi_nn_CopyDataToTensor(g_graph, tensor, data);
	_CHECK_STATUS(status, exit);

	status = vsi_nn_RunGraph(g_graph);
	_CHECK_STATUS(status, exit);

	yolo_face_postprocess(g_graph, resultData);

	ret = DET_STATUS_OK;
exit:
	return ret;
}

void model_release(dev_type type)
{
	vnn_ReleaseYoloFace(g_graph, TRUE);
	g_graph = NULL;
}