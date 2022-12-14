/****************************************************************************
*   Generated by ACUITY 3.13.0
*   Match ovxlib 1.0.10
*
*   Neural Network appliction network definition header file
****************************************************************************/

#ifndef _VSI_NN_FACENET7D_H
#define _VSI_NN_FACENET7D_H

#include "vsi_nn_pub.h"

void vnn_ReleaseFacenet7d
    (
    vsi_nn_graph_t * graph,
    vsi_bool release_ctx
    );

vsi_nn_graph_t * vnn_CreateFacenet7d
    (
    const char * data_file_name,
    vsi_nn_context_t in_ctx
    );

#endif
