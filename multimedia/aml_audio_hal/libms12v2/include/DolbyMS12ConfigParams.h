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

#ifndef _DOLBY_MS12_CONFIG_PARAMS_H_
#define _DOLBY_MS12_CONFIG_PARAMS_H_


#include <stdbool.h>
#include <system/audio.h>
//#include <system/audio_policy.h>
#include <mutex>

#include "dolby_ms12_config_parameter_struct.h"
#include "dolby_ms12_output_mask.h"

//@@@DDPlus input file
#define DEFAULT_MAIN_DDP_FILE_NAME "/data/main.ac3"
#define DEFAULT_MAIN2_DDP_FILE_NAME "/data/main2.ac3"
#define DEFAULT_ASSOCIATE_DDP_FILE_NAME "/data/associate.ac3"

#define DEFAULT_DUMMY_DDP_FILE_NAME "dummy.ac3"
#define DEFAULT_DAP_TUNING_FILE_NAME "/vendor/etc/ms12_tuning.dat"

#ifdef __cplusplus
namespace android
{

class DolbyMS12ConfigParams
{

public:

    DolbyMS12ConfigParams();
    virtual ~DolbyMS12ConfigParams();
    // static DolbyMS12ConfigParams* getInstance();


    // static DolbyMS12ConfigParams *getInstance();
    virtual void SetAudioStreamOutParams(audio_output_flags_t flags
                                         , audio_format_t input_format
                                         , audio_channel_mask_t channel_mask
                                         , int sample_rate
                                         , int output_format);
    virtual int SetInputOutputFileName(char **ConfigParams, int *row_index);
    virtual int SetFunctionalSwitches(char **ConfigParams, int *row_index);
#if 0
    virtual int SetFunctionalSwitchesRuntime(char **ConfigParams, int *row_index);
    virtual int SetFunctionalSwitchesRuntime_lite(char **ConfigParams, int *row_index);
    virtual int SetDdplusSwitches(char **ConfigParams, int *row_index);
#endif

    virtual int SetPCMSwitches(char **ConfigParams, int *row_index);
#if 0
    virtual int SetPCMSwitchesRuntime(char **ConfigParams, int *row_index);
#endif
    virtual int SetAc4Switches(char **ConfigParams, int *row_index);
    virtual int SetHEAACSwitches(char **ConfigParams, int *row_index);
    virtual int SetDAPDeviceSwitches(char **ConfigParams, int *row_index, int is_runtime);
    virtual int SetDAPContentSwitches(char **ConfigParams, int *row_index);
    virtual char **GetDolbyMS12ConfigParams(int *argc);
#if 0
    virtual char **GetDolbyMS12RuntimeConfigParams(int *argc);
#endif
    virtual char **UpdateDolbyMS12RuntimeConfigParams(int *argc, char *cmd);
#if 0
    virtual char **GetDolbyMS12RuntimeConfigParams_lite(int *argc);
#endif

    //init the  mConfigParams Array
    virtual char **PrepareConfigParams(int max_raw_size, int max_column_size);
    //cleanup the mConfigParams Array
    virtual void CleanupConfigParams(char **ConfigParams, int max_raw_size);

    virtual int GetDoblyConfigOutputConfig(void)
    {
        return mDolbyMS12OutConfig;
    }
    virtual int GetDolbyConfigOutputSampleRate(void)
    {
        return mDolbyMS12OutSampleRate;
    }
    virtual audio_channel_mask_t GetDolbyConfigOutputChannelMask(void)
    {
        return mDolbyMS12OutChannelMask;
    }
    virtual void ResetConfigParams(void);
    //associate flags
    virtual void setAssociateFlag(bool flag)
    {
        ALOGI("%s() Associate flag %d\n", __FUNCTION__, flag);
        mHasAssociateInput = flag;
    }
    virtual int getAssociateFlag(void)
    {
        ALOGI("%s() mHasAssociateInput %d\n", __FUNCTION__, mHasAssociateInput);
        return mHasAssociateInput;
    }
    //system flags
    virtual void setSystemFlag(bool flag)
    {
        ALOGI("%s() System flag %d\n", __FUNCTION__, flag);
        mHasSystemInput = flag;
        mSystemSoundFlags = flag;
    }
    virtual int getSystemFlag(void)
    {
        ALOGI("%s() mHasSystemInput %d\n", __FUNCTION__, mHasSystemInput);
        return mHasSystemInput;
    }
    virtual int APPSoundChannelMaskConvertToChannelConfiguration(audio_channel_mask_t channel_mask);
    virtual int SystemSoundChannelMaskConvertToChannelConfiguration(audio_channel_mask_t channel_mask);

