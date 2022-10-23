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

#ifndef _DOLBY_MS12_CONFIG_PARAMMETER_STRUCT_H_
#define _DOLBY_MS12_CONFIG_PARAMMETER_STRUCT_H_

typedef struct{
    int target;//At end of ramp in dB [-96, 0]
    int duration;//in ms [0, 60000]
    int shape;//0: linear, 1: in cube, 2: out cube
}MixGain;

typedef struct
{
    int virtualizer_enable;//def 1
    int headphone_reverb;//[-2080, 192], def 0
    int speaker_angle;//[5,30], def 10
    int speaker_start;//[20, 2000], def 20
    int surround_boost;//[0,96], def 96
}DAPSurroundVirtualizer;

typedef struct
{
    int eq_enable;//(0,1, def: 0)
    int eq_nb_bands;//(1...20, def: 10)
    int eq_band_center[20];//(20...20000, def: {32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000})
    int eq_band_target[20];//(-576...576, def: {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
}DAPGraphicEQ;


typedef struct
{
    int optimizer_enable;//(0,1, def: 0)
    int opt_nb_bands;//(1...20, def: 10)
    int opt_band_center_freq[20];//(20...20000, def: {32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000})
    int opt_band_gains[8*20];//(-480...480, def: {10*0, 10*0, 10*0, 10*0, 10*0, 10*0, 10*0, 10*0})
}DAPOptimizer;

typedef struct
{
    int bass_enable;//(0,1, def: 0)
    int bass_boost;//(0...384, def: 192)
    int bass_cutoff;//(20...20000, def: 200)
    int bass_width;//(2..64, def: 16)
}DAPBassEnhancer;

typedef struct
{
    int regulator_enable;// (0,1, def: 1)
    int regulator_mode;//(0,1, def: 0)
    int regulator_overdrive;//(0...192, def: 0)
    int regulator_timbre;//(0...16, def: 16)
    int regulator_distortion;//(0...144, def: 96)
    int reg_nb_bands;//(1...20, def: 2)
    int reg_band_center[20];//(20...20000, def: {20,20000}
    int reg_low_thresholds[20];//(-2080...0, def: {-192, -192})
    int reg_high_thresholds[20];//(-2080...0, def: {0, 0})
    int reg_isolated_bands[20];//(0,1 def: {0,0})
}DAPRegulator;

typedef struct
{
    int virtual_bass_mode;//(0...3, def: 0)
    int virtual_bass_low_src_freq;//(30...90, def: 35)
    int virtual_bass_high_src_freq;//(90...270, def: 160)
    int virtual_bass_overall_gain;//(-480...0, def: 0)
    int virtual_bass_slope_gain;//(-3...0, def: 0)
    int virtual_bass_subgains[3];//(-480...0, def: {-32,-144,-192})
    int virtual_bass_low_mix_freq;//(0...375, def: 94)
    int virtual_bass_high_mix_freq;//(281...938, def: 469)
}DAPVirtualBass;

typedef struct
{
    int mi_ieq_enable;//(0,1, def: 0)
    int mi_dv_enable;//(0,1, def: 0)
    int mi_de_enable;//(0,1, def: 0)
    int mi_surround_enable;//(0,1, def: 0)
}DAPMISteering;

typedef struct
{
    int leveler_enable;//(0,1, def: 0)
    int leveler_amount;//(0...10, def: 7)
    int leveler_ignore_il;//(0, 1, def: 0)
}DAPLeveler;

typedef struct
{
    int ieq_enable;//(0,1, def: 0)
    int ieq_amount;//(0...16, def: 10)
    int ieq_nb_bands;//(1...20, def: 10)
    int ieq_band_center[20];//(20...20000, def: {32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000})
    int ieq_band_target[20];//(-480...480, def: {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
}DAPIEQ;

typedef struct
{
    int de_enable;// (0,1, def: 0)
    int de_amount;//(0...16, def: 0)
    int de_ducking;//(0...16, def: 0)
}DAPDialogueEnhancer;

#endif //end of _DOLBY_MS12_CONFIG_PARAMMETER_STRUCT_H_
