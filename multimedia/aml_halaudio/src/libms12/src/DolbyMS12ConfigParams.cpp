/*
 * Copyright (C) 2017 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "libms12"
// #define LOG_NDEBUG 0
// #define LOG_NALOGV 0

#include "log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "audio.h"
#include <utils/String8.h>
#include <errno.h>
#include <fcntl.h>
#include <media/AudioSystem.h>

#include "DolbyMS12ConfigParams.h"


namespace android
{

#define MAX_ARGC 100
#define MAX_ARGV_STRING_LEN 256

//here the file path is fake
//@@pcm [application sounds]
#define DEFAULT_APPLICATION_PCM_FILE_NAME "/data/application"
//@@pcm [system sounds]
#define DEFAULT_SYSTEM_PCM_FILE_NAME "/data/system48kHz.wav"
//@@pcm [ott sounds]
#define DEFAULT_OTT_PCM_FILE_NAME "/data/ott48kHz.wav"
//@@HE-AAC input file
#define DEFAULT_MAIN_HEAAC_V1_FILE_NAME "/data/main.loas"
#define DEFAULT_ASSOCIATE_HEAAC_V1_FILE_NAME "/data/associate.loas"
#define DEFAULT_MAIN_HEAAC_V2_FILE_NAME "/data/main.adts"
#define DEFAULT_ASSOCIATE_HEAAC_V2_FILE_NAME "/data/associate.adts"

//@@@DDPlus input file
#define DEFAULT_MAIN_DDP_FILE_NAME "/data/main.ac3"
#define DEFAULT_ASSOCIATE_DDP_FILE_NAME "/data/associate.ac3"

#define DEFAULT_OUTPUT_PCM_MULTI_FILE_NAME "/data/outputmulti.wav"
#define DEFAULT_OUTPUT_PCM_DOWNMIX_FILE_NAME "/data/outputdownmix.wav"
#define DEFAULT_OUTPUT_DD_FILE_NAME "/data/output.ac3"
#define DEFAULT_OUTPUT_DDP_FILE_NAME "/data/output.ec3"

#define DEFAULT_APP_SOUNDS_CHANNEL_cONFIGURATION 7//means 3/2 (L, R, C, l, r)
#define DEFAULT_SYSTEM_SOUNDS_CHANNEL_cONFIGURATION 2//means 2/0 (L, R)

//DRC Mode
#define DDPI_UDC_COMP_LINE 2
#define DRC_MODE_BIT  0
#define DRC_HIGH_CUT_BIT 3
#define DRC_LOW_BST_BIT 16


DolbyMS12ConfigParams::DolbyMS12ConfigParams():
    // mDolbyMS12GetOutProfile(NULL)
    // ,
    mParamNum(0)
    , mAudioOutFlags(AUDIO_OUTPUT_FLAG_NONE)
    , mAudioStreamOutFormat(AUDIO_FORMAT_PCM_16_BIT)
    , mAudioStreamOutChannelMask(AUDIO_CHANNEL_OUT_STEREO)
    , mAudioSteamOutSampleRate(48000)
    // , mAudioSteamOutDevices(AUDIO_DEVICE_OUT_SPEAKER)
    , mDolbyMS12OutFormat(AUDIO_FORMAT_AC3)
    , mDolbyMS12OutSampleRate(48000)
    , mDolbyMS12OutChannelMask(AUDIO_CHANNEL_OUT_STEREO)
    , mConfigParams(NULL)
    , mStereoOutputFlag(false)
    // , mMultiOutputFlag(true)
    , mDRCBoost(100)
    , mDRCCut(100)
    , mDRCBoostSystem(100)
    , mDRCCutSystem(100)
    , mChannelConfAppSoundsIn(7)//5.1
    , mChannelConfSystemIn(2)//2.0
    , mMainFlags(true)
    //, mMainFlags(false) // always have mMainFlags on? zz
    , mAppSoundFlags(false)
    , mSystemSoundFlags(false)
    , mDAPInitMode(0)
    , mDAPVirtualBassEnable(0)
    , mDBGOut(0)
    , mDRCModesOfDownmixedOutput(0)
    , mDAPDRCMode(0)
    , mDonwmixMode(0)
    , mEvaluationMode(0)
    , mLFEPresentInAppSoundIn(1)
    , mLFEPresentInSystemSoundIn(0)
    , mMaxChannels(6)//fixme, here choose 5.1ch
    , mDonwnmix71PCMto51(0)
    , mLockingChannelModeENC(1)//Encoder Channel Mode Locking Mode as 5.1
    , mRISCPrecisionFlag(1)
    , mDualMonoReproMode(0)
    , mVerbosity(2)
    , mOutputBitDepth(16)//use 16 bit per sample
    , mAssociatedAudioMixing(1)
    , mSystemAPPAudioMixing(1)
    , mUserControlVal(0)

    //DDPLUS SWITCHES
    , mDdplusAssocSubstream(1)//range 1~3, here choose 1
    , mChannelConfigInExtPCMInput(7)
    , mLFEPresentInExtPCMInput(true)
    , mCompressorProfile(0)

    //HE-AAC SWITCHES
    , mAssocInstanse(2)
    , mDefDialnormVal(108)
    , mTransportFormat(0)

    //DAP SWITCHES (device specific)
    , mDAPCalibrationBoost(0)
    , mDAPDMX(0)
    , mDAPGains(0)
    , mDAPSurDecEnable(false)
    , mHasAssociateInput(false)
    , mHasSystemInput(false)
    , mDualOutputFlag(false)
    , mActivateOTTSignal(false)
    , mChannelConfOTTSoundsIn(2)//2.0 if mActivateOTTSignal is true
    , mLFEPresentInOTTSoundIn(0)//on(default) if mActivateOTTSignal is true
    , mAtmosLock(false)//off(default) if mActivateOTTSignal is true
    , mPause(false)//Unpause(default) if mActivateOTTSignal is true
    , mMain1IsDummy(false)
    , mOTTSoundInputEnable(false)
{
    ALOGD("+%s() mAudioOutFlags %d mAudioStreamOutFormat %#x mAudioStreamOutChannelMask %#x mHasAssociateInput %d mHasSystemInput %d\n",
          __FUNCTION__, mAudioOutFlags, mAudioStreamOutFormat, mAudioStreamOutChannelMask, mHasAssociateInput, mHasSystemInput);
    mConfigParams = PrepareConfigParams(MAX_ARGC, MAX_ARGV_STRING_LEN);
    if (!mConfigParams) {
        ALOGD("%s() line %d prepare the array fail", __FUNCTION__, __LINE__);
    }
    memset(mDolbyMain1FileName, 0, sizeof(mDolbyMain1FileName));
    memcpy(mDolbyMain1FileName, DEFAULT_MAIN_DDP_FILE_NAME, sizeof(DEFAULT_MAIN_DDP_FILE_NAME));
    memset(mDolbyMain2FileName, 0, sizeof(mDolbyMain2FileName));
    memcpy(mDolbyMain2FileName, DEFAULT_DUMMY_DDP_FILE_NAME, sizeof(DEFAULT_DUMMY_DDP_FILE_NAME));
    char params_bin[] = "ms12_exec";
    sprintf(mConfigParams[mParamNum++], "%s", params_bin);
    ALOGD("-%s() main1 %s main2 %s", __FUNCTION__, mDolbyMain1FileName, mDolbyMain2FileName);
}

DolbyMS12ConfigParams::~DolbyMS12ConfigParams()
{
    ALOGD("+%s()", __FUNCTION__);
    CleanupConfigParams(mConfigParams, MAX_ARGC);
    ALOGD("-%s()", __FUNCTION__);
}


void DolbyMS12ConfigParams::SetAudioStreamOutParams(
    audio_output_flags_t flags
    , audio_format_t input_format
    , audio_channel_mask_t channel_mask
    , int sample_rate
    , audio_format_t output_format)
{
    ALOGD("+%s()", __FUNCTION__);
    mAudioOutFlags = flags;
    mAudioStreamOutFormat = input_format;
    mAudioStreamOutChannelMask = channel_mask;
    if ((input_format == AUDIO_FORMAT_PCM_16_BIT) || (input_format == AUDIO_FORMAT_PCM_32_BIT)) {
        if (mHasSystemInput != false) {
            mChannelConfAppSoundsIn = APPSoundChannelMaskConvertToChannelConfiguration(mAudioStreamOutChannelMask);
        } else {
            if (mActivateOTTSignal == true) {
                mChannelConfOTTSoundsIn = SystemSoundChannelMaskConvertToChannelConfiguration(mAudioStreamOutChannelMask);
            } else {
                mChannelConfSystemIn = SystemSoundChannelMaskConvertToChannelConfiguration(mAudioStreamOutChannelMask);
            }
        }
    }

    mAudioSteamOutSampleRate = sample_rate;
    mDolbyMS12OutFormat = output_format;
    ALOGD("-%s() AudioStreamOut Flags %x Format %#x ChannelMask %x SampleRate %d OutputFormat %#x\n",
          __FUNCTION__, mAudioOutFlags, mAudioStreamOutFormat, mAudioStreamOutChannelMask,
          mAudioSteamOutSampleRate, mDolbyMS12OutFormat);
}

bool DolbyMS12ConfigParams::SetDolbyMS12ParamsbyOutProfile(audio_policy_forced_cfg_t forceUse __unused)
{
    ALOGD("+%s()", __FUNCTION__);

    if (mDolbyMS12OutChannelMask == AUDIO_CHANNEL_OUT_7POINT1) {
        mStereoOutputFlag = false;
    } else if (mDolbyMS12OutChannelMask == AUDIO_CHANNEL_OUT_5POINT1) {
        mStereoOutputFlag = false;
    } else { //AUDIO_CHANNEL_OUT_STEREO
        mStereoOutputFlag = true;
    }
    if (mAudioStreamOutChannelMask == AUDIO_CHANNEL_OUT_7POINT1) {
        mMaxChannels = 8;
    } else {
        mMaxChannels = 6;
    }
    ALOGD("%s() mMaxChannels %x mStereoOutputFlag %#x\n", __FUNCTION__, mMaxChannels, mStereoOutputFlag);
    ALOGD("%s() mAudioStreamOutFormat %#x mDolbyMS12OutFormat %#x\n",
          __FUNCTION__, mAudioStreamOutFormat, mDolbyMS12OutFormat);

    //Todo, if DAP is enable, here need to modify!!!
    /*
        if ((mAudioStreamOutFormat == mDolbyMS12OutFormat) && \
                (mHasAssociateInput == false) && \
                (mHasSystemInput == false) && \
                (mAudioStreamOutChannelMask == mDolbyMS12OutChannelMask) && \
                (mAudioSteamOutSampleRate == mDolbyMS12OutSampleRate)) {
            ALOGD("-%s() dolbyms12 in/out Format/channelMask/SampleRate is same, bypass the audio!\n", __FUNCTION__);
            return false;//do not use dolbyms12
        }
        else {
            ALOGD("-%s() dolbyms12 in format is differ with out format!\n", __FUNCTION__);
            return true;
        }
    */
    ALOGD("-%s() Enable dolbyms12!\n", __FUNCTION__);
    return true;
}

