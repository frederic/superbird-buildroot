/*
 * Copyright (C) 2018 Amlogic Corporation.
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

#define LOG_TAG "aml_datmos_api"

#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/prctl.h>
#include <cutils/str_parms.h>
#include <dlfcn.h>

#include "log.h"
#include "aml_datmos_api.h"
#include "audio_hw.h"
#include "aml_datmos_config.h"
#include "audio_core.h"
#include "aml_audio_log.h"

/*marco define*/
#define VAL_LEN 256
#define MAX_DECODER_MAT_FRAME_LENGTH 61424
#define MAX_DECODER_DDP_FRAME_LENGTH 0X1800
#define MAX_DECODER_THD_FRAME_LENGTH 8190

#define AC3_FRAMELENGTH   1536

#define DATMOS_HT_OK                  0
#define DATMOS_HT_ENOMEM              1
#define DATMOS_HT_EINVAL_PARAM        2
#define DATMOS_HT_EINVAL_CONFIG       3
#define DATMOS_HT_EPROC_GENERAL       4
#define DATMOS_HT_EPROC_STREAM        5
#define DATMOS_HT_EPROC_NOT_PREROLLED 6

#define DATMOS_HT_DDP_PROC_INIT_SIZE (6144)
#define ONE_BLOCK_FRAME_SIZE 256
#define MAX_BLOCK_NUM 32
#define AUDIO_FORMAT_STRING(format) ((format) == TRUEHD) ? ("TRUEHD") : (((format) == AC3) ? ("AC3") : (((format) == EAC3) ? ("EAC3") : ("LPCM")))

#define DATMOS_PCM_OUT_FILE "/tmp/datmos_pcm_out.data"

#define DATMOS_RAW_IN_FILE "/tmp/datmos_raw_in.data"
/*global parameters*/

/*static parameters*/
typedef enum {
    DAP_Reserved = 0x00,
    DAP_Standard,
    DAP_Movie,
    DAP_Music,
    DAP_Night,
} GUI_DAP_VALUE;

typedef enum {
    DRC_OFF = 0x00,
    DRC_ON,
    DRC_AUTO,
} GUI_DRC_VALUE;

