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
 ** parser.c
 **
 ** This program is designed for load the config of product in HAL.
 **  - base on config loader/parser in aml tvserver!!
 ** author: aml&shen pengru
 **
 */
#define LOG_TAG "aml_parser"

#define LOG_NDEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "log.h"
#include <aml_conf_parser.h>
#include <errno.h>
#include <unistd.h>


/*************************************************
    Utils
*************************************************/
#define READ_BUFF 1024
int _copy_file(const char *src_file_name, const char *des_file_name)
{
    int ret = 0;
    int nread;
    char buf[READ_BUFF];
    FILE *fp_src = NULL;
    FILE *fp_des = NULL;

    if ((NULL == src_file_name) || (NULL == des_file_name)) {
        ALOGE("[%s:%d]sourece file doesn't exist!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    fp_src = fopen(src_file_name, "r+");
    if (NULL == fp_src) {
        ALOGE("[%s:%d] %s error %s\n", __FUNCTION__, __LINE__, src_file_name, strerror(errno));
        return -1;
    }

    fp_des = fopen(des_file_name, "a");
    if (NULL == fp_des) {
        ALOGE("[%s:%d] %s error %s\n", __FUNCTION__, __LINE__, des_file_name, strerror(errno));
        ret = -1;
        goto FAIL;
    }

#if 0
    while ( nread = fread(buf, sizeof(char), READ_BUFF, fp_src)) {
        fwrite(buf, sizeof(char), nread, fp_des);
    }
#else
    do {
        nread = fread(buf, sizeof(char), READ_BUFF, fp_src);
        fwrite(buf, sizeof(char), nread, fp_des);
    } while(nread);
#endif

    fclose(fp_des);
FAIL:
    fclose(fp_src);

    return ret;
}

static void _delete_all_trim(char *Str)
{
    char *pStr;
    pStr = strchr (Str, '\n');
    if (pStr != NULL) {
        *pStr = 0;
    }
    int Len = strlen(Str);
    if ( Len > 0 ) {
        if ( Str[Len - 1] == '\r' ) {
            Str[Len - 1] = '\0';
        }
    }
    pStr = Str;
    while (*pStr != '\0') {
        if (*pStr == ' ') {
            char *pTmp = pStr;
            while (*pTmp != '\0') {
                *pTmp = *(pTmp + 1);
                pTmp++;
            }
        } else {
            pStr++;
        }
    }
    return;
}

static eLineType _get_line_type(char *Str)
{
    eLineType type = LINE_TYPE_COMMENT;
    if (strchr(Str, '#') != NULL) {
        type = LINE_TYPE_COMMENT;
    } else if ( (strstr (Str, "[") != NULL) && (strstr (Str, "]") != NULL) ) { /* Is Section */
        type = LINE_TYPE_SECTION;
    } else {
        if (strstr (Str, "=") != NULL) {
            type = LINE_TYPE_KEY;
        } else {
            type = LINE_TYPE_COMMENT;
        }
    }
    return type;
}

static SECTION *_get_section(struct parser *pParser, const char *section)
{
    SECTION *psec = NULL;

    if (pParser->mpFirstSection == NULL)
        return NULL;

    //section
    for (psec = pParser->mpFirstSection; psec != NULL; psec = psec->pNext) {
        if (strncmp((psec->pLine->Text) + 1, section, strlen(section)) == 0)
            return psec;
    }
    return NULL;
}

static LINE *_get_key_line_at_sec(SECTION *pSec, const char *key)
{
    LINE *pline = NULL;
    //line
    for (pline = pSec->pLine->pNext; (pline != NULL && pline->type != LINE_TYPE_SECTION); pline = pline->pNext) {
        if (pline->type == LINE_TYPE_KEY) {
            if (strncmp(pline->Text, key, strlen(key)) == 0)
                return pline;
        }
    }
    return NULL;
}

/*************************************************
    APIs
*************************************************/
int parser_init(struct parser *pParser)
{
    if (pParser != NULL) {
        pParser->mpFirstSection = NULL;
        pParser->mpFileName[0]  = '\0';
        pParser->m_pIniFile     = NULL;
        pParser->mpFirstLine    = NULL;
        return 0;
    }
    return -1;
}

int parser_delete(struct parser *pParser)
{
    LINE    *pCurLine  = NULL;
    LINE    *pNextLine = NULL;
    SECTION *pCurSec   = NULL;
    SECTION *pNextSec  = NULL;

    //free all lines
    for (pCurLine = pParser->mpFirstLine; pCurLine != NULL;) {
        pNextLine = pCurLine->pNext;
        free(pCurLine);
        pCurLine = pNextLine;
    }
    pParser->mpFirstLine = NULL;

    //free all sections
    for (pCurSec = pParser->mpFirstSection; pCurSec != NULL;) {
        pNextSec = pCurSec->pNext;
        free(pCurSec);
        pCurSec = pNextSec;
    }
    pParser->mpFirstSection = NULL;

    return 0;
}

int parser_load_from_file(struct parser *pParser, const char *filename)
{
    char    lineStr[MAX_INI_FILE_LINE_LEN];
    char    *pStr;
    LINE    *pCurLINE    = NULL;
    SECTION *pCurSection = NULL;
    char             *pM = NULL;
    SECTION        *pSec = NULL;

    // open the config file
    ALOGD("%s: name = %s", __func__, filename);
    if (filename == NULL) {
        ALOGE("[%s:%d]\n", __func__, __LINE__);
        return -1;
    }
    strcpy(pParser->mpFileName, filename);
    pParser->m_pIniFile = fopen(pParser->mpFileName, "r");
    if (pParser->m_pIniFile == NULL) {
        // open default config file
        ALOGE("[%s:%d]open file %s failed error %s\n", __func__, __LINE__, pParser->mpFileName, strerror(errno));
        if (_copy_file(AML_PARAM_AUDIO_HAL_SYSTEM, pParser->mpFileName)) {
            ALOGE("[%s:%d]\n", __func__, __LINE__);
            return -1;
        }
        else {
            // reopen
            ALOGD("%s: copy config from system partition!\n", __func__);
            pParser->m_pIniFile = fopen(pParser->mpFileName, "r");
            if (pParser->m_pIniFile == NULL) {
                ALOGE("[%s:%d] error %s\n", __func__, __LINE__, strerror(errno));
                return -1;
            }
        }
    }

    // parse line by line
    while (fgets(lineStr, MAX_INI_FILE_LINE_LEN, pParser->m_pIniFile) != NULL) {
        /* remove invaild info */
        _delete_all_trim(lineStr);

        /* initial one new line structrue */
        LINE *pLINE = malloc(sizeof(LINE));
        pLINE->pKeyStart   = pLINE->Text;
        pLINE->pKeyEnd     = pLINE->Text;
        pLINE->pValueStart = pLINE->Text;
        pLINE->pValueEnd   = pLINE->Text;
        pLINE->pNext       = NULL;
        pLINE->type        = _get_line_type(lineStr);
        strcpy(pLINE->Text, lineStr);
        pLINE->LineLen = strlen(pLINE->Text);

        /* insert the the link */
        if (pParser->mpFirstLine == NULL) {
            pParser->mpFirstLine = pLINE;
        } else {
            pCurLINE->pNext = pLINE;
        }
        pCurLINE = pLINE;

        /* real parser */
        switch (pCurLINE->type) {
        case LINE_TYPE_SECTION:
            pSec = malloc(sizeof(SECTION));
            pSec->pLine = pLINE;
            pSec->pNext = NULL;
            if (pParser->mpFirstSection == NULL) { //first section
                pParser->mpFirstSection = pSec;
            } else {
                pCurSection->pNext = pSec;
            }
            pCurSection = pSec;
            break;
        case LINE_TYPE_KEY:
            pM = strchr(pCurLINE->Text, '=');
            pCurLINE->pKeyStart   = pCurLINE->Text;                         // Key, start in ->Text
            pCurLINE->pKeyEnd     = pM - 1;                                 // Key, end   in ->Text
            pCurLINE->pValueStart = pM + 1;                                 // Val, start in ->Text
            pCurLINE->pValueEnd   = pCurLINE->Text + pCurLINE->LineLen - 1; // Val, end   in ->Text
            break;
        case LINE_TYPE_COMMENT:
        default:
            break;
        }
    }

    fclose (pParser->m_pIniFile);
    pParser->m_pIniFile = NULL;
    return 0;
}

int parser_dump(struct parser *pParser, const char *section)
{
    LINE *pline = NULL;
    SECTION *pSec = NULL;

    ALOGD("%s: === start to dump %s ===\n", __func__, pParser->mpFileName);

    pSec = _get_section(pParser, section);
    if (pSec == NULL)
        return -1;

    for (pline=pSec->pLine; pline!=NULL; pline=pline->pNext) {
        if (pline != NULL) {
            if (pSec->pNext != NULL && pline == pSec->pNext->pLine) {
                break;
            }
        }
        if (pline->type == LINE_TYPE_KEY) {
            printf("%s\n", pline->Text);
        }
    }

    ALOGD("%s: === dump over %s ===\n", __func__, pParser->mpFileName);

	return 0;
}

const char *parser_get_string(struct parser *pParser, const char *section, const char *key, const char *def_value)
{
    SECTION *pSec = _get_section(pParser, section);
    if (pSec == NULL)
        return def_value;

    LINE *pLine = _get_key_line_at_sec(pSec, key);
    if (pLine == NULL)
        return def_value;

    return pLine->pValueStart;
}

int parser_get_int(struct parser *pParser, const char *section, const char *key, int def_value)
{
    const char *num = parser_get_string(pParser, section, key, NULL);
    if (num != NULL) {
        return atoi(num);
    }
    return def_value;
}

float parser_get_float(struct parser *pParser, const char *section, const char *key, float def_value)
{
    const char *num = parser_get_string(pParser, section, key, NULL);
    if (num != NULL) {
        return atof(num);
    }
    return def_value;
}

#ifndef AML_CONFIG_SUPPORT_READ_ONLY
static int _save_to_file(struct parser *pParser, const char *filename)
{
	const char *filepath = NULL;
	FILE *pFile = NULL;

	if (filename == NULL) {
		if (strlen(pParser->mpFileName) == 0) {
			ALOGD("error save file is null");
			return -1;
		} else {
			filepath = pParser->mpFileName;
		}
	} else {
		filepath = filename;
	}

	if ((pFile = fopen (filepath, "wb")) == NULL) {
		ALOGD("Save to file open error = %s", filepath);
		return -1;
	}

	LINE *pCurLine = NULL;
	for (pCurLine = pParser->mpFirstLine; pCurLine != NULL; pCurLine = pCurLine->pNext) {
		fprintf (pFile, "%s\r\n", pCurLine->Text);
	}

	fflush(pFile);
	fsync(fileno(pFile));
	fclose(pFile);

	return 0;
}

static int _insert_section(struct parser *pParser, SECTION *pSec)
{
	//insert it to sections list ,as first section
	pSec->pNext             = pParser->mpFirstSection;
	pParser->mpFirstSection = pSec;

	//insert it to lines list, at first
	pSec->pLine->pNext      = pParser->mpFirstLine;
	pParser->mpFirstLine    = pSec->pLine;

	return 0;
}

static int _insert_keyline(SECTION *pSec, LINE *line)
{
	LINE *line1 = pSec->pLine;
	LINE *line2 = line1->pNext;

	line1->pNext = line;
	line->pNext  = line2;

	return 0;
}

int parser_set_string(struct parser *pParser, const char *section, const char *key, const char *value)
{
	SECTION *pFindSec    = NULL;

	SECTION *pNewSec     = NULL;
	LINE    *pNewSecLine = NULL;
	LINE    *pNewKeyLine = NULL;

	pFindSec = _get_section(pParser, section);
	if (pFindSec == NULL) {
		/* CASE_1: can't find section. new section, new line */
		pNewSec     = malloc(sizeof(SECTION));
		pNewSecLine = malloc(sizeof(LINE));
		pNewKeyLine = malloc(sizeof(LINE));
		pNewKeyLine->type = LINE_TYPE_KEY;
		pNewSecLine->type = LINE_TYPE_SECTION;
		sprintf(pNewSecLine->Text, "[%s]", section);
		pNewSec->pLine = pNewSecLine;
		// section insert
		_insert_section(pParser, pNewSec);

		int keylen = strlen(key);
		sprintf(pNewKeyLine->Text, "%s=%s", key, value);
		pNewKeyLine->LineLen     = strlen(pNewKeyLine->Text);
		pNewKeyLine->pKeyStart   = pNewKeyLine->Text;
		pNewKeyLine->pKeyEnd     = pNewKeyLine->pKeyStart + keylen               - 1;
		pNewKeyLine->pValueStart = pNewKeyLine->pKeyStart + keylen               + 1;
		pNewKeyLine->pValueEnd   = pNewKeyLine->Text      + pNewKeyLine->LineLen - 1;
		// line insert
		_insert_keyline(pNewSec, pNewKeyLine);
	} else {
		/* CASE_2: find out the section */
		LINE *pLine = _get_key_line_at_sec(pFindSec, key);
		if (pLine == NULL) {
			/* CASE_2.1: can't find line. new line */
			pNewKeyLine       = malloc(sizeof(LINE));
			pNewKeyLine->type = LINE_TYPE_KEY;
			int keylen = strlen(key);
			sprintf(pNewKeyLine->Text, "%s=%s", key, value);
			pNewKeyLine->LineLen     = strlen(pNewKeyLine->Text);
			pNewKeyLine->pKeyStart   = pNewKeyLine->Text;
			pNewKeyLine->pKeyEnd     = pNewKeyLine->pKeyStart + keylen               - 1;
			pNewKeyLine->pValueStart = pNewKeyLine->pKeyStart + keylen               + 1;
			pNewKeyLine->pValueEnd   = pNewKeyLine->Text      + pNewKeyLine->LineLen - 1;

			// line insert
			_insert_keyline(pFindSec, pNewKeyLine);
		} else {
			/* CASE_2.2: find out the section&line */
			sprintf(pLine->Text, "%s=%s", key, value);
			pLine->LineLen   = strlen(pLine->Text);
			pLine->pValueEnd = pLine->Text + pLine->LineLen - 1;
		}
	}

	//save
	_save_to_file(pParser, NULL);

	return 0;
}

int parser_set_int(struct parser *pParser, const char *section, const char *key, int value)
{
	char tmp[64];

	sprintf(tmp, "%d", value);
	parser_set_string(pParser, section, key, tmp);

	return 0;
}

int parser_set_float(struct parser *pParser, const char *section, const char *key, float value)
{
	char tmp[64];

	sprintf(tmp, "%.2f", value);
	parser_set_string(pParser, section, key, tmp);

	return 0;
}
#endif