    //*Begin||Add the APT to set the params*//
    //Functional Switches
    //virtual void setLowComplexityNode(bool flag) { mLowComplexityMode = flag; }//did not use this part!
    virtual void setDRCboostVal(int val)
    {
        mDRCBoost = val;
    }
    virtual void setDRCcutVal(int val)
    {
        mDRCCut = val;
    }
    virtual void setDRCboostStereoVal(int val)
    {
        mDRCBoostStereo = val;
    }
    virtual void setDRCcutStereoVal(int val)
    {
        mDRCCutStereo = val;
    }
    virtual void setChannelConfigOfAppSoundsInput(audio_channel_mask_t channel_mask)
    {
        mChannelConfAppSoundsIn = APPSoundChannelMaskConvertToChannelConfiguration(channel_mask);
    }
    virtual void setChannelConfigOfSystemSoundsInput(audio_channel_mask_t channel_mask)
    {
        mChannelConfSystemIn = SystemSoundChannelMaskConvertToChannelConfiguration(channel_mask);
    }
    virtual void setDAPV2InitialisationMode(int val)
    {
        mDAPInitMode = val;    // 0 or 1
    }
    virtual void setDAPV2VirtualBassEnable(bool flag)
    {
        mDAPVirtualBassEnable = flag;    // 0 or 1
    }
    virtual void setDRCMode(int val)
    {
        mDRCModesOfDownmixedOutput = val;    // 0 or 1
    }
    virtual void setDAPDRCMode(int val)
    {
        mDAPDRCMode = val;    // 0 or 1
    }
    virtual void setDownmixModes(int val)
    {
        mDownmixMode = val;    // 0 or 1
    }
    virtual void setEvalutionMode(int val)
    {
        mEvaluationMode = val;    // 0 or 1
    }
    virtual void setLFEpresentInAPPSoundsIn(int val)
    {
        mLFEPresentInAppSoundIn = val;    // 0 or 1
    }
    virtual void setLFEpresetInSystemSoundsIn(int val)
    {
        mLFEPresentInSystemSoundIn = val;    // 0 or 1
    }
    virtual void setDownmix71PCMto51OnMultiOutputs(int val)
    {
        mDonwnmix71PCMto51 = val;    //0 or 1
    }
    virtual void setEncoderChannelModeLockingMode(int val)
    {
        mLockingChannelModeENC = val;    //0 or 1
    }
    virtual void setRISCprecisionFlag(int val)
    {
        mRISCPrecisionFlag = val;
    }
    virtual void setDualmonoReproductionMode(int val)
    {
        mDualMonoReproMode = val;
    }
    virtual void setAssociatedAudioMixing(int val)
    {
        mAssociatedAudioMixing = val;
        //ALOGI("%s() mAssociatedAudioMixing %d\n", __FUNCTION__, mAssociatedAudioMixing);
    }
    virtual void setSystemAppAudioMixing(int val)
    {
        mSystemAPPAudioMixing = val;
        //ALOGI("%s() mSystemAPPAudioMixing %d\n", __FUNCTION__, mSystemAPPAudioMixing);
    }
    virtual int getSystemAppAudioMixing(void)
    {
        return mSystemAPPAudioMixing;
    }
    virtual void setUserControlValuesForMixingMainAndAssociatedAudio(int val)
    {
        mUserControlVal = val;
    }
    virtual void setInputMixerGainValuesForMainProgramInput(MixGain *mixergain)
    {
        if (mixergain) {
            memcpy(&mMain1MixGain, mixergain, sizeof(MixGain));
            if (mMain1MixGain.target < -96) {
                mMain1MixGain.target = -96;
            }
            ALOGI("%s() set target %d duration %d shape %d", __FUNCTION__, mMain1MixGain.target, mMain1MixGain.duration, mMain1MixGain.shape);
        }
    }
    virtual void setInputMixerGainValuesFor2ndMainProgramInput(MixGain *mixergain)
    {
        if (mixergain) {
            memcpy(&mMain2MixGain, mixergain, sizeof(MixGain));
            if (mMain2MixGain.target < -96) {
                mMain2MixGain.target = -96;
            }
            ALOGI("%s() set target %d duration %d shape %d", __FUNCTION__, mMain2MixGain.target, mMain2MixGain.duration, mMain2MixGain.shape);
        }
    }
    virtual void setSystemSoundMixerGainValuesForPrimaryInput(MixGain *mixergain)
    {
        if (mixergain) {
            memcpy(&mSysPrimMixGain, mixergain, sizeof(MixGain));
            if (mSysPrimMixGain.target < -96) {
                mSysPrimMixGain.target = -96;
            }
            ALOGI("%s() set target %d duration %d shape %d", __FUNCTION__, mSysPrimMixGain.target, mSysPrimMixGain.duration, mSysPrimMixGain.shape);
        }
    }
    virtual void setSystemSoundMixerGainValuesForAppSoundsInput(MixGain *mixergain)
    {
        if (mixergain) {
            memcpy(&mSysApppsMixGain, mixergain, sizeof(MixGain));
            if (mSysApppsMixGain.target < -96) {
                mSysApppsMixGain.target = -96;
            }
            ALOGI("%s() set target %d duration %d shape %d", __FUNCTION__, mSysApppsMixGain.target, mSysApppsMixGain.duration, mSysApppsMixGain.shape);
        }
    }
    virtual void setSystemSoundMixerGainValuesForSystemSoundsInput(MixGain *mixergain)
    {
        if (mixergain) {
            memcpy(&mSysSyssMixGain, mixergain, sizeof(MixGain));
            if (mSysSyssMixGain.target < -96) {
                mSysSyssMixGain.target = -96;
            }
            ALOGI("%s() set target %d duration %d shape %d", __FUNCTION__, mSysSyssMixGain.target, mSysSyssMixGain.duration, mSysSyssMixGain.shape);
        }
    }

