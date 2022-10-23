/****************************************************************************
 *
 *		Target Tuning Symbol File
 *		-------------------------
 *
 *          Generated on:  25-Aug-2017 15:47:30
 *
 ***************************************************************************/

#ifndef AMLOGIC_VUI_SOLUTION_V6A_VOICEONLY_RELEASESIMPLE_H
#define AMLOGIC_VUI_SOLUTION_V6A_VOICEONLY_RELEASESIMPLE_H

// ----------------------------------------------------------------------
// InputMeter [Meter]
// Peak and RMS meter module

#define AWE_InputMeter_ID 30000

// int meterType - Operating mode of the meter. Selects between peak and
//         RMS calculations. See the discussion section for more details.
// Default value: 18
// Range: unrestricted
#define AWE_InputMeter_meterType_OFFSET 8
#define AWE_InputMeter_meterType_MASK 0x00000100
#define AWE_InputMeter_meterType_SIZE -1

// float attackTime - Attack time of the meter. Specifies how quickly
//         the meter value rises.
#define AWE_InputMeter_attackTime_OFFSET 9
#define AWE_InputMeter_attackTime_MASK 0x00000200
#define AWE_InputMeter_attackTime_SIZE -1

// float releaseTime - Release time of the meter. Specifies how quickly
//         the meter decays.
#define AWE_InputMeter_releaseTime_OFFSET 10
#define AWE_InputMeter_releaseTime_MASK 0x00000400
#define AWE_InputMeter_releaseTime_SIZE -1

// float attackCoeff - Internal coefficient that realizes the attack
//         time.
#define AWE_InputMeter_attackCoeff_OFFSET 11
#define AWE_InputMeter_attackCoeff_MASK 0x00000800
#define AWE_InputMeter_attackCoeff_SIZE -1

// float releaseCoeff - Internal coefficient that realizes the release
//         time.
#define AWE_InputMeter_releaseCoeff_OFFSET 12
#define AWE_InputMeter_releaseCoeff_MASK 0x00001000
#define AWE_InputMeter_releaseCoeff_SIZE -1

// float value[10] - Array of meter output values, one per channel.
#define AWE_InputMeter_value_OFFSET 13
#define AWE_InputMeter_value_MASK 0x00002000
#define AWE_InputMeter_value_SIZE 10


// ----------------------------------------------------------------------
// LatencyControlSamps [Delay]
// Time delay in which the delay is specified in samples

#define AWE_LatencyControlSamps_ID 30001

// int maxDelay - Maximum delay, in samples. The size of the delay
//         buffer is (maxDelay+1)*numChannels.
#define AWE_LatencyControlSamps_maxDelay_OFFSET 8
#define AWE_LatencyControlSamps_maxDelay_MASK 0x00000100
#define AWE_LatencyControlSamps_maxDelay_SIZE -1

// int currentDelay - Current delay.
// Default value: 0
// Range: 0 to 6400.  Step size = 1
#define AWE_LatencyControlSamps_currentDelay_OFFSET 9
#define AWE_LatencyControlSamps_currentDelay_MASK 0x00000200
#define AWE_LatencyControlSamps_currentDelay_SIZE -1

// int stateIndex - Index of the oldest state variable in the array of
//         state variables.
#define AWE_LatencyControlSamps_stateIndex_OFFSET 10
#define AWE_LatencyControlSamps_stateIndex_MASK 0x00000400
#define AWE_LatencyControlSamps_stateIndex_SIZE -1

// int stateHeap - Heap in which to allocate memory.
#define AWE_LatencyControlSamps_stateHeap_OFFSET 11
#define AWE_LatencyControlSamps_stateHeap_MASK 0x00000800
#define AWE_LatencyControlSamps_stateHeap_SIZE -1

// float state[6658] - State variable array.
#define AWE_LatencyControlSamps_state_OFFSET 12
#define AWE_LatencyControlSamps_state_MASK 0x00001000
#define AWE_LatencyControlSamps_state_SIZE 6658


// ----------------------------------------------------------------------
// FreqDomainProcessing.Direction [SinkInt]
// Copies the data at the input pin and stores it in an internal buffer

#define AWE_FreqDomainProcessing____Direction_ID 30009

