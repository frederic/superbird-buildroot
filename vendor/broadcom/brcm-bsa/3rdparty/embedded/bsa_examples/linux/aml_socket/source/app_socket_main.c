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
#include <sys/socket.h>
#include <sys/un.h>




char socket_path[] = "/etc/bsa/config/";


/*******************************************************************************
 **
 ** Function         main
 **
 ** Description      This is the main function
 **
 ** Parameters      argc and argv
 **
 ** Returns          void
 **
 *******************************************************************************/
int main(int argc, char **argv)
{
	int fd, bytes;
	char server_socket[512];


	if (argc < 3)
		return 0;

	printf("send msg:%s to app_%s\n", argv[2], argv[1]);
	sprintf(server_socket, "%s%s", socket_path, argv[1]);
	fd = setup_socket_client(server_socket);
	if (fd < 0) {
		printf("fail to connect server socket\n");
		return 0;
	}
	bytes = socket_send(fd, argv[0], strlen(argv[0]));
	sleep(1);
	bytes = socket_send(fd, argv[2], strlen(argv[2]));
	teardown_socket_client(fd);
	return 0;
}

