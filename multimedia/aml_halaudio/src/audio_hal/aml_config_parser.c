/*
 * Copyright (C) 2019 Amlogic Corporation.
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
#define LOG_TAG "config_parse"


#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include "aml_config_parser.h"
#include "log.h"

#define AML_CONFIG_FILE_PATH "/etc/aml_audio_config.json"

void aml_printf_cJSON(const char *json_name, cJSON *pcj)
{
    char *temp;
    temp = cJSON_Print(pcj);
    printf("%s %s\n", json_name, temp);
    free(temp);
}


static cJSON *aml_createJsonRoot(const char *filename)
{
    FILE *fp;
    int len, ret;
    char *input = NULL;
    cJSON *temp;
    if (NULL == filename) {
        return NULL;
    }
    fp = fopen(filename, "r+");
    if (fp == NULL) {
        ALOGD("cannot open the default json file %s\n", filename);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    len = (int)ftell(fp);
    ALOGD(" length = %d\n", len);

    fseek(fp, 0, SEEK_SET);

    input = (char *)malloc(len + 10);
    if (input == NULL) {
        ALOGD("Cannot malloc the address size = %d\n", len);
        fclose(fp);
        return NULL;
    }
    ret = fread(input, 1, len, fp);
    // allocate the
    temp = cJSON_Parse(input);

    fclose(fp);
    free(input);

    return temp;
}


cJSON * aml_config_parser(char * config_files)
{
    cJSON *config_root = NULL;
    cJSON * temp;
    int i = 0;
    int item_cnt = 0;

    config_root = aml_createJsonRoot(config_files);
    if (config_root) {
        printf("use json file name=%s\n", config_files);
    } else {
        config_root = aml_createJsonRoot(AML_CONFIG_FILE_PATH);
        printf("use default json file=%s\n", AML_CONFIG_FILE_PATH);
    }
    //printf_cJSON("aml audio config:",config_root);

    return config_root;
}