/*basic fuction*/
static int convert_format (unsigned char *buffer, int size){
    int idx;
    unsigned char tmp;

    for (idx = 0; idx < size;)
    {
        //buffer[idx] = (unsigned short)BYTE_REV(buffer[idx]);
        tmp = buffer[idx];
        buffer[idx] = buffer[idx + 1];
        buffer[idx + 1] = tmp;
        idx = idx + 2;
    }
    return size;
}
/*datmos api*/
#ifdef DATMOS
int datmos_set_parameters(struct audio_hw_device *dev, struct str_parms *parms)
{
    char value[VAL_LEN];
    int val = 0;
    int ret = 0;
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    void *opts = NULL;
    if (adev->datmos_enable)
        opts = get_datmos_current_options();

    if (!adev || !parms) {
        ALOGE("Fatal Error adev %p parms %p", adev, parms);
        goto error_exit;
    }

    /*static param*/
    ret = str_parms_get_str(parms, "speakers", value, sizeof(value));
    if (ret >= 0) {
        memset(adev->datmos_param.speaker_config, 0, sizeof(adev->datmos_param.speaker_config));
        strncpy(adev->datmos_param.speaker_config, value, strlen(value));
        ALOGI("speaker_config set to %s\n", adev->datmos_param.speaker_config);
        /*datmos parameter*/
        if (adev->datmos_enable)
            add_datmos_option(opts, "-speakers", adev->datmos_param.speaker_config);
        return 0;
    }

    /*static param*/
    ret = str_parms_get_str(parms, "directdec", value, sizeof(value));
    if (ret >= 0) {
        ALOGI("get value %s\n", value);
        if (strncmp(value, "enable", 6) == 0 || strncmp(value, "1", 1) == 0) {
            adev->datmos_param.directdec = 1;
        } else if (strncmp(value, "disable", 7) == 0 || strncmp(value, "0", 1) == 0) {
            adev->datmos_param.directdec = 0;
        } else {
            ALOGE("%s() unsupport directdec value: %s",   __func__, value);
        }
        ALOGI("directdec set to %d\n", adev->datmos_param.directdec);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            if (adev->datmos_param.directdec) {
                char directdec_config[12];
                sprintf(directdec_config, "%d", adev->datmos_param.directdec);
                add_datmos_option(opts, "-directdec", "");
            }
            else
                delete_datmos_option(opts, "-directdec");
        }
        return 0;
    }

    /*dynamic param*/
    ret = str_parms_get_str(parms, "drc", value, sizeof(value));
    if (ret >= 0) {
        ALOGI("get value %s\n", value);
        if (strcmp(adev->datmos_param.drc_config, value)) {
            adev->dec_params_update_mask |=  (1 << AML_DATMOS_PARAMS_ID_DRC_MODE);
            adev->dec_params_update_mask |=  (1 << AML_DATMOS_PARAMS_ID_DRC_SCALE_FACOTR);
        }
        memset(adev->datmos_param.drc_config, 0, sizeof(adev->datmos_param.drc_config));
        strncpy(adev->datmos_param.drc_config, value, strlen(value));
        ALOGI("drc_config set to %s\n", adev->datmos_param.drc_config);
        int drc_mode = DRC_OFF;
        if (strstr(adev->datmos_param.drc_config, "enable"))
            drc_mode = DRC_ON;
        else if (strstr(adev->datmos_param.drc_config, "auto"))
            drc_mode = DRC_AUTO;
        /*datmos parameter*/
        if (adev->datmos_enable) {
            add_datmos_option(opts, "-drc", adev->datmos_param.drc_config);
            /*
             *If Loudness management is disabled,
             *then DRC and postprocessing must also be disabled.
             */
            if ((drc_mode == DRC_ON) || (drc_mode == DRC_AUTO)) {
                delete_datmos_option(opts, "-post");
                delete_datmos_option(opts, "-nolm");
            }
            else if (adev->datmos_param.post) {
                delete_datmos_option(opts, "-nolm");
                add_datmos_option(opts, "-post", "");
            }
            else if (adev->datmos_param.nolm) {
                delete_datmos_option(opts, "-post");
                add_datmos_option(opts, "-nolm", "");
            }
        }
        return 0;
    }

    /*static param*/
    ret = str_parms_get_str(parms, "virt", value, sizeof(value));
    if (ret >= 0) {
        ALOGI("get value %s\n", value);
        memset(adev->datmos_param.virt_config, 0, sizeof(adev->datmos_param.virt_config));
        strncpy(adev->datmos_param.virt_config, value, strlen(value));
        ALOGI("virt_config set to %s\n", adev->datmos_param.virt_config);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            add_datmos_option(opts, "-virt", adev->datmos_param.virt_config);
        }
        return 0;
    }

    /*dynamic param*/
    ret = str_parms_get_int(parms, "post", &val);
    if (ret >= 0) {
        if (adev->datmos_param.post != val)
            adev->dec_params_update_mask |=  (1 << AML_DATMOS_PARAMS_ID_POST_PROCESSING_ENABLE);
        adev->datmos_param.post = val;
        ALOGI("post set to %d\n", adev->datmos_param.post);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            /*
             *If postprocessing is enabled, DRC must be disabled.
             */
            if (adev->datmos_param.post) {
                delete_datmos_option(opts, "-nolm");
                add_datmos_option(opts, "-post", "");
                add_datmos_option(opts, "-drc", "drc_mode=disable");
            }
            else {
                delete_datmos_option(opts, "-post");
                if (adev->datmos_param.nolm) {
                    add_datmos_option(opts, "-nolm", "");
                    add_datmos_option(opts, "-drc", "drc_mode=disable");
                }
                else if (strstr(adev->datmos_param.drc_config, "enable") || \
                    strstr(adev->datmos_param.drc_config, "disable") || \
                    strstr(adev->datmos_param.drc_config, "auto"))
                    add_datmos_option(opts, "-drc", adev->datmos_param.drc_config);
            }
        }
        return 0;
    }

    /*dynamic param*/
    ret = str_parms_get_int(parms, "mode", &val);
    if (ret >= 0) {
        if (adev->datmos_param.mode != val)
            adev->dec_params_update_mask |=  (1 << AML_DATMOS_PARAMS_ID_AUDIO_PROCESSING_MODE);
        adev->datmos_param.mode = val;
        ALOGI("mode set to %d\n", adev->datmos_param.mode);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            const char *opt_key = "-mode";
            switch (adev->datmos_param.mode) {
                case DAP_Standard:
                    /*use the default value, "-mode movie"*/
                    delete_datmos_option(opts, opt_key);
                    break;
                case DAP_Movie:
                    add_datmos_option(opts, opt_key, "movie");
                    break;
                case DAP_Music:
                    add_datmos_option(opts, opt_key, "music");
                    break;
                case DAP_Night:
                    add_datmos_option(opts, opt_key, "night");
                    break;
                default:
                    ALOGI("wrong mode %d\n", adev->datmos_param.mode);
            }
        }
        return 0;
    }

    /*dynamic param*/
    ret = str_parms_get_int(parms, "vmcal", &val);
    if (ret >= 0) {
        if (adev->datmos_param.vmcal != val)
            adev->dec_params_update_mask |=  (1 << AML_DATMOS_PARAMS_ID_VOLUME_MODELER_CALIBRATION);
        adev->datmos_param.vmcal = val;
        ALOGI("vmcal set to %d\n", adev->datmos_param.vmcal);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            if (adev->datmos_param.vmcal) {
                char vmcal_config[12];
                sprintf(vmcal_config, "%d", adev->datmos_param.vmcal);
                add_datmos_option(opts, "-vmcal", vmcal_config);
            }
            else
                delete_datmos_option(opts, "-vmcal");
        }
        return 0;
    }

    /*dynamic param*/
    ret = str_parms_get_int(parms, "hfilt", &val);
    if (ret >= 0) {
        if (adev->datmos_param.hfilt != val)
            adev->dec_params_update_mask |=  (1 << AML_DATMOS_PARAMS_ID_HEIGHT_FILTER_ENABLE);
        adev->datmos_param.hfilt = val;
        ALOGI("hfilt set to %d\n", adev->datmos_param.hfilt);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            /*
             *The height cue filter can only be enabled for Dolby Atmos enabled speaker pairs.
                lre
                    Left/Right Dolby Atmos enabled speakers
                lrse
                    Left/Right surround Dolby Atmos enabled speakers
                lrrse
                    Left/Right rear surround Dolby Atmos enabled speakers
             */
            if ((adev->datmos_param.hfilt) && \
                (strstr(adev->datmos_param.speaker_config, "lre") || \
                 strstr(adev->datmos_param.speaker_config, "lrse") || \
                 strstr(adev->datmos_param.speaker_config, "lrrse")))
                add_datmos_option(opts, "-hfilt", "");
            else
                delete_datmos_option(opts, "-hfilt");
        }
        return 0;
    }

    /*dynamic param*/
    ret = str_parms_get_int(parms, "nolm", &val);
    if (ret >= 0) {
        if (adev->datmos_param.nolm != val)
            adev->dec_params_update_mask |=  (1 << AML_DATMOS_PARAMS_ID_LOUDNESS_MANAGEMENT_ENABLE);
        adev->datmos_param.nolm = val;
        ALOGI("nolm set to %d\n", adev->datmos_param.nolm);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            /*
             *If Loudness management is disabled,
             *then DRC and postprocessing must also be disabled.
             */
            if (adev->datmos_param.nolm) {
                add_datmos_option(opts, "-nolm", "");
                add_datmos_option(opts, "-drc", "drc_mode=disable");
                delete_datmos_option(opts, "-post");
            }
            else {
                delete_datmos_option(opts, "-nolm");
                /*
                 *If postprocessing is enabled, DRC must be disabled.
                 */
                if (adev->datmos_param.post) {
                    add_datmos_option(opts, "-post", "");
                    add_datmos_option(opts, "-drc", "drc_mode=disable");
                }
                else {
                    delete_datmos_option(opts, "-post");
                    if (strstr(adev->datmos_param.drc_config, "enable") || \
                        strstr(adev->datmos_param.drc_config, "disable") || \
                        strstr(adev->datmos_param.drc_config, "auto"))
                        add_datmos_option(opts, "-drc", adev->datmos_param.drc_config);
                }
            }
        }
        return 0;
    }

    /*dynamic param*/
    ret = str_parms_get_int(parms, "noupmix", &val);
    if (ret >= 0) {
        if (adev->datmos_param.noupmix != val)
            adev->dec_params_update_mask |=  (1 << AML_DATMOS_PARAMS_ID_UPMIX_ENABLE);
        adev->datmos_param.noupmix = val;
        ALOGI("noupmix set to %d\n", adev->datmos_param.noupmix);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            if (adev->datmos_param.noupmix)
                add_datmos_option(opts, "-noupmix", "");
            else
                delete_datmos_option(opts, "-noupmix");
        }
        return 0;
    }

    /*dynamic param*/
    ret = str_parms_get_int(parms, "novlamp", &val);
    if (ret >= 0) {
        if (adev->datmos_param.novlamp != val)
            adev->dec_params_update_mask |=  (1 << AML_DATMOS_PARAMS_ID_VLAMP);
        adev->datmos_param.novlamp = val;
        ALOGI("novlamp set to %d\n", adev->datmos_param.novlamp);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            if (adev->datmos_param.novlamp)
                add_datmos_option(opts, "-novlamp", "");
            else
                delete_datmos_option(opts, "-novlamp");
        }
        return 0;
    }

    /*static param*/
    ret = str_parms_get_int(parms, "noupresampler", &val);
    if (ret >= 0) {
        adev->datmos_param.noupresampler = val;
        ALOGI("novlamp set to %d\n", adev->datmos_param.noupresampler);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            if (adev->datmos_param.noupresampler)
                add_datmos_option(opts, "-noupresampler", "1");
            else
                add_datmos_option(opts, "-noupresampler", "0");
        }
        return 0;
    }

    /*static param*/
