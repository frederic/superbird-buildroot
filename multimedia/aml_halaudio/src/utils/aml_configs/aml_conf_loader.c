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

/**
 ** loader.c
 **
 ** This program is designed for load the config of product in HAL.
 ** author: shen pengru
 **
 */
#define LOG_TAG "aml_parser"

#define LOG_NDEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "log.h"
#include <aml_conf_loader.h>
#include <aml_conf_parser.h>

/*************************************************
* TODO: this lib will be used in tvserver or
* mediaserver so how to protect the *.conf
* file here?
*************************************************/


/*************************************************
Function: config_load()
Description: load the config file of product
Input:
    file_name: the path of config file
Output:
Return: parser
*************************************************/
struct parser *aml_config_load(const char *file_name)
{
    struct parser *pParser = NULL;
    int ret = 0;

    pParser = malloc(sizeof(struct parser));
    ret = parser_init(pParser);
    if (ret) {
       ALOGD("%s: parser_init fail\n", __func__);
       goto init_fail;
    }
    ret = parser_load_from_file(pParser, file_name);
    if (ret) {
       ALOGD("%s: parser_load_from_file fail\n", __func__);
       goto init_fail;
    }
    return pParser;

init_fail:
    if (pParser)
        free(pParser);
    return NULL;
}

/*************************************************
Function: config_unload()
Description: free memory for the config
Input: null
Output: null
Return: 0 if success
*************************************************/
int aml_config_unload(struct parser *pParser)
{
    if (pParser != NULL) {
        parser_delete(pParser);
        free(pParser);
    }
    return 0;
}

/*************************************************
Function:     aml_config_get_str()
Description:  get value of index "key", in string
Input:
       pParser:
       section:
       key:       config index
       def_value: default value to return
Output:
Return:
*************************************************/
const char *aml_config_get_str(struct parser *pParser, const char *section,  const char *key, const char *def_value)
{
    if (pParser == NULL) {
        return def_value;
    }
    return parser_get_string(pParser, section, key, def_value);
}

/*************************************************
Function:     aml_config_get_int()
Description:  get value of index "key", in int
Input:
       pParser:
       section:
       key:       config index
       def_value: default value to return
Output:
Return:
*************************************************/
int aml_config_get_int(struct parser *pParser, const char *section, const char *key, const int def_value)
{
    if (pParser == NULL) {
        return def_value;
    }
    return parser_get_int(pParser, section, key, def_value);
}

/*************************************************
Function:     aml_config_get_float()
Description:  get value of index "key", in float
Input:
       pParser:
       section:
       key:       config index
       def_value: default value to return
Output:
Return:
*************************************************/
float aml_config_get_float(struct parser *pParser, const char *section, const char *key, const float def_value)
{
    if (pParser == NULL) {
        return def_value;
    }
    return parser_get_float(pParser, section, key, def_value);
}

/*************************************************
Function:     aml_config_dump()
Description:  dump valid content of the config file
Input:
       pParser:
Output:       config content list
Return:       0 if success
*************************************************/
int aml_config_dump(struct parser *pParser, const char *section)
{
    if (pParser == NULL) {
        return -1;
    }
	return parser_dump(pParser, section);
}

#ifndef AML_CONFIG_SUPPORT_READ_ONLY
/*************************************************
Function:     aml_config_set_str()
Description:  set key value
Input:
       pParser:
       section:
       key:
       value:
Output:
Return:        0 if success
*************************************************/
int aml_config_set_str(struct parser *pParser, const char *section,  const char *key, const char *value)
{
    if (pParser == NULL) {
        return -1;
    }
    return parser_set_string(pParser, section, key, value);
}

/*************************************************
Function:     aml_config_set_int()
Description:  set key value
Input:
       pParser:
       section:
       key:
       value:
Output:
Return:        0 if success
*************************************************/
int aml_config_set_int(struct parser *pParser, const char *section,  const char *key, int value)
{
    if (pParser == NULL) {
        return -1;
    }
    return parser_set_int(pParser, section, key, value);
}

/*************************************************
Function:     aml_config_set_float()
Description:  set key value
Input:
       pParser:
       section:
       key:
       value:
Output:
Return:        0 if success
*************************************************/
int aml_config_set_float(struct parser *pParser, const char *section,  const char *key, float value)
{
    if (pParser == NULL) {
        return -1;
    }
    return parser_set_float(pParser, section, key, value);
}
#endif