// int value[1] - Captured values
#define AWE_FreqDomainProcessing____Direction_value_OFFSET 8
#define AWE_FreqDomainProcessing____Direction_value_MASK 0x00000100
#define AWE_FreqDomainProcessing____Direction_value_SIZE 1


// ----------------------------------------------------------------------
// FreqDomainProcessing.NoiseReductionDB [Sink]
// Copies the data at the input pin and stores it in an internal buffer.

#define AWE_FreqDomainProcessing____NoiseReductionDB_ID 30010

// int enable - To enable or disable the plotting.
// Default value: 0
// Range: unrestricted
#define AWE_FreqDomainProcessing____NoiseReductionDB_enable_OFFSET 8
#define AWE_FreqDomainProcessing____NoiseReductionDB_enable_MASK 0x00000100
#define AWE_FreqDomainProcessing____NoiseReductionDB_enable_SIZE -1

// float value[1] - Captured values.
#define AWE_FreqDomainProcessing____NoiseReductionDB_value_OFFSET 9
#define AWE_FreqDomainProcessing____NoiseReductionDB_value_MASK 0x00000200
#define AWE_FreqDomainProcessing____NoiseReductionDB_value_SIZE 1

// float yRange[2] - Y-axis range.
// Default value:
//     -5  5
// Range: unrestricted
#define AWE_FreqDomainProcessing____NoiseReductionDB_yRange_OFFSET 10
#define AWE_FreqDomainProcessing____NoiseReductionDB_yRange_MASK 0x00000400
#define AWE_FreqDomainProcessing____NoiseReductionDB_yRange_SIZE 2


// ----------------------------------------------------------------------
// FreqDomainProcessing.SetDirection [SourceInt]
// Source buffer holding 1 wire of integer data

#define AWE_FreqDomainProcessing____SetDirection_ID 30008

// int value[1] - Array of interleaved audio data
// Default value:
//     3
// Range: unrestricted
#define AWE_FreqDomainProcessing____SetDirection_value_OFFSET 8
#define AWE_FreqDomainProcessing____SetDirection_value_MASK 0x00000100
#define AWE_FreqDomainProcessing____SetDirection_value_SIZE 1


// ----------------------------------------------------------------------
// FreqDomainProcessing.ManualDirection [Multiplexor]
// Selects one of N inputs

#define AWE_FreqDomainProcessing____ManualDirection_ID 30007

// int indexPinFlag - Specifies index pin available or not.
#define AWE_FreqDomainProcessing____ManualDirection_indexPinFlag_OFFSET 8
#define AWE_FreqDomainProcessing____ManualDirection_indexPinFlag_MASK 0x00000100
#define AWE_FreqDomainProcessing____ManualDirection_indexPinFlag_SIZE -1

// int index - Specifies which input pin to route to the output. The
//         index is zero based.
// Default value: 1
// Range: 0 to 1
#define AWE_FreqDomainProcessing____ManualDirection_index_OFFSET 9
#define AWE_FreqDomainProcessing____ManualDirection_index_MASK 0x00000200
#define AWE_FreqDomainProcessing____ManualDirection_index_SIZE -1


// ----------------------------------------------------------------------
// AEC_Out_Delay [Delay]
// Time delay in which the delay is specified in samples

#define AWE_AEC_Out_Delay_ID 30004

// int maxDelay - Maximum delay, in samples. The size of the delay
//         buffer is (maxDelay+1)*numChannels.
#define AWE_AEC_Out_Delay_maxDelay_OFFSET 8
#define AWE_AEC_Out_Delay_maxDelay_MASK 0x00000100
#define AWE_AEC_Out_Delay_maxDelay_SIZE -1

// int currentDelay - Current delay.
// Default value: 0
// Range: 0 to 16000.  Step size = 1
#define AWE_AEC_Out_Delay_currentDelay_OFFSET 9
#define AWE_AEC_Out_Delay_currentDelay_MASK 0x00000200
#define AWE_AEC_Out_Delay_currentDelay_SIZE -1

// int stateIndex - Index of the oldest state variable in the array of
//         state variables.
#define AWE_AEC_Out_Delay_stateIndex_OFFSET 10
#define AWE_AEC_Out_Delay_stateIndex_MASK 0x00000400
#define AWE_AEC_Out_Delay_stateIndex_SIZE -1

