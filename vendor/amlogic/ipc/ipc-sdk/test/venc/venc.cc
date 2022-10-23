#include <gtest/gtest.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "aml_ipc_sdk.h"

#ifdef __cplusplus
} /* extern "C" */
#endif

class VencTest: public ::testing::Test{};
using VencDeathTest = VencTest;

class VencNewParamTest: public ::testing::TestWithParam<enum AmlVencCodec>{};
class VencNewInvalidInputParamTest: public ::testing::TestWithParam<enum AmlVencCodec>{};


INSTANTIATE_TEST_CASE_P(VencNew, VencNewParamTest, ::testing::Values(AML_VCODEC_H264,AML_VCODEC_H265));
INSTANTIATE_TEST_CASE_P(VencNew, VencNewInvalidInputParamTest, ::testing::Values(AML_VCODEC_NONE,
    AML_VCODEC_VP8,AML_VCODEC_H263));



TEST_P(VencNewParamTest, HandleVencNew){
    struct AmlIPCVenc *venctmp = NULL;
    enum AmlVencCodec vcodec = GetParam();
    venctmp = aml_ipc_venc_new(vcodec);
    EXPECT_NE(nullptr, venctmp);
    if ( venctmp ) {
        aml_obj_release(AML_OBJECT(venctmp));
        venctmp = NULL;
    }
}

TEST_P(VencNewInvalidInputParamTest, HandleVencNewInvalidInput){
    struct AmlIPCVenc *venctmp = NULL;
    enum AmlVencCodec vcodec = GetParam();
    venctmp = aml_ipc_venc_new(vcodec);
    EXPECT_EQ(nullptr, venctmp);
    if ( venctmp ) {
        aml_obj_release(AML_OBJECT(venctmp));
        venctmp = NULL;
    }
}

TEST_F(VencTest, VencStart){
    struct AmlIPCVenc *venctmp = NULL;
    venctmp = aml_ipc_venc_new(AML_VCODEC_H264);
    EXPECT_NE(nullptr, venctmp);
    if ( venctmp ) {
        AmlStatus ret = aml_ipc_start(AML_IPC_COMPONENT(venctmp));
        EXPECT_EQ(AML_STATUS_OK, ret);
        //if ( ret == AML_STATUS_OK ) {
        //    aml_ipc_stop(AML_IPC_COMPONENT(venctmp));
        //}
        aml_obj_release(AML_OBJECT(venctmp));
        venctmp = NULL;
    }
}

TEST_F(VencDeathTest, VencStartNullInput){
    EXPECT_DEATH(aml_ipc_start(nullptr), "");
}

/*TEST_F(VencTest, VencStop){
    struct AmlIPCVenc *venctmp = NULL;
    venctmp = aml_ipc_venc_new(AML_VCODEC_H264);
    EXPECT_NE(nullptr, venctmp);
    if ( venctmp ) {
        AmlStatus ret = aml_ipc_start(AML_IPC_COMPONENT(venctmp));
        EXPECT_EQ(AML_STATUS_OK, ret);
        if ( ret == AML_STATUS_OK ) {
            EXPECT_EQ(AML_STATUS_OK, aml_ipc_stop(AML_IPC_COMPONENT(venctmp)));
        }
        aml_obj_release(AML_OBJECT(venctmp));
        venctmp = NULL;
    }
}

TEST_F(VencDeathTest, VencStopNullInput){
    EXPECT_DEATH(aml_ipc_stop(nullptr), "");
}

TEST_F(VencTest, SetInputFmt){

}

TEST_F(VencTest, SetInputFmtNullInput){

}

TEST_F(VencDeathTest, SetInputFmtNullInput){

}

TEST_F(VencTest, SetBitrate){

}

TEST_F(VencTest, SetBitrateInvalidInput){

}

TEST_F(VencDeathTest, SetBitrateNullInput){

}

TEST_F(VencTest, SetGopSize){

}

TEST_F(VencTest, SetGopSizeInvalidInput){

}

TEST_F(VencDeathTest, SetGopSizeNullInput){

}

TEST_F(VencTest, ForceInsertIDR){

}

TEST_F(VencTest, ForceInsertIDRInvalidInput){

}

TEST_F(VencDeathTest, ForceInsertIDRNullInput){

}

TEST_F(VencTest, SetROI){

}

TEST_F(VencTest, SetROIInvalidInput){

}

TEST_F(VencDeathTest, SetROINullInput){

}

TEST_F(VencTest, SetQpparams){

}

TEST_F(VencTest, SetQpparamsInvalidInput){

}

TEST_F(VencDeathTest, SetQpparamsNullInput){

}

TEST_F(VencTest, GetStatus){

}

TEST_F(VencTest, GetStatusInvalidInput){

}

TEST_F(VencDeathTest, GetStatusNullInput){

}
*/