#if 0
    ret = str_parms_get_int(parms, "dec_joc", &val);
    if (ret >= 0) {
        adev->datmos_param.dec_joc = val;
        ALOGI("novlamp set to %d\n", adev->datmos_param.dec_joc);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            if (adev->datmos_param.dec_joc)
                add_datmos_option(opts, "-dec_joc", "1");
            else
                add_datmos_option(opts, "-dec_joc", "0");
        }
        return 0;
    }
#endif

    /*static param*/
    ret = str_parms_get_str(parms, "verbose", value, sizeof(value));
    if (ret >= 0) {
        ALOGI("get value %s\n", value);
        if (strncmp(value, "enable", 6) == 0 || strncmp(value, "1", 1) == 0) {
            adev->datmos_param.verbose = 1;
        } else if (strncmp(value, "disable", 7) == 0 || strncmp(value, "0", 1) == 0) {
            adev->datmos_param.verbose = 0;
        } else {
            ALOGE("%s() unsupport directdec value: %s",   __func__, value);
        }
        ALOGI("verbose set to %d\n", adev->datmos_param.verbose);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            if (adev->datmos_param.verbose){
                ALOGI("add_datmos_option -v\n");
                add_datmos_option(opts, "-v", "");
            }
            else
                delete_datmos_option(opts, "-v");
        }
        return 0;
    }

    /*dynamic param*/
    ret = str_parms_get_str(parms, "dapcustomize", value, sizeof(value));
    if (ret >= 0) {
        ALOGI("get value %s\n", value);
        if (strcmp(adev->datmos_param.dapcustomize, value)) {
            adev->dec_params_update_mask |=  (1 << AML_DATMOS_HT_PARAMS_ID_DAP_CUSTOMIZE);
            adev->dec_params_update_mask |=  (1 << AML_DATMOS_HT_PARAMS_ID_DAP_CUSTOMIZE_FACTOR);
        }
        memset(adev->datmos_param.dapcustomize, 0, sizeof(adev->datmos_param.dapcustomize));
        strncpy(adev->datmos_param.dapcustomize, value, strlen(value));
        ALOGI("dapcustomize set to %s\n", adev->datmos_param.dapcustomize);
        if (strstr(adev->datmos_param.dapcustomize, "dapcustomize_mode=enable"))
            adev->datmos_param.b_dap_customize = true;
        else if (strstr(adev->datmos_param.dapcustomize, "dapcustomize_mode=disable"))
            adev->datmos_param.b_dap_customize = false;

        if (adev->datmos_enable) {
            if (adev->datmos_param.b_dap_customize){
                ALOGI("add_datmos_option -v\n");
                add_datmos_option(opts, "-dapcustomize", adev->datmos_param.dapcustomize);
            }
            else
                delete_datmos_option(opts, "-dapcustomize");
        }


        return 0;
    }

    /*static param*/
    ret = str_parms_get_int(parms, "inputpcm", &val);
    if (ret >= 0) {
        memset(adev->datmos_param.inputpcm_config, 0, sizeof(adev->datmos_param.inputpcm_config));
        strncpy(adev->datmos_param.inputpcm_config, value, strlen(value));
        ALOGI("speaker_config set to %s\n", adev->datmos_param.inputpcm_config);
        /*datmos parameter*/
        if (adev->datmos_enable)
            add_datmos_option(opts, "-inputpcm", adev->datmos_param.inputpcm_config);
        return 0;
    }


    return -1;