    //DDPLUS SWITCHES
#if 0
    virtual void setDDPAssociatedSubstreamSelection(int val)
    {
        mDdplusAssocSubstream = val;
    }
#endif

    //AC4 SWITCHES
    virtual void setAC4Lang(char *str)
    {
        memset(mAC4Lang, 0, sizeof(mAC4Lang));
        strncpy(mAC4Lang, str, 3);
    }
    virtual void setAC4Lang2(char *str)
    {
        memset(mAC4Lang2, 0, sizeof(mAC4Lang2));
        strncpy(mAC4Lang2, str, 3);
    }
    virtual void setAC4Ac(int val)
    {
        mAC4Ac = val;
    }
    virtual void setAC4Pat(int val)
    {
        mAC4Pat = val;
    }
    virtual void setAC4PresGroupIdx(int val)
    {
        mAC4PresGroupIdx = val;
    }
    virtual void setAC4De(int val)
    {
        mAC4De = val;
    }
    virtual void setAC4ShortProgId(int val)
    {
        mAC4ShortProgId = val;
    }

    //PCM SWITCHES
    virtual void setChnanelConfOfExternalPCMInput(audio_channel_mask_t channel_mask)
    {
        mChannelConfigInExtPCMInput = APPSoundChannelMaskConvertToChannelConfiguration(channel_mask);
    }
    virtual void setLFEpresentInExternalPCMInput(int val)
    {
        mLFEPresentInExtPCMInput = val;
    }
    virtual void setPCMCompressorProfile(int val)
    {
        mCompressorProfile = val;
    }

    //HE-AAC SWITCHES
    virtual void setHEAACAsocciatedInstanceRestrictedTo2Channels(int val)
    {
        mAssocInstanse = val;
    }
    virtual void setHEAACDefaultDialnormValue(int val)
    {
        mDefDialnormVal = val;
    }
    virtual void setDualMonoreproductionMode(int val)
    {
        mDualMonoreproductionMode = val;
    }
    virtual void setAribChannelMappingFlag(int val)
    {
        mAribChannelMappingFlag = val;
    }

