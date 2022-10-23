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

#ifndef _DOLBY_MS12_CONFIG_PARAMS_C_H_
#define _DOLBY_MS12_CONFIG_PARAMS_C_H_

#include <stdbool.h>
#include "audio_core.h"
//#include <system/audio_policy.h>
#include "dolby_ms12_config_parameter_struct.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Set assciate flag
 */
void dolby_ms12_config_params_set_associate_flag(bool flag);
/**
 * @brief Get assciate flag
 */
bool dolby_ms12_config_params_get_associate_flag(void);

/**
 * @brief Set system flag
 */
void dolby_ms12_config_params_set_system_flag(bool flag);

/**
 * @brief Get system flag
 */
bool dolby_ms12_config_params_get_system_flag(void);

/**
 * @brief Set input&output parameters

 * @audio_output_flags_t flags //audio stream out flags
 * @audio_format_t input_format //audio stream out format
 * @audio_channel_mask_t channel_mask //audio stream out channel mask
 * @int sample_rate //audio stream out sample rate
 * @audio_format_t output_format //dolby ms12 output format[ec3/ac3/pcm]
 */
void dolby_ms12_config_params_set_audio_stream_out_params(
    audio_output_flags_t flags
    , audio_format_t input_format
    , audio_channel_mask_t channel_mask
    , int sample_rate
    , audio_format_t output_format);

/**
 * @brief Set the surround sound to DolbyMS12GetOutProfile
 */
bool dolby_ms12_config_params_set_surround_sound_by_out_profile(void);

// /*config params begin*/
// /**/
// int dolby_ms12_config_params_set_input_output_file_name(char **ConfigParams, int *row_index);
// int dolby_ms12_config_params_set_functional_switches(char **ConfigParams, int *row_index);
// int dolby_ms12_config_params_set_ddplus_switches(char **ConfigParams, int *row_index);
// int dolby_ms12_config_params_set_pcm_switches(char **ConfigParams, int *row_index);
// int dolby_ms12_config_params_set_heaac_switches(char **ConfigParams, int *row_index);
// int dolby_ms12_config_params_set_dap_device_switches(char **ConfigParams, int *row_index);
// int dolby_ms12_config_params_set_dap_content_switches(char **ConfigParams, int *row_index);
// /*config params end*/

/**
 * @brief Get the dolby_ms12_init() input params
 * @int *argc //dolby_ms12_init *argc

 * @return the char **argv
 */
char **dolby_ms12_config_params_get_config_params(int *argc);

/**
 * @brief Get the dolby_ms12_update_runtime_params() input params
 * @int *argc //dolby_ms12_init *argc

 * @return the char **argv
 */
char **dolby_ms12_config_params_get_runtime_config_params(int *argc);
//char **dolby_ms12_config_params_prepare_config_params(int max_raw_size, int max_column_size);
//void dolby_ms12_config_params_cleanup_config_params(char **ConfigParams, int max_raw_size);

/**
 * @brief Get dolby ms12 output format
 */
audio_format_t dolby_ms12_config_params_get_dobly_config_output_format(void);

/**
 * @brief Get dolby ms12 output sample rate
 */
int dolby_ms12_config_params_get_dolby_config_output_samplerate(void);

/**
 * @brief Get dolby ms12 output channel mask
 */
audio_channel_mask_t dolby_ms12_config_params_get_dolby_config_output_channelmask(void);

/**
 * @brief reset the dolby_ms12_init() input params
 */
void dolby_ms12_config_params_reset_config_params(void);

/**
 * @brief cleanup the ms12 config params
 */
void dolby_ms12_config_params_self_cleanup(void);

/*Begin*/
//*Begin||Add the APT to set the params*//
//Functional Switches
//virtual void setLowComplexityNode(bool flag) { mLowComplexityMode = flag; }//did not use this part!
/**
 * @brief set DRC boost value
 *
 * int val//0 - 100; Default = 100
 */
void dolby_ms12_set_drc_boost(int val);

