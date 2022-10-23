#ifndef _CTL_SOCKET_H
#define _CTL_SOCKET_H

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <sys/stat.h>

#define CLIENT_FD_MAX 5
typedef struct{
    int server_sockfd;
    int client_sockfd[CLIENT_FD_MAX];
    int server_len;
    int client_len;
    struct sockaddr_un server_address;
    struct sockaddr_un client_address;
    char sock_path[64];
	int client_seq;
} tAPP_SOCKET;



/********************SERVER API***************************/

tAPP_SOCKET * setup_socket_server(const char *path);
int accept_client(tAPP_SOCKET *app_socket);
void teardown_socket_server(tAPP_SOCKET *app_socket);

void send_all_client(tAPP_SOCKET *app_socket, char *msg, int len);
void shutdown_all_client(tAPP_SOCKET *app_socket);
void close_all_clent(tAPP_SOCKET *app_socket);
void remove_one_client(tAPP_SOCKET *app_socket, int fd);
void remove_all_client(tAPP_SOCKET *app_socket);

/********************CLIENT API***************************/
int setup_socket_client(char *socket_path);
void teardown_socket_client(int sockfd);
#endif



