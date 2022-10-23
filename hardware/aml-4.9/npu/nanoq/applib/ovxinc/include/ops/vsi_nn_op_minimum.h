/****************************************************************************
*
*    Copyright (c) 2019 Vivante Corporation
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
#ifndef _VSI_NN_OP_MINIMUM_H
#define _VSI_NN_OP_MINIMUM_H

#include "vsi_nn_types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    TENSOR_MIN_CPU_KERNEL,

    TENSOR_MIN_F16F16TOF16_KERNEL,
    TENSOR_MIN_I8F16TOI8_KERNEL,
    TENSOR_MIN_I8F16TOF16_KERNEL,
    TENSOR_MIN_U8F16TOU8_KERNEL,
    TENSOR_MIN_U8F16TOF16_KERNEL,
    TENSOR_MIN_I8I8TOI8_KERNEL,
    TENSOR_MIN_U8TOU8_KERNEL,
    TENSOR_MIN_I16I16TOI16_KERNEL,
    TENSOR_MIN_I16F16TOI16_KERNEL,
    TENSOR_MIN_I16F16TOF16_KERNEL,
    TENSOR_MIN_F16F16TOU8_KERNEL,

    TENSOR_MIN_F16F16TOF16_2D_KERNEL,
    TENSOR_MIN_I8F16TOI8_2D_KERNEL,
    TENSOR_MIN_I8F16TOF16_2D_KERNEL,
    TENSOR_MIN_U8F16TOU8_2D_KERNEL,
    TENSOR_MIN_U8F16TOF16_2D_KERNEL,
    TENSOR_MIN_I8I8TOI8_2D_KERNEL,
    TENSOR_MIN_U8TOU8_2D_KERNEL,
    TENSOR_MIN_I16I16TOI16_2D_KERNEL,
    TENSOR_MIN_I16F16TOI16_2D_KERNEL,
    TENSOR_MIN_I16F16TOF16_2D_KERNEL,
    TENSOR_MIN_F16F16TOU8_2D_KERNEL,

    TENSOR_MIN_KERNEL_COUNTS,
};

#define _VSI_NN_MINIMUM_LOCAL_TENSOR_NUM 3

typedef struct _vsi_nn_minimum_lcl_data
{
    vx_tensor   local_tensor[_VSI_NN_MINIMUM_LOCAL_TENSOR_NUM];
} vsi_nn_minimum_lcl_data;

typedef struct _vsi_nn_minimum_param
{
    /* local data must be the first. */
    vsi_nn_minimum_lcl_data local;
} vsi_nn_minimum_param;

#ifdef __cplusplus
}
#endif

#endif