/**
 * @brief set DRC cut value
 * int val//0 - 100; Default = 100
 */
void dolby_ms12_set_drc_cut(int val);

/**
 * @brief set DRC boost value for 2-channel downmix
 * int val//0 - 100; Default = 100
 */
void dolby_ms12_set_drc_boost_system(int val);

/**
 * @brief set DRC cut value for 2-channel downmix
 * int val//0 - 100; Default = 100
 */
void dolby_ms12_set_drc_cut_system(int val);

/**
 * @brief Channel configuration of Application Sounds input
 */
void dolby_ms12_set_channel_config_of_app_sound_input(audio_channel_mask_t channel_mask);

/**
 * @brief Channel configuration of System Sounds input
 */
void dolby_ms12_set_channel_config_of_system_sound_input(audio_channel_mask_t channel_mask);

/**
 * @brief DAPv2 initialisation mode
 * 0 = DAPv2 instances are not present (default)
 * 1 = DAPv2 content processing instance is present only
 * 2 = DAPv2 device processing instance is present only
 * 3 = DAPv2 both content and device processing instances are present
 * 4 = DAPv2 single instance processing (both content and device
 */
void dolby_ms12_set_dap2_initialisation_mode(int val);

/**
 * @brief DAPv2 Virtual Bass enable (additional delay)
 * 0 = DAPv2 Virtual Bass is not enabled (default)
 * 1 = DAPv2 Virtual Bass is enabled
 */
void dolby_ms12_set_dap2_virtual_bass_enable(bool flag);

/**
 * @brief DRC modes (for downmixed output)
 * 0 = Line (Default)
 * 1 = RF
 */
void dolby_ms12_set_drc_mode(int val);

/**
 * @brief DAP DRC mode (for multichannel and DAP output)
 * 0 = Line (Default)
 * 1 = RF
 */
void dolby_ms12_set_dap_drc_mode(int val);

/**
 * @brief Downmix modes
 * 0 = Lt/Rt (Default)
 * 1 = Lo/Ro
 * [he-aac] 2 = ARIB
 */
void dolby_ms12_set_downmix_modes(int val);

/**
 * @brief Evaluation Mode (periodically mutes the signal)
 * 0 = off (default)
 * 1 = on
 */
void dolby_ms12_set_evalution_mode(int val);

/**
 * @brief LFE present in Application Sounds input
 * 0 = off
 * 1 = on (default)
 */
void dolby_ms12_set_lfe_present_in_app_sounds_in(int val);

/**
 * @brief LFE present in System Sounds input
 * 0 = off
 * 1 = on (default)
 */
void dolby_ms12_set_lfe_present_in_system_sounds_in(int val);

/**
 * @brief Maximum number of channels in the signal chain (6 or 8)
 */
void dolby_ms12_set_maximum_number_of_channels_in_the_signal_chain(int val);

/**
 * @brief Downmix 7.1 PCM signal to 5.1 on the multichannel outputs
 * 0 = off(default)
 * 1 = on
 */
void dolby_ms12_set_donwmix_51_pcm_to_51_on_multi_outputs(int val);

/**
 * @brief Encoder channel mode locking mode:
 * 0 = Auto: Channel mode is driven by the input channel mode
 * 1 = 5.1 Locked: Channel mode is always 5.1. [amlogic default]
 */
void dolby_ms12_set_encoder_channel_mode_locking_mode(int val);

/**
 * @brief RISC precision flag (fixpoint backends only)
 * 0 = PCM data has less precision (e.g. 16 bit for 16x32)
 * 1 = PCM data has high precision (default, e.g. 32 bit for 16x32)
 */
void dolby_ms12_set_risc_precision_flag(int val);

/**
 * @brief Dual-mono reproduction mode
 * 0 = Stereo (Default);
 * 1 = Left / first channel;
 * 2 = Right / second channel
 */
void dolby_ms12_set_dual_mono_reproduction_mode(int val);

/**
 * @brief Associated audio mixing
 * 0 = Off
 * 1 = On (Default)
 */
