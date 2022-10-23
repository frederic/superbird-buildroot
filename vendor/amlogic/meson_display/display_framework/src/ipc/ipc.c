/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
 */

#include "ipc.h"

typedef enum {
    SOCKET_LISTENER,
    SOCKET_NOLISTERNER,
} socket_type;

static int g_max_rcvbuf_size = 0;
static int g_max_sndbuf_size = 0;
#define MAX_REQUEST_BUFFER_SIZE 1024*1024*3


int socket_create(const char* pipe_name, int* socket_fd, socket_type type ) {
    int sfd,len;
    struct sockaddr_un addr,peer_addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    //Use block socket for reduce the function complication.
    //sfd = socket(AF_UNIX, SOCK_STREAM|SOCK_NONBLOCK, 0);
    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1) {
        DEBUG_INFO("socket create error:%s", strerror(errno));
        return -1;
    }
    if (g_max_rcvbuf_size == 0) {
        int max_rcvbuf_size = 0;
        int max_sndbuf_size = 0;
        socklen_t size = sizeof(int);
        if (!getsockopt(sfd, SOL_SOCKET, SO_RCVBUF, (void *)&max_rcvbuf_size, &size)) {
            g_max_rcvbuf_size = max_rcvbuf_size;
        }
        if (!getsockopt(sfd, SOL_SOCKET,SO_SNDBUF, (void *)&max_sndbuf_size, &size)) {
            g_max_sndbuf_size = max_sndbuf_size;
        }
    }
    //remve old file
    unlink(pipe_name);
    //make sure the path exists.
    if (-1 == access(SOCKET_PATH, F_OK|X_OK)) {
        if (-1 == mkdir(SOCKET_PATH, 0777)) {
            DEBUG_INFO("the socket dir create failed");
        }
    }

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, pipe_name, sizeof(addr.sun_path) - 1);
    if (bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        DEBUG_INFO("bind error:%s", strerror(errno));
        close(sfd);
        return errno;
    }
    if (type == SOCKET_LISTENER) {
        if (listen(sfd, MAX_CONNECTOR) == -1) {
            DEBUG_INFO("listen error:%s", strerror(errno));
            close(sfd);
            return errno;
        }
    }
    *socket_fd = sfd;
    return 0;
}

int socket_connect(int socket, const char* pipe_name) {
    struct sockaddr_un addr;
    int ret;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, pipe_name, sizeof(addr.sun_path) -1);
    ret = connect(socket, (struct sockaddr*) &addr, sizeof(struct sockaddr_un));
    if (ret == -1) {
        DEBUG_INFO("Socket(fd=%d) connect to %s error!%d:%s", socket, pipe_name, errno, strerror(errno));
    }
    return ret;
}

int socket_send(int socket, const char* buffer_in, int len) {
    int ret = write(socket, buffer_in, len);
    if (ret <= 0 ) {
        DEBUG_INFO("Socket write error(%d): %s", errno, strerror(errno));
    }
}

int socket_read(int socket, char* buffer, int len) {
    int ret;
    ret = read(socket, buffer, len);
    if (ret < 0) {
        DEBUG_INFO("Socket read(%d, 0x%p, %d) error(%d): %s", socket, buffer, len, errno, strerror(errno));
        return ret;
    }
    if (ret == 0) {
        DEBUG_INFO("Socket closed:%d", socket);
    }
    return ret;
}

int socket_accept(int socket, json_object* data) {
    struct sockaddr_un addr;
    int fd;
    socklen_t len = sizeof(addr);
    fd = accept(socket, (struct sockaddr*)&addr, &len);
    if (fd == -1) {
        DEBUG_INFO("socket_read error");
        return -1;
    }
    if (len > sizeof(addr)) {
        DEBUG_INFO("socket address truncated");
    }
    if (addr.sun_family != AF_UNIX) {
        DEBUG_INFO("socket family not AF_UNIX");
    }
    //DEBUG_INFO("connect comming form: %s",addr.sun_path);
    return fd;
}


int send_request(int fd, json_object* data) {
    int ret = 0;
    const char* str = json_object_to_json_string(data);
    ret = socket_send(fd, str, strlen(str));
    json_object_put(data);
    return ret;
}


int read_and_check_complete(int fd,char* buffer, char* pos, int buffer_len, json_object** data) {
    int nleft, ret;
    char* now_pos;
    nleft = buffer_len;
    now_pos = pos;
    while (nleft > 0) {
        ret = socket_read(fd, now_pos, nleft);
        if (ret < 0) {
            break;
        }
        *data = json_tokener_parse(buffer);
        if (*data != NULL || ret == 0) {
            break;
        }
        nleft -= ret;
        now_pos += ret;
    }
    return ret;
}

