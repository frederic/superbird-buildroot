#include <gtest/gtest.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "aml_ipc_sdk.h"

#ifdef __cplusplus
} /* extern "C" */
#endif

class Ge2dTest: public ::testing::Test{};
using Ge2dDeathTest = Ge2dTest;

class Ge2dNewParamTest: public ::testing::TestWithParam<enum AmlIPCGE2DOP>{};
class Ge2dNewInvalidInputParamTest: public ::testing::TestWithParam<enum AmlIPCGE2DOP>{};

INSTANTIATE_TEST_CASE_P(Ge2dNew, Ge2dNewParamTest, ::testing::Values(AML_GE2D_OP_FILLRECT,
    AML_GE2D_OP_BITBLT_TO,AML_GE2D_OP_BITBLT_FROM,AML_GE2D_OP_STRETCHBLT_TO,AML_GE2D_OP_STRETCHBLT_FROM,
    AML_GE2D_OP_ALPHABLEND_TO,AML_GE2D_OP_ALPHABLEND_FROM));
INSTANTIATE_TEST_CASE_P(Ge2dNewInvalidInput, Ge2dNewInvalidInputParamTest, ::testing::Values(AML_GE2D_OP_SDK_MIN,
    AML_GE2D_OP_SDK_MAX));


TEST_P(Ge2dNewParamTest, HandleGe2dNew){
    struct AmlIPCGE2D *ge2dtmp = NULL;
    enum AmlIPCGE2DOP op = GetParam();
    ge2dtmp = aml_ipc_ge2d_new(op);
    EXPECT_NE(nullptr, ge2dtmp);
    if ( ge2dtmp ) {
        aml_obj_release(AML_OBJECT(ge2dtmp));
        ge2dtmp = NULL;
    }
}

TEST_P(Ge2dNewInvalidInputParamTest, HandleGe2dNewInvalidInput){
    struct AmlIPCGE2D *ge2dtmp = NULL;
    enum AmlIPCGE2DOP op = GetParam();
    ge2dtmp = aml_ipc_ge2d_new(op);
    EXPECT_EQ(nullptr, ge2dtmp);
    if ( ge2dtmp ) {
        aml_obj_release(AML_OBJECT(ge2dtmp));
        ge2dtmp = NULL;
    }
}


class SetOpParamTest: public ::testing::TestWithParam<enum AmlIPCGE2DOP>{};

INSTANTIATE_TEST_CASE_P(SetOp, SetOpParamTest, ::testing::Values(AML_GE2D_OP_FILLRECT,
    AML_GE2D_OP_BITBLT_TO,AML_GE2D_OP_BITBLT_FROM,AML_GE2D_OP_STRETCHBLT_TO,AML_GE2D_OP_STRETCHBLT_FROM,
    AML_GE2D_OP_ALPHABLEND_TO,AML_GE2D_OP_ALPHABLEND_FROM));

TEST_P(SetOpParamTest, HandleSetOp){
    struct AmlIPCGE2D *ge2dtmp = aml_ipc_ge2d_new(AML_GE2D_OP_FILLRECT);
    EXPECT_NE(nullptr, ge2dtmp);
    if ( ge2dtmp ) {
        enum AmlIPCGE2DOP op = GetParam();
        EXPECT_EQ(AML_STATUS_OK, aml_ipc_ge2d_set_op(ge2dtmp, op));
        aml_obj_release(AML_OBJECT(ge2dtmp));
        ge2dtmp = NULL;
    }
}

TEST_F(Ge2dDeathTest, SetOpInvalidInput){
    struct AmlIPCGE2D *ge2dtmp = aml_ipc_ge2d_new(AML_GE2D_OP_FILLRECT);
    EXPECT_NE(nullptr, ge2dtmp);
    if ( ge2dtmp ) {
        EXPECT_DEATH(aml_ipc_ge2d_set_op(ge2dtmp,AML_GE2D_OP_SDK_MIN), "");
        EXPECT_DEATH(aml_ipc_ge2d_set_op(ge2dtmp,AML_GE2D_OP_SDK_MAX), "");
        aml_obj_release(AML_OBJECT(ge2dtmp));
        ge2dtmp = NULL;
    }
}

TEST_F(Ge2dDeathTest, SetOpNullInput){
    EXPECT_DEATH(aml_ipc_ge2d_set_op(nullptr,AML_GE2D_OP_FILLRECT), "");
}

TEST_F(Ge2dTest, SetStaticBuffer){
    struct AmlIPCGE2D *ge2dtmp = aml_ipc_ge2d_new(AML_GE2D_OP_FILLRECT);
    EXPECT_NE(nullptr, ge2dtmp);
    if ( ge2dtmp ) {
        struct AmlIPCFrame *frame = AML_OBJ_NEW(AmlIPCFrame);
        EXPECT_EQ(AML_STATUS_OK, aml_ipc_ge2d_set_static_buffer(ge2dtmp, frame));
        aml_obj_release(AML_OBJECT(ge2dtmp));
        ge2dtmp = NULL;
    }
}