void dolby_ms12_set_asscociated_audio_mixing(int val);

/**
 * @brief System / Application audio mixing
 * 0 = Off
 * 1 = On (Default)
 */
void dolby_ms12_set_system_app_audio_mixing(int val);

/**
 * @brief get the status of System / Application audio mixing
 * 0 = Off
 * 1 = On
 */
int dolby_ms12_get_system_app_audio_mixing(void);

/**
 * @brief User control values for mixing main and associated audio
 * Range: -32 (mute assoc) to 32 (mute main) in steps of 1 (dB)
 * 1 ... 31   - dB to favor associated (attenuate main)
 * -1 ... -31 - dB to favor main (attenuate associated)
 */
void dolby_ms12_set_user_control_value_for_mixing_main_and_associated_audio(int val);

/**
 * @brief Input mixer gain values for Main program input
 * - target gain at end of ramp in dB (range: -96..0)
 * - duration of ramp in milliseconds (range: 0..60000)
 * - shape of the ramp (0: linear, 1: in cube, 2: out cube)
 */
void dolby_ms12_set_input_mixer_gain_values_for_main_program_input(MixGain *mixergain);

/**
 * @brief Input mixer gain values for 2nd Main program input
 * - target gain at end of ramp in dB (range: -96..0)
 * - duration of ramp in milliseconds (range: 0..60000)
 * - shape of the ramp (0: linear, 1: in cube, 2: out cube)
 */
void dolby_ms12_set_input_mixer_gain_values_for_2nd_main_program_input(MixGain *mixergain);

/**
 * @brief System sound mixer gain values for primary input (Input/AD mixer output)
 * - target gain at end of ramp in dB (range: -96..0)
 * - duration of ramp in milliseconds (range: 0..60000)
 * - shape of the ramp (0: linear, 1: in cube, 2: out cube)
 */
void dolby_ms12_set_system_sound_mixer_gain_values_for_primary_input(MixGain *mixergain);

/**
 * @brief System sound mixer gain values for Application Sounds input
 * - target gain at end of ramp in dB (range: -96..0)
 * - duration of ramp in milliseconds (range: 0..60000)
 * - shape of the ramp (0: linear, 1: in cube, 2: out cube)
 */
void dolby_ms12_set_system_sound_mixer_gain_values_for_app_sounds_input(MixGain *mixergain);

/**
 * @brief System sound mixer gain values for System Sounds input
 * - target gain at end of ramp in dB (range: -96..0)
 * - duration of ramp in milliseconds (range: 0..60000)
 * - shape of the ramp (0: linear, 1: in cube, 2: out cube)
 */
void dolby_ms12_set_system_sound_mixer_gain_values_for_system_sounds_input(MixGain *mixergain);


//DDPLUS SWITCHES
/**
 * @brief [ddplus] Associated substream selection
 * 1 - 3; No default
 */
void dolby_ms12_set_ddp_associated_substream_selection(int val);

//PCM SWITCHES
/**
 * @brief Channel configuration of external PCM input
 * 0 = reserved
 * 1 = 1/0 (C)
 * 2 = 2/0 (L, R)
 * 7 = 3/2 (L, C, R, l, r) (default)
 * 21 = 3/2/2 (L, C, R, l, r, Lrs, Rrs)
 */
void dolby_ms12_set_channel_config_of_external_pcm_input(int val);

/**
 * @brief LFE present in external PCM input
 * 0 = off
 * 1 = on (default)
 */
void dolby_ms12_set_lfe_present_in_external_pcm_input(int val);

/**
 * @brief [pcm] Compressor profile
 * 0 [clipping protection only]
 * 1 [film-standard, default]
 * 2 [film-light]
 * 3 [music-standard]
 * 4 [music-light]
 * 5 [speech]
 */
void dolby_ms12_set_pcm_compressor_profile(int val);


//HE-AAC SWITCHES

/**
 * @brief [he-aac] Associated instance restricted to 2 channels
 */