//input and output
int DolbyMS12ConfigParams::SetInputOutputFileName(char **ConfigParams, int *row_index)
{
    ALOGV("+%s() line %d\n", __FUNCTION__, __LINE__);

    if (mActivateOTTSignal == false) {
        if (mHasAssociateInput == false) {
            sprintf(ConfigParams[*row_index], "%s", "-im");
            (*row_index)++;
            if ((mAudioStreamOutFormat == AUDIO_FORMAT_AC3) || (mAudioStreamOutFormat == AUDIO_FORMAT_E_AC3)) {
                sprintf(ConfigParams[*row_index], "%s", DEFAULT_MAIN_DDP_FILE_NAME);
                (*row_index)++;
                mMainFlags = true;
                mAppSoundFlags = false;
                mSystemSoundFlags = false;
            } else if ((mAudioStreamOutFormat == AUDIO_FORMAT_AAC) || (mAudioStreamOutFormat == AUDIO_FORMAT_HE_AAC_V1)) {
                //fixme, which he-aac format is allowed to this flow.
                sprintf(ConfigParams[*row_index], "%s", DEFAULT_MAIN_HEAAC_V1_FILE_NAME);
                (*row_index)++;
                mMainFlags = true;
                mAppSoundFlags = false;
                mSystemSoundFlags = false;
            } else if (mAudioStreamOutFormat == AUDIO_FORMAT_HE_AAC_V2) {
                //fixme, which he-aac format is allowed to this flow.
                sprintf(ConfigParams[*row_index], "%s", DEFAULT_MAIN_HEAAC_V2_FILE_NAME);
                (*row_index)++;
                mMainFlags = true;
                mAppSoundFlags = false;
                mSystemSoundFlags = false;
            } else { //others the format is pcm
                sprintf(ConfigParams[*row_index], "%s%d%s", DEFAULT_APPLICATION_PCM_FILE_NAME, mAudioSteamOutSampleRate, "kHz.wav");
                (*row_index)++;
                mMainFlags = true;
                mAppSoundFlags = false;
                mSystemSoundFlags = true;
            }
        } else {

            if ((mAudioStreamOutFormat == AUDIO_FORMAT_AC3) || (mAudioStreamOutFormat == AUDIO_FORMAT_E_AC3)) {
                sprintf(ConfigParams[*row_index], "%s", "-im");
                (*row_index)++;
                sprintf(ConfigParams[*row_index], "%s", DEFAULT_MAIN_DDP_FILE_NAME);
                (*row_index)++;

                sprintf(ConfigParams[*row_index], "%s", "-ia");
                (*row_index)++;
                sprintf(ConfigParams[*row_index], "%s", DEFAULT_ASSOCIATE_DDP_FILE_NAME);
                (*row_index)++;

                mMainFlags = true;
                mAppSoundFlags = false;
                mSystemSoundFlags = false;
            } else if ((mAudioStreamOutFormat == AUDIO_FORMAT_AAC) || (mAudioStreamOutFormat == AUDIO_FORMAT_HE_AAC_V1)) {
                sprintf(ConfigParams[*row_index], "%s", "-im");
                (*row_index)++;
                sprintf(ConfigParams[*row_index], "%s", DEFAULT_MAIN_HEAAC_V1_FILE_NAME);
                (*row_index)++;

                sprintf(ConfigParams[*row_index], "%s", "-ia");
                (*row_index)++;
                sprintf(ConfigParams[*row_index], "%s", DEFAULT_ASSOCIATE_HEAAC_V1_FILE_NAME);
                (*row_index)++;

                mMainFlags = true;
                mAppSoundFlags = false;
                mSystemSoundFlags = false;
            } else if (mAudioStreamOutFormat == AUDIO_FORMAT_HE_AAC_V2) {
                sprintf(ConfigParams[*row_index], "%s", "-im");
                (*row_index)++;
                sprintf(ConfigParams[*row_index], "%s", DEFAULT_MAIN_HEAAC_V2_FILE_NAME);
                (*row_index)++;

                sprintf(ConfigParams[*row_index], "%s", "-ia");
                (*row_index)++;
                sprintf(ConfigParams[*row_index], "%s", DEFAULT_ASSOCIATE_HEAAC_V2_FILE_NAME);
                (*row_index)++;

                mMainFlags = true;
                mAppSoundFlags = false;
                mSystemSoundFlags = false;
            }
        }
    }

    // have active OTT signal,then configure input Main program input filename? zz
    if (mActivateOTTSignal == true) {
        if ((mAudioStreamOutFormat == AUDIO_FORMAT_AC3) || (mAudioStreamOutFormat == AUDIO_FORMAT_E_AC3)) {
            sprintf(ConfigParams[*row_index], "%s", "-im");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%s", mDolbyMain1FileName);
            (*row_index)++;

            sprintf(ConfigParams[*row_index], "%s", "-im2");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%s", mDolbyMain2FileName);
            (*row_index)++;
            ALOGD("%s() main1 %s main2 %s", __FUNCTION__, mDolbyMain1FileName, mDolbyMain2FileName);
        }
        if (mOTTSoundInputEnable == true) {
            sprintf(ConfigParams[*row_index], "%s", "-ios");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%s", DEFAULT_OTT_PCM_FILE_NAME);
            (*row_index)++;
        }
    }

    if (mHasSystemInput == true) {
        sprintf(ConfigParams[*row_index], "%s", "-is");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%s", DEFAULT_SYSTEM_PCM_FILE_NAME);
        (*row_index)++;
    }


    //OUTPUT mDolbyMS12OutFormat(AUDIO_FORMAT_E_AC3)
    if (mDolbyMS12OutFormat == AUDIO_FORMAT_AC3) {
        sprintf(ConfigParams[*row_index], "%s", "-od");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%s", DEFAULT_OUTPUT_DD_FILE_NAME);
        (*row_index)++;
        if (mDualOutputFlag == true) {
            sprintf(ConfigParams[*row_index], "%s", "-oms");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%s", DEFAULT_OUTPUT_PCM_DOWNMIX_FILE_NAME);
            (*row_index)++;
        }
    } else if (mDolbyMS12OutFormat == AUDIO_FORMAT_E_AC3) {
        sprintf(ConfigParams[*row_index], "%s", "-odp");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%s", DEFAULT_OUTPUT_DDP_FILE_NAME);
        (*row_index)++;
    } else if ((mDolbyMS12OutFormat == AUDIO_FORMAT_PCM_16_BIT) && (mStereoOutputFlag == false) && (mDAPInitMode != 0)) { //pcm multi
        sprintf(ConfigParams[*row_index], "%s", "-om");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%s", DEFAULT_OUTPUT_PCM_MULTI_FILE_NAME);
        (*row_index)++;
    } else { //pcm 2-channel downmix output
        sprintf(ConfigParams[*row_index], "%s", "-oms");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%s", DEFAULT_OUTPUT_PCM_DOWNMIX_FILE_NAME);
        (*row_index)++;
    }

    ALOGV("-%s() line %d\n", __FUNCTION__, __LINE__);
    return 0;
}