error_exit:
    ret = -1;
    ALOGE("Error exit!");
    return ret;
}

int datmos_get_parameters(struct audio_hw_device *dev, const char *keys, char *temp_buf, size_t temp_buf_size)
{
    return 0;
}


int get_datmos_func(struct aml_datmos_param *datmos_handle)
{
    ALOGI("");

    if (!datmos_handle) {
        ALOGE("datmos_handle is NULL");
        return 1;
    }

    if (!datmos_handle->fd) {
        datmos_handle->fd = dlopen("/tmp/ds/0xc7_0xfba4_0x5e.so", RTLD_LAZY);
        if (datmos_handle->fd == NULL) {
            ALOGI("error dlopen on default place, try system lib place");
            datmos_handle->fd = dlopen("0xc7_0xfba4_0x5e.so", RTLD_LAZY);
            if (datmos_handle->fd == NULL) {
                ALOGI("error dlopen %s", dlerror());
                return 1;
            }
        }

        datmos_handle->get_audio_info = (int (*)(void *, int *, int *, int *))dlsym(datmos_handle->fd, "get_audio_info");
        if (datmos_handle->get_audio_info == NULL) {
            ALOGI("cant find lib interface %s", dlerror());
            goto error;
        }
        datmos_handle->aml_atmos_init = (int (*)(unsigned int, void **, const int, const char **))dlsym(datmos_handle->fd, "aml_atmos_init");
        if (datmos_handle->aml_atmos_init == NULL) {
            ALOGI("cant find lib interface %s", dlerror());
            goto error;
        }
        datmos_handle->aml_atmos_process = (int (*)(void *, unsigned int, void *, size_t *, char *, unsigned int , size_t *))dlsym(datmos_handle->fd, "aml_atmos_process");
        if (datmos_handle->aml_atmos_process == NULL) {
            ALOGI("cant find lib interface %s", dlerror());
            goto error;
        }
        datmos_handle->aml_atmos_cleanup = (void (*)(void *))dlsym(datmos_handle->fd, "aml_atmos_cleanup");
        if (datmos_handle->aml_atmos_cleanup == NULL) {
            ALOGI("cant find lib interface %s", dlerror());
            goto error;
        }
        datmos_handle->aml_get_output_info = (int (*)(void *, int *, int *, int *))dlsym(datmos_handle->fd, "aml_get_output_info");
        if (datmos_handle->aml_get_output_info == NULL) {
            ALOGI("cant find lib interface %s", dlerror());
            goto error;
        }
        datmos_handle->aml_datmos_dynamic_parameter_set = (int (*)(void *, const int, const char **))dlsym(datmos_handle->fd, "aml_datmos_dynamic_parameter_set");
        if (datmos_handle->aml_datmos_dynamic_parameter_set == NULL) {
            ALOGI("cant find lib interface %s", dlerror());
            goto error;
        }
        ALOGI("fd=%p", datmos_handle->fd);
        ALOGI("get_audio_info=%p", datmos_handle->get_audio_info);
        ALOGI("aml_atmos_init=%p", datmos_handle->aml_atmos_init);
        ALOGI("aml_atmos_process=%p", datmos_handle->aml_atmos_process);
        ALOGI("aml_atmos_cleanup=%p", datmos_handle->aml_atmos_cleanup);
        ALOGI("aml_get_output_info=%p", datmos_handle->aml_get_output_info);
        ALOGI("aml_datmos_dynamic_parameter_set=%p", datmos_handle->aml_datmos_dynamic_parameter_set);
        return 0;
    } else {
        ALOGI("");
        return 0;
    }

error:
    ALOGE("dlopen failed!");
    dlclose(datmos_handle->fd);
    datmos_handle->fd = NULL;
    return 1;
}

int cleanup_atmos_func(struct aml_datmos_param *datmos_handle)
{
    ALOGI("");
    if (!datmos_handle) {
        ALOGE("datmos_handle is NULL");
        return 1;
    }
    datmos_handle->get_audio_info = NULL;
    datmos_handle->aml_atmos_init = NULL;
    datmos_handle->aml_atmos_process = NULL;
    datmos_handle->aml_atmos_cleanup = NULL;
    datmos_handle->aml_get_output_info = NULL;
    datmos_handle->aml_datmos_dynamic_parameter_set = NULL;

    /* close the shared object */
    if (datmos_handle->fd) {
        dlclose(datmos_handle->fd);
        datmos_handle->fd = NULL;
        ALOGE("fd is dlclosed!");
    }
    return 0;
}


