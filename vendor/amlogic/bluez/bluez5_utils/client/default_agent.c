/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2012  Intel Corporation. All rights reserved.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/signalfd.h>


#include <glib.h>

#include "src/shared/mainloop.h"
#include "gdbus/gdbus.h"
#include "agent.h"

#define AGENT_PATH "/org/bluez/agent"
#define AGENT_INTERFACE "org.bluez.Agent1"

#define AGENT_PROMPT "defualt-agent: "
static gboolean agent_registered = FALSE;
static const char *agent_capability = NULL;
static DBusMessage *pending_message = NULL;
static const char *agent_option = "DisplayOnly";
static DBusConnection *dbus_conn;

static GMainLoop *main_loop;
static GDBusProxy *agent_manager;
static char *auto_register_agent = NULL;

static void agent_release_prompt(void)
{
	if (!pending_message)
		return;

}

dbus_bool_t agent_completion(void)
{
	if (!pending_message)
		return FALSE;

	return TRUE;
}

static void pincode_response(const char *input, void *user_data)
{
	DBusConnection *conn = user_data;

	g_dbus_send_reply(conn, pending_message, DBUS_TYPE_STRING, &input,
			DBUS_TYPE_INVALID);
}

static void passkey_response(const char *input, void *user_data)
{
	DBusConnection *conn = user_data;
	dbus_uint32_t passkey;

	if (sscanf(input, "%u", &passkey) == 1)
		g_dbus_send_reply(conn, pending_message, DBUS_TYPE_UINT32,
				&passkey, DBUS_TYPE_INVALID);
	else if (!strcmp(input, "no"))
		g_dbus_send_error(conn, pending_message,
				"org.bluez.Error.Rejected", NULL);
	else
		g_dbus_send_error(conn, pending_message,
				"org.bluez.Error.Canceled", NULL);
}

static void confirm_response(const char *input, void *user_data)
{
	DBusConnection *conn = user_data;

	if (!strcmp(input, "yes"))
		g_dbus_send_reply(conn, pending_message, DBUS_TYPE_INVALID);
	else if (!strcmp(input, "no"))
		g_dbus_send_error(conn, pending_message,
				"org.bluez.Error.Rejected", NULL);
	else
		g_dbus_send_error(conn, pending_message,
				"org.bluez.Error.Canceled", NULL);
}

static void agent_release(DBusConnection *conn)
{
	agent_registered = FALSE;
	agent_capability = NULL;

	if (pending_message) {
		dbus_message_unref(pending_message);
		pending_message = NULL;
	}

	agent_release_prompt();

	g_dbus_unregister_interface(conn, AGENT_PATH, AGENT_INTERFACE);
}

static DBusMessage *release_agent(DBusConnection *conn,
		DBusMessage *msg, void *user_data)
{
	printf("Agent released\n");

	agent_release(conn);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *request_pincode(DBusConnection *conn,
		DBusMessage *msg, void *user_data)
{
	const char *device;

	printf("Request PIN code\n");

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
			DBUS_TYPE_INVALID);

	pincode_response("1234", conn);

	pending_message = dbus_message_ref(msg);

	return NULL;
}

static DBusMessage *display_pincode(DBusConnection *conn,
		DBusMessage *msg, void *user_data)
{
	const char *device;
	const char *pincode;

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
			DBUS_TYPE_STRING, &pincode, DBUS_TYPE_INVALID);

	printf(AGENT_PROMPT "PIN code: %s\n", pincode);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *request_passkey(DBusConnection *conn,
		DBusMessage *msg, void *user_data)
{
	const char *device;

	printf("Request passkey\n");

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
			DBUS_TYPE_INVALID);


	pending_message = dbus_message_ref(msg);

	return NULL;
}

static DBusMessage *display_passkey(DBusConnection *conn,
		DBusMessage *msg, void *user_data)
{
	const char *device;
	dbus_uint32_t passkey;
	dbus_uint16_t entered;
	char passkey_full[7];

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
			DBUS_TYPE_UINT32, &passkey, DBUS_TYPE_UINT16, &entered,
			DBUS_TYPE_INVALID);

	snprintf(passkey_full, sizeof(passkey_full), "%.6u", passkey);
	passkey_full[6] = '\0';

	if (entered > strlen(passkey_full))
		entered = strlen(passkey_full);

	printf(AGENT_PROMPT "Passkey: "
			"%.*s  %s\n" ,
			entered, passkey_full, passkey_full + entered);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *request_confirmation(DBusConnection *conn,
		DBusMessage *msg, void *user_data)
{
	const char *device;
	dbus_uint32_t passkey;
	char *str = "yes";

	printf("Request confirmation\n");

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
			DBUS_TYPE_UINT32, &passkey, DBUS_TYPE_INVALID);
	pending_message = dbus_message_ref(msg);
	confirm_response(str, conn);

	//	pending_message = dbus_message_ref(msg);

	return NULL;
}

