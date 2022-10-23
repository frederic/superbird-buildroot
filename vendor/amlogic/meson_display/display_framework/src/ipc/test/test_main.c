/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
 */
#include "ipc.h"
#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/wait.h>

#define SOCKET_TYPE "test1"

#ifdef __cplusplus
extern "C" {
#endif

static void* client_process(void) {
    DEBUG_INFO("Start client");
    client_ctx * client = NULL;
    do {
        DEBUG_INFO("Connect with Server");
        client = client_create(SOCKET_TYPE);
    } while (client == NULL);
    json_object* data = json_object_new_object();
    json_object_object_add(data, "cmd", json_object_new_string("get modes"));
    if (client_send_request_wait_reply(client, &data) < 0) {
        DEBUG_INFO("Server disconnected");
    } else {
        DEBUG_INFO("Get mode :%s", json_object_to_json_string(data));
    }
    json_object_put(data);

    data = json_object_new_object();
    json_object_object_add(data, "cmd", json_object_new_string("set mode"));
    json_object_object_add(data, "value", json_object_new_string("1280x720@60"));
    DEBUG_INFO("Set mode to 1280x720@60");
    client_send_request(client, data);

    int i;
    for (i = 0; i < 2; i++) {
        client = client_create(SOCKET_TYPE);
        if (client == NULL) {
            DEBUG_INFO("client2 create failed with error: %d", errno);
            return NULL;
        }
        json_object* data = json_object_new_object();
        json_object_object_add(data, "cmd", json_object_new_string("get modes"));
        if (client_send_request_wait_reply(client, &data) < 0) {
            DEBUG_INFO("Server disconnected");
        } else {
            DEBUG_INFO("Get mode :%s", json_object_to_json_string(data));
        }
        json_object_put(data);
    }

    client_destory(client);
    DEBUG_INFO("client exit");
    return NULL;

}


void m_message_handle(json_object* data_in, json_object** data_out) {
    json_object* tmp;
    if (!json_object_object_get_ex(data_in, "cmd", &tmp)) {
        DEBUG_INFO("No cmd sended!");
    } else {
        //cmd's buffer under the json object memory
        const char* cmd = json_object_get_string(tmp);
        if (0 == strcmp("set mode", cmd)) {
            if (!json_object_object_get_ex(data_in, "value", &tmp)) {
                DEBUG_INFO("value can't accept!");
            } else {
                const char* mode = json_object_get_string(tmp);

                DEBUG_INFO("Set mode to:%s", mode);
            }

        } else if (0 == strcmp("get modes", cmd)) {
            *data_out = json_tokener_parse("{'value':'1080x720@60',\n'value1':'1920x1280@90'}");
        }
    }
    json_object_put(data_in);

}

void* server_process(void* ppctx) {
    server_ctx* server = NULL;
    DEBUG_INFO("Start server");
    server = server_create(SOCKET_TYPE);
    if (server == NULL) {
        DEBUG_INFO("server create failed with error: %s", strerror(errno));
        return NULL;
    }

    *(server_ctx**)ppctx = server;
    //Set server msg handle
    server_register_handle(server, m_message_handle);
    server_start(server);
}

int main(int argc, char* argv[]) {
    int ret;
    pid_t pid;
    pid = fork();
    server_ctx* server = NULL;
    if (pid == -1) {
        DEBUG_INFO("Fork failed");
        return -1;
    }
    if (pid == 0) {
        client_process();
    } else {
        server_process(&server);
        wait(NULL);
        server_destory(server);
        return 0;
    }
}
#ifdef __cplusplus
}
#endif