//ret 0:eos <=0:get error.
int read_request(int fd, json_object** data) {
    int max_cyc = MAX_REQUEST_BUFFER_SIZE / g_max_rcvbuf_size;
    char* buffer = NULL;
    int buffer_size = 0;
    char* pos = NULL;
    int ret;
    int i;
    for (i = 0; i < max_cyc; i++) {
        int old_size = buffer_size;
        buffer_size += g_max_rcvbuf_size;
        char* tmp = (char*)realloc(buffer, buffer_size);
        if (tmp == NULL) {
            DEBUG_INFO("Out of memory!");
            ret = -1;
            break;
        }
        buffer = tmp;
        pos = buffer + old_size;
        memset(pos, 0, g_max_rcvbuf_size);
        *data = NULL;
        ret = read_and_check_complete(fd, buffer, pos, g_max_rcvbuf_size, data);
        if (*data != NULL || ret <= 0 ) {
            break;
        }
    }
    free(buffer);

    if (*data == NULL) {
        *data = json_object_new_object();
        if (ret != 0) {
            DEBUG_INFO("Read request not complete!");
        }
        if (ret > 0) {
            ret = -1;
        }
    }

    return ret;
}


server_ctx* server_create(const char* type) {
    int server_socket = 0;
    char pip_name[126] = {0};
    server_ctx* ctx;

    ctx = (server_ctx*) malloc(sizeof(server_ctx));
    if (ctx == NULL) {
        return NULL;
    }

    ctx->name = NULL;
    strncpy(ctx->type, type, sizeof(ctx->type));
    SERVER_SOCKET_PATH(pip_name, ctx->type);
    int ret = socket_create(pip_name, &server_socket, SOCKET_LISTENER);
    if (ret != 0) {
        DEBUG_INFO("server create error:%s", strerror(errno));
        free(ctx);
        return NULL;
    }
    ctx->name = strdup(pip_name);
    ctx->setup_sfd = server_socket;
    FD_ZERO(&ctx->socket_set);
    FD_SET(server_socket, &ctx->socket_set);
    ctx->connector_count = 0;
    memset(&ctx->sfd, -1, sizeof(ctx->sfd));
    ctx->max_fd = server_socket;
    ctx->status = SERVER_INITED;
    ctx->thread_id = 0;
    ctx->msg_handle = NULL;
    return ctx;
}


static void* server_routine_start(void* arg) {
    server_ctx* ctx = (server_ctx*)arg;
    int ret = -1;
    int i;
    int count;
    fd_set set;
    ctx->status = SERVER_LISTENING;
    //Just read the socket request
    while (ctx->status == SERVER_LISTENING) {
        json_object* data_in;
        json_object* data_out;
        count = ctx->max_fd + 1;
        set = ctx->socket_set;
        ret = select(count, &set, NULL, NULL, NULL);
        if (ret == -1 || ret == 0) {
            DEBUG_INFO("listening get error:%s", strerror(errno));
            continue;
        }
        for (i = 0; i < count; i++) {
            if (FD_ISSET(i, &set)) {
                if (ctx->setup_sfd == i) {
                    data_in = json_object_new_object();
                    ret = socket_accept(ctx->setup_sfd, data_in);
                    if (ret != -1) {
                        DEBUG_INFO("Add client:(%d)", ret);
                        ctx->sfd[ctx->connector_count] = ret;
                        ctx->connector_count ++;
                        FD_SET(ret, &ctx->socket_set);
                        ctx->max_fd = ctx->max_fd > ret ? ctx->max_fd:ret;
                    }
                    json_object_put(data_in);
                } else {
                    //DEBUG_INFO("Get Event from:%d", i);
                    data_in = NULL;
                    ret = read_request(i, &data_in);
                    if (ret <= 0) {
                        //DEBUG_INFO("remove client:%d", i);
                        //the client maybe close when the read_request get 0
                        int max = 0;
                        int tmp;
                        FD_CLR(i, &ctx->socket_set);
                        close(i);
                        for (tmp = 0; tmp < ctx->connector_count; tmp++) {
                            if (ctx->sfd[tmp] == i) {
                                ctx->connector_count--;
                                ctx->sfd[tmp] = ctx->sfd[ctx->connector_count];
                                ctx->sfd[ctx->connector_count] = -1;
                            }
                            if (max < ctx->sfd[tmp]) {
                                max = ctx->sfd[tmp];
                            }
                        }
                        if (max < ctx->setup_sfd) {
                            max = ctx->setup_sfd;
                        }
                        ctx->max_fd = max;
                    }
                    if (json_object_object_length(data_in) > 0 && ctx->msg_handle) {
                        data_out = NULL;
                        DEBUG_INFO("Receive msg:%s", json_object_to_json_string(data_in));
                        ctx->msg_handle(data_in, &data_out);
                        if (data_out != NULL) {
                            DEBUG_INFO("Replay msg:%s", json_object_to_json_string(data_out));
                            send_request(i, data_out);
                        }
                    } else {
                        json_object_put(data_in);
                    }
                }
            }
        }
    }
    if (ctx->status == SERVER_GOINGTOSTOP) {
        for (i = 0; i < ctx->connector_count; i++) {
            close(ctx->sfd[i]);
            ctx->sfd[i] = -1;
        }
        ctx->connector_count = 0;
        ctx->status = SERVER_STOPED;
    }
    return NULL;
}