static DBusMessage *request_authorization(DBusConnection *conn,
		DBusMessage *msg, void *user_data)
{
	const char *device;
	char *str = "yes";

	printf("Request authorization\n");

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
			DBUS_TYPE_INVALID);
	pending_message = dbus_message_ref(msg);

	confirm_response(str, conn);



	return NULL;
}

static DBusMessage *authorize_service(DBusConnection *conn,
		DBusMessage *msg, void *user_data)
{
	const char *device, *uuid;
	char *str = "yes";

	printf("Authorize service\n");

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
			DBUS_TYPE_STRING, &uuid, DBUS_TYPE_INVALID);
	pending_message = dbus_message_ref(msg);
	confirm_response(str, conn);


	//pending_message = dbus_message_ref(msg);

	return NULL;
}

static DBusMessage *cancel_request(DBusConnection *conn,
		DBusMessage *msg, void *user_data)
{
	printf("Request canceled\n");

	agent_release_prompt();
	dbus_message_unref(pending_message);
	pending_message = NULL;

	return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable methods[] = {
	{ GDBUS_METHOD("Release", NULL, NULL, release_agent) },
	{ GDBUS_ASYNC_METHOD("RequestPinCode",
			GDBUS_ARGS({ "device", "o" }),
			GDBUS_ARGS({ "pincode", "s" }), request_pincode) },
	{ GDBUS_METHOD("DisplayPinCode",
			GDBUS_ARGS({ "device", "o" }, { "pincode", "s" }),
			NULL, display_pincode) },
	{ GDBUS_ASYNC_METHOD("RequestPasskey",
			GDBUS_ARGS({ "device", "o" }),
			GDBUS_ARGS({ "passkey", "u" }), request_passkey) },
	{ GDBUS_METHOD("DisplayPasskey",
			GDBUS_ARGS({ "device", "o" }, { "passkey", "u" },
				{ "entered", "q" }),
			NULL, display_passkey) },
	{ GDBUS_ASYNC_METHOD("RequestConfirmation",
			GDBUS_ARGS({ "device", "o" }, { "passkey", "u" }),
			NULL, request_confirmation) },
	{ GDBUS_ASYNC_METHOD("RequestAuthorization",
			GDBUS_ARGS({ "device", "o" }),
			NULL, request_authorization) },
	{ GDBUS_ASYNC_METHOD("AuthorizeService",
			GDBUS_ARGS({ "device", "o" }, { "uuid", "s" }),
			NULL,  authorize_service) },
	{ GDBUS_METHOD("Cancel", NULL, NULL, cancel_request) },
	{ }
};

static void register_agent_setup(DBusMessageIter *iter, void *user_data)
{
	const char *path = AGENT_PATH;
	const char *capability = agent_capability;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &capability);
}

static void register_agent_reply(DBusMessage *message, void *user_data)
{
	DBusConnection *conn = user_data;
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == FALSE) {
		agent_registered = TRUE;
		printf("Agent registered\n");
		agent_default(dbus_conn, agent_manager);
	} else {
		printf("Failed to register agent: %s\n", error.name);
		dbus_error_free(&error);

		if (g_dbus_unregister_interface(conn, AGENT_PATH,
					AGENT_INTERFACE) == FALSE)
			printf("Failed to unregister agent object\n");
	}
}

void agent_register(DBusConnection *conn, GDBusProxy *manager,
		const char *capability)

{
	if (agent_registered == TRUE) {
		printf("Agent is already registered\n");
		return;
	}

	agent_capability = capability;

	if (g_dbus_register_interface(conn, AGENT_PATH,
				AGENT_INTERFACE, methods,
				NULL, NULL, NULL, NULL) == FALSE) {
		printf("Failed to register agent object\n");
		return;
	}

	if (g_dbus_proxy_method_call(manager, "RegisterAgent",
				register_agent_setup,
				register_agent_reply,
				conn, NULL) == FALSE) {
		printf("Failed to call register agent method\n");
		return;
	}

	agent_capability = NULL;
}

static void unregister_agent_setup(DBusMessageIter *iter, void *user_data)
{
	const char *path = AGENT_PATH;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
}

static void unregister_agent_reply(DBusMessage *message, void *user_data)
{
	DBusConnection *conn = user_data;
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == FALSE) {
		printf("Agent unregistered\n");
		agent_release(conn);
	} else {
		printf("Failed to unregister agent: %s\n", error.name);
		dbus_error_free(&error);
	}
}