// int stateHeap - Heap in which to allocate memory.
#define AWE_AEC_Out_Delay_stateHeap_OFFSET 11
#define AWE_AEC_Out_Delay_stateHeap_MASK 0x00000800
#define AWE_AEC_Out_Delay_stateHeap_SIZE -1

// float state[16258] - State variable array.
#define AWE_AEC_Out_Delay_state_OFFSET 12
#define AWE_AEC_Out_Delay_state_MASK 0x00001000
#define AWE_AEC_Out_Delay_state_SIZE 16258


// ----------------------------------------------------------------------
// endIndex [SinkInt]
// Copies the data at the input pin and stores it in an internal buffer

#define AWE_endIndex_ID 30012

// int value[1] - Captured values
#define AWE_endIndex_value_OFFSET 8
#define AWE_endIndex_value_MASK 0x00000100
#define AWE_endIndex_value_SIZE 1


// ----------------------------------------------------------------------
// LatencyPeak [SinkInt]
// Copies the data at the input pin and stores it in an internal buffer

#define AWE_LatencyPeak_ID 30002

// int value[1] - Captured values
#define AWE_LatencyPeak_value_OFFSET 8
#define AWE_LatencyPeak_value_MASK 0x00000100
#define AWE_LatencyPeak_value_SIZE 1


// ----------------------------------------------------------------------
// isTriggered [SinkInt]
// Copies the data at the input pin and stores it in an internal buffer

#define AWE_isTriggered_ID 30003

// int value[1] - Captured values
#define AWE_isTriggered_value_OFFSET 8
#define AWE_isTriggered_value_MASK 0x00000100
#define AWE_isTriggered_value_SIZE 1


// ----------------------------------------------------------------------
// VR_State [SourceInt]
// Source buffer holding 1 wire of integer data

#define AWE_VR_State_ID 30006

// int value[1] - Array of interleaved audio data
// Default value:
//     0
// Range: unrestricted
#define AWE_VR_State_value_OFFSET 8
#define AWE_VR_State_value_MASK 0x00000100
#define AWE_VR_State_value_SIZE 1


// ----------------------------------------------------------------------
// OutputMeter [Meter]
// Peak and RMS meter module

#define AWE_OutputMeter_ID 30005

// int meterType - Operating mode of the meter. Selects between peak and
//         RMS calculations. See the discussion section for more details.
// Default value: 18
// Range: unrestricted
#define AWE_OutputMeter_meterType_OFFSET 8
#define AWE_OutputMeter_meterType_MASK 0x00000100
#define AWE_OutputMeter_meterType_SIZE -1

// float attackTime - Attack time of the meter. Specifies how quickly
//         the meter value rises.
#define AWE_OutputMeter_attackTime_OFFSET 9
#define AWE_OutputMeter_attackTime_MASK 0x00000200
#define AWE_OutputMeter_attackTime_SIZE -1

// float releaseTime - Release time of the meter. Specifies how quickly
//         the meter decays.
#define AWE_OutputMeter_releaseTime_OFFSET 10
#define AWE_OutputMeter_releaseTime_MASK 0x00000400
#define AWE_OutputMeter_releaseTime_SIZE -1

// float attackCoeff - Internal coefficient that realizes the attack
//         time.
#define AWE_OutputMeter_attackCoeff_OFFSET 11
#define AWE_OutputMeter_attackCoeff_MASK 0x00000800
#define AWE_OutputMeter_attackCoeff_SIZE -1

// float releaseCoeff - Internal coefficient that realizes the release
//         time.
#define AWE_OutputMeter_releaseCoeff_OFFSET 12
#define AWE_OutputMeter_releaseCoeff_MASK 0x00001000
#define AWE_OutputMeter_releaseCoeff_SIZE -1

// float value[2] - Array of meter output values, one per channel.
#define AWE_OutputMeter_value_OFFSET 13
#define AWE_OutputMeter_value_MASK 0x00002000
#define AWE_OutputMeter_value_SIZE 2


// ----------------------------------------------------------------------
// startIndex [SinkInt]
// Copies the data at the input pin and stores it in an internal buffer

#define AWE_startIndex_ID 30011

// int value[1] - Captured values
#define AWE_startIndex_value_OFFSET 8
#define AWE_startIndex_value_MASK 0x00000100
#define AWE_startIndex_value_SIZE 1



#endif // AMLOGIC_VUI_SOLUTION_V6A_VOICEONLY_RELEASESIMPLE_H