    //DAP SWITCHES (device specific)
    virtual void setDAPGains(int val)
    {
        mDAPGains = val;
    }
    virtual void setDAPSurroundDecoderEnable(bool val)
    {
        mDAPSurDecEnable = val;
    }
    virtual void setDAPSurroundVirtualizer(DAPSurroundVirtualizer *dapVirtualizerParamters)
    {
        if (dapVirtualizerParamters) {
            memcpy(&DeviceDAPSurroundVirtualizer, dapVirtualizerParamters, sizeof(DeviceDAPSurroundVirtualizer));
        }
    }
    virtual void setDAPGraphicEQ(DAPGraphicEQ *dapGraphicEQParamters)
    {
        if (dapGraphicEQParamters) {
            memcpy(&DeviceDAPGraphicEQ, dapGraphicEQParamters, sizeof(DeviceDAPGraphicEQ));
        }
    }
    virtual void setDAPBassEnhancer(DAPBassEnhancer *dapBassEnhancerParameters)
    {
        if (dapBassEnhancerParameters) {
            memcpy(&DeviceDAPBassEnhancer, dapBassEnhancerParameters, sizeof(DeviceDAPBassEnhancer));
        }
    }
    virtual void setDAPVirtualBass(DAPVirtualBass *dapVirtualBassParamters)
    {
        if (dapVirtualBassParamters) {
            memcpy(&DeviceDAPVirtualBass, dapVirtualBassParamters, sizeof(DeviceDAPVirtualBass));
        }
    }

    //DAP SWITCHES (content specific)
    virtual void setDAPMIStreering(DAPMISteering *dapMiSteeringParamters)
    {
        if (dapMiSteeringParamters) {
            memcpy(&ContentDAPMISteering, dapMiSteeringParamters, sizeof(ContentDAPMISteering));
        }
    }
    virtual void setDAPLeveler(DAPLeveler *dapLevelerParameters)
    {
        if (dapLevelerParameters) {
            memcpy(&ContentDAPLeveler, dapLevelerParameters, sizeof(ContentDAPLeveler));
        }
    }
    virtual void setDAPIEQ(DAPIEQ *dapIEQParameters)
    {
        if (dapIEQParameters) {
            memcpy(&ContentDAPIEQ, dapIEQParameters, sizeof(ContentDAPIEQ));
        }
    }
    virtual void setDAPDialogueEnhancer(DAPDialogueEnhancer *dapDialogueEnhancerParamters)
    {
        if (dapDialogueEnhancerParamters) {
            memcpy(&ContenDAPDialogueEnhancer, dapDialogueEnhancerParamters, sizeof(ContenDAPDialogueEnhancer));
        }
    }

    /*OTT Processing Graph Begin*/
    virtual int SetOTTProcessingGraphSwitches(char **ConfigParams, int *row_index);

    virtual int SetOTTProcessingGraphSwitchesRuntime(char **ConfigParams, int *row_index);

    virtual void setInputMixerGainValuesForOTTSoundsInput(MixGain *mixergain)
    {
        if (mixergain) {
            memcpy(&mOTTMixGain, mixergain, sizeof(MixGain));
            if (mOTTMixGain.target < -96) {
                mOTTMixGain.target = -96;
            }
            ALOGI("%s() set target %d duration %d shape %d", __FUNCTION__, mOTTMixGain.target, mOTTMixGain.duration, mOTTMixGain.shape);
        }
    }

    virtual void setActiveOTTSignalFlag(bool flag)
    {
        mActivateOTTSignal = flag;
        ALOGI("%s() set mActivateOTTSignal %d", __FUNCTION__, mActivateOTTSignal);
    }


    virtual bool getActiveOTTSignalFlag(void)
    {
        return mActivateOTTSignal;
    }

    virtual void setAtmosLockFlag(bool flag)
    {
        mAtmosLock = flag;
        ALOGI("%s() set mAtmosLock %d", __FUNCTION__, mAtmosLock);
    }

    virtual int getAtmosLockFlag(void)
    {
        return mAtmosLock;
    }

    virtual void setPauseFlag(bool flag)
    {
        mPause = flag;
        ALOGI("%s() set mPause %d", __FUNCTION__, mPause);
    }

    virtual void setDolbyMain1FileNameAsDummy(bool is_dummy)
    {
        mMain1IsDummy = is_dummy;
        if (is_dummy) {
            memcpy(mDolbyMain1FileName, DEFAULT_DUMMY_DDP_FILE_NAME, sizeof(DEFAULT_DUMMY_DDP_FILE_NAME));
        } else {
            memcpy(mDolbyMain1FileName, DEFAULT_MAIN_DDP_FILE_NAME, sizeof(DEFAULT_MAIN_DDP_FILE_NAME));
        }
        ALOGI("%s() is_dummy %d mDolbyMain1FileName %s mMain1IsDummy %d\n", __FUNCTION__, is_dummy, mDolbyMain1FileName, mMain1IsDummy);
    }