void dolby_ms12_set_heaac_associated_instance_restricted_to_2channels(int val);

/**
 * @brief [he-aac] Default dialnorm value (dB/4)
 * 0 - 127; Default = 108 (-27dB)
 */
void dolby_ms12_set_heaac_default_dialnorm_value(int val);

/**
 * @brief [he-aac] Set transport format
 * 0 = auto-detect (Default)
 * 1 = ADTS
 * 2 = LOAS
 * 3 = RAW (Default for file playback)
 */
void dolby_ms12_set_heaac_tranport_format(int val);


//DAP SWITCHES (device specific)

/**
 * @brief dap calibration boost
 * (0...192, def: 0)
 */
void dolby_ms12_set_dap_calibration_boost(int val);

/**
 * @brief DAP Downmix mode
 * 0 = Lt/Rt (Default)
 * 1 = Lo/Ro
 */
void dolby_ms12_set_dap_downmix_mode(int val);

/**
 * @brief dap gains
 * postgain (-2080...480, def: 0)
 */
void dolby_ms12_set_dap_gains(int val);

/**
 * @brief DAP surround decoder enable flag (Default 1)
 */
void dolby_ms12_set_dap_surround_decoder_enable(bool val);

/**
 * @brief Virtualizer Parameter
 * - virtualizer_enable (0,1, def: 1)
 * - headphone_reverb (-2080...192, def: 0)
 * - speaker_angle (5..30, def: 10)
 * - speaker_start (20...2000, def: 20)
 * - surround_boost (0...96, def: 96)
 */
void dolby_ms12_set_dap_surround_virtuallizer(DAPSurroundVirtualizer *dapVirtualizerParamters);

/**
 * @brief dap graphic eq
 * - eq_enable (0,1, def: 0)
 * - eq_nb_bands (1...20, def: 10)
 * - eq_band_center (20...20000, def: {32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000})
 * - eq_band_target (-576...576, def: {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
 */
void dolby_ms12_set_dap_graphic_eq(DAPGraphicEQ *dapGraphicEQParamters);

/**
 * @brief dap optimizer
 * - optimizer_enable (0,1, def: 0)
 * - opt_nb_bands (1...20, def: 10)
 * - opt_band_center_freq (20...20000, def: {32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000})
 * - opt_band_gains[MAX_CHANNELS] (-480...480, def: {{10*0, 10*0, 10*0, 10*0, 10*0, 10*0, 10*0, 10*0})
 */
void dolby_ms12_set_dap_optimizer(DAPOptimizer *dapOptimizerParamters);

/**
 * @brief dap bass enhancer
 * - bass_enable (0,1, def: 0)
 * - bass_boost (0...384, def: 192)
 * - bass_cutoff (20...20000, def: 200)
 * - bass_width (2..64, def: 16)
 */
void dolby_ms12_set_dap_bass_enhancer(DAPBassEnhancer *dapBassEnhancerParameters);

/**
 * @brief dap regulator
 * - regulator_enable (0,1, def: 1)
 * - regulator_mode (0,1, def: 0)
 * - regulator_overdrive (0...192, def: 0)
 * - regulator_timbre (0...16, def: 16)
 * - regulator_distortion (0...144, def: 96)
 * - reg_nb_bands (1...20, def: 2)
 * - reg_band_center (20...20000, def: {20,20000}
 * - reg_low_thresholds (-2080...0, def: {-192, -192})
 * - reg_high_thresholds (-2080...0, def: {0, 0})
 * - reg_isolated_bands (0,1 def: {0,0})
 */
void dolby_ms12_set_dap_regulator(DAPRegulator *dapRegulatorParamters);

/**
 * @brief dap virtual bass
 * - virtual_bass_mode (0...3, def: 0)
 * - virtual_bass_low_src_freq (30...90, def: 35)
 * - virtual_bass_high_src_freq (90...270, def: 160)
 * - virtual_bass_overall_gain(-480...0, def: 0)
 * - virtual_bass_slope_gain (-3...0, def: 0)
 * - virtual_bass_subgains[3] (-480...0, def: {-32,-144,-192})
 * - virtual_bass_low_mix_freq (0...375, def: 94)
 * - virtual_bass_high_mix_freq (281...938, def: 469)
 */