static void mat_frame_debug(const char *mat_frame, int raw_size)
{
    int i = 0;
    if (mat_frame) {
        for (i = 0; i < 16; i++) {
            printf("%2x ", mat_frame[i]);
        }
        printf("\n");
        for (i = 0; i < 16; i++) {
            printf("%2x ", mat_frame[raw_size - 16 + i]);
        }
        printf("\n");
    }
}

static int datmos_get_output_info(aml_dec_t *aml_dec, aml_dec_info_t *dec_info)
{
    struct dolby_atmos_dec *datmos_dec = (struct dolby_atmos_dec *)aml_dec;
    int ret = 0;
    ALOGV("<<IN>>");
    if (!datmos_dec || !dec_info) {
        ALOGE("%s dec_info is NULL\n", __func__);
        return -1;
    }
    else {
        if (datmos_dec->aml_get_output_info) {
            ret = datmos_dec->aml_get_output_info(aml_dec->dec_ptr
                , &dec_info->output_sr
                , &dec_info->output_ch
                , &dec_info->output_bitwidth);
            ALOGV("output_sr %d output_ch %d output_bitwidth %d",
                dec_info->output_sr, dec_info->output_ch, 
                dec_info->output_bitwidth);
        }
        return ret;
    }

}

static int datmos_get_audio_info(aml_dec_t *aml_dec)
{
    struct dolby_atmos_dec *datmos_dec = (struct dolby_atmos_dec *)aml_dec;
    int ret = 0;

    if (!datmos_dec) {
        ALOGE("%s datmos_dec is NULL\n", __func__);
        return -1;
    }
    else {
        if (datmos_dec->get_audio_info && aml_dec->dec_ptr) {
            ret = datmos_dec->get_audio_info(aml_dec->dec_ptr
                    , &datmos_dec->is_dolby_atmos
                    , &datmos_dec->audio_samplerate
                    , &datmos_dec->system_latency);
            ALOGV("is_dolby_atmos %d Audio sr %d latency %d",
                datmos_dec->is_dolby_atmos, datmos_dec->audio_samplerate, datmos_dec->system_latency);
            aml_dec->dec_info.stream_sr = datmos_dec->audio_samplerate;
            ret = 0;
        }
        else
            ret = -1;
        return ret;
    }

}

