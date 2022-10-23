#include <stdlib.h>
#include <unistd.h>

#include "shared/ctl-socket.h"
#include "shared/log.h"
#include <errno.h>


/********************SERVER API***************************/

tAPP_SOCKET * setup_socket_server(const char *path)
{
	tAPP_SOCKET *app_socket = calloc(1, sizeof(tAPP_SOCKET));

	if (!app_socket)
		return NULL;

	strcpy(app_socket->sock_path, path);

	unlink(path);
	if ((app_socket->server_sockfd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0) {
		error("fail to create socket: %s", strerror(errno));
		return NULL;
	}
	app_socket->server_address.sun_family = AF_UNIX;
	strncpy (app_socket->server_address.sun_path, app_socket->sock_path, sizeof(app_socket->server_address.sun_path));
	app_socket->server_len = sizeof (app_socket->server_address);
	app_socket->client_len = sizeof (app_socket->client_address);
	if ((bind (app_socket->server_sockfd, (struct sockaddr *)&app_socket->server_address, app_socket->server_len)) < 0) {
		error("bind: %s", strerror(errno));
		return NULL;

	}
	if (listen (app_socket->server_sockfd, 10) < 0) {
		error("listen: %s", strerror(errno));
		return NULL;
	}
	debug("Server is ready for client connect...\n");

	return app_socket;
}

int accept_client(tAPP_SOCKET *app_socket)
{
	int fd;
	fd = accept (app_socket->server_sockfd, (struct sockaddr *)&app_socket->server_address, (socklen_t *)&app_socket->client_len);


	if (fd == -1)
		error("accept: %s", strerror(errno));
	else {
		app_socket->client_sockfd[app_socket->client_seq] = fd;
		app_socket->client_seq++;

		app_socket->client_seq %= CLIENT_FD_MAX;
	}

	return fd;
}

void teardown_socket_server(tAPP_SOCKET *app_socket)
{
	unlink (app_socket->sock_path);
	free(app_socket);
}


void send_all_client(tAPP_SOCKET *app_socket, char *msg, int len)
{
	int i;
	for (i = 0 ; i < CLIENT_FD_MAX; i++) {
		if (app_socket->client_sockfd[i]) {
			debug("%s client fd%d", __func__, app_socket->client_sockfd[i]);
			send(app_socket->client_sockfd[i], msg, len, 0);
		}
	}
}

void shutdown_all_client(tAPP_SOCKET *app_socket)
{
	int i;
	for (i = 0 ; i < CLIENT_FD_MAX; i++) {
		if (app_socket->client_sockfd[i]) {
			debug("%s: shutingdown client fd%d", __func__, app_socket->client_sockfd[i]);
			shutdown(app_socket->client_sockfd[i], SHUT_RDWR);
		}
	}
}

void close_all_clent(tAPP_SOCKET *app_socket)
{
	int i;
	for (i = 0 ; i < CLIENT_FD_MAX; i++) {
		if (app_socket->client_sockfd[i]) {
			debug("%s: closing client fd%d", __func__, app_socket->client_sockfd[i]);
			close(app_socket->client_sockfd[i]);
		}
	}
}

void remove_one_client(tAPP_SOCKET *app_socket, int fd)
{
	int i;
	for (i = 0 ; i < CLIENT_FD_MAX; i++) {
		if (app_socket->client_sockfd[i] == fd) {
			debug("%s: remove client fd%d", __func__, app_socket->client_sockfd[i]);
			app_socket->client_sockfd[i] = 0;
		}
	}
}

void remove_all_client(tAPP_SOCKET *app_socket)
{
	int i;
	for (i = 0 ; i < CLIENT_FD_MAX; i++) {
		if (app_socket->client_sockfd[i]) {
			debug("%s: remove client fd%d", __func__, app_socket->client_sockfd[i]);
			app_socket->client_sockfd[i] = 0;
		}
	}
}

/********************CLIENT API***************************/
int setup_socket_client(char *socket_path)
{
	struct sockaddr_un address;
	int sockfd,  len;

	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		error("%s", strerror(errno));
		return -1;
	}

	address.sun_family = AF_UNIX;
	strncpy (address.sun_path, socket_path, sizeof(address.sun_path));
	len = sizeof (address);

	if (connect (sockfd, (struct sockaddr *)&address, len) == -1) {
		/*if socket is not avialable or not ready, don't report error*/
		if (errno != 111 && errno != 2)
			debug("connect server: %s", strerror(errno));
		close(sockfd);
		return -1;
	}

	return sockfd;

}

void teardown_socket_client(int sockfd)
{
	close(sockfd);
}


