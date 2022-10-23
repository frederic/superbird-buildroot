/*
 * *
 * * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 * *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License as published by
 * * the Free Software Foundation; either version 2 of the License, or
 * * (at your option) any later version.
 * *
 * * This program is distributed in the hope that it will be useful, but WITHOUT
 * * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * * more details.
 * *
 * * Auther：zhenhua.he
 * *
 * * Name: bt.c
 * *
 * * Describe: this file use for discovery bluetooth device
 * */


#include <stdio.h>
/* for exit */
#include <stdlib.h>
/* for sleep */
#include <unistd.h>
/* for strcpy */
#include <string.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "app_av.h"
#ifdef APP_AV_BCST_INCLUDED
#include "app_av_bcst.h"
#endif
//#include "app_utils.h"
#include "app_mgt.h"
#include "app_disc.h"
#include "app_xml_param.h"
#include "cJSON.h"

int discovery = 1;

/*discover callback*/
void  disc_callback(tBSA_DISC_EVT event, tBSA_DISC_MSG *p_data) {
    switch (event) {
        case BSA_DISC_CMPL_EVT:
            discovery = 0;
            break;
        default:
            break;
    }
   return;
}

/*bt management callback*/
static BOOLEAN mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data) {
    switch (event) {
        case BSA_MGT_DISCONNECT_EVT:
            exit(-1);
            break;
        default:
            break;
    }
    return FALSE;
}
/*init a discovery request*/
int bt_discover_init() {
    app_mgt_init();
    if (app_mgt_open("/etc/bsa/config/", mgt_callback) < 0) {
        return -1;
    }
    app_xml_init();
    app_disc_start_regular(disc_callback);

    app_disc_display_devices();
    return 0;
}

int main(int argc, char **argv) {

    int sfd = dup(STDOUT_FILENO),testfd;
    testfd = open("/tmp/bt.log",O_CREAT | O_RDWR | O_APPEND);
    if (-1 == testfd)
        return -1;
    int bddr_fd;
    bddr_fd = open("/tmp/bddr_fd", O_WRONLY|O_CREAT);
    int i;
    char *out;
    printf("Content-type: text/html;charset=utf-8\n\n");  //  BD_ADDR bd_addr;
    cJSON *root = cJSON_CreateObject();
    cJSON *array_bt = cJSON_CreateArray();
    if (-1 == dup2(testfd, STDOUT_FILENO))
        return -1;
    if ( bt_discover_init() < 0)
       return -1;
    while (discovery) {
        sleep(1);
    }

    for (i = 0; i < APP_DISC_NB_DEVICES; i++ )
    {
        if (strlen((char *)app_discovery_cb.devs[i].device.name) == 0)
            continue;
        char s[50];
        write(bddr_fd, &app_discovery_cb.devs[i].device.bd_addr[0],sizeof(UINT8) * 6);
        snprintf(s, 50, "%02x:%02x:%02x:%02x:%02x:%02x",
                app_discovery_cb.devs[i].device.bd_addr[0],
                app_discovery_cb.devs[i].device.bd_addr[1],
                app_discovery_cb.devs[i].device.bd_addr[2],
                app_discovery_cb.devs[i].device.bd_addr[3],
                app_discovery_cb.devs[i].device.bd_addr[4],
                app_discovery_cb.devs[i].device.bd_addr[5]);
        cJSON *obj_bt_device = cJSON_CreateObject();
        cJSON *item = cJSON_CreateString((char *)app_discovery_cb.devs[i].device.name);
        cJSON *bddr = cJSON_CreateString(s);
        cJSON_AddItemToObject(obj_bt_device,"name", item);
        cJSON_AddItemToObject(obj_bt_device,"bddr", bddr);
        cJSON_AddItemToArray(array_bt,obj_bt_device);
    }
    cJSON_AddItemToObject(root, "bt_list", array_bt);
    out = cJSON_Print(root);
    if (-1 == dup2(sfd, STDOUT_FILENO))
        return -1;
    printf("%s",out);
    cJSON_Delete(root);
    free(out);
    close(bddr_fd);
    close(testfd);
    return 0;
}