int datmos_decoder_init_patch(aml_dec_t ** ppdatmos_dec, audio_format_t format, aml_dec_config_t * dec_config)
{
    struct dolby_atmos_dec *datmos_dec;
    aml_dec_t  *aml_dec = NULL;
    struct aml_datmos_param *datmos_handle = NULL;
    void *opts = NULL;

    ALOGV("<<IN>>");
    if (!dec_config) {
        ALOGE("dec_config is error\n");
        return -1;
    }
    aml_datmos_config_t *datmos_config = (aml_datmos_config_t *)dec_config;
    datmos_dec = calloc(1, sizeof(struct dolby_atmos_dec));
    if (datmos_dec == NULL) {
        ALOGE("malloc datmos_dec failed\n");
        return -1;
    }

    aml_dec = &datmos_dec->aml_dec;

    if (aml_dec && aml_dec->init_argv == NULL) {
        aml_dec->init_argv = (char **)malloc(MAX_PARAM_COUNT * VALUE_BUF_SIZE);
        if (aml_dec->init_argv) {
            memset(aml_dec->init_argv, 0, sizeof(MAX_PARAM_COUNT * VALUE_BUF_SIZE));
            for (int i = 0; i < MAX_PARAM_COUNT; i++) {
                aml_dec->init_argv[i] = (char *)malloc(VALUE_BUF_SIZE);
                memset(aml_dec->init_argv[i], 0, sizeof(VALUE_BUF_SIZE));
            }
        }
    }

    datmos_config->audio_type = android_audio_format_t_convert_to_andio_type(format);
    /*Fixme: how to get the eb3 extension?*/
    datmos_config->is_eb3_extension = 0;

    datmos_handle = (struct aml_datmos_param *)datmos_config->reserved;
    ALOGD("%s line %d datmos_handle %p\n", __func__, __LINE__, datmos_handle);

    if (!datmos_handle) {
        ALOGE("datmos_handle is NULL\n");
        goto exit;
        }

    if (datmos_handle->aml_atmos_init == NULL) {
        goto exit;
    }

    opts = get_datmos_current_options();

    switch (datmos_config->audio_type) {
        case TRUEHD: {
            add_datmos_option(opts, "-i", "/media/test.mat");
        }
        break;
        case EAC3: {
            if (datmos_config->is_eb3_extension) {
                add_datmos_option(opts, "-i", "/media/test.eb3");
            }
            else
                add_datmos_option(opts, "-i", "/media/test.ec3");
        }
        break;
        case AC3: {
            add_datmos_option(opts, "-i", "/media/test.ac3");
        }
        break;
        case LPCM: {
            add_datmos_option(opts, "-i", "/media/test.wav");
            char param[256] = {0};
            snprintf(param, sizeof(param), "inputpcm_mode=enable:samplerate=%d:channels=%d:bitdepth=%d",
                datmos_config->samplerate,datmos_config->channel,datmos_config->bitwidth);
            ALOGE("param %s\n", param);
            aml_dec->dec_info.stream_sr = datmos_config->samplerate;
            aml_dec->dec_info.stream_ch = datmos_config->channel;
            aml_dec->dec_info.stream_bitwidth = datmos_config->bitwidth;
            add_datmos_option(opts, "-inputpcm", param);
        }
        break;
        default: {
            ALOGE("unsuitable audio format %d!\n", datmos_config->audio_type);
            return -1;
        }
    }
    ALOGD("%s line %d\n", __func__, __LINE__);

    if (get_datmos_config(opts, aml_dec->init_argv, &aml_dec->init_argc) != 0) {
        ALOGE("get datmos config fail\n");
        return -1;
    }

    ALOGI("audio_type %s is_eb3_extension %d\n", AUDIO_FORMAT_STRING(datmos_config->audio_type), datmos_config->is_eb3_extension);

    if (datmos_handle->aml_atmos_init)
        aml_dec->status =
            datmos_handle->aml_atmos_init(
                    datmos_config->audio_type
                    , &(aml_dec->dec_ptr)
                    , aml_dec->init_argc
                    , aml_dec->init_argv);

    ALOGI("aml_dec %p status %d format %#x\n", aml_dec, aml_dec->status, aml_dec->format);

    ALOGI("aml_atmos_init return %s\n", ((aml_dec->status == 0) ? "success" : "fail"));

    if (aml_dec->status < 0) {
        goto exit;
    }

    aml_dec->remain_size = 0;
    aml_dec->outlen_pcm = 0;
    aml_dec->outlen_raw = 0;
    aml_dec->inbuf = NULL;
    aml_dec->outbuf = NULL;
    aml_dec->outbuf_raw = NULL;
    if ((datmos_config->audio_type == AC3) || (datmos_config->audio_type == EAC3))
        aml_dec->inbuf_max_len = MAX_DECODER_DDP_FRAME_LENGTH*8;
    else if (datmos_config->audio_type == TRUEHD)
        aml_dec->inbuf_max_len = MAX_DECODER_MAT_FRAME_LENGTH*4;
    else if (datmos_config->audio_type == LPCM)
        aml_dec->inbuf_max_len = MAX_DECODER_DDP_FRAME_LENGTH*8;//Be careful about this Length!!!
    ALOGV("aml_dec inbuf_max_len %x\n", aml_dec->inbuf_max_len);
    aml_dec->inbuf = (unsigned char*) malloc(aml_dec->inbuf_max_len);
    /*
     *FIXME:local playback of truehd case is TODO.
    else if (datmos_config->audio_type == TRUEHD)
        aml_dec->inbuf = (unsigned char*) malloc(MAX_DECODER_THD_FRAME_LENGTH);
    */
    if (!aml_dec->inbuf) {
        ALOGE("malloc buffer failed\n");
        goto exit;
    }
    aml_dec->outbuf_max_len = MAX_BLOCK_NUM*ONE_BLOCK_FRAME_SIZE*MAX_OUTPUT_CH*BYTES_PER_SAMPLE;
    ALOGV("aml_dec outbuf_max_len %x\n", aml_dec->outbuf_max_len);
    aml_dec->outbuf = (unsigned char*) malloc(aml_dec->outbuf_max_len);
    if (!aml_dec->outbuf) {
        ALOGE("malloc buffer failed\n");
        goto exit;
    }

    datmos_dec->get_audio_info = datmos_handle->get_audio_info;
    datmos_dec->aml_atmos_init = datmos_handle->aml_atmos_init;
    datmos_dec->aml_atmos_process = datmos_handle->aml_atmos_process;
    datmos_dec->aml_atmos_cleanup = datmos_handle->aml_atmos_cleanup;
    datmos_dec->aml_get_output_info = datmos_handle->aml_get_output_info;
    datmos_dec->aml_datmos_dynamic_parameter_set = datmos_handle->aml_datmos_dynamic_parameter_set;
    aml_dec->status = 1;

    *ppdatmos_dec = (aml_dec_t *)datmos_dec;

    ALOGV("<<OUT>>");
    return 1;

exit:
    if (aml_dec->init_argv) {
        for (int i = 0; i < MAX_PARAM_COUNT; i++) {
            if (aml_dec->init_argv[i])
                free(aml_dec->init_argv[i]);
        }
        free(aml_dec->init_argv);
        aml_dec->init_argv = NULL;
    }

    if (aml_dec->inbuf) {
        free(aml_dec->inbuf);
        aml_dec->inbuf = NULL;
    }
    if (aml_dec->outbuf) {
        free(aml_dec->outbuf);
        aml_dec->outbuf = NULL;
    }
    if (datmos_dec) {
        free(datmos_dec);
        datmos_dec = NULL;
    }

    *ppdatmos_dec = NULL;
    ALOGV("<<OUT>>");
    return -1;

}

