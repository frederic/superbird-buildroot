#ifndef LIB_BASSMANAGEMENT_H
#define LIB_BASSMANAGEMENT_H

int aml_bm_lowerpass_process(const void *buffer
                    , size_t bytes
                    , int sample_num
                    , int channel_num
                    , int lfe_index
                    , float *coef_table /*Number of array elements <= AML_MAX_CHANNELS*/
                    , int bit_width/*reserved*/
                    );
int aml_bass_management_init(int param_index);

#endif

