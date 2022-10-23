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
 * * Name: Bt_con.c
 * *
 * * Describe: this File is for BT connect, get user input and tell BT server Which one to connect
 * */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>


struct msg_st{
    long int msg_type;
    int index;
};



/*init a massage queue ,  send the connect device index to Broadcom bluetooth server and lisetion server event ajax to user*/

int main(){
    int index = 0;
    printf("Content-type:text/html\n\n");
    char* input;
    input = getenv("QUERY_STRING");

    while (*input != '=') {
        input++;
    }

    input++;
    sscanf(input, "%d", &index);
    struct msg_st data;
    struct msg_st gt_data;
    int msgid = -1;
    int msgidg = -1;
    int msg_type = 0;

    msgid = msgget((key_t)1219, 0666 | IPC_CREAT);

    if (msgid == -1) {
        printf("{\"status\" : \"msg get error\" }");
        return -1;
    }

    data.index = index;

    if (msgsnd(msgid, (void*)&data, sizeof(int), 0) == -1) {
        printf("{\"status\" : \"msg send error\" }");
        return -1;
    }

    msgidg = msgget((key_t)1220, 0666 | IPC_CREAT);

    if (msgidg == -1) {
        printf ("{\"status\" : \"msgget error\" }");
        return -1;
    }

    if (msgrcv(msgidg, (void*)&gt_data, sizeof(int), msg_type, 0) == -1) {
        printf ("{\"status\" : \"msgrcv error\" }");
        return -1;
    }

    if (gt_data.index < 0) {
        printf ("{\"status\" : \"connect error\" }");
        return -1;
    }

    if (msgctl(msgidg, IPC_RMID, 0) == -1) {
        printf ("{\"status\" : \"msgctl error\" }");
        return -1;
    }

    /*success connect*/
    printf ("{\"status\" : \"ture\" }");

    return 0;
}
