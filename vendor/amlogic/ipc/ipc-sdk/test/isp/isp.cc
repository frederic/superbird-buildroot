#include <gtest/gtest.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "aml_ipc_sdk.h"

#ifdef __cplusplus
} /* extern "C" */
#endif

class IspTest: public ::testing::Test{};
using IspDeathTest = IspTest;


TEST_F(IspTest, IspNew){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, IspNewInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new(nullptr,0,0,0);
    EXPECT_EQ(nullptr, isptmp);
    if ( isptmp ) {
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
    isptmp = aml_ipc_isp_new("/dev/video0",-1,-1,-1);
    EXPECT_EQ(nullptr, isptmp);
    if ( isptmp ) {
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, IspOpen){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, IspOpenInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetCapFmt){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        struct AmlIPCVideoFormat vfmt = {AML_PIXFMT_NV12,1920,1080,30};
        AmlStatus ret = aml_ipc_isp_set_capture_format(isptmp,AML_ISP_FR,&vfmt);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetCapFmtInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        struct AmlIPCVideoFormat vfmt = {AML_PIXFMT_NV12,1920,1080,30};
        AmlStatus ret = aml_ipc_isp_set_capture_format(isptmp,AML_ISP_DS2,&vfmt);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_capture_format(isptmp,AML_ISP_META,&vfmt);
        EXPECT_NE(AML_STATUS_OK, ret);
        struct AmlIPCVideoFormat vfmt1 = {AML_PIXFMT_RGBX_8888,3840,2160,60};
        ret = aml_ipc_isp_set_capture_format(isptmp,AML_ISP_FR,&vfmt1);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, SetCapFmtNullInput){
    EXPECT_DEATH(aml_ipc_isp_set_capture_format(nullptr,AML_ISP_SDK_MAX,nullptr), "");
}

TEST_F(IspTest, SetPicParams){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_picture_params(isptmp,AML_ISP_FR,128,128,128,160,128);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetPicParamsInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_picture_params(isptmp,AML_ISP_DS1,128,128,128,160,128);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_picture_params(isptmp,AML_ISP_FR,256,256,256,361,256);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_picture_params(isptmp,AML_ISP_FR,-1,-1,-1,-1,-1);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, SetPicParamsNullInput){
    EXPECT_DEATH(aml_ipc_isp_set_picture_params(nullptr,AML_ISP_SDK_MAX,256,256,256,361,256), "");
}

TEST_F(IspTest, SetWdrMode){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_WDR_mode(isptmp,AML_WDR_MODE_ENABLE);
        EXPECT_EQ(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_WDR_mode(isptmp,AML_WDR_MODE_DISABLE);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetWdrModeInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_WDR_mode(isptmp,AML_WDR_MODE_SDK_MAX);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, SetWdrModeNullInput){
    EXPECT_DEATH(aml_ipc_isp_set_WDR_mode(nullptr,AML_WDR_MODE_SDK_MAX), "");
}

TEST_F(IspTest, GetWdrMode){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        enum AmlWDRMode mode;
        AmlStatus ret = aml_ipc_isp_get_WDR_mode(isptmp,&mode);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, GetWdrModeInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_get_WDR_mode(isptmp,nullptr);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, GetWdrModeNullInput){
    EXPECT_DEATH(aml_ipc_isp_get_WDR_mode(nullptr,nullptr), "");
}

TEST_F(IspTest, SetWbGain){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_WB_gain(isptmp,AML_WB_GAIN_AUTO,AML_WB_GAIN_AUTO);
        EXPECT_EQ(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_WB_gain(isptmp,1000,1000);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetWbGainInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_WB_gain(isptmp,AML_WB_GAIN_AUTO,1000);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_WB_gain(isptmp,1000,AML_WB_GAIN_AUTO);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_WB_gain(isptmp,-100,65536);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, SetWbGainNullInput){
    EXPECT_DEATH(aml_ipc_isp_set_WB_gain(nullptr,65536,65536), "");
}

TEST_F(IspTest, GetWbGain){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        int rgain, bgain;
        AmlStatus ret = aml_ipc_isp_get_WB_gain(isptmp,&rgain,&bgain);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, GetWbGainInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        int rgain, bgain;
        AmlStatus ret = aml_ipc_isp_get_WB_gain(isptmp,&rgain,nullptr);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_get_WB_gain(isptmp,nullptr,&bgain);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, GetWbGainNullInput){
    EXPECT_DEATH(aml_ipc_isp_get_WB_gain(nullptr,nullptr,nullptr), "");
}

TEST_F(IspTest, SetIrcut){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_ircut(isptmp,true);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetIrcutInvalidInput){
    AmlStatus ret = aml_ipc_isp_set_ircut(nullptr,true);
    EXPECT_NE(AML_STATUS_OK, ret);
}

TEST_F(IspDeathTest, SetIrcutNullInput){
    EXPECT_DEATH(aml_ipc_isp_set_ircut(nullptr,true), "");
}

TEST_F(IspTest, GetIrcut){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        int ircut;
        AmlStatus ret = aml_ipc_isp_get_ircut(isptmp,&ircut);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, GetIrcutInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_get_ircut(isptmp,nullptr);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, GetIrcutNullInput){
    EXPECT_DEATH(aml_ipc_isp_get_ircut(nullptr,nullptr), "");
}

TEST_F(IspTest, SetExposure){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_exposure(isptmp,33);
        EXPECT_EQ(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_exposure(isptmp,-1);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetExposureInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_exposure(isptmp,0);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_exposure(isptmp,1001);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, SetExposureNullInput){
    EXPECT_DEATH(aml_ipc_isp_set_exposure(nullptr,0), "");
}

TEST_F(IspTest, GetExposure){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        int exposure;
        AmlStatus ret = aml_ipc_isp_get_exposure(isptmp,&exposure);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, GetExposureInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_get_exposure(isptmp,nullptr);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, GetExposureNullInput){
    EXPECT_DEATH(aml_ipc_isp_get_exposure(nullptr,nullptr), "");
}

TEST_F(IspTest, SetSharpness){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_sharpness(isptmp,128);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetSharpnessInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_sharpness(isptmp,-1);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_sharpness(isptmp,256);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, SetSharpnessNullInput){
    EXPECT_DEATH(aml_ipc_isp_set_sharpness(nullptr,-1), "");
}

TEST_F(IspTest, GetSharpness){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        int sharpness;
        AmlStatus ret = aml_ipc_isp_get_sharpness(isptmp,&sharpness);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, GetSharpnessInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_get_sharpness(isptmp,nullptr);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, GetSharpnessNullInput){
    EXPECT_DEATH(aml_ipc_isp_get_sharpness(nullptr,nullptr), "");
}

TEST_F(IspTest, SetBrightness){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_brightness(isptmp,128);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetBrightnessInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_brightness(isptmp,-1);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_brightness(isptmp,256);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, SetBrightnessNullInput){
    EXPECT_DEATH(aml_ipc_isp_set_brightness(nullptr,-1), "");
}

TEST_F(IspTest, GetBrightness){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        int brightness;
        AmlStatus ret = aml_ipc_isp_get_brightness(isptmp,&brightness);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, GetBrightnessInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_get_brightness(isptmp,nullptr);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, GetBrightnessNullInput){
    EXPECT_DEATH(aml_ipc_isp_get_brightness(nullptr,nullptr), "");
}

TEST_F(IspTest, SetContrast){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_contrast(isptmp,128);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetContrastInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_contrast(isptmp,-1);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_contrast(isptmp,256);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, SetContrastNullInput){
    EXPECT_DEATH(aml_ipc_isp_set_contrast(nullptr,-1), "");
}

TEST_F(IspTest, GetContrast){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        int contrast;
        AmlStatus ret = aml_ipc_isp_get_contrast(isptmp,&contrast);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, GetContrastInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_get_contrast(isptmp,nullptr);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, GetContrastNullInput){
    EXPECT_DEATH(aml_ipc_isp_get_contrast(nullptr,nullptr), "");
}

TEST_F(IspTest, SetHue){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_hue(isptmp,180);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetHueInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_hue(isptmp,-1);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_hue(isptmp,361);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, SetHueNullInput){
    EXPECT_DEATH(aml_ipc_isp_set_hue(nullptr,-1), "");
}

TEST_F(IspTest, GetHue){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        int hue;
        AmlStatus ret = aml_ipc_isp_get_hue(isptmp,&hue);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, GetHueInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_get_hue(isptmp,nullptr);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, GetHueNullInput){
    EXPECT_DEATH(aml_ipc_isp_get_hue(nullptr,nullptr), "");
}

TEST_F(IspTest, SetSaturation){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_saturation(isptmp,128);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetSaturationInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_saturation(isptmp,-1);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_saturation(isptmp,256);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, SetSaturationNullInput){
    EXPECT_DEATH(aml_ipc_isp_set_saturation(nullptr,-1), "");
}

TEST_F(IspTest, GetSaturation){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        int saturation;
        AmlStatus ret = aml_ipc_isp_get_saturation(isptmp,&saturation);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, GetSaturationInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_get_saturation(isptmp,nullptr);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, GetSaturationNullInput){
    EXPECT_DEATH(aml_ipc_isp_get_saturation(nullptr,nullptr), "");
}

TEST_F(IspTest, SetHflip){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_hflip(isptmp,true);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetHflipInvalidInput){
    AmlStatus ret = aml_ipc_isp_set_hflip(nullptr,true);
    EXPECT_NE(AML_STATUS_OK, ret);
}

TEST_F(IspDeathTest, SetHflipNullInput){
    EXPECT_DEATH(aml_ipc_isp_set_hflip(nullptr,true), "");
}

TEST_F(IspTest, GetHflip){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        bool hflip = false;
        AmlStatus ret = aml_ipc_isp_get_hflip(isptmp,&hflip);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, GetHflipInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_get_hflip(isptmp,nullptr);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, GetHflipNullInput){
    EXPECT_DEATH(aml_ipc_isp_get_hflip(nullptr,nullptr), "");
}

TEST_F(IspTest, SetVflip){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_vflip(isptmp,true);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetVflipInvalidInput){
    AmlStatus ret = aml_ipc_isp_set_vflip(nullptr,true);
    EXPECT_NE(AML_STATUS_OK, ret);
}

TEST_F(IspDeathTest, SetVflipNullInput){
    EXPECT_DEATH(aml_ipc_isp_set_vflip(nullptr,true), "");
}

TEST_F(IspTest, GetVflip){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        bool vflip = false;
        AmlStatus ret = aml_ipc_isp_get_vflip(isptmp,&vflip);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, GetVflipInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_get_vflip(isptmp,nullptr);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, GetVflipNullInput){
    EXPECT_DEATH(aml_ipc_isp_get_vflip(nullptr,nullptr), "");
}

TEST_F(IspTest, SetSensorFps){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_set_sensor_fps(isptmp,0);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetSensorFpsInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_set_sensor_fps(isptmp,-1);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_set_sensor_fps(isptmp,121);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, SetSensorFpsNullInput){
    EXPECT_DEATH(aml_ipc_set_sensor_fps(nullptr,-1), "");
}

TEST_F(IspTest, GetSensorFps){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        int sensor_fps;
        AmlStatus ret = aml_ipc_get_sensor_fps(isptmp,&sensor_fps);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, GetSensorFpsInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_get_sensor_fps(isptmp,nullptr);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, GetSensorFpsNullInput){
    EXPECT_DEATH(aml_ipc_get_sensor_fps(nullptr,nullptr), "");
}

TEST_F(IspTest, SetFps){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_fps(isptmp,AML_ISP_FR,0);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetFpsInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_fps(isptmp,AML_ISP_FR,-1);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_fps(isptmp,AML_ISP_FR,121);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_fps(isptmp,AML_ISP_DS1,0);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, SetFpsNullInput){
    EXPECT_DEATH(aml_ipc_isp_set_fps(nullptr,AML_ISP_SDK_MAX,-1), "");
}

TEST_F(IspTest, GetFps){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        int fps;
        AmlStatus ret = aml_ipc_isp_get_fps(isptmp,AML_ISP_FR,&fps);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, GetFpsInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_get_fps(isptmp,AML_ISP_FR,nullptr);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_get_fps(isptmp,AML_ISP_DS2,nullptr);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, GetFpsNullInput){
    EXPECT_DEATH(aml_ipc_isp_get_fps(nullptr,AML_ISP_SDK_MAX,nullptr), "");
}

TEST_F(IspTest, SetCompensation){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_compensation(isptmp,0);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetCompensationInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_compensation(isptmp,-1);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_compensation(isptmp,256);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, SetCompensationNullInput){
    EXPECT_DEATH(aml_ipc_isp_set_compensation(nullptr,-1), "");
}

TEST_F(IspTest, GetCompensation){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        int compensation;
        AmlStatus ret = aml_ipc_isp_get_compensation(isptmp,&compensation);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, GetCompensationInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_get_compensation(isptmp,nullptr);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, GetCompensationNullInput){
    EXPECT_DEATH(aml_ipc_isp_get_compensation(nullptr,nullptr), "");
}

TEST_F(IspTest, SetNr){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_nr(isptmp,0,0);
        EXPECT_EQ(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_nr(isptmp,-1,-1);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, SetNrInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_set_nr(isptmp,-2,-2);
        EXPECT_NE(AML_STATUS_OK, ret);
        ret = aml_ipc_isp_set_nr(isptmp,256,256);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, SetNrNullInput){
    EXPECT_DEATH(aml_ipc_isp_set_nr(nullptr,256,256), "");
}

TEST_F(IspTest, GetNr){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        int spacezone,timezone;
        AmlStatus ret = aml_ipc_isp_get_nr(isptmp,&spacezone,&timezone);
        EXPECT_EQ(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspTest, GetNrInvalidInput){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        AmlStatus ret = aml_ipc_isp_get_nr(isptmp,nullptr,nullptr);
        EXPECT_NE(AML_STATUS_OK, ret);
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, GetNrNullInput){
    EXPECT_DEATH(aml_ipc_isp_get_nr(nullptr,nullptr,nullptr), "");
}

TEST_F(IspTest, IspStart){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        struct AmlIPCVideoFormat vfmt = {AML_PIXFMT_NV12,1920,1080,30};
        AmlStatus ret = aml_ipc_isp_set_capture_format(isptmp,AML_ISP_FR,&vfmt);
        EXPECT_EQ(AML_STATUS_OK, ret);
        if ( ret == AML_STATUS_OK ) {
            ret = aml_ipc_start(AML_IPC_COMPONENT(isptmp));
            EXPECT_EQ(AML_STATUS_OK, ret);
            if ( ret == AML_STATUS_OK ) {
                aml_ipc_stop(AML_IPC_COMPONENT(isptmp));
            }
        }
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, IspStartNullInput){
    EXPECT_DEATH(aml_ipc_start(nullptr), "");
}

TEST_F(IspTest, IspStop){
    struct AmlIPCISP *isptmp = aml_ipc_isp_new("/dev/video0",2,0,0);
    EXPECT_NE(nullptr, isptmp);
    if ( isptmp ) {
        struct AmlIPCVideoFormat vfmt = {AML_PIXFMT_NV12,1920,1080,30};
        AmlStatus ret = aml_ipc_isp_set_capture_format(isptmp,AML_ISP_FR,&vfmt);
        EXPECT_EQ(AML_STATUS_OK, ret);
        if ( ret == AML_STATUS_OK ) {
            ret = aml_ipc_start(AML_IPC_COMPONENT(isptmp));
            EXPECT_EQ(AML_STATUS_OK, ret);
            if ( ret == AML_STATUS_OK ) {
                ret = aml_ipc_stop(AML_IPC_COMPONENT(isptmp));
                EXPECT_EQ(AML_STATUS_OK, ret);
            }
        }
        aml_obj_release(AML_OBJECT(isptmp));
        isptmp = NULL;
    }
}

TEST_F(IspDeathTest, IspStopNullInput){
    EXPECT_DEATH(aml_ipc_stop(nullptr), "");
}