TEST_F(Ge2dTest, SetStaticBufferNullInput){
    struct AmlIPCGE2D *ge2dtmp = aml_ipc_ge2d_new(AML_GE2D_OP_FILLRECT);
    EXPECT_NE(nullptr, ge2dtmp);
    if ( ge2dtmp ) {
        EXPECT_NE(AML_STATUS_OK, aml_ipc_ge2d_set_static_buffer(ge2dtmp, nullptr));
        aml_obj_release(AML_OBJECT(ge2dtmp));
        ge2dtmp = NULL;
    }
}

TEST_F(Ge2dDeathTest, SetStaticBufferNullInput){
    EXPECT_DEATH(aml_ipc_ge2d_set_static_buffer(nullptr,nullptr), "");
}

class GetImageSizeParamTest: public ::testing::TestWithParam<enum AmlPixelFormat>{};

INSTANTIATE_TEST_CASE_P(GetImageSize, GetImageSizeParamTest, ::testing::Values(AML_PIXFMT_RGB_565,
    AML_PIXFMT_RGB_888,AML_PIXFMT_BGR_888,AML_PIXFMT_RGBA_8888,AML_PIXFMT_BGRA_8888,AML_PIXFMT_RGBX_8888,
    AML_PIXFMT_YU12,AML_PIXFMT_YV12,AML_PIXFMT_NV12,AML_PIXFMT_NV21));

TEST_P(GetImageSizeParamTest, HandleGetImageSize){
    enum AmlPixelFormat pixfmt = GetParam();
    EXPECT_NE(0, aml_ipc_ge2d_get_image_size(pixfmt,1920,1080, NULL));
}

TEST_F(Ge2dTest, GetImageSizeInvalidInput){
    EXPECT_EQ(0, aml_ipc_ge2d_get_image_size(AML_PIXFMT_SDK_MAX,1920,1080, NULL));
}

TEST_F(Ge2dTest, GetImageSizeInvalidWH){
    EXPECT_EQ(0, aml_ipc_ge2d_get_image_size(AML_PIXFMT_NV12,3840,2160, NULL));
}

TEST_F(Ge2dTest, OsdFrameNewAlloc){
    struct AmlIPCVideoFormat vfmt = {AML_PIXFMT_NV12,1920,1080,30};
    EXPECT_NE(nullptr, aml_ipc_osd_frame_new_alloc(&vfmt));
}

TEST_F(Ge2dTest, OsdFrameNewAllocInvalidInput){
    struct AmlIPCVideoFormat vfmt1 = {AML_PIXFMT_SDK_MAX,1920,1080,30};
    EXPECT_EQ(nullptr, aml_ipc_osd_frame_new_alloc(&vfmt1));
    struct AmlIPCVideoFormat vfmt2 = {AML_PIXFMT_NV12,3840,2160,30};
    EXPECT_EQ(nullptr, aml_ipc_osd_frame_new_alloc(&vfmt2));
}

TEST_F(Ge2dDeathTest, OsdFrameNewAllocNullInput){
    EXPECT_DEATH(aml_ipc_osd_frame_new_alloc(nullptr), "");
}

TEST_F(Ge2dTest, OsdFrameNewFillRect){
    struct AmlRect rc = {0,0,300,200};
    EXPECT_NE(nullptr, aml_ipc_osd_frame_new_fill_rect(&rc,0xff000000));
}

TEST_F(Ge2dDeathTest, OsdFrameNewFillRectNullInput){
    EXPECT_DEATH(aml_ipc_osd_frame_new_fill_rect(nullptr,0xff000000), "");
}

TEST_F(Ge2dTest, Process){
    struct AmlIPCGE2D *ge2dtmp = NULL;
    ge2dtmp = aml_ipc_ge2d_new(AML_GE2D_OP_FILLRECT);
    EXPECT_NE(nullptr, ge2dtmp);
    if ( ge2dtmp ) {
        struct AmlIPCVideoFormat vfmt = {AML_PIXFMT_NV12,1280,720,30};
        struct AmlIPCOSDFrame *fill0 = aml_ipc_osd_frame_new_alloc(&vfmt);
        struct AmlRect rc1 = {0,0,300,200};
        struct AmlIPCOSDFrame *fill1 = aml_ipc_osd_frame_new_fill_rect(&rc1, 0xff000000);
        aml_ipc_ge2d_set_static_buffer(ge2dtmp, (struct AmlIPCFrame*)fill1);
        EXPECT_EQ(AML_STATUS_OK, aml_ipc_ge2d_process(ge2dtmp,(struct AmlIPCFrame*)fill0,(struct AmlIPCFrame*)fill1));
        aml_obj_release(AML_OBJECT(fill0));
        fill0 = NULL;
        aml_obj_release(AML_OBJECT(ge2dtmp));
        ge2dtmp = NULL;
    }
}