    virtual void setDolbyMain2NameAsDummy(bool is_dummy)
    {
        if (is_dummy) {
            memcpy(mDolbyMain2FileName, DEFAULT_DUMMY_DDP_FILE_NAME, sizeof(DEFAULT_DUMMY_DDP_FILE_NAME));
        } else {
            memcpy(mDolbyMain2FileName, DEFAULT_MAIN2_DDP_FILE_NAME, sizeof(DEFAULT_MAIN2_DDP_FILE_NAME));
        }
        ALOGI("%s() is_dummy %d mDolbyMain2FileName %s\n", __FUNCTION__, is_dummy, mDolbyMain2FileName);
    }

    virtual bool getMain1IsDummy(void)
    {
        return mMain1IsDummy;
    }

    virtual void setOTTSoundInputEnable(bool flag)
    {
        mOTTSoundInputEnable = flag;
        ALOGI("%s() mOTTSoundInputEnable %d\n", __FUNCTION__, mOTTSoundInputEnable);
    }

    virtual bool getOTTSoundInputEnable(void)
    {
        return mOTTSoundInputEnable;
    }
    /*OTT Processing Graph End*/

    //*End||Add the APT to set the params*//



protected:

private:
    int ms_get_int_array_from_str(char **p_csv_string, int num_el, int *p_vals);
    int ms_get_int_from_str(char **p_csv_string, int *p_vals);
    // DolbyMS12ConfigParams(const DolbyMS12ConfigParams&);
    // DolbyMS12ConfigParams& operator = (const DolbyMS12ConfigParams&);
    // static DolbyMS12ConfigParams *gInstance;
    // static android::Mutex mLock;
    static std::mutex mLock;
    // audio_devices_t mAudioSteamOutDevices;
    int mParamNum;

    //dolby ms12 input
    audio_output_flags_t mAudioOutFlags;
    audio_format_t mAudioStreamOutFormat;
    audio_channel_mask_t mAudioStreamOutChannelMask;
    int mAudioSteamOutSampleRate;

    //dolby ms12 output

    int mDolbyMS12OutConfig;
    int mDolbyMS12OutSampleRate;
    audio_channel_mask_t mDolbyMS12OutChannelMask;
    char **mConfigParams;//[MAX_ARGC][MAX_ARGV_STRING_LEN];


    // bool mMultiOutputFlag;
    //bool mLowComplexityMode = false;
    int mDRCBoost;
    int mDRCCut;
    int mDRCBoostStereo;
    int mDRCCutStereo;
    int mChannelConfAppSoundsIn;
    int mChannelConfSystemIn;
    bool mMainFlags;//has dd/ddp/he-aac audio
    bool mAppSoundFlags;
    bool mSystemSoundFlags;
    int mDAPInitMode;
    int mDAPVirtualBassEnable;
    int mDBGOut;//(default: none)
    int mDRCModesOfDownmixedOutput;
    int mDAPDRCMode;//for multi-ch and dap output[default is 0]
    int mDownmixMode;//Lt/Rt[val=0, default] or Lo/Ro
    int mEvaluationMode;//default is 0
    int mLFEPresentInAppSoundIn;//default is 1[means on]
    int mLFEPresentInSystemSoundIn;//default is 0[means off]
    int mDonwnmix71PCMto51;//default 0[means off]
    int mLockingChannelModeENC;//0 default, auto; 1 locked as 5.1 channel mode.
    int mRISCPrecisionFlag;//0 less&16bits for 16x32;1 high&32bits for 16x32
    int mDualMonoReproMode;//0-stereo;1-Left or first;2-right or second
    int mVerbosity;//here choose 0
    int mOutputBitDepth;//here choose 16(bit)
    int mAssociatedAudioMixing;//0 off;1 on&default;
    int mSystemAPPAudioMixing;//0 off;1 on&default;
    int mUserControlVal;//-32(mute assoc) <--> 32(mute main)

    //fixme, which params are suitable
    MixGain mMain1MixGain = {
        .target = 0,
        .duration = 0,
        .shape = 0,
    };//Input mixer gain values for Main program input
    MixGain mMain2MixGain = {
        .target = 0,
        .duration = 0,
        .shape = 0,
    };//Input mixer gain values for 2nd Main program input
    MixGain mSysPrimMixGain = {
        .target = 0,
        .duration = 0,
        .shape = 0,
    };//System sound mixer gain values for primary input (Input/AD mixer output)
    MixGain mSysApppsMixGain = {
        .target = 0,
        .duration = 0,
        .shape = 0,
    };//System sound mixer gain values for Application Sounds input
    MixGain mSysSyssMixGain = {
        .target = 0,
        .duration = 0,
        .shape = 0,
    };//System sound mixer gain values for System Sounds input