int DolbyMS12ConfigParams::APPSoundChannelMaskConvertToChannelConfiguration(audio_channel_mask_t channel_mask)
{
    ALOGV("+%s() line %d\n", __FUNCTION__, __LINE__);
    int ChannelConfiguration = 0;
    switch (channel_mask) {
    case AUDIO_CHANNEL_OUT_MONO:
        ChannelConfiguration = 1;// L
        break;
    case AUDIO_CHANNEL_OUT_STEREO:// L,R
        ChannelConfiguration = 2;
        break;
    case (AUDIO_CHANNEL_OUT_FRONT_LEFT | AUDIO_CHANNEL_OUT_FRONT_RIGHT | AUDIO_CHANNEL_OUT_FRONT_CENTER):// L,R,C
        ChannelConfiguration = 3;
        break;
    case (AUDIO_CHANNEL_OUT_FRONT_LEFT | AUDIO_CHANNEL_OUT_FRONT_RIGHT | AUDIO_CHANNEL_OUT_BACK_LEFT):// L, R, l
        ChannelConfiguration = 4;
        break;
    case (AUDIO_CHANNEL_OUT_FRONT_LEFT | AUDIO_CHANNEL_OUT_FRONT_RIGHT | AUDIO_CHANNEL_OUT_FRONT_CENTER | AUDIO_CHANNEL_OUT_BACK_LEFT):// L, R, C, l
        ChannelConfiguration = 5;
        break;
    case AUDIO_CHANNEL_OUT_5POINT1:// L, R, C, l
        ChannelConfiguration = 6;
        break;
    case (AUDIO_CHANNEL_OUT_FRONT_LEFT | AUDIO_CHANNEL_OUT_FRONT_RIGHT | AUDIO_CHANNEL_OUT_FRONT_CENTER | AUDIO_CHANNEL_OUT_BACK_LEFT | AUDIO_CHANNEL_OUT_BACK_RIGHT):// L, R, C, l
        ChannelConfiguration = 7;
        break;
    case  AUDIO_CHANNEL_OUT_7POINT1:// (L, C, R, l, r, Lrs, Rrs)
        ChannelConfiguration = 21;
        break;
    default:
        ChannelConfiguration = DEFAULT_APP_SOUNDS_CHANNEL_cONFIGURATION;
        break;
    }

    ALOGV("-%s() line %d ChannelConfiguration %d\n", __FUNCTION__, __LINE__, ChannelConfiguration);
    return ChannelConfiguration;
}

