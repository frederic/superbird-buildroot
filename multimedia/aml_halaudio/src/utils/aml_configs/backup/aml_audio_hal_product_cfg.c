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
 ** aml_audio_hal_product_cfg.c
 **
 ** This program is designed for load the tvconfig section [AUDIO_HAL].
 ** author: shen pengru
 **
 */
#define LOG_TAG "aml_halcfg_parser"

#define LOG_NDEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <cutils/log.h>
#include "loader.h"
#include "parser.h"

// APIs for load/unload tvconfig
struct parser *aml_hal_config_load(const char *file_name)
{
	return aml_config_load(file_name);
}

int aml_hal_config_unload(struct parser *pParser)
{
	return aml_config_unload(pParser);
}

int aml_hal_config_dump(struct parser *pParser)
{
    return aml_config_dump(pParser, "AUDIO_HAL");
}

// APIs for get value
const char *aml_hal_cfg_get_str(struct parser *pParser, const char *key, const char *def_value)
{
	return aml_config_get_str(pParser, "AUDIO_HAL", key, def_value);
}

int aml_hal_config_get_int(struct parser *pParser, const char *key, const int def_value)
{
    return aml_config_get_int(pParser, "AUDIO_HAL", key, def_value);
}

float aml_hal_config_get_float(struct parser *pParser, const char *key, const float def_value)
{
    return aml_config_get_float(pParser, "AUDIO_HAL", key, def_value);
}