TEST_F(Ge2dTest, ProcessNullInput){
    struct AmlIPCGE2D *ge2dtmp = NULL;
    ge2dtmp = aml_ipc_ge2d_new(AML_GE2D_OP_FILLRECT);
    EXPECT_NE(nullptr, ge2dtmp);
    if ( ge2dtmp ) {
        EXPECT_EQ(AML_STATUS_OK, aml_ipc_ge2d_process(ge2dtmp,nullptr,nullptr));
        aml_obj_release(AML_OBJECT(ge2dtmp));
        ge2dtmp = NULL;
    }
}

TEST_F(Ge2dDeathTest, ProcessInvalidInput){
    struct AmlIPCGE2D *ge2dtmp = NULL;
    ge2dtmp = aml_ipc_ge2d_new(AML_GE2D_OP_FILLRECT);
    EXPECT_NE(nullptr, ge2dtmp);
    if ( ge2dtmp ) {
        struct AmlRect rc1 = {0,0,300,200};
        struct AmlIPCOSDFrame *fill1 = aml_ipc_osd_frame_new_fill_rect(&rc1, 0xff000000);
        aml_ipc_ge2d_set_static_buffer(ge2dtmp, (struct AmlIPCFrame*)fill1);
        EXPECT_DEATH(aml_ipc_ge2d_process(ge2dtmp,nullptr,(struct AmlIPCFrame*)fill1), "");
        /*struct AmlIPCAudioFrame *aframe = AML_OBJ_NEW(AmlIPCAudioFrame);
        struct AmlIPCAudioFormat afmt = {AML_ACODEC_PCM_S16LE,48000,16,2};
        aframe->format = afmt;
        EXPECT_EXIT(aml_ipc_ge2d_process(ge2dtmp,(struct AmlIPCFrame*)aframe, (struct AmlIPCFrame*)fill1), "");
        aml_obj_release(AML_OBJECT(aframe));
        aframe = NULL;*/
    }
}

TEST_F(Ge2dDeathTest, ProcessNullInput){
    struct AmlRect rc1 = {0,0,300,200};
    struct AmlIPCOSDFrame *fill1 = aml_ipc_osd_frame_new_fill_rect(&rc1, 0xff000000);
    EXPECT_DEATH(aml_ipc_ge2d_process(nullptr,nullptr,(struct AmlIPCFrame*)fill1), "");
}


TEST_F(Ge2dTest, SetRect){
    struct AmlRect rc1 = {0,0,300,200};
    struct AmlIPCOSDFrame *fill1 = aml_ipc_osd_frame_new_fill_rect(&rc1, 0xff000000);
    struct AmlRect rc2 = {0,0,60,40};
    EXPECT_EQ(AML_STATUS_OK, aml_ipc_osd_frame_set_rect(fill1,&rc2,nullptr));
    if ( fill1 ) {
        aml_obj_release(AML_OBJECT(fill1));
        fill1 = NULL;
    }
}

TEST_F(Ge2dTest, SetRectNullInput){
    struct AmlRect rc1 = {0,0,300,200};
    struct AmlIPCOSDFrame *fill1 = aml_ipc_osd_frame_new_fill_rect(&rc1, 0xff000000);
    EXPECT_NE(AML_STATUS_OK, aml_ipc_osd_frame_set_rect(fill1,nullptr,nullptr));
    if ( fill1 ) {
        aml_obj_release(AML_OBJECT(fill1));
        fill1 = NULL;
    }
}

TEST_F(Ge2dDeathTest, SetRectNullInput){
    struct AmlRect rc0 = {0,0,600,400};
    struct AmlRect rc1 = {0,0,300,200};
    EXPECT_DEATH(aml_ipc_osd_frame_set_rect(nullptr,&rc0,&rc1), "");
}

TEST_F(Ge2dTest, SetColor){
    struct AmlRect rc1 = {0,0,300,200};
    struct AmlIPCOSDFrame *fill1 = aml_ipc_osd_frame_new_fill_rect(&rc1, 0xff000000);
    EXPECT_EQ(AML_STATUS_OK, aml_ipc_osd_frame_set_color(fill1,0xff000000));
    if ( fill1 ) {
        aml_obj_release(AML_OBJECT(fill1));
        fill1 = NULL;
    }
}

TEST_F(Ge2dDeathTest, SetColorNullInput){
    EXPECT_DEATH(aml_ipc_osd_frame_set_color(nullptr,0xff000000), "");
}

