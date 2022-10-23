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

#ifndef _AML_ANDROID_UTILS_H_
#define _AML_ANDROID_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum AML_BOOLEAN_DEFINE {
	eAML_BOOL_FALSE = 0,
	eAML_BOOL_TRUE  = 1,
} eAmlBoolean;

/*
 * Android Property Interface
 */
int aml_getprop_bool(const char *path);
int aml_getprop_int(const char *path);

/*
 * Sys Fs Interface
 */
int aml_sysfs_get_int(const char *path);
int aml_sysfs_get_int16(const char *path, unsigned *value);
int aml_sysfs_get_str(const char *path, char *buf, int count);
int aml_sysfs_set_int(const char *path, int value);
int aml_sysfs_set_str(const char *path, const char *value);

/*
 * Others
 */
int aml_strstr(char *mystr,char *substr);

#ifdef __cplusplus
}
#endif

#endif