int DolbyMS12ConfigParams::SystemSoundChannelMaskConvertToChannelConfiguration(audio_channel_mask_t channel_mask)
{
    ALOGV("+%s() line %d\n", __FUNCTION__, __LINE__);
    int ChannelConfiguration = 0;
    switch (channel_mask) {
    case AUDIO_CHANNEL_OUT_MONO:
        ChannelConfiguration = 1;// L
        break;
    case AUDIO_CHANNEL_OUT_STEREO:// L,R
        ChannelConfiguration = 2;
        break;
    case (AUDIO_CHANNEL_OUT_FRONT_LEFT | AUDIO_CHANNEL_OUT_FRONT_RIGHT | AUDIO_CHANNEL_OUT_FRONT_CENTER):// L,R,C
        ChannelConfiguration = 3;
        break;
    case (AUDIO_CHANNEL_OUT_FRONT_LEFT | AUDIO_CHANNEL_OUT_FRONT_RIGHT | AUDIO_CHANNEL_OUT_BACK_LEFT):// L, R, l
        ChannelConfiguration = 4;
        break;
    case (AUDIO_CHANNEL_OUT_FRONT_LEFT | AUDIO_CHANNEL_OUT_FRONT_RIGHT | AUDIO_CHANNEL_OUT_FRONT_CENTER | AUDIO_CHANNEL_OUT_BACK_LEFT):// L, R, C, l
        ChannelConfiguration = 5;
        break;
    case AUDIO_CHANNEL_OUT_5POINT1:// L, R, C, l
        ChannelConfiguration = 6;
        break;
    case (AUDIO_CHANNEL_OUT_FRONT_LEFT | AUDIO_CHANNEL_OUT_FRONT_RIGHT | AUDIO_CHANNEL_OUT_FRONT_CENTER | AUDIO_CHANNEL_OUT_BACK_LEFT | AUDIO_CHANNEL_OUT_BACK_RIGHT):// L, R, C, l
        ChannelConfiguration = 7;
        break;
    case  AUDIO_CHANNEL_OUT_7POINT1:// (L, C, R, l, r, Lrs, Rrs)
        ChannelConfiguration = 21;
        break;
    default:
        ChannelConfiguration = DEFAULT_SYSTEM_SOUNDS_CHANNEL_cONFIGURATION;
        break;
    }

    ALOGV("-%s() line %d ChannelConfiguration %d\n", __FUNCTION__, __LINE__, ChannelConfiguration);
    return ChannelConfiguration;
}

//functional switches
int DolbyMS12ConfigParams::SetFunctionalSwitches(char **ConfigParams, int *row_index)
{
    ALOGV("+%s() line %d\n", __FUNCTION__, __LINE__);
    if (mStereoOutputFlag == true) {
        if ((mDRCBoost >= 0) && (mDRCBoost <= 100)) {
            sprintf(ConfigParams[*row_index], "%s", "-bs");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%d", mDRCBoostSystem);
            (*row_index)++;
        }

        if ((mDRCCut >= 0) && (mDRCCut <= 100)) {
            sprintf(ConfigParams[*row_index], "%s", "-cs");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%d", mDRCCutSystem);
            (*row_index)++;
        }
    }

    // //fixme, not use this params
    // if (mMultiOutputFlag == true) {
    //     sprintf(ConfigParams[*row_index], "%s", "-mc");
    //     (*row_index)++;
    //     sprintf(ConfigParams[*row_index], "%d", 1);
    //     (*row_index)++;
    // }

    if ((mDRCBoost >= 0) && (mDRCBoost <= 100)) {
        sprintf(ConfigParams[*row_index], "%s", "-b");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDRCBoost);
        (*row_index)++;
    }

    if ((mDRCCut >= 0) && (mDRCCut <= 100)) {
        sprintf(ConfigParams[*row_index], "%s", "-c");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDRCCut);
        (*row_index)++;
    }


    if (mAppSoundFlags == true) {
        sprintf(ConfigParams[*row_index], "%s", "-chas");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mChannelConfAppSoundsIn);
        (*row_index)++;
    }

    // Channel configuration of OTT Sounds input
    if ((mActivateOTTSignal == true) && (mMain1IsDummy == true) && (mOTTSoundInputEnable == true)) {
        sprintf(ConfigParams[*row_index], "%s", "-chos");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mChannelConfOTTSoundsIn);
        (*row_index)++;
    }

    // use mHasSystemInput to replace mSystemSoundFlags instead //zz
    if (mHasSystemInput == true) {
        sprintf(ConfigParams[*row_index], "%s", "-chs");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mChannelConfSystemIn);
        (*row_index)++;
    }

    if ((mDAPInitMode >= 0) && (mDAPInitMode <=  4)) {
        sprintf(ConfigParams[*row_index], "%s", "-dap_init_mode");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDAPInitMode);
        (*row_index)++;
    }

    if (mDAPVirtualBassEnable == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-b_dap_vb_enable");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDAPVirtualBassEnable);
        (*row_index)++;
    }

    if (!mDBGOut) {
        sprintf(ConfigParams[*row_index], "%s", "-dbgout");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDBGOut);
        (*row_index)++;
    }

    if (mDRCModesOfDownmixedOutput == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-drc");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDRCModesOfDownmixedOutput);
        (*row_index)++;
    }

    if (mDAPDRCMode == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-dap_drc");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDAPDRCMode);
        (*row_index)++;
    }

    if (mDonwmixMode == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-dmx");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDonwmixMode);
        (*row_index)++;
    }

    if (mEvaluationMode == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-eval");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mEvaluationMode);
        (*row_index)++;
    }

    if (mLFEPresentInAppSoundIn == 0) {
        sprintf(ConfigParams[*row_index], "%s", "-las");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mLFEPresentInAppSoundIn);
        (*row_index)++;
    }

    if (mLFEPresentInSystemSoundIn == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-ls");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mLFEPresentInSystemSoundIn);
        (*row_index)++;
    }

    // LFE present in OTT Sounds input
    if ((mActivateOTTSignal == true) && (mMain1IsDummy == true) && (mOTTSoundInputEnable == true)) {
        sprintf(ConfigParams[*row_index], "%s", "-los");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mLFEPresentInOTTSoundIn);
        (*row_index)++;
    }

    if ((mMaxChannels == 6) || (mMaxChannels == 8)) {
        sprintf(ConfigParams[*row_index], "%s", "-max_channels");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mMaxChannels);
        (*row_index)++;
    } else { //here we choose 7.1 as the max channels
        sprintf(ConfigParams[*row_index], "%s", "-max_channels");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", 8);
        (*row_index)++;
    }

    if (mDonwnmix71PCMto51 == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-mc_5_1_dmx");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDonwnmix71PCMto51);
        (*row_index)++;
    }

    if (mLockingChannelModeENC == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-chmod_locking");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mLockingChannelModeENC);
        (*row_index)++;
    }

    if (mRISCPrecisionFlag == 0) {
        sprintf(ConfigParams[*row_index], "%s", "-p");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mRISCPrecisionFlag);
        (*row_index)++;
    }

    if (mDualMonoReproMode != 0) {
        sprintf(ConfigParams[*row_index], "%s", "-u");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDualMonoReproMode);
        (*row_index)++;
    }

    if ((mVerbosity >= 0) && (mVerbosity <= 3)) {
        sprintf(ConfigParams[*row_index], "%s", "-v");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mVerbosity);
        (*row_index)++;
    }

    if ((mOutputBitDepth == 16) || (mOutputBitDepth == 24) || (mOutputBitDepth == 32)) {
        sprintf(ConfigParams[*row_index], "%s", "-w");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mOutputBitDepth);
        (*row_index)++;
    }

    if ((mAssociatedAudioMixing == 0) && (mHasAssociateInput == true)) {
        sprintf(ConfigParams[*row_index], "%s", "-xa");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mAssociatedAudioMixing);
        (*row_index)++;
    }

    //if (mSystemAPPAudioMixing == 0)
    {
        sprintf(ConfigParams[*row_index], "%s", "-xs");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mSystemAPPAudioMixing);
        (*row_index)++;
    }

    if (mHasAssociateInput == true) { //set this params when dual input.
        if (mUserControlVal > 32) {
            mUserControlVal = 32;
        } else if (mUserControlVal < -32) {
            mUserControlVal = -32;
        }
        sprintf(ConfigParams[*row_index], "%s", "-xu");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mUserControlVal);
        (*row_index)++;
    }


    //fixme, which params are suitable
    if (mMainFlags == true) {
        sprintf(ConfigParams[*row_index], "%s", "-main1_mixgain");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d", mMain1MixGain.target, mMain1MixGain.duration, mMain1MixGain.shape);//choose mid-val
        (*row_index)++;
    }

    if (mHasAssociateInput == true) {
        sprintf(ConfigParams[*row_index], "%s", "-main2_mixgain");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d", mMain2MixGain.target, mMain2MixGain.duration, mMain2MixGain.shape);//choose mid-val
        (*row_index)++;
    }

    if ((mActivateOTTSignal == true) && (mMain1IsDummy == true) && (mOTTSoundInputEnable == true)) {
        sprintf(ConfigParams[*row_index], "%s", "-ott_mixgain");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d", mOTTMixGain.target, mOTTMixGain.duration, mOTTMixGain.shape);
        (*row_index)++;
    }

    if ((mMainFlags == true) && (mHasSystemInput == true)) {
        sprintf(ConfigParams[*row_index], "%s", "-sys_prim_mixgain");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d", mSysPrimMixGain.target, mSysPrimMixGain.duration, mSysPrimMixGain.shape);//choose mid-val
        (*row_index)++;
    }

    if (mAppSoundFlags == true) {
        sprintf(ConfigParams[*row_index], "%s", "-sys_apps_mixgain");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d", mSysApppsMixGain.target, mSysApppsMixGain.duration, mSysApppsMixGain.shape);//choose mid-val
        (*row_index)++;
    }

    if (mHasSystemInput == true) {
        sprintf(ConfigParams[*row_index], "%s", "-sys_syss_mixgain");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d", mSysSyssMixGain.target, mSysSyssMixGain.duration, mSysSyssMixGain.shape);//choose mid-val
        (*row_index)++;
    }

    ALOGV("-%s() line %d\n", __FUNCTION__, __LINE__);
    return 0;
}