void agent_unregister(DBusConnection *conn, GDBusProxy *manager)
{
	if (agent_registered == FALSE) {
		printf("No agent is registered\n");
		return;
	}

	if (!manager) {
		printf("Agent unregistered\n");
		agent_release(conn);
		return;
	}

	if (g_dbus_proxy_method_call(manager, "UnregisterAgent",
				unregister_agent_setup,
				unregister_agent_reply,
				conn, NULL) == FALSE) {
		printf("Failed to call unregister agent method\n");
		return;
	}
}

static void request_default_setup(DBusMessageIter *iter, void *user_data)
{
	const char *path = AGENT_PATH;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
}

static void request_default_reply(DBusMessage *message, void *user_data)
{
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		printf("Failed to request default agent: %s\n",
				error.name);
		dbus_error_free(&error);
		return;
	}

	printf("Default agent request successful\n");

	return;
}

void agent_default(DBusConnection *conn, GDBusProxy *manager)
{
	printf("%s enter\n", __func__);
	if (agent_registered == FALSE) {
		printf("No agent is registered\n");
		return;
	}

	if (g_dbus_proxy_method_call(manager, "RequestDefaultAgent",
				request_default_setup,
				request_default_reply,
				NULL, NULL) == FALSE) {
		printf("Failed to call RequestDefaultAgent method\n");
		return;
	}
}


static void proxy_added(GDBusProxy *proxy, void *user_data)
{
	const char *interface;

	interface = g_dbus_proxy_get_interface(proxy);
	if (!strcmp(interface, "org.bluez.AgentManager1")) {
		if (!agent_manager) {
			agent_manager = proxy;

			if (auto_register_agent) {
				printf("register agent\n");
				agent_register(dbus_conn, agent_manager,
						auto_register_agent);
			}
		}

	}
}


static void proxy_removed(GDBusProxy *proxy, void *user_data)
{
	const char *interface;

	interface = g_dbus_proxy_get_interface(proxy);

	if (!strcmp(interface, "org.bluez.AgentManager1")) {
		if (agent_manager == proxy) {
			agent_manager = NULL;
			if (auto_register_agent)
				agent_unregister(dbus_conn, NULL);
		}
	}
}


static gboolean signal_handler(GIOChannel *channel, GIOCondition cond,
		gpointer user_data)
{
	static bool __terminated = false;
	struct signalfd_siginfo si;
	ssize_t result;
	int fd;

	if (cond & (G_IO_NVAL | G_IO_ERR | G_IO_HUP))
		return FALSE;

	fd = g_io_channel_unix_get_fd(channel);

	result = read(fd, &si, sizeof(si));
	if (result != sizeof(si))
		return FALSE;

	switch (si.ssi_signo) {
		case SIGINT:
		case SIGTERM:
			if (!__terminated) {
				printf("Terminating\n");
				g_main_loop_quit(main_loop);
			}

			__terminated = true;
			break;
	}

	return TRUE;
}

static guint setup_signalfd(void)
{
	GIOChannel *channel;
	guint source;
	sigset_t mask;
	int fd;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
		perror("Failed to set signal mask");
		return 0;
	}

	fd = signalfd(-1, &mask, 0);
	if (fd < 0) {
		perror("Failed to create signal descriptor");
		return 0;
	}

	channel = g_io_channel_unix_new(fd);

	g_io_channel_set_close_on_unref(channel, TRUE);
	g_io_channel_set_encoding(channel, NULL, NULL);
	g_io_channel_set_buffered(channel, FALSE);

	source = g_io_add_watch(channel,
			G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
			signal_handler, NULL);

	g_io_channel_unref(channel);

	return source;
}


int main(int argc, char *argv[])
{


	GDBusClient *client;
	guint signal;

	signal = setup_signalfd();
	if (signal == 0)
		return -errno;

	mainloop_init();

	if (agent_option)
		auto_register_agent = g_strdup(agent_option);
	else
		auto_register_agent = g_strdup("");

	dbus_conn = g_dbus_setup_bus(DBUS_BUS_SYSTEM, NULL, NULL);
	g_dbus_attach_object_manager(dbus_conn);

	main_loop = g_main_loop_new(NULL, FALSE);

	client = g_dbus_client_new(dbus_conn, "org.bluez", "/org/bluez");

	g_dbus_client_set_proxy_handlers(client, proxy_added, proxy_removed,
			NULL, NULL);

	g_main_loop_run(main_loop);

	g_dbus_client_unref(client);

	g_source_remove(signal);

	g_free(auto_register_agent);

	dbus_connection_unref(dbus_conn);

	return 0;

}