int datmos_decoder_release_patch(aml_dec_t *aml_dec)
{
    struct dolby_atmos_dec *datmos_dec = (struct dolby_atmos_dec *)aml_dec;
    ALOGV("<<IN>>");
    if (datmos_dec && datmos_dec->aml_atmos_cleanup) {
        datmos_dec->aml_atmos_cleanup(aml_dec->dec_ptr);
        aml_dec->dec_ptr = NULL;
    }

    if (aml_dec->status == 1) {
        aml_dec->status = 0;
        aml_dec->remain_size = 0;
        aml_dec->outlen_pcm = 0;
        aml_dec->outlen_raw = 0;

        if (aml_dec->init_argv) {
            for (int i = 0; i < MAX_PARAM_COUNT; i++) {
                if (aml_dec->init_argv[i])
                    free(aml_dec->init_argv[i]);
            }
            free(aml_dec->init_argv);
            aml_dec->init_argv = NULL;
        }

        if (aml_dec->inbuf) {
            free(aml_dec->inbuf);
            aml_dec->inbuf = NULL;
        }

        if (aml_dec->outbuf) {
            free(aml_dec->outbuf);
            aml_dec->outbuf = NULL;
        }

        if (aml_dec->outbuf_raw) {
            free(aml_dec->outbuf_raw);
            aml_dec->outbuf_raw = NULL;
        }

        datmos_dec->get_audio_info = NULL;
        datmos_dec->aml_atmos_init = NULL;
        datmos_dec->aml_atmos_process = NULL;
        datmos_dec->aml_atmos_cleanup = NULL;
        datmos_dec->aml_get_output_info = NULL;
        datmos_dec->aml_datmos_dynamic_parameter_set = NULL;
        free(aml_dec);
        aml_dec = NULL;
    }
    ALOGV("<<OUT>>");
    return 1;
}

static int swap_center_lfe_for_8ch_pcm(unsigned char*in_buffer, int in_bytes, int bytes_per_sample)
{
    int nframe = 0;
    int nch = 0;
    int i = 0;
    bool b_aligned = (0 == in_bytes % (8*bytes_per_sample));

    if (!in_buffer || (in_bytes <= 0) || (bytes_per_sample == 0) || !b_aligned) {
        ALOGE("in_buffer %p in_bytes %#x bytes_per_sample %p b_aligned %d\n",
            in_buffer, in_bytes, bytes_per_sample, b_aligned);
        return -1;
    }

    nframe = in_bytes/(8*bytes_per_sample);

    if (bytes_per_sample == sizeof(int16_t)) {
        int16_t *in_16 = (int16_t *)in_buffer;
        int16_t temp_16 = 0;
        for (i = 0; i < nframe; i++) {
            //temp = lfe; lfe = c; c = temp;
            temp_16 = in_16[8*i + 2];
            in_16[8*i + 2] = in_16[8*i + 3];
            in_16[8*i + 3] = temp_16;
        }
    }
    else if (bytes_per_sample == sizeof(int32_t)) {
        int32_t *in_32 = (int32_t *)in_buffer;
        int32_t temp_32 = 0;
        for (i = 0; i < nframe; i++) {
            //temp = lfe; lfe = c; c = temp;
            temp_32 = in_32[8*i + 2];
            in_32[8*i + 2] = in_32[8*i + 3];
            in_32[8*i + 3] = temp_32;
        }
    }
    else
        return -1;

    return 0;
}