//functional switches
int DolbyMS12ConfigParams::SetFunctionalSwitchesRuntime(char **ConfigParams, int *row_index)
{
    ALOGV("+%s() line %d\n", __FUNCTION__, __LINE__);
    if (mStereoOutputFlag == true) {
        if ((mDRCBoost >= 0) && (mDRCBoost <= 100)) {
            sprintf(ConfigParams[*row_index], "%s", "-bs");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%d", mDRCBoostSystem);
            (*row_index)++;
        }

        if ((mDRCCut >= 0) && (mDRCCut <= 100)) {
            sprintf(ConfigParams[*row_index], "%s", "-cs");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%d", mDRCCutSystem);
            (*row_index)++;
        }
    }

    // //fixme, not use this params
    // if (mMultiOutputFlag == true) {
    //     sprintf(ConfigParams[*row_index], "%s", "-mc");
    //     (*row_index)++;
    //     sprintf(ConfigParams[*row_index], "%d", 1);
    //     (*row_index)++;
    // }

    if ((mDRCBoost >= 0) && (mDRCBoost <= 100)) {
        sprintf(ConfigParams[*row_index], "%s", "-b");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDRCBoost);
        (*row_index)++;
    }

    if ((mDRCCut >= 0) && (mDRCCut <= 100)) {
        sprintf(ConfigParams[*row_index], "%s", "-c");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDRCCut);
        (*row_index)++;
    }


    if (mAppSoundFlags == true) {
        sprintf(ConfigParams[*row_index], "%s", "-chas");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mChannelConfAppSoundsIn);
        (*row_index)++;
    }

    if ((mActivateOTTSignal == true) && (mMain1IsDummy == true) && (mOTTSoundInputEnable == true)) {
        sprintf(ConfigParams[*row_index], "%s", "-chos");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mChannelConfOTTSoundsIn);
        (*row_index)++;
    }

    if (mHasSystemInput == true) {
        sprintf(ConfigParams[*row_index], "%s", "-chs");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mChannelConfSystemIn);
        (*row_index)++;
    }

    if (mDRCModesOfDownmixedOutput == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-drc");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDRCModesOfDownmixedOutput);
        (*row_index)++;
    }

    if (mDAPDRCMode == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-dap_drc");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDAPDRCMode);
        (*row_index)++;
    }

    if (mDonwmixMode == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-dmx");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDonwmixMode);
        (*row_index)++;
    }

    if (mLFEPresentInAppSoundIn == 0) {
        sprintf(ConfigParams[*row_index], "%s", "-las");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mLFEPresentInAppSoundIn);
        (*row_index)++;
    }

    if (mLFEPresentInSystemSoundIn == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-ls");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mLFEPresentInSystemSoundIn);
        (*row_index)++;
    }

    if ((mActivateOTTSignal == true) && (mMain1IsDummy == true) && (mOTTSoundInputEnable == true)) {
        sprintf(ConfigParams[*row_index], "%s", "-los");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mLFEPresentInOTTSoundIn);
        (*row_index)++;
    }

    if (mDualMonoReproMode != 0) {
        sprintf(ConfigParams[*row_index], "%s", "-u");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDualMonoReproMode);
        (*row_index)++;
    }

    if ((mAssociatedAudioMixing == 0) && (mHasAssociateInput == true)) {
        sprintf(ConfigParams[*row_index], "%s", "-xa");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mAssociatedAudioMixing);
        (*row_index)++;
    }

    //if (mSystemAPPAudioMixing == 0)
    {
        sprintf(ConfigParams[*row_index], "%s", "-xs");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mSystemAPPAudioMixing);
        (*row_index)++;
    }

    if (mHasAssociateInput == true) { //set this params when dual input.
        if (mUserControlVal > 32) {
            mUserControlVal = 32;
        } else if (mUserControlVal < -32) {
            mUserControlVal = -32;
        }
        sprintf(ConfigParams[*row_index], "%s", "-xu");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mUserControlVal);
        (*row_index)++;
    }


    //fixme, which params are suitable
    if (mMainFlags == true) {
        sprintf(ConfigParams[*row_index], "%s", "-main1_mixgain");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d", mMain1MixGain.target, mMain1MixGain.duration, mMain1MixGain.shape);//choose mid-val
        (*row_index)++;
    }

    if (mHasAssociateInput == true) {
        sprintf(ConfigParams[*row_index], "%s", "-main2_mixgain");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d", mMain2MixGain.target, mMain2MixGain.duration, mMain2MixGain.shape);//choose mid-val
        (*row_index)++;
    }

    if ((mActivateOTTSignal == true) && (mMain1IsDummy == true) && (mOTTSoundInputEnable == true)) {
        sprintf(ConfigParams[*row_index], "%s", "-ott_mixgain");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d", mOTTMixGain.target, mOTTMixGain.duration, mOTTMixGain.shape);
        (*row_index)++;
    }

    if ((mMainFlags == true) && (mHasSystemInput == true)) {
        sprintf(ConfigParams[*row_index], "%s", "-sys_prim_mixgain");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d", mSysPrimMixGain.target, mSysPrimMixGain.duration, mSysPrimMixGain.shape);//choose mid-val
        (*row_index)++;
    }

    if (mAppSoundFlags == true) {
        sprintf(ConfigParams[*row_index], "%s", "-sys_apps_mixgain");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d", mSysApppsMixGain.target, mSysApppsMixGain.duration, mSysApppsMixGain.shape);//choose mid-val
        (*row_index)++;
    }

    if (mHasSystemInput == true) {
        sprintf(ConfigParams[*row_index], "%s", "-sys_syss_mixgain");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d", mSysSyssMixGain.target, mSysSyssMixGain.duration, mSysSyssMixGain.shape);//choose mid-val
        (*row_index)++;
    }

    ALOGV("-%s() line %d\n", __FUNCTION__, __LINE__);
    return 0;
}