void dolby_ms12_set_dap_virtual_bass(DAPVirtualBass *dapVirtualBassParamters);

//DAP SWITCHES (content specific)

/**
 * @brief dap Media Intelligence Steering
 * - mi_ieq_enable (0,1, def: 0)
 * - mi_dv_enable (0,1, def: 0)
 * - mi_de_enable (0,1, def: 0)
 * - mi_surround_enable (0,1, def: 0)
 */
void dolby_ms12_set_dap_mi_streering(DAPMISteering *dapMiSteeringParamters);

/**
 * @brief dap leveler
 * - leveler_enable (0,1, def: 0)
 * - leveler_amount (0...10, def: 7)
 * - leveler_ignore_il (0, 1, def: 0)
 */
void dolby_ms12_set_dap_leveler(DAPLeveler *dapLevelerParameters);

/**
 * @brief dap ieq
 * - ieq_enable (0,1, def: 0)
 * - ieq_amount (0...16, def: 10)
 * - ieq_nb_bands (1...20, def: 10)
 * - ieq_band_center (20...20000, def: {32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000})
 * - ieq_band_target (-480...480, def: {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
 */
void dolby_ms12_set_dap_ieq(DAPIEQ *dapIEQParameters);

/**
 * @brief dap dialogue enhancer
 * - de_enable (0,1, def: 0)
 * - de_amount (0...16, def: 0)
 * - de_ducking (0...16, def: 0)
 */
void dolby_ms12_set_dap_dialogue_enhancer(DAPDialogueEnhancer *dapDialogueEnhancerParamters);

/**
 * @brief set dual output flag, when hdmi-arc not connected, and using the dolby ms12, that optical is always on.
 */
void dolby_ms12_set_dual_output_flag(bool need_dual_output);


/**
 * @brief set active ott singal flag(continuous output mode)
 */
void set_dolby_ms12_continuous_mode(bool flag);

/**
 * @brief get active ott singal flag(continuous output mode)
 */
bool is_dolby_ms12_continuous_mode(void);

/**
 * @brief Input mixer gain values for OTT Sounds input
 * - target gain at end of ramp in dB (range: -96..0)
 * - duration of ramp in milliseconds (range: 0..60000)
 * - shape of the ramp (0: linear, 1: in cube, 2: out cube)
 */
void dolby_ms12_set_input_mixer_gain_values_for_ott_sounds_input(MixGain *mixergain);

/**
 * @brief set active atmos lock flag, when hwsync(tunnel mode) is enable
 */
void dolby_ms12_set_atmos_lock_flag(bool flag);

/**
 * @brief set active pause flag, when hwsync(tunnel mode) is enable
 */
void dolby_ms12_set_pause_flag(bool flag);


/**
 * @brief set dolby main input file

   @bool is_dummy, true no input, false real input
 */
void dolby_ms12_set_dolby_main1_as_dummy_file(bool is_dummy);

/**
 * @brief set dolby associate input file

   @bool is_dummy, true no input, false real input
 */
void dolby_ms12_set_dolby_main2_as_dummy_file(bool is_dummy);

/**
 * @brief get dolby main1 input file status
   @if dummy input, return true;
   @if true input, return false;
 */
bool dolby_ms12_get_dolby_main1_file_is_dummy(void);

/**
 * @brief set ott sound input enable

   @bool flag, true ott sound input, false no ott sound input
 */
void dolby_ms12_set_ott_sound_input_enable(bool flag);

/**
 * @brief get ott sound input enable
 */
bool dolby_ms12_get_ott_sound_input_enable(void);
/*End*/

#ifdef __cplusplus
}
#endif

#endif //end of _DOLBY_MS12_CONFIG_PARAMS_C_H_
