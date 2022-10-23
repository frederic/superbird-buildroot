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

#include <stdlib.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include "list.h"

#include "log.h"
#include "aml_audio_log.h"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 16 * ( EVENT_SIZE + 16 ) )

#define DEBUG_FILE_NAME "/tmp/AML_AUDIO_DEBUG"

#define AML_AUDIO_DEBUG_LEVEL "AML_AUDIO_DEBUG"

#define AML_AUDIO_DUMP_INFO  "AML_AUDIO_DUMP_INFO"

#define AML_AUDIO_DUMP_FILE  "AML_AUDIO_DUMP_FILE"

typedef void (*debug_action_func_t)(char *);

typedef struct debug_item {
    char name[32];
    debug_action_func_t action;
} debug_item_t;


typedef struct aml_audio_log {
    int bExit;
    pthread_t threadid;
    int notify_fd;
    int watch_desc;
    struct listnode dumpinfo_list;
    struct listnode dumpfile_list;
} aml_audio_log_t;


typedef struct aml_log_dump_info {
    struct listnode listnode;
    char name[32];
    dump_info_func_t dump_func;
    void * private_date;

} aml_log_dump_info_t;

typedef struct aml_log_dump_file {
    struct listnode listnode;
    char name[32];
    int  dump_enable;
} aml_log_dump_file_t;

static void parse_debug_level(char * data);
static void parse_debug_info(char * data);
static void parse_debug_dump_file(char * data);



static debug_item_t debugitems[] = {
    {AML_AUDIO_DEBUG_LEVEL, parse_debug_level},
    {AML_AUDIO_DUMP_INFO, parse_debug_info},
    {AML_AUDIO_DUMP_FILE, parse_debug_dump_file},
};

static aml_audio_log_t * aml_log_handle = NULL;
int aml_audio_debug_level = LEVEL_FATAL;
static char buffer[BUF_LEN];


static void parse_debug_level(char * data)
{
    char *end;
    char *token = NULL;
    int debug_level = aml_audio_debug_level;
    token = strtok(data, "=");
    if (token != NULL) {
        token = strtok(NULL, "=");
        if (token != NULL) {
            debug_level = (int)strtol(token, &end, 0);
            if (debug_level != aml_audio_debug_level) {
                printf("Debug level changed from =%d to %d\n", aml_audio_debug_level, debug_level);
                aml_audio_debug_level = debug_level;
            }
        }
    }
    return;
}

static void parse_debug_info(char * data)
{
    struct listnode *node = NULL;
    aml_log_dump_info_t *dump_info;
    printf("info name: %s\n", data);
    char *end;
    char *token = NULL;
    token = strtok(data, "=");
    if (token != NULL) {
        token = strtok(NULL, "=");
        if (token != NULL) {
            list_for_each(node, &aml_log_handle->dumpinfo_list) {
                dump_info = node_to_item(node, aml_log_dump_info_t, listnode);
                if (!strncmp(dump_info->name, token, strlen(dump_info->name))) {
                    if (dump_info->dump_func) {
                        dump_info->dump_func(dump_info->private_date);
                    }
                    break;
                }

            }
        }
    }
    return;
}

static void parse_debug_dump_file(char * data)
{
    struct listnode *node = NULL;
    aml_log_dump_file_t *dump_file;
    char *end;
    char *token = NULL;
    char *enable = NULL;
    int bfound = 0;
    token = strtok(data, "=");
    if (token != NULL) {
        token = strtok(NULL, "=");
        if (token != NULL) {
            enable = strtok(NULL, "=");

            list_for_each(node, &aml_log_handle->dumpfile_list) {
                dump_file = node_to_item(node, aml_log_dump_file_t, listnode);
                if (!strncmp(dump_file->name, token, strlen(token))) {
                    if (enable) {
                        dump_file->dump_enable = atoi(enable);
                    }
                    bfound = 1;
                    break;
                }

            }
        }
    }
    /*remove all the dump file setting*/
    if (!strncmp("disable_all", token, strlen("disable_all"))) {
        list_for_each(node, &aml_log_handle->dumpfile_list) {
            dump_file = node_to_item(node, aml_log_dump_file_t, listnode);
            printf("disable dump setting =%s\n", dump_file->name);
            list_remove(&dump_file->listnode);
            free(dump_file);
            node = &aml_log_handle->dumpfile_list;
        }
        return;
    }

    /*show all the dump file setting*/
    if (!strncmp("show_all", token, strlen("show_all"))) {
        list_for_each(node, &aml_log_handle->dumpfile_list) {
            dump_file = node_to_item(node, aml_log_dump_file_t, listnode);
            printf("dump setting =%s enable=%d\n", dump_file->name,dump_file->dump_enable);
        }
        return;
    }



    if (bfound == 0) {
        dump_file = calloc(1,sizeof(aml_log_dump_file_t));
        if (token != NULL) {
            strncpy(dump_file->name, token, strlen(token));
            if (enable) {
                dump_file->dump_enable = atoi(enable);
            }

        }
        list_add_head(&aml_log_handle->dumpfile_list, &dump_file->listnode);
    }
    if (dump_file) {
        printf("dump file name=%s enable=%d\n", dump_file->name, dump_file->dump_enable);
    }

    return;
}