//ddplus switches
int DolbyMS12ConfigParams::SetDdplusSwitches(char **ConfigParams, int *row_index)
{
    ALOGV("+%s() line %d\n", __FUNCTION__, __LINE__);
    if ((mHasAssociateInput == true) && ((mAudioStreamOutFormat == AUDIO_FORMAT_AC3) || (mAudioStreamOutFormat == AUDIO_FORMAT_E_AC3))) {
        sprintf(ConfigParams[*row_index], "%s", "-at");
        (*row_index)++;
        ALOGD("+%s() mDdplusAssocSubstream %d\n", __FUNCTION__, mDdplusAssocSubstream);
        sprintf(ConfigParams[*row_index], "%d", mDdplusAssocSubstream);//choose mid-val
        (*row_index)++;
    }

    ALOGV("-%s() line %d\n", __FUNCTION__, __LINE__);
    return 0;
}

//PCM switches
int DolbyMS12ConfigParams::SetPCMSwitches(char **ConfigParams, int *row_index)
{
    ALOGV("+%s() line %d\n", __FUNCTION__, __LINE__);
    if ((mSystemSoundFlags == true) || (mAppSoundFlags == true)) {
        mChannelConfigInExtPCMInput = APPSoundChannelMaskConvertToChannelConfiguration(mAudioStreamOutChannelMask);
        {
            sprintf(ConfigParams[*row_index], "%s", "-chp");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%d", mChannelConfigInExtPCMInput);
            (*row_index)++;
        }

        if ((mChannelConfigInExtPCMInput == 1) || (mChannelConfigInExtPCMInput == 2)) {
            mLFEPresentInExtPCMInput = false;
            sprintf(ConfigParams[*row_index], "%s", "-lp");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%d", 0);
            (*row_index)++;
        }

        if (mActivateOTTSignal == false) {
            if ((mCompressorProfile >= 0) && (mCompressorProfile <= 5)) {
                sprintf(ConfigParams[*row_index], "%s", "-rp");
                (*row_index)++;
                sprintf(ConfigParams[*row_index], "%d", mCompressorProfile);
                (*row_index)++;
            }
        }
    }

    ALOGV("-%s() line %d\n", __FUNCTION__, __LINE__);
    return 0;
}

//PCM switches
int DolbyMS12ConfigParams::SetPCMSwitchesRuntime(char **ConfigParams, int *row_index)
{
    ALOGV("+%s() line %d\n", __FUNCTION__, __LINE__);
    if ((mSystemSoundFlags == true) || (mAppSoundFlags == true)) {
        mChannelConfigInExtPCMInput = APPSoundChannelMaskConvertToChannelConfiguration(mAudioStreamOutChannelMask);
        {
            sprintf(ConfigParams[*row_index], "%s", "-chp");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%d", mChannelConfigInExtPCMInput);
            (*row_index)++;
        }

        if ((mChannelConfigInExtPCMInput == 1) || (mChannelConfigInExtPCMInput == 2)) {
            mLFEPresentInExtPCMInput = false;
            sprintf(ConfigParams[*row_index], "%s", "-lp");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%d", 0);
            (*row_index)++;
        }
    }

    ALOGV("-%s() line %d\n", __FUNCTION__, __LINE__);
    return 0;
}

//HE-AAC switches, all none-run-time
int DolbyMS12ConfigParams::SetHEAACSwitches(char **ConfigParams, int *row_index)
{
    ALOGV("+%s() line %d\n", __FUNCTION__, __LINE__);
    if ((mHasAssociateInput == true) && ((mAudioStreamOutFormat == AUDIO_FORMAT_AAC) || \
                                         (mAudioStreamOutFormat == AUDIO_FORMAT_HE_AAC_V1) || (mAudioStreamOutFormat == AUDIO_FORMAT_HE_AAC_V2))) {
        {
            sprintf(ConfigParams[*row_index], "%s", "-as");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%d", mAssocInstanse);
            (*row_index)++;
        }

        if ((mDefDialnormVal >= 0) && (mDefDialnormVal <= 127)) {
            sprintf(ConfigParams[*row_index], "%s", "-dn");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%d", mDefDialnormVal);
            (*row_index)++;
        }

        if ((mTransportFormat >= 0) && (mTransportFormat <= 3)) {
            sprintf(ConfigParams[*row_index], "%s", "-t");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%d", mTransportFormat);
            (*row_index)++;
        }
    }

    ALOGV("-%s() line %d\n", __FUNCTION__, __LINE__);
    return 0;
}


//OTT PROCESSING GRAPH SWITCHES
int DolbyMS12ConfigParams::SetOTTProcessingGraphSwitches(char **ConfigParams, int *row_index)
{
    ALOGV("+%s() line %d\n", __FUNCTION__, __LINE__);
    {
        //TODO: already exist in other setting, remove here , otherwise will cause error when initializing MS12 zz
        //sprintf(ConfigParams[*row_index], "%s", "-chmod_locking");
        //(*row_index)++;
        //sprintf(ConfigParams[*row_index], "%d", mLockingChannelModeENC);
        //(*row_index)++;
    }

    if (mActivateOTTSignal == true) {
        sprintf(ConfigParams[*row_index], "%s", "-ott");
        (*row_index)++;

        if (mAtmosLock == true) {
            sprintf(ConfigParams[*row_index], "%s", "-atmos_lock");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%d", mAtmosLock);
            (*row_index)++;
        }

        if ((mPause == true) || (mPause == false)) {
            sprintf(ConfigParams[*row_index], "%s", "-pause");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%d", mPause);
            (*row_index)++;
        }
    }

    ALOGV("-%s() line %d\n", __FUNCTION__, __LINE__);
    return 0;
}

//OTT PROCESSING GRAPH SWITCHES(runtime)
int DolbyMS12ConfigParams::SetOTTProcessingGraphSwitchesRuntime(char **ConfigParams, int *row_index)
{
    ALOGV("+%s() line %d\n", __FUNCTION__, __LINE__);
    if (mActivateOTTSignal == true) {
        if (mAtmosLock == true) {
            sprintf(ConfigParams[*row_index], "%s", "-atmos_lock");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%d", mAtmosLock);
            (*row_index)++;
        }

        if ((mPause == true) || (mPause == false)) {
            sprintf(ConfigParams[*row_index], "%s", "-pause");
            (*row_index)++;
            sprintf(ConfigParams[*row_index], "%d", mPause);
            (*row_index)++;
        }
    }

    ALOGV("-%s() line %d\n", __FUNCTION__, __LINE__);
    return 0;
}

