/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
 */
#ifndef _SOCKET_IPC_H__
#define _SOCKET_IPC_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/select.h>
#include <errno.h>
#include <json.h>
#include <stdio.h>
#include <pthread.h>


#define SOCKET_PATH "/tmp/0x1socket_ipc/"
#define SERVER_SOCKET_PATH(c, in) sprintf(c, SOCKET_PATH"0x1socket_ipc_server_%s", in)
#define CLIENT_SOCKET_PATH(c, in) sprintf(c, SOCKET_PATH"0x1socket_ipc_client_%s%d", in, getpid())
#define MAX_CONNECTOR 50

#if DEBUG
#include <time.h>
#define COLOR_F (getpid()%6)+1
#define COLOR_B 8
#define DEBUG_INFO(fmt, arg...) do {fprintf(stderr, "[Debug:PID[%5d]:%8ld]\033[3%d;4%dm " fmt "\033[0m [in %s   %s:%d]\n",getpid(), time(NULL), COLOR_F, COLOR_B, ##arg, __FILE__, __func__, __LINE__);}while(0)
#else
#define DEBUG_INFO(fmt, arg...)
#endif //DEBUG


/* The ipc server runing status*/
typedef enum {
	SERVER_INITED,
	SERVER_LISTENING,
	SERVER_GOINGTOSTOP,
	SERVER_STOPED,
} server_status;

/* The ipc message handle
 *     data_in: the json message input, it not be NULL, the json object ref should
 * be decrease use json_object_put
 *     data_out: address of the output json reply should hold at data_out, if let
 * *data_out to be NULL, it means no need replay action.
 */
typedef void (*message_handle)(json_object* data_in, json_object** data_out);

/* The struct of server context
 *    type: it save the type of server which used on server_create.socket
 * path relate with it.
 *    setup_sfd: the server socket fd
 *    socket_set: the fdset which set with the server socket fd and the peer socket fd
 *    connector_count: the peer socket fd count
 *    sfd: the peer socket fds, the empty should hold with -1,the position after
 * connector_count-1 should be -1.
 *    max_fd: it hold the max fd, which used by select.
 *    status: it should be volatile, and set just on one thread beside of
 * server thread,Use to stop the server routine.
 *    msg_handle: the msg handle function. see message_handle.
 */
typedef struct _server_ctx {
    char* name;
    char type[64];
	int setup_sfd;
	fd_set socket_set;
	int connector_count;
	int sfd[MAX_CONNECTOR];
	int max_fd;
	volatile server_status status;
    pthread_t thread_id;
    message_handle msg_handle;
} server_ctx;

typedef struct _client_ctx {
    char* name;
    char type[64];
	int fd;
    message_handle msg_handle;
} client_ctx;


/* Create server socket with the "type" name. */
server_ctx* server_create(const char* type);

/* Register an handle into the socket server thread, it should
   call before server_start */
int server_register_handle(server_ctx* ctx, message_handle handle);

/* Start a new thread to read the socket message
   return 0 if success
   */
int server_start(server_ctx* server);

/* It should used after server_create */
int server_destory(server_ctx* ctx);


/* Create client socket with the "type" name,connect to "type" named server */
client_ctx* client_create(const char* type);

/* Reservation */
int client_register_handle(client_ctx* ctx, message_handle handle);

/* It should used after client_create */
int client_destory(client_ctx* ctx);

/* Client send a json'ptr at *data ,and wait for server reply, the reply is
 * a new json, it ptr will story at data. the send json object will be consumed,
 * by json_object_put, the new json reply need you consume use json_object_put
 */
int client_send_request_wait_reply(client_ctx* ctx, json_object** data);

/* Client send a json, the send json will be consume by json_object_put */
int client_send_request(client_ctx* ctx, json_object* data);


#ifdef __cplusplus
}
#endif

#endif//_SOCKET_IPC_H__
