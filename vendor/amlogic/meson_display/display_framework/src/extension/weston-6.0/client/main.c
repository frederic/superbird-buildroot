/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
 */
#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/wait.h>
#include <json.h>
#include "drm-help-client.h"
#include <stdio.h>
#include <getopt.h>
#include <string.h>


static const char* short_option = "";
static const struct option long_option[] = {
    {"list-modes", no_argument, 0, 'l'},
    {"chang-mode", required_argument, 0, 'c'},
    {"get-property", required_argument, 0, 'g'},
    {"set-property", required_argument, 0, 's'},
    {"raw-cmd", required_argument, 0, 'r'},
    {"G", required_argument, 0, 'G'},
    {"S", required_argument, 0, 'S'},
    {0, 0, 0, 0}
};

static void print_usage(const char* name) {
    printf("Usage: %s [-lcrgsGS]\n"
            "Get or change the mode setting of the weston drm output.\n"
            "Options:\n"
            "       -l,--list-modes        \tlist connector support modes\n"
            "       -c,--change-mode MODE  \tchange connector current mode, MODE format like:%%dx%%d@%%d width,height,refresh\n"
            "       -g,--get-connector-property \"PROPERTY\"\tget connector property\n"
            "       -s,--set-connector-property \"[Content Protection]\"=value\tset connector property\n"
            "       -G \"[ui-rect|display-mode]\"\tget [logic ui rect|display mode]\n"
            "       -S \"[ui-rect]\"\tset [logic ui rect]\n"
            "       -r,--raw-cmd           \tsend raw cmd\n", name);
}

static void list_modes(drm_client_ctx* client) {
    drm_connection_list* conn;
    conn = drm_help_client_get_connection(client);
    if (conn == NULL) {
        printf("Without connector\n");
        return;
    }
    drm_connection_list* curr_conn;
    for (curr_conn = conn; curr_conn != NULL; curr_conn = curr_conn->next) {
        printf("Connnection %d has %d modes:\n", curr_conn->id, curr_conn->count_modes);
        drm_output_mode_list* curr_mode;
        if (curr_conn->modes == NULL) {
            printf("This connector not connected\n");
        }
        for (curr_mode = curr_conn->modes; curr_mode != NULL; curr_mode = curr_mode->next) {
            printf("[%s] refresh %d\n", curr_mode->mode.name, curr_mode->mode.refresh);
        }
    }

    free_connection_list(conn);
}

int main(int argc, char* argv[]) {
    int ret = 0;
    drm_client_ctx* client;
    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }
    DEBUG_INFO("Start client");
    do {
        client = drm_help_client_create();
    } while (client == NULL);

    int opt;
    json_object* reply;
    while ((opt = getopt_long_only(argc, argv, short_option, long_option, NULL)) != -1) {
        switch (opt) {
            case 'l':
                list_modes(client);
                break;
            case 'c':
                drm_help_client_switch_mode_s(client, optarg, 0);
                break;
            case 'g':
                reply = send_cmd_sync(client, "get connector range properties", &optarg, OPT_TYPE_STRING);
                if (reply != NULL) {
                    printf("%s\n", json_object_to_json_string(reply));
                    json_object_put(reply);
                } else {
                    ret = -1;
                }
                break;
            case 's':
                {
                    char name[32] = {0};
                    uint64_t value = 0;
                    int count = 0;
                    count = sscanf(optarg, "%32[^=]=%"SCNu64, name, &value);
                    if (count == 2) {
                        drm_help_client_set_connector_properties(client, name, value);
                        DEBUG_INFO("Set %s = %"PRIu64, name ,value);
                    } else {
                        DEBUG_INFO("Set properties format error");
                        ret = -1;
                    }
                }
                break;
            case 'G':
                if (strcmp("ui-rect", optarg) == 0) {
                    drm_output_rect rect;
                    if (drm_help_client_get_ui_rect(client, &rect, 0)) {
                        printf("%"PRId32",%"PRId32",%"PRId32",%"PRId32"\n", rect.x, rect.y, rect.w, rect.h);
                    } else {
                        DEBUG_INFO("get ui-rect failed");
                        ret = -1;
                    }
                } else if (strcmp("display-mode", optarg) == 0) {
                    drm_output_mode mode;
                    if (drm_help_client_get_display_mode(client, &mode, 0)) {
                        printf("%s,%"PRId32",%"PRId32",%"PRIu32"\n", mode.name, mode.width, mode.height, mode.refresh);
                    } else {
                        ret = -1;
                    }
                } else {
                    DEBUG_INFO("cmd not recognized");
                    ret = -1;
                }
                break;
            case 'S':
                if (strcmp("ui-rect", optarg) == 0) {
                    if (optind + 4 > argc) {
                        DEBUG_INFO("miss rect parameter");
                        ret = -1;
                        break;
                    }
                    drm_output_rect rect;
                    rect.x = atoi(argv[optind]);
                    optind++;
                    rect.y = atoi(argv[optind]);
                    optind++;
                    rect.w = atoi(argv[optind]);
                    optind++;
                    rect.h = atoi(argv[optind]);
                    optind++;
                    DEBUG_INFO("set ui rect to %d,%d,%dx%d", rect.x, rect.y, rect.w, rect.h);
                    drm_help_client_set_ui_rect(client, &rect, 0);
                } else {
                    DEBUG_INFO("cmd not recognized");
                    ret = -1;
                }
                break;

            case 'r':
                if (strcmp("get ", optarg) == 0) {
                    reply = send_cmd_sync(client, optarg, NULL, OPT_TYPE_NULL);
                    if (reply != NULL) {
                        printf("%s\n", json_object_to_json_string(reply));
                        json_object_put(reply);
                    }
                } else {
                    send_cmd(client, optarg, NULL, OPT_TYPE_NULL);
                }
                break;
            default:
                print_usage(argv[0]);
        }
    };

    drm_help_client_destory(client);
    DEBUG_INFO("Exit client");
    return ret;
}