int datmos_decoder_process_patch(aml_dec_t *aml_dec, unsigned char*in_buffer, int in_bytes)
{
    struct dolby_atmos_dec *datmos_dec = (struct dolby_atmos_dec *)aml_dec;
    size_t bytes_consumed = 0;
    size_t pcm_produced_bytes = 0;
    int ret = 0;
    size_t input_threshold = 0;
    size_t dump_input = 0;
    size_t check_data = 0;
    int bps = 0;

    ALOGV("<<IN>>");
    if (!aml_dec || !in_buffer || (in_bytes <= 0) || !aml_dec->inbuf) {
        ALOGE("aml_dec %p in_buffer %p in_bytes %#x inbuf %p inbuf_wt %#x\n",
            aml_dec, in_buffer, in_bytes, aml_dec->inbuf, aml_dec->inbuf_wt);
        goto EXIT;
    }

    /*FIXME, sometimes the data is not right, decoder could not preroll the datmos at all*/
    if (audio_is_linear_pcm(aml_dec->format)) {
        input_threshold = 1536*aml_dec->dec_info.stream_ch* aml_dec->dec_info.stream_bitwidth/8;
    }
    else
        input_threshold = aml_dec->burst_payload_size;

    aml_dec->outlen_pcm = 0;
    bps = aml_dec->dec_info.stream_bitwidth/8;

    ALOGV("valid bytes %#x input_threshold %#x\n", aml_dec->inbuf_wt, input_threshold);

    while((aml_dec->inbuf_wt >= input_threshold) && (input_threshold > 0)){
    if ((aml_dec->inbuf_wt >= input_threshold) && (input_threshold > 0)) {
        //ALOGE("inbuf_wt %#x burst_payload_size %#x, input_threshold %#x 0x%x 0x%x\n",
        //  aml_dec->inbuf_wt, aml_dec->burst_payload_size, input_threshold,aml_dec->inbuf[0],aml_dec->inbuf[1]);
        datmos_dec->is_truehd_within_mat = is_truehd_within_mat(aml_dec->inbuf, input_threshold);

        /*for the 8ch pcm, the lfe and center channel are reversed*/
        if (audio_is_linear_pcm(aml_dec->format) && (aml_dec->dec_info.stream_ch == 8))
            if (swap_center_lfe_for_8ch_pcm(aml_dec->inbuf, input_threshold, bps)) {
                ALOGE("in_bytes %#x inbuf %p bps %#x\n", input_threshold, aml_dec->inbuf, bps);
                goto EXIT;
            }

        /*begin to decode the data*/
        ret = datmos_dec->aml_atmos_process
                        (aml_dec->inbuf
                        , input_threshold
                        , aml_dec->dec_ptr
                        , &bytes_consumed
                        , aml_dec->outbuf + aml_dec->outlen_pcm
                        , aml_dec->outbuf_max_len
                        , &pcm_produced_bytes);

        if (check_data) {
            mat_frame_debug(aml_dec->inbuf, input_threshold);
            char *datmos_data = aml_dec->inbuf;
            ALOGE("header %2x %2x", datmos_data[0], datmos_data[1]);
        }

        if (aml_dec->format == AUDIO_FORMAT_E_AC3 || aml_dec->format == AUDIO_FORMAT_AC3) {
              /*
              *in datmos decoder, the max size in one circle determinated by
              *datmos_ptr->procblocksize = DATMOS_HT_DDP_PROC_BLOCK_SIZE;//0x1800
              */
            if (ret != 0 && bytes_consumed == 0) {
                bytes_consumed = input_threshold > 0x1800? 0x1800:input_threshold;
            }
        } else {
            bytes_consumed = input_threshold;
        }


        /*end of the decode*/
        {
            //dump the input data, as the data should convert to LSB
            if (dump_input) {
                FILE *fpa=fopen(DATMOS_RAW_IN_FILE,"a+");
                // convert_format(aml_dec->inbuf, aml_dec->burst_payload_size);
                fwrite((char *)aml_dec->inbuf, 1, bytes_consumed,fpa);
                fclose(fpa);
            }
            //update the inbuf_wt and flush the consumed data in the inbuf
            {
                if (aml_dec->inbuf_wt - bytes_consumed > 0) {
                    memmove(aml_dec->inbuf, aml_dec->inbuf + bytes_consumed, aml_dec->inbuf_wt - bytes_consumed);
                    aml_dec->inbuf_wt -= bytes_consumed;
                }
                else {
                    memset(aml_dec->inbuf, 0, aml_dec->inbuf_wt);
                    aml_dec->inbuf_wt = 0;
                }
            }
            /*
             *for this ret means that datmos would block this status
             *we should return -1, the decoder should be reinit again!
             */
            if ((ret == 4) && (aml_dec->format == AUDIO_FORMAT_DOLBY_TRUEHD))  {
                /*pay more attention here, if flush the inbuf, should reset the raw_deficiency&inbuf_wt to zero*/
                // memset(aml_dec->inbuf, 0, aml_dec->inbuf_max_len);
                // aml_dec->inbuf_wt = 0;
                // aml_dec->raw_deficiency =0;
                aml_dec->outlen_pcm = 0;
                return -1;
            }
        }

        aml_dec->outlen_pcm += pcm_produced_bytes;
        if(aml_dec->outlen_pcm > aml_dec->outbuf_max_len) {
            ALOGF("output too much data\n");
        }
        datmos_get_output_info(aml_dec, &(aml_dec->dec_info));
        datmos_get_audio_info(aml_dec);

        aml_dec->is_truehd_within_mat = datmos_dec->is_truehd_within_mat;
        aml_dec->is_dolby_atmos = datmos_dec->is_dolby_atmos;

        if (aml_dec->dec_info.output_bitwidth == SAMPLE_24BITS)
            aml_dec->dec_info.output_bitwidth = SAMPLE_32BITS;

        if (aml_dec->format == AUDIO_FORMAT_E_AC3 || aml_dec->format == AUDIO_FORMAT_AC3) {
            if (aml_dec->outlen_pcm >= AC3_FRAMELENGTH * (aml_dec->dec_info.output_bitwidth >>3) * (aml_dec->dec_info.output_ch)) {
                break;
            }

        }
        /*information about Datmos decoder*/
        ALOGV("ret=%d valid bytes %#x outlen_pcm %#x consume=0x%x pcm_produced_bytes %#x",
            ret, aml_dec->inbuf_wt, aml_dec->outlen_pcm,bytes_consumed, pcm_produced_bytes);
    }
    else {
        /*
         *do nothing, just store the data to aml_dec->inbuf
         *clear the output pcm length as zero.
         */
        //aml_dec->outlen_pcm = 0;
        break;
    }
    }
    if (aml_dec->outlen_pcm !=0 ) {
        if (aml_log_get_dumpfile_enable("dump_decoder")) {
            //dump the decoded pcm data
            FILE *fp1=fopen(DATMOS_PCM_OUT_FILE,"a+");
            fwrite((char *)aml_dec->outbuf, 1, aml_dec->outlen_pcm ,fp1);
            fclose(fp1);
        }
    }

    ALOGV("<<ret %d OUT>>", ret);
    return 0;
EXIT:
    return -1;
}

int datmos_decoder_dynamic_param_set_patch(aml_dec_t *aml_dec)
{
    int ret = 0;
    void *opts = NULL;
    struct dolby_atmos_dec *datmos_dec = (struct dolby_atmos_dec *)aml_dec;

    if (!aml_dec) {
        ALOGE("aml_dec %p\n", aml_dec);
        goto EXIT;
    }

    opts = get_datmos_current_options();

    if (get_datmos_config(opts, aml_dec->init_argv, &aml_dec->init_argc) != 0) {
        ALOGE("get datmos config fail\n");
        return -1;
    }
    ret = datmos_dec->aml_datmos_dynamic_parameter_set
                    (aml_dec->dec_ptr
                    , (const int) aml_dec->init_argc
                    , (const char **)aml_dec->init_argv);

    return ret;
EXIT:
    return -1;
}

aml_dec_func_t aml_datmos_func = {
    .f_init                 = datmos_decoder_init_patch,
    .f_release              = datmos_decoder_release_patch,
    .f_process              = datmos_decoder_process_patch,
    .f_dynamic_param_set    = datmos_decoder_dynamic_param_set_patch,
};
#endif

