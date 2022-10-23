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

#ifndef _PARSER_H_
#define _PARSER_H_

#define MAX_INI_FILE_LINE_LEN (512)

/*
 * example.conf
 * ----------------------------------------------------------------------------
 * [SECTION_0]
 *
 * # this is a comment
 * configname_1 = true
 * configname_2 = 1234
 * configname_3 = 11.2
 *
 * [SECTION_1]
 *
 * configname_1 = true
 * configname_2 = 1234
 * # this is a comment
 * configname_3 = 11.2
 * ----------------------------------------------------------------------------
 */
/**
 ** Line Type
 **/
typedef enum _LINE_TYPE {
    LINE_TYPE_SECTION = 0, // Section line
    LINE_TYPE_KEY,         // Config item line
    LINE_TYPE_COMMENT,     // Comment line
} eLineType;

/**
 ** Link of One Line
 **/
typedef struct _LINE {
    eLineType    type;                        // line type
    char         Text[MAX_INI_FILE_LINE_LEN]; // whole content of this line
    int          LineLen;                     // length
    char         *pKeyStart;                  // pointer to start of key
    char         *pKeyEnd;
    char         *pValueStart;                // pointer to start of value
    char         *pValueEnd;
    struct _LINE *pNext;                      // next line
} LINE;

/**
 ** Link of Section
 **/
typedef struct _SECTION {
    LINE            *pLine;                   // first line in this section
    struct _SECTION *pNext;                   // next section in this config file
} SECTION;

/**
 ** Parser
 **/
struct parser {
    char    mpFileName[256];                  // path of config file
    FILE    *m_pIniFile;                      // handle
    LINE    *mpFirstLine;                     // first line in this config file
    SECTION *mpFirstSection;                  // first section in this config file
};

/* load from */
#define AML_PARAM_AUDIO_HAL_SYSTEM "/system/etc/tvaudiohal.conf"
/* save to */
#define AML_PARAM_AUDIO_HAL_PARAM  "/param/tvaudiohal.conf"

#define AML_SECTION_AUDIO_HAL      "AUDIO_HAL"

/**
 **  APIS
 **/
int parser_init(struct parser *pParser);
int parser_delete(struct parser *pParser);
int parser_load_from_file(struct parser *pParser, const char *filename);
/* dump api */
int parser_dump(struct parser *pParser, const char *section);
/* get apis */
const char *parser_get_string(struct parser *pParser, const char *section, const char *key, const char *def_value);
int parser_get_int(struct parser *pParser, const char *section, const char *key, int def_value);
float parser_get_float(struct parser *pParser, const char *section, const char *key, float def_value);
#ifndef AML_CONFIG_SUPPORT_READ_ONLY
/* set apis */
int parser_set_string(struct parser *pParser, const char *section, const char *key, const char *value);
int parser_set_int(struct parser *pParser, const char *section, const char *key, int value);
int parser_set_float(struct parser *pParser, const char *section, const char *key, float value);
#endif

#endif
