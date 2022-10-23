/*
 * Copyright (C) 2020 Amlogic Corporation.
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
#define LOG_TAG "aml_external_process"


#include <dlfcn.h>
#include "log.h"
#include "aml_audio_log.h"
#include "aml_audio_effect.h"
#include "aml_external_process.h"

#define EXTERNAL_POST_PROCESS "external_post_process"
#define LIB_NAME              "lib_name"

#define EXTERNAL_POST_PROCESS_CONFIG "external_pp_config"
#define EXTERNAL_POST_PROCESS_INFO   "external_pp_info"

#define DEFAULT_EXTERNAL_POST_PROCESS_LIB  "/usr/lib/libpostexample.so"

typedef struct aml_external_pp_s {

    void *lib_handle;
    aml_audio_effect_library_t *desc;
    aml_effect_handle_t pEffectHandle;
} aml_external_pp_t;


static char * get_post_process_lib(cJSON * config)
{
    cJSON *temp;
    temp = cJSON_GetObjectItem((cJSON *)config, EXTERNAL_POST_PROCESS);
    if (temp) {
        temp = cJSON_GetObjectItem((cJSON *)temp, LIB_NAME);
        if (temp) {
            return temp->valuestring;
        }
    }

    return DEFAULT_EXTERNAL_POST_PROCESS_LIB;
}

static int load_lib(aml_external_pp_t *phandle, char *lib_name)
{
    void *hdl = NULL;
    aml_audio_effect_library_t *desc;

    hdl = dlopen(lib_name, RTLD_NOW);
    if (hdl == NULL) {
        return -1;
    }

    desc = (aml_audio_effect_library_t *)dlsym(hdl, AML_AUDIO_EFFECT_LIBRARY_INFO_SYM_AS_STR);
    if (desc == NULL) {
        ALOGE("loadLibrary() could not find symbol %s", AML_AUDIO_EFFECT_LIBRARY_INFO_SYM_AS_STR);
        goto error;
    }

    if (AML_AUDIO_EFFECT_LIBRARY_TAG != desc->tag) {
        ALOGE("getLibrary() bad tag %08x in lib info struct", desc->tag);
        goto error;
    }
    phandle->lib_handle = hdl;
    phandle->desc       = desc;
    ALOGI("lib =%s name =%s version=%s", lib_name, desc->name, desc->version);
    return 0;

error:
    if (hdl != NULL) {
        dlclose(hdl);
    }
    phandle->lib_handle = NULL;
    phandle->desc       = NULL;

    return -1;
}

static int unload_lib(aml_external_pp_t *phandle)
{
    void *hdl = NULL;

    hdl = phandle->lib_handle;
    if (hdl != NULL) {
        dlclose(hdl);
    }
    return 0;
}


static int create_effetct(aml_external_pp_t *phandle)
{

    return phandle->desc->create_effect(&phandle->pEffectHandle);
}

static int release_effetct(aml_external_pp_t *phandle)
{

    if (phandle->pEffectHandle) {
        phandle->desc->release_effect(phandle->pEffectHandle);
        phandle->pEffectHandle = NULL;
    }

    return 0;
}


int aml_external_pp_init(struct audio_hw_device *dev, cJSON * config)
{
    int ret = 0;
    char * lib_name = NULL;
    aml_external_pp_t *phandle = NULL;
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    if (config == NULL) {
        ret = -1;
        goto error;
    }

    phandle =  calloc(1, sizeof(aml_external_pp_t));
    if (phandle == NULL) {
        ret = -1;
        goto error;
    }
    /*get post process lib*/
    lib_name = get_post_process_lib(config);
    ALOGI("lib name=%s", lib_name);
    if (lib_name == NULL) {
        ret = -1;
        ALOGE("can't find external post process lib name");
        goto error;
    }
    /*load lib*/
    ret = load_lib(phandle, lib_name);
    if (ret != 0) {
        ALOGE("load lib %s error", lib_name);
        goto error;
    }

    /*create lib effect*/
    ret = create_effetct(phandle);
    if (ret != 0) {
        ALOGE("crate effect error");
        goto error;
    }

    adev->post_handle = phandle;

    return 0;

error:

    if (phandle) {
        unload_lib(phandle);
        free(phandle);
    }
    adev->post_handle = NULL;
    return ret;
}