//DAP SWITCHES (device specific)
//all run-time
int DolbyMS12ConfigParams::SetDAPDeviceSwitches(char **ConfigParams, int *row_index)
{
    String8 tmpParam("");
    ALOGV("+%s() line %d\n", __FUNCTION__, __LINE__);
    if ((mDAPCalibrationBoost >= 0) && (mDAPCalibrationBoost <= 192)) {
        sprintf(ConfigParams[*row_index], "%s", "-dap_calibration_boost");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDAPCalibrationBoost);
        (*row_index)++;
    }

    if (mDAPDMX == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-dap_dmx");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDAPDMX);
        (*row_index)++;
    }

    if ((mDAPGains >= -2080) && (mDAPGains <= 480)) {
        sprintf(ConfigParams[*row_index], "%s", "-dap_gains");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDAPGains);
        (*row_index)++;
    }

    if (mDAPSurDecEnable == false) {
        sprintf(ConfigParams[*row_index], "%s", "-dap_surround_decoder_enable");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d", mDAPSurDecEnable);
        (*row_index)++;
    }

    if (DeviceDAPSurroundVirtualizer.virtualizer_enable == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-dap_surround_virtualizer");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d,%d,%d", DeviceDAPSurroundVirtualizer.virtualizer_enable,
                DeviceDAPSurroundVirtualizer.headphone_reverb, DeviceDAPSurroundVirtualizer.speaker_angle,
                DeviceDAPSurroundVirtualizer.speaker_start, DeviceDAPSurroundVirtualizer.surround_boost);
        (*row_index)++;
    }

    if (DeviceDAPGraphicEQ.eq_enable == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-dap_graphic_eq");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d", DeviceDAPGraphicEQ.eq_enable, DeviceDAPGraphicEQ.eq_nb_bands);
        tmpParam += String8::format("%s", ConfigParams[*row_index]);
        int i = 0;
        for (i = 0; i < DeviceDAPGraphicEQ.eq_nb_bands; i++) {
            sprintf(ConfigParams[*row_index], ",%d", DeviceDAPGraphicEQ.eq_band_center[i]);
            tmpParam += String8::format("%s", ConfigParams[*row_index]);
        }

        for (i = 0; i < DeviceDAPGraphicEQ.eq_nb_bands; i++) {
            sprintf(ConfigParams[*row_index], ",%d", DeviceDAPGraphicEQ.eq_band_target[i]);
            tmpParam += String8::format("%s", ConfigParams[*row_index]);
        }
        memcpy(ConfigParams[*row_index], tmpParam.string(), strlen(tmpParam.string()));
        (*row_index)++;
    }

    tmpParam.clear();
    if (DeviceDAPOptimizer.optimizer_enable == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-dap_optimizer");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d", DeviceDAPOptimizer.optimizer_enable, DeviceDAPOptimizer.opt_nb_bands);
        tmpParam += String8::format("%s", ConfigParams[*row_index]);
        int i = 0;
        for (i = 0; i < DeviceDAPOptimizer.opt_nb_bands; i++) {
            sprintf(ConfigParams[*row_index], ",%d", DeviceDAPOptimizer.opt_band_center_freq[i]);
            tmpParam += String8::format("%s", ConfigParams[*row_index]);
        }
        for (i = 0; i < mMaxChannels * DeviceDAPOptimizer.opt_nb_bands; i++) {
            sprintf(ConfigParams[*row_index], ",%d", DeviceDAPOptimizer.opt_band_gains[i]);
            tmpParam += String8::format("%s", ConfigParams[*row_index]);
        }
        memcpy(ConfigParams[*row_index], tmpParam.string(), strlen(tmpParam.string()));
        (*row_index)++;
    }


    if (DeviceDAPBassEnhancer.bass_enable == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-dap_bass_enhancer");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d,%d", DeviceDAPBassEnhancer.bass_enable,
                DeviceDAPBassEnhancer.bass_boost, DeviceDAPBassEnhancer.bass_cutoff, DeviceDAPBassEnhancer.bass_width);
        (*row_index)++;
    }


    tmpParam.clear();
    if (DeviceDAPRegulator.regulator_enable == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-dap_regulator");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d,%d,%d,%d",
                DeviceDAPRegulator.regulator_enable, DeviceDAPRegulator.regulator_mode,
                DeviceDAPRegulator.regulator_overdrive, DeviceDAPRegulator.regulator_timbre,
                DeviceDAPRegulator.regulator_distortion, DeviceDAPRegulator.reg_nb_bands);
        tmpParam += String8::format("%s", ConfigParams[*row_index]);
        int i = 0;
        for (i = 0; i < DeviceDAPRegulator.reg_nb_bands; i++) {
            sprintf(ConfigParams[*row_index], ",%d", DeviceDAPRegulator.reg_band_center[i]);
            tmpParam += String8::format("%s", ConfigParams[*row_index]);
        }
        for (i = 0; i < DeviceDAPRegulator.reg_nb_bands; i++) {
            sprintf(ConfigParams[*row_index], ",%d", DeviceDAPRegulator.reg_low_thresholds[i]);
            tmpParam += String8::format("%s", ConfigParams[*row_index]);
        }
        for (i = 0; i < DeviceDAPRegulator.reg_nb_bands; i++) {
            sprintf(ConfigParams[*row_index], ",%d", DeviceDAPRegulator.reg_high_thresholds[i]);
            tmpParam += String8::format("%s", ConfigParams[*row_index]);
        }
        for (i = 0; i < DeviceDAPRegulator.reg_nb_bands; i++) {
            sprintf(ConfigParams[*row_index], ",%d", DeviceDAPRegulator.reg_isolated_bands[i]);
            tmpParam += String8::format("%s", ConfigParams[*row_index]);
        }
        memcpy(ConfigParams[*row_index], tmpParam.string(), strlen(tmpParam.string()));
        (*row_index)++;
    }

    if (mDAPVirtualBassEnable == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-dap_virtual_bass");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", DeviceDAPVirtualBass.virtual_bass_mode,
                DeviceDAPVirtualBass.virtual_bass_low_src_freq, DeviceDAPVirtualBass.virtual_bass_high_src_freq,
                DeviceDAPVirtualBass.virtual_bass_overall_gain, DeviceDAPVirtualBass.virtual_bass_slope_gain,
                DeviceDAPVirtualBass.virtual_bass_subgains[0], DeviceDAPVirtualBass.virtual_bass_subgains[1],
                DeviceDAPVirtualBass.virtual_bass_subgains[2], DeviceDAPVirtualBass.virtual_bass_low_mix_freq,
                DeviceDAPVirtualBass.virtual_bass_high_mix_freq);
        (*row_index)++;
    }

    ALOGV("-%s() line %d\n", __FUNCTION__, __LINE__);
    return 0;
}

