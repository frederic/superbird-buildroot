/*****************************************************************************
 **
 **  Name:           app_socket.h
 **
 **  Description:    Bluetooth Manager application
 **
 **  Copyright (c) 2010-2013, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

/* idempotency */
#ifndef APP_SOCKET_H
#define APP_SOCKET_H

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <sys/stat.h>

/* self-sufficiency */
#include "bsa_status.h"

typedef struct {
	int server_sockfd;
	int client_sockfd[10];
	int client_num;
	int server_len;
	int client_len;
	struct sockaddr_un server_address;
	struct sockaddr_un client_address;
	char sock_path[64];
} tAPP_SOCKET;



/********************SERVER API***************************/

int setup_socket_server(tAPP_SOCKET *app_socket);
int accpet_client(tAPP_SOCKET *app_socket);
void teardown_socket_server(tAPP_SOCKET *app_socket);

/********************CLIENT API***************************/
int setup_socket_client(char *socket_path);
void teardown_socket_client(int sockfd);
/********************COMMON API***************************/
int socket_send(int sockfd, char *msg, int len);
int socket_recieve(int sockfd, char *msg, int len);
#endif