int aml_external_pp_start(struct audio_hw_device *dev)
{
    int ret = 0;
    char value[256];
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    aml_external_pp_t *phandle = NULL;

    if (!adev) {
        ALOGE("%s Fatal Error adev %p", __FUNCTION__, adev);
        return -1;
    }
    phandle = (aml_external_pp_t *)adev->post_handle;
    if (phandle == NULL) {
        return -1;
    }

    ALOGI("%s %s %p", __FUNCTION__, value, (*phandle->pEffectHandle)->start);
    if ((*phandle->pEffectHandle)->start != NULL) {
        ret = (*phandle->pEffectHandle)->start(phandle->pEffectHandle);
    }

    return ret;
}

int aml_external_pp_stop(struct audio_hw_device *dev)
{
    int ret = 0;
    char value[256];
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    aml_external_pp_t *phandle = NULL;

    if (!adev) {
        ALOGE("%s Fatal Error adev %p", __FUNCTION__, adev);
        return -1;
    }
    phandle = (aml_external_pp_t *)adev->post_handle;
    if (phandle == NULL) {
        return -1;
    }

    ALOGI("%s %s %p", __FUNCTION__, value, (*phandle->pEffectHandle)->stop);
    if ((*phandle->pEffectHandle)->stop != NULL) {
        ret = (*phandle->pEffectHandle)->stop(phandle->pEffectHandle);
    }

    return 0;
}



int aml_external_pp_config(struct audio_hw_device *dev, struct str_parms *parms)
{
    int ret = 0;
    char value[256];
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    aml_external_pp_t *phandle = NULL;

    if (!adev || !parms) {
        ALOGE("%s Fatal Error adev %p parms %p", __FUNCTION__, adev, parms);
        return -1;
    }
    phandle = (aml_external_pp_t *)adev->post_handle;
    if (phandle == NULL) {
        return -1;
    }
    ret = str_parms_get_str(parms, EXTERNAL_POST_PROCESS_CONFIG, value, sizeof(value));
    if (ret >= 0) {
        ALOGI("%s %s %p", __FUNCTION__, value, (*phandle->pEffectHandle)->config);
        if ((*phandle->pEffectHandle)->config != NULL) {
            ret = (*phandle->pEffectHandle)->config(phandle->pEffectHandle, value);
        }
        return 0;
    }

    return -1;
}


int aml_external_pp_getinfo(struct audio_hw_device *dev, const char *keys, char *temp_buf, size_t temp_buf_size)
{
    int ret = 0;
    int i = 0;
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    aml_external_pp_t *phandle = NULL;

    if (!adev || !keys) {
        ALOGE("Fatal Error adev %p parms %p", adev, keys);
        return -1;
    }

    phandle = (aml_external_pp_t *)adev->post_handle;
    if (phandle == NULL) {
        return -1;
    }

    if (strstr(keys, EXTERNAL_POST_PROCESS_INFO)) {
        char * info = NULL;
        if ((*phandle->pEffectHandle)->getinfo != NULL) {
            info = (*phandle->pEffectHandle)->getinfo(phandle->pEffectHandle, keys);
        }
        if (info) {
            snprintf(temp_buf, temp_buf_size, "%s", info);
            ALOGI("%s key: %s value: %s", __FUNCTION__, keys, info);
            free(info);
            return 0;
        }
    }
    return -1;
}


int aml_external_pp_process(struct audio_hw_device *dev, void * in_data, size_t size, aml_data_format_t *format)
{
    int ret = 0;
    int output_size = 0;
    int used_size = 0;
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    aml_external_pp_t *phandle = NULL;

    phandle = (aml_external_pp_t *)adev->post_handle;
    if (phandle == NULL) {
        return -1;
    }
    data_format_t info = { 0 };
    info.ch           = format->ch;
    info.bitwidth     = format->bitwidth;
    info.endian       = format->endian;
    info.sr           = format->sr;

    if ((*phandle->pEffectHandle)->process != NULL) {
        output_size = size;
        ret = (*phandle->pEffectHandle)->process(phandle->pEffectHandle, in_data, size, &used_size, in_data, &output_size, &info);
        if (ret != 0) {
            ALOGE("%s error=%d", __FUNCTION__, ret);
        }
        if (size != used_size) {
            ALOGE("partial consumed data is not support in =%d used =%d", size, used_size);
        }
    }

    return ret;
}

int aml_external_pp_deinit(struct audio_hw_device *dev)
{
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    aml_external_pp_t *phandle = NULL;

    phandle = (aml_external_pp_t *)adev->post_handle;
    if (phandle == NULL) {
        return -1;
    }

    release_effetct(phandle);
    unload_lib(phandle);
    free(phandle);
    adev->post_handle = NULL;

    return 0;
}
