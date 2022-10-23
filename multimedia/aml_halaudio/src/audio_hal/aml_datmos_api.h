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

#ifndef _AML_DATMOS_API_H_
#define _AML_DATMOS_API_H_

#include <stdbool.h>
#include <audio_hal.h>
#include <cutils/str_parms.h>
#include "aml_dec_api.h"

#ifdef DATMOS

#define CONFIG_MAX 256


struct aml_datmos_param {
    bool is_dolby_atmos;
    void *fd;
    int (*get_audio_info)(void *, int *, int *, int *);
    int (*aml_atmos_init)(unsigned int, void **, const int, const char **);
    int (*aml_atmos_process)(void *, unsigned int, void *, size_t *, char *, unsigned int , size_t *);
    void (*aml_atmos_cleanup)(void *);
    int (*aml_get_output_info)(void *, int *, int *, int *);
    int (*aml_datmos_dynamic_parameter_set)(void *, const int, const char **);
    char speaker_config[CONFIG_MAX];
    bool directdec;
    char drc_config[CONFIG_MAX];
    char virt_config[CONFIG_MAX];
    bool post;
    int mode;
    int vmcal;
    bool hfilt;
    bool nolm;
    bool noupmix;
    bool novlamp;
    bool verbose;
    bool noupresampler;
    bool dec_joc;
    char dapcustomize[CONFIG_MAX];
    bool b_dap_customize;
    char inputpcm_config[CONFIG_MAX];
};

typedef enum
{
    AML_DATMOS_PARAMS_NONE = -1,
    AML_DATMOS_PARAMS_ID_UPMIX_ENABLE,
    AML_DATMOS_PARAMS_ID_CENTER_SPREAD_ENABLE,
    AML_DATMOS_PARAMS_ID_LOUDNESS_MANAGEMENT_ENABLE,
    AML_DATMOS_PARAMS_ID_POST_PROCESSING_ENABLE,
    AML_DATMOS_PARAMS_ID_HEIGHT_FILTER_ENABLE,
    AML_DATMOS_PARAMS_ID_AUDIO_PROCESSING_MODE,
    AML_DATMOS_PARAMS_ID_DRC_MODE,
    AML_DATMOS_PARAMS_ID_DRC_SCALE_FACOTR,
    AML_DATMOS_PARAMS_ID_VOLUME_MODELER_CALIBRATION,
    AML_DATMOS_PARAMS_ID_VLAMP,
    AML_DATMOS_HT_PARAMS_ID_DAP_CUSTOMIZE,
    AML_DATMOS_HT_PARAMS_ID_DAP_CUSTOMIZE_FACTOR,
    AML_DATMOS_PARAMS_ID_LAST
} aml_datmos_dynamic_params_id;

struct dolby_atmos_dec {
    aml_dec_t  aml_dec;
    int (*get_audio_info)(void *, int *, int *, int *);
    int (*aml_atmos_init)(unsigned int, void **, const int, const char **);
    int (*aml_atmos_process)(void *, unsigned int, void *, size_t *, char *, unsigned int , size_t *);
    void (*aml_atmos_cleanup)(void *);
    int (*aml_get_output_info)(void *, int *, int *, int *);
    int (*aml_datmos_dynamic_parameter_set)(void *, const int, const char **);
    int is_truehd_within_mat;
    int is_dolby_atmos;
    int audio_samplerate;
    int system_latency;
};
/*
 *@brief datmos set parameters
 * input params:
 *          struct audio_hw_device *dev: audio_hw_device handle
 *           struct str_parms *parms: parameters string
 *
 * return value:
 *          0, success set datmos' parameters
 *         -1, the parameters is not listed in datmos
 */
int datmos_set_parameters(struct audio_hw_device *dev, struct str_parms *parms);

/*
 *@brief datmos get parameters
 * input params:
 *          struct audio_hw_device *dev: audio_hw_device handle
 *          const char *keys: parameter keys
            char *temp_buf: output temp_buf
            size_t temp_buf_size: size of temp_buf(bytes)
 *
 * return value:
 *          0, success get parameters about datmos
 *         -1, the parameters is not listed in datmos
 */
int datmos_get_parameters(struct audio_hw_device *dev, const char *keys, char *temp_buf, size_t temp_buf_size);

/*
 *@brief get datmos func
 * input params:
 *          struct aml_datmos_param *datmos_handle: aml_datmos_param handle
 *
 * return value:
 *          0, success get datmos function
 *          1, fail to get datmos function
 */
int get_datmos_func(struct aml_datmos_param *datmos_handle);

/*
 *@brief cleanup datmos func
 * input params:
 *          struct aml_datmos_param *datmos_handle: aml_datmos_param handle
 *
 * return value:
 *          0, success cleanup datmos function
 */
int cleanup_atmos_func(struct aml_datmos_param *datmos_handle);
/*
 *@brief datmos decoder dynamic param set
 * input params:
 *          struct aml_datmos_param *datmos_handle: aml_datmos_param handle
 *
 * return value:
 *          0, success
 *          other, error occur
 */
int datmos_decoder_dynamic_param_set_patch(aml_dec_t *aml_dec);
extern aml_dec_func_t aml_datmos_func;
#endif

#endif//end of _AML_DATMOS_API_H_