int server_start(server_ctx* ctx) {
    if (ctx == NULL) {
        return -1;
    }
    int ret = pthread_create(&(ctx->thread_id), NULL, server_routine_start, ctx);
    if (ret != 0) {
        ret = pthread_create(&(ctx->thread_id), NULL, server_routine_start, ctx);
    }
    return ret;
}

client_ctx* client_create(const char* type) {
    int client_socket = 0;
    char pip_name[126] = {0};
    client_ctx* ctx;
    ctx = (client_ctx*) malloc(sizeof(client_ctx));
    if (ctx == NULL) {
        return NULL;
    }
    ctx->name = NULL;
    strncpy(ctx->type, type, sizeof(ctx->type));
    CLIENT_SOCKET_PATH(pip_name, type);
    int ret = socket_create(pip_name, &client_socket, SOCKET_NOLISTERNER);
    if (ret != 0) {
        DEBUG_INFO("client create failed!");
        free(ctx);
        return NULL;
    }
    ctx->name = strdup(pip_name);
    //Connect with server socket
    SERVER_SOCKET_PATH(pip_name, ctx->type);
    ret = socket_connect(client_socket, pip_name);
    if (ret != 0) {
        DEBUG_INFO("client connect failed!");
        close(client_socket);
        free(ctx);
        return NULL;
    }
    ctx->fd = client_socket;
    ctx->msg_handle = NULL;
    DEBUG_INFO("client connect success!");

    return ctx;
}

int client_send_request(client_ctx* ctx, json_object* data) {
    return send_request(ctx->fd, data);
}

int client_send_request_wait_reply(client_ctx* ctx, json_object** data) {
    json_object* out;
    send_request(ctx->fd, *data);
    *data = json_object_new_object();
    int len = read_request(ctx->fd, &out);
    json_object_object_add(*data, "ret", out);
    return len;
}

int client_read_request(client_ctx* ctx, json_object** data) {
    return read_request(ctx->fd, data);
}

int server_register_handle(server_ctx* ctx, message_handle handle) {
    ctx->msg_handle = handle;
}

int client_register_handle(client_ctx* ctx, message_handle handle) {
    ctx->msg_handle = handle;
}

int server_destory(server_ctx* ctx) {
    if (ctx != NULL) {
        switch (ctx->status) {
            case SERVER_INITED:
                break;
            case SERVER_LISTENING:
                DEBUG_INFO("Destory IPC server");
                ctx->status = SERVER_GOINGTOSTOP;
                //The server thread may be blocked by select
                client_ctx* client = client_create(ctx->type);
                if (client == NULL) {
                    DEBUG_INFO("Error happen when destory IPC server!");
                }
                client_destory(client);
            case SERVER_GOINGTOSTOP:
                while (ctx->status != SERVER_STOPED) { };
            case SERVER_STOPED:
                 break;
            default:
                 DEBUG_INFO("IPC server status error");
        }
        if (ctx->setup_sfd != 0) {
            close(ctx->setup_sfd);
        }
        if (ctx->name != NULL) {
            unlink(ctx->name);
            free(ctx->name);
        }
        pthread_join(ctx->thread_id, NULL);

        free(ctx);
        DEBUG_INFO("IPC server destroy done");
    }

    return 0;
}

int client_destory(client_ctx* ctx) {
    if (ctx != NULL) {
        close(ctx->fd);
        if (ctx->name != NULL) {
            unlink(ctx->name);
            free(ctx->name);
        }
        free(ctx);
    }
    return 0;
}
