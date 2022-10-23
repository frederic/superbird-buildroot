/*
 * Copyright (C) 2018 Amlogic Corporation.
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
#define LOG_TAG "aml_datmos_config"

#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <string>
#include <vector>
#include "log.h"
#include "audio_hw.h"

#include "aml_datmos_config.h"

//#define DATMOS_DEBUG

using namespace std;

class Option
{
public:
    Option(const char *k, const char *v)
    {
        this->key = string(k);
        this->value = string(v);
    }

    ~Option() {}

    void set_key(const char *k)
    {
        if (k != NULL) {
            this->key = string(k);
        }
    }

    void set_value(const char *v)
    {
        if (v == NULL) {
            this->value = string("");
        } else {
            this->value = string(v);
        }
    }

    const char *get_key()
    {
        return key.c_str();
    }

    const char *get_value()
    {
        if (strlen(value.c_str()) > 0) {
            return value.c_str();
        } else {
            return NULL;
        }
    }

private:
    string key;
    string value;
};

static vector<Option> opts;

static char **atmos_params = NULL;


extern "C" void *datmos_default_options()
{
    opts.clear();
    opts.push_back(Option("sidk", ""));
    opts.push_back(Option("-o", "/tmp/out_dut.wav"));
    opts.push_back(Option("-bo", "24"));
    opts.push_back(Option("-speakers", "lr:c:lfe:lrs:lrrs"));
    opts.push_back(Option("-noupresampler", "1"));
    opts.push_back(Option("-dec_joc", "0"));
    opts.push_back(Option("-inputpcm", "inputpcm_mode=disable"));
#ifdef DATMOS_DEBUG
    opts.push_back(Option("-dbgout", "0x1f"));
#endif

    return (void *)&opts;
}

extern "C" void *get_datmos_current_options()
{
    ALOGI("");
    return (void *)&opts;
}


extern "C" void add_datmos_option(void *opt_handle, const char *key, const char *value)
{
    ALOGI("key:%s value:%s", key, value);

    vector<Option> *opt = (vector<Option>*)opt_handle;
    bool haskey = false;

    for (vector<Option>::iterator it = opt->begin() ; it != opt->end(); ++it) {
        const char *item_key = it->get_key();
        if (0 == strcmp(item_key, key)) {
            it->set_value(value);
            haskey = true;
            break;
        }
    }

    if (!haskey) {
        opt->push_back(Option(key, value));
    }
    ALOGI("opt->size() %d", opt->size());
}

extern "C" void delete_datmos_option(void *opt_handle, const char *key)
{
    ALOGI("key:%s", key);

    vector<Option> *opt = (vector<Option>*)opt_handle;

    for (int i = 0; i < opt->size(); i++) {
        Option item = opt->at(i);
        string item_key = item.get_key();
        if (0 == item_key.compare(key)) {
            opt->erase(opt->begin() + i);
        }
    }
    ALOGI("opt->size() %d", opt->size());
}

extern "C" bool get_datmos_config(void *opt_handle, char **init_argv, int *init_argc)
{
    ALOGI("");
    vector<Option> *opt = (vector<Option>*)opt_handle;

    int opt_count = opt->size();
    int i = 0, k = 0;

    for (i = 0; i < opt_count; i++) {
        Option &item = opt->at(i);
        const char *key = item.get_key();
        const char *value =  item.get_value();

        if (key != NULL) {
            memset(init_argv[k], 0, VALUE_BUF_SIZE);
            memcpy(init_argv[k],  key, strlen(key));
            k++;
        }

        if (k >= MAX_PARAM_COUNT) {
            break;
        }

        if (value != NULL) {
            memset(init_argv[k], 0, VALUE_BUF_SIZE);
            memcpy(init_argv[k],  value, strlen(value));
            k++;
        }

        if (k >= MAX_PARAM_COUNT) {
            break;
        }
    }

    ALOGI("params count:%d\n\n", k);
    // ALOGI("\n\n*****************************************************************\n\n");
    for (i = 0; i < k; i++) {
        printf("%s ", init_argv[i]);
    }
    printf("\n");
    // ALOGI("\n\n*****************************************************************\n");
    *init_argc = k;

    return 0;
}



