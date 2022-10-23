/****************************************************************************
*   amlogic nn api model header file
*
*   define nn model struct and additional api
*
*   AUTHOR: zxw
*
*   Date: 2019.9
****************************************************************************/

#ifndef _VSI_NN_MODEL_H
#define _VSI_NN_MODEL_H

#include "nn_api.h"

#define VNN_APP_DEBUG (FALSE)
#define VNN_VERSION_MAJOR 1
#define VNN_VERSION_MINOR 0
#define VNN_VERSION_PATCH 12
#define VNN_RUNTIME_VERSION \
    (VNN_VERSION_MAJOR * 10000 + VNN_VERSION_MINOR * 100 + VNN_VERSION_PATCH)

#define INPUT_CNANNEL 3

typedef struct
{
	float mean[INPUT_CNANNEL];
	float scale;
	float zeropoint;
	char  name[MAX_NAME_LEGTH];
}nn_input_info_list;

typedef struct __extra_model_info
{
	unsigned int network;
	unsigned int qntype;
	unsigned int formattype;
	nn_input_info_list input_list[INPUT_MAX_NUM];
}input_extra_info;

typedef struct __nn_api_context_
{
	void  *graph;
	nn_output        output;
	input_extra_info input_info;
	assign_user_address_t user_addr;
}nn_context;

int set_preprocess(void* context,nn_input *pInput);
#endif