//DAP SWITCHES (content specific)
//all run-time
int DolbyMS12ConfigParams::SetDAPContentSwitches(char **ConfigParams, int *row_index)
{
    ALOGV("+%s() line %d\n", __FUNCTION__, __LINE__);
    {
        sprintf(ConfigParams[*row_index], "%s", "-dap_mi_steering");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d,%d", ContentDAPMISteering.mi_ieq_enable, ContentDAPMISteering.mi_dv_enable, ContentDAPMISteering.mi_de_enable, ContentDAPMISteering.mi_surround_enable);
        (*row_index)++;
    }

    if (ContentDAPLeveler.leveler_enable == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-dap_leveler");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d", ContentDAPLeveler.leveler_enable, ContentDAPLeveler.leveler_amount, ContentDAPLeveler.leveler_ignore_il);
        (*row_index)++;
    }


    if (ContentDAPIEQ.ieq_enable == 1) {
        String8 tmpParam("");
        sprintf(ConfigParams[*row_index], "%s", "-dap_ieq");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d", ContentDAPIEQ.ieq_enable, ContentDAPIEQ.ieq_amount, ContentDAPIEQ.ieq_nb_bands);
        tmpParam += String8::format("%s", ConfigParams[*row_index]);
        int i = 0;
        for (i = 0; i < ContentDAPIEQ.ieq_nb_bands; i++) {
            sprintf(ConfigParams[*row_index], ",%d", ContentDAPIEQ.ieq_band_center[i]);
            tmpParam += String8::format("%s", ConfigParams[*row_index]);
        }
        for (i = 0; i < ContentDAPIEQ.ieq_nb_bands; i++) {
            sprintf(ConfigParams[*row_index], ",%d", ContentDAPIEQ.ieq_band_target[i]);
            tmpParam += String8::format("%s", ConfigParams[*row_index]);
        }
        memcpy(ConfigParams[*row_index], tmpParam.string(), strlen(tmpParam.string()));
        (*row_index)++;
    }


    if (ContenDAPDialogueEnhancer.de_enable == 1) {
        sprintf(ConfigParams[*row_index], "%s", "-dap_dialogue_enhancer");
        (*row_index)++;
        sprintf(ConfigParams[*row_index], "%d,%d,%d", ContenDAPDialogueEnhancer.de_enable, ContenDAPDialogueEnhancer.de_amount, ContenDAPDialogueEnhancer.de_ducking);
        (*row_index)++;
    }

    ALOGV("-%s() line %d\n", __FUNCTION__, __LINE__);
    return 0;
}

//get dolby ms12 config params
char **DolbyMS12ConfigParams::GetDolbyMS12ConfigParams(int *argc)
{
    ALOGD("+%s()\n", __FUNCTION__);

    if (argc && mConfigParams) {
        char params_bin[] = "ms12_exec";
        sprintf(mConfigParams[mParamNum++], "%s", params_bin);
        SetInputOutputFileName(mConfigParams, &mParamNum);
        SetFunctionalSwitches(mConfigParams, &mParamNum);
        SetDdplusSwitches(mConfigParams, &mParamNum);
        SetPCMSwitches(mConfigParams, &mParamNum);
        SetHEAACSwitches(mConfigParams, &mParamNum);
        SetOTTProcessingGraphSwitches(mConfigParams, &mParamNum);
        if (mDAPInitMode) {
            SetDAPDeviceSwitches(mConfigParams, &mParamNum);
            SetDAPContentSwitches(mConfigParams, &mParamNum);
        }
        *argc = mParamNum;
        ALOGV("%s() line %d argc %d\n", __FUNCTION__, __LINE__, *argc);
        //here is to check the config params

        int config_params_check = 1;
        if (config_params_check) {
            int i = 0;
            for (i = 0; i < mParamNum; i++) {
                ALOGD("param #%d: %s\n", i, mConfigParams[i]);
            }
        }
    }

    ALOGD("-%s()", __FUNCTION__);
    return mConfigParams;
}

char **DolbyMS12ConfigParams::GetDolbyMS12RuntimeConfigParams(int *argc)
{
    ALOGD("+%s()", __FUNCTION__);

    if (argc && mConfigParams) {
        char params_bin[] = "ms12_exec";
        sprintf(mConfigParams[mParamNum++], "%s", params_bin);
        SetFunctionalSwitchesRuntime(mConfigParams, &mParamNum);
        SetDdplusSwitches(mConfigParams, &mParamNum);
        SetPCMSwitchesRuntime(mConfigParams, &mParamNum);
        SetOTTProcessingGraphSwitchesRuntime(mConfigParams, &mParamNum);
        if (mDAPInitMode) {
            SetDAPDeviceSwitches(mConfigParams, &mParamNum);
            SetDAPContentSwitches(mConfigParams, &mParamNum);
        }
        *argc = mParamNum;
        ALOGV("%s() line %d argc %d\n", __FUNCTION__, __LINE__, *argc);
        //here is to check the config params

        int config_params_check = 1;
        if (config_params_check) {
            int i = 0;
            for (i = 0; i < mParamNum; i++) {
                ALOGD("param #%d: %s\n", i, mConfigParams[i]);
            }
        }
    }

    ALOGD("-%s()", __FUNCTION__);
    return mConfigParams;
}

char **DolbyMS12ConfigParams::PrepareConfigParams(int max_raw_size, int max_column_size)
{
    ALOGD("+%s() line %d\n", __FUNCTION__, __LINE__);
    int i = 0;
    int cnt = 0;
    char **ConfigParams = (char **)malloc(sizeof(char *)*max_raw_size);
    if (ConfigParams == NULL) {
        ALOGE("%s::%d, malloc error\n", __FUNCTION__, __LINE__);
        goto Error_Prepare;
    }

    for (i = 0; i < MAX_ARGC; i++) {
        ConfigParams[i] = (char *)malloc(max_column_size);
        if (ConfigParams[i] == NULL) {
            ALOGE("%s() line %d, malloc error\n", __FUNCTION__, __LINE__);
            for (cnt = 0; cnt < i; cnt++) {
                free(ConfigParams[cnt]);
                ConfigParams[cnt] = NULL;
            }
            free(ConfigParams);
            ConfigParams = NULL;
            goto Error_Prepare;
        }
    }
    ALOGD("+%s() line %d\n", __FUNCTION__, __LINE__);
    return ConfigParams;
Error_Prepare:
    ALOGD("-%s() line %d error prepare\n", __FUNCTION__, __LINE__);
    return NULL;
}

void DolbyMS12ConfigParams::CleanupConfigParams(char **ConfigParams, int max_raw_size)
{
    ALOGD("+%s() line %d\n", __FUNCTION__, __LINE__);
    int i = 0;
    for (i = 0; i < max_raw_size; i++) {
        if (ConfigParams[i]) {
            free(ConfigParams[i]);
            ConfigParams[i] = NULL;
        }
    }

    if (ConfigParams) {
        free(ConfigParams);
        ConfigParams = NULL;
    }

    ALOGD("-%s() line %d\n", __FUNCTION__, __LINE__);
    return ;
}

void DolbyMS12ConfigParams::ResetConfigParams(void)
{
    ALOGD("+%s() line %d\n", __FUNCTION__, __LINE__);
    int i = 0;
    if (mConfigParams) {
        for (i = 0; i < MAX_ARGC; i++) {
            if (mConfigParams[i]) {
                memset(mConfigParams[i], 0, MAX_ARGV_STRING_LEN);
            }
        }
    }
    mParamNum = 0;//reset the input params
    mHasAssociateInput = false;
    mHasSystemInput = false;
    mMainFlags = 1;
    ALOGD("%s() mHasAssociateInput %d mHasSystemInput %d\n", __FUNCTION__, mHasAssociateInput, mHasSystemInput);
    ALOGD("-%s() line %d\n", __FUNCTION__, __LINE__);
    return ;
}

}//end android



