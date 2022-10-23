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

#ifndef _LOADER_H_
#define _LOADER_H_

#ifdef __cplusplus
extern "C" {
#endif

struct parser *aml_config_load(const char *file_name);
int aml_config_unload(struct parser *pParser);
int aml_config_dump(struct parser *pParser, const char *section);
/* get */
const char *aml_config_get_str(struct parser *pParser, const char *section,  const char *key, const char *def_value);
int aml_config_get_int(struct parser *pParser, const char *section, const char *key, const int def_value);
float aml_config_get_float(struct parser *pParser, const char *section, const char *key, const float def_value);
#ifndef AML_CONFIG_SUPPORT_READ_ONLY
/* set */
int aml_config_set_str(struct parser *pParser, const char *section,  const char *key, const char *value);
int aml_config_set_int(struct parser *pParser, const char *section,  const char *key, int value);
int aml_config_set_float(struct parser *pParser, const char *section,  const char *key, float value);
#endif

#ifdef __cplusplus
}
#endif

#endif
