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
#pragma once
#ifndef _OPENVX_EXT_LIBNNEXT_H_
#define _OPENVX_EXT_LIBNNEXT_H_
#include <VX/vx.h>
#include <VX/vx_types.h>

#define gcoMATH_MIN(X, Y) (((X) < (Y))?(X):(Y))
#define gcoMATH_MAX(X, Y) (((X) > (Y))?(X):(Y))
#define DIM_SIZE 4

#ifdef __cplusplus
extern "C" {
#endif

/*! if there are more than 1 kernel in solution
the KERNEL_ENUM_LIBNNEXT_OFFSET must be modified keep different for any kernel
*/
enum vx_kernel_libnnext_offset_e
{
    KERNEL_ENUM_LIBNNEXT_OFFSET = 1,
    KERNEL_ENUM_PREMUTE_OFFSET,
    KERNEL_ENUM_PRIORBOX_OFFSET = 2 + KERNEL_ENUM_PREMUTE_OFFSET,
    KERNEL_ENUM_FLATTEN_OFFSET,
    KERNEL_ENUM_L2NORMALIZESCALE_OFFSET,
    KERNEL_ENUM_PARAMETRICRELU_OFFSET,
    KERNEL_ENUM_PREBBOX_OFFSET = 3 + KERNEL_ENUM_PARAMETRICRELU_OFFSET,
    KERNEL_ENUM_ADD_RELU_KERNEL_OFFSET,
    KERNEL_ENUM_POOLING_WITH_ARGMAX_OFFSET,
    KERNEL_ENUM_UNPOOLING_OFFSET = 2 + KERNEL_ENUM_POOLING_WITH_ARGMAX_OFFSET,
    KERNEL_ENUM_ARGMAX_OFFSET = 2 + KERNEL_ENUM_UNPOOLING_OFFSET,
    KERNEL_ENUM_ALEXNET_GEMM_OFFSET = 2 + KERNEL_ENUM_ARGMAX_OFFSET,
    KERNEL_ENUM_IMG2COL_DILATED_OFFSET,
    KERNEL_ENUM_IMG2COL_DILATED_INT8_OFFSET,
    KERNEL_ENUM_ALEXNET_GEMM_INT8_OFFSET,
    KERNEL_ENUM_ELTWISE_MAX,
    KERNEL_ENUM_FULLYCONNECTED_AXIS2,
    KERNEL_ENUM_TENSORCROP_INT16,
    KERNEL_ENUM_TENSORCROP_INT8,
    KERNEL_ENUM_TENSORCROP_INT16_FP16,
    KERNEL_ENUM_DROPOUT,
    KERNEL_ENUM_SHUFFLECHANNEL,
    KERNEL_ENUM_RESIZE,
    KERNEL_ENUM_REVERSE,
    KERNEL_ENUM_RESIZE_16BITS_DOWNSAMPLE_QUARTER,
    KERNEL_ENUM_RESIZE_8BITS_DOWNSAMPLE_QUARTER,
    KERNEL_ENUM_SCALE,
    KERNEL_ENUM_TENSORREVERSE,
    KERNEL_ENUM_TENSORELU_OFFSET,
    KERNEL_ENUM_SPACE2BATCH,
    KERNEL_ENUM_BATCH2SPACE,
    KERNEL_ENUM_SPACE2DEPTH,
    KERNEL_ENUM_IMAGEPROCESS,
    KERNEL_ENUM_SCALETOTENSOR,
    KERNEL_ENUM_GEMM,
    KERNEL_ENUM_LAYERNORM,
    KERNEL_ENUM_LAYERNORMFP16TOU8_OFFSET,
    KERNEL_ENUM_REDUCE,
    KERNEL_ENUM_INSTANCENORM,
    KERNEL_ENUM_TENSORSTACKCONCAT,
    KERNEL_ENUM_TENSORSTACKCONCAT8BITS_OFFSET,
    KERNEL_ENUM_SIGNALFRAME,
    KERNEL_ENUM_RELATIONALOPS,
    KERNEL_ENUM_SYNC_HOST,
    KERNEL_ENUM_POW,
    KERNEL_ENUM_FLOORDIV,
    KERNEL_ENUM_MINIMUM,
    KERNEL_ENUM_SPATIAL_TRANSFORMER,
    KERNEL_ENUM_LOGICAL_OPS,
    KERNEL_ENUM_SELECT,
    KERNEL_ENUM_LSTMUNIT_ACTIVATION,
    KERNEL_ENUM_TENSOR_ADD_MEAN_STDDEV_NORM,
    KERNEL_ENUM_STACK,
    KERNEL_ENUM_GRAYSCALETOTENSOR,
    KERNEL_ENUM_NEG,
    KERNEL_ENUM_EXP,
    KERNEL_ENUM_CLIP,
    KERNEL_ENUM_PRE_PROCESS_GRAY,
    KERNEL_ENUM_UNSTACK,
    KERNEL_ENUM_PRE_PROCESS_RGB,
    KERNEL_ENUM_ADDN,
    KERNEL_ENUM_PRE_PROCESS_YUV420,
    KERNEL_ENUM_CONV2D,
    KERNEL_ENUM_EXTRA_ENDING,
    KERNEL_ENUM_GATHER,
    KERNEL_ENUM_TILE,
    KERNEL_ENUM_TOPK,
    KERNEL_ENUM_PRE_PROCESS_BGRA,
    KERNEL_ENUM_LOGICAL_NOT,
    KERNEL_ENUM_SIN,
    KERNEL_ENUM_LOG,
    KERNEL_ENUM_ARGMIN,
    KERNEL_ENUM_ROI_ALIGN,
    KERNEL_ENUM_HEATMAP_MAX_KEYPOINT,
    KERNEL_ENUM_AXIS_ALIGNED_BBOX_TRANSFORM,
    KERNEL_ENUM_BOX_WITH_NMS_LIMIT,
    KERNEL_ENUM_GENERATE_PROPOSALS,
    KERNEL_ENUM_DETECTION_POSTPROCESS,
    KERNEL_ENUM_RANDOM_MULTINOMIAL,
    KERNEL_ENUM_LOG_SOFTMAX,
    KERNEL_ENUM_RELU_KERAS_INTERNAL,
    KERNEL_ENUM_DECONV2D,
    KERNEL_ENUM_REDUCEMAX_INTERNAL,
    KERNEL_ENUM_REDUCEMIN_INTERNAL,
    KERNEL_ENUM_REDUCEPROD_INTERNAL,
    KERNEL_ENUM_REDUCEALL_INTERNAL,
    KERNEL_ENUM_REDUCEANY_INTERNAL,
};

//! [KERNEL NAME]
#define VX_KERNEL_NAME_PERMUTECWH                          "com.vivantecorp.extension.vxcPermuteCWH"
#define VX_KERNEL_NAME_PERMUTECHW                          "com.vivantecorp.extension.vxcPermuteCWH"
#define VX_KERNEL_NAME_PRIORBOX                            "com.vivantecorp.extension.vxcPriorBox"
#define VX_KERNEL_NAME_FLATTEN                             "com.vivantecorp.extension.flatten"
#define VX_KERNEL_NAME_L2NORMALIZESCALE                    "com.vivantecorp.extension.vxcL2NormalizeScale"
#define VX_KERNEL_NAME_L2NORMSCALE_SUMRSQRT                "com.vivantecorp.extension.vxcL2NormScale_SumRsqrt"
#define VX_KERNEL_NAME_L2NORMSCALE_SUMRSQRT_INT8           "com.vivantecorp.extension.vxcL2NormScale_SumRsqrt_int8"
#define VX_KERNEL_NAME_L2NORMSCALE_SUMRSQRT_UINT8          "com.vivantecorp.extension.vxcL2NormScale_SumRsqrt_uint8"
#define VX_KERNEL_NAME_L2NORMSCALE_SUMRSQRT_INT16          "com.vivantecorp.extension.vxcL2NormScale_SumRsqrt_int16"
#define VX_KERNEL_NAME_L2NORMSCALE_FP16TOFP16              "com.vivantecorp.extension.vxcL2NormScale_Fp16toFp16"
#define VX_KERNEL_NAME_L2NORMSCALE_INT8TOINT8              "com.vivantecorp.extension.vxcL2NormScale_Int8toInt8"
#define VX_KERNEL_NAME_L2NORMSCALE_INT8TOFP16              "com.vivantecorp.extension.vxcL2NormScale_Int8toFp16"
#define VX_KERNEL_NAME_L2NORMSCALE_UINT8TOUINT8            "com.vivantecorp.extension.vxcL2NormScale_UInt8toUInt8"
#define VX_KERNEL_NAME_L2NORMSCALE_UINT8TOFP16             "com.vivantecorp.extension.vxcL2NormScale_UInt8toFp16"
#define VX_KERNEL_NAME_L2NORMSCALE_INT16TOINT16            "com.vivantecorp.extension.vxcL2NormScale_Int16toInt16"
#define VX_KERNEL_NAME_L2NORMSCALE_INT16TOFP16             "com.vivantecorp.extension.vxcL2NormScale_Int16toFp16"
#define VX_KERNEL_NAME_PARAMETRICRELU                      "com.vivantecorp.extension.vxcParametricRelu"
#define VX_KERNEL_NAME_PARAMETRICRELU_INT8                 "com.vivantecorp.extension.vxcParametricRelu_int8"
#define VX_KERNEL_NAME_PARAMETRICRELU_INT8_OPT             "com.vivantecorp.extension.vxcParametricRelu_int8_opt"
#define VX_KERNEL_NAME_PARAMETRICRELU_INT8_OPT1            "com.vivantecorp.extension.vxcParametricRelu_int8_opt1"
#define VX_KERNEL_NAME_PARAMETRICRELU_INT8_FP16            "com.vivantecorp.extension.vxcParametricRelu_int8_fp16"
#define VX_KERNEL_NAME_PARAMETRICRELU_INT16_INT16          "com.vivantecorp.extension.vxcParametricReluInt16_Int16"
#define VX_KERNEL_NAME_PARAMETRICRELU_INT16_INT16_OPT      \
                                "com.vivantecorp.extension.vxcParametricReluInt16_Int16_opt"
#define VX_KERNEL_NAME_PARAMETRICRELU_INT16_INT16_OPT1     \
                                "com.vivantecorp.extension.vxcParametricReluInt16_Int16_opt1"
#define VX_KERNEL_NAME_PARAMETRICRELU_UINT8_UINT8          "com.vivantecorp.extension.vxcParametricReluUint8_Uint8"
#define VX_KERNEL_NAME_PARAMETRICRELU_UINT8_2D             "com.vivantecorp.extension.vxcParametricRelu_uint8_2d"
#define VX_KERNEL_NAME_PARAMETRICRELU_FP16_UINT8           "com.vivantecorp.extension.vxcParametricReluFp16_Uint8"
#define VX_KERNEL_NAME_PARAMETRICRELU_FP16_INT16           "com.vivantecorp.extension.vxcParametricReluFp16_Int16"
#define VX_KERNEL_NAME_PARAMETRICRELU_INT16_FP16           "com.vivantecorp.extension.vxcParametricReluInt16_Fp16"
#define VX_KERNEL_NAME_PARAMETRICRELU_UINT8_FP16           "com.vivantecorp.extension.vxcParametricReluUint8_Fp16"
#define VX_KERNEL_NAME_PARAMETRICRELU_UINT8_FP16_2D        \
                                "com.vivantecorp.extension.vxcParametricRelu_uint8tofp16_2d"
#define VX_KERNEL_NAME_PARAMETRICRELU_FP16_INT8            "com.vivantecorp.extension.vxcParametricReluFp16_Int8"
#define VX_KERNEL_NAME_PREBBOX                             "com.vivantecorp.extension.preBBoxVXC"
#define VX_KERNEL_NAME_ADD_RELU_KERNEL                     "com.vivantecorp.extension.addRelu"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX                 "com.vivantecorp.extension.poolingWithArgmax"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT8            "com.vivantecorp.extension.poolingWithArgmaxInt8"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT8_OPT        "com.vivantecorp.extension.poolingWithArgmaxInt8_Int8_opt"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT8_INT8       "com.vivantecorp.extension.poolingWithArgmaxInt8_Int8"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT16           "com.vivantecorp.extension.poolingWithArgmaxInt16_s2k2p0"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT16_INT16     \
                                "com.vivantecorp.extension.poolingWithArgmaxInt16_int16_s2k2p0"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT16_OPT       \
                                "com.vivantecorp.extension.poolingWithArgmaxInt16_s2k2p0_opt"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT16_FP16      \
                                "com.vivantecorp.extension.poolingWithArgmaxInt16_fp16_s2k2p0"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT16_AXINT16   \
                                "com.vivantecorp.extension.poolingWithArgmaxInt16_axI16_s2k2p0"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_UINT8           "com.vivantecorp.extension.poolingWithArgmaxUint8_s2k2p0"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_UINT8_FP16      \
                                "com.vivantecorp.extension.poolingWithArgmaxUint8_fp16_s2k2p0"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_UINT8_FP16_FP16 \
                                "com.vivantecorp.extension.poolingWithArgmaxUint8_fp16_fp16_s2k2p0"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT8_FP16       \
                                "com.vivantecorp.extension.poolingWithArgmaxInt8_fp16_s2k2p0"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_UINT8_2D        "com.vivantecorp.extension.poolingWithArgmaxU8_s2k2p0_2D"
#define VX_KERNEL_NAME_UNPOOLING                           "com.vivantecorp.extension.unpooling"
#define VX_KERNEL_NAME_UNPOOLING_INT8                      "com.vivantecorp.extension.unpoolingInt8"
#define VX_KERNEL_NAME_UNPOOLING_INT8_INT8                 "com.vivantecorp.extension.unpoolingInt8_Int8"
#define VX_KERNEL_NAME_UNPOOLING_INT8_INT8_OPT             "com.vivantecorp.extension.unpoolingInt8_Int8_opt"
#define VX_KERNEL_NAME_UNPOOLING_UINT8                     "com.vivantecorp.extension.unpoolingUint8_Uint8"
#define VX_KERNEL_NAME_UNPOOLING_INT16_INT16               "com.vivantecorp.extension.unpoolingInt16_Int16"
#define VX_KERNEL_NAME_UNPOOLING_INT16_INT16_OPT           "com.vivantecorp.extension.unpoolingInt16_Int16_opt"
#define VX_KERNEL_NAME_UNPOOLING_INT16_INT16_AXINT16       "com.vivantecorp.extension.unpoolingInt16_Int16_axI16"
#define VX_KERNEL_NAME_UNPOOLING_INT16_FP16_AXINT16        "com.vivantecorp.extension.unpoolingInt16_Fp16_axI16"
#define VX_KERNEL_NAME_UNPOOLING_INT16_FP16                "com.vivantecorp.extension.unpoolingInt16_Fp16"
#define VX_KERNEL_NAME_UNPOOLING_FP16_UINT8                "com.vivantecorp.extension.unpoolingFp16_Uint8"
#define VX_KERNEL_NAME_UNPOOLING_FP16_INT8                 "com.vivantecorp.extension.unpoolingFp16_Int8"
#define VX_KERNEL_NAME_UNPOOLING_FP16_INT16                "com.vivantecorp.extension.unpoolingFp16_Int16"
#define VX_KERNEL_NAME_UNPOOLING_FP16FP16_UINT8            "com.vivantecorp.extension.unpoolingFp16Fp16_Uint8"
#define VX_KERNEL_NAME_UNPOOLING_UINT8_FP16                "com.vivantecorp.extension.unpoolingUint8_Fp16"
#define VX_KERNEL_NAME_UNPOOLING_INT8_FP16                 "com.vivantecorp.extension.unpoolingInt8_Fp16"
#define VX_KERNEL_NAME_UNPOOLING_UINT8_2D                  "com.vivantecorp.extension.unpoolingUint8_Uint8_2D"
#define VX_KERNEL_NAME_UNPOOLING_FP16_UINT8_2D             "com.vivantecorp.extension.unpoolingFp16_Uint8_2D"
#define VX_KERNEL_NAME_ALEXNET_GEMM                        "com.vivantecorp.extension.alexNet_gemmVXC"
#define VX_KERNEL_NAME_IMG2COL_DILATED                     "com.vivantecorp.extension.img2col_dilatedVXC"
#define VX_KERNEL_NAME_IMG2COL_DILATED_INT8                "com.vivantecorp.extension.img2col_dilated_int8VXC"
#define VX_KERNEL_NAME_ALEXNET_GEMM_INT8                   "com.vivantecorp.extension.alexNet_gemm_int8VXC"
#define VX_KERNEL_NAME_FULLYCONNECTED_AXIS2                "com.vivantecorp.extension.vxcFullyConnected_Axis2"
#define VX_KERNEL_NAME_TENSORCROP_INT16                    "com.vivantecorp.extension.vxcTensorCrop_Int16"
#define VX_KERNEL_NAME_TENSORCROP_INT8                     "com.vivantecorp.extension.vxcTensorCrop_Int8"
#define VX_KERNEL_NAME_TENSORCROP_INT16_FP16               "com.vivantecorp.extension.vxcTensorCrop_Int16_Fp16"
#define VX_KERNEL_NAME_DROPOUT                             "com.vivantecorp.extension.dropoutVXC"
#define VX_KERNEL_NAME_SHUFFLECHANNEL                      "com.vivantecorp.extension.shuffleChannelVXC"
#define VX_KERNEL_NAME_SHUFFLECHANNEL8BITS                 "com.vivantecorp.extension.shuffleChannel8BitsVXC"
#define VX_KERNEL_NAME_RESIZE_16BITS_DOWNSAMPLE_QUARTER    \
                                "com.vivantecorp.extension.resize_16bits_downsample_quarter"
#define VX_KERNEL_NAME_RESIZE_8BITS_DOWNSAMPLE_QUARTER     \
                                "com.vivantecorp.extension.resize_8bits_downsample_quarter"
#define VX_KERNEL_NAME_SCALE_FP16                          "com.vivantecorp.extension.scale_fp16"
#define VX_KERNEL_NAME_TENSORREVERSE                       "com.vivantecorp.extension.tensorReverse_axis0_fp16"
#define VX_KERNEL_NAME_TENSOR_ELU_F16TOF16                 "com.vivantecorp.extension.vxTensorElu_F16toF16"
#define VX_KERNEL_NAME_TENSOR_ELU_F16TOF16_2D              "com.vivantecorp.extension.vxTensorElu_F16toF16_2D"
#define VX_KERNEL_NAME_TENSOR_ELU_F16TOI8                  "com.vivantecorp.extension.vxTensorElu_F16toI8"
#define VX_KERNEL_NAME_TENSOR_ELU_F16TOI8_2D               "com.vivantecorp.extension.vxTensorElu_F16toI8_2D"
#define VX_KERNEL_NAME_TENSOR_ELU_F16TOI16                 "com.vivantecorp.extension.vxTensorElu_F16toI16"
#define VX_KERNEL_NAME_TENSOR_ELU_F16TOI16_2D              "com.vivantecorp.extension.vxTensorElu_F16toI16_2D"
#define VX_KERNEL_NAME_TENSOR_ELU_F16TOU8                  "com.vivantecorp.extension.vxTensorElu_F16toU8"
#define VX_KERNEL_NAME_TENSOR_ELU_F16TOU8_2D               "com.vivantecorp.extension.vxTensorElu_F16toU8_2D"
#define VX_KERNEL_NAME_TENSOR_ELU_I8TOI8                   "com.vivantecorp.extension.vxTensorElu_I8toI8"
#define VX_KERNEL_NAME_TENSOR_ELU_I8TOI8_2D                "com.vivantecorp.extension.vxTensorElu_I8toI8_2D"
#define VX_KERNEL_NAME_TENSOR_ELU_I8TOF16                  "com.vivantecorp.extension.vxTensorElu_I8toF16"
#define VX_KERNEL_NAME_TENSOR_ELU_I8TOF16_2D               "com.vivantecorp.extension.vxTensorElu_I8toF16_2D"
#define VX_KERNEL_NAME_TENSOR_ELU_I16TOI16                 "com.vivantecorp.extension.vxTensorElu_I16toI16"
#define VX_KERNEL_NAME_TENSOR_ELU_I16TOI16_2D              "com.vivantecorp.extension.vxTensorElu_I16toI16_2D"
#define VX_KERNEL_NAME_TENSOR_ELU_I16TOF16                 "com.vivantecorp.extension.vxTensorElu_I16toF16"
#define VX_KERNEL_NAME_TENSOR_ELU_I16TOF16_2D              "com.vivantecorp.extension.vxTensorElu_I16toF16_2D"
#define VX_KERNEL_NAME_TENSOR_ELU_U8TOU8                   "com.vivantecorp.extension.vxTensorElu_U8toU8"
#define VX_KERNEL_NAME_TENSOR_ELU_U8TOU8_2D                "com.vivantecorp.extension.vxTensorElu_U8toU8_2D"
#define VX_KERNEL_NAME_TENSOR_ELU_U8TOF16                  "com.vivantecorp.extension.vxTensorElu_U8toF16"
#define VX_KERNEL_NAME_TENSOR_ELU_U8TOF16_2D               "com.vivantecorp.extension.vxTensorElu_U8toF16_2D"
#define VX_KERNEL_NAME_SPACE2DEPTH_INT16_INT16             "com.vivantecorp.extension.vxcReorg2_fp16_fp16_sx2_sy1"
#define VX_KERNEL_NAME_SCALETOTENSOR_FP16                  "com.vivantecorp.extension.ScaletoTensor_Fp16"
#define VX_KERNEL_NAME_SCALETOTENSOR_INT8                  "com.vivantecorp.extension.ScaletoTensor_Int8"
#define VX_KERNEL_NAME_SCALETOTENSOR_FP16_COPY             "com.vivantecorp.extension.ScaletoTensor_Fp16_copy"
#define VX_KERNEL_NAME_SCALETOTENSOR_INT8_COPY             "com.vivantecorp.extension.ScaletoTensor_Int8_copy"
#define VX_KERNEL_NAME_SCALETOTENSOR_INT16                 "com.vivantecorp.extension.ScaletoTensor_Int16"
#define VX_KERNEL_NAME_SCALETOTENSOR_INT16_COPY            "com.vivantecorp.extension.ScaletoTensor_Int16_copy"
#define VX_KERNEL_NAME_SCALETOTENSOR_UINT8                 "com.vivantecorp.extension.ScaletoTensor_UInt8"
#define VX_KERNEL_NAME_SCALETOTENSOR_UINT8_COPY            "com.vivantecorp.extension.ScaletoTensor_UInt8_copy"
#define VX_KERNEL_NAME_GRAYSCALETOTENSOR_FP16              "com.vivantecorp.extension.GrayScaletoTensor_Fp16"
#define VX_KERNEL_NAME_GRAYSCALETOTENSOR_INT8              "com.vivantecorp.extension.GrayScaletoTensor_Int8"
#define VX_KERNEL_NAME_GRAYSCALETOTENSOR_FP16_COPY         "com.vivantecorp.extension.GrayScaletoTensor_Fp16_copy"
#define VX_KERNEL_NAME_GRAYSCALETOTENSOR_INT8_COPY         "com.vivantecorp.extension.GrayScaletoTensor_Int8_copy"
#define VX_KERNEL_NAME_GRAYSCALETOTENSOR_INT16             "com.vivantecorp.extension.GrayScaletoTensor_Int16"
#define VX_KERNEL_NAME_GRAYSCALETOTENSOR_INT16_COPY        "com.vivantecorp.extension.GrayScaletoTensor_Int16_copy"
#define VX_KERNEL_NAME_GRAYSCALETOTENSOR_UINT8             "com.vivantecorp.extension.GrayScaletoTensor_UInt8"
#define VX_KERNEL_NAME_GRAYSCALETOTENSOR_UINT8_COPY        "com.vivantecorp.extension.GrayScaletoTensor_UInt8_copy"
#define VX_KERNEL_NAME_GEMM                                "com.vivantecorp.extension.gemm_block4x4_fp16"
#define VX_KERNEL_NAME_GEMM_UINT8                          "com.vivantecorp.extension.gemm_block4x4_u8"
#define VX_KERNEL_NAME_GEMM_FP16U8_Fp16                    "com.vivantecorp.extension.gemm_block4x4_fp16_u8_fp16"
#define VX_KERNEL_NAME_GEMM_FP16U8_U8                      "com.vivantecorp.extension.gemm_block4x4_fp16_u8_u8"
#define VX_KERNEL_NAME_GEMM_TRANSB_FP16U8TOFP16            "com.vivantecorp.extension.gemmTransBFp16U8toFp16"
#define VX_KERNEL_NAME_GEMM_TRANSB_FP16U8TOU8              "com.vivantecorp.extension.gemmTransBFp16U8toU8"
#define VX_KERNEL_NAME_GEMM_TRANSB_U8U8TOFP16              "com.vivantecorp.extension.gemmTransBU8U8toFp16"
#define VX_KERNEL_NAME_GEMM_FP16FP16_U8                    "com.vivantecorp.extension.gemm_block4x4_fp16_fp16_u8"
#define VX_KERNEL_NAME_LAYERNORM                           "com.vivantecorp.extension.vxcLayerNorm"
#define VX_KERNEL_NAME_LAYERNORM_UINT8                     "com.vivantecorp.extension.vxcLayerNorm_u8"
#define VX_KERNEL_NAME_LAYERNORM_FP16TOU8                  "com.vivantecorp.extension.vxcLayerNormFP16toU8"
#define VX_KERNEL_NAME_INSTANCENORM                        "com.vivantecorp.extension.vxcInstanceNorm"
#define VX_KERNEL_NAME_INSTANCENORM_UINT8                  "com.vivantecorp.extension.vxcInstanceNorm_U8"
#define VX_KERNEL_NAME_INSTANCENORM_INT8                   "com.vivantecorp.extension.vxcInstanceNorm_int8"
#define VX_KERNEL_NAME_INSTANCENORM_INT16                  "com.vivantecorp.extension.vxcInstanceNorm_int16"
#define VX_KERNEL_NAME_INSTANCENORM_UINT8_FP16             "com.vivantecorp.extension.vxcInstanceNormU8_fp16"
#define VX_KERNEL_NAME_INSTANCENORM_INT8_FP16              "com.vivantecorp.extension.vxcInstanceNormInt8_fp16"
#define VX_KERNEL_NAME_INSTANCENORM_INT16_FP16             "com.vivantecorp.extension.vxcInstanceNormInt16_fp16"
#define VX_KERNEL_NAME_INSTANCENORMSUM_UINT8               "com.vivantecorp.extension.vxcInstanceNormSum_u8"
#define VX_KERNEL_NAME_INSTANCENORMSQR_UINT8               "com.vivantecorp.extension.vxcInstanceNormSqr_u8"
#define VX_KERNEL_NAME_INSTANCENORMMEAN_VARI_UINT8         "com.vivantecorp.extension.vxcInstanceNormMeanVari_u8"
#define VX_KERNEL_NAME_TENSORSTACKCONCAT                   "com.vivantecorp.extension.vxcTensorStackConcat"
#define VX_KERNEL_NAME_TENSORSTACKCONCAT8BITS              "com.vivantecorp.extension.vxcTensorStackConcat8Bits"
#define VX_KERNEL_NAME_SIGNALFRAME_WIDTH                   "com.vivantecorp.extension.vxcSignalFrame_width"
#define VX_KERNEL_NAME_SIGNALFRAME_HEIGHT                  "com.vivantecorp.extension.vxcSignalFrame_height"
#define VX_KERNEL_NAME_SIGNALFRAME_CHANNEL                 "com.vivantecorp.extension.vxcSignalFrame_channel"
#define VX_KERNEL_NAME_SIGNALFRAME_WIDTH_8BITS             "com.vivantecorp.extension.vxcSignalFrame_width_8bit"
#define VX_KERNEL_NAME_SIGNALFRAME_HEIGHT_8BITS            "com.vivantecorp.extension.vxcSignalFrame_height_8bit"
#define VX_KERNEL_NAME_SIGNALFRAME_CHANNEL_8BITS           "com.vivantecorp.extension.vxcSignalFrame_channel_8bit"
#define VX_KERNEL_NAME_RELATIONAL_GREATER_INT8             "com.vivantecorp.extension.vxcTensorRelation_Gt_Int8"
#define VX_KERNEL_NAME_RELATIONAL_GREATEREQUAL_INT8        "com.vivantecorp.extension.vxcTensorRelation_Gte_Int8"
#define VX_KERNEL_NAME_RELATIONAL_LESS_INT8                "com.vivantecorp.extension.vxcTensorRelation_Ls_Int8"
#define VX_KERNEL_NAME_RELATIONAL_LESSEQUAL_INT8           "com.vivantecorp.extension.vxcTensorRelation_Lse_Int8"
#define VX_KERNEL_NAME_RELATIONAL_NOTEQUAL_INT8            "com.vivantecorp.extension.vxcTensorRelation_Ne_Int8"
#define VX_KERNEL_NAME_RELATIONAL_EQUAL_INT8               "com.vivantecorp.extension.vxcTensorRelation_E_Int8"
#define VX_KERNEL_NAME_RELATIONAL_GREATER_INT16            "com.vivantecorp.extension.vxcTensorRelation_Gt_Int16"
#define VX_KERNEL_NAME_RELATIONAL_GREATEREQUAL_INT16       "com.vivantecorp.extension.vxcTensorRelation_Gte_Int16"
#define VX_KERNEL_NAME_RELATIONAL_LESS_INT16               "com.vivantecorp.extension.vxcTensorRelation_Ls_Int16"
#define VX_KERNEL_NAME_RELATIONAL_LESSEQUAL_INT16          "com.vivantecorp.extension.vxcTensorRelation_Lse_Int16"
#define VX_KERNEL_NAME_RELATIONAL_NOTEQUAL_INT16           "com.vivantecorp.extension.vxcTensorRelation_Ne_Int16"
#define VX_KERNEL_NAME_RELATIONAL_EQUAL_INT16              "com.vivantecorp.extension.vxcTensorRelation_E_Int16"
#define VX_KERNEL_NAME_RELATIONAL_GREATER_UINT8            "com.vivantecorp.extension.vxcTensorRelation_Gt_Uint8"
#define VX_KERNEL_NAME_RELATIONAL_GREATEREQUAL_UINT8       "com.vivantecorp.extension.vxcTensorRelation_Gte_Uint8"
#define VX_KERNEL_NAME_RELATIONAL_LESS_UINT8               "com.vivantecorp.extension.vxcTensorRelation_Ls_Uint8"
#define VX_KERNEL_NAME_RELATIONAL_LESSEQUAL_UINT8          "com.vivantecorp.extension.vxcTensorRelation_Lse_Uint8"
#define VX_KERNEL_NAME_RELATIONAL_NOTEQUAL_UINT8           "com.vivantecorp.extension.vxcTensorRelation_Ne_Uint8"
#define VX_KERNEL_NAME_RELATIONAL_EQUAL_UINT8              "com.vivantecorp.extension.vxcTensorRelation_E_Uint8"
#define VX_KERNEL_NAME_RELATIONAL_GREATER_FP16             "com.vivantecorp.extension.vxcTensorRelation_Gt_Fp16"
#define VX_KERNEL_NAME_RELATIONAL_GREATEREQUAL_FP16        "com.vivantecorp.extension.vxcTensorRelation_Gte_Fp16"
#define VX_KERNEL_NAME_RELATIONAL_LESS_FP16                "com.vivantecorp.extension.vxcTensorRelation_Ls_Fp16"
#define VX_KERNEL_NAME_RELATIONAL_LESSEQUAL_FP16           "com.vivantecorp.extension.vxcTensorRelation_Lse_Fp16"
#define VX_KERNEL_NAME_RELATIONAL_NOTEQUAL_FP16            "com.vivantecorp.extension.vxcTensorRelation_Ne_Fp16"
#define VX_KERNEL_NAME_RELATIONAL_EQUAL_FP16               "com.vivantecorp.extension.vxcTensorRelation_E_Fp16"
#define VX_KERNEL_NAME_POW_FP16                            "com.vivantecorp.extension.vxcTensorPow_Fp16"
#define VX_KERNEL_NAME_POW_INT16                           "com.vivantecorp.extension.vxcTensorPow_Int16"
#define VX_KERNEL_NAME_POW_INT8                            "com.vivantecorp.extension.vxcTensorPow_Int8"
#define VX_KERNEL_NAME_POW_UINT8                           "com.vivantecorp.extension.vxcTensorPow_Uint8"
#define VX_KERNEL_NAME_POW_UINT8_FP16FP16                  "com.vivantecorp.extension.vxcTensorPow_U8Fp16_Fp16"
#define VX_KERNEL_NAME_POW_INT8_FP16FP16                   "com.vivantecorp.extension.vxcTensorPow_I8Fp16_Fp16"
#define VX_KERNEL_NAME_POW_INT16_FP16FP16                  "com.vivantecorp.extension.vxcTensorPow_I16Fp16_Fp16"
#define VX_KERNEL_NAME_FLOORDIV_FP16                       "com.vivantecorp.extension.vxcTensorFloorDiv_Fp16"
#define VX_KERNEL_NAME_FLOORDIV_INT16                      "com.vivantecorp.extension.vxcTensorFloorDiv_Int16"
#define VX_KERNEL_NAME_FLOORDIV_INT8                       "com.vivantecorp.extension.vxcTensorFloorDiv_Int8"
#define VX_KERNEL_NAME_FLOORDIV_UINT8                      "com.vivantecorp.extension.vxcTensorFloorDiv_Uint8"
#define VX_KERNEL_NAME_MINIMUM_F16F16TOF16                 "com.vivantecorp.extension.vxcTensorMinimum_F16F16toF16"
#define VX_KERNEL_NAME_MINIMUM_F16F16TOF16_2D              "com.vivantecorp.extension.vxcTensorMinimum_F16F16toF16_2D"
#define VX_KERNEL_NAME_MINIMUM_I8I8TOI8                    "com.vivantecorp.extension.vxcTensorMinimum_I8I8toI8"
#define VX_KERNEL_NAME_MINIMUM_I8I8TOI8_2D                 "com.vivantecorp.extension.vxcTensorMinimum_I8I8toI8_2D"
#define VX_KERNEL_NAME_MINIMUM_I8F16TOI8                   "com.vivantecorp.extension.vxcTensorMinimum_I8F16toI8"
#define VX_KERNEL_NAME_MINIMUM_I8F16TOI8_2D                "com.vivantecorp.extension.vxcTensorMinimum_I8F16toI8_2D"
#define VX_KERNEL_NAME_MINIMUM_I8F16TOF16                  "com.vivantecorp.extension.vxcTensorMinimum_I8F16toF16"
#define VX_KERNEL_NAME_MINIMUM_I8F16TOF16_2D               "com.vivantecorp.extension.vxcTensorMinimum_I8F16toF16_2D"
#define VX_KERNEL_NAME_MINIMUM_U8F16TOF16                  "com.vivantecorp.extension.vxcTensorMinimum_U8F16toF16"
#define VX_KERNEL_NAME_MINIMUM_U8F16TOF16_2D               "com.vivantecorp.extension.vxcTensorMinimum_U8F16toF16_2D"
#define VX_KERNEL_NAME_MINIMUM_U8F16TOU8                   "com.vivantecorp.extension.vxcTensorMinimum_U8F16toU8"
#define VX_KERNEL_NAME_MINIMUM_U8F16TOU8_2D                "com.vivantecorp.extension.vxcTensorMinimum_U8F16toU8_2D"
#define VX_KERNEL_NAME_MINIMUM_U8U8TOU8                    "com.vivantecorp.extension.vxcTensorMinimum_U8U8toU8"
#define VX_KERNEL_NAME_MINIMUM_U8U8TOU8_2D                 "com.vivantecorp.extension.vxcTensorMinimum_U8U8toU8_2D"
#define VX_KERNEL_NAME_MINIMUM_I16I16TOI16                 "com.vivantecorp.extension.vxcTensorMinimum_I16I16toI16"
#define VX_KERNEL_NAME_MINIMUM_I16I16TOI16_2D              "com.vivantecorp.extension.vxcTensorMinimum_I16I16toI16_2D"
#define VX_KERNEL_NAME_MINIMUM_I16F16TOI16                 "com.vivantecorp.extension.vxcTensorMinimum_I16F16toI16"
#define VX_KERNEL_NAME_MINIMUM_I16F16TOI16_2D              "com.vivantecorp.extension.vxcTensorMinimum_I16F16toI16_2D"
#define VX_KERNEL_NAME_MINIMUM_I16F16TOF16                 "com.vivantecorp.extension.vxcTensorMinimum_I16F16toF16"
#define VX_KERNEL_NAME_MINIMUM_I16F16TOF16_2D              "com.vivantecorp.extension.vxcTensorMinimum_I16F16toF16_2D"
#define VX_KERNEL_NAME_MINIMUM_F16F16TOU8                  "com.vivantecorp.extension.vxcTensorMinimum_F16F16toU8"
#define VX_KERNEL_NAME_MINIMUM_F16F16TOU8_2D               "com.vivantecorp.extension.vxcTensorMinimum_F16F16toU8_2D"
#define VX_KERNEL_NAME_MAXIMUM_F16F16TOF16                 "com.vivantecorp.extension.vxcTensorMaximum_F16F16toF16"
#define VX_KERNEL_NAME_MAXIMUM_F16F16TOF16_2D              "com.vivantecorp.extension.vxcTensorMaximum_F16F16toF16_2D"
#define VX_KERNEL_NAME_MAXIMUM_I8I8TOI8                    "com.vivantecorp.extension.vxcTensorMaximum_I8I8toI8"
#define VX_KERNEL_NAME_MAXIMUM_I8I8TOI8_2D                 "com.vivantecorp.extension.vxcTensorMaximum_I8I8toI8_2D"
#define VX_KERNEL_NAME_MAXIMUM_I8F16TOI8                   "com.vivantecorp.extension.vxcTensorMaximum_I8F16toI8"
#define VX_KERNEL_NAME_MAXIMUM_I8F16TOI8_2D                "com.vivantecorp.extension.vxcTensorMaximum_I8F16toI8_2D"
#define VX_KERNEL_NAME_MAXIMUM_I8F16TOF16                  "com.vivantecorp.extension.vxcTensorMaximum_I8F16toF16"
#define VX_KERNEL_NAME_MAXIMUM_I8F16TOF16_2D               "com.vivantecorp.extension.vxcTensorMaximum_I8F16toF16_2D"
#define VX_KERNEL_NAME_MAXIMUM_U8F16TOF16                  "com.vivantecorp.extension.vxcTensorMaximum_U8F16toF16"
#define VX_KERNEL_NAME_MAXIMUM_U8F16TOF16_2D               "com.vivantecorp.extension.vxcTensorMaximum_U8F16toF16_2D"
#define VX_KERNEL_NAME_MAXIMUM_U8F16TOU8                   "com.vivantecorp.extension.vxcTensorMaximum_U8F16toU8"
#define VX_KERNEL_NAME_MAXIMUM_U8F16TOU8_2D                "com.vivantecorp.extension.vxcTensorMaximum_U8F16toU8_2D"
#define VX_KERNEL_NAME_MAXIMUM_U8U8TOU8                    "com.vivantecorp.extension.vxcTensorMaximum_U8U8toU8"
#define VX_KERNEL_NAME_MAXIMUM_U8U8TOU8_2D                 "com.vivantecorp.extension.vxcTensorMaximum_U8U8toU8_2D"
#define VX_KERNEL_NAME_MAXIMUM_F16F16TOU8                  "com.vivantecorp.extension.vxcTensorMaximum_F16F16toU8"
#define VX_KERNEL_NAME_MAXIMUM_F16F16TOU8_2D               "com.vivantecorp.extension.vxcTensorMaximum_F16F16toU8_2D"
#define VX_KERNEL_NAME_MAXIMUM_I16I16TOI16                 "com.vivantecorp.extension.vxcTensorMaximum_I16I16toI16"
#define VX_KERNEL_NAME_MAXIMUM_I16I16TOI16_2D              "com.vivantecorp.extension.vxcTensorMaximum_I16I16toI16_2D"
#define VX_KERNEL_NAME_MAXIMUM_I16F16TOI16                 "com.vivantecorp.extension.vxcTensorMaximum_I16F16toI16"
#define VX_KERNEL_NAME_MAXIMUM_I16F16TOI16_2D              "com.vivantecorp.extension.vxcTensorMaximum_I16F16toI16_2D"
#define VX_KERNEL_NAME_MAXIMUM_I16F16TOF16                 "com.vivantecorp.extension.vxcTensorMaximum_I16F16toF16"
#define VX_KERNEL_NAME_MAXIMUM_I16F16TOF16_2D              "com.vivantecorp.extension.vxcTensorMaximum_I16F16toF16_2D"
#define VX_KERNEL_NAME_SPATIAL_TRANSFORMER                 "com.vivantecorp.extension.vxcTransform_Gemm_F16toF16"
#define VX_KERNEL_NAME_TRANSFORM_SETUP_THRES_F16TOF16      "com.vivantecorp.extension.vxcTransform_setupThres_F16toF16"
#define VX_KERNEL_NAME_TRANSFORM_INTERP_F16TOF16_2D        "com.vivantecorp.extension.vxcTransform_InterP_F16toF16_2D"
#define VX_KERNEL_NAME_TRANSFORM_INTERP_F16TOF16           "com.vivantecorp.extension.vxcTransform_InterP_F16toF16"
#define VX_KERNEL_NAME_LOGICAL_OR_INT16                    "com.vivantecorp.extension.vxcTensorLogical_or_int16"
#define VX_KERNEL_NAME_LOGICAL_OR_INT8                     "com.vivantecorp.extension.vxcTensorLogical_or_int8"
#define VX_KERNEL_NAME_LOGICAL_OR_UINT8                    "com.vivantecorp.extension.vxcTensorLogical_or_uint8"
#define VX_KERNEL_NAME_LOGICAL_OR_FP16                     "com.vivantecorp.extension.vxcTensorLogical_or_fp16"
#define VX_KERNEL_NAME_LOGICAL_AND_INT16                   "com.vivantecorp.extension.vxcTensorLogical_and_int16"
#define VX_KERNEL_NAME_LOGICAL_AND_INT8                    "com.vivantecorp.extension.vxcTensorLogical_and_int8"
#define VX_KERNEL_NAME_LOGICAL_AND_UINT8                   "com.vivantecorp.extension.vxcTensorLogical_and_uint8"
#define VX_KERNEL_NAME_LOGICAL_AND_FP16                    "com.vivantecorp.extension.vxcTensorLogical_and_fp16"
#define VX_KERNEL_NAME_SELECT_INT8                         "com.vivantecorp.extension.vxcTensorSelect_int8"
#define VX_KERNEL_NAME_SELECT_UINT8                        "com.vivantecorp.extension.vxcTensorSelect_uint8"
#define VX_KERNEL_NAME_SELECT_INT16                        "com.vivantecorp.extension.vxcTensorSelect_int16"
#define VX_KERNEL_NAME_LSTMUNIT_ACTIVATION                 "com.vivantecorp.extension.vxcLSTMUnit_Activation_SW"
#define VX_KERNEL_NAME_TENSORADD_MEAN_STDDEV_NORM_FP16     "com.vivantecorp.extension.vxcTensorAddMeanStdNorm_fp16"
#define VX_KERNEL_NAME_TENSORADD_MEAN_STDDEV_NORM_U8_FP16  "com.vivantecorp.extension.vxcTensorAddMeanStdNorm_u8_fp16"
#define VX_KERNEL_NAME_TENSORADD_MEAN_STDDEV_NORM_I16_FP16 "com.vivantecorp.extension.vxcTensorAddMeanStdNorm_i16_fp16"
#define VX_KERNEL_NAME_TENSORADD_MEAN_STDDEV_NORM_I16_FP16 "com.vivantecorp.extension.vxcTensorAddMeanStdNorm_i16_fp16"
#define VX_KERNEL_NAME_STACK                               "com.vivantecorp.extension.vxcStack"
//! negative kernel name define
#define VX_KERNEL_NAME_TENSOR_NEG_F16TOF16          "com.vivantecorp.extension.vxTensorNeg_F16toF16"
#define VX_KERNEL_NAME_TENSOR_NEG_F16TOF16_2D       "com.vivantecorp.extension.vxTensorNeg_F16toF16_2D"
#define VX_KERNEL_NAME_TENSOR_NEG_F16TOI8           "com.vivantecorp.extension.vxTensorNeg_F16toI8"
#define VX_KERNEL_NAME_TENSOR_NEG_F16TOI8_2D        "com.vivantecorp.extension.vxTensorNeg_F16toI8_2D"
#define VX_KERNEL_NAME_TENSOR_NEG_F16TOI16          "com.vivantecorp.extension.vxTensorNeg_F16toI16"
#define VX_KERNEL_NAME_TENSOR_NEG_F16TOI16_2D       "com.vivantecorp.extension.vxTensorNeg_F16toI16_2D"
#define VX_KERNEL_NAME_TENSOR_NEG_F16TOU8           "com.vivantecorp.extension.vxTensorNeg_F16toU8"
#define VX_KERNEL_NAME_TENSOR_NEG_F16TOU8_2D        "com.vivantecorp.extension.vxTensorNeg_F16toU8_2D"
#define VX_KERNEL_NAME_TENSOR_NEG_I8TOI8            "com.vivantecorp.extension.vxTensorNeg_I8toI8"
#define VX_KERNEL_NAME_TENSOR_NEG_I8TOI8_2D         "com.vivantecorp.extension.vxTensorNeg_I8toI8_2D"
#define VX_KERNEL_NAME_TENSOR_NEG_I8TOF16           "com.vivantecorp.extension.vxTensorNeg_I8toF16"
#define VX_KERNEL_NAME_TENSOR_NEG_I8TOF16_2D        "com.vivantecorp.extension.vxTensorNeg_I8toF16_2D"
#define VX_KERNEL_NAME_TENSOR_NEG_I16TOI16          "com.vivantecorp.extension.vxTensorNeg_I16toI16"
#define VX_KERNEL_NAME_TENSOR_NEG_I16TOI16_2D       "com.vivantecorp.extension.vxTensorNeg_I16toI16_2D"
#define VX_KERNEL_NAME_TENSOR_NEG_I16TOF16          "com.vivantecorp.extension.vxTensorNeg_I16toF16"
#define VX_KERNEL_NAME_TENSOR_NEG_I16TOF16_2D       "com.vivantecorp.extension.vxTensorNeg_I16toF16_2D"
#define VX_KERNEL_NAME_TENSOR_NEG_U8TOU8            "com.vivantecorp.extension.vxTensorNeg_U8toU8"
#define VX_KERNEL_NAME_TENSOR_NEG_U8TOU8_2D         "com.vivantecorp.extension.vxTensorNeg_U8toU8_2D"
#define VX_KERNEL_NAME_TENSOR_NEG_U8TOF16           "com.vivantecorp.extension.vxTensorNeg_U8toF16"
#define VX_KERNEL_NAME_TENSOR_NEG_U8TOF16_2D        "com.vivantecorp.extension.vxTensorNeg_U8toF16_2D"
#define VX_KERNEL_NAME_TENSOR_EXP_F16TOF16                 "com.vivantecorp.extension.vxTensorExp_F16toF16"
#define VX_KERNEL_NAME_TENSOR_EXP_F16TOF16_2D              "com.vivantecorp.extension.vxTensorExp_F16toF16_2D"
#define VX_KERNEL_NAME_TENSOR_EXP_F16TOI8                  "com.vivantecorp.extension.vxTensorExp_F16toI8"
#define VX_KERNEL_NAME_TENSOR_EXP_F16TOI8_2D               "com.vivantecorp.extension.vxTensorExp_F16toI8_2D"
#define VX_KERNEL_NAME_TENSOR_EXP_F16TOI16                 "com.vivantecorp.extension.vxTensorExp_F16toI16"
#define VX_KERNEL_NAME_TENSOR_EXP_F16TOI16_2D              "com.vivantecorp.extension.vxTensorExp_F16toI16_2D"
#define VX_KERNEL_NAME_TENSOR_EXP_F16TOU8                  "com.vivantecorp.extension.vxTensorExp_F16toU8"
#define VX_KERNEL_NAME_TENSOR_EXP_F16TOU8_2D               "com.vivantecorp.extension.vxTensorExp_F16toU8_2D"
#define VX_KERNEL_NAME_TENSOR_EXP_I8TOI8                   "com.vivantecorp.extension.vxTensorExp_I8toI8"
#define VX_KERNEL_NAME_TENSOR_EXP_I8TOI8_2D                "com.vivantecorp.extension.vxTensorExp_I8toI8_2D"
#define VX_KERNEL_NAME_TENSOR_EXP_I8TOF16                  "com.vivantecorp.extension.vxTensorExp_I8toF16"
#define VX_KERNEL_NAME_TENSOR_EXP_I8TOF16_2D               "com.vivantecorp.extension.vxTensorExp_I8toF16_2D"
#define VX_KERNEL_NAME_TENSOR_EXP_I16TOI16                 "com.vivantecorp.extension.vxTensorExp_I16toI16"
#define VX_KERNEL_NAME_TENSOR_EXP_I16TOI16_2D              "com.vivantecorp.extension.vxTensorExp_I16toI16_2D"
#define VX_KERNEL_NAME_TENSOR_EXP_I16TOF16                 "com.vivantecorp.extension.vxTensorExp_I16toF16"
#define VX_KERNEL_NAME_TENSOR_EXP_I16TOF16_2D              "com.vivantecorp.extension.vxTensorExp_I16toF16_2D"
#define VX_KERNEL_NAME_TENSOR_EXP_U8TOU8                   "com.vivantecorp.extension.vxTensorExp_U8toU8"
#define VX_KERNEL_NAME_TENSOR_EXP_U8TOU8_2D                "com.vivantecorp.extension.vxTensorExp_U8toU8_2D"
#define VX_KERNEL_NAME_TENSOR_EXP_U8TOF16                  "com.vivantecorp.extension.vxTensorExp_U8toF16"
#define VX_KERNEL_NAME_TENSOR_EXP_U8TOF16_2D               "com.vivantecorp.extension.vxTensorExp_U8toF16_2D"
#define VX_KERNEL_NAME_CLIP_FP16                           "com.vivantecorp.extension.vxcTensorClip_Fp16"
#define VX_KERNEL_NAME_CLIP_INT16                          "com.vivantecorp.extension.vxcTensorClip_Int16"
#define VX_KERNEL_NAME_CLIP_INT8                           "com.vivantecorp.extension.vxcTensorClip_Int8"
#define VX_KERNEL_NAME_CLIP_UINT8                          "com.vivantecorp.extension.vxcTensorClip_Uint8"
#define VX_KERNEL_NAME_PRE_PROCESS_GRAY_F16                "com.vivantecorp.extension.vxGrayScaletoTensor_F16"
#define VX_KERNEL_NAME_PRE_PROCESS_GRAY_I16                "com.vivantecorp.extension.vxGrayScaletoTensor_I16"
#define VX_KERNEL_NAME_PRE_PROCESS_GRAY_I8                 "com.vivantecorp.extension.vxGrayScaletoTensor_I8"
#define VX_KERNEL_NAME_PRE_PROCESS_GRAY_U8                 "com.vivantecorp.extension.vxGrayScaletoTensor_U8"
#define VX_KERNEL_NAME_PRE_PROCESS_GRAY_F16_COPY           "com.vivantecorp.extension.vxGrayScaletoTensor_F16_copy"
#define VX_KERNEL_NAME_PRE_PROCESS_GRAY_I16_COPY           "com.vivantecorp.extension.vxGrayScaletoTensor_I16_copy"
#define VX_KERNEL_NAME_PRE_PROCESS_GRAY_I8_COPY            "com.vivantecorp.extension.vxGrayScaletoTensor_I8_copy"
#define VX_KERNEL_NAME_PRE_PROCESS_GRAY_U8_COPY            "com.vivantecorp.extension.vxGrayScaletoTensor_U8_copy"
#define VX_KERNEL_NAME_UNSTACK                             "com.vivantecorp.extension.vxcUnstack"
#define VX_KERNEL_NAME_PRE_PROCESS_RGB_F16                "com.vivantecorp.extension.vxRGBScaletoTensor_F16"
#define VX_KERNEL_NAME_PRE_PROCESS_RGB_I16                "com.vivantecorp.extension.vxRGBScaletoTensor_I16"
#define VX_KERNEL_NAME_PRE_PROCESS_RGB_I8                 "com.vivantecorp.extension.vxRGBScaletoTensor_I8"
#define VX_KERNEL_NAME_PRE_PROCESS_RGB_U8                 "com.vivantecorp.extension.vxRGBScaletoTensor_U8"
#define VX_KERNEL_NAME_PRE_PROCESS_RGB_F16_COPY           "com.vivantecorp.extension.vxRGBScaletoTensor_F16_copy"
#define VX_KERNEL_NAME_PRE_PROCESS_RGB_I16_COPY           "com.vivantecorp.extension.vxRGBScaletoTensor_I16_copy"
#define VX_KERNEL_NAME_PRE_PROCESS_RGB_I8_COPY            "com.vivantecorp.extension.vxRGBScaletoTensor_I8_copy"
#define VX_KERNEL_NAME_PRE_PROCESS_RGB_U8_COPY            "com.vivantecorp.extension.vxRGBScaletoTensor_U8_copy"
#define VX_KERNEL_NAME_PRE_PROCESS_RGB_F16_NHWC           "com.vivantecorp.extension.vxRGBScaletoTensor_F16_NHWC"
#define VX_KERNEL_NAME_PRE_PROCESS_RGB_I16_NHWC           "com.vivantecorp.extension.vxRGBScaletoTensor_I16_NHWC"
#define VX_KERNEL_NAME_PRE_PROCESS_RGB_I8_NHWC            "com.vivantecorp.extension.vxRGBScaletoTensor_I8_NHWC"
#define VX_KERNEL_NAME_PRE_PROCESS_RGB_U8_NHWC            "com.vivantecorp.extension.vxRGBScaletoTensor_U8_NHWC"
#define VX_KERNEL_NAME_PRE_PROCESS_RGB_F16_COPY_NHWC      "com.vivantecorp.extension.vxRGBScaletoTensor_F16_copy_NHWC"
#define VX_KERNEL_NAME_PRE_PROCESS_RGB_I16_COPY_NHWC      "com.vivantecorp.extension.vxRGBScaletoTensor_I16_copy_NHWC"
#define VX_KERNEL_NAME_PRE_PROCESS_RGB_I8_COPY_NHWC       "com.vivantecorp.extension.vxRGBScaletoTensor_I8_copy_NHWC"
#define VX_KERNEL_NAME_PRE_PROCESS_RGB_U8_COPY_NHWC       "com.vivantecorp.extension.vxRGBScaletoTensor_U8_copy_NHWC"
#define VX_KERNEL_NAME_ADDN                               "com.vivantecorp.extension.vxcAddn"
#define VX_KERNEL_NAME_PRE_PROCESS_YUV2RBG_U8             "com.vivantecorp.extension.vxcYuv2rbg_u8"
#define VX_KERNEL_NAME_PRE_PROCESS_YUV2RBG_TRANS_U8       "com.vivantecorp.extension.vxcYuv2rbg_trans_u8"
#define VX_KERNEL_NAME_PRE_PROCESS_YUV2RBG_RESIZE_NORM_U8 "com.vivantecorp.extension.vxcYuv2rbg_resize_norm_u8"
#define VX_KERNEL_NAME_EXTRA_ENDING_I16                   "com.vivantecorp.extension.vxcExtra_ending_i16"
#define VX_KERNEL_NAME_EXTRA_ENDING_I8                    "com.vivantecorp.extension.vxcExtra_ending_i8"
#define VX_KERNEL_NAME_EXTRA_ENDING_U8                    "com.vivantecorp.extension.vxcExtra_ending_u8"
#define VX_KERNEL_NAME_GATHER                             "com.vivantecorp.extension.vxcGather"
#define VX_KERNEL_NAME_GATHER_INT8                        "com.vivantecorp.extension.vxcGather_i8"
#define VX_KERNEL_NAME_GATHER_INT16                       "com.vivantecorp.extension.vxcGather_i16"
#define VX_KERNEL_NAME_TILE                               "com.vivantecorp.extension.vxcTile"
#define VX_KERNEL_NAME_TOPK                               "com.vivantecorp.extension.vxcTopk"
#define VX_KERNEL_NAME_PRE_PROCESS_BGRA                   "com.vivantecorp.extension.vxcPre_process_bgra"
#define VX_KERNEL_NAME_PRE_PROCESS_BGRA_TRANS             "com.vivantecorp.extension.vxcPre_process_bgra_trans"
#define VX_KERNEL_NAME_PRE_PROCESS_BGRA_COPY              "com.vivantecorp.extension.vxcPre_process_bgra_copy"
#define VX_KERNEL_NAME_LOGICAL_NOT_INT8                   "com.vivantecorp.extension.vxcLogical_not_i8"
#define VX_KERNEL_NAME_LOGICAL_NOT_INT16                  "com.vivantecorp.extension.vxcLogical_not_i16"
#define VX_KERNEL_NAME_LOGICAL_NOT_UINT8                  "com.vivantecorp.extension.vxcLogical_not_u8"
#define VX_KERNEL_NAME_ROI_ALIGN                          "com.vivantecorp.extension.vxcRoi_align"
#define VX_KERNEL_NAME_HEATMAP_MAX_KEYPOINT               "com.vivantecorp.extension.vxcHeatmap_max_keypoint"
#define VX_KERNEL_NAME_AXIS_ALIGNED_BBOX_TRANSFORM        "com.vivantecorp.extension.vxcAxis_aligned_bbox_transform"
#define VX_KERNEL_NAME_BOX_WITH_NMS_LIMIT                 "com.vivantecorp.extension.vxcBox_with_nms_limit"
#define VX_KERNEL_NAME_GENERATE_PROPOSALS                 "com.vivantecorp.extension.vxcGenerate_proposals"
#define VX_KERNEL_NAME_DETECTION_POSTPROCESS              "com.vivantecorp.extension.vxcDetection_postprocess"
#define VX_KERNEL_NAME_RANDOM_MULTINOMIAL                 "com.vivantecorp.extension.vxcRandom_multinomial"
#define VX_KERNEL_NAME_RANDOM_GENERATE                    "com.vivantecorp.extension.vxcRandom_generate"
#define VX_KERNEL_NAME_RANDOM_SUM_FP16                    "com.vivantecorp.extension.vxcRandom_sum_fp16"
#define VX_KERNEL_NAME_RANDOM_SUM_FP32                    "com.vivantecorp.extension.vxcRandom_sum_fp32"
#define VX_KERNEL_NAME_TENSOR_EXP_U8TOF16_2D              "com.vivantecorp.extension.vxTensorExp_U8toF16_2D"
//! logsoftmax kernel name define
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_F16TOF16          "com.vivantecorp.extension.vxcLogSoftmaxAxis0_F16toF16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_F16TOI16          "com.vivantecorp.extension.vxcLogSoftmaxAxis0_F16toI16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_F16TOI8           "com.vivantecorp.extension.vxcLogSoftmaxAxis0_F16toI8"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_F16TOU8           "com.vivantecorp.extension.vxcLogSoftmaxAxis0_F16toU8"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_F16TOF32          "com.vivantecorp.extension.vxcLogSoftmaxAxis0_F16toF32"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_F16TOF16_2D       "com.vivantecorp.extension.vxcLogSoftmaxAxis0_F16toF16_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_F16TOI16_2D       "com.vivantecorp.extension.vxcLogSoftmaxAxis0_F16toI16_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_F16TOI8_2D        "com.vivantecorp.extension.vxcLogSoftmaxAxis0_F16toI8_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_F16TOU8_2D        "com.vivantecorp.extension.vxcLogSoftmaxAxis0_F16toU8_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_F16TOF32_2D       "com.vivantecorp.extension.vxcLogSoftmaxAxis0_F16toF32_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_I16TOI16          "com.vivantecorp.extension.vxcLogSoftmaxAxis0_I16toI16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_I16TOF16          "com.vivantecorp.extension.vxcLogSoftmaxAxis0_I16toF16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_I16TOF32          "com.vivantecorp.extension.vxcLogSoftmaxAxis0_I16toF32"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_I16TOI16_2D       "com.vivantecorp.extension.vxcLogSoftmaxAxis0_I16toI16_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_I16TOF16_2D       "com.vivantecorp.extension.vxcLogSoftmaxAxis0_I16toF16_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_I16TOF32_2D       "com.vivantecorp.extension.vxcLogSoftmaxAxis0_I16toF32_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_I8TOI8            "com.vivantecorp.extension.vxcLogSoftmaxAxis0_I8toI8"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_I8TOF16           "com.vivantecorp.extension.vxcLogSoftmaxAxis0_I8toF16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_I8TOF32           "com.vivantecorp.extension.vxcLogSoftmaxAxis0_I8toF32"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_I8TOI8_2D         "com.vivantecorp.extension.vxcLogSoftmaxAxis0_I8toI8_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_I8TOF16_2D        "com.vivantecorp.extension.vxcLogSoftmaxAxis0_I8toF16_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_I8TOF32_2D        "com.vivantecorp.extension.vxcLogSoftmaxAxis0_I8toF32_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_U8TOU8            "com.vivantecorp.extension.vxcLogSoftmaxAxis0_U8toU8"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_U8TOF16           "com.vivantecorp.extension.vxcLogSoftmaxAxis0_U8toF16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_U8TOF32           "com.vivantecorp.extension.vxcLogSoftmaxAxis0_U8toF32"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_U8TOU8_2D         "com.vivantecorp.extension.vxcLogSoftmaxAxis0_U8toU8_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_U8TOF16_2D        "com.vivantecorp.extension.vxcLogSoftmaxAxis0_U8toF16_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_U8TOF32_2D        "com.vivantecorp.extension.vxcLogSoftmaxAxis0_U8toF32_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_BF16TOF32         "com.vivantecorp.extension.vxcLogSoftmaxAxis0_BF16toF32"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_BF16TOF16         "com.vivantecorp.extension.vxcLogSoftmaxAxis0_BF16toF16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_BF16TOBF16        "com.vivantecorp.extension.vxcLogSoftmaxAxis0_BF16toBF16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_BF16TOF32_2D      "com.vivantecorp.extension.vxcLogSoftmaxAxis0_BF16toF32_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_BF16TOF16_2D      "com.vivantecorp.extension.vxcLogSoftmaxAxis0_BF16toF16_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI0_BF16TOBF16_2D     "com.vivantecorp.extension.vxcLogSoftmaxAxis0_BF16toBF16_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_F16TOF16          "com.vivantecorp.extension.vxcLogSoftmaxAxis1_F16toF16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_F16TOI16          "com.vivantecorp.extension.vxcLogSoftmaxAxis1_F16toI16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_F16TOI8           "com.vivantecorp.extension.vxcLogSoftmaxAxis1_F16toI8"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_F16TOU8           "com.vivantecorp.extension.vxcLogSoftmaxAxis1_F16toU8"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_F16TOF32          "com.vivantecorp.extension.vxcLogSoftmaxAxis1_F16toF32"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_F16TOF16_2D       "com.vivantecorp.extension.vxcLogSoftmaxAxis1_F16toF16_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_F16TOI16_2D       "com.vivantecorp.extension.vxcLogSoftmaxAxis1_F16toI16_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_F16TOI8_2D        "com.vivantecorp.extension.vxcLogSoftmaxAxis1_F16toI8_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_F16TOU8_2D        "com.vivantecorp.extension.vxcLogSoftmaxAxis1_F16toU8_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_F16TOF32_2D       "com.vivantecorp.extension.vxcLogSoftmaxAxis1_F16toF32_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_I16TOI16          "com.vivantecorp.extension.vxcLogSoftmaxAxis1_I16toI16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_I16TOF16          "com.vivantecorp.extension.vxcLogSoftmaxAxis1_I16toF16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_I16TOF32          "com.vivantecorp.extension.vxcLogSoftmaxAxis1_I16toF32"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_I16TOI16_2D       "com.vivantecorp.extension.vxcLogSoftmaxAxis1_I16toI16_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_I16TOF16_2D       "com.vivantecorp.extension.vxcLogSoftmaxAxis1_I16toF16_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_I16TOF32_2D       "com.vivantecorp.extension.vxcLogSoftmaxAxis1_I16toF32_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_I8TOI8            "com.vivantecorp.extension.vxcLogSoftmaxAxis1_I8toI8"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_I8TOF16           "com.vivantecorp.extension.vxcLogSoftmaxAxis1_I8toF16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_I8TOF32           "com.vivantecorp.extension.vxcLogSoftmaxAxis1_I8toF32"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_I8TOI8_2D         "com.vivantecorp.extension.vxcLogSoftmaxAxis1_I8toI8_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_I8TOF16_2D        "com.vivantecorp.extension.vxcLogSoftmaxAxis1_I8toF16_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_I8TOF32_2D        "com.vivantecorp.extension.vxcLogSoftmaxAxis1_I8toF32_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_U8TOU8            "com.vivantecorp.extension.vxcLogSoftmaxAxis1_U8toU8"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_U8TOF16           "com.vivantecorp.extension.vxcLogSoftmaxAxis1_U8toF16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_U8TOF32           "com.vivantecorp.extension.vxcLogSoftmaxAxis1_U8toF32"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_U8TOU8_2D         "com.vivantecorp.extension.vxcLogSoftmaxAxis1_U8toU8_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_U8TOF16_2D        "com.vivantecorp.extension.vxcLogSoftmaxAxis1_U8toF16_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_U8TOF32_2D        "com.vivantecorp.extension.vxcLogSoftmaxAxis1_U8toF32_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_BF16TOF32         "com.vivantecorp.extension.vxcLogSoftmaxAxis1_BF16toF32"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_BF16TOF16         "com.vivantecorp.extension.vxcLogSoftmaxAxis1_BF16toF16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_BF16TOBF16        "com.vivantecorp.extension.vxcLogSoftmaxAxis1_BF16toBF16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_BF16TOF32_2D      "com.vivantecorp.extension.vxcLogSoftmaxAxis1_BF16toF32_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_BF16TOF16_2D      "com.vivantecorp.extension.vxcLogSoftmaxAxis1_BF16toF16_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI1_BF16TOBF16_2D     "com.vivantecorp.extension.vxcLogSoftmaxAxis1_BF16toBF16_2D"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI2_F16TOF16          "com.vivantecorp.extension.vxcLogSoftmaxAxis2_F16toF16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI2_F16TOI16          "com.vivantecorp.extension.vxcLogSoftmaxAxis2_F16toI16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI2_F16TOI8           "com.vivantecorp.extension.vxcLogSoftmaxAxis2_F16toI8"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI2_F16TOU8           "com.vivantecorp.extension.vxcLogSoftmaxAxis2_F16toU8"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI2_F16TOF32          "com.vivantecorp.extension.vxcLogSoftmaxAxis2_F16toF32"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI2_I16TOI16          "com.vivantecorp.extension.vxcLogSoftmaxAxis2_I16toI16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI2_I16TOF16          "com.vivantecorp.extension.vxcLogSoftmaxAxis2_I16toF16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI2_I16TOF32          "com.vivantecorp.extension.vxcLogSoftmaxAxis2_I16toF32"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI2_I8TOI8            "com.vivantecorp.extension.vxcLogSoftmaxAxis2_I8toI8"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI2_I8TOF16           "com.vivantecorp.extension.vxcLogSoftmaxAxis2_I8toF16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI2_I8TOF32           "com.vivantecorp.extension.vxcLogSoftmaxAxis2_I8toF32"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI2_U8TOU8            "com.vivantecorp.extension.vxcLogSoftmaxAxis2_U8toU8"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI2_U8TOF16           "com.vivantecorp.extension.vxcLogSoftmaxAxis2_U8toF16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI2_U8TOF32           "com.vivantecorp.extension.vxcLogSoftmaxAxis2_U8toF32"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI2_BF16TOF32         "com.vivantecorp.extension.vxcLogSoftmaxAxis2_BF16toF32"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI2_BF16TOF16         "com.vivantecorp.extension.vxcLogSoftmaxAxis2_BF16toF16"
#define VX_KERNEL_NAME_LOG_SOFTMAX_AXI2_BF16TOBF16        "com.vivantecorp.extension.vxcLogSoftmaxAxis2_BF16toBF16"
//! reducemax kernel
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_F16TOF16          "com.vivantecorp.extension.vxcReducemaxAxis0_F16toF16"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_F16TOI16          "com.vivantecorp.extension.vxcReducemaxAxis0_F16toI16"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_F16TOI8           "com.vivantecorp.extension.vxcReducemaxAxis0_F16toI8"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_F16TOU8           "com.vivantecorp.extension.vxcReducemaxAxis0_F16toU8"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_F16TOF16_2D       "com.vivantecorp.extension.vxcReducemaxAxis0_F16toF16_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_F16TOI16_2D       "com.vivantecorp.extension.vxcReducemaxAxis0_F16toI16_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_F16TOI8_2D        "com.vivantecorp.extension.vxcReducemaxAxis0_F16toI8_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_F16TOU8_2D        "com.vivantecorp.extension.vxcReducemaxAxis0_F16toU8_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_I16TOI16          "com.vivantecorp.extension.vxcReducemaxAxis0_I16toI16"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_I16TOF16          "com.vivantecorp.extension.vxcReducemaxAxis0_I16toF16"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_I16TOI16_2D       "com.vivantecorp.extension.vxcReducemaxAxis0_I16toI16_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_I16TOF16_2D       "com.vivantecorp.extension.vxcReducemaxAxis0_I16toF16_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_I8TOI8            "com.vivantecorp.extension.vxcReducemaxAxis0_I8toI8"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_I8TOF16           "com.vivantecorp.extension.vxcReducemaxAxis0_I8toF16"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_I8TOI8_2D         "com.vivantecorp.extension.vxcReducemaxAxis0_I8toI8_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_I8TOF16_2D        "com.vivantecorp.extension.vxcReducemaxAxis0_I8toF16_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_U8TOU8            "com.vivantecorp.extension.vxcReducemaxAxis0_U8toU8"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_U8TOF16           "com.vivantecorp.extension.vxcReducemaxAxis0_U8toF16"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_U8TOU8_2D         "com.vivantecorp.extension.vxcReducemaxAxis0_U8toU8_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI0_U8TOF16_2D        "com.vivantecorp.extension.vxcReducemaxAxis0_U8toF16_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_F16TOF16          "com.vivantecorp.extension.vxcReducemaxAxis1_F16toF16"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_F16TOI16          "com.vivantecorp.extension.vxcReducemaxAxis1_F16toI16"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_F16TOI8           "com.vivantecorp.extension.vxcReducemaxAxis1_F16toI8"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_F16TOU8           "com.vivantecorp.extension.vxcReducemaxAxis1_F16toU8"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_F16TOF16_2D       "com.vivantecorp.extension.vxcReducemaxAxis1_F16toF16_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_F16TOI16_2D       "com.vivantecorp.extension.vxcReducemaxAxis1_F16toI16_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_F16TOI8_2D        "com.vivantecorp.extension.vxcReducemaxAxis1_F16toI8_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_F16TOU8_2D        "com.vivantecorp.extension.vxcReducemaxAxis1_F16toU8_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_I16TOI16          "com.vivantecorp.extension.vxcReducemaxAxis1_I16toI16"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_I16TOF16          "com.vivantecorp.extension.vxcReducemaxAxis1_I16toF16"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_I16TOI16_2D       "com.vivantecorp.extension.vxcReducemaxAxis1_I16toI16_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_I16TOF16_2D       "com.vivantecorp.extension.vxcReducemaxAxis1_I16toF16_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_I8TOI8            "com.vivantecorp.extension.vxcReducemaxAxis1_I8toI8"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_I8TOF16           "com.vivantecorp.extension.vxcReducemaxAxis1_I8toF16"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_I8TOI8_2D         "com.vivantecorp.extension.vxcReducemaxAxis1_I8toI8_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_I8TOF16_2D        "com.vivantecorp.extension.vxcReducemaxAxis1_I8toF16_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_U8TOU8            "com.vivantecorp.extension.vxcReducemaxAxis1_U8toU8"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_U8TOF16           "com.vivantecorp.extension.vxcReducemaxAxis1_U8toF16"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_U8TOU8_2D         "com.vivantecorp.extension.vxcReducemaxAxis1_U8toU8_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI1_U8TOF16_2D        "com.vivantecorp.extension.vxcReducemaxAxis1_U8toF16_2D"
#define VX_KERNEL_NAME_REDUCEMAX_AXI2_F16TOF16          "com.vivantecorp.extension.vxcReducemaxAxis2_F16toF16"
#define VX_KERNEL_NAME_REDUCEMAX_AXI2_F16TOI16          "com.vivantecorp.extension.vxcReducemaxAxis2_F16toI16"
#define VX_KERNEL_NAME_REDUCEMAX_AXI2_F16TOI8           "com.vivantecorp.extension.vxcReducemaxAxis2_F16toI8"
#define VX_KERNEL_NAME_REDUCEMAX_AXI2_F16TOU8           "com.vivantecorp.extension.vxcReducemaxAxis2_F16toU8"
#define VX_KERNEL_NAME_REDUCEMAX_AXI2_I16TOI16          "com.vivantecorp.extension.vxcReducemaxAxis2_I16toI16"
#define VX_KERNEL_NAME_REDUCEMAX_AXI2_I16TOF16          "com.vivantecorp.extension.vxcReducemaxAxis2_I16toF16"
#define VX_KERNEL_NAME_REDUCEMAX_AXI2_I8TOI8            "com.vivantecorp.extension.vxcReducemaxAxis2_I8toI8"
#define VX_KERNEL_NAME_REDUCEMAX_AXI2_I8TOF16           "com.vivantecorp.extension.vxcReducemaxAxis2_I8toF16"
#define VX_KERNEL_NAME_REDUCEMAX_AXI2_U8TOU8            "com.vivantecorp.extension.vxcReducemaxAxis2_U8toU8"
#define VX_KERNEL_NAME_REDUCEMAX_AXI2_U8TOF16           "com.vivantecorp.extension.vxcReducemaxAxis2_U8toF16"
//! reducemin kernel
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_F16TOF16          "com.vivantecorp.extension.vxcReduceminAxis0_F16toF16"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_F16TOI16          "com.vivantecorp.extension.vxcReduceminAxis0_F16toI16"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_F16TOI8           "com.vivantecorp.extension.vxcReduceminAxis0_F16toI8"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_F16TOU8           "com.vivantecorp.extension.vxcReduceminAxis0_F16toU8"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_F16TOF16_2D       "com.vivantecorp.extension.vxcReduceminAxis0_F16toF16_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_F16TOI16_2D       "com.vivantecorp.extension.vxcReduceminAxis0_F16toI16_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_F16TOI8_2D        "com.vivantecorp.extension.vxcReduceminAxis0_F16toI8_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_F16TOU8_2D        "com.vivantecorp.extension.vxcReduceminAxis0_F16toU8_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_I16TOI16          "com.vivantecorp.extension.vxcReduceminAxis0_I16toI16"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_I16TOF16          "com.vivantecorp.extension.vxcReduceminAxis0_I16toF16"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_I16TOI16_2D       "com.vivantecorp.extension.vxcReduceminAxis0_I16toI16_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_I16TOF16_2D       "com.vivantecorp.extension.vxcReduceminAxis0_I16toF16_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_I8TOI8            "com.vivantecorp.extension.vxcReduceminAxis0_I8toI8"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_I8TOF16           "com.vivantecorp.extension.vxcReduceminAxis0_I8toF16"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_I8TOI8_2D         "com.vivantecorp.extension.vxcReduceminAxis0_I8toI8_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_I8TOF16_2D        "com.vivantecorp.extension.vxcReduceminAxis0_I8toF16_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_U8TOU8            "com.vivantecorp.extension.vxcReduceminAxis0_U8toU8"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_U8TOF16           "com.vivantecorp.extension.vxcReduceminAxis0_U8toF16"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_U8TOU8_2D         "com.vivantecorp.extension.vxcReduceminAxis0_U8toU8_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI0_U8TOF16_2D        "com.vivantecorp.extension.vxcReduceminAxis0_U8toF16_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_F16TOF16          "com.vivantecorp.extension.vxcReduceminAxis1_F16toF16"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_F16TOI16          "com.vivantecorp.extension.vxcReduceminAxis1_F16toI16"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_F16TOI8           "com.vivantecorp.extension.vxcReduceminAxis1_F16toI8"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_F16TOU8           "com.vivantecorp.extension.vxcReduceminAxis1_F16toU8"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_F16TOF16_2D       "com.vivantecorp.extension.vxcReduceminAxis1_F16toF16_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_F16TOI16_2D       "com.vivantecorp.extension.vxcReduceminAxis1_F16toI16_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_F16TOI8_2D        "com.vivantecorp.extension.vxcReduceminAxis1_F16toI8_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_F16TOU8_2D        "com.vivantecorp.extension.vxcReduceminAxis1_F16toU8_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_I16TOI16          "com.vivantecorp.extension.vxcReduceminAxis1_I16toI16"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_I16TOF16          "com.vivantecorp.extension.vxcReduceminAxis1_I16toF16"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_I16TOI16_2D       "com.vivantecorp.extension.vxcReduceminAxis1_I16toI16_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_I16TOF16_2D       "com.vivantecorp.extension.vxcReduceminAxis1_I16toF16_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_I8TOI8            "com.vivantecorp.extension.vxcReduceminAxis1_I8toI8"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_I8TOF16           "com.vivantecorp.extension.vxcReduceminAxis1_I8toF16"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_I8TOI8_2D         "com.vivantecorp.extension.vxcReduceminAxis1_I8toI8_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_I8TOF16_2D        "com.vivantecorp.extension.vxcReduceminAxis1_I8toF16_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_U8TOU8            "com.vivantecorp.extension.vxcReduceminAxis1_U8toU8"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_U8TOF16           "com.vivantecorp.extension.vxcReduceminAxis1_U8toF16"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_U8TOU8_2D         "com.vivantecorp.extension.vxcReduceminAxis1_U8toU8_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI1_U8TOF16_2D        "com.vivantecorp.extension.vxcReduceminAxis1_U8toF16_2D"
#define VX_KERNEL_NAME_REDUCEMIN_AXI2_F16TOF16          "com.vivantecorp.extension.vxcReduceminAxis2_F16toF16"
#define VX_KERNEL_NAME_REDUCEMIN_AXI2_F16TOI16          "com.vivantecorp.extension.vxcReduceminAxis2_F16toI16"
#define VX_KERNEL_NAME_REDUCEMIN_AXI2_F16TOI8           "com.vivantecorp.extension.vxcReduceminAxis2_F16toI8"
#define VX_KERNEL_NAME_REDUCEMIN_AXI2_F16TOU8           "com.vivantecorp.extension.vxcReduceminAxis2_F16toU8"
#define VX_KERNEL_NAME_REDUCEMIN_AXI2_I16TOI16          "com.vivantecorp.extension.vxcReduceminAxis2_I16toI16"
#define VX_KERNEL_NAME_REDUCEMIN_AXI2_I16TOF16          "com.vivantecorp.extension.vxcReduceminAxis2_I16toF16"
#define VX_KERNEL_NAME_REDUCEMIN_AXI2_I8TOI8            "com.vivantecorp.extension.vxcReduceminAxis2_I8toI8"
#define VX_KERNEL_NAME_REDUCEMIN_AXI2_I8TOF16           "com.vivantecorp.extension.vxcReduceminAxis2_I8toF16"
#define VX_KERNEL_NAME_REDUCEMIN_AXI2_U8TOU8            "com.vivantecorp.extension.vxcReduceminAxis2_U8toU8"
#define VX_KERNEL_NAME_REDUCEMIN_AXI2_U8TOF16           "com.vivantecorp.extension.vxcReduceminAxis2_U8toF16"
//! reduceprod kernel
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_F16TOF16          "com.vivantecorp.extension.vxcReduceProdAxis0_F16toF16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_F16TOI16          "com.vivantecorp.extension.vxcReduceProdAxis0_F16toI16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_F16TOI8           "com.vivantecorp.extension.vxcReduceProdAxis0_F16toI8"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_F16TOU8           "com.vivantecorp.extension.vxcReduceProdAxis0_F16toU8"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_F16TOF16_2D       "com.vivantecorp.extension.vxcReduceProdAxis0_F16toF16_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_F16TOI16_2D       "com.vivantecorp.extension.vxcReduceProdAxis0_F16toI16_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_F16TOI8_2D        "com.vivantecorp.extension.vxcReduceProdAxis0_F16toI8_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_F16TOU8_2D        "com.vivantecorp.extension.vxcReduceProdAxis0_F16toU8_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_I16TOI16          "com.vivantecorp.extension.vxcReduceProdAxis0_I16toI16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_I16TOF16          "com.vivantecorp.extension.vxcReduceProdAxis0_I16toF16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_I16TOI16_2D       "com.vivantecorp.extension.vxcReduceProdAxis0_I16toI16_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_I16TOF16_2D       "com.vivantecorp.extension.vxcReduceProdAxis0_I16toF16_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_I8TOI8            "com.vivantecorp.extension.vxcReduceProdAxis0_I8toI8"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_I8TOF16           "com.vivantecorp.extension.vxcReduceProdAxis0_I8toF16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_I8TOI8_2D         "com.vivantecorp.extension.vxcReduceProdAxis0_I8toI8_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_I8TOF16_2D        "com.vivantecorp.extension.vxcReduceProdAxis0_I8toF16_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_U8TOU8            "com.vivantecorp.extension.vxcReduceProdAxis0_U8toU8"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_U8TOF16           "com.vivantecorp.extension.vxcReduceProdAxis0_U8toF16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_U8TOU8_2D         "com.vivantecorp.extension.vxcReduceProdAxis0_U8toU8_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_U8TOF16_2D        "com.vivantecorp.extension.vxcReduceProdAxis0_U8toF16_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_BF16TOBF16        "com.vivantecorp.extension.vxcReduceProdAxis0_BF16toBF16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI0_BF16TOBF16_2D     "com.vivantecorp.extension.vxcReduceProdAxis0_BF16toBF16_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_F16TOF16          "com.vivantecorp.extension.vxcReduceProdAxis1_F16toF16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_F16TOI16          "com.vivantecorp.extension.vxcReduceProdAxis1_F16toI16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_F16TOI8           "com.vivantecorp.extension.vxcReduceProdAxis1_F16toI8"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_F16TOU8           "com.vivantecorp.extension.vxcReduceProdAxis1_F16toU8"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_F16TOF16_2D       "com.vivantecorp.extension.vxcReduceProdAxis1_F16toF16_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_F16TOI16_2D       "com.vivantecorp.extension.vxcReduceProdAxis1_F16toI16_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_F16TOI8_2D        "com.vivantecorp.extension.vxcReduceProdAxis1_F16toI8_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_F16TOU8_2D        "com.vivantecorp.extension.vxcReduceProdAxis1_F16toU8_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_I16TOI16          "com.vivantecorp.extension.vxcReduceProdAxis1_I16toI16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_I16TOF16          "com.vivantecorp.extension.vxcReduceProdAxis1_I16toF16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_I16TOI16_2D       "com.vivantecorp.extension.vxcReduceProdAxis1_I16toI16_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_I16TOF16_2D       "com.vivantecorp.extension.vxcReduceProdAxis1_I16toF16_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_I8TOI8            "com.vivantecorp.extension.vxcReduceProdAxis1_I8toI8"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_I8TOF16           "com.vivantecorp.extension.vxcReduceProdAxis1_I8toF16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_I8TOI8_2D         "com.vivantecorp.extension.vxcReduceProdAxis1_I8toI8_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_I8TOF16_2D        "com.vivantecorp.extension.vxcReduceProdAxis1_I8toF16_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_U8TOU8            "com.vivantecorp.extension.vxcReduceProdAxis1_U8toU8"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_U8TOF16           "com.vivantecorp.extension.vxcReduceProdAxis1_U8toF16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_U8TOU8_2D         "com.vivantecorp.extension.vxcReduceProdAxis1_U8toU8_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_U8TOF16_2D        "com.vivantecorp.extension.vxcReduceProdAxis1_U8toF16_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_BF16TOBF16        "com.vivantecorp.extension.vxcReduceProdAxis1_BF16toBF16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI1_BF16TOBF16_2D     "com.vivantecorp.extension.vxcReduceProdAxis1_BF16toBF16_2D"
#define VX_KERNEL_NAME_REDUCEPROD_AXI2_F16TOF16          "com.vivantecorp.extension.vxcReduceProdAxis2_F16toF16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI2_F16TOI16          "com.vivantecorp.extension.vxcReduceProdAxis2_F16toI16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI2_F16TOI8           "com.vivantecorp.extension.vxcReduceProdAxis2_F16toI8"
#define VX_KERNEL_NAME_REDUCEPROD_AXI2_F16TOU8           "com.vivantecorp.extension.vxcReduceProdAxis2_F16toU8"
#define VX_KERNEL_NAME_REDUCEPROD_AXI2_I16TOI16          "com.vivantecorp.extension.vxcReduceProdAxis2_I16toI16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI2_I16TOF16          "com.vivantecorp.extension.vxcReduceProdAxis2_I16toF16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI2_I8TOI8            "com.vivantecorp.extension.vxcReduceProdAxis2_I8toI8"
#define VX_KERNEL_NAME_REDUCEPROD_AXI2_I8TOF16           "com.vivantecorp.extension.vxcReduceProdAxis2_I8toF16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI2_U8TOU8            "com.vivantecorp.extension.vxcReduceProdAxis2_U8toU8"
#define VX_KERNEL_NAME_REDUCEPROD_AXI2_U8TOF16           "com.vivantecorp.extension.vxcReduceProdAxis2_U8toF16"
#define VX_KERNEL_NAME_REDUCEPROD_AXI2_BF16TOBF16        "com.vivantecorp.extension.vxcReduceProdAxis2_BF16toBF16"
//! reduceall kernel
#define VX_KERNEL_NAME_REDUCEALL_AXI0_I8TOI8            "com.vivantecorp.extension.vxcReduceallAxis0_I8toI8"
#define VX_KERNEL_NAME_REDUCEALL_AXI0_I8TOI8_2D         "com.vivantecorp.extension.vxcReduceallAxis0_I8toI8_2D"
#define VX_KERNEL_NAME_REDUCEALL_AXI1_I8TOI8            "com.vivantecorp.extension.vxcReduceallAxis1_I8toI8"
#define VX_KERNEL_NAME_REDUCEALL_AXI1_I8TOI8_2D         "com.vivantecorp.extension.vxcReduceallAxis1_I8toI8_2D"
#define VX_KERNEL_NAME_REDUCEALL_AXI2_I8TOI8            "com.vivantecorp.extension.vxcReduceallAxis2_I8toI8"
//! reduceany kernel
#define VX_KERNEL_NAME_REDUCEANY_AXI0_I8TOI8            "com.vivantecorp.extension.vxcReduceanyAxis0_I8toI8"
#define VX_KERNEL_NAME_REDUCEANY_AXI0_I8TOI8_2D         "com.vivantecorp.extension.vxcReduceanyAxis0_I8toI8_2D"
#define VX_KERNEL_NAME_REDUCEANY_AXI1_I8TOI8            "com.vivantecorp.extension.vxcReduceanyAxis1_I8toI8"
#define VX_KERNEL_NAME_REDUCEANY_AXI1_I8TOI8_2D         "com.vivantecorp.extension.vxcReduceanyAxis1_I8toI8_2D"
#define VX_KERNEL_NAME_REDUCEANY_AXI2_I8TOI8            "com.vivantecorp.extension.vxcReduceanyAxis2_I8toI8"

/*! \brief The Example Library Set
 * \ingroup group_example_ext
 */
#define VX_LIBRARY_LIBNNEXT (0x3) // assigned from Khronos, vendors control their own

/*! \brief The list of Example Kernels.
 * \ingroup group_xyz_ext
 */
//! [KERNEL ENUM]
enum vx_kernel_libnnext_ext_e
{
    /*! \brief The Example Kernel */
    VX_KERNEL_ENUM_LIBNNEXT             =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_LIBNNEXT_OFFSET,
    VX_KERNEL_ENUM_PERMUTECWH           =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_PREMUTE_OFFSET,
    VX_KERNEL_ENUM_PERMUTECHW           =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_PREMUTE_OFFSET + 1,
    VX_KERNEL_ENUM_PRIORBOX             =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_PRIORBOX_OFFSET,
    VX_KERNEL_ENUM_FLATTEN              =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_FLATTEN_OFFSET,
    VX_KERNEL_ENUM_L2NORMALIZESCALE     =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_L2NORMALIZESCALE_OFFSET,
    VX_KERNEL_ENUM_L2NORMSCALE_SUMRSQRT =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_L2NORMALIZESCALE_OFFSET + 1,
    VX_KERNEL_ENUM_L2NORMSCALE_MULSCALE =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_L2NORMALIZESCALE_OFFSET + 2,
    VX_KERNEL_ENUM_PARAMETRICRELU       =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_PARAMETRICRELU_OFFSET,
    VX_KERNEL_ENUM_PREBBOX              =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_PREBBOX_OFFSET,
    VX_KERNEL_ENUM_ADD_RELU_KERNEL      =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_ADD_RELU_KERNEL_OFFSET,
    VX_KERNEL_ENUM_POOLING_WITH_ARGMAX  =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_POOLING_WITH_ARGMAX_OFFSET,
    VX_KERNEL_ENUM_UNPOOLING            =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_UNPOOLING_OFFSET,
    VX_KERNEL_ENUM_ARGMAX               =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_ARGMAX_OFFSET,
    VX_KERNEL_ENUM_ALEXNET_GEMM         =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_ALEXNET_GEMM_OFFSET,
    VX_KERNEL_ENUM_IMG2COL_DILATED      =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_IMG2COL_DILATED_OFFSET,
    VX_KERNEL_ENUM_IMG2COL_DILATED_INT8 =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_IMG2COL_DILATED_INT8_OFFSET,
    VX_KERNEL_ENUM_ALEXNET_GEMM_INT8    =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_ALEXNET_GEMM_INT8_OFFSET,
    VX_KERNEL_ENUM_MAXIMUM          =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_ELTWISE_MAX,
    VX_KERNEL_ENUM_FULLYCONNECTED_AXIS2 =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_FULLYCONNECTED_AXIS2,
    VX_KERNEL_ENUM_TENSORCROP_INT16     =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TENSORCROP_INT16,
    VX_KERNEL_ENUM_TENSORCROP_INT8      =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TENSORCROP_INT8,
    VX_KERNEL_ENUM_TENSORCROP_INT16_FP16 =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TENSORCROP_INT16_FP16,
    VX_KERNEL_ENUM_DROPOUT              =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_DROPOUT,
    VX_KERNEL_ENUM_SHUFFLECHANNEL       =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_SHUFFLECHANNEL,
    VX_KERNEL_ENUM_RESIZE               =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_RESIZE,
    VX_KERNEL_ENUM_REVERSE              =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_REVERSE,
    VX_KERNEL_ENUM_RESIZE_16BITS_DOWNSAMPLE_QUARTER =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_RESIZE_16BITS_DOWNSAMPLE_QUARTER,
    VX_KERNEL_ENUM_RESIZE_8BITS_DOWNSAMPLE_QUARTER =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_RESIZE_8BITS_DOWNSAMPLE_QUARTER,
    VX_KERNEL_ENUM_SCALE                = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_SCALE,
    VX_KERNEL_ENUM_TENSORREVERSE        =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TENSORREVERSE,
    VX_KERNEL_ENUM_TENSORELU            =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TENSORELU_OFFSET,
    VX_KERNEL_ENUM_SPACE2BATCH          =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_SPACE2BATCH,
    VX_KERNEL_ENUM_BATCH2SPACE          =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_BATCH2SPACE,
    VX_KERNEL_ENUM_SPACE2DEPTH          =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_SPACE2DEPTH,
    VX_KERNEL_ENUM_IMAGEPROCESS         =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_IMAGEPROCESS,
    VX_KERNEL_ENUM_SCALETOTENSOR        =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_SCALETOTENSOR,
    VX_KERNEL_ENUM_GRAYSCALETOTENSOR    =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_GRAYSCALETOTENSOR,
    VX_KERNEL_ENUM_GEMM                 = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_GEMM,
    VX_KERNEL_ENUM_LAYERNORM            = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_LAYERNORM,
    VX_KERNEL_ENUM_LAYERNORM_FP16TOU8   =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_LAYERNORMFP16TOU8_OFFSET,
    VX_KERNEL_ENUM_REDUCE               = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_REDUCE,
    VX_KERNEL_ENUM_INSTANCENORM         =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_INSTANCENORM,
    VX_KERNEL_ENUM_TENSORSTACKCONCAT    =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TENSORSTACKCONCAT,
    VX_KERNEL_ENUM_TENSORSTACKCONCAT8BITS =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TENSORSTACKCONCAT8BITS_OFFSET,
    VX_KERNEL_ENUM_SIGNALFRAME          =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_SIGNALFRAME,
    VX_KERNEL_ENUM_RELATIONALOPS        =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_RELATIONALOPS,
    VX_KERNEL_ENUM_POW        =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_POW,
    VX_KERNEL_ENUM_FLOORDIV        =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_FLOORDIV,
    VX_KERNEL_ENUM_MINIMUM              = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_MINIMUM,
    VX_KERNEL_ENUM_SPATIAL_TRANSFORMER  =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_SPATIAL_TRANSFORMER,
    VX_KERNEL_ENUM_LOGICAL_OPS          = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_LOGICAL_OPS,
    VX_KERNEL_ENUM_SELECT               = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_SELECT,
    VX_KERNEL_ENUM_LSTMUNIT_ACTIVATION  =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_LSTMUNIT_ACTIVATION,
    VX_KERNEL_ENUM_TENSOR_ADD_MEAN_STDDEV_NORM =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TENSOR_ADD_MEAN_STDDEV_NORM,
    VX_KERNEL_ENUM_STACK                = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_STACK,
    VX_KERNEL_ENUM_NEG                  = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_NEG,
    VX_KERNEL_ENUM_TENSOR_EXP           = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_EXP,
    VX_KERNEL_ENUM_CLIP                 = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_CLIP,
    VX_KERNEL_ENUM_PRE_PROCESS_GRAY     =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_PRE_PROCESS_GRAY,
    VX_KERNEL_ENUM_UNSTACK              = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_UNSTACK,
    VX_KERNEL_ENUM_PRE_PROCESS_RGB      =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_PRE_PROCESS_RGB,
    VX_KERNEL_ENUM_ADDN                 = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_ADDN,
    VX_KERNEL_ENUM_PRE_PROCESS_YUV420   =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_PRE_PROCESS_YUV420,
    VX_KERNEL_ENUM_CONV2D               = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_CONV2D,
    VX_KERNEL_ENUM_EXTRA_ENDING         =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_EXTRA_ENDING,
    VX_KERNEL_ENUM_GATHER               = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_GATHER,
    VX_KERNEL_ENUM_TILE                 = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TILE,
    VX_KERNEL_ENUM_TOPK                 = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TOPK,
    VX_KERNEL_ENUM_PRE_PROCESS_BGRA     =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_PRE_PROCESS_BGRA,
    VX_KERNEL_ENUM_LOGICAL_NOT          =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_LOGICAL_NOT,
    VX_KERNEL_ENUM_TENSOR_SIN           = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_SIN,
    VX_KERNEL_ENUM_TENSOR_LOG           = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_LOG,
    VX_KERNEL_ENUM_ARGMIN               = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_ARGMIN,
    VX_KERNEL_ENUM_ROI_ALIGN            = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_ROI_ALIGN,
    VX_KERNEL_ENUM_HEATMAP_MAX_KEYPOINT =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_HEATMAP_MAX_KEYPOINT,
    VX_KERNEL_ENUM_AXIS_ALIGNED_BBOX_TRANSFORM =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_AXIS_ALIGNED_BBOX_TRANSFORM,
    VX_KERNEL_ENUM_BOX_WITH_NMS_LIMIT   =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_BOX_WITH_NMS_LIMIT,
    VX_KERNEL_ENUM_GENERATE_PROPOSALS   =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_GENERATE_PROPOSALS,
    VX_KERNEL_ENUM_DETECTION_POSTPROCESS =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_DETECTION_POSTPROCESS,
    VX_KERNEL_ENUM_RANDOM_MULTINOMIAL   =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_RANDOM_MULTINOMIAL,
    VX_KERNEL_ENUM_LOG_SOFTMAX          = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_LOG_SOFTMAX,
    VX_KERNEL_ENUM_RELU_KERAS_INTERNAL  =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_RELU_KERAS_INTERNAL,
    VX_KERNEL_ENUM_DECONV2D               = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_DECONV2D,
    VX_KERNEL_ENUM_REDUCEMAX_INTERNAL   =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_REDUCEMAX_INTERNAL,
    VX_KERNEL_ENUM_REDUCEMIN_INTERNAL   =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_REDUCEMIN_INTERNAL,
    VX_KERNEL_ENUM_REDUCEPROD_INTERNAL  =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_REDUCEPROD_INTERNAL,
    VX_KERNEL_ENUM_REDUCEALL_INTERNAL   =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_REDUCEALL_INTERNAL,
    VX_KERNEL_ENUM_REDUCEANY_INTERNAL   =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_REDUCEANY_INTERNAL,
    // up to 0xFFF kernel enums can be created.
};

/* Assigned from Khronos, for custom */
#define VX_LIBRARY_CUSTOM (0x4)
enum vx_kernel_custom_id_e
{
    _VX_CLIENT_ID_START = VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_CUSTOM ),
#define DEF_OP( name )     VX_CLIENT_ID_##name,
    #include "custom/custom_ops.def"
#undef DEF_OP
};
#define VX_KERNEL_ID( name ) VX_CLIENT_ID_##name

#ifndef gvxOBJ_CHECK
#define gvxOBJ_CHECK(ref) \
    do \
    { \
        status = vxGetStatus((vx_reference)ref); \
        if (ref == 0 || status != VX_SUCCESS) \
        { \
            printf("Obj ERROR: status=%d @ %s(%d)\n", status, __FUNCTION__, __LINE__); \
        } \
    } \
    while (0)
#endif
#ifndef gvxSTATUS_CHECK
#define gvxSTATUS_CHECK(status) \
    do \
    { \
        if (status != VX_SUCCESS) \
        { \
            printf("status ERROR: status=%d @ %s(%d)\n", status, __FUNCTION__, __LINE__); \
        } \
    } \
    while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif
