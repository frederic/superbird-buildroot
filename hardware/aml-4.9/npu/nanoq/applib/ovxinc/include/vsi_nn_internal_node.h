/****************************************************************************
*
*    Copyright (c) 2018 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/
#ifndef _VSI_NN_INTRNAL_NODE_H
#define _VSI_NN_INTRNAL_NODE_H

#include "vsi_nn_platform.h"
#include "vsi_nn_context.h"
#include "vsi_nn_tensor.h"
#include "vsi_nn_node.h"
#include "vsi_nn_ops.h"
#include "vsi_nn_types.h"
#include "vsi_nn_rnn.h"
#include "utils/vsi_nn_map.h"
#include "utils/vsi_nn_link_list.h"

/**********************************************************
* MACROS
**********************************************************/
#define INTERNAL_NODE_DEBUG FALSE

/**********************************************************
* TYPES
**********************************************************/
typedef struct _vsi_nn_internal_node_param_t
{
    vsi_nn_link_list_t link_list;
    uint8_t param[1];
} vsi_nn_internal_node_param_t;

typedef struct _vsi_nn_internal_node_t
{
    vsi_nn_link_list_t link_list;

    vsi_nn_node_t* node;
    vsi_nn_tensor_t** inputs;
    vsi_nn_tensor_t** outputs;
    vsi_nn_internal_node_param_t* param;

    #if( INTERNAL_NODE_DEBUG )
    char name[32];
    #endif
} vsi_nn_internal_node_t;

typedef struct _vsi_nn_internal_tensor_t
{
    vsi_nn_link_list_t link_list;

    vsi_nn_tensor_t* t;

    #if( INTERNAL_NODE_DEBUG )
    char name[32];
    #endif
} vsi_nn_internal_tensor_t;

typedef struct _vsi_nn_internal_node_wksp_t
{
    vsi_nn_internal_node_t* nodes;
    vsi_nn_internal_tensor_t* tensors;
    int curr_node_uid;
} vsi_nn_internal_node_wksp_t;

/**********************************************************
* PUBLIC FUNCTIONS
**********************************************************/
vsi_status vsi_nn_init_internal_node_wksp
    (
    vsi_nn_node_t* node
    );

vsi_status vsi_nn_deinit_internal_node_wksp
    (
    vsi_nn_node_t* node
    );

vsi_nn_internal_node_t* vsi_nn_new_internal_node
    (
    vsi_nn_node_t* node,
    vsi_nn_op_t op,
    uint32_t input_num,
    uint32_t output_num
    );

void* vsi_nn_new_internal_node_param
    (
    vsi_nn_internal_node_t* inode,
    size_t size /* in bytes */
    );

vsi_nn_internal_tensor_t* vsi_nn_new_internal_tensor
    (
    vsi_nn_node_t*          node,
    vsi_nn_tensor_attr_t*   attr,
    float                   default_value
    );

vsi_bool vsi_nn_setup_internal_node_op
    (
    vsi_nn_node_t* node,
    vsi_nn_internal_node_t* inode
    );

vsi_status vsi_nn_compute_internal_node
    (
    vsi_nn_node_t * node
    );

vsi_status vsi_nn_optimize_internal_node
    (
    vsi_nn_node_t * node,
    vsi_nn_opt_direction_e direction
    );

vsi_status vsi_nn_deinit_internal_node
    (
    vsi_nn_node_t * node
    );

vsi_status vsi_nn_release_internal_node
    (
    vsi_nn_internal_node_t** node
    );

vsi_status vsi_nn_release_internal_tensor
    (
    vsi_nn_internal_tensor_t** tensor
    );

vsi_nn_internal_tensor_t* vsi_nn_create_zero_bias_tensor
    (
    vsi_nn_node_t * node,
    vsi_nn_tensor_attr_t* input_attr,
    vsi_nn_tensor_attr_t* weight_attr
    );

#endif /* _VSI_NN_INTRNAL_NODE_H */
