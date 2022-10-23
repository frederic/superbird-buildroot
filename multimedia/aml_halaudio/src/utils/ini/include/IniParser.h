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

/*
 * Author:  Shoufu Zhao <shoufu.zhao@amlogic.com>
 */

#ifndef __INI_PARSER_H__
#define __INI_PARSER_H__

#define CC_MAX_INI_FILE_NAME_LEN    (512)
#define CC_MAX_INI_FILE_LINE_LEN    (2048)

#define CC_MAX_INI_LINE_NAME_LEN    (128)

typedef struct S_INI_LINE {
    struct S_INI_LINE *pNext;
    char Name[CC_MAX_INI_LINE_NAME_LEN];
    char Value[CC_MAX_INI_FILE_LINE_LEN];
} INI_LINE;

typedef struct S_INI_SECTION {
    INI_LINE* pLine;
    INI_LINE* pCurLine;
    struct S_INI_SECTION *pNext;
    char Name[CC_MAX_INI_LINE_NAME_LEN];
} INI_SECTION;

class IniParser
{
public:
    IniParser();
    ~IniParser();

    void free();
    int parse(const char* fname);
    int SetSaveFileName(const char* fname);
    int parse_mem(char* file_buf);
    static INI_SECTION* getSection(void* user, const char* section);
    const char* GetString(const char* section, const char* key, const char* def_value);
    int SetString(const char *section, const char *key, const char *value);
    void print();

private:
    int SaveToFile(const char *filename);

    static void trim(char *str, char ch);
    static void trim(char *str);
    static INI_LINE* getKeyLineAtSec(INI_SECTION* pSec, const char* key);
    static int setKeyValue(void* user, const char* section, const char* name, const char* value, int set_mode);
    static int handler(void* user, const char* section, const char* name, const char* value);
    static INI_LINE* newLine(const char* name, const char* value);
    static INI_SECTION* newSection(const char* section, INI_LINE* pLINE);

private:
    INI_SECTION* mpFirstSection;
    INI_SECTION* mpCurSection;

    char mpFileName[CC_MAX_INI_FILE_NAME_LEN];
};

//for memory new and delete debug
#define CC_MEMORY_NEW_DEL_TRACE              (1)
#define CC_MEMORY_NEW_DEL_TRACE_PRINT_ALL    (0)

#if CC_MEMORY_NEW_DEL_TRACE == 1
static void new_mem(const char *fun_name, const char *var_name, void *ptr);
static void del_mem(const char *fun_name, const char *var_name, void *ptr);
static void printDelMemND(const char *fun_name);
static void printNewMemND(const char *fun_name);
static void clearMemND();
#endif

#endif //__INI_PARSER_H__
