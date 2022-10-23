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

#define LOG_TAG "IniParser"
#define LOG_NDEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

#include "log.h"

#include "ini.h"
#include "IniParser.h"

IniParser::IniParser() {
    ALOGD("%s, entering...\n", __FUNCTION__);
    mpFirstSection = NULL;
    mpCurSection = NULL;

    mpFileName[0] = '\0';
}

IniParser::~IniParser() {
    ALOGD("%s, entering...\n", __FUNCTION__);
    free();
}

int IniParser::parse(const char* filename) {
    ALOGD("%s, entering...\n", __FUNCTION__);

    strncpy(mpFileName, filename, CC_MAX_INI_FILE_NAME_LEN - 1);
    return ini_parse(filename, handler, this);
}

int IniParser::parse_mem(char* file_buf) {
    ALOGD("%s, entering...\n", __FUNCTION__);
    return ini_parse_mem(file_buf, handler, this);
}

int IniParser::SetSaveFileName(const char* filename) {
    ALOGD("%s, entering...\n", __FUNCTION__);

    strncpy(mpFileName, filename, CC_MAX_INI_FILE_NAME_LEN - 1);
    return 0;
}

void IniParser::free() {
    ALOGD("%s, entering...\n", __FUNCTION__);

    INI_SECTION* pNextSec = NULL;
    for (INI_SECTION* pSec = mpFirstSection; pSec != NULL;) {
        pNextSec = pSec->pNext;

        INI_LINE* pNextLine = NULL;
        for (INI_LINE* pLine = pSec->pLine; pLine != NULL;) {
            pNextLine = pLine->pNext;

            if (pLine != NULL) {
#if CC_MEMORY_NEW_DEL_TRACE == 1
                del_mem(__FUNCTION__, "pLine", pLine);
#endif

                delete pLine;
                pLine = NULL;
            }

            pLine = pNextLine;
        }

        if (pSec != NULL) {
#if CC_MEMORY_NEW_DEL_TRACE == 1
            del_mem(__FUNCTION__, "pSec", pSec);
#endif

            delete pSec;
            pSec = NULL;
        }

        pSec = pNextSec;
    }

    mpFirstSection = NULL;
    mpCurSection = NULL;

#if CC_MEMORY_NEW_DEL_TRACE == 1
    printNewMemND(__FUNCTION__);
    printDelMemND(__FUNCTION__);
    clearMemND();
#endif
}

void IniParser::trim(char *str, char ch) {
    char* pStr;

    pStr = str;
    while (*pStr != '\0') {
        if (*pStr == ch) {
            char* pTmp = pStr;
            while (*pTmp != '\0') {
                *pTmp = *(pTmp + 1);
                pTmp++;
            }
        } else {
            pStr++;
        }
    }
}

void IniParser::trim(char *str) {
    char* pStr = NULL;

    pStr = strchr(str, '\n');
    if (pStr != NULL) {
        *pStr = 0;
    }

    int Len = strlen(str);
    if (Len > 0) {
        if (str[Len - 1] == '\r') {
            str[Len - 1] = '\0';
        }
    }

    pStr = strchr(str, '#');
    if (pStr != NULL) {
        *pStr = 0;
    }

    pStr = strchr(str, ';');
    if (pStr != NULL) {
        *pStr = 0;
    }

    trim(str, ' ');
    trim(str, '{');
    trim(str, '\\');
    trim(str, '}');
    trim(str, '\"');
    return;
}

void IniParser::print() {
    for (INI_SECTION* pSec = mpFirstSection; pSec != NULL; pSec = pSec->pNext) {
        ALOGD("[%s]\n", pSec->Name);
        for (INI_LINE* pLine = pSec->pLine; pLine != NULL; pLine = pLine->pNext) {
            ALOGD("%s = %s\n", pLine->Name, pLine->Value);
        }
        ALOGD("\n\n\n");
    }
}

INI_SECTION* IniParser::getSection(void* user, const char* section) {
    IniParser* parser = (IniParser*) user;
    if (parser == NULL) {
        return NULL;
    }

    for (INI_SECTION* pSec = parser->mpFirstSection; pSec != NULL; pSec = pSec->pNext) {
        if (strncmp(pSec->Name, section, strlen(section)) == 0) {
            return pSec;
        }
    }

    return NULL;
}

INI_LINE* IniParser::getKeyLineAtSec(INI_SECTION* pSec, const char* key) {
    for (INI_LINE* pLine = pSec->pLine; pLine != NULL; pLine = pLine->pNext) {
        if (strncmp(pLine->Name, key, strlen(key)) == 0) {
            return pLine;
        }
    }
    return NULL;
}

const char* IniParser::GetString(const char* section, const char* key,
        const char* def_value) {
    INI_SECTION* pSec = getSection(this, section);
    if (pSec == NULL) {
        //ALOGD("%s, section %s is NULL\n", __FUNCTION__, section);
        return def_value;
    }

    INI_LINE* pLine = getKeyLineAtSec(pSec, key);
    if (pLine == NULL) {
        //ALOGD("%s, key \"%s\" is NULL\n", __FUNCTION__, key);
        return def_value;
    }

    return pLine->Value;
}