void parse_debug()
{
    FILE *file_fd = NULL;
    int item = 0;
    int i = 0;
    int len = 0;
    file_fd = fopen(DEBUG_FILE_NAME, "r");
    if (file_fd == NULL) {
        return;
    }
    memset(buffer, 0, BUF_LEN);
    len = fread(buffer, 1, 128, file_fd);
    if (len <= 0) {
        fclose(file_fd);
        return;
    }

    item = sizeof(debugitems) / sizeof(debug_item_t);
    for (i = 0; i < item; i++) {
        if (!strncmp(debugitems[i].name, buffer, strlen(debugitems[i].name))) {
            debugitems[i].action(buffer);
        }
    }
    fclose(file_fd);
    return;
}


void *aml_log_threadloop(void *data)
{
    aml_audio_log_t * log_handle = (aml_audio_log_t *)data;
    FILE *file_fd = NULL;
    int len = 0;
    unsigned int watch_flag = IN_CLOSE_WRITE | IN_CREATE | IN_DELETE
                              | IN_DELETE_SELF | IN_MOVE | IN_MOVE_SELF | IN_IGNORED;

    prctl(PR_SET_NAME, (unsigned long)"aml_audio_log");
    file_fd = fopen(DEBUG_FILE_NAME, "a+");
    if (file_fd) {
        fclose(file_fd);
        parse_debug();
    }

    while (!log_handle->bExit) {
        log_handle->watch_desc = inotify_add_watch(log_handle->notify_fd , DEBUG_FILE_NAME , watch_flag);
        len = read(log_handle->notify_fd, buffer, BUF_LEN);
        if (len < 0) {
            printf("read error\n");
            usleep(10 * 1000);
            continue;
        }

        if (log_handle->bExit) {
            break;
        }

        parse_debug();

        usleep(100 * 1000);
    }
    return ((void *) 0);
}



void aml_log_init(void)
{
    aml_log_handle = calloc(1, sizeof(aml_audio_log_t));
    if (aml_log_handle == NULL) {
        return;
    }
    list_init(&aml_log_handle->dumpinfo_list);
    list_init(&aml_log_handle->dumpfile_list);

    aml_log_handle->bExit = 0;
    aml_log_handle->notify_fd = inotify_init();
    pthread_create(&(aml_log_handle->threadid), NULL,
                   &aml_log_threadloop, aml_log_handle);

    return;
}

void aml_log_exit(void)
{
    struct listnode *node = NULL;
    aml_log_dump_info_t *dump_info;
    aml_log_dump_file_t *dump_file;

    aml_log_handle->bExit = 1;
    inotify_rm_watch(aml_log_handle->notify_fd, aml_log_handle->watch_desc);
    pthread_join(aml_log_handle->threadid, NULL);
    close(aml_log_handle->notify_fd);


    // release the list resources
    list_for_each(node, &aml_log_handle->dumpinfo_list) {
        dump_info = node_to_item(node, aml_log_dump_info_t, listnode);
        printf("remove dump item =%s\n", dump_info->name);
        list_remove(&dump_info->listnode);
        free(dump_info);
        node = &aml_log_handle->dumpinfo_list;
    }

    list_for_each(node, &aml_log_handle->dumpfile_list) {
        dump_file = node_to_item(node, aml_log_dump_file_t, listnode);
        printf("remove dump item =%s\n", dump_file->name);
        list_remove(&dump_file->listnode);
        free(dump_file);
        node = &aml_log_handle->dumpfile_list;
    }



    free(aml_log_handle);
    aml_log_handle = NULL;
    printf("Exit %s\n", __func__);
    return;
}


void aml_log_dumpinfo_install(char * name, dump_info_func_t dump_func, void * private)
{
    struct listnode *node = NULL;
    aml_log_dump_info_t *dump_info;
    int bInstalled = 0;
    list_for_each(node, &aml_log_handle->dumpinfo_list) {
        dump_info = node_to_item(node, aml_log_dump_info_t, listnode);
        if (!strncmp(dump_info->name, name, strlen(name))) {
            //printf("%s name =%s is installed\n", __func__, name);
            bInstalled = 1;
            break;
        }
    }

    if (bInstalled == 0) {
        dump_info = calloc(1, sizeof(aml_log_dump_info_t));
        memcpy(dump_info->name, name, strlen(name));
        dump_info->dump_func = dump_func;
        dump_info->private_date = private;
        bInstalled = 1;
        //printf("%s: %s\n", __func__, name);
        list_add_head(&aml_log_handle->dumpinfo_list, &dump_info->listnode);
    }

    return;
}

void aml_log_dumpinfo_remove(char * name)
{
    struct listnode *node = NULL;
    aml_log_dump_info_t *dump_info;

    list_for_each(node, &aml_log_handle->dumpinfo_list) {
        dump_info = node_to_item(node, aml_log_dump_info_t, listnode);
        if (!strncmp(dump_info->name, name, strlen(name))) {
            list_remove(&dump_info->listnode);
            //printf("%s: %s\n", __func__, name);
            free(dump_info);
            break;
        }
    }
    return;
}

int aml_log_get_dumpfile_enable(char * name)
{
    struct listnode *node = NULL;
    aml_log_dump_file_t *dump_file;

    list_for_each(node, &aml_log_handle->dumpfile_list) {
        dump_file = node_to_item(node, aml_log_dump_file_t, listnode);
        if (!strncmp(dump_file->name, name, strlen(name))) {
            return dump_file->dump_enable;
        }
    }

    return 0;
}