    //DDPLUS SWITCHES

    //PCM SWITCHES
    int mChannelConfigInExtPCMInput;//Channel configuration of external PCM input, default is 7;
    bool mLFEPresentInExtPCMInput = true;//LFE present in external PCM input
    int mCompressorProfile;//[pcm] Compressor profile

    //AC4 SWITCHES
    char mAC4Lang[4];
    char mAC4Lang2[4];
    int mAC4Ac;//[ac4] Preferred associated type of service, 1:Visually Impaired (VI, default), 2: Hearing Impaired (HI), 3: Commentary
    int mAC4Pat;//[ac4] Prefer Presentation Selection by associated type over language, 0: Prefer selection by language, 1: Prefer selection by associated type (default)
    int mAC4PresGroupIdx;//[ac4] Presentation group index to be decoded. 0>>>510: Presentation group index, -1: switch back to automatic selection by language and associated type (default)
    int mAC4De;//[ac4] Dialogue Enhancement gain [0-12], default 0
    int mAC4ShortProgId;//[ac4] The short program identifier as 16 bit unsigned value or -1 for no program (default)

    //HE-AAC SWITCHES
    int mAssocInstanse;//[he-aac] Associated instance restricted to 2 channels
    int mDefDialnormVal;//[he-aac] Default dialnorm value (dB/4),  0 - 127; Default = 108 (-27dB)
    int mDualMonoreproductionMode;//[he-aac] Dual-mono reproduction mode, 0:Stereo(default), 1:Left/first, 2:Right/second
    int mAribChannelMappingFlag;//[he-aac] ARIB channel mapping flag, 0:standard(default), 1:ARIB (w/o attenuation)

    //DAP SWITCHES (device specific)
    const char *mDAPTuningFile;
    int mDAPGains = 0;//postgain (-2080...480, def: 0)
    bool mDAPSurDecEnable = true;//DAP surround decoder enable flag (Default 1)
    bool mHasAssociateInput = false;
    bool mHasSystemInput = false;
    DAPSurroundVirtualizer DeviceDAPSurroundVirtualizer = {
        .virtualizer_enable = 1,
        .surround_boost = 96,
    };
    DAPGraphicEQ DeviceDAPGraphicEQ = {
        .eq_enable = 0,
        .eq_nb_bands = 10,
        .eq_band_center = {32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000},
        .eq_band_target = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    };
    DAPBassEnhancer DeviceDAPBassEnhancer = {
        .bass_enable = 0,
        .bass_boost = 192,
        .bass_cutoff = 200,
        .bass_width = 16,
    };
    DAPVirtualBass DeviceDAPVirtualBass = {
        .virtual_bass_mode = 0,
        .virtual_bass_low_src_freq = 35,
        .virtual_bass_high_src_freq = 160,
        .virtual_bass_overall_gain = 0,
        .virtual_bass_slope_gain = 0,
        .virtual_bass_subgains = { -32, -144, -192},
        .virtual_bass_low_mix_freq = 94,
        .virtual_bass_high_mix_freq = 469,
    };

    //DAP SWITCHES (content specific)
    DAPMISteering ContentDAPMISteering = {
        .mi_enable = 0,
    };
    DAPLeveler ContentDAPLeveler = {
        .leveler_enable = 0,
        .leveler_amount = 4,
    };
    DAPIEQ ContentDAPIEQ = {
        .ieq_enable = 0,
        .ieq_amount = 10,
        .ieq_nb_bands = 10,
        .ieq_band_center = {32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000},
        .ieq_band_target = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    };
    DAPDialogueEnhancer ContenDAPDialogueEnhancer = {
        .de_enable = 0,
        .de_amount = 0,
        .de_ducking = 0,
    };

    bool mActivateOTTSignal;
    int mChannelConfOTTSoundsIn;
    int mLFEPresentInOTTSoundIn;
    bool mAtmosLock;
    bool mPause;

    char mDolbyMain1FileName[256];
    char mDolbyMain2FileName[256];
    bool mMain1IsDummy;
    bool mOTTSoundInputEnable;

    MixGain mOTTMixGain = {
        .target = 0,
        .duration = 0,
        .shape = 0,
    };//System sound mixer gain values for System Sounds input
}; //class DolbyMS12ConfigParams


}//end of namespace android
#endif

#endif //end of _DOLBY_MS12_CONFIG_PARAMS_H_