int IniParser::SaveToFile(const char *filename) {
    const char *fname = NULL;
    FILE *fp = NULL;

    if (filename == NULL) {
        if (strlen(mpFileName) == 0) {
            ALOGE("%s, save file name is NULL!!!\n", __FUNCTION__);
            return -1;
        } else {
            fname = mpFileName;
        }
    } else {
        fname = filename;
    }

    if ((fp = fopen (fname, "wb")) == NULL) {
        ALOGE("%s, Open file \"%s\" ERROR (%s)!!!\n", __FUNCTION__, fname, strerror(errno));
        return -1;
    }

    for (INI_SECTION* pSec = mpFirstSection; pSec != NULL; pSec = pSec->pNext) {
        fprintf(fp, "[%s]\r\n", pSec->Name);
        for (INI_LINE* pLine = pSec->pLine; pLine != NULL; pLine = pLine->pNext) {
            fprintf(fp, "%s = %s\r\n", pLine->Name, pLine->Value);
        }
    }

    fflush(fp);
    fsync(fileno(fp));

    fclose(fp);
    fp = NULL;

    return 0;
}

int IniParser::SetString(const char *section, const char *key, const char *value) {
    setKeyValue(this, section, key, value, 1);
    SaveToFile(NULL);
    return 0;
}

INI_LINE* IniParser::newLine(const char* name, const char* value) {
    INI_LINE* pLine = NULL;

    pLine = new INI_LINE();
    if (pLine != NULL) {
        pLine->pNext = NULL;
        strcpy(pLine->Name, name);
        strcpy(pLine->Value, value);

#if CC_MEMORY_NEW_DEL_TRACE == 1
        new_mem(__FUNCTION__, "pLine", pLine);
#endif
    }

    return pLine;
}

INI_SECTION* IniParser::newSection(const char* section, INI_LINE* pLine) {
    INI_SECTION* pSec = NULL;

    pSec = new INI_SECTION();
    if (pSec != NULL) {
        pSec->pLine = pLine;
        pSec->pNext = NULL;
        strcpy(pSec->Name, section);

#if CC_MEMORY_NEW_DEL_TRACE == 1
        new_mem(__FUNCTION__, "pSec", pSec);
#endif
    }

    return pSec;
}

int IniParser::setKeyValue(void* user, const char* section, const char* key, const char* value, int set_mode) {
    IniParser* parser = NULL;
    INI_LINE* pLine = NULL;
    INI_SECTION *pSec = NULL;

    if (user == NULL || section == NULL || key == NULL || value == NULL) {
        return 1;
    }

    parser = (IniParser*) user;

    parser->trim((char *) value);
    if (value[0] == '\0') {
        return 1;
    }

    if (parser->mpFirstSection == NULL) {
        pLine = newLine(key, value);
        pSec = newSection(section, pLine);

        parser->mpFirstSection = pSec;
        parser->mpCurSection = pSec;
        pSec->pCurLine = pLine;
    } else {
        pSec = getSection(user, section);
        if (pSec == NULL) {
            pLine = newLine(key, value);
            pSec = newSection(section, pLine);

            parser->mpCurSection->pNext = pSec;
            parser->mpCurSection = pSec;
            pSec->pCurLine = pLine;

            pSec->pCurLine = pLine;
        } else {
            pLine = getKeyLineAtSec(pSec, key);
            if (pLine == NULL) {
                pLine = newLine(key, value);

                pSec->pCurLine->pNext = pLine;
                pSec->pCurLine = pLine;
            } else {
                if (set_mode == 1) {
                    strcpy(pLine->Value, value);
                } else {
                    strcat(pLine->Value, value);
                }
            }
        }
    }

    return 0;
}

int IniParser::handler(void* user, const char* section, const char* name,
        const char* value) {
    //ALOGD("%s, section = %s, name = %s, value = %s\n", __FUNCTION__, section, name, value);
    setKeyValue(user, section, name, value, 0);
    return 1;
}

#if CC_MEMORY_NEW_DEL_TRACE == 1

#define CC_MEM_RECORD_CNT    (1024)

typedef struct tag_memnd {
    char fun_name[50];
    char var_name[50];
    void *ptr;
} memnd;

static memnd gMemNewItems[CC_MEM_RECORD_CNT];
static int gMemNewInd = 0;

static memnd gMemDelItems[CC_MEM_RECORD_CNT];
static int gMemDelInd = 0;

static void new_mem(const char *fun_name, const char *var_name, void *ptr) {
    strcpy(gMemNewItems[gMemNewInd].fun_name, fun_name);
    strcpy(gMemNewItems[gMemNewInd].var_name, var_name);
    gMemNewItems[gMemNewInd].ptr = ptr;

    gMemNewInd += 1;
}

static void del_mem(const char *fun_name, const char *var_name, void *ptr) {
    strcpy(gMemDelItems[gMemDelInd].fun_name, fun_name);
    strcpy(gMemDelItems[gMemDelInd].var_name, var_name);
    gMemDelItems[gMemDelInd].ptr = ptr;

    gMemDelInd += 1;
}

static void printMemND(const char *fun_name, memnd *tmp_nd, int tmp_cnt) {
    int i = 0;

    ALOGD("fun_name = %s, total_cnt = %d\n", fun_name, tmp_cnt);

#if CC_MEMORY_NEW_DEL_TRACE_PRINT_ALL == 1
    for (i = 0; i < tmp_cnt; i++) {
        ALOGD("fun_name = %s, var_name = %s, ptr = %p\n", tmp_nd[i].fun_name, tmp_nd[i].var_name, tmp_nd[i].ptr);
    }
#endif
}

static void printDelMemND(const char *fun_name) {
    printMemND(__FUNCTION__, gMemDelItems, gMemDelInd);
}

static void printNewMemND(const char *fun_name) {
    printMemND(__FUNCTION__, gMemNewItems, gMemNewInd);
}

static void clearMemND() {
    int i = 0;

    gMemNewInd = 0;
    gMemDelInd = 0;
    memset((void *)gMemNewItems, 0, sizeof(memnd) * CC_MEM_RECORD_CNT);
    memset((void *)gMemDelItems, 0, sizeof(memnd) * CC_MEM_RECORD_CNT);
}
#endif
