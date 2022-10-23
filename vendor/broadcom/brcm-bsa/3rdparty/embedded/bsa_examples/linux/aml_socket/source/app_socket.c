/*****************************************************************************
 **
 **  Name:           app_opc_main.c
 **
 **  Description:    Bluetooth Manager application
 **
 **  Copyright (c) 2010-2013, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdlib.h>
#include <unistd.h>

#include "app_socket.h"
#include "app_utils.h"
#include "app_disc.h"
#include "app_mgt.h"
#include "app_xml_utils.h"



/********************SERVER API***************************/

int setup_socket_server(tAPP_SOCKET *app_socket)
{
	unlink (app_socket->sock_path);
	if ((app_socket->server_sockfd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0) {
		printf("fail to create socket\n");
		perror("socket");
		return -1;
	}
	app_socket->server_address.sun_family = AF_UNIX;
	strcpy (app_socket->server_address.sun_path, app_socket->sock_path);
	memset(app_socket->client_sockfd, 0, sizeof(app_socket->client_sockfd));
	app_socket->client_num = 0;
	app_socket->server_len = sizeof (app_socket->server_address);
	app_socket->client_len = sizeof (app_socket->client_address);
	if ((bind (app_socket->server_sockfd, (struct sockaddr *)&app_socket->server_address, app_socket->server_len)) < 0) {
		perror("bind");
		return -1;

	}
	if (listen (app_socket->server_sockfd, 10) < 0) {
		perror("listen");
		return -1;
	}
	printf ("Server is ready for client connect...\n");

	return 0;
}

int accpet_client(tAPP_SOCKET *app_socket)
{
	int sk = 0;

	sk = accept (app_socket->server_sockfd, (struct sockaddr *)&app_socket->server_address, (socklen_t *)&app_socket->client_len);
	if (sk == -1) {
		perror ("accept");
		return -1;
	}
	app_socket->client_sockfd[app_socket->client_num] = sk;
	app_socket->client_num++;

	return sk;
}

void teardown_socket_server(tAPP_SOCKET *app_socket)
{
	unlink (app_socket->sock_path);
	app_socket->server_sockfd = 0;
	app_socket->client_num = 0;
	memset(app_socket->client_sockfd, 0, sizeof(app_socket->client_sockfd));
}



/********************CLIENT API***************************/
int setup_socket_client(char *socket_path)
{
	struct sockaddr_un address;
	int sockfd,  len;

	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		printf("%s: can not creat socket\n", __func__);
		return -1;
	}

	address.sun_family = AF_UNIX;
	strcpy (address.sun_path, socket_path);
	len = sizeof (address);

	if (connect (sockfd, (struct sockaddr *)&address, len) == -1) {
		printf("%s: can not connect to socket\n", __func__);;
		return -1;
	}

	return sockfd;

}

void teardown_socket_client(int sockfd)
{
	close(sockfd);
}

/********************COMMON API***************************/
int socket_send(int sockfd, char *msg, int len)
{
	int bytes;
	if (sockfd < 0) {
		printf("%s: invalid sockfd\n", __func__);
		return -1;
	}
	if ((bytes = send(sockfd, msg, len, 0)) < 0) {
		perror ("send");
	}
	return bytes;
}

int socket_recieve(int sockfd, char *msg, int len)
{
	int bytes;

	if (sockfd < 0) {
		printf("%s: invalid sockfd\n", __func__);
		return -1;
	}

	if ((bytes = recv(sockfd, msg, len, 0)) < 0)
	{
		perror ("recv");
	}
	return bytes;

}


